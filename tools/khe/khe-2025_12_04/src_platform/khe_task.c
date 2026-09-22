
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
/*  FILE:         khe_task.c                                                 */
/*  DESCRIPTION:  A task                                                     */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define USE_MAGIC 0
#if USE_MAGIC
#define MAGIC 2019573
#endif

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0
#define DEBUG9 0
#define DEBUG10 0
#define DEBUG11 0
#define DEBUG12 0
#define DEBUG13(task) (false && strcmp(KheTaskId(task), "3Sat:Night.4") == 0)

#define DEBUG_DOM 0
#define DEBUG_DOMAIN_MEET_ID "1Fri:12"
#define DEBUG_DOMAIN_TASK_INDEX 10
#define DEBUG_DOMAIN(task) (DEBUG_DOM && (task)->meet != NULL &&	\
  strcmp(KheEventId(KheMeetEvent((task)->meet)), DEBUG_DOMAIN_MEET_ID) == 0 && \
  (task)->meet_index == DEBUG_DOMAIN_TASK_INDEX)

#define DEBUG_VISITED 0
#define DEBUG_VISITED_MEET_ID "1Tue:D"
#define DEBUG_VISITED_TASK_INDEX 0
#define DEBUG_VISIT(task) (DEBUG_VISITED &&				  \
  strcmp(KheEventId(KheMeetEvent(task->meet)), DEBUG_VISITED_MEET_ID) == 0 \
  && task->meet_index == DEBUG_VISITED_TASK_INDEX)

#define DEBUG_TASK_ID "1Mon:Night.5"
#define DEBUG_TASK2_ID "1Tue:Night.6"
#define DEBUG_TASK(task) (false && (strcmp(KheTaskId(task), DEBUG_TASK_ID)==0 \
    || strcmp(KheTaskId(task), DEBUG_TASK2_ID)==0))


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK - a task                                                        */
/*                                                                           */
/*****************************************************************************/

struct khe_task_rec {
#if USE_MAGIC
  int				magic;			/* check is task     */
#endif
  void				*back;			/* back pointer      */
  char				*id;			/* Id                */
  KHE_SOLN			soln;			/* enclosing soln    */
  int				soln_index;		/* index in soln     */
  int				meet_index;		/* index in meet     */
  KHE_MEET			meet;			/* optional meet     */
  float				workload_per_time;	/* per unit time     */
  ARRAY_KHE_TASK_BOUND		task_bounds;		/* task bounds       */
  KHE_RESOURCE_GROUP		domain;			/* resource domain   */
  /* KHE_TASKING		tasking; */		/* optional tasking  */
  /* int			tasking_index; */	/* index in tasking  */
  bool				target_fixed;		/* target fixed      */
  KHE_TASK			target_task;		/* task assigned to  */
  ARRAY_KHE_TASK		assigned_tasks;		/* assigned to this  */
  int				visit_num;		/* visit number      */
  int				reference_count;	/* reference count   */
  ARRAY_KHE_ORDINARY_DEMAND_MONITOR all_monitors;	/* all demand mon's  */
  ARRAY_KHE_ORDINARY_DEMAND_MONITOR attached_monitors;	/* attached monitors */
  KHE_RESOURCE_IN_SOLN		assigned_rs;		/* assigned resource */
  int				assigned_index;		/* index in rs       */
  KHE_EVENT_RESOURCE_IN_SOLN	event_resource_in_soln;	/* optional er in s  */
  /* ARRAY_KHE_MONITOR		tmp_monitors; */	/* temp value        */
  KHE_TASK			copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "magic"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskCheckMagic(KHE_TASK task)                                    */
/*                                                                           */
/*  Check that the magic field is correct.                                   */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused, but ready to add if needed
void KheTaskCheckMagic(KHE_TASK task)
{
#if USE_MAGIC
  HnAssert(task == NULL || task->magic == MAGIC,
    "KheTaskCheckMagic internal error (wrong)");
#else
  HnAbort("KheTaskCheckMagic internal error (magic not in use)");
#endif
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "back pointers"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelSetBack(KHE_TASK task, void *back)                     */
/*                                                                           */
/*  Set the back pointer of task to back, assuming all is well.              */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelSetBack(KHE_TASK task, void *back)
{
  KheSolnOpTaskSetBack(task->soln, task, task->back, back);
  task->back = back;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelSetBackUndo(KHE_TASK task, void *old_back,             */
/*    void *new_back)                                                        */
/*                                                                           */
/*  Undo KheTaskKernelSetBack.                                               */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelSetBackUndo(KHE_TASK task, void *old_back, void *new_back)
{
  task->back = old_back;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetBack(KHE_TASK task, void *back)                           */
/*                                                                           */
/*  Set the back pointer of task.                                            */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetBack(KHE_TASK task, void *back)
{
  KheTaskKernelSetBack(task, back);
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheTaskBack(KHE_TASK task)                                         */
/*                                                                           */
/*  Return the back pointer of task.                                         */
/*                                                                           */
/*****************************************************************************/

void *KheTaskBack(KHE_TASK task)
{
  return task->back;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "visit numbers"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetVisitNum(KHE_TASK task, int num)                          */
/*                                                                           */
/*  Set the visit number of task.                                            */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetVisitNum(KHE_TASK task, int num)
{
  task->visit_num = num;
  if( DEBUG_VISIT(task) )
  {
    fprintf(stderr, "  KheTaskSetVisitNum(");
    KheTaskDebug(task, 1, -1, stderr);
    fprintf(stderr, ") setting visit_num to %d\n", task->visit_num);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskVisitNum(KHE_TASK task)                                       */
/*                                                                           */
/*  Return the visit number of task.                                         */
/*                                                                           */
/*****************************************************************************/

int KheTaskVisitNum(KHE_TASK task)
{
  if( DEBUG_VISIT(task) )
  {
    fprintf(stderr, "  KheTaskVisitNum(");
    KheTaskDebug(task, 1, -1, stderr);
    fprintf(stderr, ") returning visit_num %d\n", task->visit_num);
  }
  return task->visit_num;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskVisited(KHE_TASK task, int slack)                            */
/*                                                                           */
/*  Return true if task has been visited recently.                           */
/*                                                                           */
/*****************************************************************************/

bool KheTaskVisited(KHE_TASK task, int slack)
{
  if( DEBUG_VISIT(task) )
  {
    fprintf(stderr, "  KheTaskVisited(");
    KheTaskDebug(task, 1, -1, stderr);
    fprintf(stderr, ") returning (%d - %d <= %d) = %s\n",
      KheSolnGlobalVisitNum(KheTaskSoln(task)), task->visit_num, slack,
      (KheSolnGlobalVisitNum(KheTaskSoln(task)) - task->visit_num <= slack) ?
      "true" : "false");
  }
  return KheSolnGlobalVisitNum(KheTaskSoln(task)) - task->visit_num <= slack;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskVisit(KHE_TASK task)                                         */
/*                                                                           */
/*  Visit task.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheTaskVisit(KHE_TASK task)
{
  task->visit_num = KheSolnGlobalVisitNum(KheTaskSoln(task));
  if( DEBUG_VISIT(task) )
  {
    fprintf(stderr, "  KheTaskVisit(");
    KheTaskDebug(task, 1, -1, stderr);
    fprintf(stderr, ") setting visit_num to %d\n", task->visit_num);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskUnVisit(KHE_TASK task)                                       */
/*                                                                           */
/*  Unvisit task.                                                            */
/*                                                                           */
/*****************************************************************************/

void KheTaskUnVisit(KHE_TASK task)
{
  task->visit_num = KheSolnGlobalVisitNum(KheTaskSoln(task)) - 1;
  if( DEBUG_VISIT(task) )
  {
    fprintf(stderr, "  KheTaskUnVisit(");
    KheTaskDebug(task, 1, -1, stderr);
    fprintf(stderr, ") setting visit_num to %d\n", task->visit_num);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "other simple attributes"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheTaskMeet(KHE_TASK task)                                      */
/*                                                                           */
/*  Return the meet attribute of task.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_MEET KheTaskMeet(KHE_TASK task)
{
  return task->meet;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetMeet(KHE_TASK task, KHE_MEET meet)                        */
/*                                                                           */
/*  Set the meet attribute of task.                                          */
/*                                                                           */
/*  This is used internally when meets split and merge.  It has no effect    */
/*  on workload per time.                                                    */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetMeet(KHE_TASK task, KHE_MEET meet)
{
  task->meet = meet;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskMeetIndex(KHE_TASK task)                                      */
/*                                                                           */
/*  Return the index number of task in its meet.                             */
/*                                                                           */
/*****************************************************************************/

/* *** no use to outsiders
static int KheTaskMeetIndex(KHE_TASK task)
{
  return task->meet_index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetMeetIndex(KHE_TASK task, int meet_index)                  */
/*                                                                           */
/*  Set the index number of task in its meet to meet_index.                  */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetMeetIndex(KHE_TASK task, int meet_index)
{
  task->meet_index = meet_index;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskDuration(KHE_TASK task)                                       */
/*                                                                           */
/*  Return the duration of the meet containing task, or 0 if none.           */
/*                                                                           */
/*****************************************************************************/

int KheTaskDuration(KHE_TASK task)
{
  return task->meet == NULL ? 0 : KheMeetDuration(task->meet);
}


/*****************************************************************************/
/*                                                                           */
/*  float KheTaskWorkload(KHE_TASK task)                                     */
/*                                                                           */
/*  Return the workload of task, according to the formula                    */
/*                                                                           */
/*    Workload(task) = Duration(meet) * Workload(er) / Duration(e)           */
/*                                                                           */
/*  where meet is its meet, er is its event resource, and e is the           */
/*  enclosing event.                                                         */
/*                                                                           */
/*****************************************************************************/

float KheTaskWorkload(KHE_TASK task)
{
  KHE_EVENT_RESOURCE er;
  if( task->event_resource_in_soln == NULL )
    return 0.0;
  else
  {
    HnAssert(task->meet != NULL, "KheTaskWorkload internal error");
    er = KheEventResourceInSolnEventResource(task->event_resource_in_soln);
    return (float) KheMeetDuration(task->meet) * KheEventResourceWorkload(er) /
      (float) KheEventDuration(KheEventResourceEvent(er));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  float KheTaskWorkloadPerTime(KHE_TASK task)                              */
/*                                                                           */
/*  Return task's workload per time, i.e. workload(er) / duration(er) where  */
/*  er is the event resource that task is derived from.  If task is not      */
/*  derived from any event resource, its workload per time is 0.0.           */
/*                                                                           */
/*****************************************************************************/

float KheTaskWorkloadPerTime(KHE_TASK task)
{
  return task->workload_per_time;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheTaskSoln(KHE_TASK task)                                      */
/*                                                                           */
/*  Return the soln attribute of task.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheTaskSoln(KHE_TASK task)
{
  return task->soln;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskSolnIndex(KHE_TASK task)                                      */
/*                                                                           */
/*  Return the index number of task in its soln.                             */
/*                                                                           */
/*****************************************************************************/

int KheTaskSolnIndex(KHE_TASK task)
{
  return task->soln_index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetSolnIndex(KHE_TASK task, int num)                         */
/*                                                                           */
/*  Set the index number of task in its soln to num.                         */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetSolnIndex(KHE_TASK task, int soln_index)
{
  task->soln_index = soln_index;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TYPE KheTaskResourceType(KHE_TASK task)                     */
/*                                                                           */
/*  Return the resource type attribute of task.                              */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TYPE KheTaskResourceType(KHE_TASK task)
{
  return KheResourceGroupResourceType(task->domain);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE KheTaskEventResource(KHE_TASK task)                   */
/*                                                                           */
/*  Return the optional event resource of task.                              */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_RESOURCE KheTaskEventResource(KHE_TASK task)
{
  return task->event_resource_in_soln == NULL ? NULL :
    KheEventResourceInSolnEventResource(task->event_resource_in_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskIsPreassigned(KHE_TASK task, KHE_RESOURCE *r)                */
/*                                                                           */
/*  Return true if task is derived from a preassigned event resource,        */
/*  setting *r to that resource in that case.                                */
/*                                                                           */
/*****************************************************************************/

bool KheTaskIsPreassigned(KHE_TASK task, KHE_RESOURCE *r)
{
  KHE_EVENT_RESOURCE er;  KHE_RESOURCE res;
  er = KheTaskEventResource(task);
  if( er == NULL )
  {
    if( r != NULL )
      *r = NULL;
    return false;
  }
  res = KheEventResourcePreassignedResource(er);
  if( res == NULL )
  {
    if( r != NULL )
      *r = NULL;
    return false;
  }
  if( r != NULL )
    *r = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "creation and deletion"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskDoMake(HA_ARENA a)                                       */
/*                                                                           */
/*  Obtain a new task from a; initialize its arrays.                         */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK KheTaskDoMake(HA_ARENA a)
{
  KHE_TASK res;
  HaMake(res, a);
#if USE_MAGIC
  res->magic = MAGIC;
#endif
  HaArrayInit(res->task_bounds, a);
  HaArrayInit(res->assigned_tasks, a);
  HaArrayInit(res->all_monitors, a);
  HaArrayInit(res->attached_monitors, a);
  /* HaArrayInit(res->tmp_monitors, a); */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskUnMake(KHE_TASK task)                                        */
/*                                                                           */
/*  Undo KheTaskDoMake, returning task's memory to the memory allocator.     */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheTaskUnMake(KHE_TASK task)
{
  MArrayFree(task->task_bounds);
  MArrayFree(task->assigned_tasks);
  MArrayFree(task->all_monitors);
  MArrayFree(task->attached_monitors);
  MFree(task);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskDoGet(KHE_SOLN soln)                                     */
/*                                                                           */
/*  Get a task object, either from soln's free list or allocated.            */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheTaskDoGet(KHE_SOLN soln)
{
  KHE_TASK res;
  res = KheSolnGetTaskFromFreeList(soln);
  if( res == NULL )
    res = KheTaskDoMake(KheSolnArena(soln));
  res->reference_count = 0;
  if( DEBUG6 )
    fprintf(stderr, "KheTaskDoGet %p\n", (void *) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskUnGet(KHE_TASK task)                                         */
/*                                                                           */
/*  Undo KheTaskDoGet, adding task to the free list and clearing its arrays. */
/*                                                                           */
/*****************************************************************************/

static void KheTaskUnGet(KHE_TASK task)
{
  if( DEBUG6 )
    fprintf(stderr, "KheTaskUnGet %p\n", (void *) task);
  KheSolnAddTaskToFreeList(task->soln, task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskReferenceCountIncrement(KHE_TASK task)                       */
/*                                                                           */
/*  Increment task's reference count.                                        */
/*                                                                           */
/*****************************************************************************/

void KheTaskReferenceCountIncrement(KHE_TASK task)
{
  task->reference_count++;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskReferenceCountDecrement(KHE_TASK task)                       */
/*                                                                           */
/*  Decrement task's reference count, and possibly add it to the free list.  */
/*                                                                           */
/*****************************************************************************/

void KheTaskReferenceCountDecrement(KHE_TASK task)
{
  HnAssert(task->reference_count >= 1,
    "KheTaskReferenceCountDecrement internal error");
  if( --task->reference_count == 0 )
    KheTaskUnGet(task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoAdd(KHE_TASK task, KHE_SOLN soln, KHE_RESOURCE_TYPE rt,    */
/*    KHE_MEET meet, KHE_EVENT_RESOURCE er)                                  */
/*                                                                           */
/*  Initialize task (assuming its arrays are initialized, but possibly not   */
/*  empty, and its reference_count is initialized) and add it to soln.       */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoAdd(KHE_TASK task, KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_MEET meet, KHE_EVENT_RESOURCE er)
{
  /* int i;  KHE_EVENT e; */  KHE_RESOURCE r;  KHE_RESOURCE_IN_SOLN rs;
  /* KHE_MATCHING_DEMAND_CHUNK dc;  KHE_ORDINARY_DEMAND_MONITOR m; */

  task->back = NULL;
  task->id = NULL;
  task->soln = soln;
  KheSolnAddTask(soln, task);  /* sets soln->task_index too */
  if( meet != NULL )
  {
    KheMeetAddTask(meet, task, true);
  }
  else
  {
    task->meet = NULL;
    task->meet_index = -1;
  }
  HaArrayClear(task->task_bounds);
  task->domain = KheResourceTypeFullResourceGroup(rt);  /* may be adjusted */

  /* ***
  if( er != NULL && KheEventResourcePreassignedResource(er) != NULL )
    task->domain = KheResourceSingletonResourceGroup(
      KheEventResourcePreassignedResource(er));
  else
    task->domain = KheResourceTypeFullResourceGroup(rt);
  *** */
  if( DEBUG_DOMAIN(task) )
    KheTaskDebug(task, 3, 2, stderr);
  /* task->tasking = NULL; */
  /* task->tasking_index = -1; */
  task->target_fixed = false;
  task->target_task = NULL;
  HaArrayClear(task->assigned_tasks);
  task->visit_num = KheSolnGlobalVisitNum(soln);
  KheTaskReferenceCountIncrement(task);
  HaArrayClear(task->all_monitors);
  HaArrayClear(task->attached_monitors);
  /* ***
  if( KheSolnMatching(task->soln) != NULL )
    KheTaskMatchingBegin(task);
  *** */
  /* ***
  if( task->meet != NULL && KheSolnMatching(task->soln) != NULL )
    for( i = 0;  i < KheMeetDuration(task->meet);  i++ )
    {
      dc = KheMeetDemandChunk(task->meet, i);
      m = KheOrdinary DemandMonitorMake(task->soln, dc, task, i);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) task->soln,
	(KHE_MONITOR) m);
    }
  *** */
  task->assigned_rs = NULL;
  task->assigned_index = -1;
  if( er != NULL )
  {
    /* e = KheEventResourceEvent(er); */
    task->event_resource_in_soln = KheSolnEventResourceInSoln(soln, er);
    KheEventResourceInSolnAddTask(task->event_resource_in_soln, task);
    task->workload_per_time = KheEventResourceWorkloadPerTime(er);
    /* ***
    task->workload_per_time = (float) KheEventResourceWorkload(er) /
      (float) KheEventDuration(e);
    *** */
  }
  else
  {
    task->event_resource_in_soln = NULL;
    task->workload_per_time = 0.0;
  }
  /* HaArrayClear(task->tmp_monitors); */
  task->copy = NULL;
  /* *** this does not work (KheTaskId does not work properly here)
  if( DEBUG_TASK(task) )
    fprintf(stderr, "  KheTaskDoAdd(task \"%s\")\n", KheTaskId(task));
  *** */

  /* tighten domain if preassigned */
  if( er != NULL )
  {
    r = KheEventResourcePreassignedResource(er);
    if( r != NULL )
    {
      rs = KheSolnResourceInSoln(soln, r);
      KheTaskAddTaskBound(task, KheResourceInSolnSingletonTaskBound(rs));
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskUnAdd(KHE_TASK task)                                         */
/*                                                                           */
/*  Undo KheTaskDoAdd, leaving task unlinked from the solution.              */
/*                                                                           */
/*****************************************************************************/

static void KheTaskUnAdd(KHE_TASK task)
{
  /* remove task from any task bounds */
  while( HaArrayCount(task->task_bounds) > 0 )
    KheTaskDeleteTaskBound(task, HaArrayLast(task->task_bounds));

  /* inform task's event resource in soln */
  if( task->event_resource_in_soln != NULL )
    KheEventResourceInSolnDeleteTask(task->event_resource_in_soln, task);

  /* inform task's meet */
  if( task->meet != NULL )
    KheMeetDeleteTask(task->meet, task->meet_index);

  /* remove task's demand monitors */
  /* ***
  if( KheSolnMatching(task->soln) != NULL )
    KheTaskMatchingEnd(task);
  *** */
  while( HaArrayCount(task->all_monitors) > 0 )
    KheOrdinaryDemandMonitorDelete(HaArrayLast(task->all_monitors));

  /* delete from soln */
  KheSolnDeleteTask(task->soln, task);

  /* task is now not referenced from solution (this call may free task) */
  KheTaskReferenceCountDecrement(task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelAdd(KHE_TASK task, KHE_SOLN soln,                      */
/*    KHE_RESOURCE_TYPE rt, KHE_MEET meet, KHE_EVENT_RESOURCE er)            */
/*                                                                           */
/*  Kernel operation which adds task to soln (but does not make it).         */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelAdd(KHE_TASK task, KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_MEET meet, KHE_EVENT_RESOURCE er)
{
  KheTaskDoAdd(task, soln, rt, meet, er);
  KheSolnOpTaskAdd(soln, task, rt, meet, er);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelAddUndo(KHE_TASK task)                                 */
/*                                                                           */
/*  Undo KheTaskKernelAdd.                                                   */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelAddUndo(KHE_TASK task)
{
  KheTaskUnAdd(task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelDelete(KHE_TASK task)                                  */
/*                                                                           */
/*  Kernel operation which deletes task (but does not free it).              */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelDelete(KHE_TASK task)
{
  KheSolnOpTaskDelete(task->soln, task, KheTaskResourceType(task),
    task->meet, KheTaskEventResource(task));
  KheTaskUnAdd(task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelDeleteUndo(KHE_TASK task)                              */
/*                                                                           */
/*  Undo KheTaskKernelDelete.                                                */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelDeleteUndo(KHE_TASK task, KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_MEET meet, KHE_EVENT_RESOURCE er)
{
  KheTaskDoAdd(task, soln, rt, meet, er);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt, KHE_MEET meet, */
/*    KHE_EVENT_RESOURCE er)                                                 */
/*                                                                           */
/*  Make and return a new task with these attributes.  The first two are     */
/*  compulsory.  If meet is absent, then er must be absent too.              */
/*                                                                           */
/*****************************************************************************/
static void KheTaskMatchingCheckConsistency(KHE_TASK task);

KHE_TASK KheTaskMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt, KHE_MEET meet,
  KHE_EVENT_RESOURCE er)
{
  KHE_TASK res;  KHE_RESOURCE_GROUP rg;  KHE_TASK_BOUND tb;

  /* ensure parameters are legal */
  if( meet == NULL )
    HnAssert(er == NULL, "KheTaskMake: meet == NULL && er != NULL");
  if( er != NULL )
    HnAssert(KheMeetEvent(meet) == KheEventResourceEvent(er),
      "KheTaskMake: KheMeetEvent(meet) != KheEventResourceEvent(er)");

  /* make and initialize a new task object from scratch */
  res = KheTaskDoGet(soln);

  /* add it to the soln */
  KheTaskKernelAdd(res, soln, rt, meet, er);

  /* if its event resource is preassigned, add a singleton task bound */
  if( er != NULL && KheEventResourcePreassignedResource(er) != NULL )
  {
    rg = KheResourceSingletonResourceGroup(
      KheEventResourcePreassignedResource(er));
    tb = KheTaskBoundMake(soln, rg);
    if( !KheTaskAddTaskBound(res, tb) )
      HnAbort("KheTaskMake: cannot add preassignment task bound");
  }

  /* return it */
  KheTaskMatchingCheckConsistency(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheCycleTaskMake(KHE_SOLN soln, KHE_RESOURCE r)                 */
/*                                                                           */
/*  Make and return a cycle task for resource r.                             */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheCycleTaskMake(KHE_SOLN soln, KHE_RESOURCE r)
{
  KHE_TASK res;
  res = KheTaskMake(soln, KheResourceResourceType(r), NULL, NULL);
  res->assigned_rs = KheSolnResourceInSoln(soln, r);
  res->assigned_index = -1;  /* cycle task not added to assigned_rs */
  res->domain = KheResourceSingletonResourceGroup(r);
  if( DEBUG_DOMAIN(res) )
    KheTaskDebug(res, 3, 2, stderr);
  KheTaskAssignFix(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDelete(KHE_TASK task)                                        */
/*                                                                           */
/*  Delete task.  If freed, its memory goes on soln's task free list.        */
/*                                                                           */
/*****************************************************************************/

void KheTaskDelete(KHE_TASK task)
{
  KHE_TASK t2;

  /* remove task from its tasking, if any */
  if( DEBUG_TASK(task) )
    fprintf(stderr, "  KheTaskDelete(task \"%s\")\n", KheTaskId(task));
  /* ***
  if( task->tasking != NULL )
    KheTaskingDeleteTask(task->tasking, task);
  *** */

  /* unassign task, if assigned */
  if( task->target_task != NULL )
  {
    KheTaskAssignUnFix(task);
    KheTaskUnAssign(task);
  }

  /* unassign its child tasks */
  while( HaArrayCount(task->assigned_tasks) > 0 )
  {
    t2 = HaArrayLast(task->assigned_tasks);
    KheTaskAssignUnFix(t2);
    KheTaskUnAssign(t2);
  }

  /* delete the task bounds of task */
  while( HaArrayCount(task->task_bounds) > 0 )
    KheTaskBoundDelete(HaArrayLast(task->task_bounds));

  /* carry out the kernel delete operation (may free task) */
  KheTaskKernelDelete(task);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "copy"                                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskCopyPhase1(KHE_TASK task, HA_ARENA a)                    */
/*                                                                           */
/*  Carry out Phase 1 of the operation of copying task.                      */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheTaskCopyPhase1(KHE_TASK task, HA_ARENA a)
{
  KHE_TASK copy, child_task;  KHE_ORDINARY_DEMAND_MONITOR m;  int i;
  KHE_TASK_BOUND tb;
  if( task->copy == NULL )
  {
    HaMake(copy, a);
    task->copy = copy;
#if USE_MAGIC
    copy->magic = MAGIC;
#endif
    copy->back = task->back;
    copy->id = NULL;  /* will be regenerated later if required */
    copy->soln = KheSolnCopyPhase1(task->soln);
    copy->soln_index = task->soln_index;
    copy->meet = task->meet == NULL ? NULL : KheMeetCopyPhase1(task->meet, a);
    copy->workload_per_time = task->workload_per_time;
    copy->meet_index = task->meet_index;
    copy->target_fixed = task->target_fixed;
    HaArrayInit(copy->task_bounds, a);
    HaArrayForEach(task->task_bounds, tb, i)
      HaArrayAddLast(copy->task_bounds, KheTaskBoundCopyPhase1(tb, a));
    copy->domain = KheResourceGroupCopy(task->domain, a);  /* copy or share */
    if( DEBUG_DOMAIN(copy) )
      KheTaskDebug(copy, 3, 2, stderr);
    /* ***
    copy->tasking = task->tasking == NULL ? NULL :
      KheTaskingCopyPhase1(task->tasking, a);
    copy->tasking_index = task->tasking_index;
    *** */
    copy->target_task = task->target_task == NULL ? NULL :
      KheTaskCopyPhase1(task->target_task, a);
    if( DEBUG_TASK(task) )
      fprintf(stderr, "  KheTaskCopyPhase1(task \"%s\")\n", KheTaskId(task));
    HaArrayInit(copy->assigned_tasks, a);
    HaArrayForEach(task->assigned_tasks, child_task, i)
      HaArrayAddLast(copy->assigned_tasks, KheTaskCopyPhase1(child_task, a));
    copy->visit_num = task->visit_num;
    copy->reference_count = 1;  /* there are no paths, and copy is linked in */
    HaArrayInit(copy->all_monitors, a);
    HaArrayForEach(task->all_monitors, m, i)
      HaArrayAddLast(copy->all_monitors,
	KheOrdinaryDemandMonitorCopyPhase1(m, a));
    HaArrayInit(copy->attached_monitors, a);
    HaArrayForEach(task->attached_monitors, m, i)
      HaArrayAddLast(copy->attached_monitors,
	KheOrdinaryDemandMonitorCopyPhase1(m, a));
    copy->assigned_rs = task->assigned_rs == NULL ? NULL :
      KheResourceInSolnCopyPhase1(task->assigned_rs, a);
    copy->assigned_index = task->assigned_index;
    copy->event_resource_in_soln = task->event_resource_in_soln == NULL ? NULL :
      KheEventResourceInSolnCopyPhase1(task->event_resource_in_soln, a);
    /* HaArrayInit(copy->tmp_monitors, a); */
    copy->copy = NULL;
  }
  return task->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskCopyPhase2(KHE_TASK task)                                    */
/*                                                                           */
/*  Carry out Phase 2 of copying task.                                       */
/*                                                                           */
/*****************************************************************************/

void KheTaskCopyPhase2(KHE_TASK task)
{
  KHE_ORDINARY_DEMAND_MONITOR m;  int i;  KHE_TASK_BOUND tb;
  if( task->copy != NULL )
  {
    task->copy = NULL;
    HaArrayForEach(task->task_bounds, tb, i)
      KheTaskBoundCopyPhase2(tb);
    HaArrayForEach(task->all_monitors, m, i)
      KheOrdinaryDemandMonitorCopyPhase2(m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "domain calculations"                                          */
/*                                                                           */
/*  This private submodule collects together all the helper functions for    */
/*  calculating domains required at various places throughout this file.     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheTaskDomainWhenTaskBoundAdded(KHE_TASK task,        */
/*    KHE_TASK_BOUND tb)                                                     */
/*                                                                           */
/*  Return the domain that task would have if tb were added to it.           */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_GROUP KheTaskDomainWhenTaskBoundAdded(KHE_TASK task,
  KHE_TASK_BOUND tb)
{
  KHE_RESOURCE_GROUP res;
  if( DEBUG13(task) )
  {
    KHE_SOLN soln;  int i;  KHE_SOLN_GROUP soln_group;
    fprintf(stderr, "[ KheTaskDomainWhenTaskBoundAdded(%s from",
      KheTaskId(task));
    soln = KheTaskSoln(task);
    for( i = 0;  i < KheSolnSolnGroupCount(soln);  i++ )
    {
      soln_group = KheSolnSolnGroup(soln, i);
      fprintf(stderr, " %s", KheSolnGroupId(soln_group));
    }
    fprintf(stderr, ", tb)\n");
  }
  if( HaArrayCount(task->task_bounds) == 0 )
    res = KheTaskBoundResourceGroup(tb);
  /* ***
  else if( HaArrayCount(task->task_bounds) == 1 &&
	   HaArrayFirst(task->task_bounds) == tb )
    res = KheTaskBoundResourceGroup(tb);
  *** */
  else
  {
    KheSolnResourceGroupBegin(task->soln, KheTaskResourceType(task));
    KheSolnResourceGroupUnion(task->soln, task->domain);
    KheSolnResourceGroupIntersect(task->soln, KheTaskBoundResourceGroup(tb));
    res = KheSolnResourceGroupEnd(task->soln);
  }
  if( DEBUG13(task) )
  {
    fprintf(stderr, "] KheTaskDomainWhenTaskBoundAdded returning ");
    KheResourceGroupDebug(res, 2, 0, stderr);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheTaskDomainWhenTaskBoundDeleted(KHE_TASK task,      */
/*    int pos)                                                               */
/*                                                                           */
/*  Return the domain of task after deleting the task domain at pos.         */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_GROUP KheTaskDomainWhenTaskBoundDeleted(KHE_TASK task, int pos)
{
  enum { KHE_STATE_NONE, KHE_STATE_ONE, KHE_STATE_MANY } state;
  KHE_TASK_BOUND tb;  int i;  KHE_RESOURCE_GROUP res;
  state = KHE_STATE_NONE;
  res = NULL;  /* keep compiler happy */
  HaArrayForEach(task->task_bounds, tb, i) if( i != pos )
  {
    switch( state )
    {
      case KHE_STATE_NONE:

	res = KheTaskBoundResourceGroup(tb);
	state = KHE_STATE_ONE;
	break;

      case KHE_STATE_ONE:

	KheSolnResourceGroupBegin(task->soln, KheTaskResourceType(task));
	KheSolnResourceGroupUnion(task->soln, res);
	KheSolnResourceGroupIntersect(task->soln,KheTaskBoundResourceGroup(tb));
	state = KHE_STATE_MANY;
	break;

      case KHE_STATE_MANY:

	KheSolnResourceGroupIntersect(task->soln,KheTaskBoundResourceGroup(tb));
	break;
    }
  }
  if( state == KHE_STATE_NONE )
    res = KheResourceTypeFullResourceGroup(KheTaskResourceType(task));
  else if( state == KHE_STATE_MANY )
    res = KheSolnResourceGroupEnd(task->soln);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "task equivalence"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskEquivalent(KHE_TASK task1, KHE_TASK task2)                   */
/*                                                                           */
/*  Return true if task1 and task2 are equivalent, that is, if assigning     */
/*  task1 to a resource must have the same effect on its timetable, and      */
/*  on solution cost, as assigning task2.                                    */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
bool KheTaskEquivalent(KHE_TASK task1, KHE_TASK task2)
{
  KHE_EVENT_RESOURCE er1, er2;  KHE_MEET meet1, meet2;
  KHE_TASK child_task1, child_task2;  int i;

  ** if task1 is NULL, it depends on whether task2 is NULL **
  ** *** not doing this now
  if( task1 == NULL )
    return task2 == NULL;
  *** **

  ** if task2 is NULL, it depends on whether task1 is NULL (it isn't) **
  ** *** not doing this now
  if( task2 == NULL )
    return false;
  *** **

  ** must be assigned the same resource, or none **
  ** *** not doing this now
  if( incl_asst && KheTaskAsstResource(task1) != KheTaskAsstResource(task2) )
    return false;
  *** **

  ** must be derived from equivalent event resources **
  er1 = KheTaskEventResource(task1);
  er2 = KheTaskEventResource(task2);
  if( er1 == NULL || er2 == NULL || !KheEventResourceEquivalent(er1, er2) )
    return false;

  ** must have the same duration and the same starting time **
  meet1 = KheTaskMeet(task1);
  meet2 = KheTaskMeet(task2);
  if( KheMeetDuration(meet1) != KheMeetDuration(meet2) )
    return false;
  if( KheMeetAsstTime(meet1) != KheMeetAsstTime(meet2) )
    return false;

  ** must have the same domain **
  if( !KheResourceGroupEqual(KheTaskDomain(task1), KheTaskDomain(task2)) )
    return false;

  ** assigned tasks must be equivalent (no need to check their assignments) **
  ** we should really sort them first, but we don't **
  if( KheTaskAssignedToCount(task1) != KheTaskAssignedToCount(task2) )
    return false;
  for( i = 0;  i < KheTaskAssignedToCount(task1);  i++ )
  {
    child_task1 = KheTaskAssignedTo(task1, i);
    child_task2 = KheTaskAssignedTo(task2, i);
    if( !KheTaskEquivalent(child_task1, child_task2) )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "relation with enclosing tasking"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASKING KheTaskTasking(KHE_TASK task)                                */
/*                                                                           */
/*  Return the tasking containing task, or NULL if none.                     */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_TASKING KheTaskTasking(KHE_TASK task)
{
  return task->tasking;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetTasking(KHE_TASK task, KHE_TASKING tasking)               */
/*                                                                           */
/*  Set the tasking attribute of task.                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheTaskSetTasking(KHE_TASK task, KHE_TASKING tasking)
{
  task->tasking = tasking;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskTaskingIndex(KHE_TASK task)                                   */
/*                                                                           */
/*  Return task's index in the enclosing tasking, or -1 if none.             */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheTaskTaskingIndex(KHE_TASK task)
{
  return task->tasking_index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetTaskingIndex(KHE_TASK task, int tasking_index)            */
/*                                                                           */
/*  Set the tasking_index attributes of task.                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheTaskSetTaskingIndex(KHE_TASK task, int tasking_index)
{
  task->tasking_index = tasking_index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "demand monitor domains"                                       */
/*                                                                           */
/*  Implementation note.  The domains of the ordinary demand monitors of     */
/*  a task depend on the matching type, the domain of the task, the chain    */
/*  of assignments out of the task (defining its root task), and the         */
/*  domain of the root task.                                                 */
/*                                                                           */
/*  The precise dependence is given by KheTaskMatchingDomain immediately     */
/*  below.  The other functions in this submodule implement the changes      */
/*  required when one of the things that the domains depend on changes.      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheTaskMatchingDomain(KHE_TASK task)                  */
/*                                                                           */
/*  Return a suitable domain for the matching demand nodes of task.          */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheTaskMatchingDomain(KHE_TASK task)
{
  switch( KheSolnMatchingType(KheTaskSoln(task)) )
  {
    case KHE_MATCHING_TYPE_EVAL_INITIAL:
    case KHE_MATCHING_TYPE_EVAL_TIMES:

      return KheTaskDomain(task);

    case KHE_MATCHING_TYPE_SOLVE:
    case KHE_MATCHING_TYPE_EVAL_RESOURCES:

      return KheTaskDomain(KheTaskRoot(task));

    default:

      HnAbort("KheTaskMatchingDomain internal error");
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskMatchingReset(KHE_TASK task)                                 */
/*                                                                           */
/*  Reset the matching within task.                                          */
/*                                                                           */
/*  This function makes no attempt to move gracefully from one state to      */
/*  another; rather, it starts again from scratch.  This is useful for       */
/*  initializing, and also when the matching type changes, since in those    */
/*  cases a reset is the sensible way forward.                               */
/*                                                                           */
/*  This function is called separately for every task, so there is no need   */
/*  to worry about making recursive calls.                                   */
/*                                                                           */
/*****************************************************************************/

void KheTaskMatchingReset(KHE_TASK task)
{
  KHE_RESOURCE_GROUP rg;  int i;  KHE_ORDINARY_DEMAND_MONITOR m;
  if( HaArrayCount(task->attached_monitors) > 0 )
  {
    rg = KheTaskMatchingDomain(task);
    HaArrayForEach(task->attached_monitors, m, i)
      KheOrdinaryDemandMonitorSetDomain(m, rg,
	KHE_MATCHING_DOMAIN_CHANGE_TO_OTHER);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskMatchingDoSetDomain(KHE_TASK task, KHE_RESOURCE_GROUP rg,    */
/*    KHE_MATCHING_DOMAIN_CHANGE_TYPE change_type)                           */
/*                                                                           */
/*  Recursively set the domains of the demand monitors of task and its       */
/*  descendants to rg, with change_type saying what kind of change this is.  */
/*                                                                           */
/*****************************************************************************/

static void KheTaskMatchingDoSetDomain(KHE_TASK task, KHE_RESOURCE_GROUP rg,
  KHE_MATCHING_DOMAIN_CHANGE_TYPE change_type)
{
  KHE_ORDINARY_DEMAND_MONITOR m;  KHE_TASK child_task;  int i;
  if( DEBUG_TASK(task) )
    fprintf(stderr, "[ KheTaskMatchingDoSetDomain(%s, %s, %d)\n",
      KheTaskId(task), KheResourceGroupId(rg), (int) change_type);

  /* change the domains within task itself */
  HaArrayForEach(task->attached_monitors, m, i)
    KheOrdinaryDemandMonitorSetDomain(m, rg, change_type);

  /* change the domains within the followers */
  HaArrayForEach(task->assigned_tasks, child_task, i)
    KheTaskMatchingDoSetDomain(child_task, rg, change_type);
  if( DEBUG_TASK(task) )
    fprintf(stderr, "] KheTaskMatchingDoSetDomain\n");
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_DOMAIN_CHANGE_TYPE KheTaskChangeType(                       */
/*    KHE_RESOURCE_GROUP old_rg, KHE_RESOURCE_GROUP new_rg)                  */
/*                                                                           */
/*  Return the type of change when moving from old_rg to new_rg.             */
/*                                                                           */
/*****************************************************************************/

static KHE_MATCHING_DOMAIN_CHANGE_TYPE KheTaskChangeType(
  KHE_RESOURCE_GROUP old_rg, KHE_RESOURCE_GROUP new_rg)
{
  if( DEBUG2 )
  {
    fprintf(stderr, "calling KheTaskChangeType(");
    KheResourceGroupDebug(old_rg, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheResourceGroupDebug(new_rg, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }
  if( KheResourceGroupSubset(new_rg, old_rg) )
    return KHE_MATCHING_DOMAIN_CHANGE_TO_SUBSET;
  else if( KheResourceGroupSubset(old_rg, new_rg) )
    return KHE_MATCHING_DOMAIN_CHANGE_TO_SUPERSET;
  else
    return KHE_MATCHING_DOMAIN_CHANGE_TO_OTHER;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskMatchingSetDomain(KHE_TASK task, KHE_RESOURCE_GROUP rg)      */
/*                                                                           */
/*  Given that the domain of task is about to change to rg, update the       */
/*  matching in task appropriately, including updating the descendants       */
/*  of task if appropriate.                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheTaskMatchingSetDomain(KHE_TASK task, KHE_RESOURCE_GROUP rg)
{
  int i;  KHE_ORDINARY_DEMAND_MONITOR m;
  KHE_MATCHING_DOMAIN_CHANGE_TYPE change_type;
  if( DEBUG_TASK(task) )
    fprintf(stderr, "[ KheTaskMatchingSetDomain(%s, %s)\n", KheTaskId(task),
      KheResourceGroupId(rg));
  switch( KheSolnMatchingType(KheTaskSoln(task)) )
  {
    case KHE_MATCHING_TYPE_EVAL_INITIAL:
    case KHE_MATCHING_TYPE_EVAL_TIMES:

      /* set the domains to rg within task only */
      if( HaArrayCount(task->attached_monitors) > 0 )
      {
	change_type = KheTaskChangeType(task->domain, rg);
	HaArrayForEach(task->attached_monitors, m, i)
	  KheOrdinaryDemandMonitorSetDomain(m, rg, change_type);
      }
      break;

    case KHE_MATCHING_TYPE_SOLVE:
    case KHE_MATCHING_TYPE_EVAL_RESOURCES:

      /* set the domains to rg, but only if unassigned root task; */
      /* and in that case, set them in all the followers too      */
      if( task->target_task == NULL )
      {
	change_type = KheTaskChangeType(task->domain, rg);
	KheTaskMatchingDoSetDomain(task, rg, change_type);
      }
      break;

    default:

      HnAbort("KheTaskSetDomain internal error 4");
      break;
  }
  if( DEBUG_TASK(task) )
    fprintf(stderr, "] KheTaskMatchingSetDomain\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskMatchingAssign(KHE_TASK task, KHE_TASK target_task)          */
/*                                                                           */
/*  Given that task is about to be assigned to target_task, update the       */
/*  matching appropriately, including the descendants if appropriate.        */
/*                                                                           */
/*****************************************************************************/

static void KheTaskMatchingAssign(KHE_TASK task, KHE_TASK target_task)
{
  KHE_RESOURCE_GROUP new_rg;
  if( DEBUG_TASK(task) )
    fprintf(stderr, "[ KheTaskMatchingAssign(%s, %s)\n", KheTaskId(task),
      KheTaskId(target_task));
  switch( KheSolnMatchingType(KheTaskSoln(task)) )
  {
    case KHE_MATCHING_TYPE_EVAL_INITIAL:
    case KHE_MATCHING_TYPE_EVAL_TIMES:

      /* assignments don't affect matchings of these types */
      break;

    case KHE_MATCHING_TYPE_SOLVE:
    case KHE_MATCHING_TYPE_EVAL_RESOURCES:

      /* domain is tightening from task->domain to new root's domain */
      new_rg = KheTaskDomain(KheTaskRoot(target_task));
      if( !KheResourceGroupEqual(task->domain, new_rg) )
	KheTaskMatchingDoSetDomain(task, new_rg,
	  KHE_MATCHING_DOMAIN_CHANGE_TO_SUBSET);
      break;

    default:

      HnAbort("KheTaskMatchingAssign internal error");
      break;
  }
  if( DEBUG_TASK(task) )
    fprintf(stderr, "] KheTaskMatchingAssign\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskMatchingUnAssign(KHE_TASK task)                              */
/*                                                                           */
/*  Given that task is about to be unassigned, update the matching           */
/*  appropriately, including the descendants if appropriate.                 */
/*                                                                           */
/*****************************************************************************/

static void KheTaskMatchingUnAssign(KHE_TASK task)
{
  KHE_RESOURCE_GROUP old_rg;
  switch( KheSolnMatchingType(KheTaskSoln(task)) )
  {
    case KHE_MATCHING_TYPE_EVAL_INITIAL:
    case KHE_MATCHING_TYPE_EVAL_TIMES:

      /* assignments don't affect matchings of these types */
      break;

    case KHE_MATCHING_TYPE_SOLVE:
    case KHE_MATCHING_TYPE_EVAL_RESOURCES:

      /* domain is loosening from old_rg to task->domain */
      old_rg = KheTaskDomain(KheTaskRoot(task));
      if( !KheResourceGroupEqual(task->domain, old_rg) )
	KheTaskMatchingDoSetDomain(task, task->domain,
	  KHE_MATCHING_DOMAIN_CHANGE_TO_SUPERSET);
      break;

    default:

      HnAbort("KheTaskMatchingUnAssign internal error");
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskMatchingCheckConsistency(KHE_TASK task)                      */
/*                                                                           */
/*  Check the consistency of task's demand monitors with its domain.         */
/*                                                                           */
/*****************************************************************************/

static void KheTaskMatchingCheckConsistency(KHE_TASK task)
{
  KHE_RESOURCE_GROUP rg;  KHE_ORDINARY_DEMAND_MONITOR odm;  int i;
  KHE_TASK child_task;
  if( DEBUG12 && KheSolnHasMatching(task->soln) )
  {
    /* check consistency of task */
    rg = KheTaskMatchingDomain(task);
    HaArrayForEach(task->attached_monitors, odm, i)
      KheOrdinaryDemandMonitorCheckConsistency(odm, rg);

    /* check consistency of everything assigned to task */
    HaArrayForEach(task->assigned_tasks, child_task, i)
      KheTaskMatchingCheckConsistency(child_task);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time assignment"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskAssignTime(KHE_TASK task, int assigned_time_index)           */
/*                                                                           */
/*  Inform task that the enclosing meet has been assigned this time.         */
/*                                                                           */
/*****************************************************************************/

void KheTaskAssignTime(KHE_TASK task, int assigned_time_index)
{
  KHE_EVENNESS_HANDLER eh;
  if( task->assigned_rs != NULL )
    KheResourceInSolnAssignTime(task->assigned_rs, task, assigned_time_index);
  eh = KheSolnEvennessHandler(task->soln);
  if( eh != NULL )
    KheEvennessHandlerAddTask(eh, task, assigned_time_index);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskUnAssignTime(KHE_TASK task, int assigned_time_index)         */
/*                                                                           */
/*  Inform task that the enclosing meet has been unassigned this time.       */
/*                                                                           */
/*****************************************************************************/

void KheTaskUnAssignTime(KHE_TASK task, int assigned_time_index)
{
  KHE_EVENNESS_HANDLER eh;
  if( DEBUG5 )
  {
    fprintf(stderr, "[ KheTaskUnAssignTime(");
    KheTaskDebug(task, 2, -1, stderr);
    fprintf(stderr, ", %d)\n", assigned_time_index);
  }
  if( task->assigned_rs != NULL )
    KheResourceInSolnUnAssignTime(task->assigned_rs, task, assigned_time_index);
  eh = KheSolnEvennessHandler(task->soln);
  if( eh != NULL )
    KheEvennessHandlerDeleteTask(eh, task, assigned_time_index);
  if( DEBUG5 )
    fprintf(stderr, "] KheTaskUnAssignTime returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "split and merge"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoSplit(KHE_TASK task1, KHE_TASK task2, int durn1,           */
/*    KHE_MEET meet2)                                                        */
/*                                                                           */
/*  Split task at durn1 into task2, including adding task2 to meet2.         */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoSplit(KHE_TASK task1, KHE_TASK task2, int durn1,
  KHE_MEET meet2)
{
  int i, duration2;  KHE_ORDINARY_DEMAND_MONITOR m;  KHE_TASK_BOUND tb;
  KHE_RESOURCE_TYPE rt;

  /* back, soln, and enclosing meet */
  if( DEBUG10 )
    fprintf(stderr, "  splitting task %s\n", KheTaskId(task1));
  task2->id = NULL;
  task2->back = task1->back;
  task2->soln = task1->soln;
  KheSolnAddTask(task1->soln, task2);  /* will set task2->soln_index */
  HnAssert(meet2 != NULL, "KheTaskSplit internal error");
  task2->meet = meet2;
  task2->workload_per_time = task1->workload_per_time;
  KheMeetAddTask(meet2, task2, false);  /* will set task2->meet_index */

  /* set task bounds and domain */
  rt = KheTaskResourceType(task1);
  task2->domain = KheResourceTypeFullResourceGroup(rt);
  HaArrayForEach(task1->task_bounds, tb, i)
    KheTaskAddTaskBound(task2, tb);
  /* task2->domain = task1->domain;  done by KheTaskAddTaskBound */
  /* task2->tasking = NULL; */
  /* task2->tasking_index = -1; */

  /* same fixed values */
  task2->target_fixed = task1->target_fixed;

  /* same target task as task */
  if( DEBUG_TASK(task1) )
    fprintf(stderr, "  KheTaskSplit(task \"%s\")\n", KheTaskId(task1));
  task2->target_task = task1->target_task;
  if( task2->target_task != NULL )
    HaArrayAddLast(task1->target_task->assigned_tasks, task2);

  /* assigned tasks stay with task */
  task2->visit_num = task1->visit_num;
  KheTaskReferenceCountIncrement(task2);
  task2->copy = NULL;

  /* split the monitors between task1 and task2 */
  if( HaArrayCount(task1->all_monitors) > 0 )
  {
    duration2 = HaArrayCount(task1->all_monitors) - durn1;
    HnAssert(duration2 >= 1, "KheTaskSplit internal error");
    for( i = durn1;  i < HaArrayCount(task1->all_monitors);  i++ )
    {
      m = HaArray(task1->all_monitors, i);
      KheOrdinaryDemandMonitorSetTaskAndOffset(m, task2, i - durn1);
      HaArrayAddLast(task2->all_monitors, m);
    }
    HaArrayDeleteLastSlice(task1->all_monitors, duration2);
    HaArrayForEach(task1->attached_monitors, m, i)
      if( KheOrdinaryDemandMonitorTask(m) == task2 )
      {
	HaArrayDeleteAndShift(task1->attached_monitors, i);
	HaArrayAddLast(task2->attached_monitors, m);
	i--;
      }
  }

  /* initialize assigned_rs and inform it of what is happening */
  task2->assigned_rs = task1->assigned_rs;
  task2->assigned_index = -1;
  if( task1->assigned_rs != NULL )
    KheResourceInSolnSplitTask(task1->assigned_rs, task1, task2);
    /* will reset task2->assigned_index */

  /* initialize event_resource_in_soln and inform it of what is happening */
  task2->event_resource_in_soln = task1->event_resource_in_soln;
  if( task2->event_resource_in_soln != NULL )
    KheEventResourceInSolnSplitTask(task2->event_resource_in_soln, task1,task2);

  /* add to tasking, if any (non-kernel, but worry about that later) */
  /* ***
  if( task1->tasking != NULL )
    KheTaskingAddTask(task1->tasking, task2);
  *** */
  if( DEBUG_DOMAIN(task2) )
    KheTaskDebug(task2, 3, 2, stderr);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoMerge(KHE_TASK task1, KHE_TASK task2)                      */
/*                                                                           */
/*  Merge task2 into task1, leaving it unlinked but not freed.               */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoMerge(KHE_TASK task1, KHE_TASK task2)
{
  int i, pos;  KHE_ORDINARY_DEMAND_MONITOR m;  KHE_TASK child_task;

  /* remove task2 from its tasking, if any */
  if( DEBUG10 )
    fprintf(stderr, "  merging task %s\n", KheTaskId(task1));
  /* ***
  if( task2->tasking != NULL )
    KheTaskingDeleteTask(task2->tasking, task2);
  *** */

  /* remove task2 from its target task, if any */
  if( DEBUG_TASK(task2) )
    fprintf(stderr, "  KheTaskDoMerge(task \"%s\")\n", KheTaskId(task2));
  if( task1->target_task != NULL )
  {
    if( !HaArrayContains(task1->target_task->assigned_tasks, task2, &pos) )
      HnAbort("KheTaskMerge internal error");
    HaArrayDeleteAndShift(task1->target_task->assigned_tasks, pos);
  }

  /* move task2's assigned tasks to task1 */
  while( HaArrayCount(task2->assigned_tasks) > 0 )
  {
    child_task = HaArrayLastAndDelete(task2->assigned_tasks);
    child_task->target_task = task1;
    HaArrayAddLast(task1->assigned_tasks, child_task);
  }

  /* move the demand monitors */
  if( HaArrayCount(task2->all_monitors) > 0 )
  {
    HaArrayForEach(task2->all_monitors, m, i)
    {
      KheOrdinaryDemandMonitorSetTaskAndOffset(m, task1,
	HaArrayCount(task1->all_monitors));
      HaArrayAddLast(task1->all_monitors, m);
    }
    HaArrayClear(task2->all_monitors);
    HaArrayAppend(task1->attached_monitors, task2->attached_monitors, i);
    HaArrayClear(task2->attached_monitors);
  }

  /* inform the assigned_rs, if any */
  if( task1->assigned_rs != NULL )
    KheResourceInSolnMergeTask(task1->assigned_rs, task1, task2);

  /* inform the event resource in soln, if any */
  if( task1->event_resource_in_soln != NULL )
    KheEventResourceInSolnMergeTask(task1->event_resource_in_soln,
      task1, task2);

  /* delete the task from soln and optionally add it to the free list */
  KheSolnDeleteTask(task1->soln, task2);
  task2->meet = NULL;
  KheTaskReferenceCountDecrement(task2);  /* may delete task */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelSplit(KHE_TASK task1, KHE_TASK task2, int durn1,       */
/*    KHE_MEET meet2)                                                        */
/*                                                                           */
/*  Kernel operation for splitting task1 into task2 at durn1.                */
/*                                                                           */
/*  This operation does nothing about adding task bounds to task2.           */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelSplit(KHE_TASK task1, KHE_TASK task2, int durn1,
  KHE_MEET meet2)
{
  KheTaskDoSplit(task1, task2, durn1, meet2);
  KheSolnOpTaskSplit(task1->soln, task1, task2, durn1, meet2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelSplitUndo(KHE_TASK task1, KHE_TASK task2, int durn1,   */
/*    KHE_MEET meet2)                                                        */
/*                                                                           */
/*  Kernel operation for undoing a split of task1 into task2 at durn1.       */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelSplitUndo(KHE_TASK task1, KHE_TASK task2, int durn1,
  KHE_MEET meet2)
{
  KheTaskDoMerge(task1, task2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelMerge(KHE_TASK task1, KHE_TASK task2, int durn1,       */
/*    KHE_MEET meet2)                                                        */
/*                                                                           */
/*  Kernel operation for merging task1 into task2.                           */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelMerge(KHE_TASK task1, KHE_TASK task2, int durn1,
  KHE_MEET meet2)
{
  KheSolnOpTaskMerge(task1->soln, task1, task2, durn1, meet2);
  KheTaskDoMerge(task1, task2);  /* must *follow* KheSolnOpTaskMerge!! */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelMergeUndo(KHE_TASK task1, KHE_TASK task2, int durn1,   */
/*    KHE_MEET meet2)                                                        */
/*                                                                           */
/*  Kernel operation for undoing a merge of task1 into task2 at durn1.       */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelMergeUndo(KHE_TASK task1, KHE_TASK task2, int durn1,
  KHE_MEET meet2)
{
  KheTaskDoSplit(task1, task2, durn1, meet2);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskMergeCheck(KHE_TASK task1, KHE_TASK task2)                   */
/*                                                                           */
/*  Check that it is safe to merge these two tasks.                          */
/*                                                                           */
/*****************************************************************************/

bool KheTaskMergeCheck(KHE_TASK task1, KHE_TASK task2)
{
  return task1 != task2 &&
    /* task1->tasking == task2->tasking && */
    task1->target_task == task2->target_task &&
    task1->assigned_rs == task2->assigned_rs &&
    task1->event_resource_in_soln == task2->event_resource_in_soln &&
    KheResourceGroupEqual(task1->domain, task2->domain);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "assignment (basic functions)"                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheTaskAssignedIndex(KHE_TASK task)                                  */
/*                                                                           */
/*  Return the assigned index attribute of task.                             */
/*                                                                           */
/*****************************************************************************/

int KheTaskAssignedIndex(KHE_TASK task)
{
  return task->assigned_index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetAssignedIndex(KHE_TASK task, int index)                   */
/*                                                                           */
/*  Set the assigned_index field of task to index.                           */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetAssignedIndex(KHE_TASK task, int index)
{
  task->assigned_index = index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoAssignResource(KHE_TASK task, KHE_RESOURCE r)              */
/*                                                                           */
/*  Task has just been assigned to a task that has a resource, so inform     */
/*  it and its descendants of that fact.                                     */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoAssignResource(KHE_TASK task, KHE_RESOURCE r)
{
  KHE_TASK child_task;  int i;
  task->assigned_rs = task->target_task->assigned_rs;
  KheResourceInSolnAssignResource(task->assigned_rs, task);
  /* KheResourceInSolnAssignResource will set task->assigned_index */
  if( task->event_resource_in_soln != NULL )
    KheEventResourceInSolnAssignResource(task->event_resource_in_soln, task, r);
  HaArrayForEach(task->assigned_tasks, child_task, i)
    KheTaskDoAssignResource(child_task, r);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoUnAssignResource(KHE_TASK task, KHE_RESOURCE r)            */
/*                                                                           */
/*  Task has just been unassigned from a task that has a resource, so        */
/*  inform it and its descendants of that fact.                              */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoUnAssignResource(KHE_TASK task, KHE_RESOURCE r)
{
  KHE_TASK child_task;  int i;
  KheResourceInSolnUnAssignResource(task->assigned_rs, task);
  if( task->event_resource_in_soln != NULL )
    KheEventResourceInSolnUnAssignResource(task->event_resource_in_soln,
      task, r);
  HaArrayForEach(task->assigned_tasks, child_task, i)
    KheTaskDoUnAssignResource(child_task, r);
  task->assigned_rs = NULL;
  task->assigned_index = -1;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoAssign(KHE_TASK task, KHE_TASK target_task)                */
/*                                                                           */
/*  Carry out the task assignment operation, without the initial check.      */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoAssign(KHE_TASK task, KHE_TASK target_task)
{
  if( DEBUG8 )
  {
    fprintf(stderr, "  KheTaskDoAssign(");
    KheTaskDebug(task, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheTaskDebug(target_task, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }
  if( DEBUG_TASK(task) )
    fprintf(stderr, "[ KheTaskDoAssign(task %s, target_task %s), %d monitors\n",
      KheTaskId(task), KheTaskId(target_task),HaArrayCount(task->all_monitors));

  /* update the matching */
  /* if( HaArrayCount(task->all_monitors) > 0 ) wrong JeffK 14/01/24 */
  if( KheSolnHasMatching(task->soln) )
    KheTaskMatchingAssign(task, target_task);

  /* record the assignment */
  HnAssert(!KheTaskIsCycleTask(task), "KheTaskDoAssign internal error");
  task->target_task = target_task;
  HaArrayAddLast(target_task->assigned_tasks, task);

  /* if assigning a resource, inform descendants of task */
  if( target_task->assigned_rs != NULL )
    KheTaskDoAssignResource(task,
      KheResourceInSolnResource(target_task->assigned_rs));
  if( DEBUG_TASK(task) )
    fprintf(stderr, "] KheTaskDoAssign\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoUnAssign(KHE_TASK task)                                    */
/*                                                                           */
/*  Carry out the task unassign operation, without the initial check.        */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoUnAssign(KHE_TASK task)
{
  int pos;
  if( DEBUG8 )
  {
    fprintf(stderr, "  KheTaskDoUnAssign(");
    KheTaskDebug(task, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }

  /* update the matching */
  /* if( HaArrayCount(task->all_monitors) > 0 ) wrong JeffK 14/01/24 */
  if( KheSolnHasMatching(task->soln) )
    KheTaskMatchingUnAssign(task);

  /* if unassigning a resource, inform descendants of task */
  if( DEBUG_TASK(task) )
    fprintf(stderr, "  KheTaskDoUnAssign(task \"%s\")\n", KheTaskId(task));
  /* ***
  {
    KHE_RESOURCE r2;
    fprintf(stderr, "  KheTaskDoUnAssign(task \"%s\")\n", KheTaskId(task));
    HnAssert(!KheTaskIsPreassigned(task, &r2),
      "KheTaskDoUnAssign(%s):  task is preassigned", KheTaskId(task));
  }
  *** */
  if( task->target_task->assigned_rs != NULL )
    KheTaskDoUnAssignResource(task,
      KheResourceInSolnResource(task->target_task->assigned_rs));

  /* record the unassignment */
  if( !HaArrayContains(task->target_task->assigned_tasks, task, &pos) )
    HnAbort("KheTaskUnAssign internal error");
  HaArrayDeleteAndShift(task->target_task->assigned_tasks, pos);
  task->target_task = NULL;
  if( DEBUG9 )
  {
    KHE_RESOURCE r;
    if( KheTaskIsPreassigned(task, &r) )
    {
      fprintf(stderr, "KheTaskDoUnAssign(");
      KheTaskDebug(task, 1, -1, stderr);
      fprintf(stderr, ") where task is preassigned\n");
      HnAbort("KheTaskDoUnAssign aborting on unassignment of preassigned task");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoMove(KHE_TASK task, KHE_TASK target_task)                  */
/*                                                                           */
/*  Carry out a move, without reporting it to the solution path.             */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoMove(KHE_TASK task, KHE_TASK target_task)
{
  if( DEBUG_TASK(task) )
    fprintf(stderr, "[ KheTaskDoMove(\"%s\", \"%s\")\n", KheTaskId(task),
      target_task == NULL ? "=" : KheTaskId(target_task));
  KheTaskMatchingCheckConsistency(task);
  if( task->target_task != NULL )
    KheTaskDoUnAssign(task);
  if( DEBUG_TASK(task) )
    fprintf(stderr, "  KheTaskDoMove(\"%s\", \"%s\") (b)\n",
      KheTaskId(task), target_task == NULL ? "=" : KheTaskId(target_task));
  KheTaskMatchingCheckConsistency(task);
  if( DEBUG_TASK(task) )
    fprintf(stderr, "  KheTaskDoMove(\"%s\", \"%s\") (c)\n",
      KheTaskId(task), target_task == NULL ? "=" : KheTaskId(target_task));
  if( target_task != NULL )
    KheTaskDoAssign(task, target_task);
  KheTaskMatchingCheckConsistency(task);
  if( DEBUG_TASK(task) )
    fprintf(stderr, "] KheTaskDoMove\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelMove(KHE_TASK task, KHE_TASK target_task)              */
/*                                                                           */
/*  Move task to target_task, assuming all is well.                          */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelMove(KHE_TASK task, KHE_TASK target_task)
{
  KheSolnOpTaskMove(task->soln, task, task->target_task, target_task);
  KheTaskDoMove(task, target_task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelMoveUndo(KHE_TASK task,                                */
/*    KHE_TASK old_target_task, KHE_TASK new_target_task)                    */
/*                                                                           */
/*  Undo KheTaskKernelMove.                                                  */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelMoveUndo(KHE_TASK task, KHE_TASK old_target_task,
  KHE_TASK new_target_task)
{
  KheTaskDoMove(task, old_target_task);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskMoveCheck(KHE_TASK task, KHE_TASK target_task)               */
/*                                                                           */
/*  Check whether task can be moved to target_task.                          */
/*                                                                           */
/*****************************************************************************/

bool KheTaskMoveCheck(KHE_TASK task, KHE_TASK target_task)
{
  /* the current assignment must not be fixed */
  if( task->target_fixed )
  {
    if( DEBUG3 || DEBUG_TASK(task) )
      fprintf(stderr, "  KheTaskMoveCheck(%s) returning false (fixed)\n",
	KheTaskId(task));
    return false;
  }

  /* the task must not be a cycle task */
  if( KheTaskIsCycleTask(task) )
  {
    if( DEBUG3 || DEBUG_TASK(task) )
      fprintf(stderr, "  KheTaskMoveCheck(%s) returning false (cycle task)\n",
	KheTaskId(task));
    return false;
  }

  /* the move must actually change something */
  if( task->target_task == target_task )
  {
    if( DEBUG3 || DEBUG_TASK(task) )
      fprintf(stderr, "  KheTaskMoveCheck(%s) returning false (no change)\n",
	KheTaskId(task));
    return false;
  }

  /* if target_task != NULL, resource domains must match */
  if( target_task != NULL &&
      !KheResourceGroupSubset(target_task->domain, task->domain) )
  {
    if( DEBUG3 || DEBUG_TASK(task) )
    {
      fprintf(stderr, "  KheTaskMoveCheck(%s) returning false (subset ",
	KheTaskId(task));
      KheResourceGroupDebug(target_task->domain, 1, -1, stderr);
      fprintf(stderr, ", ");
      KheResourceGroupDebug(task->domain, 1, -1, stderr);
      fprintf(stderr, ")\n");
    }
    return false;
  }

  /* no problems, move is allowed */
  if( DEBUG3 )
  {
    fprintf(stderr, "  KheTaskMoveCheck(%s) returning true",
      KheTaskId(task));
    if( target_task != NULL )
    {
      fprintf(stderr, ":  KheResourceGroupSubset(");
      KheResourceGroupDebug(target_task->domain, 1, -1, stderr);
      fprintf(stderr, ", ");
      KheResourceGroupDebug(task->domain, 1, -1, stderr);
      fprintf(stderr, ")");
    }
    fprintf(stderr, "\n");
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskMove(KHE_TASK task, KHE_TASK target_task)                    */
/*                                                                           */
/*  Move task to target_task.                                                */
/*                                                                           */
/*****************************************************************************/

bool KheTaskMove(KHE_TASK task, KHE_TASK target_task)
{
  KHE_RESOURCE r;
  if( DEBUG7 && KheTaskIsPreassigned(task, &r) )
    HnAssert(target_task != NULL, "KheTaskMove clearing preassignment");
  if( !KheTaskMoveCheck(task, target_task) )
    return false;
  KheTaskKernelMove(task, target_task);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "assignment (helper functions)"                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskAssignCheck(KHE_TASK task, KHE_TASK target_task)             */
/*                                                                           */
/*  Return true if task can be assigned to target_task.                      */
/*                                                                           */
/*****************************************************************************/

bool KheTaskAssignCheck(KHE_TASK task, KHE_TASK target_task)
{
  return task->target_task == NULL && KheTaskMoveCheck(task, target_task);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskAssign(KHE_TASK task, KHE_TASK target_task)                  */
/*                                                                           */
/*  Assign task to target_task.                                              */
/*                                                                           */
/*****************************************************************************/

bool KheTaskAssign(KHE_TASK task, KHE_TASK target_task)
{
  KheTaskMatchingCheckConsistency(task);
  return task->target_task == NULL && KheTaskMove(task, target_task);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskUnAssignCheck(KHE_TASK task)                                 */
/*                                                                           */
/*  Return true if task may be unassigned.                                   */
/*                                                                           */
/*****************************************************************************/

bool KheTaskUnAssignCheck(KHE_TASK task)
{
  return KheTaskMoveCheck(task, NULL);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskUnAssign(KHE_TASK task)                                      */
/*                                                                           */
/*  Unassign task.                                                           */
/*                                                                           */
/*****************************************************************************/

bool KheTaskUnAssign(KHE_TASK task)
{
  KheTaskMatchingCheckConsistency(task);
  return KheTaskMove(task, NULL);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSwapCheck(KHE_TASK task1, KHE_TASK task2)                    */
/*                                                                           */
/*  Check whether swapping task1 and task2 would succeed.  This returns      */
/*  false if the swap would change nothing, and if either move would fail.   */
/*                                                                           */
/*****************************************************************************/

bool KheTaskSwapCheck(KHE_TASK task1, KHE_TASK task2)
{
  return KheTaskMoveCheck(task1, task2->target_task) &&
    KheTaskMoveCheck(task2, task1->target_task);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSwap(KHE_TASK task1, KHE_TASK task2)                         */
/*                                                                           */
/*  Swap task1 and task2.                                                    */
/*                                                                           */
/*****************************************************************************/

bool KheTaskSwap(KHE_TASK task1, KHE_TASK task2)
{
  KHE_TASK task1_target_task;
  if( !KheTaskSwapCheck(task1, task2) )
    return false;
  task1_target_task = task1->target_task;
  KheTaskMove(task1, task2->target_task);
  KheTaskMove(task2, task1_target_task);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "assignment (queries)"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskAsst(KHE_TASK task)                                      */
/*                                                                           */
/*  Return the task that task is currently assigned to, or NULL if none.     */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheTaskAsst(KHE_TASK task)
{
  return task->target_task;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskAssignedToCount(KHE_TASK target_task)                         */
/*                                                                           */
/*  Return the number of tasks assigned to target_task.                      */
/*                                                                           */
/*****************************************************************************/

int KheTaskAssignedToCount(KHE_TASK target_task)
{
  return HaArrayCount(target_task->assigned_tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskAssignedTo(KHE_TASK target_task, int i)                  */
/*                                                                           */
/*  Return the i'th task assigned to target_task.                            */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheTaskAssignedTo(KHE_TASK target_task, int i)
{
  return HaArray(target_task->assigned_tasks, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskTotalDuration(KHE_TASK task)                                  */
/*                                                                           */
/*  Return the total duration of task and the tasks that are assigned to     */
/*  it, directly and indirectly.                                             */
/*                                                                           */
/*****************************************************************************/

int KheTaskTotalDuration(KHE_TASK task)
{
  int res, i;  KHE_TASK child_task;
  res = KheTaskDuration(task);
  HaArrayForEach(task->assigned_tasks, child_task, i)
    res += KheTaskTotalDuration(child_task);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  float KheTaskTotalWorkload(KHE_TASK task)                                */
/*                                                                           */
/*  Return the total workload of task and the tasks that are assigned to     */
/*  it, directly and indirectly.                                             */
/*                                                                           */
/*****************************************************************************/

float KheTaskTotalWorkload(KHE_TASK task)
{
  float res;  int i;  KHE_TASK child_task;
  res = KheTaskWorkload(task);
  HaArrayForEach(task->assigned_tasks, child_task, i)
    res += KheTaskTotalWorkload(child_task);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskRoot(KHE_TASK task)                                      */
/*                                                                           */
/*  Return the root of the task's assignment chain.                          */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheTaskRoot(KHE_TASK task)
{
  while( task->target_task != NULL )
    task = task->target_task;
  return task;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskProperRoot(KHE_TASK task)                                */
/*                                                                           */
/*  Return the proper root of the task's assignment chain.  This is the      */
/*  same as KheTaskRoot except that it excludes assignments to cycle         */
/*  tasks from the assignment chain.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheTaskProperRoot(KHE_TASK task)
{
  while( task->target_task != NULL && !KheTaskIsCycleTask(task->target_task) )
    task = task->target_task;
  return task;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskIsProperRoot(KHE_TASK task)                                  */
/*                                                                           */
/*  Return true if task is a proper root task:  not a cycle task, and not    */
/*  assigned to any non-cycle task.                                          */
/*                                                                           */
/*****************************************************************************/

bool KheTaskIsProperRoot(KHE_TASK task)
{
  if( KheTaskIsCycleTask(task) )
    return false;
  return task->target_task == NULL || KheTaskIsCycleTask(task->target_task);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskTotalMonitoredDuration(KHE_TASK task, KHE_MONITOR m)          */
/*                                                                           */
/*  Return the total duration of those tasks which are both descendants      */
/*  of task (including task itself) and monitored by m.                      */
/*                                                                           */
/*****************************************************************************/

static int KheTaskTotalMonitoredDuration(KHE_TASK task, KHE_MONITOR m)
{
  int i, res;  KHE_EVENT_RESOURCE er;  KHE_TASK child_task;

  /* monitored duration of task itself */
  res = 0;
  er = KheTaskEventResource(task);
  if( er != NULL )
  {
    for( i = 0;  i < KheSolnEventResourceMonitorCount(task->soln, er);  i++ )
      if( KheSolnEventResourceMonitor(task->soln, er, i) == m )
	res += KheTaskDuration(task);
  }

  /* monitored duration of task's proper descendants */
  HaArrayForEach(task->assigned_tasks, child_task, i)
    res += KheTaskTotalMonitoredDuration(child_task, m);

  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskDoNeedsAssignment(KHE_TASK task, KHE_TASK root_task,         */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Return true if task or any task assigned to it, directly or indirectly,  */
/*  needs assignment.  Here root_task is task's proper root, and r is the    */
/*  resource that root_task is assigned to, or NULL if none.                 */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskDoNeedsAssignment(KHE_TASK task, KHE_TASK root_task,
  KHE_RESOURCE r)
{
  KHE_TASK child_task;  KHE_MONITOR m;  KHE_RESOURCE_GROUP rg;
  int i, minimum, maximum, active_durn, task_durn;  KHE_EVENT_RESOURCE er;
  KHE_LIMIT_RESOURCES_MONITOR lrm;  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;

  /* check the tasks assigned to task */
  HaArrayForEach(task->assigned_tasks, child_task, i)
    if( KheTaskDoNeedsAssignment(child_task, root_task, r) )
      return true;

  /* at this point the result depends only on task itself */
  er = KheTaskEventResource(task);
  if( er == NULL )
    return false;

  switch( KheEventResourceNeedsAssignment(er) )
  {
    case KHE_NO:

      return false;

    case KHE_MAYBE:

      for( i = 0;  i < KheSolnEventResourceMonitorCount(task->soln, er);  i++ )
      {
	m = KheSolnEventResourceMonitor(task->soln, er, i);
	if( KheMonitorTag(m) == KHE_LIMIT_RESOURCES_MONITOR_TAG )
	{
	  lrm = (KHE_LIMIT_RESOURCES_MONITOR) m;
	  lrc = KheLimitResourcesMonitorConstraint(lrm);
	  rg = KheLimitResourcesConstraintDomain(lrc);
	  KheLimitResourcesMonitorActiveDuration(lrm, &minimum,
	    &maximum, &active_durn);
	  if( active_durn < minimum )
	  {
	    /* lrm is a problem right now, so task needs to be assigned */
	    return true;
	  }
	  else if( minimum > 0 && r != NULL && KheResourceGroupContains(rg, r) )
	  {
	    /* at this point, lrm is potentially a problem, though not */
	    /* right now, but task is assigned and that may be why not */
	    if( active_durn - 1 < minimum )
	      return true;
	    task_durn = KheTaskTotalMonitoredDuration(root_task, m);
	    if( active_durn - task_durn < minimum )
	      return true;
	  }
	}
      }
      return false;

    case KHE_YES:

      return true;

    default:

      HnAbort("KheTaskNeedsAssignment internal error");
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskNeedsAssignment(KHE_TASK task)                               */
/*                                                                           */
/*  Return true if task needs assignment.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheTaskNeedsAssignment(KHE_TASK task)
{
  bool res;
  if( DEBUG4 )
  {
    fprintf(stderr, "KheTaskNeedsAssignment(");
    KheTaskDebug(task, 1, -1, stderr);
    fprintf(stderr, " = ");
  }
  task = KheTaskProperRoot(task);
  res = KheTaskDoNeedsAssignment(task, task, KheTaskAsstResource(task));
  if( DEBUG4 )
    fprintf(stderr, "%s\n", res ? "true" : "false");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskAsstDoCostReduction(KHE_TASK task, KHE_RESOURCE r,           */
/*    KHE_COST *cost)                                                        */
/*                                                                           */
/*  Carry out KheTaskAssignmentCostReduction for the task and for the        */
/*  tasks assigned, directly or indirectly, to task.  Here r is the          */
/*  resource that task is currently assigned to, or NULL if none.            */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
static void KheTaskAsstDoCostReduction(KHE_TASK task, KHE_RESOURCE r,
  KHE_COST *cost)
{
  KHE_TASK child_task;  int i, durn, minimum, maximum, active_durn;
  KHE_MONITOR m;  KHE_EVENT_RESOURCE er;
  ** KHE_PREFER_RESOURCES_CONSTRAINT prc; **  KHE_RESOURCE_GROUP rg;
  KHE_LIMIT_RESOURCES_MONITOR lrm;  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  KHE_PREFER_RESOURCES_MONITOR prm;

  ** do it for the tasks assigned task, directly or indirectly **
  HaArrayForEach(task->assigned_tasks, child_task, i)
    KheTaskAsstDoCostReduction(child_task, r, cost);

  ** do it for task itself **
  er = KheTaskEventResource(task);
  if( er == NULL )
    return;
  durn = KheTaskDuration(task);
  for( i = 0;  i < KheSolnEventResourceMonitorCount(task->soln, er);  i++ )
  {
    m = KheSolnEventResourceMonitor(task->soln, er, i);
    switch( KheMonitorTag(m) )
    {
      case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

	** always produces a cost reduction **
	*cost += KheMonitorCombinedWeight(m) * durn;
	break;

      case KHE_PREFER_RESOURCES_MONITOR_TAG:

	** produces a cost increase if domain is empty **
	prm = (KHE_PREFER_RESOURCES_MONITOR) m;
	rg = KhePreferResourcesMonitorDomain(prm);
	** ***
	prc = (KHE_PREFER_RESOURCES_CONSTRAINT) KheMonitorConstraint(m);
	rg = KhePreferResourcesConstraintDomain(prc);
	*** **
	if( KheResourceGroupResourceCount(rg) == 0 )
	  *cost -= KheMonitorCombinedWeight(m) * durn;
	break;

      case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

	** nothing to do here **
	break;

      case KHE_LIMIT_RESOURCES_MONITOR_TAG:

	** increase or decrease, depending on where wrt the limits **
	lrm = (KHE_LIMIT_RESOURCES_MONITOR) m;
	lrc = KheLimitResourcesMonitorConstraint(lrm);
	rg = KheLimitResourcesConstraintDomain(lrc);
	KheLimitResourcesMonitorActiveDuration(lrm, &minimum,
	  &maximum, &active_durn);
	if( r != NULL )
	  active_durn -= durn;
	if( active_durn < minimum )
	{
	  ** assignment produces a cost reduction **
	  *cost += KheMonitorCombinedWeight(m) * durn;
	}
	else if( active_durn >= maximum )
	{
	  ** assignment produces a cost increase **
	  *cost -= KheMonitorCombinedWeight(m) * durn;
	}
	break;

      default:

	** nothing to do here (could be a demand monitor) **
	break;
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheTaskAssignmentCostReduction(KHE_TASK task)                   */
/*                                                                           */
/*  Return the reduction in solution cost if task is assigned.               */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
KHE_COST KheTaskAssignmentCostReduction(KHE_TASK task)
{
  KHE_COST res;
  res = 0;
  task = KheTaskProperRoot(task);
  KheTaskAsstDoCostReduction(task, KheTaskAsstResource(task), &res);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskAssignmentHasCost(KHE_TASK task)                             */
/*                                                                           */
/*  Return true if assigning this task would create a cost, because it,      */
/*  or some task assigned directly or indirectly to it, is monitored by a    */
/*  prefer resources constraint with non-zero weight and an empty domain.    */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete (was not used anyway)
bool KheTaskAssignmentHasCost(KHE_TASK task)
{
  KHE_PREFER_RESOURCES_CONSTRAINT prc;  KHE_TASK child_task;  KHE_MONITOR m;
  KHE_SOLN soln;  int i;  KHE_EVENT_RESOURCE er;  KHE_RESOURCE_GROUP rg;

  ** check the task itself **
  soln = KheTaskSoln(task);
  er = KheTaskEventResource(task);
  if( er != NULL )
    for( i = 0;  i < KheSolnEventResourceMonitorCount(soln, er);  i++ )
    {
      m = KheSolnEventResourceMonitor(soln, er, i);
      if( KheMonitorTag(m) == KHE_PREFER_RESOURCES_MONITOR_TAG )
      {
	prc = KhePreferResourcesMonitorConstraint(
	  (KHE_PREFER_RESOURCES_MONITOR) m);
	rg = KhePreferResourcesConstraintDomain(prc);
	if( KheResourceGroupResourceCount(rg) == 0 &&
	    KheConstraintWeight((KHE_CONSTRAINT) prc) > 0 )
	  return true;
      }
    }

  ** check the tasks assigned to task **
  HaArrayForEach(task->assigned_tasks, child_task, i)
    if( KheTaskAssignmentHasCost(child_task) )
      return true;

  ** no luck anywhere **
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskNonAssignme ntHasCost(KHE_TASK task)                         */
/*                                                                           */
/*  Return true if not assigning this task would create a cost, because it,  */
/*  or some task assigned directly or indirectly to it, is monitored by an   */
/*  assign resource constraint with non-zero penalty, or by a limit          */
/*  resources constraint with a non-zero minimum limit and non-zero penalty. */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete (see KheTaskNeedsAssignment above)
bool KheTaskNonAs signmentHasCost(KHE_TASK task, bool certain)
{
  KHE_EVENT_RESOURCE er;  KHE_SOLN soln;  int i;  KHE_MONITOR m;
  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;  KHE_ASSIGN_RESOURCE_CONSTRAINT arc;
  KHE_TASK child_task;

  ** check the task itself **
  soln = KheTaskSoln(task);
  er = KheTaskEventResource(task);
  if( er != NULL )
    for( i = 0;  i < KheSolnEventResourceMonitorCount(soln, er);  i++ )
    {
      m = KheSolnEventResourceMonitor(soln, er, i);
      switch( KheMonitorTag(m) )
      {
	case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

	  arc = KheAssignResourceMonitorConstraint(
	    (KHE_ASSIGN_RESOURCE_MONITOR) m);
	  if( KheConstraintWeight((KHE_CONSTRAINT) arc) > 0 )
	    return true;
	  break;

	case KHE_LIMIT_RESOURCES_MONITOR_TAG:

	  if( !certain )
	  {
	    lrc = KheLimitResourcesMonitorConstraint(
	      (KHE_LIMIT_RESOURCES_MONITOR) m);
	    if( KheLimitResourcesConstraintMinimum(lrc) > 0 &&
		KheConstraintWeight((KHE_CONSTRAINT) lrc) > 0 )
	      return true;
	  }
	  break;

	default:

	  break;
      }
    }

  ** check the tasks assigned to task **
  HaArrayForEach(task->assigned_tasks, child_task, i)
    if( KheTaskNonAssignm entHasCost(child_task, certain) )
      return true;

  ** no luck anywhere **
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "assignment (fixing and unfixing)"                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoAssignFix(KHE_TASK task)                                   */
/*                                                                           */
/*  Fix the assignment of task.                                              */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoAssignFixRecursive(KHE_TASK task)
{
  KHE_TASK sub_task;  int i;
  if( task->event_resource_in_soln != NULL )
    KheEventResourceInSolnTaskAssignFix(task->event_resource_in_soln);
  HaArrayForEach(task->assigned_tasks, sub_task, i)
    if( sub_task->target_fixed )
      KheTaskDoAssignFixRecursive(sub_task);
}

static void KheTaskDoAssignFix(KHE_TASK task)
{
  task->target_fixed = true;
  KheTaskDoAssignFixRecursive(task);
  if( DEBUG_TASK(task) )
    fprintf(stderr, "  KheTaskDoAssignFix(task \"%s\")\n", KheTaskId(task));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoAssignUnFix(KHE_TASK task)                                 */
/*                                                                           */
/*  Unfix the assignment of task.                                            */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoAssignUnFixRecursive(KHE_TASK task)
{
  KHE_TASK sub_task;  int i;
  if( task->event_resource_in_soln != NULL )
    KheEventResourceInSolnTaskAssignUnFix(task->event_resource_in_soln);
  HaArrayForEach(task->assigned_tasks, sub_task, i)
    if( sub_task->target_fixed )
      KheTaskDoAssignUnFixRecursive(sub_task);
}

static void KheTaskDoAssignUnFix(KHE_TASK task)
{
  task->target_fixed = false;
  KheTaskDoAssignUnFixRecursive(task);
  if( DEBUG_TASK(task) )
    fprintf(stderr, "  KheTaskDoAssignUnFix(task \"%s\")\n", KheTaskId(task));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelAssignFix(KHE_TASK task)                               */
/*                                                                           */
/*  The kernel operation which fixes the assignment of task.                 */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelAssignFix(KHE_TASK task)
{
  KheTaskDoAssignFix(task);
  KheSolnOpTaskAssignFix(task->soln, task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelAssignFixUndo(KHE_TASK task)                           */
/*                                                                           */
/*  Undo KheTaskKernelAssignFix.                                             */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelAssignFixUndo(KHE_TASK task)
{
  KheTaskDoAssignUnFix(task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelAssignUnFix(KHE_TASK task)                             */
/*                                                                           */
/*  The kernel operation which unfixes the assignment of task.               */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelAssignUnFix(KHE_TASK task)
{
  KheTaskDoAssignUnFix(task);
  KheSolnOpTaskAssignUnFix(task->soln, task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelAssignUnFixUndo(KHE_TASK task)                         */
/*                                                                           */
/*  Undo KheTaskKernelAssignFix.                                             */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelAssignUnFixUndo(KHE_TASK task)
{
  KheTaskDoAssignFix(task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskAssignFix(KHE_TASK task)                                     */
/*                                                                           */
/*  Prevent the assignment of task from changing.                            */
/*                                                                           */
/*****************************************************************************/

void KheTaskAssignFix(KHE_TASK task)
{
  if( !task->target_fixed )
  {
    if( DEBUG1 )
    {
      fprintf(stderr, "[ KheTaskAssignFix(");
      KheTaskDebug(task, 1, -1, stderr);
      fprintf(stderr, ")\n");
    }
    KheTaskKernelAssignFix(task);
    if( DEBUG1 )
      fprintf(stderr, "]\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskAssignUnFix(KHE_TASK task)                                   */
/*                                                                           */
/*  Allow the assignment of task to change.                                  */
/*                                                                           */
/*****************************************************************************/

void KheTaskAssignUnFix(KHE_TASK task)
{
#if USE_MAGIC
  HnAssert(task->magic == MAGIC,
    "KheTaskAssignUnFix internal error (wrong magic number)");
#endif
  if( task->target_fixed )
  {
    if( DEBUG1 )
    {
      fprintf(stderr, "[ KheTaskAssignUnFix(");
      KheTaskDebug(task, 1, -1, stderr);
      fprintf(stderr, ")\n");
    }
    KheTaskKernelAssignUnFix(task);
    if( DEBUG1 )
      fprintf(stderr, "]\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskAssignIsFixed(KHE_TASK task)                                 */
/*                                                                           */
/*  Return true if the current assignment of task is fixed.                  */
/*                                                                           */
/*****************************************************************************/

bool KheTaskAssignIsFixed(KHE_TASK task)
{
  return task->target_fixed;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskFirstUnFixed(KHE_TASK task)                              */
/*                                                                           */
/*  Return the first unfixed task on the chain of assignments leading out    */
/*  of task, or NULL if none.                                                */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheTaskFirstUnFixed(KHE_TASK task)
{
  while( task != NULL && task->target_fixed )
    task = task->target_task;
  return task;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "cycle tasks and resource assignment"                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskIsCycleTask(KHE_TASK task)                                   */
/*                                                                           */
/*  Return true if task is a cycle task.                                     */
/*                                                                           */
/*  Implementation note.  To save a boolean field, we don't store this       */
/*  condition explicitly in task.  Instead, we use the fact that the only    */
/*  tasks that have an assigned resource without being assigned to another   */
/*  task are cycle tasks.                                                    */
/*                                                                           */
/*****************************************************************************/

bool KheTaskIsCycleTask(KHE_TASK task)
{
  return task->target_task == NULL && task->assigned_rs != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheSolnResourceCycleTask(KHE_SOLN soln, KHE_RESOURCE r)         */
/*                                                                           */
/*  Return the cycle task for r in soln.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheSolnResourceCycleTask(KHE_SOLN soln, KHE_RESOURCE r)
{
  return KheSolnTask(soln, KheResourceInstanceIndex(r));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskMoveResourceCheck(KHE_TASK task, KHE_RESOURCE r)             */
/*                                                                           */
/*  Check whether it is possible to move task from wherever it is now to r.  */
/*                                                                           */
/*****************************************************************************/

bool KheTaskMoveResourceCheck(KHE_TASK task, KHE_RESOURCE r)
{
  KHE_TASK target_task;
  target_task = (r == NULL ? NULL :
    KheSolnTask(task->soln, KheResourceInstanceIndex(r)));
  return KheTaskMoveCheck(task, target_task);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskMoveResource(KHE_TASK task, KHE_RESOURCE r)                  */
/*                                                                           */
/*  Move task from wherever it is now to r.                                  */
/*                                                                           */
/*****************************************************************************/

bool KheTaskMoveResource(KHE_TASK task, KHE_RESOURCE r)
{
  KHE_TASK target_task;
  target_task = (r == NULL ? NULL :
    KheSolnTask(task->soln, KheResourceInstanceIndex(r)));
  return KheTaskMove(task, target_task);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskAssignResourceCheck(KHE_TASK task, KHE_RESOURCE r)           */
/*                                                                           */
/*  Return true if r can be assigned to task.                                */
/*                                                                           */
/*****************************************************************************/

bool KheTaskAssignResourceCheck(KHE_TASK task, KHE_RESOURCE r)
{
  KHE_TASK target_task;
  target_task = KheSolnResourceCycleTask(task->soln, r);
  return KheTaskAssignCheck(task, target_task);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskAssignResource(KHE_TASK task, KHE_RESOURCE r)                */
/*                                                                           */
/*  Assign r to task if possible.                                            */
/*                                                                           */
/*****************************************************************************/

bool KheTaskAssignResource(KHE_TASK task, KHE_RESOURCE r)
{
  KHE_TASK target_task;
  target_task = KheSolnResourceCycleTask(task->soln, r);
  return KheTaskAssign(task, target_task);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskUnAssignResourceCheck(KHE_TASK task)                         */
/*                                                                           */
/*  Return true if task may be unassigned.                                   */
/*                                                                           */
/*****************************************************************************/

bool KheTaskUnAssignResourceCheck(KHE_TASK task)
{
  return KheTaskUnAssignCheck(task);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskUnAssignResource(KHE_TASK task)                              */
/*                                                                           */
/*  Unassign task.                                                           */
/*                                                                           */
/*****************************************************************************/

bool KheTaskUnAssignResource(KHE_TASK task)
{
  return KheTaskUnAssign(task);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheTaskAsstResource(KHE_TASK task)                          */
/*                                                                           */
/*  Return the resource that task is assigned to, or NULL if none.           */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheTaskAsstResource(KHE_TASK task)
{
  return task->assigned_rs == NULL ? NULL :
    KheResourceInSolnResource(task->assigned_rs);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "task domains and bounds"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheTaskDomain(KHE_TASK task)                          */
/*                                                                           */
/*  Return the domain of task.                                               */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheTaskDomain(KHE_TASK task)
{
  return task->domain;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoSetDomain(KHE_TASK task, KHE_RESOURCE_GROUP rg)            */
/*                                                                           */
/*  Set the domain of task to rg, assuming that this is safe to do.          */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoSetDomain(KHE_TASK task, KHE_RESOURCE_GROUP rg)
{
  int assigned_time_index;  KHE_EVENNESS_HANDLER eh;
  KheTaskMatchingCheckConsistency(task);
  /* if( HaArrayCount(task->all_monitors) > 0 ) wrong JeffK 14/01/24 */
  if( KheSolnHasMatching(task->soln) )
    KheTaskMatchingSetDomain(task, rg);
  if( task->meet == NULL )
    task->domain = rg;
  else
  {
    assigned_time_index = KheMeetAssignedTimeIndex(task->meet);
    if( assigned_time_index != NO_TIME_INDEX )
    {
      eh = KheSolnEvennessHandler(task->soln);
      if( eh != NULL )
      {
	KheEvennessHandlerDeleteTask(eh, task, assigned_time_index);
	task->domain = rg;
	KheEvennessHandlerAddTask(eh, task, assigned_time_index);
      }
      else
	task->domain = rg;
    }
    else
      task->domain = rg;
  }
  if( DEBUG_DOMAIN(task) )
    KheTaskDebug(task, 3, 2, stderr);
  KheTaskMatchingCheckConsistency(task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoAddTaskBound(KHE_TASK task, KHE_TASK_BOUND tb,             */
/*    KHE_RESOURCE_GROUP new_domain)                                         */
/*                                                                           */
/*  Add tb to task.  Here new_domain is the value of task's domain after     */
/*  the addition.                                                            */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoAddTaskBound(KHE_TASK task, KHE_TASK_BOUND tb)
  /* KHE_RESOURCE_GROUP new_domain) */
{
  KHE_RESOURCE_GROUP new_domain;

  /* find the new domain */
  new_domain = KheTaskDomainWhenTaskBoundAdded(task, tb);

  /* update task and tb */
  HaArrayAddLast(task->task_bounds, tb);
  KheTaskBoundAddTask(tb, task);

  /* change task's domain */
  KheTaskDoSetDomain(task, new_domain);
  /* KheTaskDoSet Domain(task, KheTask DomainWhenTaskBoundAdded(task, tb)); */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoDeleteTaskBound(KHE_TASK task, KHE_TASK_BOUND tb,          */
/*    int pos, KHE_RESOURCE_GROUP new_domain)                                */
/*                                                                           */
/*  Delete tb from task.  Here pos is tb's position in task, and new_domain  */
/*  is the value of task's domain after the deletion.                        */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoDeleteTaskBound(KHE_TASK task, KHE_TASK_BOUND tb,
  int pos, KHE_RESOURCE_GROUP new_domain)
{
  /* update task and tb */
  HnAssert(HaArray(task->task_bounds, pos) == tb,
    "KheTaskDoDeleteTaskBound internal error");
  HaArrayDeleteAndPlug(task->task_bounds, pos);
  KheTaskBoundDeleteTask(tb, task);

  /* change task's domain */
  KheTaskDoSetDomain(task, new_domain);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelAddTaskBound(KHE_TASK task, KHE_TASK_BOUND tb)         */
/*                                                                           */
/*  Kernel operation which adds tb to task.                                  */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelAddTaskBound(KHE_TASK task, KHE_TASK_BOUND tb)
{
  KheSolnOpTaskAddTaskBound(task->soln, task, tb);
  KheTaskDoAddTaskBound(task, tb);
  /* , KheTaskDomainWhenTaskBoundAdded(task, tb)); */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelAddTaskBoundUndo(KHE_TASK task, KHE_TASK_BOUND tb)     */
/*                                                                           */
/*  Undo KheTaskKernelAddTaskBound.                                          */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelAddTaskBoundUndo(KHE_TASK task, KHE_TASK_BOUND tb)
{
  int pos;  KHE_RESOURCE_GROUP new_domain;
  KheTaskMatchingCheckConsistency(task);
  if( !HaArrayContains(task->task_bounds, tb, &pos) )
    HnAbort("KheTaskKernelAddTaskBoundUndo internal error");
  new_domain = KheTaskDomainWhenTaskBoundDeleted(task, pos);
  KheTaskDoDeleteTaskBound(task, tb, pos, new_domain);
  KheTaskMatchingCheckConsistency(task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelDeleteTaskBound(KHE_TASK task, KHE_TASK_BOUND tb)      */
/*                                                                           */
/*  Delete tb from task.                                                     */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelDeleteTaskBound(KHE_TASK task, KHE_TASK_BOUND tb)
{
  int pos;  KHE_RESOURCE_GROUP new_domain;
  KheTaskMatchingCheckConsistency(task);
  if( !HaArrayContains(task->task_bounds, tb, &pos) )
    HnAbort("KheTaskKernelAddTaskBoundUndo internal error");
  new_domain = KheTaskDomainWhenTaskBoundDeleted(task, pos);
  KheTaskDoDeleteTaskBound(task, tb, pos, new_domain);
  KheSolnOpTaskDeleteTaskBound(task->soln, task, tb);
  KheTaskMatchingCheckConsistency(task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskKernelDeleteTaskBoundUndo(KHE_TASK task, KHE_TASK_BOUND tb)  */
/*                                                                           */
/*  Undo KheTaskKernelDeleteTaskBound.                                       */
/*                                                                           */
/*****************************************************************************/

void KheTaskKernelDeleteTaskBoundUndo(KHE_TASK task, KHE_TASK_BOUND tb)
{
  KheTaskDoAddTaskBound(task, tb);
  /* , KheTaskDomainWhenTaskBoundAdded(task, tb)); */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskAddTaskBoundCheck(KHE_TASK task, KHE_TASK_BOUND tb)          */
/*                                                                           */
/*  Return true if it is safe to add tb to task.                             */
/*                                                                           */
/*****************************************************************************/

bool KheTaskAddTaskBoundCheck(KHE_TASK task, KHE_TASK_BOUND tb)
{
  KHE_RESOURCE_GROUP rg;

  /* rg must have the right type */
  rg = KheTaskBoundResourceGroup(tb);
  HnAssert(KheResourceGroupResourceType(task->domain) ==
    KheResourceGroupResourceType(rg),
    "KheTaskAddTaskBoundCheck: rg has wrong type");

  /* task may not be a cycle task */
  if( KheTaskIsCycleTask(task) )
    return false;

  /* rg must be a superset of any target task's domain */
  if( task->target_task != NULL &&
      !KheResourceGroupSubset(task->target_task->domain, rg) )
    return false;

  /* all OK */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskAddTaskBound(KHE_TASK task, KHE_TASK_BOUND tb)               */
/*                                                                           */
/*  Add tb to task.  This has already been checked for safety by a call to   */
/*  KheTaskTaskBoundMakeCheck above.                                         */
/*                                                                           */
/*****************************************************************************/

bool KheTaskAddTaskBound(KHE_TASK task, KHE_TASK_BOUND tb)
{
  KheTaskMatchingCheckConsistency(task);
  if( !KheTaskAddTaskBoundCheck(task, tb) )
    return false;

  /* carry out the kernel add operation */
  KheTaskKernelAddTaskBound(task, tb);
  KheTaskMatchingCheckConsistency(task);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskDoDeleteTaskBoundCheck(KHE_TASK task, KHE_TASK_BOUND tb,     */
/*    int *pos, KHE_RESOURCE_GROUP *new_domain)                              */
/*                                                                           */
/*  Check that it is safe to delete tb from task.                            */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskDoDeleteTaskBoundCheck(KHE_TASK task, KHE_TASK_BOUND tb,
  int *pos, KHE_RESOURCE_GROUP *new_domain)
{
  KHE_TASK child_task;  int i;

  /* abort if tb is not present in task */
  *new_domain = NULL;
  if( !HaArrayContains(task->task_bounds, tb, pos) )
    HnAbort("KheTaskDeleteTaskBoundCheck:  task does not contain tb");

  /* task may not be a cycle task */
  if( KheTaskIsCycleTask(task) )
    return false;

  /* get *new_domain and check compatibility with descendant domains */
  *new_domain = KheTaskDomainWhenTaskBoundDeleted(task, *pos);
  HaArrayForEach(task->assigned_tasks, child_task, i)
    if( !KheResourceGroupSubset(*new_domain, child_task->domain) )
      return false;

  /* all in order */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskDeleteTaskBoundCheck(KHE_TASK task, KHE_TASK_BOUND tb)       */
/*                                                                           */
/*  Check that it is safe to delete tb from task.                            */
/*                                                                           */
/*****************************************************************************/

bool KheTaskDeleteTaskBoundCheck(KHE_TASK task, KHE_TASK_BOUND tb)
{
  int pos;  KHE_RESOURCE_GROUP new_domain;
  return KheTaskDoDeleteTaskBoundCheck(task, tb, &pos, &new_domain);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskDeleteTaskBound(KHE_TASK task, KHE_TASK_BOUND tb)            */
/*                                                                           */
/*  Delete tb from task.                                                     */
/*                                                                           */
/*****************************************************************************/

bool KheTaskDeleteTaskBound(KHE_TASK task, KHE_TASK_BOUND tb)
{
  int pos;  KHE_RESOURCE_GROUP new_domain;
  /* KHE_TASK_BOUND tb2;  int index; */

  /* make sure that the operation can proceed */
  KheTaskMatchingCheckConsistency(task);
  if( !KheTaskDoDeleteTaskBoundCheck(task, tb, &pos, &new_domain) )
    return false;

  /* carry out the kernel delete operation */
  /* the body of KheTaskKernelDeleteTaskBound, minus things already done */
  KheTaskDoDeleteTaskBound(task, tb, pos, new_domain);
  KheSolnOpTaskDeleteTaskBound(task->soln, task, tb);
  KheTaskMatchingCheckConsistency(task);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskTaskBoundCount(KHE_TASK task)                                 */
/*                                                                           */
/*  Return the number of task bounds of task.                                */
/*                                                                           */
/*****************************************************************************/

int KheTaskTaskBoundCount(KHE_TASK task)
{
  return HaArrayCount(task->task_bounds);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_BOUND KheTaskTaskBound(KHE_TASK task, int i)                    */
/*                                                                           */
/*  Return the i'th task bound of task.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_BOUND KheTaskTaskBound(KHE_TASK task, int i)
{
  return HaArray(task->task_bounds, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "demand monitors"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskMatchingBegin(KHE_TASK task)                                 */
/*                                                                           */
/*  Begin the matching in task.                                              */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete
void KheTaskMatchingBegin(KHE_TASK task)
{
  int i;  KHE_MATCHING_DEMAND_CHUNK dc;  KHE_ORDINARY_DEMAND_MONITOR m;
  if( task->meet != NULL )
    for( i = 0;  i < KheMeetDuration(task->meet);  i++ )
    {
      dc = KheMeetDemandChunk(task->meet, i);
      m = KheOrdinaryDemandMonitorMake(task->soln, dc, task, i);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) task->soln,
	(KHE_MONITOR) m);
    }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskMatchingEnd(KHE_TASK task)                                   */
/*                                                                           */
/*  End the matching in task.                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheTaskMatchingEnd(KHE_TASK task)
{
  while( HaArrayCount(task->all_monitors) > 0 )
    KheOrdinaryDemandM onitorDelete(HaArrayLast(task->all_monitors));
  HnAssert(HaArrayCount(task->attached_monitors) == 0,
    "KheTaskMatchingEnd internal error");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskMatchingAttachAllOrdinaryDemandMonitors(KHE_TASK task)       */
/*                                                                           */
/*  Ensure that all the ordinary demand monitors of task are attached.       */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheTaskMatchingAttachAllOrdinaryDemandMonitors(KHE_TASK task)
{
  KHE_ORDINARY_DEMAND_MONITOR m;  int i;
  HaArrayForEach(task->all_monitors, m, i)
    if( !KheMonitorAttachedToSoln((KHE_MONITOR) m) )
      KheMonitorAttachToSoln((KHE_MONITOR) m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskMatchingDetachAllOrdinaryDemandMonitors(KHE_TASK task)       */
/*                                                                           */
/*  Ensure that all the ordinary demand monitors of task are detached.       */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheTaskMatchingDetachAllOrdinaryDemandMonitors(KHE_TASK task)
{
  while( HaArrayCount(task->attached_monitors) > 0 )
    KheMonitorDetachFromSoln((KHE_MONITOR) HaArrayLast(task->attached_monitors));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_DEMAND_CHUNK KheTaskDemandChunk(KHE_TASK task, int offset)  */
/*                                                                           */
/*  Return the demand chunk for task at offset.                              */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_DEMAND_CHUNK KheTaskDemandChunk(KHE_TASK task, int offset)
{
  HnAssert(task->meet != NULL,
    "KheOrdinaryDemandMonitorMake: task has no meet");
  HnAssert(KheSolnMatching(task->soln) != NULL,
    "KheOrdinaryDemandMonitorMake: there is no matching");
  HnAssert(0 <= offset && offset < KheMeetDuration(task->meet),
    "KheOrdinaryDemandMonitorMake: offset (%d) out of range (0 .. %d)",
    offset, KheMeetDuration(task->meet) - 1);
  return KheMeetDemandChunk(task->meet, offset);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskMatchingSetWeight(KHE_TASK task, KHE_COST new_weight)        */
/*                                                                           */
/*  Change the weight of all the attached monitors of task.                  */
/*  NB it would actually be wrong to do this for all monitors.               */
/*                                                                           */
/*****************************************************************************/

void KheTaskMatchingSetWeight(KHE_TASK task, KHE_COST new_weight)
{
  KHE_ORDINARY_DEMAND_MONITOR m;  int i;
  HaArrayForEach(task->attached_monitors, m, i)
    KheOrdinaryDemandMonitorSetWeight(m, new_weight);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskDemandMonitorCount(KHE_TASK task)                             */
/*                                                                           */
/*  Return the number of demand monitors of task.                            */
/*                                                                           */
/*****************************************************************************/

int KheTaskDemandMonitorCount(KHE_TASK task)
{
  return HaArrayCount(task->all_monitors);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ORDINARY_DEMAND_MONITOR KheTaskDemandMonitor(KHE_TASK task, int i)   */
/*                                                                           */
/*  Return the i'th demand monitor of task.                                  */
/*                                                                           */
/*****************************************************************************/

KHE_ORDINARY_DEMAND_MONITOR KheTaskDemandMonitor(KHE_TASK task, int i)
{
  return HaArray(task->all_monitors, i);
}


/*****************************************************************************/
/*                                                                           */
/* void KheTaskAddDemandMonitor(KHE_TASK task, KHE_ORDINARY_DEMAND_MONITOR m)*/
/*                                                                           */
/*  Add m to task.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheTaskAddDemandMonitor(KHE_TASK task, KHE_ORDINARY_DEMAND_MONITOR m)
{
  HaArrayAddLast(task->all_monitors, m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDeleteDemandMonitor(KHE_TASK task,                           */
/*    KHE_ORDINARY_DEMAND_MONITOR m)                                         */
/*                                                                           */
/*  Remove m from task.                                                      */
/*                                                                           */
/*****************************************************************************/

void KheTaskDeleteDemandMonitor(KHE_TASK task, KHE_ORDINARY_DEMAND_MONITOR m)
{
  int pos;
  if( !HaArrayContains(task->all_monitors, m, &pos) )
    HnAbort("KheTaskDeleteDemandMonitor internal error");
  HaArrayDeleteAndShift(task->all_monitors, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskAttachDemandMonitor(KHE_TASK task,                           */
/*    KHE_ORDINARY_DEMAND_MONITOR m)                                         */
/*                                                                           */
/*  Attach m to task.                                                        */
/*                                                                           */
/*****************************************************************************/

void KheTaskAttachDemandMonitor(KHE_TASK task, KHE_ORDINARY_DEMAND_MONITOR m)
{
  HaArrayAddLast(task->attached_monitors, m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDetachDemandMonitor(KHE_TASK task,                           */
/*    KHE_ORDINARY_DEMAND_MONITOR m)                                         */
/*                                                                           */
/*  Detach m from task.                                                      */
/*                                                                           */
/*****************************************************************************/

void KheTaskDetachDemandMonitor(KHE_TASK task, KHE_ORDINARY_DEMAND_MONITOR m)
{
  int pos;
  if( !HaArrayContains(task->attached_monitors, m, &pos) )
    HnAbort("KheTaskDetachDemandMonitor internal error");
  HaArrayDeleteAndShift(task->attached_monitors, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskMakeFromKml(KML_ELT task_elt, KHE_MEET meet, KML_ERROR *ke)  */
/*                                                                           */
/*  Make a task based on task_elt and add it to meet.                        */
/*                                                                           */
/*****************************************************************************/

bool KheTaskMakeFromKml(KML_ELT task_elt, KHE_MEET meet, KML_ERROR *ke)
{
  char *role, *ref;  KHE_TASK task;  KHE_INSTANCE ins;
  KHE_EVENT_RESOURCE er;  KHE_EVENT e;  HA_ARENA a;
  KHE_RESOURCE r, preassigned_r;
  KHE_RESOURCE_TYPE r_rt, er_rt;

  /* check task_elt */
  a = KheSolnArena(KheMeetSoln(meet));
  ins = KheSolnInstance(KheMeetSoln(meet));
  if( !KmlCheck(task_elt, "Reference : $Role", ke) &&
      !KmlCheck(task_elt, "R : $Role", ke) )
    return false;

  /* make sure the role makes sense; find the event resource */
  role = KmlText(KmlChild(task_elt, 0));
  e = KheMeetEvent(meet);
  HnAssert(e != NULL, "KheTaskMakeFromKml: no event (internal error)");
  if( !KheEventRetrieveEventResource(e, role, &er) )
    return KmlError(ke, a, KmlLineNum(task_elt), KmlColNum(task_elt),
      "<Role> \"%s\" unknown in <Event> \"%s\"", role, KheEventId(e));

  /* make sure that meet does not already have a task for this */
  if( KheMeetRetrieveTask(meet, role, &task) )
    return KmlError(ke, a, KmlLineNum(task_elt), KmlColNum(task_elt),
      "<Role> \"%s\" already assigned in <Event> \"%s\"", role, KheEventId(e));

  /* make sure the reference is to a legal resource */
  ref = KmlAttributeValue(task_elt, 0);
  if( !KheInstanceRetrieveResource(ins, ref, &r) )
    return KmlError(ke, a, KmlLineNum(task_elt), KmlColNum(task_elt),
      "<Resource> Reference \"%s\" unknown", ref);

  /* check that r is compatible with preassignment */
  preassigned_r = KheEventResourcePreassignedResource(er);
  if( preassigned_r != NULL && preassigned_r != r )
    return KmlError(ke, a, KmlLineNum(task_elt), KmlColNum(task_elt),
      "<Resource> \"%s\" conflicts with preassigned resource \"%s\"",
      ref, KheResourceId(preassigned_r));

  /* check that r is compatible with ResourceType */
  r_rt = KheResourceTypeOriginalResourceType(KheResourceResourceType(r));
  er_rt = KheResourceTypeOriginalResourceType(KheEventResourceResourceType(er));
  if( r_rt != er_rt )
    return KmlError(ke, a, KmlLineNum(task_elt), KmlColNum(task_elt),
      "<Resource> of type \"%s\" where type \"%s\" expected",
      KheResourceTypeId(r_rt), KheResourceTypeId(er_rt));

  /* make a task, link it to meet, and assign r to it */
  task = KheTaskMake(KheMeetSoln(meet), KheResourceResourceType(r), meet, er);
  if( !KheTaskAssignResource(task, r) )
    return KmlError(ke, a, KmlLineNum(task_elt), KmlColNum(task_elt),
      "<Resource> \"%s\" unassignable here", ref);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskCheckForWriting(KHE_TASK task)                               */
/*                                                                           */
/*  Check that task can be written safely; abort with error message if not.  */
/*                                                                           */
/*****************************************************************************/

void KheTaskCheckForWriting(KHE_TASK task)
{
  KHE_RESOURCE ass_r, pre_r;  KHE_EVENT e;  char *role;  KHE_EVENT_RESOURCE er;
  KHE_INSTANCE ins;

  /* only interested in tasks derived from event resources */
  er = KheTaskEventResource(task);
  if( er == NULL )
    return;

  /* get assigned and preassigned resources and check that they have Ids */
  role = KheEventResourceRole(er);
  e = KheEventResourceEvent(er);
  ass_r = KheTaskAsstResource(task);
  pre_r = KheEventResourcePreassignedResource(er);
  ins = KheEventInstance(e);
  HnAssert(ass_r == NULL || KheResourceId(ass_r) != NULL,
    "KheArchiveWrite: in instance %s, resource without Id assigned to "
    "event %s (role %s)", KheInstanceId(ins), KheEventId(e),
    role == NULL ? "(none)" : role);
  HnAssert(pre_r == NULL || KheResourceId(pre_r) != NULL,
    "KheArchiveWrite: in instance %s, resource without Id preassigned to "
    "event %s (role %s)", KheInstanceId(ins), KheEventId(e),
    role == NULL ? "(none)" : role);

  if( pre_r != NULL )
  {
    /* if preassigned, check that the assigned resource is equal to it */
    HnAssert(ass_r != NULL,
      "KheArchiveWrite: in event %s of instance %s, event resource with "
      "preassigned resource %s has task with missing resource assignment",
      KheEventId(e), KheInstanceId(ins), KheResourceId(pre_r));
    HnAssert(ass_r == pre_r,
      "KheArchiveWrite: in event %s of instance %s, event resource with "
      "preassigned resource %s has task with inconsistent assignment %s",
      KheEventId(e), KheInstanceId(ins), KheResourceId(pre_r),
      KheResourceId(ass_r));
  }
  else
  {
    /* if unpreassigned, must have a role */
    HnAssert(role != NULL, "KheArchiveWrite: in event %s of instance %s, "
      "unpreassigned event resource has no role", KheEventId(e),
      KheInstanceId(ins));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskMustWrite(KHE_TASK task, KHE_EVENT_RESOURCE er)              */
/*                                                                           */
/*  Return true if it is necessary to write task, because it is derived      */
/*  from an event resource and assigned a value which is not preassigned.    */
/*  This function assumes that KheTaskCheckForWriting returned successfully. */
/*                                                                           */
/*****************************************************************************/

bool KheTaskMustWrite(KHE_TASK task)
{
  KHE_RESOURCE ass_r;  KHE_EVENT_RESOURCE er;
  er = KheTaskEventResource(task);
  if( er == NULL )
    return false;
  ass_r = KheTaskAsstResource(task);
  return ass_r != NULL && ass_r != KheEventResourcePreassignedResource(er);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskWrite(KHE_TASK task, KML_FILE kf)                            */
/*                                                                           */
/*  Write task to kf.                                                        */
/*                                                                           */
/*  This function assumes that KheTaskCheckForWriting and KheTaskMustWrite   */
/*  returned successfully, and that the event from which the meet            */
/*  containing this task is derived has an Id.                               */
/*                                                                           */
/*****************************************************************************/

void KheTaskWrite(KHE_TASK task, KML_FILE kf)
{
  KHE_RESOURCE ass_r;  KHE_EVENT_RESOURCE er;

  /* print assigned resource (must be present, different from preassigned) */
  ass_r = KheTaskAsstResource(task);
  er = KheTaskEventResource(task);
  KmlEltAttributeEltPlainText(kf, "Resource", "Reference",
    KheResourceId(ass_r), "Role", KheEventResourceRole(er));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *KheTaskId(KHE_TASK task)                                           */
/*                                                                           */
/*  Return the Id of task.                                                   */
/*                                                                           */
/*****************************************************************************/

char *KheTaskId(KHE_TASK task)
{
  KHE_RESOURCE r;  HA_ARENA a;  KHE_EVENT_RESOURCE er;
  if( task->id == NULL )
  {
    a = KheSolnArena(task->soln);
    if( KheTaskIsCycleTask(task) )
    {
      r = KheResourceInSolnResource(task->assigned_rs);
      task->id = HnStringMake(a, "/%s/",
	KheResourceId(r) != NULL ? KheResourceId(r) : "-");
    }
    else if( task->meet == NULL )
    {
      if( DEBUG11 )
      {
	fprintf(stderr, "[KheTaskId soln_index %d, meet_index %d, "
	  "assigned_rs %p, meet %p, ers %p]", task->soln_index,
          task->meet_index, (void *) task->assigned_rs,
	  (void *) task->meet, (void *) task->event_resource_in_soln);
	HnAbort("KheTaskId aborting now");
      }
      task->id = HnStringMake(a, "/?/");
    }
    else
    {
      er = KheTaskEventResource(task);
      if( er != NULL )
	task->id = HnStringMake(a, "%s.%d", KheMeetId(task->meet),
	  KheEventResourceEventIndex(er));
      else
	task->id = HnStringMake(a, "%s.?", KheMeetId(task->meet));
    }
  }
  return task->id;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoDebug(KHE_TASK task, int verbosity, FILE *fp)              */
/*                                                                           */
/*  A basic debug print identifying task, and if it has an assigned          */
/*  time, when it is running.                                                */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoDebug(KHE_TASK task, int verbosity, FILE *fp)
{
  KHE_RESOURCE r;  KHE_TIME t;  int i;  KHE_TASK child_task;

  /* print the task itself */
  if( KheTaskIsCycleTask(task) )
  {
    r = KheResourceInSolnResource(task->assigned_rs);
    fprintf(fp, "/%s/", KheResourceId(r) != NULL ? KheResourceId(r) : "-");
  }
  else if( task->meet == NULL )
    fprintf(fp, "??");
  else
  {
    KheMeetDebug(task->meet, 1, -1, fp);
    fprintf(fp, ".%d", task->meet_index);
    /* ***
    fprintf(fp, ".%d%c", task->meet_index, KheTaskVisited(task, 0) ? 'v' : 'u');
    *** */
    if( verbosity >= 2 )
    {
      t = KheMeetAsstTime(task->meet);
      if( t != NULL && KheEventPreassignedTime(KheMeetEvent(task->meet)) != t )
      {
	fprintf(fp, "$%s", KheTimeId(t) == NULL ? "-" : KheTimeId(t));
	if( KheTaskDuration(task) > 1 )
	{
	  i = KheTimeIndex(t) + KheTaskDuration(task) - 1;
	  t = KheInstanceTime(KheSolnInstance(KheTaskSoln(task)), i);
	  fprintf(fp, "-%s", KheTimeId(t) == NULL ? "-" : KheTimeId(t));
	}
      }
    }
  }

  /* print the tasks assigned to task */
  if( verbosity >= 2 && HaArrayCount(task->assigned_tasks) > 0 )
  {
    fprintf(fp, "{");
    HaArrayForEach(task->assigned_tasks, child_task, i)
    {
      if( i > 0 )
	fprintf(fp, ", ");
      KheTaskDoDebug(child_task, 2, fp);
    }
    fprintf(fp, "}");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDebug(KHE_TASK task, int verbosity, int indent, FILE *fp)    */
/*                                                                           */
/*  Debug print of task onto fp with the given verbosity and indent.         */
/*                                                                           */
/*    Verbosity 0:  print nothing, as usual.                                 */
/*    Verbosity 1:  just enough to identify the task.                        */
/*    Verbosity 2:  add the tasks assigned to this task.                     */
/*    Verbosity 3:  add domain and what this task is assigned to.            */
/*                                                                           */
/*****************************************************************************/

void KheTaskDebug(KHE_TASK task, int verbosity, int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    /* initial indenting and verbosity stuff */
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    if( verbosity >= 3 )
      fprintf(fp, "[ ");

    /* print task and the tasks assigned to it */
    KheTaskDoDebug(task, verbosity, fp);

    /* print domain and assignment */
    if( DEBUG_DOMAIN(task) || verbosity >= 3 )
    {
      fprintf(fp, ": ");
      KheResourceGroupDebug(task->domain, 1, -1, fp);
      if( task->assigned_rs != NULL )
      {
	fprintf(fp, " := ");
	KheResourceDebug(KheResourceInSolnResource(task->assigned_rs),
	  1, -1, fp);
      }
    }

    /* final indenting and verbosity stuff */
    if( verbosity >= 3 )
      fprintf(fp, " ]");
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}

/* *** old and weird
void KheTaskDebug(KHE_TASK task, int verbosity, int indent, FILE *fp)
{
  KHE_RESOURCE r;  KHE_TIME t;  KHE_TASK child_task;  int i, j, pos;
  ARRAY_KHE_EVENT_RESOURCE event_resources;  KHE_EVENT_RESOURCE er;
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    if( verbosity > 1 )
      fprintf(fp, "[ ");
    if( KheTaskIsCycleTask(task) )
    {
      r = KheResourceInSolnResource(task->assigned_rs);
      fprintf(fp, "/%s/", KheResourceId(r) != NULL ? KheResourceId(r) : "-");
    }
    else
    {
      if( task->meet != NULL )
      {
	KheMeetDebug(task->meet, 1, -1, fp);
	fprintf(fp, ".%d", task->meet_index);
	t = KheMeetAsstTime(task->meet);
	if(t != NULL && KheEventPreassignedTime(KheMeetEvent(task->meet)) != t)
	{
	  fprintf(stderr, "$%s", KheTimeId(t) == NULL ? "-" : KheTimeId(t));
	  if( KheTaskDuration(task) > 1 )
	  {
	    i = KheTimeIndex(t) + KheTaskDuration(task) - 1;
	    t = KheInstanceTime(KheSolnInstance(KheTaskSoln(task)), i);
	    fprintf(stderr, "-%s", KheTimeId(t) == NULL ? "-" : KheTimeId(t));
	  }
	}
      }
      if( HaArrayCount(task->assigned_tasks) > 0 )
      {
	HaArrayInit(event_resources);
	HaArrayForEach(task->assigned_tasks, child_task, i)
	{
	  er = KheTaskEventResource(child_task);
	  if( !HaArrayContains(event_resources, er, &pos) )
	    HaArrayAddLast(event_resources, er); ** may add NULL **
	}
	fprintf(fp, "{");
	HaArrayForEach(event_resources, er, i)
	{
	  if( i > 0 )
	    fprintf(fp, ", ");
	  fprintf(fp, "%s", er == NULL ? "??" :
	    KheEventId(KheEventResourceEvent(er)) == NULL ?  "-" :
	    KheEventId(KheEventResourceEvent(er)));
	  HaArrayForEach(task->assigned_tasks, child_task, j)
	    if( KheTaskEventResource(child_task) == er )
	    {
	      if( child_task->meet == NULL )
		fprintf(fp, "+??");
	      else if( KheMeetAsstTime(child_task->meet) == NULL )
		fprintf(fp, "+__");
	      else
	      {
		t = KheMeetAsstTime(child_task->meet);
		fprintf(fp, "+%s", KheTimeId(t) == NULL ? "-" : KheTimeId(t));
	      }
	    }
	}
	fprintf(fp, "}");
	MArrayFree(event_resources);
      }
    }
    if( verbosity >= 2 )
    {
      fprintf(fp, ": ");
      KheResourceGroupDebug(task->domain, 1, -1, fp);
      if( task->assigned_rs != NULL )
      {
	fprintf(fp, " := ");
	KheResourceDebug(KheResourceInSolnResource(task->assigned_rs),
	  1, -1, fp);
      }
    }
    if( verbosity > 1 )
      fprintf(fp, " ]");
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}
*** */

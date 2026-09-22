
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
/*  FILE:         khe_task_set.c                                             */
/*  DESCRIPTION:  Task sets                                                  */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_SET                                                             */
/*                                                                           */
/*****************************************************************************/

struct khe_task_set_rec
{
  KHE_SOLN			soln;			/* encl solution     */
  ARRAY_KHE_TASK		tasks;			/* soln tasks        */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_SET KheTaskSetMake(KHE_SOLN soln)                               */
/*                                                                           */
/*  Make a new task set with these attributes.                               */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_SET KheTaskSetMake(KHE_SOLN soln)
{
  KHE_TASK_SET res;  HA_ARENA a;
  res = KheSolnGetTaskSetFromFreeList(soln);
  if( res == NULL )
  {
    /* MMakeCount(res, 50); */
    a = KheSolnArena(soln);
    HaMake(res, a);
    HaArrayInit(res->tasks, a);
  }
  else
    HaArrayClear(res->tasks);
  res->soln = soln;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheTaskSetSoln(KHE_TASK_SET ts)                                 */
/*                                                                           */
/*  Return the solution that ts is for.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheTaskSetSoln(KHE_TASK_SET ts)
{
  return ts->soln;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetDelete(KHE_TASK_SET ts)                                   */
/*                                                                           */
/*  Delete ts, that is, return it to its solution's free list.               */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetDelete(KHE_TASK_SET ts)
{
  KheSolnAddTaskSetToFreeList(ts->soln, ts);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetFree(KHE_TASK_SET ts)                                     */
/*                                                                           */
/*  Free ts.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheTaskSetFree(KHE_TASK_SET ts)
{
  MArrayFree(ts->tasks);
  MFree(ts);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetClear(KHE_TASK_SET ts)                                    */
/*                                                                           */
/*  Clear ts back to the empty set of tasks.                                 */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetClear(KHE_TASK_SET ts)
{
  HaArrayClear(ts->tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetClearFromEnd(KHE_TASK_SET ts, int count)                  */
/*                                                                           */
/*  Remove tasks from the end of ts until it contains count elements.        */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetClearFromEnd(KHE_TASK_SET ts, int count)
{
  int n = HaArrayCount(ts->tasks);
  HnAssert(count <= n, "KheTaskSetClearFromEnd: count exceeds size of set");
  HaArrayDeleteLastSlice(ts->tasks, n - count);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetDropFromEnd(KHE_TASK_SET ts, int n)                       */
/*                                                                           */
/*  Remove the last n tasks from ts.                                         */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetDropFromEnd(KHE_TASK_SET ts, int n)
{
  HaArrayDeleteLastSlice(ts->tasks, n);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetAddTask(KHE_TASK_SET ts, KHE_TASK task)                   */
/*                                                                           */
/*  Add task to ts.                                                          */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetAddTask(KHE_TASK_SET ts, KHE_TASK task)
{
  HaArrayAddLast(ts->tasks, task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetAddTaskSet(KHE_TASK_SET ts, KHE_TASK_SET ts2)             */
/*                                                                           */
/*  Add the tasks of ts2 to ts.                                              */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetAddTaskSet(KHE_TASK_SET ts, KHE_TASK_SET ts2)
{
  int i;
  HaArrayAppend(ts->tasks, ts2->tasks, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetAddTaskSorted(KHE_TASK_SET ts, KHE_TASK task,             */
/*    int(*compar)(const void *, const void *))                              */
/*                                                                           */
/*  Like KheTaskSetAddTask, only keep the tasks in sorted order by compar.   */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
void KheTaskSetAddTaskSorted(KHE_TASK_SET ts, KHE_TASK task,
  int(*compar)(const void *, const void *))
{
  ** int i;  KHE_TASK *task_p; **
  HaArrayAddLast(ts->tasks, task);
  ** *** can't get this to work, don't know why not
  task_p = &HaArrayLast(ts->tasks);
  for( i = HaArrayCount(ts->tasks) - 2;  i >= 0;  i-- )
  {
    if( compar((void *) &HaArray(ts->tasks, i), (void *) task_p) <= 0 )
      break;
    HaArrayPut(ts->tasks, i + 1, HaArray(ts->tasks, i));
  }
  HaArrayPut(ts->tasks, i + 1, task);
  *** **
  KheTaskSetSort(ts, compar);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetDeleteTask(KHE_TASK_SET ts, KHE_TASK task)                */
/*                                                                           */
/*  Delete task from ts.                                                     */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetDeleteTask(KHE_TASK_SET ts, KHE_TASK task)
{
  int pos;
  if( !HaArrayContains(ts->tasks, task, &pos) )
    HnAbort("KheTaskSetDeleteTask:  ts does not contain task");
  HaArrayDeleteAndPlug(ts->tasks, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskSetLastAndDelete(KHE_TASK_SET ts)                        */
/*                                                                           */
/*  Delete and return the last task of ts.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheTaskSetLastAndDelete(KHE_TASK_SET ts)
{
  HnAssert(HaArrayCount(ts->tasks) > 0, "KheTaskSetLastAndDelete: ts is empty");
  return HaArrayLastAndDelete(ts->tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetContainsTask(KHE_TASK_SET ts, KHE_TASK task, int *pos)    */
/*                                                                           */
/*  Return true and set *pos if ts contains task.                            */
/*                                                                           */
/*****************************************************************************/

bool KheTaskSetContainsTask(KHE_TASK_SET ts, KHE_TASK task, int *pos)
{
  return HaArrayContains(ts->tasks, task, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskSetTaskCount(KHE_TASK_SET ts)                                 */
/*                                                                           */
/*  Return the number of tasks in ts.                                        */
/*                                                                           */
/*****************************************************************************/

int KheTaskSetTaskCount(KHE_TASK_SET ts)
{
  return HaArrayCount(ts->tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskSetTask(KHE_TASK_SET ts, int i)                          */
/*                                                                           */
/*  Return the i'th task of ts.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheTaskSetTask(KHE_TASK_SET ts, int i)
{
  return HaArray(ts->tasks, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskSetFirst(KHE_TASK_SET ts)                                */
/*                                                                           */
/*  Return the first task of ts.                                             */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheTaskSetFirst(KHE_TASK_SET ts)
{
  return HaArrayFirst(ts->tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskSetLast(KHE_TASK_SET ts)                                 */
/*                                                                           */
/*  Return the last task of ts.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheTaskSetLast(KHE_TASK_SET ts)
{
  return HaArrayLast(ts->tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetSort(KHE_TASK_SET ts,                                     */
/*    int(*compar)(const void *, const void *))                              */
/*                                                                           */
/*  Sort the tasks of ts.                                                    */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetSort(KHE_TASK_SET ts, int(*compar)(const void *, const void *))
{
  HaArraySort(ts->tasks, compar);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetSortUnique(KHE_TASK_SET ts,                               */
/*    int(*compar)(const void *, const void *))                              */
/*                                                                           */
/*  Sort and uniquefy the tasks of ts.                                       */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetSortUnique(KHE_TASK_SET ts,
  int(*compar)(const void *, const void *))
{
  HaArraySortUnique(ts->tasks, compar);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskSetTotalDuration(KHE_TASK_SET ts)                             */
/*                                                                           */
/*  Return the total duration of the tasks of ts.                            */
/*                                                                           */
/*****************************************************************************/

int KheTaskSetTotalDuration(KHE_TASK_SET ts)
{
  int res, i;  KHE_TASK task;
  res = 0;
  HaArrayForEach(ts->tasks, task, i)
    res += KheTaskTotalDuration(task);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  float KheTaskSetTotalWorkload(KHE_TASK_SET ts)                           */
/*                                                                           */
/*  Return the total workload of the tasks of ts.                            */
/*                                                                           */
/*****************************************************************************/

float KheTaskSetTotalWorkload(KHE_TASK_SET ts)
{
  float res;  int i;  KHE_TASK task;
  res = 0.0;
  HaArrayForEach(ts->tasks, task, i)
    res += KheTaskTotalWorkload(task);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetUnGroup(KHE_TASK_SET ts)                                  */
/*                                                                           */
/*  Ungroup the tasks of ts.  That is, unassign them from their              */
/*  immediate parents, but then assign them to the resources that            */
/*  those parents are assigned to.                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheTaskSetUnGroup(KHE_TASK_SET ts)
{
  KHE_TASK task;  int i;  KHE_RESOURCE r;
  HaArrayForEach(ts->tasks, task, i)
  {
    r = KheTaskAsstResource(task);
    if( KheTaskUnAssign(task) && r != NULL )
      KheTaskAssignResource(task, r);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetNeedsAssignment(KHE_TASK_SET ts)                          */
/*                                                                           */
/*  Return true if at least one of the tasks of ts needs assignment.         */
/*                                                                           */
/*****************************************************************************/

bool KheTaskSetNeedsAssignment(KHE_TASK_SET ts)
{
  KHE_TASK task;  int i;
  HaArrayForEach(ts->tasks, task, i)
    if( KheTaskNeedsAssignment(task) )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetSetVisitNum(KHE_TASK_SET ts, int num)                     */
/*                                                                           */
/*  Call KheTaskSetVisitNum on each task of ts.                              */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetSetVisitNum(KHE_TASK_SET ts, int num)
{
  KHE_TASK task;  int i;
  HaArrayForEach(ts->tasks, task, i)
    KheTaskSetVisitNum(task, num);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskSetGetVisitNum(KHE_TASK_SET ts)                               */
/*                                                                           */
/*  Call KheTaskVisitNum on any task of ts.                                  */
/*                                                                           */
/*****************************************************************************/

int KheTaskSetGetVisitNum(KHE_TASK_SET ts)
{
  HnAssert(HaArrayCount(ts->tasks) > 0, "KheTaskSetGetVisitNum:  ts is empty");
  return KheTaskVisitNum(HaArrayFirst(ts->tasks));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetAllVisited(KHE_TASK_SET ts, int slack)                    */
/*                                                                           */
/*  Return true if all calls to KheTaskVisited(task, slack) for task in      */
/*  ts return true.                                                          */
/*                                                                           */
/*****************************************************************************/

bool KheTaskSetAllVisited(KHE_TASK_SET ts, int slack)
{
  KHE_TASK task;  int i;
  HaArrayForEach(ts->tasks, task, i)
    if( !KheTaskVisited(task, slack) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetAnyVisited(KHE_TASK_SET ts, int slack)                    */
/*                                                                           */
/*  Return true if any call to KheTaskVisited(task, slack) for task in       */
/*  ts returns true.                                                         */
/*                                                                           */
/*****************************************************************************/

bool KheTaskSetAnyVisited(KHE_TASK_SET ts, int slack)
{
  KHE_TASK task;  int i;
  HaArrayForEach(ts->tasks, task, i)
    if( KheTaskVisited(task, slack) )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetAllVisit(KHE_TASK_SET ts)                                 */
/*                                                                           */
/*  Mark the tasks of ts visited.                                            */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetAllVisit(KHE_TASK_SET ts)
{
  KHE_TASK task;  int i;
  HaArrayForEach(ts->tasks, task, i)
    KheTaskVisit(task);
  if( DEBUG1 )
  {
    fprintf(stderr, "  KheTaskSetAllVisit(");
    KheTaskSetDebug(ts, 2, -1, stderr);
    fprintf(stderr, ")\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetAllUnVisit(KHE_TASK_SET ts)                               */
/*                                                                           */
/*  Mark the tasks of ts unvisited.                                          */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetAllUnVisit(KHE_TASK_SET ts)
{
  KHE_TASK task;  int i;
  HaArrayForEach(ts->tasks, task, i)
    KheTaskUnVisit(task);
  if( DEBUG1 )
  {
    fprintf(stderr, "  KheTaskSetAllUnVisit(");
    KheTaskSetDebug(ts, 2, -1, stderr);
    fprintf(stderr, ")\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetMoveResourceCheck(KHE_TASK_SET ts, KHE_RESOURCE r)        */
/*                                                                           */
/*  Return true if the tasks of ts can be moved to r.                        */
/*                                                                           */
/*****************************************************************************/

bool KheTaskSetMoveResourceCheck(KHE_TASK_SET ts, KHE_RESOURCE r)
{
  KHE_TASK task;  int i;
  /* ***
  if( HaArrayCount(ts->tasks) == 0 )
    return false;
  *** */
  HaArrayForEach(ts->tasks, task, i)
    if( !KheTaskMoveResourceCheck(task, r) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetMoveResource(KHE_TASK_SET ts, KHE_RESOURCE r)             */
/*                                                                           */
/*  Move the tasks of ts to r, returning true if all successful.  If         */
/*  false is returned, some may be moved and others not.                     */
/*                                                                           */
/*****************************************************************************/

bool KheTaskSetMoveResource(KHE_TASK_SET ts, KHE_RESOURCE r)
{
  KHE_TASK task;  int i;
  /* ***
  if( HaArrayCount(ts->tasks) == 0 )
    return false;
  *** */
  HaArrayForEach(ts->tasks, task, i)
    if( !KheTaskMoveResource(task, r) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetAssignResourceCheck(KHE_TASK_SET ts, KHE_RESOURCE r)      */
/*                                                                           */
/*  Return true if the tasks of ts can be assigned r.                        */
/*                                                                           */
/*****************************************************************************/

bool KheTaskSetAssignResourceCheck(KHE_TASK_SET ts, KHE_RESOURCE r)
{
  KHE_TASK task;  int i;
  /* ***
  if( HaArrayCount(ts->tasks) == 0 )
    return false;
  *** */
  HaArrayForEach(ts->tasks, task, i)
    if( !KheTaskAssignResourceCheck(task, r) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetAssignResource(KHE_TASK_SET ts, KHE_RESOURCE r)           */
/*                                                                           */
/*  Assign r to the tasks of ts, returning true if all successful.  If       */
/*  false is returned, some may be assigned and others not.                  */
/*                                                                           */
/*****************************************************************************/

bool KheTaskSetAssignResource(KHE_TASK_SET ts, KHE_RESOURCE r)
{
  KHE_TASK task;  int i;
  /* ***
  if( HaArrayCount(ts->tasks) == 0 )
    return false;
  *** */
  HaArrayForEach(ts->tasks, task, i)
    if( !KheTaskAssignResource(task, r) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetUnAssignResourceCheck(KHE_TASK_SET ts)                    */
/*                                                                           */
/*  Return true if the tasks of ts can be unassigned.                        */
/*                                                                           */
/*****************************************************************************/

bool KheTaskSetUnAssignResourceCheck(KHE_TASK_SET ts)
{
  KHE_TASK task;  int i;
  /* ***
  if( HaArrayCount(ts->tasks) == 0 )
    return false;
  *** */
  HaArrayForEach(ts->tasks, task, i)
    if( !KheTaskUnAssignResourceCheck(task) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetUnAssignResource(KHE_TASK_SET ts)                         */
/*                                                                           */
/*  Unassign the tasks of ts, returning true if all successful.  If          */
/*  false is returned, some may be unassigned and others not.                */
/*                                                                           */
/*****************************************************************************/

bool KheTaskSetUnAssignResource(KHE_TASK_SET ts)
{
  KHE_TASK task;  int i;
  /* ***
  if( HaArrayCount(ts->tasks) == 0 )
    return false;
  *** */
  HaArrayForEach(ts->tasks, task, i)
    if( !KheTaskUnAssignResource(task) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetPartMoveResourceCheck(KHE_TASK_SET ts,                    */
/*    int first_index, int last_index, KHE_RESOURCE r)                       */
/*                                                                           */
/*  Like KheTaskSetMoveResourceCheck except that only the tasks between      */
/*  first_index and last_index (inclusive) are checked.                      */
/*                                                                           */
/*****************************************************************************/

bool KheTaskSetPartMoveResourceCheck(KHE_TASK_SET ts,
  int first_index, int last_index, KHE_RESOURCE r)
{
  KHE_TASK task;  int i;
  for( i = first_index;  i <= last_index;  i++ )
  {
    task = HaArray(ts->tasks, i);
    if( !KheTaskMoveResourceCheck(task, r) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetPartMoveResource(KHE_TASK_SET ts,                         */
/*    int first_index, int last_index, KHE_RESOURCE r)                       */
/*                                                                           */
/*  Like KheTaskSetMoveResource except that only the tasks between           */
/*  first_index and last_index (inclusive) are moved.                        */
/*                                                                           */
/*****************************************************************************/

bool KheTaskSetPartMoveResource(KHE_TASK_SET ts,
  int first_index, int last_index, KHE_RESOURCE r)
{
  KHE_TASK task;  int i;
  for( i = first_index;  i <= last_index;  i++ )
  {
    task = HaArray(ts->tasks, i);
    if( !KheTaskMoveResource(task, r) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetAssignFix(KHE_TASK_SET ts)                                */
/*                                                                           */
/*  Fix the tasks of ts.                                                     */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetAssignFix(KHE_TASK_SET ts)
{
  KHE_TASK task;  int i;
  HaArrayForEach(ts->tasks, task, i)
    KheTaskAssignFix(task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetAssignUnFix(KHE_TASK_SET ts)                              */
/*                                                                           */
/*  Unfix the tasks of ts.                                                   */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetAssignUnFix(KHE_TASK_SET ts)
{
  KHE_TASK task;  int i;
  HaArrayForEach(ts->tasks, task, i)
    KheTaskAssignUnFix(task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetOneLineDebug(KHE_TASK_SET ts, int verbosity, FILE *fp)    */
/*                                                                           */
/*  Print ts in full onto a single line of fp.                               */
/*                                                                           */
/*****************************************************************************/

static void KheTaskSetOneLineDebug(KHE_TASK_SET ts, int verbosity, FILE *fp)
{
  KHE_TASK task;  int i;
  fprintf(fp, "{");
  HaArrayForEach(ts->tasks, task, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    KheTaskDebug(task, verbosity, -1, fp);
  }
  fprintf(fp, "}");
}


/*****************************************************************************/
/*                                                                           */
/* void KheTaskSetDebug(KHE_TASK_SET ts, int verbosity, int indent, FILE *fp)*/
/*                                                                           */
/*  Debug print of task set ts onto fp with the given verbosity and indent.  */
/*                                                                           */
/*****************************************************************************/

void KheTaskSetDebug(KHE_TASK_SET ts, int verbosity, int indent, FILE *fp)
{
  KHE_TASK task;  int i;
  if( verbosity < 1 )
  {
    /* print nothing */
  }
  else if( indent < 0 )
  {
    if( HaArrayCount(ts->tasks) <= 3 )
    {
      /* complete one-line print */
      KheTaskSetOneLineDebug(ts, verbosity, fp);
    }
    else
    {
      /* incomplete one-line print */
      fprintf(fp, "{");
      task = HaArrayFirst(ts->tasks);
      KheTaskDebug(task, verbosity, -1, fp);
      fprintf(fp, ", ... (%d tasks)", HaArrayCount(ts->tasks));
      fprintf(fp, "}");
    }
  }
  else
  {
    fprintf(fp, "%*s", indent, "");
    if( HaArrayCount(ts->tasks) <= 5 )
    {
      /* complete one-line print */
      KheTaskSetOneLineDebug(ts, verbosity, fp);
    }
    else
    {
      /* complete multi-line print */
      fprintf(fp, "{ ");
      HaArrayForEach(ts->tasks, task, i)
      {
	if( i > 0 )
	{
	  if( i % 5 == 0 )
	    fprintf(fp, ",\n%*s", indent + 2, "");
	  else
	    fprintf(fp, ", ");
	}
	KheTaskDebug(task, verbosity, -1, fp);
      }
      /* fprintf(fp, "\n%*s", indent, ""); */
      fprintf(fp, " }");
    }
    fprintf(fp, "\n");
  }
}

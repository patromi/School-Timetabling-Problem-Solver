
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
/*  FILE:         khe_task_bound.c                                           */
/*  DESCRIPTION:  Task bounds                                                */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_BOUND - a task bound                                            */
/*                                                                           */
/*****************************************************************************/

struct khe_task_bound_rec {
  KHE_SOLN			soln;			/* enclosing soln    */
  /* int			soln_index; */		/* index in soln     */
  int				reference_count;	/* reference count   */
  KHE_RESOURCE_GROUP		resource_group;		/* resource group*/
  ARRAY_KHE_TASK		tasks;			/* tasks             */
  KHE_TASK_BOUND		copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "creation and deletion"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_BOUND KheTaskBoundDoMake(HA_ARENA a)                            */
/*                                                                           */
/*  Obtain a new task bound object from a.                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_BOUND KheTaskBoundDoMake(HA_ARENA a)
{
  KHE_TASK_BOUND res;
  HaMake(res, a);
  HaArrayInit(res->tasks, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskBoundUnMake(KHE_TASK_BOUND tb)                               */
/*                                                                           */
/*  Undo KheTaskBoundDoMake, returning tb's memory to the memory allocator.  */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheTaskBoundUnMake(KHE_TASK_BOUND tb)
{
  MArrayFree(tb->tasks);
  MFree(tb);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_BOUND KheTaskBoundDoGet(KHE_SOLN soln)                          */
/*                                                                           */
/*  Get a task bound object, either from soln's free list or allocated.      */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_BOUND KheTaskBoundDoGet(KHE_SOLN soln)
{
  KHE_TASK_BOUND res;
  res = KheSolnGetTaskBoundFromFreeList(soln);
  if( res == NULL )
    res = KheTaskBoundDoMake(KheSolnArena(soln));
  res->reference_count = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskBoundUnGet(KHE_TASK_BOUND tb)                                */
/*                                                                           */
/*  Undo KheTaskBoundDoGet, adding tb to its soln's free list.               */
/*                                                                           */
/*****************************************************************************/

static void KheTaskBoundUnGet(KHE_TASK_BOUND tb)
{
  KheSolnAddTaskBoundToFreeList(tb->soln, tb);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskBoundReferenceCountIncrement(KHE_TASK_BOUND tb)              */
/*                                                                           */
/*  Increment tb's reference count.                                          */
/*                                                                           */
/*****************************************************************************/

void KheTaskBoundReferenceCountIncrement(KHE_TASK_BOUND tb)
{
  tb->reference_count++;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskBoundReferenceCountDecrement(KHE_TASK_BOUND tb)              */
/*                                                                           */
/*  Decrement tb's reference count, and possibly add tb to the free list.    */
/*                                                                           */
/*****************************************************************************/

void KheTaskBoundReferenceCountDecrement(KHE_TASK_BOUND tb)
{
  HnAssert(tb->reference_count >= 1,
    "KheTaskBoundReferenceCountDecrement internal error");
  if( --tb->reference_count == 0 )
    KheTaskBoundUnGet(tb);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskBoundDoAdd(KHE_TASK_BOUND tb, KHE_SOLN soln,                 */
/*    KHE_RESOURCE_GROUP rg)                                                 */
/*                                                                           */
/*  Initialize tb and add it to soln (NB reference_count is already set).    */
/*                                                                           */
/*****************************************************************************/

void KheTaskBoundDoAdd(KHE_TASK_BOUND tb, KHE_SOLN soln, KHE_RESOURCE_GROUP rg)
{
  if( DEBUG1 )
  {
    fprintf(stderr, "  KheTaskBoundDoAdd(tb %p, ", (void *) tb);
    KheResourceGroupDebug(rg, 2, -1, stderr);
    fprintf(stderr, ")\n");
  }
  tb->soln = soln;
  /* KheSolnAddTaskBound(soln, tb);  will set soln and soln_index */
  KheTaskBoundReferenceCountIncrement(tb);
  tb->resource_group = rg;
  HaArrayClear(tb->tasks);
  tb->copy = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskBoundUnAdd(KHE_TASK_BOUND tb)                                */
/*                                                                           */
/*  Undo KheTaskBoundDoAdd, leaving tb unlinked from the solution.           */
/*                                                                           */
/*****************************************************************************/

static void KheTaskBoundUnAdd(KHE_TASK_BOUND tb)
{
  if( DEBUG1 )
    fprintf(stderr, "  KheTaskBoundUnAdd(tb %p)\n", (void *) tb);

  /* delete from soln */
  /* KheSolnDeleteTaskBound(tb->soln, tb); */

  /* tb is now not referenced from solution (this call may free tb) */
  KheTaskBoundReferenceCountDecrement(tb);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskBoundKernelAdd(KHE_TASK_BOUND tb, KHE_SOLN soln,             */
/*    KHE_RESOURCE_GROUP rg)                                                 */
/*                                                                           */
/*  Kernel operation which adds tb to soln (but does not make it).           */
/*                                                                           */
/*****************************************************************************/

void KheTaskBoundKernelAdd(KHE_TASK_BOUND tb, KHE_SOLN soln,
  KHE_RESOURCE_GROUP rg)
{
  if( DEBUG3 )
    fprintf(stderr, "KheTaskBoundKernelAdd(tb %p, ...)\n", (void *) tb);
  KheTaskBoundDoAdd(tb, soln, rg);
  KheSolnOpTaskBoundAdd(soln, tb, rg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskBoundKernelAddUndo(KHE_TASK_BOUND tb)                        */
/*                                                                           */
/*  Undo KheTaskBoundKernelAdd.                                              */
/*                                                                           */
/*****************************************************************************/

void KheTaskBoundKernelAddUndo(KHE_TASK_BOUND tb)
{
  if( DEBUG3 )
    fprintf(stderr, "KheTaskBoundKernelAddUndo(tb %p)\n", (void *) tb);
  KheTaskBoundUnAdd(tb);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskBoundKernelDelete(KHE_TASK_BOUND tb)                         */
/*                                                                           */
/*  Kernel operation which deletes tb (but does not free it).                */
/*                                                                           */
/*****************************************************************************/

void KheTaskBoundKernelDelete(KHE_TASK_BOUND tb)
{
  if( DEBUG3 )
    fprintf(stderr, "KheTaskBoundKernelDelete(tb %p)\n", (void *) tb);
  KheSolnOpTaskBoundDelete(tb->soln, tb, tb->resource_group);
  KheTaskBoundUnAdd(tb);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskBoundKernelDeleteUndo(KHE_TASK_BOUND tb, KHE_SOLN soln,      */
/*    KHE_RESOURCE_GROUP rg)                                                 */
/*                                                                           */
/*  Undo KheTaskBoundKernelDelete.                                           */
/*                                                                           */
/*****************************************************************************/

void KheTaskBoundKernelDeleteUndo(KHE_TASK_BOUND tb, KHE_SOLN soln,
  KHE_RESOURCE_GROUP rg)
{
  if( DEBUG3 )
    fprintf(stderr, "KheTaskBoundKernelDeleteUndo(tb %p, ...)\n", (void *) tb);
  KheTaskBoundDoAdd(tb, soln, rg);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_BOUND KheTaskBoundMake(KHE_SOLN soln, KHE_RESOURCE_GROUP rg)    */
/*                                                                           */
/*  Make a task bound with these attributes.                                 */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_BOUND KheTaskBoundMake(KHE_SOLN soln, KHE_RESOURCE_GROUP rg)
{
  KHE_TASK_BOUND res;

  /* make and initialize a new task bound object from scratch */
  HnAssert(rg != NULL, "KheTaskBoundMake: rg == NULL");
  res = KheTaskBoundDoGet(soln);

  /* add it to the soln and return it */
  KheTaskBoundKernelAdd(res, soln, rg);
  if( DEBUG2 )
  {
    fprintf(stderr, "%p = KheTaskBoundMake(soln, ", (void *) res);
    KheResourceGroupDebug(rg, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskBoundDeleteCheck(KHE_TASK_BOUND tb)                          */
/*                                                                           */
/*  Check whether it's safe to delete tb.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheTaskBoundDeleteCheck(KHE_TASK_BOUND tb)
{
  KHE_TASK task;  int i;
  HaArrayForEach(tb->tasks, task, i)
    if( !KheTaskDeleteTaskBoundCheck(task, tb) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskBoundDelete(KHE_TASK_BOUND tb)                               */
/*                                                                           */
/*  Delete tb, if safe.                                                      */
/*                                                                           */
/*****************************************************************************/

bool KheTaskBoundDelete(KHE_TASK_BOUND tb)
{
  /* ensure operation will succeed */
  if( !KheTaskBoundDeleteCheck(tb) )
    return false;

  /* delete tb from all its tasks */
  while( HaArrayCount(tb->tasks) > 0 )
    KheTaskDeleteTaskBound(HaArrayFirst(tb->tasks), tb);

  /* carry out the kernel delete operation */
  KheTaskBoundKernelDelete(tb);
  if( DEBUG2 )
    fprintf(stderr, "KheTaskBoundDelete(%p)\n", (void *) tb);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "copy"                                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_BOUND KheTaskBoundCopyPhase1(KHE_TASK_BOUND tb, HA_ARENA a)     */
/*                                                                           */
/*  Carry out phase 1 of copying tb.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_BOUND KheTaskBoundCopyPhase1(KHE_TASK_BOUND tb, HA_ARENA a)
{
  KHE_TASK_BOUND copy;  int i;  KHE_TASK task;
  if( tb->copy == NULL )
  {
    HaMake(copy, a);
    tb->copy = copy;
    copy->soln = KheSolnCopyPhase1(tb->soln);
    /* copy->soln_index = tb->soln_index; */
    copy->reference_count = 1;  /* no paths, and tb is linked in */
    copy->resource_group = KheResourceGroupCopy(tb->resource_group, a);
    HaArrayInit(copy->tasks, a);
    HaArrayForEach(tb->tasks, task, i)
      HaArrayAddLast(copy->tasks, KheTaskCopyPhase1(task, a));
    copy->copy = NULL;
  }
  return tb->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskBoundCopyPhase2(KHE_TASK_BOUND tb)                           */
/*                                                                           */
/*  Carry out phase 2 of copying tb.                                         */
/*                                                                           */
/*****************************************************************************/

void KheTaskBoundCopyPhase2(KHE_TASK_BOUND tb)
{
  int i;  KHE_TASK task;
  if( tb->copy != NULL )
  {
    tb->copy = NULL;
    KheSolnCopyPhase2(tb->soln);
    HaArrayForEach(tb->tasks, task, i)
      KheTaskCopyPhase2(task);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "simple attributes"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheTaskBoundSoln(KHE_TASK_BOUND tb)                             */
/*                                                                           */
/*  Return the enclosing solution of tb.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheTaskBoundSoln(KHE_TASK_BOUND tb)
{
  return tb->soln;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskBoundSetSoln(KHE_TASK_BOUND tb, KHE_SOLN soln)               */
/*                                                                           */
/*  Set the soln attribute of tb.                                            */
/*                                                                           */
/*****************************************************************************/

void KheTaskBoundSetSoln(KHE_TASK_BOUND tb, KHE_SOLN soln)
{
  tb->soln = soln;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskBoundSolnIndex(KHE_TASK_BOUND tb)                             */
/*                                                                           */
/*  Return the soln_index attribute of tb.                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheTaskBoundSolnIndex(KHE_TASK_BOUND tb)
{
  return tb->soln_index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskBoundSetSolnIndex(KHE_TASK_BOUND tb, int soln_index)         */
/*                                                                           */
/*  Set the soln_index attribute of tb.                                      */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheTaskBoundSetSolnIndex(KHE_TASK_BOUND tb, int soln_index)
{
  tb->soln_index = soln_index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheTaskBoundResourceGroup(KHE_TASK_BOUND tb)          */
/*                                                                           */
/*  Return the resource group of tb.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheTaskBoundResourceGroup(KHE_TASK_BOUND tb)
{
  return tb->resource_group;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "tasks"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskBoundAddTask(KHE_TASK_BOUND tb, KHE_TASK task)               */
/*                                                                           */
/*  Add task to tb.                                                          */
/*                                                                           */
/*****************************************************************************/

void KheTaskBoundAddTask(KHE_TASK_BOUND tb, KHE_TASK task)
{
  HnAssert(tb->reference_count > 0, "KheTaskBoundAddTask internal error");
  HaArrayAddLast(tb->tasks, task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskBoundDeleteTask(KHE_TASK_BOUND tb, KHE_TASK task)            */
/*                                                                           */
/*  Delete task from tb.                                                     */
/*                                                                           */
/*****************************************************************************/

void KheTaskBoundDeleteTask(KHE_TASK_BOUND tb, KHE_TASK task)
{
  int pos;
  HnAssert(tb->reference_count > 0, "KheTaskBoundDeleteTask internal error 1");
  if( !HaArrayContains(tb->tasks, task, &pos) )
    HnAbort("KheTaskBoundDeleteTask internal error 2");
  HaArrayDeleteAndPlug(tb->tasks, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskBoundTaskCount(KHE_TASK_BOUND tb)                             */
/*                                                                           */
/*  Return the number of tasks that tb applies to.                           */
/*                                                                           */
/*****************************************************************************/

int KheTaskBoundTaskCount(KHE_TASK_BOUND tb)
{
  return HaArrayCount(tb->tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskBoundTask(KHE_TASK_BOUND tb, int i)                      */
/*                                                                           */
/*  Return the ith task that tb applies to.                                  */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheTaskBoundTask(KHE_TASK_BOUND tb, int i)
{
  return HaArray(tb->tasks, i);
}

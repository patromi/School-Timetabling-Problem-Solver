
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
/*  FILE:         khe_sr_task_bound_group.c                                  */
/*  DESCRIPTION:  Task bound groups                                          */
/*                                                                           */
/*****************************************************************************/
#include <limits.h>
#include "khe_solvers.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_BOUND_GROUP - a set of task bounds                              */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_TASK_BOUND) ARRAY_KHE_TASK_BOUND;

struct khe_task_bound_group_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;
  ARRAY_KHE_TASK_BOUND		task_bounds;		/* the task bounds   */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_BOUND_GROUP KheTaskBoundGroupMake(KHE_SOLN soln)                */
/*                                                                           */
/*  Make a new task bound group object.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_BOUND_GROUP KheTaskBoundGroupMake(KHE_SOLN soln)
{
  KHE_TASK_BOUND_GROUP res;  HA_ARENA a;
  a = KheSolnArenaBegin(soln);
  /* a = HaAren aMake(); */
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  HaArrayInit(res->task_bounds, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskBoundGroupAddTaskBound(KHE_TASK_BOUND_GROUP tbg,             */
/*    KHE_TASK_BOUND tb)                                                     */
/*                                                                           */
/*  Add tb to tbg.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheTaskBoundGroupAddTaskBound(KHE_TASK_BOUND_GROUP tbg, KHE_TASK_BOUND tb)
{
  HaArrayAddLast(tbg->task_bounds, tb);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskBoundGroupTaskBoundCount(KHE_TASK_BOUND_GROUP tbg)            */
/*                                                                           */
/*  Return the number of task bounds in tbg.                                 */
/*                                                                           */
/*****************************************************************************/

int KheTaskBoundGroupTaskBoundCount(KHE_TASK_BOUND_GROUP tbg)
{
  return HaArrayCount(tbg->task_bounds);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_BOUND KheTaskBoundGroupTaskBound(KHE_TASK_BOUND_GROUP tbg,      */
/*    int i)                                                                 */
/*                                                                           */
/*  Return the i'th task bound of tbg.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_BOUND KheTaskBoundGroupTaskBound(KHE_TASK_BOUND_GROUP tbg, int i)
{
  return HaArray(tbg->task_bounds, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskBoundGroupDelete(KHE_TASK_BOUND_GROUP tbg)                   */
/*                                                                           */
/*  Delete tbg and the task bounds within it.  Return true if all calls to   */
/*  KheTaskBoundDelete returned true.                                        */
/*                                                                           */
/*****************************************************************************/

bool KheTaskBoundGroupDelete(KHE_TASK_BOUND_GROUP tbg)
{
  KHE_TASK_BOUND tb;  int i;  bool res;
  res = true;
  HaArrayForEach(tbg->task_bounds, tb, i)
    res = res && KheTaskBoundDelete(tb);
  KheSolnArenaEnd(tbg->soln, tbg->arena);
  /* HaAren aDelete(HaArrayArena(tbg->task_bounds)); */
  /* ***
  MArrayFree(tbg->task_bounds);
  MFree(tbg);
  *** */
  return res;
}

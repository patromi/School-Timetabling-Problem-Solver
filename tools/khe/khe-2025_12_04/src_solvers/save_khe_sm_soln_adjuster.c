
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
/*  FILE:         khe_sm_soln_adjuster.c                                     */
/*  DESCRIPTION:  Solution adjustments                                       */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_SA_OP_TYPE - the type of adjustment we are remembering               */
/*                                                                           */
/*****************************************************************************/

typedef enum {

  /* operations on monitors */
  KHE_SA_OP_MONITOR_ATTACH_TAG,
  KHE_SA_OP_MONITOR_DETACH_TAG,
  KHE_SA_OP_MONITOR_WEIGHT_TAG,
  KHE_SA_OP_MONITOR_TILT_TAG,

  /* operations on tasks */
  KHE_SA_OP_TASK_MOVE_TAG,
  KHE_SA_OP_TASK_FIX_TAG,
  KHE_SA_OP_TASK_UNFIX_TAG,
  KHE_SA_OP_TASK_GROUP_TAG,

  /* operations on task bounds */
  KHE_SA_OP_TASK_BOUND_MAKE_TAG
} KHE_SA_OP_TYPE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_ADJUSTMENT - one adjustment                                          */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_adjustment_rec {
  KHE_SA_OP_TYPE		type;
  KHE_MONITOR                   monitor;
  KHE_COST                      combined_weight;
  KHE_TASK			task;
  KHE_TASK			target_task;
  KHE_TASK_BOUND		task_bound;
} *KHE_ADJUSTMENT;

typedef HA_ARRAY(KHE_ADJUSTMENT) ARRAY_KHE_ADJUSTMENT;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_ADJUSTER - a monitor adjuster                                   */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_SOLN_ADJUSTER_AT_START,
  KHE_SOLN_ADJUSTER_AT_END
} KHE_SOLN_ADJUSTER_STATE;

struct khe_soln_adjuster_rec {
  KHE_SOLN			soln;
  HA_ARENA			arena;
  KHE_SOLN_ADJUSTER_STATE	state;
  ARRAY_KHE_ADJUSTMENT		adjustments;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_ADJUSTMENT"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ADJUSTMENT KheAdjustmentMake(KHE_SA_OP_TYPE type, KHE_MONITOR m,     */
/*    KHE_COST combined_weight, KHE_TASK task, KHE_TASK target_task,         */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Make an adjustment object with these attributes.                         */
/*                                                                           */
/*****************************************************************************/

static KHE_ADJUSTMENT KheAdjustmentMake(KHE_SA_OP_TYPE type, KHE_MONITOR m,
  KHE_COST combined_weight, KHE_TASK task, KHE_TASK target_task,
  KHE_TASK_BOUND task_bound, HA_ARENA a)
{
  KHE_ADJUSTMENT res;
  HaMake(res, a);
  res->type = type;
  res->monitor = m;
  res->combined_weight = combined_weight;
  res->task = task;
  res->target_task = target_task;
  res->task_bound = task_bound;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAdjustmentUndo(KHE_ADJUSTMENT am)                                */
/*                                                                           */
/*  Undo whatever am is remembering that we did.                             */
/*                                                                           */
/*****************************************************************************/

static void KheAdjustmentUndo(KHE_ADJUSTMENT am)
{
  /* KHE_RESOURCE r; */  KHE_TASK leader_task;
  switch( am->type )
  {
    case KHE_SA_OP_MONITOR_ATTACH_TAG:

      /* we attached, so detach now */
      KheMonitorDetachFromSoln(am->monitor);
      break;

    case KHE_SA_OP_MONITOR_DETACH_TAG:

      /* we detached, so attach now */
      KheMonitorAttachToSoln(am->monitor);
      break;

    case KHE_SA_OP_MONITOR_WEIGHT_TAG:

      /* we changed the weight, so change it back */
      KheMonitorSetCombinedWeight(am->monitor, am->combined_weight);
      break;

    case KHE_SA_OP_MONITOR_TILT_TAG:

      /* we set the tilt, so clear it */
      KheLimitActiveIntervalsMonitorClearTilt(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) am->monitor);
      break;

    case KHE_SA_OP_TASK_MOVE_TAG:

      /* we moved the task, so move it back again */
      KheTaskMove(am->task, am->target_task);
      break;

    case KHE_SA_OP_TASK_FIX_TAG:

      /* we fixed the task, so unfix it */
      KheTaskAssignUnFix(am->task);
      break;

    case KHE_SA_OP_TASK_UNFIX_TAG:

      /* we unfixed the task, so fix it */
      KheTaskAssignFix(am->task);
      break;

    case KHE_SA_OP_TASK_GROUP_TAG:

      /* we grouped the task, so ungroup it */
      leader_task = KheTaskAsst(am->task);
      HnAssert(leader_task != NULL,
	"KheSolnAdjusterDelete internal error (cannot ungroup task)");
      /* no, doing this separately now KheTaskAssignUnFix(am->task); */
      if( !KheTaskMove(am->task, KheTaskAsst(leader_task)) )
	HnAbort("KheSolnAdjusterDelete internal error (cannot ungroup task)");
      break;

    case KHE_SA_OP_TASK_BOUND_MAKE_TAG:

      /* we created am->task_bound, so delete it */
      KheTaskBoundDelete(am->task_bound);
      break;

    default:

      HnAbort("KheAdjustmentUndo internal error");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SOLN_ADJUSTER"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_ADJUSTER KheSolnAdjusterMake(KHE_SOLN soln)                     */
/*                                                                           */
/*  Make a monitor adjuster object.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN_ADJUSTER KheSolnAdjusterMake(KHE_SOLN soln)
{
  KHE_SOLN_ADJUSTER res;  HA_ARENA a;
  a = KheSolnArenaBegin(soln);
  HaMake(res, a);
  res->soln = soln;
  res->arena = a;
  res->state = KHE_SOLN_ADJUSTER_AT_END;
  HaArrayInit(res->adjustments, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAdjusterDelete(KHE_SOLN_ADJUSTER sa)                         */
/*                                                                           */
/*  Delete sa, first undoing all the adjustments (in reverse order).         */
/*                                                                           */
/*****************************************************************************/

void KheSolnAdjusterDelete(KHE_SOLN_ADJUSTER sa)
{
  KHE_ADJUSTMENT am;  int i;
  HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
    "KheSolnAdjusterDelete internal error (called after undo)");
  HaArrayForEachReverse(sa->adjustments, am, i)
    KheAdjustmentUndo(am);
  KheSolnArenaEnd(sa->soln, sa->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheSolnAdjusterSoln(KHE_SOLN_ADJUSTER sa)                       */
/*                                                                           */
/*  Return sa's solution.                                                    */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheSolnAdjusterSoln(KHE_SOLN_ADJUSTER sa)
{
  return sa->soln;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAdjusterUndo(KHE_SOLN_ADJUSTER sa)                           */
/*                                                                           */
/*  Undo sa.                                                                 */
/*                                                                           */
/*****************************************************************************/

void KheSolnAdjusterUndo(KHE_SOLN_ADJUSTER sa)
{
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAdjusterRedo(KHE_SOLN_ADJUSTER sa)                           */
/*                                                                           */
/*  Redo sa.                                                                 */
/*                                                                           */
/*****************************************************************************/

void KheSolnAdjusterRedo(KHE_SOLN_ADJUSTER sa)
{
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAdjusterMonitorEnsureAttached(KHE_SOLN_ADJUSTER sa,          */
/*    KHE_MONITOR m)                                                         */
/*                                                                           */
/*  Ensure that m is attached.                                               */
/*                                                                           */
/*****************************************************************************/

void KheSolnAdjusterMonitorEnsureAttached(KHE_SOLN_ADJUSTER sa, KHE_MONITOR m)
{
  KHE_ADJUSTMENT am;
  HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
    "KheSolnAdjusterMonitorEnsureAttached internal error (called after undo)");
  if( !KheMonitorAttachedToSoln(m) )
  {
    if( sa != NULL )
    {
      am = KheAdjustmentMake(KHE_SA_OP_MONITOR_ATTACH_TAG, m, 0,
	NULL, NULL, NULL, sa->arena);
      HaArrayAddLast(sa->adjustments, am);
    }
    KheMonitorAttachToSoln(m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAdjusterMonitorEnsureDetached(KHE_SOLN_ADJUSTER sa,          */
/*    KHE_MONITOR m)                                                         */
/*                                                                           */
/*  Ensure that m is detached.                                               */
/*                                                                           */
/*****************************************************************************/

void KheSolnAdjusterMonitorEnsureDetached(KHE_SOLN_ADJUSTER sa, KHE_MONITOR m)
{
  KHE_ADJUSTMENT am;
  HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
    "KheSolnAdjusterMonitorEnsureDetached internal error (called after undo)");
  if( KheMonitorAttachedToSoln(m) )
  {
    if( sa != NULL )
    {
      am = KheAdjustmentMake(KHE_SA_OP_MONITOR_DETACH_TAG, m, 0,
	NULL, NULL, NULL, sa->arena);
      HaArrayAddLast(sa->adjustments, am);
    }
    KheMonitorDetachFromSoln(m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAdjusterMonitorChangeWeight(KHE_SOLN_ADJUSTER sa,            */
/*    KHE_MONITOR m, KHE_COST combined_weight)                               */
/*                                                                           */
/*  Change the combined weight of m to combined_weight.                      */
/*                                                                           */
/*****************************************************************************/

void KheSolnAdjusterMonitorChangeWeight(KHE_SOLN_ADJUSTER sa,
  KHE_MONITOR m, KHE_COST combined_weight)
{
  KHE_ADJUSTMENT am;
  HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
    "KheSolnAdjusterMonitorChangeWeight internal error (called after undo)");
  if( combined_weight != KheMonitorCombinedWeight(m) )
  {
    if( sa != NULL )
    {
      am = KheAdjustmentMake(KHE_SA_OP_MONITOR_WEIGHT_TAG, m,
	KheMonitorCombinedWeight(m), NULL, NULL, NULL, sa->arena);
      HaArrayAddLast(sa->adjustments, am);
    }
    KheMonitorSetCombinedWeight(m, combined_weight);
    if( DEBUG1 )
    {
      fprintf(stderr, "  KheSolnAdjusterMonitorChangeWeight to %.5f: ",
	KheCostShow(combined_weight));
      KheMonitorDebug(m, 2, 0, stderr);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAdjusterMonitorSetTilt(KHE_SOLN_ADJUSTER sa,                 */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim)                               */
/*                                                                           */
/*  Set the tilt of laim, unless it's set already.                           */
/*                                                                           */
/*****************************************************************************/

void KheSolnAdjusterMonitorSetTilt(KHE_SOLN_ADJUSTER sa,
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim)
{
  KHE_ADJUSTMENT am;
  HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
    "KheSolnAdjusterMonitorSetTilt internal error (called after undo)");
  if( !KheLimitActiveIntervalsMonitorTilt(laim) )
  {
    if( sa != NULL )
    {
      am = KheAdjustmentMake(KHE_SA_OP_MONITOR_TILT_TAG,
	(KHE_MONITOR) laim, 0, NULL, NULL, NULL, sa->arena);
      HaArrayAddLast(sa->adjustments, am);
    }
    KheLimitActiveIntervalsMonitorSetTilt(laim);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAdjusterTaskMove(KHE_SOLN_ADJUSTER sa,                       */
/*    KHE_TASK task, KHE_TASK target_task)                                   */
/*                                                                           */
/*  Move task to target_task and remember that in sa.                        */
/*                                                                           */
/*****************************************************************************/

bool KheSolnAdjusterTaskMove(KHE_SOLN_ADJUSTER sa,
  KHE_TASK task, KHE_TASK target_task)
{
  KHE_ADJUSTMENT am;  KHE_TASK init_target_task;
  HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
    "KheSolnAdjusterTaskMove internal error (called after undo)");
  HnAssert(task != NULL, "KheSolnAdjusterTaskMove internal error");
  if( KheTaskMoveCheck(task, target_task) )
  {
    init_target_task = KheTaskAsst(task);
    if( sa != NULL )
    {
      am = KheAdjustmentMake(KHE_SA_OP_TASK_MOVE_TAG, NULL, 0,
	task, init_target_task, NULL, sa->arena);
      HaArrayAddLast(sa->adjustments, am);
    }
    return KheTaskMove(task, target_task);
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnAdjusterTaskAssignResource(KHE_SOLN_ADJUSTER sa,             */
/*    KHE_TASK task, KHE_RESOURCE r)                                         */
/*                                                                           */
/*  Assign r to task and return true if successful, remembering what         */
/*  was done in sa.                                                          */
/*                                                                           */
/*****************************************************************************/

bool KheSolnAdjusterTaskAssignResource(KHE_SOLN_ADJUSTER sa,
  KHE_TASK task, KHE_RESOURCE r)
{
  KHE_ADJUSTMENT am;  KHE_TASK init_target_task;
  HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
    "KheSolnAdjusterTaskAssignResource internal error (called after undo)");
  HnAssert(task != NULL, "KheSolnAdjusterTaskMove internal error");
  if( KheTaskAssignResourceCheck(task, r) )
  {
    init_target_task = KheTaskAsst(task);
    if( sa != NULL )
    {
      am = KheAdjustmentMake(KHE_SA_OP_TASK_MOVE_TAG, NULL, 0,
	task, init_target_task, NULL, sa->arena);
      HaArrayAddLast(sa->adjustments, am);
    }
    return KheTaskAssignResource(task, r);
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAdjusterTaskEnsureFixed(KHE_SOLN_ADJUSTER sa, KHE_TASK task) */
/*                                                                           */
/*  Ensure that task's assignment is fixed.                                  */
/*                                                                           */
/*****************************************************************************/

void KheSolnAdjusterTaskEnsureFixed(KHE_SOLN_ADJUSTER sa, KHE_TASK task)
{
  KHE_ADJUSTMENT am;
  HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
    "KheSolnAdjusterTaskEnsureFixed internal error (called after undo)");
  HnAssert(task != NULL, "KheSolnAdjusterTaskEnsureFixed internal error");
  if( !KheTaskAssignIsFixed(task) )
  {
    if( sa != NULL )
    {
      am = KheAdjustmentMake(KHE_SA_OP_TASK_FIX_TAG, NULL, 0,
	task, NULL, NULL, sa->arena);
      HaArrayAddLast(sa->adjustments, am);
    }
    KheTaskAssignFix(task);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAdjusterTaskEnsureUnFixed(KHE_SOLN_ADJUSTER sa,              */
/*    KHE_TASK task)                                                         */
/*                                                                           */
/*  Ensure that task's assignment is unfixed.                                */
/*                                                                           */
/*****************************************************************************/

void KheSolnAdjusterTaskEnsureUnFixed(KHE_SOLN_ADJUSTER sa,
  KHE_TASK task)
{
  KHE_ADJUSTMENT am;
  HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
    "KheSolnAdjusterTaskEnsureUnFixed internal error (called after undo)");
  HnAssert(task != NULL, "KheSolnAdjusterTaskEnsureUnFixed internal error");
  if( KheTaskAssignIsFixed(task) )
  {
    if( sa != NULL )
    {
      am = KheAdjustmentMake(KHE_SA_OP_TASK_UNFIX_TAG, NULL, 0,
	task, NULL, NULL, sa->arena);
      HaArrayAddLast(sa->adjustments, am);
    }
    KheTaskAssignUnFix(task);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnAdjusterTaskGroup(KHE_SOLN_ADJUSTER sa,                      */
/*    KHE_TASK task, KHE_TASK leader_task)                                   */
/*                                                                           */
/*  Group task under leader_task.                                            */
/*                                                                           */
/*****************************************************************************/

bool KheSolnAdjusterTaskGroup(KHE_SOLN_ADJUSTER sa,
  KHE_TASK task, KHE_TASK leader_task)
{
  KHE_ADJUSTMENT am;  bool res;
  HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
    "KheSolnAdjusterTaskGroup internal error (called after undo)");
  HnAssert(task != NULL, "KheSolnAdjusterTaskGroup internal error 1");
  HnAssert(leader_task != NULL, "KheSolnAdjusterTaskGroup internal error 2");
  if( KheTaskMoveCheck(task, leader_task) )
  {
    if( sa != NULL )
    {
      am = KheAdjustmentMake(KHE_SA_OP_TASK_GROUP_TAG, NULL, 0,
	task, NULL, NULL, sa->arena);
      HaArrayAddLast(sa->adjustments, am);
    }
    res = KheTaskMove(task, leader_task);
    /* no, doing this separately now KheTaskAssignFix(task); */
    return res;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_BOUND KheSolnAdjusterTaskBoundMake(KHE_SOLN_ADJUSTER sa,        */
/*    KHE_SOLN soln, KHE_RESOURCE_GROUP rg)                                  */
/*                                                                           */
/*  Make a task bound object for resource group rg.                          */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_BOUND KheSolnAdjusterTaskBoundMake(KHE_SOLN_ADJUSTER sa,
  KHE_SOLN soln, KHE_RESOURCE_GROUP rg)
{
  KHE_ADJUSTMENT am;  KHE_TASK_BOUND res;
  HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
    "KheSolnAdjusterTaskBoundMake internal error (called after undo)");
  res = KheTaskBoundMake(soln, rg);
  if( sa != NULL )
  {
    am = KheAdjustmentMake(KHE_SA_OP_TASK_BOUND_MAKE_TAG, NULL, 0,
      NULL, NULL, res, sa->arena);
    HaArrayAddLast(sa->adjustments, am);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheDetachLowCostMonitors and KheAttachLowCostMonitors"        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheDetachLowCostMonitors(KHE_SOLN soln, KHE_COST min_weight,        */
/*    KHE_SOLN_ADJUSTER sa)                                                  */
/*                                                                           */
/*  Detach monitors whose combined weight is strictly less than min_weight.  */
/*                                                                           */
/*****************************************************************************/

void KheDetachLowCostMonitors(KHE_SOLN soln, KHE_COST min_weight,
  KHE_SOLN_ADJUSTER sa)
{
  KHE_CONSTRAINT c;  KHE_MONITOR m;  int i;
  for( i = 0;  i < KheSolnMonitorCount(soln);  i++ )
  {
    m = KheSolnMonitor(soln, i);
    if( KheMonitorAttachedToSoln(m) )
    {
      c = KheMonitorConstraint(m);
      if( c != NULL && KheMonitorCombinedWeight(m) < min_weight )
	KheSolnAdjusterMonitorEnsureDetached(sa, m);
    }
  }
}

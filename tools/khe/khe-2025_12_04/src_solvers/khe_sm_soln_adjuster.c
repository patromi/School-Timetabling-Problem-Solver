
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
  KHE_SA_OP_MONITOR_ATTACH,
  KHE_SA_OP_MONITOR_DETACH,
  KHE_SA_OP_MONITOR_WEIGHT,
  KHE_SA_OP_MONITOR_TILT,

  /* operations on tasks */
  KHE_SA_OP_TASK_MOVE,
  KHE_SA_OP_TASK_ASSIGN_RESOURCE,
  KHE_SA_OP_TASK_FIX,
  KHE_SA_OP_TASK_UNFIX,
  KHE_SA_OP_TASK_ADD_TASK_BOUND,
  KHE_SA_OP_TASK_DELETE_TASK_BOUND,
  KHE_SA_OP_TASK_GROUP,

  /* operations on task bounds */
  KHE_SA_OP_TASK_BOUND_MAKE
} KHE_SA_OP_TYPE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SA_OP - one adjustment (NB not a pointer type)                       */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_sa_op_rec {
  KHE_SA_OP_TYPE				type;
  union {

    /* operations on monitors */
    struct {
      KHE_MONITOR				monitor;
    } monitor_attach;

    struct {
      KHE_MONITOR				monitor;
    } monitor_detach;

    struct {
      KHE_MONITOR				monitor;
      KHE_COST					old_combined_weight;
      KHE_COST					new_combined_weight;
    } monitor_weight;

    struct {
      KHE_LIMIT_ACTIVE_INTERVALS_MONITOR	monitor;
    } monitor_tilt;

    /* operations on tasks */
    struct {
      KHE_TASK					task;
      KHE_TASK					old_target_task;
      KHE_TASK					new_target_task;
    } task_move;

    struct {
      KHE_TASK					task;
      KHE_TASK					old_target_task;
      KHE_RESOURCE				new_resource;
    } task_assign_resource;

    struct {
      KHE_TASK					task;
    } task_fix;

    struct {
      KHE_TASK					task;
    } task_unfix;

    struct {
      KHE_TASK					task;
      KHE_TASK_BOUND				task_bound;
    } task_add_task_bound;

    struct {
      KHE_TASK					task;
      KHE_TASK_BOUND				task_bound;
    } task_delete_task_bound;

    struct {
      KHE_TASK					task;
      KHE_TASK					leader_task;
    } task_group;

    /* operations on task bounds */
    struct {
      KHE_SOLN					soln;
      KHE_RESOURCE_GROUP			resource_group;
      KHE_TASK_BOUND				task_bound;
    } task_bound_make;

  } u;
} KHE_SA_OP;

typedef HA_ARRAY(KHE_SA_OP) ARRAY_KHE_SA_OP;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_ADJUSTER - a solution adjuster                                  */
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
  ARRAY_KHE_SA_OP		operations;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SA_OP"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SA_OP KheGetSAOp(KHE_SOLN_ADJUSTER sa)                               */
/*                                                                           */
/*  Get an new KHE_SA_OP object.  It is dangerous to return a pointer to     */
/*  an array element like this, because if the array is resized the pointer  */
/*  becomes undefined.  However we only use the pointer very briefly, to     */
/*  initialize the fields of the result object directly after the return.    */
/*                                                                           */
/*****************************************************************************/

static KHE_SA_OP *KheGetSAOp(KHE_SOLN_ADJUSTER sa)
{
  static KHE_SA_OP op;
  HaArrayAddLast(sa->operations, op);
  return &HaArrayLast(sa->operations);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "operation loading - monitors"                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAdjusterMonitorAttachToSoln(KHE_SOLN_ADJUSTER sa,            */
/*    KHE_MONITOR m)                                                         */
/*                                                                           */
/*  Attach m to soln, and record that action in sa if sa != NULL.            */
/*                                                                           */
/*****************************************************************************/

void KheSolnAdjusterMonitorAttachToSoln(KHE_SOLN_ADJUSTER sa, KHE_MONITOR m)
{
  KHE_SA_OP *op;
  if( sa != NULL )
  {
    HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
      "KheSolnAdjusterMonitorAttachToSoln internal error "
      "(called after undo)");
    op = KheGetSAOp(sa);
    op->type = KHE_SA_OP_MONITOR_ATTACH;
    op->u.monitor_attach.monitor = m;
  }
  KheMonitorAttachToSoln(m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAdjusterMonitorDetachFromSoln(KHE_SOLN_ADJUSTER sa,          */
/*    KHE_MONITOR m)                                                         */
/*                                                                           */
/*  Detach m from soln, and record that action in sa if sa != NULL.          */
/*                                                                           */
/*****************************************************************************/

void KheSolnAdjusterMonitorDetachFromSoln(KHE_SOLN_ADJUSTER sa, KHE_MONITOR m)
{
  KHE_SA_OP *op;
  if( sa != NULL )
  {
    HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
      "KheSolnAdjusterMonitorDetachFromSoln internal error "
      "(called after undo)");
    op = KheGetSAOp(sa);
    op->type = KHE_SA_OP_MONITOR_DETACH;
    op->u.monitor_detach.monitor = m;
  }
  KheMonitorDetachFromSoln(m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAdjusterMonitorSetCombinedWeight(KHE_SOLN_ADJUSTER sa,       */
/*    KHE_MONITOR m, KHE_COST combined_weight)                               */
/*                                                                           */
/*  Change the combined weight of m to combined_weight.                      */
/*                                                                           */
/*****************************************************************************/

void KheSolnAdjusterMonitorSetCombinedWeight(KHE_SOLN_ADJUSTER sa,
  KHE_MONITOR m, KHE_COST combined_weight)
{
  KHE_SA_OP *op;
  if( sa != NULL )
  {
    HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
      "KheSolnAdjusterMonitorSetCombinedWeight internal error "
      "(called after undo)");
    op = KheGetSAOp(sa);
    op->type = KHE_SA_OP_MONITOR_WEIGHT;
    op->u.monitor_weight.monitor = m;
    op->u.monitor_weight.old_combined_weight = KheMonitorCombinedWeight(m);
    op->u.monitor_weight.new_combined_weight = combined_weight;
  }
  KheMonitorSetCombinedWeight(m, combined_weight);
  if( DEBUG1 )
  {
    fprintf(stderr, "  KheSolnAdjusterMonitorChangeWeight to %.5f: ",
      KheCostShow(combined_weight));
    KheMonitorDebug(m, 2, 0, stderr);
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
  KHE_SA_OP *op;
  if( !KheLimitActiveIntervalsMonitorTilt(laim) )
  {
    if( sa != NULL )
    {
      HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
	"KheSolnAdjusterMonitorSetTilt internal error "
	"(called after undo)");
      op = KheGetSAOp(sa);
      op->type = KHE_SA_OP_MONITOR_TILT;
      op->u.monitor_tilt.monitor = laim;
    }
    KheLimitActiveIntervalsMonitorSetTilt(laim);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "operation loading - tasks"                                    */
/*                                                                           */
/*****************************************************************************/

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
  KHE_SA_OP *op;
  HnAssert(task != NULL, "KheSolnAdjusterTaskMove internal error");
  if( KheTaskMoveCheck(task, target_task) )
  {
    if( sa != NULL )
    {
      HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
	"KheSolnAdjusterTaskMove internal error (called after undo)");
      op = KheGetSAOp(sa);
      op->type = KHE_SA_OP_TASK_MOVE;
      op->u.task_move.task = task;
      op->u.task_move.old_target_task = KheTaskAsst(task);
      op->u.task_move.new_target_task = target_task;
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
  KHE_SA_OP *op;
  HnAssert(task != NULL, "KheSolnAdjusterTaskAssignResource internal error");
  if( KheTaskAssignResourceCheck(task, r) )
  {
    if( sa != NULL )
    {
      HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
	"KheSolnAdjusterTaskAssignResource internal error (called after undo)");
      op = KheGetSAOp(sa);
      op->type = KHE_SA_OP_TASK_ASSIGN_RESOURCE;
      op->u.task_assign_resource.task = task;
      op->u.task_assign_resource.old_target_task = KheTaskAsst(task);
      op->u.task_assign_resource.new_resource = r;
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

void KheSolnAdjusterTaskAssignFix(KHE_SOLN_ADJUSTER sa, KHE_TASK task)
{
  KHE_SA_OP *op;
  HnAssert(task != NULL, "KheSolnAdjusterTaskEnsureFixed internal error");
  if( !KheTaskAssignIsFixed(task) )
  {
    if( sa != NULL )
    {
      HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
	"KheSolnAdjusterTaskEnsureFixed internal error (called after undo)");
      op = KheGetSAOp(sa);
      op->type = KHE_SA_OP_TASK_FIX;
      op->u.task_fix.task = task;
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

void KheSolnAdjusterTaskAssignUnFix(KHE_SOLN_ADJUSTER sa, KHE_TASK task)
{
  KHE_SA_OP *op;
  HnAssert(task != NULL, "KheSolnAdjusterTaskEnsureUnFixed internal error");
  if( KheTaskAssignIsFixed(task) )
  {
    if( sa != NULL )
    {
      HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
	"KheSolnAdjusterTaskEnsureUnFixed internal error (called after undo)");
      op = KheGetSAOp(sa);
      op->type = KHE_SA_OP_TASK_UNFIX;
      op->u.task_unfix.task = task;
    }
    KheTaskAssignUnFix(task);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnAdjusterTaskAddTaskBound(KHE_SOLN_ADJUSTER sa,               */
/*    KHE_TASK task, KHE_TASK_BOUND tb)                                      */
/*                                                                           */
/*  Add tb to task and return true if successful, remembering what was       */
/*  done in sa.                                                              */
/*                                                                           */
/*****************************************************************************/

bool KheSolnAdjusterTaskAddTaskBound(KHE_SOLN_ADJUSTER sa,
  KHE_TASK task, KHE_TASK_BOUND tb)
{
  bool res;  KHE_SA_OP *op;
  res = KheTaskAddTaskBound(task, tb);
  if( res && sa != NULL )
  {
    HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
      "KheSolnAdjusterTaskAddTaskBound internal error (called after undo)");
    op = KheGetSAOp(sa);
    op->type = KHE_SA_OP_TASK_ADD_TASK_BOUND;
    op->u.task_add_task_bound.task = task;
    op->u.task_add_task_bound.task_bound = tb;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnAdjusterTaskDeleteTaskBound(KHE_SOLN_ADJUSTER sa,            */
/*    KHE_TASK task, KHE_TASK_BOUND tb)                                      */
/*                                                                           */
/*  Delete tb from task and return true if successful, remembering what was  */
/*  done in sa.                                                              */
/*                                                                           */
/*****************************************************************************/

bool KheSolnAdjusterTaskDeleteTaskBound(KHE_SOLN_ADJUSTER sa,
  KHE_TASK task, KHE_TASK_BOUND tb)
{
  bool res;  KHE_SA_OP *op;
  res = KheTaskDeleteTaskBound(task, tb);
  if( res && sa != NULL )
  {
    HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
      "KheSolnAdjusterTaskDeleteTaskBound internal error (called after undo)");
    op = KheGetSAOp(sa);
    op->type = KHE_SA_OP_TASK_DELETE_TASK_BOUND;
    op->u.task_delete_task_bound.task = task;
    op->u.task_delete_task_bound.task_bound = tb;
  }
  return res;
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
  KHE_SA_OP *op;  bool res;
  HnAssert(task != NULL, "KheSolnAdjusterTaskGroup internal error 1");
  HnAssert(leader_task != NULL, "KheSolnAdjusterTaskGroup internal error 2");
  if( KheTaskMoveCheck(task, leader_task) )
  {
    if( sa != NULL )
    {
      HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
	"KheSolnAdjusterTaskGroup internal error (called after undo)");
      op = KheGetSAOp(sa);
      op->type = KHE_SA_OP_TASK_GROUP;
      op->u.task_group.task = task;
      op->u.task_group.leader_task = leader_task;
    }
    res = KheTaskMove(task, leader_task);
    return res;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "operation loading - task bounds"                              */
/*                                                                           */
/*****************************************************************************/

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
  KHE_SA_OP *op;  KHE_TASK_BOUND res;
  res = KheTaskBoundMake(soln, rg);
  if( sa != NULL )
  {
    HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
      "KheSolnAdjusterTaskBoundMake internal error (called after undo)");
    op = KheGetSAOp(sa);
    op->type = KHE_SA_OP_TASK_BOUND_MAKE;
    op->u.task_bound_make.soln = soln;
    op->u.task_bound_make.resource_group = rg;
    op->u.task_bound_make.task_bound = res;
  }
  return res;
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

KHE_SOLN_ADJUSTER KheSolnAdjusterMake(KHE_SOLN soln, HA_ARENA a)
{
  KHE_SOLN_ADJUSTER res;
  /* a = KheSolnArenaBegin(soln); */
  HaMake(res, a);
  res->soln = soln;
  res->arena = a;
  res->state = KHE_SOLN_ADJUSTER_AT_END;
  HaArrayInit(res->operations, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAdjusterDelete(KHE_SOLN_ADJUSTER sa)                         */
/*                                                                           */
/*  Delete sa, first undoing all the operations (in reverse order).          */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnAdjusterDelete(KHE_SOLN_ADJUSTER sa)
{
  ** KheSolnAdjusterUndo(sa); **
  KheSolnArenaEnd(sa->soln, sa->arena);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAdjusterClear(KHE_SOLN_ADJUSTER sa)                          */
/*                                                                           */
/*  Clear sa.                                                                */
/*                                                                           */
/*****************************************************************************/

void KheSolnAdjusterClear(KHE_SOLN_ADJUSTER sa)
{
  sa->state = KHE_SOLN_ADJUSTER_AT_END;
  HaArrayClear(sa->operations);
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
  KHE_SA_OP op;  int i;
  HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_END,
    "KheSolnAdjusterUndo internal error (called after undo)");
  HaArrayForEachReverse(sa->operations, op, i)
    switch( op.type )
    {
      case KHE_SA_OP_MONITOR_ATTACH:

	/* we attached, so detach now */
	KheMonitorDetachFromSoln(op.u.monitor_attach.monitor);
	break;

      case KHE_SA_OP_MONITOR_DETACH:

	/* we detached, so attach now */
	KheMonitorAttachToSoln(op.u.monitor_detach.monitor);
	break;

      case KHE_SA_OP_MONITOR_WEIGHT:

	/* we changed the weight, so change it back */
	KheMonitorSetCombinedWeight(op.u.monitor_weight.monitor,
          op.u.monitor_weight.old_combined_weight);
	break;

      case KHE_SA_OP_MONITOR_TILT:

	/* we set the tilt, so clear it */
	KheLimitActiveIntervalsMonitorClearTilt(op.u.monitor_tilt.monitor);
	break;

      case KHE_SA_OP_TASK_MOVE:

	/* we moved the task, so move it back again */
	KheTaskMove(op.u.task_move.task, op.u.task_move.old_target_task);
	break;

      case KHE_SA_OP_TASK_ASSIGN_RESOURCE:

	/* we moved the task (to a resource), so move it back again */
	KheTaskMove(op.u.task_assign_resource.task,
	  op.u.task_assign_resource.old_target_task);
	break;

      case KHE_SA_OP_TASK_FIX:

	/* we fixed the task, so unfix it */
	KheTaskAssignUnFix(op.u.task_fix.task);
	break;

      case KHE_SA_OP_TASK_UNFIX:

	/* we unfixed the task, so fix it */
	KheTaskAssignFix(op.u.task_unfix.task);
	break;

      case KHE_SA_OP_TASK_ADD_TASK_BOUND:

	/* we added a task bound, so delete it */
	KheTaskDeleteTaskBound(op.u.task_add_task_bound.task,
	  op.u.task_add_task_bound.task_bound);
	break;

      case KHE_SA_OP_TASK_DELETE_TASK_BOUND:

	/* we deleted a task bound, so add it */
	KheTaskAddTaskBound(op.u.task_delete_task_bound.task,
	  op.u.task_delete_task_bound.task_bound);
	break;

      case KHE_SA_OP_TASK_GROUP:

	/* we grouped the task, so ungroup it - that is, move it to the */
	/* task that the leader task is assigned to                     */
	if( !KheTaskMove(op.u.task_group.task,
	      KheTaskAsst(op.u.task_group.leader_task)) )
	  HnAbort("KheSolnAdjusterUndo internal error (cannot ungroup task)");
	break;

      case KHE_SA_OP_TASK_BOUND_MAKE:

	/* we created am->task_bound, so delete it */
	KheTaskBoundDelete(op.u.task_bound_make.task_bound);
        op.u.task_bound_make.task_bound = NULL;
	break;

      default:

	HnAbort("KheAdjustmentUndo internal error");
    }
  sa->state = KHE_SOLN_ADJUSTER_AT_START;
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
  KHE_SA_OP op;  int i;
  HnAssert(sa->state == KHE_SOLN_ADJUSTER_AT_START,
    "KheSolnAdjusterRedo internal error (not called after undo)");
  HaArrayForEach(sa->operations, op, i)
    switch( op.type )
    {
      case KHE_SA_OP_MONITOR_ATTACH:

	/* we attached, so attach it again */
	KheMonitorAttachToSoln(op.u.monitor_attach.monitor);
	break;

      case KHE_SA_OP_MONITOR_DETACH:

	/* we detached, so detach it again */
	KheMonitorDetachFromSoln(op.u.monitor_detach.monitor);
	break;

      case KHE_SA_OP_MONITOR_WEIGHT:

	/* we changed the weight, so change it again */
	KheMonitorSetCombinedWeight(op.u.monitor_weight.monitor,
          op.u.monitor_weight.new_combined_weight);
	break;

      case KHE_SA_OP_MONITOR_TILT:

	/* we set the tilt, so set it again */
	KheLimitActiveIntervalsMonitorSetTilt(op.u.monitor_tilt.monitor);
	break;

      case KHE_SA_OP_TASK_MOVE:

	/* we moved the task, so move it again */
	KheTaskMove(op.u.task_move.task, op.u.task_move.new_target_task);
	break;

      case KHE_SA_OP_TASK_ASSIGN_RESOURCE:

	/* we moved the task (to a resource), so move it again */
	KheTaskAssignResource(op.u.task_assign_resource.task,
	  op.u.task_assign_resource.new_resource);
	break;

      case KHE_SA_OP_TASK_FIX:

	/* we fixed the task, so fix it again */
	KheTaskAssignFix(op.u.task_fix.task);
	break;

      case KHE_SA_OP_TASK_UNFIX:

	/* we unfixed the task, so unfix it again */
	KheTaskAssignUnFix(op.u.task_unfix.task);
	break;

      case KHE_SA_OP_TASK_ADD_TASK_BOUND:

	/* we added a task bound, so add it again */
	KheTaskAddTaskBound(op.u.task_add_task_bound.task,
	  op.u.task_add_task_bound.task_bound);
	break;

      case KHE_SA_OP_TASK_DELETE_TASK_BOUND:

	/* we deleted a task bound, so delete it again */
	KheTaskDeleteTaskBound(op.u.task_delete_task_bound.task,
	  op.u.task_delete_task_bound.task_bound);
	break;

      case KHE_SA_OP_TASK_GROUP:

	/* we grouped the task, so group it again */
	if( !KheTaskMove(op.u.task_group.task, op.u.task_group.leader_task) )
	  HnAbort("KheSolnAdjusterRedo internal error (cannot group task)");
	break;

      case KHE_SA_OP_TASK_BOUND_MAKE:

	/* we made a task bound, so make it again; although there is */
	/* no point in it because the caller can't access it         */
        op.u.task_bound_make.task_bound = KheTaskBoundMake(
	  op.u.task_bound_make.soln, op.u.task_bound_make.resource_group);
	break;

      default:

	HnAbort("KheSolnAdjusterUndo internal error");
    }
  sa->state = KHE_SOLN_ADJUSTER_AT_END;
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
      if( c != NULL && KheMonitorCombinedWeight(m) < min_weight &&
	  KheMonitorAttachedToSoln(m) )
	KheSolnAdjusterMonitorDetachFromSoln(sa, m);
	/* KheSolnAdjusterMonitorEnsureDetached(sa, m); */
    }
  }
}

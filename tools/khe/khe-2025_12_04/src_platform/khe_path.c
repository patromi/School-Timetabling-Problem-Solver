
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
/*  FILE:         khe_path.c                                                 */
/*  DESCRIPTION:  Paths                                                      */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_PATH_OP - one operation on a path.                                   */
/*                                                                           */
/*****************************************************************************/

typedef enum {

  /* operations on meets */
  KHE_PATH_OP_MEET_SET_BACK,
  KHE_PATH_OP_MEET_ADD,
  KHE_PATH_OP_MEET_DELETE,
  KHE_PATH_OP_MEET_SPLIT,
  KHE_PATH_OP_MEET_MERGE,
  KHE_PATH_OP_MEET_MOVE,
  KHE_PATH_OP_MEET_SET_AUTO_DOMAIN,
  KHE_PATH_OP_MEET_ASSIGN_FIX,
  KHE_PATH_OP_MEET_ASSIGN_UNFIX,
  KHE_PATH_OP_MEET_ADD_MEET_BOUND,
  KHE_PATH_OP_MEET_DELETE_MEET_BOUND,

  /* operations on meet bounds */
  KHE_PATH_OP_MEET_BOUND_ADD,
  KHE_PATH_OP_MEET_BOUND_DELETE,
  KHE_PATH_OP_MEET_BOUND_ADD_TIME_GROUP,
  KHE_PATH_OP_MEET_BOUND_DELETE_TIME_GROUP,

  /* operations on tasks */
  KHE_PATH_OP_TASK_SET_BACK,
  KHE_PATH_OP_TASK_ADD,
  KHE_PATH_OP_TASK_DELETE,
  KHE_PATH_OP_TASK_SPLIT,
  KHE_PATH_OP_TASK_MERGE,
  KHE_PATH_OP_TASK_MOVE,
  KHE_PATH_OP_TASK_ASSIGN_FIX,
  KHE_PATH_OP_TASK_ASSIGN_UNFIX,
  KHE_PATH_OP_TASK_ADD_TASK_BOUND,
  KHE_PATH_OP_TASK_DELETE_TASK_BOUND,

  /* operations on task bounds */
  KHE_PATH_OP_TASK_BOUND_ADD,
  KHE_PATH_OP_TASK_BOUND_DELETE,

  /* operations on nodes */
  KHE_PATH_OP_NODE_SET_BACK,
  KHE_PATH_OP_NODE_ADD,
  KHE_PATH_OP_NODE_DELETE,
  KHE_PATH_OP_NODE_ADD_PARENT,
  KHE_PATH_OP_NODE_DELETE_PARENT,
  KHE_PATH_OP_NODE_SWAP_CHILD_NODES_AND_LAYERS,
  KHE_PATH_OP_NODE_ADD_MEET,
  KHE_PATH_OP_NODE_DELETE_MEET,

  /* operations on layers */
  KHE_PATH_OP_LAYER_SET_BACK,
  KHE_PATH_OP_LAYER_ADD,
  KHE_PATH_OP_LAYER_DELETE,
  KHE_PATH_OP_LAYER_ADD_CHILD_NODE,
  KHE_PATH_OP_LAYER_DELETE_CHILD_NODE,
  KHE_PATH_OP_LAYER_ADD_RESOURCE,
  KHE_PATH_OP_LAYER_DELETE_RESOURCE,

  /* operations on zones */
  KHE_PATH_OP_ZONE_SET_BACK,
  KHE_PATH_OP_ZONE_ADD,
  KHE_PATH_OP_ZONE_DELETE,
  KHE_PATH_OP_ZONE_ADD_MEET_OFFSET,
  KHE_PATH_OP_ZONE_DELETE_MEET_OFFSET

} KHE_PATH_OP_TYPE;

typedef struct khe_path_op_rec {
  KHE_PATH_OP_TYPE		type;		/* operation type    */
  union {

    /* operations on meets */
    struct {
      KHE_MEET			meet;
      void			*old_back;
      void			*new_back;
    } meet_set_back;

    struct {
      KHE_MEET			meet;		/* reference counted */
      int			duration;
      KHE_EVENT			event;
    } meet_add;

    struct {
      KHE_MEET			meet;		/* reference counted */
      int			duration;
      KHE_EVENT			event;
    } meet_delete;

    struct {
      KHE_MEET			meet1;
      KHE_MEET			meet2;		/* reference counted */
      int			durn1;
    } meet_split;

    struct {
      KHE_MEET			meet1;
      KHE_MEET			meet2;		/* reference counted */
      int			durn1;
    } meet_merge;

    struct {
      KHE_MEET			meet;
      KHE_MEET			old_target_meet;
      KHE_MEET			new_target_meet;
      int			old_offset;
      int			new_offset;
    } meet_move;

    struct {
      KHE_MEET			meet;
      bool			automatic;
    } meet_set_auto_domain;

    struct {
      KHE_MEET			meet;
    } meet_assign_fix;

    struct {
      KHE_MEET			meet;
    } meet_assign_unfix;

    struct {
      KHE_MEET			meet;
      KHE_MEET_BOUND		meet_bound;
    } meet_add_meet_bound;

    struct {
      KHE_MEET			meet;
      KHE_MEET_BOUND		meet_bound;
    } meet_delete_meet_bound;

    /* operations on meet bounds */
    struct {
      KHE_MEET_BOUND		meet_bound;	/* reference counted */
      bool			occupancy;
      KHE_TIME_GROUP		dft_tg;
    } meet_bound_add;

    struct {
      KHE_MEET_BOUND		meet_bound;	/* reference counted */
      bool			occupancy;
      KHE_TIME_GROUP		dft_tg;
    } meet_bound_delete;

    struct {
      KHE_MEET_BOUND		meet_bound;
      int			duration;
      KHE_TIME_GROUP		time_group;
    } meet_bound_add_time_group;

    struct {
      KHE_MEET_BOUND		meet_bound;
      int			duration;
      KHE_TIME_GROUP		time_group;
    } meet_bound_delete_time_group;

    /* operations on tasks */
    struct {
      KHE_TASK			task;
      void			*old_back;
      void			*new_back;
    } task_set_back;

    struct {
      KHE_TASK			task;		/* reference counted */
      KHE_RESOURCE_TYPE		resource_type;
      KHE_MEET			meet;
      KHE_EVENT_RESOURCE	event_resource;
    } task_add;

    struct {
      KHE_TASK			task;		/* reference counted */
      KHE_RESOURCE_TYPE		resource_type;
      KHE_MEET			meet;
      KHE_EVENT_RESOURCE	event_resource;
    } task_delete;

    struct {
      KHE_TASK			task1;
      KHE_TASK			task2;		/* reference counted */
      KHE_MEET			meet2;
      int			durn1;
    } task_split;

    struct {
      KHE_TASK			task1;
      KHE_TASK			task2;		/* reference counted */
      KHE_MEET			meet2;
      int			durn1;
    } task_merge;

    struct {
      KHE_TASK			task;
      KHE_TASK			old_target_task;
      KHE_TASK			new_target_task;
    } task_move;

    struct {
      KHE_TASK			task;
    } task_assign_fix;

    struct {
      KHE_TASK			task;
    } task_assign_unfix;

    struct {
      KHE_TASK			task;
      KHE_TASK_BOUND		task_bound;
    } task_add_task_bound;

    struct {
      KHE_TASK			task;
      KHE_TASK_BOUND		task_bound;
    } task_delete_task_bound;

    /* operations on task bounds */
    struct {
      KHE_TASK_BOUND		task_bound;	/* reference counted */
      KHE_RESOURCE_GROUP	resource_group;
    } task_bound_add;

    struct {
      KHE_TASK_BOUND		task_bound;	/* reference counted */
      KHE_RESOURCE_GROUP	resource_group;
    } task_bound_delete;

    /* operations on nodes */
    struct {
      KHE_NODE			node;
      void			*old_back;
      void			*new_back;
    } node_set_back;

    struct {
      KHE_NODE			node;
    } node_add;

    struct {
      KHE_NODE			node;
    } node_delete;

    struct {
      KHE_NODE			child_node;
      KHE_NODE			parent_node;
    } node_add_parent;

    struct {
      KHE_NODE			child_node;
      KHE_NODE			parent_node;
    } node_delete_parent;

    struct {
      KHE_NODE			node1;
      KHE_NODE			node2;
    } node_swap_child_nodes_and_layers;

    struct {
      KHE_NODE			node;
      KHE_MEET			meet;
    } node_add_meet;

    struct {
      KHE_NODE			node;
      KHE_MEET			meet;
    } node_delete_meet;

    /* operations on layers */
    struct {
      KHE_LAYER			layer;
      void			*old_back;
      void			*new_back;
    } layer_set_back;

    struct {
      KHE_LAYER			layer;
      KHE_NODE			parent_node;
    } layer_add;

    struct {
      KHE_LAYER			layer;
      KHE_NODE			parent_node;
    } layer_delete;

    struct {
      KHE_LAYER			layer;
      KHE_NODE			child_node;
    } layer_add_child_node;

    struct {
      KHE_LAYER			layer;
      KHE_NODE			child_node;
    } layer_delete_child_node;

    struct {
      KHE_LAYER			layer;
      KHE_RESOURCE		resource;
    } layer_add_resource;

    struct {
      KHE_LAYER			layer;
      KHE_RESOURCE		resource;
    } layer_delete_resource;

    /* operations on zones */
    struct {
      KHE_ZONE			zone;
      void			*old_back;
      void			*new_back;
    } zone_set_back;

    struct {
      KHE_ZONE			zone;
      KHE_NODE			node;
    } zone_add;

    struct {
      KHE_ZONE			zone;
      KHE_NODE			node;
    } zone_delete;

    struct {
      KHE_ZONE			zone;
      KHE_MEET			meet;
      int			offset;
    } zone_add_meet_offset;

    struct {
      KHE_ZONE			zone;
      KHE_MEET			meet;
      int			offset;
    } zone_delete_meet_offset;

  } u;
} KHE_PATH_OP;

typedef HA_ARRAY(KHE_PATH_OP) ARRAY_KHE_PATH_OP;


/*****************************************************************************/
/*                                                                           */
/*  KHE_PATH - a path                                                        */
/*                                                                           */
/*****************************************************************************/

struct khe_path_rec {
  KHE_SOLN			soln;			/* soln              */
  KHE_MARK			mark;			/* starting mark     */
  KHE_COST			soln_cost;		/* cost when created */
  ARRAY_KHE_PATH_OP		operations;		/* the operations    */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_PATH KhePathMake(KHE_SOLN soln)                                      */
/*                                                                           */
/*  Make a path with null attributes.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_PATH KhePathMake(KHE_SOLN soln)
{
  KHE_PATH res;  HA_ARENA a;
  a = KheSolnArena(soln);
  HaMake(res, a);
  res->soln = soln;
  res->mark = NULL;
  res->soln_cost = 0;
  HaArrayInit(res->operations, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathFree(KHE_PATH path)                                          */
/*                                                                           */
/*  Free path.                                                               */
/*                                                                           */
/*****************************************************************************/

/* *** this is not right, but anyway arenas take care of it now.
void KhePathFree(KHE_PATH path)
{
  MArrayFree(path->operations);
  MFree(path);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KhePathSoln(KHE_PATH path)                                      */
/*                                                                           */
/*  Return path's enclosing solution.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KhePathSoln(KHE_PATH path)
{
  return path->soln;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KhePathSolnCost(KHE_PATH path)                                  */
/*                                                                           */
/*  Return path's cost.                                                      */
/*                                                                           */
/*****************************************************************************/

KHE_COST KhePathSolnCost(KHE_PATH path)
{
  return path->soln_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MARK KhePathMark(KHE_PATH path)                                      */
/*                                                                           */
/*  Return path's mark.                                                      */
/*                                                                           */
/*****************************************************************************/

KHE_MARK KhePathMark(KHE_PATH path)
{
  return path->mark;
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePathIncreasingSolnCostCmp(const void *t1, const void *t2)         */
/*                                                                           */
/*  Comparison function for sorting paths into increasing cost order.        */
/*                                                                           */
/*****************************************************************************/

int KhePathIncreasingSolnCostCmp(const void *t1, const void *t2)
{
  KHE_PATH path1 = * (KHE_PATH *) t1;
  KHE_PATH path2 = * (KHE_PATH *) t2;
  return KheCostCmp(path1->soln_cost, path2->soln_cost);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathDelete(KHE_PATH path)                                        */
/*                                                                           */
/*  Delete path.                                                             */
/*                                                                           */
/*****************************************************************************/

void KhePathDelete(KHE_PATH path)
{
  KHE_PATH_OP op;  int i;

  /* decrement reference counts */
  HaArrayForEach(path->operations, op, i)
  {
    switch( op.type )
    {
      /* operations on meets */
      case KHE_PATH_OP_MEET_ADD:

	KheMeetReferenceCountDecrement(op.u.meet_add.meet);
	break;

      case KHE_PATH_OP_MEET_DELETE:

	KheMeetReferenceCountDecrement(op.u.meet_delete.meet);
	break;

      case KHE_PATH_OP_MEET_SPLIT:

	KheMeetReferenceCountDecrement(op.u.meet_split.meet2);
	break;

      case KHE_PATH_OP_MEET_MERGE:

	KheMeetReferenceCountDecrement(op.u.meet_merge.meet2);
	break;

      case KHE_PATH_OP_MEET_MOVE:
      case KHE_PATH_OP_MEET_SET_AUTO_DOMAIN:
      case KHE_PATH_OP_MEET_ASSIGN_FIX:
      case KHE_PATH_OP_MEET_ASSIGN_UNFIX:
      case KHE_PATH_OP_MEET_ADD_MEET_BOUND:
      case KHE_PATH_OP_MEET_DELETE_MEET_BOUND:

	break;


      /* operations on meet bounds */
      case KHE_PATH_OP_MEET_BOUND_ADD:

	KheMeetBoundReferenceCountDecrement(op.u.meet_bound_add.meet_bound);
	break;

      case KHE_PATH_OP_MEET_BOUND_DELETE:

	KheMeetBoundReferenceCountDecrement(op.u.meet_bound_delete.meet_bound);
	break;

      case KHE_PATH_OP_MEET_BOUND_ADD_TIME_GROUP:
      case KHE_PATH_OP_MEET_BOUND_DELETE_TIME_GROUP:

	break;


      /* operations on tasks */
      case KHE_PATH_OP_TASK_SET_BACK:

	break;

      case KHE_PATH_OP_TASK_ADD:

	KheTaskReferenceCountDecrement(op.u.task_add.task);
	break;

      case KHE_PATH_OP_TASK_DELETE:

	KheTaskReferenceCountDecrement(op.u.task_delete.task);
	break;

      case KHE_PATH_OP_TASK_SPLIT:

	KheTaskReferenceCountDecrement(op.u.task_split.task2);
	break;

      case KHE_PATH_OP_TASK_MERGE:

	KheTaskReferenceCountDecrement(op.u.task_merge.task2);
	break;

      case KHE_PATH_OP_TASK_MOVE:
      case KHE_PATH_OP_TASK_ASSIGN_FIX:
      case KHE_PATH_OP_TASK_ASSIGN_UNFIX:
      case KHE_PATH_OP_TASK_ADD_TASK_BOUND:
      case KHE_PATH_OP_TASK_DELETE_TASK_BOUND:

	break;


      /* operations on task bounds */
      case KHE_PATH_OP_TASK_BOUND_ADD:

	KheTaskBoundReferenceCountDecrement(op.u.task_bound_add.task_bound);
	break;

      case KHE_PATH_OP_TASK_BOUND_DELETE:

	KheTaskBoundReferenceCountDecrement(op.u.task_bound_delete.task_bound);
	break;


      /* operations on nodes */
      case KHE_PATH_OP_NODE_SET_BACK:

	break;

      case KHE_PATH_OP_NODE_ADD:

	KheNodeReferenceCountDecrement(op.u.node_add.node);
	break;

      case KHE_PATH_OP_NODE_DELETE:

	KheNodeReferenceCountDecrement(op.u.node_delete.node);
	break;

      case KHE_PATH_OP_NODE_ADD_PARENT:
      case KHE_PATH_OP_NODE_DELETE_PARENT:
      case KHE_PATH_OP_NODE_SWAP_CHILD_NODES_AND_LAYERS:
      case KHE_PATH_OP_NODE_ADD_MEET:
      case KHE_PATH_OP_NODE_DELETE_MEET:

	break;


      /* operations on layers */
      case KHE_PATH_OP_LAYER_SET_BACK:

	break;

      case KHE_PATH_OP_LAYER_ADD:

	KheLayerReferenceCountDecrement(op.u.layer_add.layer);
	break;

      case KHE_PATH_OP_LAYER_DELETE:

	KheLayerReferenceCountDecrement(op.u.layer_delete.layer);
	break;

      case KHE_PATH_OP_LAYER_ADD_CHILD_NODE:
      case KHE_PATH_OP_LAYER_DELETE_CHILD_NODE:
      case KHE_PATH_OP_LAYER_ADD_RESOURCE:
      case KHE_PATH_OP_LAYER_DELETE_RESOURCE:

	break;


      /* operations on zones */
      case KHE_PATH_OP_ZONE_SET_BACK:

	break;

      case KHE_PATH_OP_ZONE_ADD:

	KheZoneReferenceCountDecrement(op.u.zone_add.zone);
	break;

      case KHE_PATH_OP_ZONE_DELETE:

	KheZoneReferenceCountDecrement(op.u.zone_delete.zone);
	break;

      case KHE_PATH_OP_ZONE_ADD_MEET_OFFSET:
      case KHE_PATH_OP_ZONE_DELETE_MEET_OFFSET:

	break;


      default:

	HnAbort("KhePathDelete internal error");
	break;
    }
  }

  /* remove path from its mark, if any */
  if( path->mark != NULL )
    KheMarkDeletePath(path->mark, path);

  /* add path to free list */
  KheSolnAddPathToFreeList(path->soln, path);
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePathCount(KHE_PATH path)                                          */
/*                                                                           */
/*  Return the number of operations currently stored in path.                */
/*                                                                           */
/*****************************************************************************/

int KhePathCount(KHE_PATH path)
{
  return HaArrayCount(path->operations);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "copy, undo, and redo"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_PATH KhePathCopy(KHE_PATH path, int start_pos, KHE_MARK mark)        */
/*                                                                           */
/*  Return a new path which is a copy of path from start_pos to the end,     */
/*  starting from mark.                                                      */
/*                                                                           */
/*****************************************************************************/

KHE_PATH KhePathCopy(KHE_PATH path, int start_pos, KHE_MARK mark)
{
  KHE_PATH res;  KHE_PATH_OP op;  int i;  KHE_SOLN soln;

  /* get res, from the solution free list or fresh, and initialize it */
  soln = KheMarkSoln(mark);
  res = KheSolnTakePathFromFreeList(soln);
  if( res == NULL )
    res = KhePathMake(path->soln);
  res->mark = mark;
  /* res->info = -1; */
  res->soln_cost = KheSolnCost(soln);
  HaArrayClear(res->operations);

  /* copy the operations, including incrementing reference counts */
  for( i = start_pos;  i < HaArrayCount(path->operations);  i++ )
  {
    op = HaArray(path->operations, i);
    HaArrayAddLast(res->operations, op);
    switch( op.type )
    {
      /* operations on meets */
      case KHE_PATH_OP_MEET_SET_BACK:

	break;

      case KHE_PATH_OP_MEET_ADD:

	KheMeetReferenceCountIncrement(op.u.meet_add.meet);
	break;

      case KHE_PATH_OP_MEET_DELETE:

	KheMeetReferenceCountIncrement(op.u.meet_delete.meet);
	break;

      case KHE_PATH_OP_MEET_SPLIT:

	KheMeetReferenceCountIncrement(op.u.meet_split.meet2);
	break;

      case KHE_PATH_OP_MEET_MERGE:

	KheMeetReferenceCountIncrement(op.u.meet_merge.meet2);
	break;

      case KHE_PATH_OP_MEET_MOVE:
      case KHE_PATH_OP_MEET_SET_AUTO_DOMAIN:
      case KHE_PATH_OP_MEET_ASSIGN_FIX:
      case KHE_PATH_OP_MEET_ASSIGN_UNFIX:
      case KHE_PATH_OP_MEET_ADD_MEET_BOUND:
      case KHE_PATH_OP_MEET_DELETE_MEET_BOUND:

	break;


      /* operations on meet bounds */
      case KHE_PATH_OP_MEET_BOUND_ADD:

        KheMeetBoundReferenceCountIncrement(op.u.meet_bound_add.meet_bound);
	break;

      case KHE_PATH_OP_MEET_BOUND_DELETE:

        KheMeetBoundReferenceCountIncrement(op.u.meet_bound_delete.meet_bound);
	break;

      case KHE_PATH_OP_MEET_BOUND_ADD_TIME_GROUP:
      case KHE_PATH_OP_MEET_BOUND_DELETE_TIME_GROUP:

	break;


      /* operations on tasks */
      case KHE_PATH_OP_TASK_SET_BACK:

	break;

      case KHE_PATH_OP_TASK_ADD:

	KheTaskReferenceCountIncrement(op.u.task_add.task);
	break;

      case KHE_PATH_OP_TASK_DELETE:

	KheTaskReferenceCountIncrement(op.u.task_delete.task);
	break;

      case KHE_PATH_OP_TASK_SPLIT:

	KheTaskReferenceCountIncrement(op.u.task_split.task2);
	break;

      case KHE_PATH_OP_TASK_MERGE:

	KheTaskReferenceCountIncrement(op.u.task_merge.task2);
	break;

      case KHE_PATH_OP_TASK_MOVE:
      case KHE_PATH_OP_TASK_ASSIGN_FIX:
      case KHE_PATH_OP_TASK_ASSIGN_UNFIX:
      case KHE_PATH_OP_TASK_ADD_TASK_BOUND:
      case KHE_PATH_OP_TASK_DELETE_TASK_BOUND:

	break;


      /* operations on task bounds */
      case KHE_PATH_OP_TASK_BOUND_ADD:

        KheTaskBoundReferenceCountIncrement(op.u.task_bound_add.task_bound);
	break;

      case KHE_PATH_OP_TASK_BOUND_DELETE:

        KheTaskBoundReferenceCountIncrement(op.u.task_bound_delete.task_bound);
	break;


      /* operations on nodes */
      case KHE_PATH_OP_NODE_SET_BACK:

	break;

      case KHE_PATH_OP_NODE_ADD:

	KheNodeReferenceCountIncrement(op.u.node_add.node);
	break;

      case KHE_PATH_OP_NODE_DELETE:

	KheNodeReferenceCountIncrement(op.u.node_delete.node);
	break;

      case KHE_PATH_OP_NODE_ADD_PARENT:
      case KHE_PATH_OP_NODE_DELETE_PARENT:
      case KHE_PATH_OP_NODE_SWAP_CHILD_NODES_AND_LAYERS:
      case KHE_PATH_OP_NODE_ADD_MEET:
      case KHE_PATH_OP_NODE_DELETE_MEET:

	break;


      /* operations on layers */
      case KHE_PATH_OP_LAYER_SET_BACK:

	break;

      case KHE_PATH_OP_LAYER_ADD:

	KheLayerReferenceCountIncrement(op.u.layer_add.layer);
	break;

      case KHE_PATH_OP_LAYER_DELETE:

	KheLayerReferenceCountIncrement(op.u.layer_delete.layer);
	break;

      case KHE_PATH_OP_LAYER_ADD_CHILD_NODE:
      case KHE_PATH_OP_LAYER_DELETE_CHILD_NODE:
      case KHE_PATH_OP_LAYER_ADD_RESOURCE:
      case KHE_PATH_OP_LAYER_DELETE_RESOURCE:

	break;


      /* operations on zones */
      case KHE_PATH_OP_ZONE_SET_BACK:

	break;

      case KHE_PATH_OP_ZONE_ADD:

	KheZoneReferenceCountIncrement(op.u.zone_add.zone);
	break;

      case KHE_PATH_OP_ZONE_DELETE:

	KheZoneReferenceCountIncrement(op.u.zone_delete.zone);
	break;

      case KHE_PATH_OP_ZONE_ADD_MEET_OFFSET:
      case KHE_PATH_OP_ZONE_DELETE_MEET_OFFSET:

	break;

      default:

	HnAbort("KheMarkAddPath internal error 2");
	break;
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathUndo(KHE_PATH path, int start_pos)                           */
/*                                                                           */
/*  Undo path, from the end back to start_pos inclusive.  This path is       */
/*  always the solution's main path, and we always pop the undone ops        */
/*  off the path as we undo them.                                            */
/*                                                                           */
/*****************************************************************************/

void KhePathUndo(KHE_PATH path, int start_pos)
{
  KHE_PATH_OP op;
  while( HaArrayCount(path->operations) > start_pos )
  {
    op = HaArrayLastAndDelete(path->operations);
    switch( op.type )
    {
      /* operations on meets */
      case KHE_PATH_OP_MEET_SET_BACK:

	KheMeetKernelSetBackUndo(op.u.meet_set_back.meet,
	  op.u.meet_set_back.old_back, op.u.meet_set_back.new_back);
	break;

      case KHE_PATH_OP_MEET_ADD:

	KheMeetKernelAddUndo(op.u.meet_add.meet);
	KheMeetReferenceCountDecrement(op.u.meet_add.meet);
	break;

      case KHE_PATH_OP_MEET_DELETE:

	KheMeetKernelDeleteUndo(op.u.meet_add.meet, path->soln,
	  op.u.meet_add.duration, op.u.meet_add.event);
	KheMeetReferenceCountDecrement(op.u.meet_delete.meet);
	break;

      case KHE_PATH_OP_MEET_SPLIT:

	KheMeetKernelSplitUndo(op.u.meet_split.meet1, op.u.meet_split.meet2,
	  op.u.meet_split.durn1);
	KheMeetReferenceCountDecrement(op.u.meet_split.meet2);
	break;

      case KHE_PATH_OP_MEET_MERGE:

	KheMeetKernelMergeUndo(op.u.meet_merge.meet1, op.u.meet_merge.meet2,
	  op.u.meet_merge.durn1);
	KheMeetReferenceCountDecrement(op.u.meet_merge.meet2);
	break;

      case KHE_PATH_OP_MEET_MOVE:

	KheMeetKernelMoveUndo(op.u.meet_move.meet,
          op.u.meet_move.old_target_meet, op.u.meet_move.old_offset,
          op.u.meet_move.new_target_meet, op.u.meet_move.new_offset);
	break;

      case KHE_PATH_OP_MEET_SET_AUTO_DOMAIN:

	KheMeetKernelSetAutoDomainUndo(op.u.meet_set_auto_domain.meet,
	  op.u.meet_set_auto_domain.automatic);
	break;

      case KHE_PATH_OP_MEET_ASSIGN_FIX:

        KheMeetKernelAssignFixUndo(op.u.meet_assign_fix.meet);
	break;

      case KHE_PATH_OP_MEET_ASSIGN_UNFIX:

        KheMeetKernelAssignUnFixUndo(op.u.meet_assign_unfix.meet);
	break;

      case KHE_PATH_OP_MEET_ADD_MEET_BOUND:

	KheMeetKernelAddMeetBoundUndo(op.u.meet_add_meet_bound.meet,
          op.u.meet_add_meet_bound.meet_bound);
	break;

      case KHE_PATH_OP_MEET_DELETE_MEET_BOUND:

	KheMeetKernelDeleteMeetBoundUndo(op.u.meet_delete_meet_bound.meet,
          op.u.meet_delete_meet_bound.meet_bound);
	break;


      /* operations on meet bounds */
      case KHE_PATH_OP_MEET_BOUND_ADD:

	KheMeetBoundKernelAddUndo(op.u.meet_bound_add.meet_bound);
	KheMeetBoundReferenceCountDecrement(op.u.meet_bound_add.meet_bound);
	break;

      case KHE_PATH_OP_MEET_BOUND_DELETE:

	KheMeetBoundKernelDeleteUndo(op.u.meet_bound_delete.meet_bound,
          path->soln, op.u.meet_bound_delete.occupancy,
	  op.u.meet_bound_delete.dft_tg);
	KheMeetBoundReferenceCountDecrement(op.u.meet_bound_delete.meet_bound);
	break;

      case KHE_PATH_OP_MEET_BOUND_ADD_TIME_GROUP:

	KheMeetBoundKernelAddTimeGroupUndo(
          op.u.meet_bound_add_time_group.meet_bound,
          op.u.meet_bound_add_time_group.duration,
          op.u.meet_bound_add_time_group.time_group);
	break;

      case KHE_PATH_OP_MEET_BOUND_DELETE_TIME_GROUP:

	KheMeetBoundKernelDeleteTimeGroupUndo(
          op.u.meet_bound_delete_time_group.meet_bound,
          op.u.meet_bound_delete_time_group.duration,
          op.u.meet_bound_delete_time_group.time_group);
	break;


      /* operations on tasks */
      case KHE_PATH_OP_TASK_SET_BACK:

	KheTaskKernelSetBackUndo(op.u.task_set_back.task,
	  op.u.task_set_back.old_back, op.u.task_set_back.new_back);
	break;

      case KHE_PATH_OP_TASK_ADD:

	KheTaskKernelAddUndo(op.u.task_add.task);
	KheTaskReferenceCountDecrement(op.u.task_add.task);
	break;

      case KHE_PATH_OP_TASK_DELETE:

	KheTaskKernelDeleteUndo(op.u.task_delete.task, path->soln,
          op.u.task_delete.resource_type, op.u.task_delete.meet,
          op.u.task_delete.event_resource);
	KheTaskReferenceCountDecrement(op.u.task_delete.task);
	break;

      case KHE_PATH_OP_TASK_SPLIT:

	KheTaskKernelSplitUndo(op.u.task_split.task1, op.u.task_split.task2,
	  op.u.task_split.durn1, op.u.task_split.meet2);
	KheTaskReferenceCountDecrement(op.u.task_split.task2);
	break;

      case KHE_PATH_OP_TASK_MERGE:

	KheTaskKernelMergeUndo(op.u.task_merge.task1, op.u.task_merge.task2,
	  op.u.task_merge.durn1, op.u.task_merge.meet2);
	KheTaskReferenceCountDecrement(op.u.task_merge.task2);
	break;

      case KHE_PATH_OP_TASK_MOVE:

	KheTaskKernelMoveUndo(op.u.task_move.task,
	  op.u.task_move.old_target_task, op.u.task_move.new_target_task);
	break;

      case KHE_PATH_OP_TASK_ASSIGN_FIX:

	KheTaskKernelAssignFixUndo(op.u.task_assign_fix.task);
	break;

      case KHE_PATH_OP_TASK_ASSIGN_UNFIX:

	KheTaskKernelAssignUnFixUndo(op.u.task_assign_unfix.task);
	break;

      case KHE_PATH_OP_TASK_ADD_TASK_BOUND:

	KheTaskKernelAddTaskBoundUndo(op.u.task_add_task_bound.task,
          op.u.task_add_task_bound.task_bound);
	break;

      case KHE_PATH_OP_TASK_DELETE_TASK_BOUND:

	KheTaskKernelDeleteTaskBoundUndo(op.u.task_delete_task_bound.task,
          op.u.task_delete_task_bound.task_bound);
	break;


      /* operations on task bounds */
      case KHE_PATH_OP_TASK_BOUND_ADD:

	KheTaskBoundKernelAddUndo(op.u.task_bound_add.task_bound);
	KheTaskBoundReferenceCountDecrement(op.u.task_bound_add.task_bound);
	break;

      case KHE_PATH_OP_TASK_BOUND_DELETE:

	KheTaskBoundKernelDeleteUndo(op.u.task_bound_delete.task_bound,
          path->soln, op.u.task_bound_delete.resource_group);
	KheTaskBoundReferenceCountDecrement(op.u.task_bound_add.task_bound);
	break;


      /* operations on nodes */
      case KHE_PATH_OP_NODE_SET_BACK:

	KheNodeKernelSetBackUndo(op.u.node_set_back.node,
	  op.u.node_set_back.old_back, op.u.node_set_back.new_back);
	break;

      case KHE_PATH_OP_NODE_ADD:

	KheNodeKernelAddUndo(op.u.node_add.node);
	KheNodeReferenceCountDecrement(op.u.node_add.node);
	break;

      case KHE_PATH_OP_NODE_DELETE:

	KheNodeKernelDeleteUndo(op.u.node_add.node, path->soln);
	KheNodeReferenceCountDecrement(op.u.node_delete.node);
	break;

      case KHE_PATH_OP_NODE_ADD_PARENT:

	KheNodeKernelAddParentUndo(op.u.node_add_parent.child_node,
	  op.u.node_add_parent.parent_node);
	break;

      case KHE_PATH_OP_NODE_DELETE_PARENT:

	KheNodeKernelDeleteParentUndo(op.u.node_delete_parent.child_node,
	  op.u.node_delete_parent.parent_node);
	break;

      case KHE_PATH_OP_NODE_SWAP_CHILD_NODES_AND_LAYERS:

	KheNodeKernelSwapChildNodesAndLayersUndo(
	  op.u.node_swap_child_nodes_and_layers.node1,
	  op.u.node_swap_child_nodes_and_layers.node2);
	break;

      case KHE_PATH_OP_NODE_ADD_MEET:

	KheNodeKernelAddMeetUndo(op.u.node_add_meet.node,
	  op.u.node_add_meet.meet);
	break;

      case KHE_PATH_OP_NODE_DELETE_MEET:

	KheNodeKernelDeleteMeetUndo(op.u.node_delete_meet.node,
	  op.u.node_delete_meet.meet);
	break;


      /* operations on layers */
      case KHE_PATH_OP_LAYER_SET_BACK:

	KheLayerKernelSetBackUndo(op.u.layer_set_back.layer,
	  op.u.layer_set_back.old_back, op.u.layer_set_back.new_back);
	break;

      case KHE_PATH_OP_LAYER_ADD:

	KheLayerKernelAddUndo(op.u.layer_add.layer);
	KheLayerReferenceCountDecrement(op.u.layer_add.layer);
	break;

      case KHE_PATH_OP_LAYER_DELETE:

	KheLayerKernelDeleteUndo(op.u.layer_delete.layer,
	  op.u.layer_delete.parent_node);
	KheLayerReferenceCountDecrement(op.u.layer_delete.layer);
	break;

      case KHE_PATH_OP_LAYER_ADD_CHILD_NODE:

	KheLayerKernelAddChildNodeUndo(op.u.layer_add_child_node.layer,
	  op.u.layer_add_child_node.child_node);
	break;

      case KHE_PATH_OP_LAYER_DELETE_CHILD_NODE:

	KheLayerKernelDeleteChildNodeUndo(op.u.layer_delete_child_node.layer,
	  op.u.layer_delete_child_node.child_node);
	break;

      case KHE_PATH_OP_LAYER_ADD_RESOURCE:

	KheLayerKernelAddResourceUndo(op.u.layer_add_resource.layer,
	  op.u.layer_add_resource.resource);
	break;

      case KHE_PATH_OP_LAYER_DELETE_RESOURCE:

	KheLayerKernelDeleteResourceUndo(op.u.layer_delete_resource.layer,
	  op.u.layer_delete_resource.resource);
	break;


      /* operations on zones */
      case KHE_PATH_OP_ZONE_SET_BACK:

	KheZoneKernelSetBackUndo(op.u.zone_set_back.zone,
	  op.u.zone_set_back.old_back, op.u.zone_set_back.new_back);
	break;

      case KHE_PATH_OP_ZONE_ADD:

	KheZoneKernelAddUndo(op.u.zone_add.zone);
	KheZoneReferenceCountDecrement(op.u.zone_add.zone);
	break;

      case KHE_PATH_OP_ZONE_DELETE:

	KheZoneKernelDeleteUndo(op.u.zone_delete.zone, op.u.zone_delete.node);
	KheZoneReferenceCountDecrement(op.u.zone_delete.zone);
	break;

      case KHE_PATH_OP_ZONE_ADD_MEET_OFFSET:

	KheZoneKernelAddMeetOffsetUndo(op.u.zone_add_meet_offset.zone,
	  op.u.zone_add_meet_offset.meet, op.u.zone_add_meet_offset.offset);
	break;

      case KHE_PATH_OP_ZONE_DELETE_MEET_OFFSET:

	KheZoneKernelDeleteMeetOffsetUndo(op.u.zone_add_meet_offset.zone,
	  op.u.zone_add_meet_offset.meet, op.u.zone_add_meet_offset.offset);
	break;


      default:

        HnAbort("KhePathUndo internal error");
	break;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathRedo(KHE_PATH path)                                          */
/*                                                                           */
/*  Redo path, from the beginning to the end.                                */
/*                                                                           */
/*****************************************************************************/

void KhePathRedo(KHE_PATH path)
{
  KHE_PATH_OP op;  int i;
  HnAssert(KheMarkIsCurrent(path->mark),
    "KhePathRedo: solution is not in path's mark's state");
  HaArrayForEach(path->operations, op, i)
    switch( op.type )
    {
      /* operations on meets */
      case KHE_PATH_OP_MEET_SET_BACK:

	KheMeetKernelSetBack(op.u.meet_set_back.meet,
	  op.u.meet_set_back.new_back);
	break;

      case KHE_PATH_OP_MEET_ADD:

	KheMeetKernelAdd(op.u.meet_add.meet, path->soln,
	  op.u.meet_add.duration, op.u.meet_add.event);
	break;

      case KHE_PATH_OP_MEET_DELETE:

	KheMeetKernelDelete(op.u.meet_add.meet);
	break;

      case KHE_PATH_OP_MEET_SPLIT:

	KheMeetKernelSplit(op.u.meet_split.meet1, op.u.meet_split.meet2,
	  op.u.meet_split.durn1);
	break;

      case KHE_PATH_OP_MEET_MERGE:

	KheMeetKernelMerge(op.u.meet_merge.meet1, op.u.meet_merge.meet2,
	  op.u.meet_merge.durn1);
	break;

      case KHE_PATH_OP_MEET_MOVE:

	KheMeetKernelMove(op.u.meet_move.meet,
          op.u.meet_move.new_target_meet, op.u.meet_move.new_offset);
	break;

      case KHE_PATH_OP_MEET_SET_AUTO_DOMAIN:

	KheMeetKernelSetAutoDomain(op.u.meet_set_auto_domain.meet,
	  op.u.meet_set_auto_domain.automatic);
	break;

      case KHE_PATH_OP_MEET_ASSIGN_FIX:

        KheMeetKernelAssignFix(op.u.meet_assign_fix.meet);
	break;

      case KHE_PATH_OP_MEET_ASSIGN_UNFIX:

        KheMeetKernelAssignUnFix(op.u.meet_assign_unfix.meet);
	break;

      case KHE_PATH_OP_MEET_ADD_MEET_BOUND:

	KheMeetKernelAddMeetBound(op.u.meet_add_meet_bound.meet,
          op.u.meet_add_meet_bound.meet_bound);
	break;

      case KHE_PATH_OP_MEET_DELETE_MEET_BOUND:

	KheMeetKernelDeleteMeetBound(op.u.meet_delete_meet_bound.meet,
          op.u.meet_delete_meet_bound.meet_bound);
	break;


      /* operations on meet bounds */
      case KHE_PATH_OP_MEET_BOUND_ADD:

	KheMeetBoundKernelAdd(op.u.meet_bound_add.meet_bound, path->soln,
          op.u.meet_bound_add.occupancy, op.u.meet_bound_add.dft_tg);
	break;

      case KHE_PATH_OP_MEET_BOUND_DELETE:

	KheMeetBoundKernelDelete(op.u.meet_bound_add.meet_bound);
	break;

      case KHE_PATH_OP_MEET_BOUND_ADD_TIME_GROUP:

	KheMeetBoundKernelAddTimeGroup(
	  op.u.meet_bound_add_time_group.meet_bound,
          op.u.meet_bound_add_time_group.duration,
          op.u.meet_bound_add_time_group.time_group);
	break;

      case KHE_PATH_OP_MEET_BOUND_DELETE_TIME_GROUP:

	KheMeetBoundKernelDeleteTimeGroup(
	  op.u.meet_bound_delete_time_group.meet_bound,
          op.u.meet_bound_delete_time_group.duration,
          op.u.meet_bound_delete_time_group.time_group);
	break;


      /* operations on tasks */
      case KHE_PATH_OP_TASK_SET_BACK:

	KheTaskKernelSetBack(op.u.task_set_back.task,
	  op.u.task_set_back.new_back);
	break;

      case KHE_PATH_OP_TASK_ADD:

	KheTaskKernelAdd(op.u.task_add.task, path->soln,
	  op.u.task_add.resource_type, op.u.task_add.meet,
	  op.u.task_add.event_resource);
	break;

      case KHE_PATH_OP_TASK_DELETE:

	KheTaskKernelDelete(op.u.task_delete.task);
	break;

      case KHE_PATH_OP_TASK_SPLIT:

	KheTaskKernelSplit(op.u.task_split.task1, op.u.task_split.task2,
	  op.u.task_split.durn1, op.u.task_split.meet2);
	break;

      case KHE_PATH_OP_TASK_MERGE:

	KheTaskKernelMerge(op.u.task_merge.task1, op.u.task_merge.task2,
	  op.u.task_merge.durn1, op.u.task_merge.meet2);
	break;

      case KHE_PATH_OP_TASK_MOVE:

	KheTaskKernelMove(op.u.task_move.task,
	  op.u.task_move.new_target_task);
	break;

      case KHE_PATH_OP_TASK_ASSIGN_FIX:

	KheTaskKernelAssignFix(op.u.task_assign_fix.task);
	break;

      case KHE_PATH_OP_TASK_ASSIGN_UNFIX:

	KheTaskKernelAssignUnFix(op.u.task_assign_unfix.task);
	break;

      case KHE_PATH_OP_TASK_ADD_TASK_BOUND:

	KheTaskKernelAddTaskBound(op.u.task_add_task_bound.task,
          op.u.task_add_task_bound.task_bound);
	break;

      case KHE_PATH_OP_TASK_DELETE_TASK_BOUND:

	KheTaskKernelDeleteTaskBound(op.u.task_delete_task_bound.task,
          op.u.task_delete_task_bound.task_bound);
	break;


      /* operations on task bounds */
      case KHE_PATH_OP_TASK_BOUND_ADD:

	KheTaskBoundKernelAdd(op.u.task_bound_add.task_bound,
          path->soln, op.u.task_bound_add.resource_group);
	break;

      case KHE_PATH_OP_TASK_BOUND_DELETE:

	KheTaskBoundKernelDelete(op.u.task_bound_add.task_bound);
	break;


      /* operations on nodes */
      case KHE_PATH_OP_NODE_SET_BACK:

	KheNodeKernelSetBack(op.u.node_set_back.node,
	  op.u.node_set_back.new_back);
	break;

      case KHE_PATH_OP_NODE_ADD:

	KheNodeKernelAdd(op.u.node_add.node, path->soln);
	break;

      case KHE_PATH_OP_NODE_DELETE:

	KheNodeKernelDelete(op.u.node_delete.node);
	break;

      case KHE_PATH_OP_NODE_ADD_PARENT:

	KheNodeKernelAddParent(op.u.node_add_parent.child_node,
	  op.u.node_add_parent.parent_node);
	break;

      case KHE_PATH_OP_NODE_DELETE_PARENT:

	KheNodeKernelDeleteParent(op.u.node_delete_parent.child_node,
	  op.u.node_delete_parent.parent_node);
	break;

      case KHE_PATH_OP_NODE_SWAP_CHILD_NODES_AND_LAYERS:

	KheNodeKernelSwapChildNodesAndLayers(
	  op.u.node_swap_child_nodes_and_layers.node2,
	  op.u.node_swap_child_nodes_and_layers.node2);
	break;

      case KHE_PATH_OP_NODE_ADD_MEET:

	KheNodeKernelAddMeet(op.u.node_add_meet.node,
	  op.u.node_add_meet.meet);
	break;

      case KHE_PATH_OP_NODE_DELETE_MEET:

	KheNodeKernelDeleteMeet(op.u.node_delete_meet.node,
	  op.u.node_delete_meet.meet);
	break;


      /* operations on layers */
      case KHE_PATH_OP_LAYER_SET_BACK:

	KheLayerKernelSetBack(op.u.layer_set_back.layer,
	  op.u.layer_set_back.new_back);
	break;

      case KHE_PATH_OP_LAYER_ADD:

	KheLayerKernelAdd(op.u.layer_add.layer, op.u.layer_add.parent_node);
	break;

      case KHE_PATH_OP_LAYER_DELETE:

	KheLayerKernelDelete(op.u.layer_delete.layer);
	break;

      case KHE_PATH_OP_LAYER_ADD_CHILD_NODE:

	KheLayerKernelAddChildNode(op.u.layer_add_child_node.layer,
	  op.u.layer_add_child_node.child_node);
	break;

      case KHE_PATH_OP_LAYER_DELETE_CHILD_NODE:

	KheLayerKernelDeleteChildNode(op.u.layer_delete_child_node.layer,
	  op.u.layer_delete_child_node.child_node);
	break;

      case KHE_PATH_OP_LAYER_ADD_RESOURCE:

	KheLayerKernelAddResource(op.u.layer_add_resource.layer,
	  op.u.layer_add_resource.resource);
	break;

      case KHE_PATH_OP_LAYER_DELETE_RESOURCE:

	KheLayerKernelDeleteResource(op.u.layer_delete_resource.layer,
	  op.u.layer_delete_resource.resource);
	break;


      /* operations on zones */
      case KHE_PATH_OP_ZONE_SET_BACK:

	KheZoneKernelSetBack(op.u.zone_set_back.zone,
	  op.u.zone_set_back.new_back);
	break;

      case KHE_PATH_OP_ZONE_ADD:

	KheZoneKernelAdd(op.u.zone_add.zone, op.u.zone_add.node);
	break;

      case KHE_PATH_OP_ZONE_DELETE:

	KheZoneKernelDelete(op.u.zone_delete.zone);
	break;

      case KHE_PATH_OP_ZONE_ADD_MEET_OFFSET:

	KheZoneKernelAddMeetOffset(op.u.zone_add_meet_offset.zone,
	  op.u.zone_add_meet_offset.meet, op.u.zone_add_meet_offset.offset);
	break;

      case KHE_PATH_OP_ZONE_DELETE_MEET_OFFSET:

	KheZoneKernelDeleteMeetOffset(op.u.zone_delete_meet_offset.zone,
	  op.u.zone_delete_meet_offset.meet,
	  op.u.zone_delete_meet_offset.offset);
	break;


      default:

        HnAbort("KhePathRedo internal error");
	break;
    }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "operation loading - meets"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_PATH_OP *GetOp(KHE_PATH path)                                        */
/*                                                                           */
/*  Get a new operation object for path, either from t's free list or fresh. */
/*                                                                           */
/*****************************************************************************/

static KHE_PATH_OP *GetOp(KHE_PATH path)
{
  static KHE_PATH_OP op;
  HaArrayAddLast(path->operations, op);
  return &HaArrayLast(path->operations);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpMeetSetBack(KHE_PATH path,                                 */
/*    KHE_MEET meet, void *old_back, void *new_back)                         */
/*                                                                           */
/*  Add a record of a call to KheMeetSetBack to path.                        */
/*                                                                           */
/*****************************************************************************/

void KhePathOpMeetSetBack(KHE_PATH path,
  KHE_MEET meet, void *old_back, void *new_back)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_MEET_SET_BACK;
  op->u.meet_set_back.meet = meet;
  op->u.meet_set_back.old_back = old_back;
  op->u.meet_set_back.new_back = new_back;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpMeetAdd(KHE_PATH path, KHE_MEET meet, int duration,        */
/*    KHE_EVENT e)                                                           */
/*                                                                           */
/*  Add a record of a call to KheMeetAdd to path.                            */
/*                                                                           */
/*****************************************************************************/

void KhePathOpMeetAdd(KHE_PATH path, KHE_MEET meet, int duration,
  KHE_EVENT e)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_MEET_ADD;
  op->u.meet_add.meet = meet;
  op->u.meet_add.duration = duration;
  op->u.meet_add.event = e;
  KheMeetReferenceCountIncrement(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpMeetDelete(KHE_PATH path, KHE_MEET meet, int duration,     */
/*    KHE_EVENT e)                                                           */
/*                                                                           */
/*  Add a record of a call to KheMeetDelete to path.                         */
/*                                                                           */
/*****************************************************************************/

void KhePathOpMeetDelete(KHE_PATH path, KHE_MEET meet, int duration,
  KHE_EVENT e)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_MEET_DELETE;
  op->u.meet_delete.meet = meet;
  op->u.meet_delete.duration = duration;
  op->u.meet_delete.event = e;
  KheMeetReferenceCountIncrement(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpMeetSplit(KHE_PATH path, KHE_MEET meet1, KHE_MEET meet2,   */
/*    int durn1)                                                             */
/*                                                                           */
/*  Add a record of a call to KheMeetSplit to path.                          */
/*                                                                           */
/*****************************************************************************/

void KhePathOpMeetSplit(KHE_PATH path, KHE_MEET meet1, KHE_MEET meet2,
  int durn1)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_MEET_SPLIT;
  op->u.meet_split.meet1 = meet1;
  op->u.meet_split.meet2 = meet2;
  op->u.meet_split.durn1 = durn1;
  KheMeetReferenceCountIncrement(meet2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpMeetMerge(KHE_PATH path, KHE_MEET meet1, KHE_MEET meet2,   */
/*    int durn1)                                                             */
/*                                                                           */
/*  Add a record of a call to KheMeetMerge to path.                          */
/*                                                                           */
/*****************************************************************************/

void KhePathOpMeetMerge(KHE_PATH path, KHE_MEET meet1, KHE_MEET meet2,
  int durn1)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_MEET_MERGE;
  op->u.meet_merge.meet1 = meet1;
  op->u.meet_merge.meet2 = meet2;
  op->u.meet_merge.durn1 = durn1;
  KheMeetReferenceCountIncrement(meet2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpMeetMove(KHE_PATH path, KHE_MEET meet,                     */
/*    KHE_MEET old_target_meet, int old_offset,                              */
/*    KHE_MEET new_target_meet, int new_offset)                              */
/*                                                                           */
/*  Add a record of a call to KheMeetMove to path, unless redundant.         */
/*                                                                           */
/*****************************************************************************/

void KhePathOpMeetMove(KHE_PATH path, KHE_MEET meet,
  KHE_MEET old_target_meet, int old_offset,
  KHE_MEET new_target_meet, int new_offset)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_MEET_MOVE;
  op->u.meet_move.meet = meet;
  op->u.meet_move.old_target_meet = old_target_meet;
  op->u.meet_move.old_offset = old_offset;
  op->u.meet_move.new_target_meet = new_target_meet;
  op->u.meet_move.new_offset = new_offset;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpMeetSetAutoDomain(KHE_PATH path, KHE_MEET meet,            */
/*    bool automatic)                                                        */
/*                                                                           */
/*  Add a record of a call to KheMeetSetAutoDomain to path.                  */
/*                                                                           */
/*****************************************************************************/

void KhePathOpMeetSetAutoDomain(KHE_PATH path, KHE_MEET meet, bool automatic)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_MEET_SET_AUTO_DOMAIN;
  op->u.meet_set_auto_domain.meet = meet;
  op->u.meet_set_auto_domain.automatic = automatic;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpMeetAssignFix(KHE_PATH path, KHE_MEET meet)                */
/*                                                                           */
/*  Add a record of a call to KheMeetAssignFix(meet).                        */
/*                                                                           */
/*****************************************************************************/

void KhePathOpMeetAssignFix(KHE_PATH path, KHE_MEET meet)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_MEET_ASSIGN_FIX;
  op->u.meet_assign_fix.meet = meet;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpMeetAssignUnFix(KHE_PATH t, KHE_MEET meet)                 */
/*                                                                           */
/*  Add a record of a call to KheMeetAssignUnFix(meet).                      */
/*                                                                           */
/*****************************************************************************/

void KhePathOpMeetAssignUnFix(KHE_PATH path, KHE_MEET meet)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_MEET_ASSIGN_UNFIX;
  op->u.meet_assign_unfix.meet = meet;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpMeetAddMeetBound(KHE_PATH path, KHE_MEET meet,             */
/*    KHE_MEET_BOUND mb)                                                     */
/*                                                                           */
/*  Add a record of a call to KheMeetAddMeetBound(meet, mb).                 */
/*                                                                           */
/*****************************************************************************/

void KhePathOpMeetAddMeetBound(KHE_PATH path, KHE_MEET meet,
  KHE_MEET_BOUND mb)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_MEET_ADD_MEET_BOUND;
  op->u.meet_add_meet_bound.meet = meet;
  op->u.meet_add_meet_bound.meet_bound = mb;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpMeetDeleteMeetBound(KHE_PATH path, KHE_MEET meet,          */
/*    KHE_MEET_BOUND mb)                                                     */
/*                                                                           */
/*  Add a record of a call to KheMeetDeleteMeetBound(meet, mb).              */
/*                                                                           */
/*****************************************************************************/

void KhePathOpMeetDeleteMeetBound(KHE_PATH path, KHE_MEET meet,
  KHE_MEET_BOUND mb)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_MEET_DELETE_MEET_BOUND;
  op->u.meet_delete_meet_bound.meet = meet;
  op->u.meet_delete_meet_bound.meet_bound = mb;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "operation loading - meet bounds"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpMeetBoundAdd(KHE_PATH path, KHE_MEET_BOUND mb,             */
/*    bool occupancy, KHE_TIME_GROUP dft_tg)                                 */
/*                                                                           */
/*  Add a record of a call to KheMeetBoundAdd to path.                       */
/*                                                                           */
/*****************************************************************************/

void KhePathOpMeetBoundAdd(KHE_PATH path, KHE_MEET_BOUND mb,
  bool occupancy, KHE_TIME_GROUP dft_tg)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_MEET_BOUND_ADD;
  op->u.meet_bound_add.meet_bound = mb;
  op->u.meet_bound_add.occupancy = occupancy;
  op->u.meet_bound_add.dft_tg = dft_tg;
  KheMeetBoundReferenceCountIncrement(mb);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpMeetBoundDelete(KHE_PATH path, KHE_MEET_BOUND mb,          */
/*    bool occupancy, KHE_TIME_GROUP dft_tg)                                 */
/*                                                                           */
/*  Add a record of a call to KheMeetBoundDelete to path.                    */
/*                                                                           */
/*****************************************************************************/

void KhePathOpMeetBoundDelete(KHE_PATH path, KHE_MEET_BOUND mb,
  bool occupancy, KHE_TIME_GROUP dft_tg)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_MEET_BOUND_DELETE;
  op->u.meet_bound_delete.meet_bound = mb;
  op->u.meet_bound_delete.occupancy = occupancy;
  op->u.meet_bound_delete.dft_tg = dft_tg;
  KheMeetBoundReferenceCountIncrement(mb);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpMeetBoundAddTimeGroup(KHE_PATH path, KHE_MEET_BOUND mb,    */
/*    int duration, KHE_TIME_GROUP tg)                                       */
/*                                                                           */
/*  Add a record of a call to KheMeetBoundAddTimeGroup to path.              */
/*                                                                           */
/*****************************************************************************/

void KhePathOpMeetBoundAddTimeGroup(KHE_PATH path, KHE_MEET_BOUND mb,
  int duration, KHE_TIME_GROUP tg)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_MEET_BOUND_ADD_TIME_GROUP;
  op->u.meet_bound_add_time_group.meet_bound = mb;
  op->u.meet_bound_add_time_group.duration = duration;
  op->u.meet_bound_add_time_group.time_group = tg;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpMeetBoundDeleteTimeGroup(KHE_PATH path, KHE_MEET_BOUND mb, */
/*    int duration, KHE_TIME_GROUP tg)                                       */
/*                                                                           */
/*  Add a record of a call to KheMeetBoundDeleteTimeGroup to path.           */
/*                                                                           */
/*****************************************************************************/

void KhePathOpMeetBoundDeleteTimeGroup(KHE_PATH path, KHE_MEET_BOUND mb,
  int duration, KHE_TIME_GROUP tg)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_MEET_BOUND_DELETE_TIME_GROUP;
  op->u.meet_bound_delete_time_group.meet_bound = mb;
  op->u.meet_bound_delete_time_group.duration = duration;
  op->u.meet_bound_delete_time_group.time_group = tg;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "operation loading - tasks"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpTaskSetBack(KHE_PATH path, KHE_TASK task,                  */
/*    void *old_back, void *new_back)                                        */
/*                                                                           */
/*  Add a record of a call to KheTaskSetBack to path.                        */
/*                                                                           */
/*****************************************************************************/

void KhePathOpTaskSetBack(KHE_PATH path, KHE_TASK task,
  void *old_back, void *new_back)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_TASK_SET_BACK;
  op->u.task_set_back.task = task;
  op->u.task_set_back.old_back = old_back;
  op->u.task_set_back.new_back = new_back;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpTaskAdd(KHE_PATH path, KHE_TASK task)                      */
/*                                                                           */
/*  Add a record of a call to KheTaskMake to path.                           */
/*                                                                           */
/*****************************************************************************/

void KhePathOpTaskAdd(KHE_PATH path, KHE_TASK task, KHE_RESOURCE_TYPE rt,
  KHE_MEET meet, KHE_EVENT_RESOURCE er)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_TASK_ADD;
  op->u.task_add.task = task;
  op->u.task_add.resource_type = rt;
  op->u.task_add.meet = meet;
  op->u.task_add.event_resource = er;
  KheTaskReferenceCountIncrement(task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpTaskDelete(KHE_PATH path, KHE_TASK task,                   */
/*    KHE_RESOURCE_TYPE rt, KHE_MEET meet, KHE_EVENT_RESOURCE er)            */
/*                                                                           */
/*  Add a record of a call to KheTaskDelete to path.                         */
/*                                                                           */
/*****************************************************************************/

void KhePathOpTaskDelete(KHE_PATH path, KHE_TASK task,
  KHE_RESOURCE_TYPE rt, KHE_MEET meet, KHE_EVENT_RESOURCE er)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_TASK_DELETE;
  op->u.task_delete.task = task;
  op->u.task_delete.resource_type = rt;
  op->u.task_delete.meet = meet;
  op->u.task_delete.event_resource = er;
  KheTaskReferenceCountIncrement(task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpTaskSplit(KHE_PATH path, KHE_TASK task1,                   */
/*    KHE_TASK task2, int durn1, KHE_MEET meet2)                             */
/*                                                                           */
/*  Add a record of a task split.                                            */
/*                                                                           */
/*****************************************************************************/

void KhePathOpTaskSplit(KHE_PATH path, KHE_TASK task1,
  KHE_TASK task2, int durn1, KHE_MEET meet2)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_TASK_SPLIT;
  op->u.task_split.task1 = task1;
  op->u.task_split.task2 = task2;
  op->u.task_split.durn1 = durn1;
  op->u.task_split.meet2 = meet2;
  KheTaskReferenceCountIncrement(task2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpTaskMerge(KHE_PATH path, KHE_TASK task1,                   */
/*    KHE_TASK task2, int durn1, KHE_MEET meet2)                             */
/*                                                                           */
/*  Add a record of a task merge.                                            */
/*                                                                           */
/*****************************************************************************/

void KhePathOpTaskMerge(KHE_PATH path, KHE_TASK task1,
  KHE_TASK task2, int durn1, KHE_MEET meet2)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_TASK_MERGE;
  op->u.task_merge.task1 = task1;
  op->u.task_merge.task2 = task2;
  op->u.task_merge.durn1 = durn1;
  op->u.task_merge.meet2 = meet2;
  KheTaskReferenceCountIncrement(task2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpTaskMove(KHE_PATH path, KHE_TASK task,                     */
/*    KHE_TASK old_target_task, KHE_TASK new_target_task)                    */
/*                                                                           */
/*  Add a record of a call to KheTaskMove to path.                           */
/*                                                                           */
/*****************************************************************************/

void KhePathOpTaskMove(KHE_PATH path, KHE_TASK task,
  KHE_TASK old_target_task, KHE_TASK new_target_task)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_TASK_MOVE;
  op->u.task_move.task = task;
  op->u.task_move.old_target_task = old_target_task;
  op->u.task_move.new_target_task = new_target_task;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpTaskAssignFix(KHE_PATH path, KHE_TASK task)                */
/*                                                                           */
/*  Add a record of a call to KheTaskAssignFix(task).                        */
/*                                                                           */
/*****************************************************************************/

void KhePathOpTaskAssignFix(KHE_PATH path, KHE_TASK task)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_TASK_ASSIGN_FIX;
  op->u.task_assign_fix.task = task;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpTaskAssignUnFix(KHE_PATH path, KHE_TASK task)              */
/*                                                                           */
/*  Add a record of a call to KheTaskAssignUnFix(task).                      */
/*                                                                           */
/*****************************************************************************/

void KhePathOpTaskAssignUnFix(KHE_PATH path, KHE_TASK task)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_TASK_ASSIGN_UNFIX;
  op->u.task_assign_unfix.task = task;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpTaskAddTaskBound(KHE_PATH path, KHE_TASK task,             */
/*    KHE_TASK_BOUND tb)                                                     */
/*                                                                           */
/*  Add a record of a call to KheTaskAddTaskBound(task, mb).                 */
/*                                                                           */
/*****************************************************************************/

void KhePathOpTaskAddTaskBound(KHE_PATH path, KHE_TASK task,
  KHE_TASK_BOUND tb)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_TASK_ADD_TASK_BOUND;
  op->u.task_add_task_bound.task = task;
  op->u.task_add_task_bound.task_bound = tb;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpTaskDeleteTaskBound(KHE_PATH path, KHE_TASK task,          */
/*    KHE_TASK_BOUND tb)                                                     */
/*                                                                           */
/*  Add a record of a call to KheTaskDeleteTaskBound(task, tb).              */
/*                                                                           */
/*****************************************************************************/

void KhePathOpTaskDeleteTaskBound(KHE_PATH path, KHE_TASK task,
  KHE_TASK_BOUND tb)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_TASK_DELETE_TASK_BOUND;
  op->u.task_delete_task_bound.task = task;
  op->u.task_delete_task_bound.task_bound = tb;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "operation loading - task bounds"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpTaskBoundAdd(KHE_PATH path, KHE_TASK_BOUND tb,             */
/*    KHE_RESOURCE_GROUP rg)                                                 */
/*                                                                           */
/*  Add a record of a call to KheTaskBoundAdd to path.                       */
/*                                                                           */
/*****************************************************************************/

void KhePathOpTaskBoundAdd(KHE_PATH path, KHE_TASK_BOUND tb,
  KHE_RESOURCE_GROUP rg)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_TASK_BOUND_ADD;
  op->u.task_bound_add.task_bound = tb;
  op->u.task_bound_add.resource_group = rg;
  KheTaskBoundReferenceCountIncrement(tb);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpTaskBoundDelete(KHE_PATH path, KHE_TASK_BOUND tb,          */
/*    KHE_RESOURCE_GROUP rg)                                                 */
/*                                                                           */
/*  Add a record of a call to KheTaskBoundDelete to path.                    */
/*                                                                           */
/*****************************************************************************/

void KhePathOpTaskBoundDelete(KHE_PATH path, KHE_TASK_BOUND tb,
  KHE_RESOURCE_GROUP rg)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_TASK_BOUND_DELETE;
  op->u.task_bound_delete.task_bound = tb;
  op->u.task_bound_delete.resource_group = rg;
  KheTaskBoundReferenceCountIncrement(tb);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "operation loading - nodes"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpNodeSetBack(KHE_PATH path,                                 */
/*    KHE_NODE node, void *old_back, void *new_back)                         */
/*                                                                           */
/*  Add a record of a call to KheNodeSetBack to path.                        */
/*                                                                           */
/*****************************************************************************/

void KhePathOpNodeSetBack(KHE_PATH path,
  KHE_NODE node, void *old_back, void *new_back)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_NODE_SET_BACK;
  op->u.node_set_back.node = node;
  op->u.node_set_back.old_back = old_back;
  op->u.node_set_back.new_back = new_back;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpNodeAdd(KHE_PATH path, KHE_NODE node)                      */
/*                                                                           */
/*  Add a record of a call to KheNodeAdd to path.                            */
/*                                                                           */
/*****************************************************************************/

void KhePathOpNodeAdd(KHE_PATH path, KHE_NODE node)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_NODE_ADD;
  op->u.node_add.node = node;
  KheNodeReferenceCountIncrement(node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpNodeDelete(KHE_PATH path, KHE_NODE node)                   */
/*                                                                           */
/*  Add a record of a call to KheNodeDelete to path.                         */
/*                                                                           */
/*****************************************************************************/

void KhePathOpNodeDelete(KHE_PATH path, KHE_NODE node)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_NODE_DELETE;
  op->u.node_delete.node = node;
  KheNodeReferenceCountIncrement(node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpNodeAddParent(KHE_PATH path,                               */
/*    KHE_NODE child_node, KHE_NODE parent_node)                             */
/*                                                                           */
/*  Add a record of a call to KheNodeAddParent to path.                      */
/*                                                                           */
/*****************************************************************************/

void KhePathOpNodeAddParent(KHE_PATH path,
  KHE_NODE child_node, KHE_NODE parent_node)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_NODE_ADD_PARENT;
  op->u.node_add_parent.child_node = child_node;
  op->u.node_add_parent.parent_node = parent_node;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpNodeDeleteParent(KHE_PATH path,                            */
/*    KHE_NODE child_node, KHE_NODE parent_node)                             */
/*                                                                           */
/*  Add a record of a call to KheNodeDeleteParent to path.                   */
/*                                                                           */
/*****************************************************************************/

void KhePathOpNodeDeleteParent(KHE_PATH path,
  KHE_NODE child_node, KHE_NODE parent_node)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_NODE_DELETE_PARENT;
  op->u.node_delete_parent.child_node = child_node;
  op->u.node_delete_parent.parent_node = parent_node;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpNodeSwapChildNodesAndLayers(KHE_PATH path,                 */
/*    KHE_NODE node1, KHE_NODE node2)                                        */
/*                                                                           */
/*  Add a record of a call to KheNodeSwapChildNodesAndLayers to path.        */
/*                                                                           */
/*****************************************************************************/

void KhePathOpNodeSwapChildNodesAndLayers(KHE_PATH path,
  KHE_NODE node1, KHE_NODE node2)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_NODE_SWAP_CHILD_NODES_AND_LAYERS;
  op->u.node_swap_child_nodes_and_layers.node1 = node1;
  op->u.node_swap_child_nodes_and_layers.node2 = node2;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpNodeAddMeet(KHE_PATH path, KHE_NODE node, KHE_MEET meet)   */
/*                                                                           */
/*  Add a record of a call to KheNodeAddMeet to path.                        */
/*                                                                           */
/*****************************************************************************/

void KhePathOpNodeAddMeet(KHE_PATH path, KHE_NODE node, KHE_MEET meet)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_NODE_ADD_MEET;
  op->u.node_add_meet.node = node;
  op->u.node_add_meet.meet = meet;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpNodeDeleteMeet(KHE_PATH path, KHE_NODE node, KHE_MEET meet)*/
/*                                                                           */
/*  Add a record of a call to KheNodeDeleteMeet to path.                     */
/*                                                                           */
/*****************************************************************************/

void KhePathOpNodeDeleteMeet(KHE_PATH path, KHE_NODE node, KHE_MEET meet)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_NODE_DELETE_MEET;
  op->u.node_delete_meet.node = node;
  op->u.node_delete_meet.meet = meet;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "operation loading - layers"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpLayerSetBack(KHE_PATH path,                                */
/*    KHE_LAYER layer, void *old_back, void *new_back)                       */
/*                                                                           */
/*  Add a record of a call to KheLayerSetBack to path.                       */
/*                                                                           */
/*****************************************************************************/

void KhePathOpLayerSetBack(KHE_PATH path,
  KHE_LAYER layer, void *old_back, void *new_back)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_LAYER_SET_BACK;
  op->u.layer_set_back.layer = layer;
  op->u.layer_set_back.old_back = old_back;
  op->u.layer_set_back.new_back = new_back;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpLayerAdd(KHE_PATH path, KHE_LAYER layer,                   */
/*    KHE_NODE parent_node)                                                  */
/*                                                                           */
/*  Add a record of a call to KheLayerAdd to path.                           */
/*                                                                           */
/*****************************************************************************/

void KhePathOpLayerAdd(KHE_PATH path, KHE_LAYER layer, KHE_NODE parent_node)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_LAYER_ADD;
  op->u.layer_add.layer = layer;
  op->u.layer_add.parent_node = parent_node;
  KheLayerReferenceCountIncrement(layer);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpLayerDelete(KHE_PATH path, KHE_LAYER layer,                */
/*    KHE_NODE parent_node)                                                  */
/*                                                                           */
/*  Add a record of a call to KheLayerDelete to path.                        */
/*                                                                           */
/*****************************************************************************/

void KhePathOpLayerDelete(KHE_PATH path, KHE_LAYER layer, KHE_NODE parent_node)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_LAYER_DELETE;
  op->u.layer_delete.layer = layer;
  op->u.layer_delete.parent_node = parent_node;
  KheLayerReferenceCountIncrement(layer);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpLayerAddChildNode(KHE_PATH path, KHE_LAYER layer,          */
/*    KHE_RESOURCE child_node)                                               */
/*                                                                           */
/*  Add a record of a call to KheLayerAddChildNode to path.                  */
/*                                                                           */
/*****************************************************************************/

void KhePathOpLayerAddChildNode(KHE_PATH path, KHE_LAYER layer,
  KHE_NODE child_node)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_LAYER_ADD_CHILD_NODE;
  op->u.layer_add_child_node.layer = layer;
  op->u.layer_add_child_node.child_node = child_node;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpLayerDeleteChildNode(KHE_PATH path, KHE_LAYER layer,       */
/*    KHE_RESOURCE child_node)                                               */
/*                                                                           */
/*  Add a record of a call to KheLayerDeleteChildNode to path.               */
/*                                                                           */
/*****************************************************************************/

void KhePathOpLayerDeleteChildNode(KHE_PATH path, KHE_LAYER layer,
  KHE_NODE child_node)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_LAYER_DELETE_CHILD_NODE;
  op->u.layer_delete_child_node.layer = layer;
  op->u.layer_delete_child_node.child_node = child_node;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpLayerAddResource(KHE_PATH path, KHE_LAYER layer,           */
/*    KHE_RESOURCE resource)                                                 */
/*                                                                           */
/*  Add a record of a call to KheLayerAddResource to path.                   */
/*                                                                           */
/*****************************************************************************/

void KhePathOpLayerAddResource(KHE_PATH path, KHE_LAYER layer,
  KHE_RESOURCE resource)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_LAYER_ADD_RESOURCE;
  op->u.layer_add_resource.layer = layer;
  op->u.layer_add_resource.resource = resource;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpLayerDeleteResource(KHE_PATH path, KHE_LAYER layer,        */
/*    KHE_RESOURCE resource)                                                 */
/*                                                                           */
/*  Add a record of a call to KheLayerDeleteResource to path.                */
/*                                                                           */
/*****************************************************************************/

void KhePathOpLayerDeleteResource(KHE_PATH path, KHE_LAYER layer,
  KHE_RESOURCE resource)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_LAYER_DELETE_RESOURCE;
  op->u.layer_delete_resource.layer = layer;
  op->u.layer_delete_resource.resource = resource;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "operation loading - zones"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpZoneSetBack(KHE_PATH path,                                 */
/*    KHE_ZONE zone, void *old_back, void *new_back)                         */
/*                                                                           */
/*  Add a record of a call to KheZoneSetBack to path.                        */
/*                                                                           */
/*****************************************************************************/

void KhePathOpZoneSetBack(KHE_PATH path,
  KHE_ZONE zone, void *old_back, void *new_back)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_ZONE_SET_BACK;
  op->u.zone_set_back.zone = zone;
  op->u.zone_set_back.old_back = old_back;
  op->u.zone_set_back.new_back = new_back;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpZoneAdd(KHE_PATH path, KHE_ZONE zone, KHE_NODE node)       */
/*                                                                           */
/*  Add a record of a call to KheZoneAdd to path.                            */
/*                                                                           */
/*****************************************************************************/

void KhePathOpZoneAdd(KHE_PATH path, KHE_ZONE zone, KHE_NODE node)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_ZONE_ADD;
  op->u.zone_add.zone = zone;
  op->u.zone_add.node = node;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpZoneDelete(KHE_PATH path, KHE_ZONE zone, KHE_NODE node)    */
/*                                                                           */
/*  Add a record of a call to KheZoneDelete to path.                         */
/*                                                                           */
/*****************************************************************************/

void KhePathOpZoneDelete(KHE_PATH path, KHE_ZONE zone, KHE_NODE node)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_ZONE_DELETE;
  op->u.zone_delete.zone = zone;
  op->u.zone_delete.node = node;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpZoneAddMeetOffset(KHE_PATH path,                           */
/*    KHE_ZONE zone, KHE_MEET meet, int offset)                              */
/*                                                                           */
/*  Add a record of a call to KheZoneAddMeetOffset to path.                  */
/*                                                                           */
/*****************************************************************************/

void KhePathOpZoneAddMeetOffset(KHE_PATH path,
  KHE_ZONE zone, KHE_MEET meet, int offset)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_ZONE_ADD_MEET_OFFSET;
  op->u.zone_add_meet_offset.zone = zone;
  op->u.zone_add_meet_offset.meet = meet;
  op->u.zone_add_meet_offset.offset = offset;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpZoneDeleteMeetOffset(KHE_PATH path,                        */
/*    KHE_ZONE zone, KHE_MEET meet, int offset)                              */
/*                                                                           */
/*  Add a record of a call to KheZoneDeleteMeetOffset to path.               */
/*                                                                           */
/*****************************************************************************/

void KhePathOpZoneDeleteMeetOffset(KHE_PATH path,
  KHE_ZONE zone, KHE_MEET meet, int offset)
{
  KHE_PATH_OP *op;
  op = GetOp(path);
  op->type = KHE_PATH_OP_ZONE_DELETE_MEET_OFFSET;
  op->u.zone_delete_meet_offset.zone = zone;
  op->u.zone_delete_meet_offset.meet = meet;
  op->u.zone_delete_meet_offset.offset = offset;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KhePathOpDebug(KHE_PATH_OP op, int verbosity, FILE *fp)             */
/*                                                                           */
/*  Print one path op onto fp with the given verbosity and no extra space.   */
/*                                                                           */
/*****************************************************************************/

static void KhePathOpDebug(KHE_PATH_OP *op, int verbosity, FILE *fp)
{
  switch( op->type )
  {
    /* operations on meets */
    case KHE_PATH_OP_MEET_SET_BACK:

      fprintf(fp, "MeetSetBack");
      if( verbosity >= 2 )
      {
	fprintf(fp, "(");
	KheMeetDebug(op->u.meet_set_back.meet, 1, -1, fp);
	fprintf(fp, ", td)");
      }
      break;

    case KHE_PATH_OP_MEET_ADD:

      if( verbosity >= 2 )
      {
	KheMeetDebug(op->u.meet_add.meet, 1, -1, fp);
	fprintf(fp, " = ");
      }
      fprintf(fp, "KheMeetMake");
      if( verbosity >= 2 )
	fprintf(fp, "(-, -, -)");
      break;

    case KHE_PATH_OP_MEET_DELETE:

      fprintf(fp, "KheMeetDelete");
      if( verbosity >= 2 )
      {
	fprintf(fp, "(");
	KheMeetDebug(op->u.meet_delete.meet, 1, -1, fp);
	fprintf(fp, ")");
      }
      break;

    case KHE_PATH_OP_MEET_SPLIT:

      fprintf(fp, "MeetSplit");
      if( verbosity >= 2 )
      {
	fprintf(fp, "(-, -, ");
	KheMeetDebug(op->u.meet_split.meet1, 1, -1, fp);
	fprintf(fp, ", ");
	KheMeetDebug(op->u.meet_split.meet2, 1, -1, fp);
	fprintf(fp, ")");
      }
      break;

    case KHE_PATH_OP_MEET_MERGE:

      fprintf(fp, "MeetMerge");
      if( verbosity >= 2 )
	fprintf(fp, "(-, -, -)");
      break;

    case KHE_PATH_OP_MEET_MOVE:

      fprintf(fp, "MeetMove");
      if( verbosity >= 2 )
      {
	fprintf(fp, "(");
	KheMeetDebug(op->u.meet_move.meet, 1, -1, fp);
	fprintf(fp, ", tm, to)");
      }
      break;

    case KHE_PATH_OP_MEET_SET_AUTO_DOMAIN:

      fprintf(fp, "MeetSetAutoDomain");
      if( verbosity >= 2 )
      {
	fprintf(fp, "(");
	KheMeetDebug(op->u.meet_set_auto_domain.meet, 1, -1, fp);
	fprintf(fp, ", %s)", op->u.meet_set_auto_domain.automatic ?
	  "true" : "false");
      }
      break;

    case KHE_PATH_OP_MEET_ASSIGN_FIX:

      fprintf(fp, "MeetAssignFix");
      if( verbosity >= 2 )
      {
	fprintf(fp, "(");
	KheMeetDebug(op->u.meet_assign_fix.meet, 1, -1, fp);
	fprintf(fp, ")");
      }
      break;

    case KHE_PATH_OP_MEET_ASSIGN_UNFIX:

      fprintf(fp, "MeetAssignUnFix");
      if( verbosity >= 2 )
      {
	fprintf(fp, "(");
	KheMeetDebug(op->u.meet_assign_unfix.meet, 1, -1, fp);
	fprintf(fp, ")");
      }
      break;


    /* operations on meet bounds */
    case KHE_PATH_OP_MEET_BOUND_ADD:

      fprintf(fp, "MeetBoundMake");
      break;

    case KHE_PATH_OP_MEET_BOUND_DELETE:

      fprintf(fp, "MeetBoundDelete");
      break;


    /* operations on tasks */
    case KHE_PATH_OP_TASK_SET_BACK:

      fprintf(fp, "TaskSetBack");
      if( verbosity >= 2 )
      {
	fprintf(fp, "(");
	KheTaskDebug(op->u.task_set_back.task, 1, -1, fp);
	fprintf(fp, ")");
      }
      break;

    case KHE_PATH_OP_TASK_ADD:

      if( verbosity >= 2 )
      {
	KheTaskDebug(op->u.task_add.task, 1, -1, fp);
	fprintf(fp, " = ");
      }
      fprintf(fp, "KheTaskMake");
      if( verbosity >= 2 )
	fprintf(fp, "(-, -, -)");
      break;

    case KHE_PATH_OP_TASK_DELETE:

      fprintf(fp, "KheTaskDelete");
      if( verbosity >= 2 )
      {
	fprintf(fp, "(");
	KheTaskDebug(op->u.task_delete.task, 1, -1, fp);
	fprintf(fp, ")");
      }
      break;

    case KHE_PATH_OP_TASK_SPLIT:

      fprintf(fp, "TaskSplit");
      break;

    case KHE_PATH_OP_TASK_MERGE:

      fprintf(fp, "TaskMerge");
      break;

    case KHE_PATH_OP_TASK_MOVE:

      fprintf(fp, "TaskMove");
      if( verbosity >= 2 )
      {
	fprintf(fp, "(");
	KheTaskDebug(op->u.task_move.task, 1, -1, fp);
	fprintf(fp, ", ");
	KheTaskDebug(op->u.task_move.new_target_task, 1, -1, fp);
	fprintf(fp, ")");
      }
      break;

    case KHE_PATH_OP_TASK_ASSIGN_FIX:

      fprintf(fp, "TaskAssignFix");
      if( verbosity >= 2 )
      {
	fprintf(fp, "(");
	KheTaskDebug(op->u.task_assign_fix.task, 1, -1, fp);
	fprintf(fp, ")");
      }
      break;

    case KHE_PATH_OP_TASK_ASSIGN_UNFIX:

      fprintf(fp, "TaskAssignUnFix");
      if( verbosity >= 2 )
      {
	fprintf(fp, "(");
	KheTaskDebug(op->u.task_assign_unfix.task, 1, -1, fp);
	fprintf(fp, ")");
      }
      break;


    /* operations on task bounds */
    case KHE_PATH_OP_TASK_BOUND_ADD:

      fprintf(fp, "TaskBoundMake");
      break;

    case KHE_PATH_OP_TASK_BOUND_DELETE:

      fprintf(fp, "TaskBoundDelete");
      break;


    /* operations on nodes */
    case KHE_PATH_OP_NODE_SET_BACK:

      fprintf(fp, "NodeSetBack");
      break;

    case KHE_PATH_OP_NODE_ADD:

      fprintf(fp, "NodeAdd");
      break;

    case KHE_PATH_OP_NODE_DELETE:

      fprintf(fp, "NodeDelete");
      break;

    case KHE_PATH_OP_NODE_ADD_PARENT:

      fprintf(fp, "NodeAddParent");
      if( verbosity >= 2 )
      {
	fprintf(fp, "(");
	KheNodeDebug(op->u.node_add_parent.child_node, 1, -1, fp);
	fprintf(fp, ", ");
	KheNodeDebug(op->u.node_add_parent.parent_node, 1, -1, fp);
	fprintf(fp, ")");
      }
      break;

    case KHE_PATH_OP_NODE_DELETE_PARENT:

      fprintf(fp, "NodeDeleteParent");
      if( verbosity >= 2 )
      {
	fprintf(fp, "(");
	KheNodeDebug(op->u.node_add_parent.child_node, 1, -1, fp);
	fprintf(fp, ") was ");
	KheNodeDebug(op->u.node_add_parent.parent_node, 1, -1, fp);
      }
      break;

    case KHE_PATH_OP_NODE_SWAP_CHILD_NODES_AND_LAYERS:

      fprintf(fp, "NodeSwapChildNodesAndLayers");
      break;

    case KHE_PATH_OP_NODE_ADD_MEET:

      fprintf(fp, "NodeAddMeet");
      break;

    case KHE_PATH_OP_NODE_DELETE_MEET:

      fprintf(fp, "NodeDeleteMeet");
      break;


    /* operations on layers */
    case KHE_PATH_OP_LAYER_SET_BACK:

      fprintf(fp, "LayerSetBack");
      break;

    case KHE_PATH_OP_LAYER_ADD:

      fprintf(fp, "LayerAdd");
      break;

    case KHE_PATH_OP_LAYER_DELETE:

      fprintf(fp, "LayerDelete");
      break;

    case KHE_PATH_OP_LAYER_ADD_CHILD_NODE:

      fprintf(fp, "LayerAddChildNode");
      break;

    case KHE_PATH_OP_LAYER_DELETE_CHILD_NODE:

      fprintf(fp, "LayerDeleteChildNode");
      break;

    case KHE_PATH_OP_LAYER_ADD_RESOURCE:

      fprintf(fp, "LayerAddResource");
      break;

    case KHE_PATH_OP_LAYER_DELETE_RESOURCE:

      fprintf(fp, "LayerDeleteResource");
      break;

    /* operations on zones */
    case KHE_PATH_OP_ZONE_SET_BACK:

      fprintf(fp, "ZoneSetBack");
      break;

    case KHE_PATH_OP_ZONE_ADD:

      fprintf(fp, "ZoneAdd");
      break;

    case KHE_PATH_OP_ZONE_DELETE:

      fprintf(fp, "ZoneDelete");
      break;

    case KHE_PATH_OP_ZONE_ADD_MEET_OFFSET:

      fprintf(fp, "ZoneAddMeetOffset");
      break;

    case KHE_PATH_OP_ZONE_DELETE_MEET_OFFSET:

      fprintf(fp, "ZoneDeleteMeetOffset");
      break;

    default:

      HnAbort("KhePathOpDebug illegal op type");
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePathDebug(KHE_PATH path, int verbosity, int indent, FILE *fp)    */
/*                                                                           */
/*  Debug print of path onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

void KhePathDebug(KHE_PATH path, int verbosity, int indent, FILE *fp)
{
  KHE_PATH_OP *op;  int i;
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ Path");
    for( i = 0;  i < HaArrayCount(path->operations);  i++ )
    {
      op = &HaArray(path->operations, i);
      if( indent >= 0 )
	fprintf(fp, "\n%*s", indent + 2, "");
      else if( i > 0 )
	fprintf(fp, ", ");
      else
	fprintf(fp, " ");
      KhePathOpDebug(op, verbosity, fp);
    }
    if( indent >= 0 )
      fprintf(fp, "\n%*s]\n", indent, "");
    else
      fprintf(fp, " ]");
  }
}

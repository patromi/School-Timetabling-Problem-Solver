
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
/*  FILE:         khe_st_meet_set_repair.c                                   */
/*  DESCRIPTION:  Meet set time repair                                       */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"

#define MAX_NODE_COUNT 10000

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET_OBJ - an object representing one meet needing assignment        */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_meet_obj_rec {
  KHE_MEET			meet;		/* the meet represented      */
  KHE_MEET			best_target;	/* best asst, if any         */
  int				best_offset;	/* best asst offset, if any  */
} *KHE_MEET_OBJ;

typedef HA_ARRAY(KHE_MEET_OBJ) ARRAY_KHE_MEET_OBJ;


/*****************************************************************************/
/*                                                                           */
/*  KHE_TARGET_RANGE - a range of offsets in a target meet                   */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_target_range_rec {
  KHE_MEET			target_meet;	/* the target meet           */
  int				start_offset;	/* start offset (inclusive)  */
  int				stop_offset;	/* stop offset (exclusive)   */
} *KHE_TARGET_RANGE;

typedef HA_ARRAY(KHE_TARGET_RANGE) ARRAY_KHE_TARGET_RANGE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET_SET_SOLVER - solver object for this kind of repair              */
/*                                                                           */
/*****************************************************************************/

struct khe_meet_set_solver_rec {
  HA_ARENA			arena;		/* arena                     */
  KHE_SOLN			soln;		/* enclosing soln            */
  int				max_meets;	/* max meets accepted        */
  ARRAY_KHE_MEET_OBJ		meet_objs;	/* the meets to reassign     */
  ARRAY_KHE_TARGET_RANGE	target_ranges;	/* target ranges             */
  /* KHE_TRANSACTION		init_trans; */	/* init assignments          */
  int				node_count;	/* number of nodes           */
  int				leaf_count;	/* number of leaves          */
  int				init_tixels;	/* init unassigned tixels    */
  KHE_COST			init_cost;	/* init soln cost            */
  KHE_COST			best_cost;	/* best soln cost            */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "meet objects"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET_OBJ KheMeetObjMake(KHE_MEET meet)                               */
/*                                                                           */
/*  Make a new meet object representing meet.                                */
/*                                                                           */
/*****************************************************************************/

static KHE_MEET_OBJ KheMeetObjMake(KHE_MEET meet, HA_ARENA a)
{
  KHE_MEET_OBJ res;
  HaMake(res, a);
  res->meet = meet;
  res->best_target = NULL;
  res->best_offset = -1;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetObjDelete(KHE_MEET_OBJ mo)                                   */
/*                                                                           */
/*  Delete mo.                                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMeetObjDelete(KHE_MEET_OBJ mo)
{
  MFree(mo);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetObjDecreasingDurnCmp(const void *t1, const void *t2)          */
/*                                                                           */
/*  Comparison function for sorting meet objects by decreasing duration.     */
/*                                                                           */
/*****************************************************************************/

static int KheMeetObjDecreasingDurnCmp(const void *t1, const void *t2)
{
  KHE_MEET_OBJ mo1 = * (KHE_MEET_OBJ *) t1;
  KHE_MEET_OBJ mo2 = * (KHE_MEET_OBJ *) t2;
  int durn1 = KheMeetDuration(mo1->meet);
  int durn2 = KheMeetDuration(mo2->meet);
  if( durn1 != durn2 )
    return durn2 - durn1;
  else
    return KheMeetSolnIndex(mo1->meet) - KheMeetSolnIndex(mo2->meet);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "target ranges"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TARGET_RANGE KheTargetRangeMake(KHE_MEET target_meet,                */
/*    int start_offset, int stop_offset)                                     */
/*                                                                           */
/*  Make a new target range object with these attributes.                    */
/*                                                                           */
/*****************************************************************************/

static KHE_TARGET_RANGE KheTargetRangeMake(KHE_MEET target_meet,
  int start_offset, int stop_offset, HA_ARENA a)
{
  KHE_TARGET_RANGE res;
  HaMake(res, a);
  res->target_meet = target_meet;
  res->start_offset = start_offset;
  res->stop_offset = stop_offset;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTargetRangeDelete(KHE_TARGET_RANGE tr)                           */
/*                                                                           */
/*  Delete a target range object.                                            */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheTargetRangeDelete(KHE_TARGET_RANGE tr)
{
  MFree(tr);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheTargetRangeCmp(const void *t1, const void *t2)                    */
/*                                                                           */
/*  Comparison function for sorting target ranges by increasing target meet  */
/*  index then increasing start offset then increasing stop offset.          */
/*                                                                           */
/*****************************************************************************/

static int KheTargetRangeCmp(const void *t1, const void *t2)
{
  KHE_TARGET_RANGE tr1 = * (KHE_TARGET_RANGE *) t1;
  KHE_TARGET_RANGE tr2 = * (KHE_TARGET_RANGE *) t2;
  if( tr1->target_meet != tr2->target_meet )
    return KheMeetSolnIndex(tr1->target_meet) -
      KheMeetSolnIndex(tr2->target_meet);
  else if( tr1->start_offset != tr2->start_offset )
    return tr1->start_offset - tr2->start_offset;
  else
    return tr1->stop_offset - tr2->stop_offset;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTargetRangeMergeable(KHE_TARGET_RANGE tr1, KHE_TARGET_RANGE tr2, */
/*    int *new_stop_offset)                                                  */
/*                                                                           */
/*  Return true if tr1 and tr2 overlap or abut.  It is known that, if they   */
/*  have the same target meets, then tr1->start_offset <= tr2->start_offset  */
/*  so the start offset of the merged entity should be tr1->start_offset.    */
/*  If they are mergeable, *new_stop_offset is set to the stop offset.       */
/*                                                                           */
/*****************************************************************************/
#define max(a, b) ((a) > (b) ? (a) : (b))

static bool KheTargetRangeMergeable(KHE_TARGET_RANGE tr1, KHE_TARGET_RANGE tr2,
  int *new_stop_offset)
{
  if( tr1->target_meet != tr2->target_meet )
    return false;
  HnAssert(tr1->start_offset <= tr2->start_offset,
    "KheTargetRangeMergeable internal error");
  if( tr2->start_offset <= tr1->stop_offset )
  {
    *new_stop_offset = max(tr1->stop_offset, tr2->stop_offset);
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTargetRangeGroup(ARRAY_KHE_TARGET_RANGE *target_ranges)          */
/*                                                                           */
/*  Group target ranges so that there are no overlapping or abutting ones.   */
/*                                                                           */
/*****************************************************************************/

static void KheTargetRangeGroup(ARRAY_KHE_TARGET_RANGE *target_ranges)
{
  int i, new_stop_offset;  KHE_TARGET_RANGE tr1, tr2;

  /* sort the ranges */
  HaArraySort(*target_ranges, &KheTargetRangeCmp);

  /* merge mergeable ranges and remove the resulting gaps in *target_ranges */
  HaArrayForEach(*target_ranges, tr1, i)
  {
    while( i + 1 < HaArrayCount(*target_ranges) )
    {
      tr2 = HaArray(*target_ranges, i + 1);
      if( !KheTargetRangeMergeable(tr1, tr2, &new_stop_offset) )
	break;
      tr1->stop_offset = new_stop_offset;
      /* KheTargetRangeDelete(tr2); */
      HaArrayDeleteAndShift(*target_ranges, i + 1);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main functions"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET_SET_SOLVER KheMeetSetSolveBegin(KHE_SOLN soln, int max_meets)   */
/*                                                                           */
/*  Create and return a new meet set solver object.                          */
/*                                                                           */
/*****************************************************************************/

KHE_MEET_SET_SOLVER KheMeetSetSolveBegin(KHE_SOLN soln, int max_meets)
{
  KHE_MEET_SET_SOLVER res;  HA_ARENA a;
  a = KheSolnArenaBegin(soln);
  /* a = HaAren aMake(); */
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->max_meets = max_meets;
  HaArrayInit(res->meet_objs, a);
  HaArrayInit(res->target_ranges, a);
  /* res->init_trans = KheTransactionMake(soln); */
  res->node_count = 0;
  res->leaf_count = 0;
  res->init_tixels = 0;  /* placeholder */
  res->init_cost = 0;  /* placeholder */
  res->best_cost = 0;  /* placeholder */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetSolverDelete(KHE_MEET_SET_SOLVER mss)                     */
/*                                                                           */
/*  Delete mss.                                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMeetSetSolverDelete(KHE_MEET_SET_SOLVER mss)
{
  HaArena Delete(mss->arena);
  ** ***
  while( HaArrayCount(mss->meet_objs) > 0 )
    KheMeetObjDelete(MArrayRemoveLast(mss->meet_objs));
  MArrayFree(mss->meet_objs);
  while( HaArrayCount(mss->target_ranges) > 0 )
    KheTargetRangeDelete(MArrayRemoveLast(mss->target_ranges));
  MArrayFree(mss->target_ranges);
  ** KheTransactionDelete(mss->init_trans); **
  MFree(mss);
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetSolveAddMeet(KHE_MEET_SET_SOLVER mss, KHE_MEET meet)      */
/*                                                                           */
/*  Add meet to mss, unless the maximum number of meets has been reached.    */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetSolveAddMeet(KHE_MEET_SET_SOLVER mss, KHE_MEET meet)
{
  int i;  KHE_MEET_OBJ mo;

  /* ignore if already have enough meets */
  if( HaArrayCount(mss->meet_objs) >= mss->max_meets )
    return;

  /* ignore if already inserted */
  HaArrayForEach(mss->meet_objs, mo, i)
    if( mo->meet == meet )
      return;

  /* add the meet */
  HaArrayAddLast(mss->meet_objs, KheMeetObjMake(meet, mss->arena));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetSetMeetAssign(KHE_MEET_SET_SOLVER mss,                       */
/*    KHE_MEET meet, KHE_MEET target_meet, int offset)                       */
/*                                                                           */
/*  If meet can be assigned to target_meet at offset without increasing      */
/*  the number of unmatched tixels, make the assignment, otherwise don't.    */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetSetMeetAssign(KHE_MEET_SET_SOLVER mss,
  KHE_MEET meet, KHE_MEET target_meet, int offset)
{
  /* fail if can't assign */
  if( !KheMeetAssign(meet, target_meet, offset) )
    return false;

  /* fail if tixels increase */
  if( KheSolnMatchingDefectCount(mss->soln) > mss->init_tixels )
  {
    KheMeetUnAssign(meet);   /* OK, unassigns previous asst */
    return false;
  }

  /* otherwise all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetDoSolve(KHE_MEET_SET_SOLVER mss, int pos, int *end_index) */
/*                                                                           */
/*  Solve mss, starting at pos.  Set *end_index to the last index used.      */
/*                                                                           */
/*****************************************************************************/

static void KheMeetSetDoSolve(KHE_MEET_SET_SOLVER mss, int pos, int *end_index)
{
  KHE_MEET_OBJ mo;  int i, offs, durn, junk;  KHE_TARGET_RANGE tr;
  mss->node_count++;
  if( pos >= HaArrayCount(mss->meet_objs) )
  {
    /* at leaf, test for better */
    mss->leaf_count++;
    if( KheSolnCost(mss->soln) < mss->best_cost )
    {
      /* new best, save assignments and cost */
      HaArrayForEach(mss->meet_objs, mo, i)
      {
	mo->best_target = KheMeetAsst(mo->meet);
	mo->best_offset = KheMeetAsstOffset(mo->meet);
      }
      mss->best_cost = KheSolnCost(mss->soln);
      if( DEBUG2 )
	fprintf(stderr, "  KheMeetSetDoSolve new best: %.5f\n",
	  KheCostShow(mss->best_cost));
    }
  }
  else
  {
    /* try all assignments at pos, or just the first successful if limit */
    mo = HaArray(mss->meet_objs, pos);
    durn = KheMeetDuration(mo->meet);
    HaArrayForEach(mss->target_ranges, tr, i)
      for( offs = tr->start_offset;  offs + durn <= tr->stop_offset;  offs++ )
      {
        if( KheMeetSetMeetAssign(mss, mo->meet, tr->target_meet, offs) )
	{
          KheMeetSetDoSolve(mss, pos + 1, &junk);
	  KheMeetUnAssign(mo->meet);   /* OK, unassigns previous asst */
	  if( mss->node_count > MAX_NODE_COUNT )
	    goto TRUNCATE;
	}
      }
    TRUNCATE:
    *end_index = i;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetSolveDebug(KHE_MEET_SET_SOLVER mss, int verbosity,        */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of mss onfo fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheMeetSetSolveDebug(KHE_MEET_SET_SOLVER mss, int verbosity,
  int indent, FILE *fp)
{
  KHE_MEET_OBJ mo;  KHE_TARGET_RANGE tr;  int i;
  fprintf(fp, "%*s[ MeetSetSolver(max_meets %d)\n", indent, "", mss->max_meets);

  /* print meet objects */
  fprintf(fp, "%*s  %d meets: ", indent, "", HaArrayCount(mss->meet_objs));
  HaArrayForEach(mss->meet_objs, mo, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    KheMeetDebug(mo->meet, 1, -1, stderr);
  }
  fprintf(fp, "\n");

  /* print target ranges */
  fprintf(fp, "%*s  %d ranges: ", indent, "", HaArrayCount(mss->target_ranges));
  HaArrayForEach(mss->target_ranges, tr, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    KheMeetDebug(tr->target_meet, 1, -1, stderr);
    fprintf(fp, "::%d-%d", tr->start_offset, tr->stop_offset);
  }
  fprintf(fp, "\n");
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetSetSolveEnd(KHE_MEET_SET_SOLVER mss)                         */
/*                                                                           */
/*  Carry out the actual solve, returning true if the cost of the solution   */
/*  is reduced.  Also free the solver object.                                */
/*                                                                           */
/*****************************************************************************/

bool KheMeetSetSolveEnd(KHE_MEET_SET_SOLVER mss)
{
  KHE_MEET meet, target_meet;  int i, end_index;  KHE_TARGET_RANGE tr;
  KHE_MEET_OBJ mo;  bool success;  KHE_MARK mark;

  if( DEBUG1 )
    fprintf(stderr, "[ KheMeetSetSolveEnd(mss):\n");

  /* find and group target ranges */
  HaArrayForEach(mss->meet_objs, mo, i)
  {
    meet = mo->meet;
    target_meet = KheMeetAsst(meet);
    if( target_meet != NULL )
    {
      tr = KheTargetRangeMake(target_meet, KheMeetAsstOffset(meet),
	KheMeetAsstOffset(meet) + KheMeetDuration(meet), mss->arena);
      HaArrayAddLast(mss->target_ranges, tr);
    }
  }
  KheTargetRangeGroup(&mss->target_ranges);

  /* sort the meet objects by decreasing duration */
  HaArraySort(mss->meet_objs, &KheMeetObjDecreasingDurnCmp);
  if( DEBUG1 )
    KheMeetSetSolveDebug(mss, 1, 2, stderr);

  /* unassign the meets and try the tree search */
  mss->init_tixels = KheSolnMatchingDefectCount(mss->soln);
  mss->init_cost = mss->best_cost = KheSolnCost(mss->soln);
  mark = KheMarkBegin(mss->soln);
  HaArrayForEach(mss->meet_objs, mo, i)
    if( KheMeetAsst(mo->meet) != NULL )
      KheMeetUnAssign(mo->meet);  /* not sure; used by fuzzy meet moves */
  KheMeetSetDoSolve(mss, 0, &end_index);
  success = mss->best_cost < mss->init_cost;
  KheMarkEnd(mark, !success);

  if( success )
  {
    /* new best, redo it now */
    HaArrayForEach(mss->meet_objs, mo, i)
      if( !KheMeetAssign(mo->meet, mo->best_target, mo->best_offset) )
	HnAbort("KheMeetSetSolve internal error 1");
    HnAssert(KheSolnCost(mss->soln) == mss->best_cost,
      "KheMeetSetSolve internal error 2");
  }
  else
  {
    /* no new best, already returned to initial assignments */
  }

  /* delete the solver and return */
  HnAssert(KheSolnCost(mss->soln) <= mss->init_cost,
    "KheMeetSetSolve internal error 3");
  if( DEBUG1 )
  {
    fprintf(stderr, "  node_count %d, leaf_count %d\n",
      mss->node_count, mss->leaf_count);
    if( success )
      fprintf(stderr, "] KheMeetSetSolve returning true (%.5f -> %.5f)\n",
	KheCostShow(mss->init_cost), KheCostShow(mss->best_cost));
    else
      fprintf(stderr, "] KheMeetSetSolve returning false\n");
  }
  KheSolnArenaEnd(mss->soln, mss->arena);
  /* KheMeetSetSolverDelete(mss); */
  return success;
}


/* *** old version, uses transactions
bool KheMeet SetSolveEnd(KHE_MEET_SET_SOLVER mss)
{
  KHE_MEET meet, target_meet;  int i, end_index;  KHE_TARGET_RANGE tr;
  KHE_MEET_OBJ mo;  bool res;

  if( DEBUG1 )
    fprintf(stderr, "[ KheMeetS etSolveEnd(mss):\n");

  ** find and group target ranges **
  HaArrayForEach(mss->meet_objs, mo, i)
  {
    meet = mo->meet;
    target_meet = KheMeetAsst(meet);
    if( target_meet != NULL )
    {
      tr = KheTargetRangeMake(target_meet, KheMeetAsstOffset(meet),
	KheMeetAsstOffset(meet) + KheMeetDuration(meet));
      HaArrayAddLast(mss->target_ranges, tr);
    }
  }
  KheTargetRangeGroup(&mss->target_ranges);

  ** sort the meet objects by decreasing duration **
  HaArraySort(mss->meet_objs, &KheMeetObjDecreasingDurnCmp);
  if( DEBUG1 )
    KheMeetSetSolveDebug(mss, 1, 2, stderr);

  ** unassign the meets, but record where they were in init_trans **
  mss->init_tixels = KheSolnMatchingDefectCount(mss->soln);
  mss->init_cost = mss->best_cost = KheSolnCost(mss->soln);
  KheTransactionBegin(mss->init_trans);
  HaArrayForEach(mss->meet_objs, mo, i)
    if( KheMeetAsst(mo->meet) != NULL )
      KheMeetUn Assign(mo->meet);
  KheTransactionEnd(mss->init_trans);

  ** carry out the tree search and finalize the assignment, new or old **
  KheMeetSetDoSolve(mss, 0, &end_index);
  if( mss->best_cost < mss->init_cost )
  {
    ** new best, redo it now **
    HaArrayForEach(mss->meet_objs, mo, i)
      if( !KheMeetAssign(mo->meet, mo->best_target, mo->best_offset) )
	HnAbort("KheMeetSetSolve internal error 1");
    HnAssert(KheSolnCost(mss->soln) == mss->best_cost,
      "KheMeetSetSolve internal error 2");
    res = true;
  }
  else
  {
    ** no new best, return to initial assignments **
    KheTransactionUndo(mss->init_trans);
    res = false;
  }

  ** delete the solver and return **
  HnAssert(KheSolnCost(mss->soln) <= mss->init_cost,
    "KheMeetSetSolve internal error 3");
  if( DEBUG1 )
  {
    fprintf(stderr, "  node_count %d, leaf_count %d\n",
      mss->node_count, mss->leaf_count);
    if( res )
      fprintf(stderr, "] KheMeetSetSolve returning true (%.5f -> %.5f)\n",
	KheCostShow(mss->init_cost), KheCostShow(mss->best_cost));
    else
      fprintf(stderr, "] KheMeetSetSolve returning false\n");
  }
  KheMeetSetSolverDelete(mss);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "fuzzy meet move"                                              */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_MEET) ARRAY_KHE_MEET;
typedef HA_ARRAY(KHE_RESOURCE) ARRAY_KHE_RESOURCE;

/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAddResources(KHE_MEET meet, ARRAY_KHE_RESOURCE *resources)   */
/*                                                                           */
/*  Add to resources the resources assigned to the tasks of meet, and to     */
/*  the tasks of meets assigned to meet, directly or indirectly; but only    */
/*  add resources that are not already present.                              */
/*                                                                           */
/*****************************************************************************/

static void KheMeetAddResources(KHE_MEET meet, ARRAY_KHE_RESOURCE *resources)
{
  int i, pos;  KHE_TASK task;  KHE_MEET child_meet;  KHE_RESOURCE r;

  /* add the resources of meet's tasks */
  for( i = 0;  i < KheMeetTaskCount(meet);  i++ )
  {
    task = KheMeetTask(meet, i);
    r = KheTaskAsstResource(task);
    if( r != NULL && !HaArrayContains(*resources, r, &pos) )
      HaArrayAddLast(*resources, r);
  }

  /* add the resources of meet's proper descendants' tasks */
  for( i = 0;  i < KheMeetAssignedToCount(meet);  i++ )
  {
    child_meet = KheMeetAssignedTo(meet, i);
    KheMeetAddResources(child_meet, resources);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetWithinWidth(KHE_MEET meet, KHE_MEET target_meet,             */
/*    int offset, int width)                                                 */
/*                                                                           */
/*  Return true if meet is assigned to target_meet within width of offset.   */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetWithinWidth(KHE_MEET meet, KHE_MEET target_meet,
  int offset, int width)
{
  return KheMeetAsst(meet) == target_meet &&
    KheMeetAsstOffset(meet) >= offset - width &&
    KheMeetAsstOffset(meet) <= offset + width;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceAddMeets(KHE_RESOURCE r, KHE_MEET target_meet1,          */
/*    int offset1, KHE_MEET target_meet2, int offset2, int width,            */
/*    ARRAY_KHE_MEET *meets)                                                 */
/*                                                                           */
/*  Unless already present, add to *meets those meets containing r which     */
/*  are assigned to target_meet1 within width of offset1, or assigned to     */
/*  target_meet2 within width of offset2.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheResourceAddMeets(KHE_RESOURCE r, KHE_MEET target_meet1,
  int offset1, KHE_MEET target_meet2, int offset2, int width,
  ARRAY_KHE_MEET *meets)
{
  int i, pos;  KHE_TASK task;  KHE_SOLN soln;  KHE_MEET m;
  soln = KheMeetSoln(target_meet1);
  for( i = 0;  i < KheResourceAssignedTaskCount(soln, r);  i++ )
  {
    task = KheResourceAssignedTask(soln, r, i);
    for( m = KheTaskMeet(task);  KheMeetAsst(m) != NULL;  m = KheMeetAsst(m) )
    {
      if( KheMeetWithinWidth(m, target_meet1, offset1, width) ||
          KheMeetWithinWidth(m, target_meet2, offset2, width) )
      {
	if( !HaArrayContains(*meets, m, &pos) )
	  HaArrayAddLast(*meets, m);
	break;
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFuzzyMeetDepthDebug(int depth,                                   */
/*    ARRAY_KHE_MEET *meets, int start_mindex, int stop_mindex,              */
/*    ARRAY_KHE_RESOURCE *resources, int start_rindex, int stop_rindex)      */
/*                                                                           */
/*  Debug print of meets and resources between start and stop-1.             */
/*                                                                           */
/*****************************************************************************/

static void KheFuzzyMeetDepthDebug(int depth,
  ARRAY_KHE_MEET *meets, int start_mindex, int stop_mindex,
  ARRAY_KHE_RESOURCE *resources, int start_rindex, int stop_rindex)
{
  int j;  KHE_RESOURCE r;  KHE_MEET meet;
  fprintf(stderr, "  depth %d meets [%d-%d]: ", depth,
    start_mindex, stop_mindex - 1);
  for( j = start_mindex;  j < stop_mindex;  j++ )
  {
    if( j > start_mindex )
      fprintf(stderr, ", ");
    meet = HaArray(*meets, j);
    KheMeetDebug(meet, 1, -1, stderr);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "  depth %d resources [%d-%d]: ", depth,
    start_rindex, stop_rindex - 1);
  for( j = start_rindex;  j < stop_rindex;  j++ )
  {
    if( j > start_rindex )
      fprintf(stderr, ", ");
    r = HaArray(*resources, j);
    fprintf(stderr, "%s", KheResourceId(r));
  }
  fprintf(stderr, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFuzzyMeetMove(KHE_MEET meet, KHE_MEET target_meet, int offset,   */
/*    int width, int depth, int max_meets)                                   */
/*                                                                           */
/*  Carry out a fuzzy meet move of meet to target_meet at offset, limited    */
/*  by width, depth, and max_meets.  Return true if the solution changed.    */
/*                                                                           */
/*****************************************************************************/

bool KheFuzzyMeetMove(KHE_MEET meet, KHE_MEET target_meet, int offset,
  int width, int depth, int max_meets)
{
  ARRAY_KHE_MEET meets;  ARRAY_KHE_RESOURCE resources;  KHE_RESOURCE r;
  KHE_MEET_SET_SOLVER mss;  KHE_MEET meet2;  bool res;  HA_ARENA a;
  int i, j, start_rindex, stop_rindex, start_mindex, stop_mindex;
  KHE_SOLN soln;

  if( DEBUG3 )
  {
    fprintf(stderr, "[ KheFuzzyMeetMove(");
    KheMeetDebug(meet, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheMeetDebug(target_meet, 1, -1, stderr);
    fprintf(stderr, ", %d, w%d, d%d, m%d)\n", offset, width, depth, max_meets);
  }

  /* get the meets and resources within the given limits */
  soln = KheMeetSoln(meet);
  a = KheSolnArenaBegin(soln);
  /* a = HaArena Make(); */
  HnAssert(KheMeetAsst(meet) != NULL, "KheFuzzyMeetMove: meet is not assigned");
  HaArrayInit(meets, a);
  HaArrayInit(resources, a);
  HaArrayAddLast(meets, meet);
  KheMeetAddResources(meet, &resources);
  start_mindex = 0;
  start_rindex = 0;
  stop_mindex = HaArrayCount(meets);
  stop_rindex = HaArrayCount(resources);
  for( i = 0;  i < depth && HaArrayCount(meets) < max_meets;  i++ )
  {
    /* invariant:  meets and resources are correct here for depth i */
    if( DEBUG1 )
      KheFuzzyMeetDepthDebug(i, &meets, start_mindex, stop_mindex,
	&resources, start_rindex, stop_rindex);

    /* get the meets of depth i + 1 */
    for( j = start_rindex;  j < stop_rindex;  j++ )
    {
      r = HaArray(resources, j);
      KheResourceAddMeets(r, KheMeetAsst(meet), KheMeetAsstOffset(meet),
	target_meet, offset, width, &meets);
    }

    /* get the resources of depth i + 1 */
    for( j = start_mindex;  j < stop_mindex;  j++ )
    {
      meet2 = HaArray(meets, j);
      KheMeetAddResources(meet2, &resources);
    }

    /* prepare for next depth */
    start_mindex = stop_mindex;
    start_rindex = stop_rindex;
    stop_mindex = HaArrayCount(meets);
    stop_rindex = HaArrayCount(resources);
  }
  if( DEBUG1 )
    KheFuzzyMeetDepthDebug(i, &meets, start_mindex, stop_mindex,
      &resources, start_rindex, stop_rindex);

  /* call the meet set solver */
  mss = KheMeetSetSolveBegin(KheMeetSoln(meet), max_meets);
  HaArrayForEach(meets, meet2, i)
    KheMeetSetSolveAddMeet(mss, meet2);
  res = KheMeetSetSolveEnd(mss);
  KheSolnArenaEnd(soln, a);
  /* HaAre naDelete(a); */
  /* MArrayFree(meets); */
  /* MArrayFree(resources); */
  if( DEBUG3 )
    fprintf(stderr, "] KheFuzzyMeetMove returning %s\n",res ? "true" : "false");
  return res;
}

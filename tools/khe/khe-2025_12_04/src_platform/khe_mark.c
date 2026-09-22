
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
/*  FILE:         khe_mark.c                                                 */
/*  DESCRIPTION:  Marks                                                      */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_MARK - a mark                                                        */
/*                                                                           */
/*****************************************************************************/

struct khe_mark_rec {
  KHE_SOLN			soln;			/* encl soln         */
  int				soln_index;		/* index in soln     */
  int				start_pos;		/* start in mainpath */
  KHE_COST			soln_cost;		/* cost when created */
  ARRAY_KHE_PATH		paths;			/* paths             */
};


/*****************************************************************************/
/*                                                                           */
/*  void KheMarkFree(KHE_MARK mark)                                          */
/*                                                                           */
/*  Free mark.  This really means to free it, not to add it to a free list.  */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheMarkFree(KHE_MARK mark)
{
  while( HaArrayCount(mark->paths) > 0 )
    KhePathFree(MArrayRemoveLast(mark->paths));
  MFree(mark);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MARK KheMarkBegin(KHE_SOLN soln)                                     */
/*                                                                           */
/*  Begin a new mark for soln.                                               */
/*                                                                           */
/*****************************************************************************/

KHE_MARK KheMarkBegin(KHE_SOLN soln)
{
  KHE_MARK res;  HA_ARENA a;

  /* get a mark object with its path array initialized */
  res = KheSolnTakeMarkFromFreeList(soln);
  if( res == NULL )
  {
    a = KheSolnArena(soln);
    HaMake(res, a);
    HaArrayInit(res->paths, a);
  }

  /* initialize it, and inform the solution that it's beginning */
  res->soln = soln;
  res->soln_cost = KheSolnCost(soln);
  HaArrayClear(res->paths);
  KheSolnMarkBegin(soln, res, &res->soln_index, &res->start_pos);
  KheSolnMatchingMarkBegin(soln);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMarkEnd(KHE_MARK mark, bool undo)                                */
/*                                                                           */
/*  End a mark.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheMarkEnd(KHE_MARK mark, bool undo)
{
  KHE_PATH main_path;

  /* remove all the paths */
  while( HaArrayCount(mark->paths) > 0 )
    KhePathDelete(HaArrayLast(mark->paths));

  /* undo if required */
  if( undo )
    KheMarkUndo(mark);

  /* inform soln that the mark is ending, and add it to the free list */
  main_path = KheSolnMainPath(mark->soln);
  KheSolnMatchingMarkEnd(mark->soln, KhePathCount(main_path)==mark->start_pos);
  KheSolnMarkEnd(mark->soln, mark);
  KheSolnAddMarkToFreeList(mark->soln, mark);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMarkUndo(KHE_MARK mark)                                          */
/*                                                                           */
/*  Undo back to mark.                                                       */
/*                                                                           */
/*****************************************************************************/

void KheMarkUndo(KHE_MARK mark)
{
  HnAssert(KheSolnMarkOnTop(mark->soln, mark), "KheMarkUndo: wrong mark");
  KhePathUndo(KheSolnMainPath(mark->soln), mark->start_pos);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheMarkSoln(KHE_MARK mark)                                      */
/*                                                                           */
/*  Return mark's soln.                                                      */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheMarkSoln(KHE_MARK mark)
{
  return mark->soln;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheMarkSolnCost(KHE_MARK mark)                                  */
/*                                                                           */
/*  Return the cost of mark's solution when mark was created.                */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheMarkSolnCost(KHE_MARK mark)
{
  return mark->soln_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PATH KheMarkAddPath(KHE_MARK mark)                                   */
/*                                                                           */
/*  Build a path and add it to mark.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_PATH KheMarkAddPath(KHE_MARK mark)
{
  KHE_PATH res;
  res = KhePathCopy(KheSolnMainPath(mark->soln), mark->start_pos, mark);
  HaArrayAddLast(mark->paths, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PATH KheMarkAddBestPath(KHE_MARK mark, int k)                        */
/*                                                                           */
/*  Helper function for adding the best k paths to mark.                     */
/*                                                                           */
/*****************************************************************************/

KHE_PATH KheMarkAddBestPath(KHE_MARK mark, int k)
{
  KHE_COST soln_cost;  KHE_PATH res, worst_path;
  res = NULL;
  if( KheMarkPathCount(mark) >= k )
  {
    /* keep only the best k paths, in increasing cost order */
    KheMarkPathSort(mark, &KhePathIncreasingSolnCostCmp);
    while( KheMarkPathCount(mark) > k )
    {
      worst_path = KheMarkPath(mark, KheMarkPathCount(mark) - 1);
      KhePathDelete(worst_path);
    }

    /* if a new path would be better than the worst remaining path, */
    /* replace the worst remaining path with a new path and re-sort */
    worst_path = KheMarkPath(mark, k - 1);
    soln_cost = KheSolnCost(KheMarkSoln(mark));
    if( soln_cost < KhePathSolnCost(worst_path) )
    {
      KhePathDelete(worst_path);
      res = KheMarkAddPath(mark);
      KheMarkPathSort(mark, &KhePathIncreasingSolnCostCmp);
    }
  }
  else
  {
    /* there is room for a new path with no deletions, so just */
    /* add the new path and re-sort */
    res = KheMarkAddPath(mark);
    KheMarkPathSort(mark, &KhePathIncreasingSolnCostCmp);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMarkDeletePath(KHE_MARK mark, KHE_PATH path)                     */
/*                                                                           */
/*  Delete path from mark.                                                   */
/*                                                                           */
/*****************************************************************************/

void KheMarkDeletePath(KHE_MARK mark, KHE_PATH path)
{
  int pos;
  if( !HaArrayContains(mark->paths, path, &pos) )
    HnAbort("KheMarkDeletePath internal error");
  HaArrayDeleteAndShift(mark->paths, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMarkPathCount(KHE_MARK mark)                                      */
/*                                                                           */
/*  Return the number of paths in mark.                                      */
/*                                                                           */
/*****************************************************************************/

int KheMarkPathCount(KHE_MARK mark)
{
  return HaArrayCount(mark->paths);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PATH KheMarkPath(KHE_MARK mark, int i)                               */
/*                                                                           */
/*  Return the ith path of mark.                                             */
/*                                                                           */
/*****************************************************************************/

KHE_PATH KheMarkPath(KHE_MARK mark, int i)
{
  return HaArray(mark->paths, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMarkPathSort(KHE_MARK mark,                                      */
/*    int(*compar)(const void *, const void *))                              */
/*                                                                           */
/*  Sort the paths of mark.                                                  */
/*                                                                           */
/*****************************************************************************/

void KheMarkPathSort(KHE_MARK mark,
  int(*compar)(const void *, const void *))
{
  if( HaArrayCount(mark->paths) > 1 )
    HaArraySort(mark->paths, compar);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMarkIsCurrent(KHE_MARK mark)                                     */
/*                                                                           */
/*  Return true if the solution is in mark's state.                          */
/*                                                                           */
/*****************************************************************************/

bool KheMarkIsCurrent(KHE_MARK mark)
{
  return KhePathCount(KheSolnMainPath(mark->soln)) == mark->start_pos;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMarkDebug(KHE_MARK mark, int verbosity, int indent, FILE *fp)    */
/*                                                                           */
/*  Debug print of mark onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

void KheMarkDebug(KHE_MARK mark, int verbosity, int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ Mark %d (start_pos %d, cost %.5f, %d paths) ]",
      mark->soln_index, mark->start_pos, KheCostShow(mark->soln_cost),
      HaArrayCount(mark->paths));
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


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
/*  FILE:         khe_ss_misc.c                                              */
/*  DESCRIPTION:  Miscellaneous structural solvers.                          */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0

typedef HA_ARRAY(KHE_MEET) ARRAY_KHE_MEET;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "meet merging"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheMeetIncreasingTimeAsstCmp(const void *p1, const void *p2)         */
/*                                                                           */
/*  Comparison function for sorting an array of meets with assigned times    */
/*  by increasing chronological order.                                       */
/*                                                                           */
/*****************************************************************************/

static int KheMeetIncreasingTimeAsstCmp(const void *p1, const void *p2)
{
  KHE_MEET meet1 = * (KHE_MEET *) p1;
  KHE_MEET meet2 = * (KHE_MEET *) p2;
  return KheTimeIndex(KheMeetAsstTime(meet1)) -
    KheTimeIndex(KheMeetAsstTime(meet2));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetsMerged(KHE_MEET meet1, KHE_MEET meet2,                      */
/*    KHE_MEET *rmeet1, KHE_MEET *rmeet2)                                    */
/*                                                                           */
/*  If meet1 and meet2 can be merged and doing so reduces the cost of the    */
/*  solution, then do the merge and return true.  Otherwise don't, but       */
/*  return replacement meets for meet1 and meet2 in *rmeet1 and *rmeet2.     */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetsMerged(KHE_MEET meet1, KHE_MEET meet2,
  KHE_MEET *rmeet1, KHE_MEET *rmeet2)
{
  int duration1;  KHE_COST cost_before;  KHE_SOLN soln;
  if( DEBUG4 )
  {
    fprintf(stderr, "  [ KheMeetsMerged(");
    KheMeetDebug(meet1, 1, -1, stderr);
    fprintf(stderr, " -> %s, ", KheTimeId(KheMeetAsstTime(meet1)));
    KheMeetDebug(meet2, 1, -1, stderr);
    fprintf(stderr, " -> %s)\n", KheTimeId(KheMeetAsstTime(meet2)));
  }
  duration1 = KheMeetDuration(meet1);
  soln = KheMeetSoln(meet1);
  cost_before = KheSolnCost(soln);
  if( !KheMeetMerge(meet1, meet2, true, rmeet1) )
  {
    /* merge failed, nothing has changed */
    *rmeet1 = meet1;
    *rmeet2 = meet2;
    if( DEBUG4 )
      fprintf(stderr, "  ] KheMeetsMerged returning false (no merge)\n");
    return false;
  }
  else if( KheSolnCost(soln) >= cost_before )
  {
    /* merge succeeded but cost failed, so undo the merge */
    if( !KheMeetSplit(*rmeet1, duration1, true, rmeet1, rmeet2) )
      HnAbort("KheMeetsMerged internal error");
    if( DEBUG4 )
      fprintf(stderr, "  ] KheMeetsMerged returning false (%.5f -> %.5f)\n",
	KheCostShow(cost_before), KheCostShow(KheSolnCost(soln)));
    return false;
  }
  else
  {
    /* success, return true (*rmeet2 is undefined) */
    if( DEBUG4 )
      fprintf(stderr, "  ] KheMeetsMerged returning true (%.5f -> %.5f)\n",
	KheCostShow(cost_before), KheCostShow(KheSolnCost(soln)));
    *rmeet2 = NULL;
    return true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFindAssignedMeets(KHE_EVENT e, KHE_SOLN soln,                    */
/*    ARRAY_KHE_MEET *meets)                                                 */
/*                                                                           */
/*  Set *meets to those meets of e in soln assigned a time, sorted by        */
/*  increasing starting time.                                                */
/*                                                                           */
/*****************************************************************************/

static void KheFindAssignedMeets(KHE_EVENT e, KHE_SOLN soln,
  ARRAY_KHE_MEET *meets)
{
  KHE_MEET meet;  int i;
  HaArrayClear(*meets);
  for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
  {
    meet = KheEventMeet(soln, e, i);
    if( KheMeetAsstTime(meet) != NULL )
      HaArrayAddLast(*meets, meet);
  }
  HaArraySort(*meets, &KheMeetIncreasingTimeAsstCmp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMergeMeets(KHE_SOLN soln)                                        */
/*                                                                           */
/*  Merge meets of soln where it's possible and reduces cost.                */
/*  NB for this function to work correctly, split events and distribute      */
/*  split events monitors must be attached.                                  */
/*                                                                           */
/*****************************************************************************/

void KheMergeMeets(KHE_SOLN soln)
{
  ARRAY_KHE_MEET meets;  KHE_MEET meet1, meet2, rmeet1, rmeet2;  KHE_EVENT e;
  KHE_INSTANCE ins;  int i, j;  HA_ARENA a;
  if( DEBUG3 )
    fprintf(stderr, "[ KheMergeMeets(soln)\n");
  a = KheSolnArenaBegin(soln);
  /* a = HaArena Make(); */
  HaArrayInit(meets, a);
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    /* find the assigned meets of e and sort them chronologically */
    e = KheInstanceEvent(ins, i);
    KheFindAssignedMeets(e, soln, &meets);

    /* try merging adjacent pairs */
    for( j = 1;  j < HaArrayCount(meets);  j++ )
    {
      meet1 = HaArray(meets, j - 1);
      meet2 = HaArray(meets, j);
      if( KheMeetsMerged(meet1, meet2, &rmeet1, &rmeet2) )
      {
	/* meets merged, so meet2 no longer exists */
	HaArrayPut(meets, j - 1, rmeet1);
	HaArrayDeleteAndShift(meets, j);
	j--;
      }
      else
      {
	/* meets did not merge, but replace them by rmeet1 and rmeet2 */
	HaArrayPut(meets, j - 1, rmeet1);
	HaArrayPut(meets, j, rmeet2);
      }
    }
  }
  /* MArrayFree(meets); */
  /* HaArenaDe lete(a); */
  KheSolnArenaEnd(soln, a);
  if( DEBUG3 )
    fprintf(stderr, "] KheMergeMeets returning\n");
}

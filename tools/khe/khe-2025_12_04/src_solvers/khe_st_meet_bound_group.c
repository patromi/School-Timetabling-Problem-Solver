
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
/*  FILE:         khe_st_meet_bound_group.c                                  */
/*  DESCRIPTION:  Meet bound groups                                          */
/*                                                                           */
/*****************************************************************************/
#include <limits.h>
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET_BOUND_GROUP - a set of meet bounds                              */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_MEET_BOUND) ARRAY_KHE_MEET_BOUND;

struct khe_meet_bound_group_rec {
  HA_ARENA			arena;			/* arena             */
  KHE_SOLN			soln;			/* soln              */
  ARRAY_KHE_MEET_BOUND		meet_bounds;		/* the meet bounds   */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET_BOUND_GROUP KheMeetBoundGroupMake(void)                         */
/*                                                                           */
/*  Make a new meet bound group object.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_MEET_BOUND_GROUP KheMeetBoundGroupMake(KHE_SOLN soln)
{
  KHE_MEET_BOUND_GROUP res;  HA_ARENA a;
  a = KheSolnArenaBegin(soln);
  /* a = HaAr enaMake(); */
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  HaArrayInit(res->meet_bounds, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetBoundGroupAddMeetBound(KHE_MEET_BOUND_GROUP mbg,             */
/*    KHE_MEET_BOUND mb)                                                     */
/*                                                                           */
/*  Add mb to mbg.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheMeetBoundGroupAddMeetBound(KHE_MEET_BOUND_GROUP mbg, KHE_MEET_BOUND mb)
{
  HaArrayAddLast(mbg->meet_bounds, mb);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetBoundGroupMeetBoundCount(KHE_MEET_BOUND_GROUP mbg)            */
/*                                                                           */
/*  Return the number of meet bounds in mbg.                                 */
/*                                                                           */
/*****************************************************************************/

int KheMeetBoundGroupMeetBoundCount(KHE_MEET_BOUND_GROUP mbg)
{
  return HaArrayCount(mbg->meet_bounds);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET_BOUND KheMeetBoundGroupMeetBound(KHE_MEET_BOUND_GROUP mbg,      */
/*    int i)                                                                 */
/*                                                                           */
/*  Return the i'th meet bound of mbg.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_MEET_BOUND KheMeetBoundGroupMeetBound(KHE_MEET_BOUND_GROUP mbg, int i)
{
  return HaArray(mbg->meet_bounds, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetBoundGroupDelete(KHE_MEET_BOUND_GROUP mbg)                   */
/*                                                                           */
/*  Delete mbg and the meet bounds within it.  Return true if all calls to   */
/*  KheMeetBoundDelete returned true.                                        */
/*                                                                           */
/*****************************************************************************/

bool KheMeetBoundGroupDelete(KHE_MEET_BOUND_GROUP mbg)
{
  KHE_MEET_BOUND mb;  int i;  bool res;
  res = true;
  HaArrayForEach(mbg->meet_bounds, mb, i)
    res = res && KheMeetBoundDelete(mb);
  KheSolnArenaEnd(mbg->soln, mbg->arena);
  /* HaArena Delete(HaArrayArena(mbg->meet_bounds)); */
  /* MArrayFree(mbg->meet_bounds); */
  /* MFree(mbg); */
  return res;
}

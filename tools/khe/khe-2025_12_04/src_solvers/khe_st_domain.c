
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
/*  FILE:         khe_st_domain.c                                            */
/*  DESCRIPTION:  Meet domain reduction                                      */
/*                                                                           */
/*****************************************************************************/
#include <limits.h>
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheSolnStartingTimeGroup(KHE_SOLN soln,                   */
/*    KHE_TIME_GROUP tg, int durn)                                           */
/*                                                                           */
/*  Return the set of all starting times that ensure that a meet of duration */
/*  durn lies entirely within tg.                                            */
/*                                                                           */
/*****************************************************************************/

/* *** moved into platform
KHE_TIME_GROUP KheSolnS tartingTimeGroup(KHE_SOLN soln,
  KHE_TIME_GROUP tg, int durn)
{
  int i;  KHE_TIME time;
  if( durn == 1 )
    return tg;
  KheSolnTimeG roupBegin(soln);
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    time = KheTimeGroupTime(tg, i);
    if( KheTimeGroupOverlap(tg, time, durn) == durn )
      KheSolnTimeGroupAddTime(soln, time);
  }
  return KheSolnTimeGroupEnd(soln);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSubstractUnavailableTimes(KHE_SOLN soln, KHE_TIME_GROUP tg,      */
/*    int duration)                                                          */
/*                                                                           */
/*  Subtract the times of tg, and preceding times as well if duration > 1,   */
/*  from the time group currently being constructed in soln.                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSubstractUnavailableTimes(KHE_SOLN soln, KHE_TIME_GROUP tg,
  int duration)
{
  int delta;
  if( duration == KHE_ANY_DURATION || duration == 1 )
  {
    ** substract the times of tg **
    KheSolnTimeGroupDifference(soln, tg);
  }
  else
  {
    ** subtract the times of tg and neighbouring times **
    for( delta = 1 - duration;  delta <= 0;  delta++ )
      KheSolnTimeGroupDifference(soln, KheTimeGrou pNeighbour(tg, delta));
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoAddUnavailableBound(KHE_TASK task, KHE_COST min_weight,    */
/*    int offset)                                                            */
/*                                                                           */
/*  Carry out the specification of KheMeetDoAddUnavailableBound for task.    */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoAddUnavailableBound(KHE_TASK task, KHE_COST min_weight,
  int offset)
{
  KHE_RESOURCE r;  KHE_SOLN soln;  int i;  KHE_MONITOR m;
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR autm;  KHE_TIME_GROUP tg;
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT autc;
  if( KheTaskIsPreassigned(task, &r) )
  {
    soln = KheTaskSoln(task);
    for( i = 0;  i < KheSolnResourceMonitorCount(soln, r);  i++ )
    {
      m = KheSolnResourceMonitor(soln, r, i);
      if( KheMonitorTag(m) == KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG )
      {
	autm = (KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m;
	autc = KheAvoidUnavailableTimesMonitorConstraint(autm);
	if( KheMonitorCombinedWeight(m) >= min_weight )
	{
	  tg = KheAvoidUnavailableTimesConstraintUnavailableTimes(autc);
	  /* ***
	  if( offset != 0 )
	    tg = KheTimeGroup Neighbour(tg, -offset);
	  KheSubstractUnavailableTimes(soln, tg);
	  *** */
	  KheSolnTimeGroupDifference(soln, KheTimeGroupNeighbour(tg, -offset));
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDoAddUnavailableBound(KHE_MEET meet, KHE_COST min_weight,    */
/*    int offset)                                                            */
/*                                                                           */
/*  Subtract the unavailable times of the preassigned resources of meet      */
/*  and its fixed descendants, adjusted by offset, from the time group       */
/*  currently under construction in meet's soln.                             */
/*                                                                           */
/*****************************************************************************/

static void KheMeetDoAddUnavailableBound(KHE_MEET meet, KHE_COST min_weight,
  int offset)
{
  int i;  KHE_TASK task;  KHE_MEET child_meet;

  /* do the job for the tasks of meet */
  for( i = 0;  i < KheMeetTaskCount(meet);  i++ )
  {
    task = KheMeetTask(meet, i);
    KheTaskDoAddUnavailableBound(task, min_weight, offset);
  }

  /* do the job for the fixed child meets of meet */
  for( i = 0;  i < KheMeetAssignedToCount(meet);  i++ )
  {
    child_meet = KheMeetAssignedTo(meet, i);
    if( KheMeetAssignIsFixed(child_meet) )
      KheMeetDoAddUnavailableBound(child_meet, min_weight,
        offset + KheMeetAsstOffset(child_meet));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAddUnavailableBound(KHE_MEET meet, KHE_COST min_weight,      */
/*    KHE_MEET_BOUND_GROUP mbg)                                              */
/*                                                                           */
/*  Optionally add a meet bound to meet based on the unavailable times of    */
/*  resources preassigned to meet and meets with fixed assignments to it.    */
/*  If mbg != NULL, add the new bound to mbg.                                */
/*                                                                           */
/*****************************************************************************/

void KheMeetAddUnavailableBound(KHE_MEET meet, KHE_COST min_weight,
  KHE_MEET_BOUND_GROUP mbg)
{
  KHE_SOLN soln;  KHE_TIME_GROUP full_tg, tg;  KHE_MEET_BOUND mb;

  /* start off the time group */
  HnAssert(!KheMeetIsCycleMeet(meet),
    "KheMeetAddUnavailableBound: meet is a cycle meet");
  soln = KheMeetSoln(meet);
  full_tg = KheInstanceFullTimeGroup(KheSolnInstance(soln));
  KheSolnTimeGroupBegin(soln);
  KheSolnTimeGroupUnion(soln, full_tg);

  /* traverse meet and the meets fix-assigned to it */
  KheMeetDoAddUnavailableBound(meet, min_weight, 0);

  /* finish off the time group and add a meet bound if it is not full */
  tg = KheSolnTimeGroupEnd(soln);
  if( !KheTimeGroupEqual(tg, full_tg) )
  {
    mb = KheMeetBoundMake(soln, true, tg);
    KheMeetAddMeetBound(meet, mb);
    if( mbg != NULL )
      KheMeetBoundGroupAddMeetBound(mbg, mb);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAddUnavailableBounds(KHE_MEET meet, KHE_COST min_weight,     */
/*    KHE_MEET_BOUND_GROUP mbg)                                              */
/*                                                                           */
/*  Optionally add one meet bound to meet for each possible duration.        */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheMeetAddUnavailableBounds(KHE_MEET meet, KHE_COST min_weight,
  KHE_MEET_BOUND_GROUP mbg)
{
  int max_durn, durn;
  if( KheMeetEvent(meet) != NULL )
  {
    ** one time bound for each possible duration **
    max_durn = KheEventDuration(KheMeetEvent(meet));
    for( durn = 1;  durn <= max_durn;  durn++ )
      KheMeetAddUnavailableBound(meet, min_weight, durn, mbg);
  }
  else
  {
    ** one KHE_ANY_DURATION time bound **
    KheMeetAddUnavailableBound(meet, min_weight, KHE_ANY_DURATION, mbg);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDoAddUnavailableBounds(KHE_MEET meet, KHE_COST min_weight,   */
/*    KHE_MEET_BOUND_GROUP mbg)                                              */
/*                                                                           */
/*  Call KheMeetAddUnavailableBounds for every descendant of meet that is    */
/*  not a cycle meet and does not have a fixed assignment to another meet.   */
/*                                                                           */
/*****************************************************************************/

void KheMeetDoAddUnavailableBounds(KHE_MEET meet, KHE_COST min_weight,
  KHE_MEET_BOUND_GROUP mbg)
{
  int i;  KHE_MEET child_meet;

  /* handle meet, unless it's a cycle meet or fixed */
  if( !KheMeetIsCycleMeet(meet) && !KheMeetAssignIsFixed(meet) )
    KheMeetAddUnavailableBound(meet, min_weight, mbg);

  /* handle meet's proper descendants */
  for( i = 0;  i < KheMeetAssignedToCount(meet);  i++ )
  {
    child_meet = KheMeetAssignedTo(meet, i);
    KheMeetDoAddUnavailableBounds(child_meet, min_weight, mbg);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAddUnavailableBounds(KHE_SOLN soln, KHE_COST min_weight,     */
/*    KHE_MEET_BOUND_GROUP mbg)                                              */
/*                                                                           */
/*  Call KheMeetAddUnavailableBounds for every meet of soln that is not      */
/*  a cycle meet and does not have a fixed assignment to some other meet.    */
/*  Do this in a safe order (i.e. parents before children).                  */
/*                                                                           */
/*****************************************************************************/

void KheSolnAddUnavailableBounds(KHE_SOLN soln, KHE_COST min_weight,
  KHE_MEET_BOUND_GROUP mbg)
{
  int i;  KHE_MEET meet;
  for( i = 0;  i < KheSolnMeetCount(soln);  i++ )
  {
    meet = KheSolnMeet(soln, i);
    if( KheMeetAsst(meet) == NULL )
      KheMeetDoAddUnavailableBounds(meet, min_weight, mbg);
  }
}

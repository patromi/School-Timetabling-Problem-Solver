
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
/*  FILE:         khe_st_elm_irregular.c                                     */
/*  DESCRIPTION:  Elm layer matching - handling irregular monitors           */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "khe_elm.h"
#include "howard_n.h"

#define DEBUG1 0


/*****************************************************************************/
/*                                                                           */
/*  Submodule "detaching and attaching"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheElmDetachIrregularMonitors(KHE_ELM elm)                          */
/*                                                                           */
/*  Detach those irregular monitors of elm which are not already detached.   */
/*                                                                           */
/*****************************************************************************/

void KheElmDetachIrregularMonitors(KHE_ELM elm)
{
  KHE_MONITOR m;  int i;
  for( i = 0;  i < KheElmIrregularMonitorCount(elm);  i++ )
  {
    m = KheElmIrregularMonitor(elm, i);
    if( KheMonitorAttachedToSoln(m) )
      KheMonitorDetachFromSoln(m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmAttachIrregularMonitors(KHE_ELM elm)                          */
/*                                                                           */
/*  Attach those irregular monitors of elm which are not already attached.   */
/*                                                                           */
/*****************************************************************************/

void KheElmAttachIrregularMonitors(KHE_ELM elm)
{
  KHE_MONITOR m;  int i;
  for( i = 0;  i < KheElmIrregularMonitorCount(elm);  i++ )
  {
    m = KheElmIrregularMonitor(elm, i);
    if( !KheMonitorAttachedToSoln(m) )
      KheMonitorAttachToSoln(m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reducing irregular monitors"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheElmTestBest(KHE_ELM elm)                                     */
/*                                                                           */
/*  Return the cost of the best matching, irregulars included.               */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheElmTestBest(KHE_ELM elm)
{
  KHE_MARK mark;  KHE_COST res;  KHE_SOLN soln;
  soln = KheLayerSoln(KheElmLayer(elm));
  mark = KheMarkBegin(soln);
  KheElmBestAssignMeets(elm);
  KheElmAttachIrregularMonitors(elm);
  res = KheSolnCost(soln);
  KheElmDetachIrregularMonitors(elm);
  KheMarkEnd(mark, true);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmReduceIrregularMonitors(KHE_ELM elm)                          */
/*                                                                           */
/*  Reduce the cost of elm's irregular monitors by excluding supplies.       */
/*                                                                           */
/*****************************************************************************/

void KheElmReduceIrregularMonitors(KHE_ELM elm)
{
  bool monitors_attached;  KHE_ELM_SUPPLY s, best_s;
  KHE_COST cost, best_cost;  KHE_ELM_SUPPLY_GROUP sg;
  int i, j, durn, total_supply_excess;  HA_ARENA a;
  HA_ARRAY_INT supply_excess;  KHE_ELM_DEMAND_GROUP dg;  KHE_ELM_DEMAND d;
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheElmReduceIrregularMonitors(");
    KheLayerDebug(KheElmLayer(elm), 1, -1, stderr);
    fprintf(stderr, ")\n");
  }

  /* do nothing if no irregular monitors */
  if( KheElmIrregularMonitorCount(elm) == 0 )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheElmReduceIrregularMonitors ret. (no irregulars)\n");
    return;
  }

  /* initialize supply_excess, the excess supply for each duration */
  a = KheElmArena(elm);
  HaArrayInit(supply_excess, a);
  total_supply_excess = 0;
  for( i = 0;  i < KheElmSupplyGroupCount(elm);  i++ )
  {
    sg = KheElmSupplyGroup(elm, i);
    for( j = 0;  j < KheElmSupplyGroupSupplyCount(sg);  j++ )
    {
      s = KheElmSupplyGroupSupply(sg, j);
      if( !KheElmSupplyIsRemoved(s) )
      {
	durn = KheElmSupplyDuration(s);
        total_supply_excess += durn;
	while( HaArrayCount(supply_excess) < durn )
	  HaArrayAddLast(supply_excess, 0);
	++HaArray(supply_excess, durn - 1);
      }
    }
  }
  for( i = 0;  i < KheElmDemandGroupCount(elm);  i++ )
  {
    dg = KheElmDemandGroup(elm, i);
    for( j = 0;  j < KheElmDemandGroupDemandCount(dg);  j++ )
    {
      d = KheElmDemandGroupDemand(dg, j);
      durn = KheMeetDuration(KheElmDemandMeet(d));
      total_supply_excess -= durn;
      while( HaArrayCount(supply_excess) < durn )
	HaArrayAddLast(supply_excess, 0);
      --HaArray(supply_excess, durn - 1);
    }
  }
  if( total_supply_excess <= 0 )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheElmReduceIrregularMonitors ret. (no excess)\n");
    return;
  }
  if( DEBUG1 )
  {
    fprintf(stderr, "  excess: total:%d", total_supply_excess);
    HaArrayForEach(supply_excess, i, j)
      fprintf(stderr, ", %d:%d", j + 1, i);
    fprintf(stderr, "\n");
  }

  /* detach irregular monitors */
  monitors_attached = KheElmIrregularMonitorsAttached(elm);
  if( monitors_attached )
    KheElmDetachIrregularMonitors(elm);

  /* carry out as many removals as will help */
  while( total_supply_excess > 0 )
  {
    /* try the current state */
    best_cost = KheElmTestBest(elm);
    best_s = NULL;
    if( DEBUG1 )
      fprintf(stderr, "  init best: %.5f\n", KheCostShow(best_cost));

    /* try removing each supply node */
    for( i = 0;  i < KheElmSupplyGroupCount(elm);  i++ )
    {
      sg = KheElmSupplyGroup(elm, i);
      for( j = 0;  j < KheElmSupplyGroupSupplyCount(sg);  j++ )
      {
	s = KheElmSupplyGroupSupply(sg, j);
	durn = KheElmSupplyDuration(s);
	if( HaArray(supply_excess, durn - 1) > 0 &&
            total_supply_excess >= durn && !KheElmSupplyIsRemoved(s) )
	{
	  KheElmSupplyRemove(s);
	  cost = KheElmTestBest(elm);
	  if( cost < best_cost )
	  {
	    best_cost = cost;
	    best_s = s;
	    if( DEBUG1 )
	      fprintf(stderr, "  new best: %.5f\n", KheCostShow(best_cost));
	  }
	  KheElmSupplyUnRemove(s);
	}
      }
    }

    /* if no good removal was found, it's time to quit */
    if( best_s == NULL )
      break;

    /* remove best_s and go round again */
    KheElmSupplyRemove(best_s);
    durn = KheElmSupplyDuration(best_s);
    --HaArray(supply_excess, durn - 1);
    total_supply_excess -= durn;
  }

  /* return irregular monitors to their original state and return */
  if( monitors_attached )
    KheElmAttachIrregularMonitors(elm);
  /* MArrayFree(supply_excess); */
  if( DEBUG1 )
    fprintf(stderr, "] KheElmReduceIrregularMonitors ret. (end)\n");
}


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
/*  FILE:         khe_sr_balance.c                                           */
/*  DESCRIPTION:  Supply and demand balance                                  */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#define min(a, b) ((a) < (b) ? (a) : (b))
#define NO_COST -1


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskIsIncluded(KHE_TASK task, int *total_durn,                   */
/*    KHE_COST *cost_per_time)                                               */
/*                                                                           */
/*  Return true if task is included.  In that case, also set *total_durn     */
/*  and *cost_per_time.                                                      */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskIsIncluded(KHE_TASK task, KHE_RESOURCE_TYPE rt,
  int *total_durn, KHE_COST *cost_per_time)
{
  KHE_EVENT_RESOURCE er;  KHE_COST non_asst_cost, asst_cost;
  er = KheTaskEventResource(task);
  *total_durn = KheTaskTotalDuration(task);
  if( er != NULL && KheEventResourceResourceType(er) == rt &&
      KheTaskIsProperRoot(task) && *total_durn > 0 &&
      KheTaskAsstResource(task) == NULL )
  {
    KheTaskNonAsstAndAsstCost(task, &non_asst_cost, &asst_cost);
    if( non_asst_cost > asst_cost )
    {
      *cost_per_time = (non_asst_cost - asst_cost) / *total_durn;
      return true;
    }
  }
  *cost_per_time = 0;   /* unused, but we don't like to return it undefined */
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSetDemandAndTaskCost(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,        */
/*    int *demand, KHE_COST *task_cost)                                      */
/*                                                                           */
/*  Set the demand and task cost.                                            */
/*                                                                           */
/*****************************************************************************/

static void KheSetDemandAndTaskCost(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  int *demand, KHE_COST *task_cost)
{
  KHE_TASK task;  int i, total_durn;  KHE_COST cost_per_time;
  *demand = 0;
  *task_cost = NO_COST;
  for( i = 0;  i < KheSolnTaskCount(soln);  i++ )
  {
    task = KheSolnTask(soln, i);
    if( KheTaskIsIncluded(task, rt, &total_durn, &cost_per_time) )
    {
      *demand += total_durn;
      if( *task_cost == NO_COST || cost_per_time < *task_cost )
	*task_cost = cost_per_time;
    }
  }
  if( *task_cost == NO_COST )
    *task_cost = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceHasOverloadCost(KHE_RESOURCE r, KHE_AVAIL_SOLVER as,     */
/*    KHE_COST *overload_cost)                                               */
/*                                                                           */
/*  If r has an overload cost, set *overload_cost to that cost and return    */
/*  true.  Otherwise set *overload_cost to NO_COST and return false.         */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceHasOverloadCost(KHE_RESOURCE r, KHE_AVAIL_SOLVER as,
  KHE_COST *overload_cost)
{
  int i, limit;  KHE_AVAIL_NODE_TYPE type;  KHE_TIME_SET ts;
  KHE_MONITOR m;  KHE_COST m_weight;
  *overload_cost = NO_COST;
  for( i = 0;  i < KheAvailSolverMaxBusyTimesAvailNodeCount(as, r);  i++ )
  {
    KheAvailSolverMaxBusyTimesAvailNode(as, r, i, &type, &limit, &ts, &m);
    if( m != NULL )
    {
      m_weight = KheMonitorCombinedWeight(m);
      if( *overload_cost == NO_COST || m_weight < *overload_cost )
	*overload_cost = m_weight;
    }
  }
  return *overload_cost != NO_COST;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSetSupplyAndResourceCost(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,    */
/*    int *supply, KHE_COST *resource_cost)                                  */
/*                                                                           */
/*  Set the supply and resource cost.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheSetSupplyAndResourceCost(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  int *supply, KHE_COST *resource_cost)
{
  KHE_RESOURCE r;  int i, r_supply;  KHE_AVAIL_SOLVER as;
  KHE_COST r_overload_cost;
  *supply = 0;
  *resource_cost = NO_COST;
  as = KheSolnAvailSolver(soln);
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    if( KheAvailSolverMaxBusyTimes(as, r, &r_supply) )
    {
      *supply += r_supply;
      if( KheResourceHasOverloadCost(r, as, &r_overload_cost) &&
	  (*resource_cost == NO_COST || r_overload_cost < *resource_cost) )
	*resource_cost = r_overload_cost;
    }
  }
  if( *resource_cost == NO_COST )
    *resource_cost = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceDemandExceedsSupply(KHE_SOLN soln, KHE_RESOURCE_TYPE rt, */
/*    int *demand, int *supply, KHE_COST *task_cost, KHE_COST *resource_cost)*/
/*                                                                           */
/*  Return true if demand for resources of type rt exceeds supply, and       */
/*  set *demand to the demand, *supply to the supply, *task_cost to the      */
/*  task cost, and *resource_cost to the resource cost.                      */
/*                                                                           */
/*****************************************************************************/

bool KheResourceDemandExceedsSupply(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  int *demand, int *supply, KHE_COST *task_cost, KHE_COST *resource_cost)
{
  KheSetDemandAndTaskCost(soln, rt, demand, task_cost);
  KheSetSupplyAndResourceCost(soln, rt, supply, resource_cost);
  return *demand > *supply;
}

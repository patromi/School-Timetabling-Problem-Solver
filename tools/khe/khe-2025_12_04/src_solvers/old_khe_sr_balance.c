
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

/*****************************************************************************/
/*                                                                           */
/*  KHE_BALANCE_SOLVER                                                       */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_balance_group_rec {
  KHE_COST		cost_reduction;
  int			total_durn;
} *KHE_BALANCE_GROUP;

typedef HA_ARRAY(KHE_BALANCE_GROUP) ARRAY_KHE_BALANCE_GROUP;


struct khe_balance_solver_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;
  /* KHE_OPTIONS		options; */
  KHE_FRAME			days_frame;
  KHE_RESOURCE_TYPE		resource_type;
  ARRAY_KHE_BALANCE_GROUP	balance_groups;
  int				total_supply;
  int				total_demand;
  KHE_COST			marginal_cost;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "implementation functions (private)"                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheBalanceSolverGetTotalSupply(KHE_BALANCE_SOLVER bs)                */
/*                                                                           */
/*  Calculate and return the total supply.                                   */
/*                                                                           */
/*****************************************************************************/

static int KheBalanceSolverGetTotalSupply(KHE_BALANCE_SOLVER bs)
{
  int res, i, r_busy_times;  KHE_RESOURCE r;
  res = 0;
  for( i = 0;  i < KheResourceTypeResourceCount(bs->resource_type);  i++ )
  {
    r = KheResourceTypeResource(bs->resource_type, i);
    if( KheResourceMaxBusyTimes(bs->soln, r, &r_busy_times) )
      res += min(KheFrameTimeGroupCount(bs->days_frame), r_busy_times);
    else
      res += KheFrameTimeGroupCount(bs->days_frame);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_BALANCE_GROUP KheBalanceGroupMake(KHE_COST cost_reduction,           */
/*    int total_durn, HA_ARENA a)                                            */
/*                                                                           */
/*  Make a new balance group with these attributes.                          */
/*                                                                           */
/*****************************************************************************/

static KHE_BALANCE_GROUP KheBalanceGroupMake(KHE_COST cost_reduction,
  int total_durn, HA_ARENA a)
{
  KHE_BALANCE_GROUP res;
  HaMake(res, a);
  res->cost_reduction = cost_reduction;
  res->total_durn = total_durn;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheBalanceSolverAddToGroups(KHE_BALANCE_SOLVER bs,                  */
/*    int total_durn, KHE_COST cost_reduction)                               */
/*                                                                           */
/*  Accumulate these values in the balance groups.                           */
/*                                                                           */
/*****************************************************************************/

static void KheBalanceSolverAddToGroups(KHE_BALANCE_SOLVER bs,
  int total_durn, KHE_COST cost_reduction)
{
  KHE_BALANCE_GROUP bg;  int i;

  /* if there is already a balance group with this cost reduction, add to it */
  HaArrayForEach(bs->balance_groups, bg, i)
    if( bg->cost_reduction == cost_reduction )
    {
      bg->total_durn += total_durn;
      return;
    }

  /* if there is no balance group with this cost reduction, make and add one */
  bg = KheBalanceGroupMake(cost_reduction, total_durn, bs->arena);
  HaArrayAddLast(bs->balance_groups, bg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheBalanceSolverGetTotalDemand(KHE_BALANCE_SOLVER bs)                */
/*                                                                           */
/*  Calculate and return the total demand.  Also build the balance groups.   */
/*                                                                           */
/*****************************************************************************/

static int KheBalanceSolverGetTotalDemand(KHE_BALANCE_SOLVER bs)
{
  int i, total_durn, res;  KHE_TASK task;  KHE_COST cost_reduction;
  res = 0;
  for( i = 0;  i < KheSolnTaskCount(bs->soln);  i++ )
  {
    task = KheSolnTask(bs->soln, i);
    if( KheTaskResourceType(task) == bs->resource_type &&
	!KheTaskIsCycleTask(task) && KheTaskAsst(task) == NULL )
    {
      total_durn = KheTaskTotalDuration(task);
      cost_reduction = KheTaskAssignmentCostReduction(task);
      KheBalanceSolverAddToGroups(bs, total_durn, cost_reduction);
      res += total_durn;
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheBalanceGroupTypedCmp(KHE_BALANCE_GROUP bg1, KHE_BALANCE_GROUP bg2)*/
/*                                                                           */
/*  Typed comparison function for sorting an array of balance groups into    */
/*  decreasing cost reduction order.                                         */
/*                                                                           */
/*  Implementation note.  Value bg2->cost_reduction - bg1->cost_reduction    */
/*  is not a suitable return value; its type is wider than int.              */
/*                                                                           */
/*****************************************************************************/

static int KheBalanceGroupTypedCmp(KHE_BALANCE_GROUP bg1, KHE_BALANCE_GROUP bg2)
{
  KHE_COST diff;
  diff = bg2->cost_reduction - bg1->cost_reduction;
  return diff < 0 ? -1 : diff > 0 ? 1 : 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheBalanceGroupUntypedCmp(const void *t1, const void *t2)            */
/*                                                                           */
/*  Untyped comparison function for sorting an array of balance groups into  */
/*  decreasing cost reduction order.                                         */
/*                                                                           */
/*****************************************************************************/

static int KheBalanceGroupUntypedCmp(const void *t1, const void *t2)
{
  KHE_BALANCE_GROUP bg1 = * (KHE_BALANCE_GROUP *) t1;
  KHE_BALANCE_GROUP bg2 = * (KHE_BALANCE_GROUP *) t2;
  return KheBalanceGroupTypedCmp(bg1, bg2);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheBalanceSolverGetMarginalCost(KHE_BALANCE_SOLVER bs)          */
/*                                                                           */
/*  Calculate and return the marginal cost.                                  */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheBalanceSolverGetMarginalCost(KHE_BALANCE_SOLVER bs)
{
  int remaining_supply, i;  KHE_BALANCE_GROUP bg;
  remaining_supply = bs->total_supply;
  HaArrayForEach(bs->balance_groups, bg, i)
  {
    remaining_supply -= bg->total_durn;
    if( remaining_supply <= 0 )
      return bg->cost_reduction;
  }
  return KheCost(0, 0);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "public functions"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_BALANCE_SOLVER KheBalanceSolverMake(KHE_SOLN soln,                   */
/*    KHE_RESOURCE_TYPE rt, KHE_FRAME days_frame, HA_ARENA a)                */
/*                                                                           */
/*  Make a new balance solver with these attributes.                         */
/*                                                                           */
/*****************************************************************************/

KHE_BALANCE_SOLVER KheBalanceSolverMake(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_FRAME days_frame, HA_ARENA a)
{
  KHE_BALANCE_SOLVER res;
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  /* res->options = options; */
  res->days_frame = days_frame;
  /* res->days_frame = KheOptionsFrame(options, "gs_common_frame", soln); */
  res->resource_type = rt;
  HaArrayInit(res->balance_groups, a);
  res->total_supply = KheBalanceSolverGetTotalSupply(res);
  res->total_demand = KheBalanceSolverGetTotalDemand(res);
  HaArraySort(res->balance_groups, &KheBalanceGroupUntypedCmp);
  res->marginal_cost = KheBalanceSolverGetMarginalCost(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheBalanceSolverTotalSupply(KHE_BALANCE_SOLVER bs)                   */
/*                                                                           */
/*  Return the total supply found by bs.                                     */
/*                                                                           */
/*****************************************************************************/

int KheBalanceSolverTotalSupply(KHE_BALANCE_SOLVER bs)
{
  return bs->total_supply;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheBalanceSolverTotalDemand(KHE_BALANCE_SOLVER bs)                   */
/*                                                                           */
/*  Return the total demand found by bs.                                     */
/*                                                                           */
/*****************************************************************************/

int KheBalanceSolverTotalDemand(KHE_BALANCE_SOLVER bs)
{
  return bs->total_demand;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheBalanceSolverDemandGroupCount(KHE_BALANCE_SOLVER bs)              */
/*                                                                           */
/*  Return the number of demand groups found by bs.                          */
/*                                                                           */
/*****************************************************************************/

int KheBalanceSolverDemandGroupCount(KHE_BALANCE_SOLVER bs)
{
  return HaArrayCount(bs->balance_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheBalanceSolverDemandGroup(KHE_BALANCE_SOLVER bs, int i,           */
/*    KHE_COST *cost_reduction, int *total_durn)                             */
/*                                                                           */
/*  Return the information kept by bs about the i'th demand group.           */
/*                                                                           */
/*****************************************************************************/

void KheBalanceSolverDemandGroup(KHE_BALANCE_SOLVER bs, int i,
  KHE_COST *cost_reduction, int *total_durn)
{
  KHE_BALANCE_GROUP bg;
  bg = HaArray(bs->balance_groups, i);
  *cost_reduction = bg->cost_reduction;
  *total_durn = bg->total_durn;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheBalanceSolverMarginalCost(KHE_BALANCE_SOLVER bs)             */
/*                                                                           */
/*  Return the marginal cost of not assigning a task, as found by bs.        */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheBalanceSolverMarginalCost(KHE_BALANCE_SOLVER bs)
{
  return bs->marginal_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheBalanceGroupDebug(KHE_BALANCE_GROUP bg, int verbosity,           */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of bg onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheBalanceGroupDebug(KHE_BALANCE_GROUP bg, int verbosity,
  int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "[%.5f %d]", KheCostShow(bg->cost_reduction), bg->total_durn);
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheBalanceSolverDebugHeader(KHE_BALANCE_SOLVER bs, FILE *fp)        */
/*                                                                           */
/*  Print the header part of the debug of bs.                                */
/*                                                                           */
/*****************************************************************************/

static void KheBalanceSolverDebugHeader(KHE_BALANCE_SOLVER bs, FILE *fp)
{
  fprintf(fp, "Balance Solver(%s %s, supply %d, demand %d, marg_cost %.5f)",
    KheInstanceId(KheSolnInstance(bs->soln)),
    KheResourceTypeId(bs->resource_type),
    bs->total_supply, bs->total_demand, KheCostShow(bs->marginal_cost));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheBalanceSolverDebug(KHE_BALANCE_SOLVER bs, int verbosity,         */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of bs onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

void KheBalanceSolverDebug(KHE_BALANCE_SOLVER bs, int verbosity,
  int indent, FILE *fp)
{
  KHE_BALANCE_GROUP bg;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheBalanceSolverDebugHeader(bs, fp);
    fprintf(fp, "\n");
    HaArrayForEach(bs->balance_groups, bg, i)
      KheBalanceGroupDebug(bg, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheBalanceSolverDebugHeader(bs, fp);
}

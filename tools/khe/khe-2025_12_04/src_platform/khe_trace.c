
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
/*  FILE:         khe_trace.c                                                */
/*  DESCRIPTION:  A trace                                                    */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

/*****************************************************************************/
/*                                                                           */
/*  KHE_TRACE - a trace                                                      */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(int64_t) ARRAY_INT64;

typedef struct khe_monitor_info_rec {
  KHE_MONITOR			monitor;
  KHE_COST			init_cost;
} KHE_MONITOR_INFO;

typedef HA_ARRAY(KHE_MONITOR_INFO) ARRAY_KHE_MONITOR_INFO;

struct khe_trace_rec {
  KHE_GROUP_MONITOR		group_monitor;		/* tracing this      */
  KHE_COST			gm_init_cost;		/* init cost of gm   */
  int				init_matching_defect_count;
  /* int			soln_defect_count; */	/* soln defect count */
  bool				active;			/* tracing now       */
  ARRAY_KHE_MONITOR_INFO	monitor_info;		/* monitor info      */
  /* ARRAY_KHE_MONITOR		monitors; */		/* changed ones      */
  /* ARRAY_INT64		init_costs; */		/* init costs        */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TRACE KheTraceMake(KHE_GROUP_MONITOR gm)                             */
/*                                                                           */
/*  Create a new trace for gm.                                               */
/*                                                                           */
/*****************************************************************************/

KHE_TRACE KheTraceMake(KHE_GROUP_MONITOR gm)
{
  KHE_TRACE res;  KHE_SOLN soln;  HA_ARENA a;
  soln = KheMonitorSoln((KHE_MONITOR) gm);
  KheSolnMatchingUpdate(soln);
  res = KheSolnGetTraceFromFreeList(soln);
  if( res == NULL )
  {
    a = KheSolnArena(soln);
    HaMake(res, a);
    HaArrayInit(res->monitor_info, a);
    /* ***
    HaArrayInit(res->monitors, a);
    HaArrayInit(res->init_costs, a);
    *** */
  }
  else
  {
    HaArrayClear(res->monitor_info);
    /* ***
    HaArrayClear(res->monitors);
    HaArrayClear(res->init_costs);
    *** */
  }
  res->group_monitor = gm;
  res->gm_init_cost = 0;
  res->init_matching_defect_count = 0;
  /* res->soln_defect_count = 0; */
  res->active = false;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP_MONITOR KheTraceGroupMonitor(KHE_TRACE t)                      */
/*                                                                           */
/*  Return the group monitor that t traces.                                  */
/*                                                                           */
/*****************************************************************************/

KHE_GROUP_MONITOR KheTraceGroupMonitor(KHE_TRACE t)
{
  return t->group_monitor;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTraceDelete(KHE_TRACE t)                                         */
/*                                                                           */
/*  Delete t.                                                                */
/*                                                                           */
/*****************************************************************************/

void KheTraceDelete(KHE_TRACE t)
{
  KHE_SOLN soln;
  if( t->active )
    KheTraceEnd(t);
  soln = KheMonitorSoln((KHE_MONITOR) t->group_monitor);
  HnAssert(!t->active, "KheTraceDelete called before KheTraceEnd");
  KheSolnAddTraceToFreeList(soln, t);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTraceBegin(KHE_TRACE t)                                          */
/*                                                                           */
/*  Begin tracing with t.                                                    */
/*                                                                           */
/*****************************************************************************/

void KheTraceBegin(KHE_TRACE t)
{
  KHE_MONITOR m;
  HnAssert(!t->active,
    "KheTraceBegin called twice with no intervening KheTraceEnd");
  m = (KHE_MONITOR) t->group_monitor;
  t->gm_init_cost = KheMonitorCost(m);
  t->init_matching_defect_count = KheSolnMatchingDefectCount(KheMonitorSoln(m));
  /* t->soln_defect_count = KheSolnDefectCount(KheMonitorSoln(m)); */
  t->active = true;
  HaArrayClear(t->monitor_info);
  /* ***
  HaArrayClear(t->monitors);
  HaArrayClear(t->init_costs);
  *** */
  KheGroupMonitorBeginTrace(t->group_monitor, t);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTraceEnd(KHE_TRACE t)                                            */
/*                                                                           */
/*  End tracing with t.                                                      */
/*                                                                           */
/*****************************************************************************/

void KheTraceEnd(KHE_TRACE t)
{
  HnAssert(t->active, "KheTraceEnd with no matching call to KheTraceBegin");
  t->active = false;
  KheGroupMonitorEndTrace(t->group_monitor, t);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTraceFree(KHE_TRACE t)                                           */
/*                                                                           */
/*  Reclaim the memory used by t (internal use only).                        */
/*                                                                           */
/*****************************************************************************/

/* *** no longer required
void KheTraceFree(KHE_TRACE t)
{
  MArrayFree(t->monitors);
  MArrayFree(t->init_costs);
  MFree(t);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTraceContainsMonitor(KHE_TRACE t, KHE_MONITOR m)                 */
/*                                                                           */
/*  Return true if t's monitor info list contains m.                         */
/*                                                                           */
/*****************************************************************************/

bool KheTraceContainsMonitor(KHE_TRACE t, KHE_MONITOR m)
{
  int i;
  for( i = 0;  i < HaArrayCount(t->monitor_info);  i++ )
    if( HaArray(t->monitor_info, i).monitor == m )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTraceChangeCost(KHE_TRACE t, KHE_MONITOR m, KHE_COST old_cost)   */
/*                                                                           */
/*  The cost of m has changed.  Trace it.                                    */
/*                                                                           */
/*****************************************************************************/

void KheTraceChangeCost(KHE_TRACE t, KHE_MONITOR m, KHE_COST old_cost)
{
  KHE_MONITOR_INFO mi;
  if( !KheTraceContainsMonitor(t, m) )
  {
    mi.monitor = m;
    mi.init_cost = old_cost;
    HaArrayAddLast(t->monitor_info, mi);
    /* ***
    HaArrayAddLast(t->monitors, m);
    HaArrayAddLast(t->init_costs, old_cost);
    *** */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheTraceInitCost(KHE_TRACE t)                                   */
/*                                                                           */
/*  Return the cost of t's group monitor at the time tracing started.        */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheTraceInitCost(KHE_TRACE t)
{
  return t->gm_init_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTraceMonitorCount(KHE_TRACE t)                                    */
/*                                                                           */
/*  Return the number of child monitors of t's group monitor that changed    */
/*  their cost during the most recent trace.                                 */
/*                                                                           */
/*****************************************************************************/

int KheTraceMonitorCount(KHE_TRACE t)
{
  KheSolnMatchingUpdate(KheMonitorSoln((KHE_MONITOR) t->group_monitor));
  return HaArrayCount(t->monitor_info);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheTraceMonitor(KHE_TRACE t, int i)                          */
/*                                                                           */
/*  Return the i'th monitor that changed its cost.                           */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR KheTraceMonitor(KHE_TRACE t, int i)
{
  KheSolnMatchingUpdate(KheMonitorSoln((KHE_MONITOR) t->group_monitor));
  return HaArray(t->monitor_info, i).monitor;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheTraceMonitorInitCost(KHE_TRACE t, int i)                     */
/*                                                                           */
/*  Return the initial cost of the i'th monitor.                             */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheTraceMonitorInitCost(KHE_TRACE t, int i)
{
  return HaArray(t->monitor_info, i).init_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheMonitorInfoCostIncrease(KHE_MONITOR_INFO mi)                 */
/*                                                                           */
/*  Return the cost increase of mi.                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheMonitorInfoCostIncrease(KHE_MONITOR_INFO mi)
{
  return KheMonitorCost(mi.monitor) - mi.init_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheTraceMonitorCostIncrease(KHE_TRACE t, int i)                 */
/*                                                                           */
/*  Return the cost increase of the i'th monitor of t.                       */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheTraceMonitorCostIncrease(KHE_TRACE t, int i)
{
  return KheMonitorInfoCostIncrease(HaArray(t->monitor_info, i));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTraceMonitorInfoCmp(const void *p1, const void *p2)               */
/*                                                                           */
/*  Comparison function for sorting an array of monitor info objects by      */
/*  decreasing increased cost.                                               */
/*                                                                           */
/*****************************************************************************/

static int KheMonitorInfoCmp(const void *p1, const void *p2)
{
  KHE_MONITOR_INFO *m1 = (KHE_MONITOR_INFO *) p1;
  KHE_MONITOR_INFO *m2 = (KHE_MONITOR_INFO *) p2;
  return KheCostCmp(KheMonitorInfoCostIncrease(*m2),
    KheMonitorInfoCostIncrease(*m1));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTraceReduceByCostIncrease(KHE_TRACE t, int max_num)              */
/*                                                                           */
/*  Reduce t, that is, sort its monitors by decreasing cost increase,        */
/*  remove any whose cost decreased, and remove others until at most         */
/*  max_num remain.                                                          */
/*                                                                           */
/*****************************************************************************/

void KheTraceReduceByCostIncrease(KHE_TRACE t, int max_num)
{
  int excess, i;  KHE_MONITOR_INFO mi;

  /* remove monitors whose cost did not increase */
  HaArrayForEach(t->monitor_info, mi, i)
    if( KheMonitorInfoCostIncrease(mi) <= 0 )
      HaArrayDeleteAndPlug(t->monitor_info, i);

  /* sort the monitors by decreasing cost increase */
  HaArraySort(t->monitor_info, &KheMonitorInfoCmp);

  /* remove monitors beyond max_num */
  excess = HaArrayCount(t->monitor_info) - max_num;
  if( excess > 0 )
    HaArrayDeleteLastSlice(t->monitor_info, excess);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTraceDebug(KHE_TRACE t, int verbosity, int indent, FILE *fp)     */
/*                                                                           */
/*  Debug print of t onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheTraceDebug(KHE_TRACE t, int verbosity, int indent, FILE *fp)
{
  KHE_MONITOR_INFO mi;  int i;  KHE_MONITOR m;  KHE_SOLN soln;
  m = (KHE_MONITOR) t->group_monitor;
  soln = KheMonitorSoln(m);
  fprintf(fp, "%*s[ trace (%s, cost %.5f -> %.5f, matching %d -> %d)\n",
    indent, "", t->active ? "active" : "inactive",
    KheCostShow(t->gm_init_cost), KheCostShow(KheMonitorCost(m)),
    t->init_matching_defect_count, KheSolnMatchingDefectCount(soln));
  fprintf(fp, "%*s  [ %d matching defects according to trace:\n", indent, "",
    KheSolnMatchingDefectCount(soln));
  for( i = 0;  i < KheSolnMatchingDefectCount(soln);  i++ )
  {
    m = KheSolnMatchingDefect(soln, i);
    KheMonitorDebug(m, 2, 4, stderr);
  }
  fprintf(fp, "%*s  ]\n", indent, "");
  HaArrayForEach(t->monitor_info, mi, i)
    if( verbosity >= 4 || mi.init_cost != KheMonitorCost(mi.monitor) )
    {
      fprintf(fp, "%*s  %.5f -> %.5f ", indent, "", KheCostShow(mi.init_cost),
	KheCostShow(KheMonitorCost(mi.monitor)));
      KheMonitorDebug(mi.monitor, verbosity, 0, fp);
    }
  fprintf(fp, "%*s]\n", indent, "");
}

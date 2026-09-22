
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
/*  FILE:         khe_st_cluster_and_limit.c                                 */
/*  DESCRIPTION:  Preventing cluster busy times and limit idle times defects */
/*                                                                           */
/*  NB "Cal" in this file stands for "cluster and limit"                     */
/*                                                                           */
/*****************************************************************************/
#include <limits.h>
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"
#include "khe_priqueue.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG6 0

typedef struct khe_cal_time_group_rec *KHE_CAL_TIME_GROUP;
typedef HA_ARRAY(KHE_CAL_TIME_GROUP) ARRAY_KHE_CAL_TIME_GROUP;

typedef struct khe_cal_cluster_monitor_rec *KHE_CAL_CLUSTER_MONITOR;
typedef HA_ARRAY(KHE_CAL_CLUSTER_MONITOR) ARRAY_KHE_CAL_CLUSTER_MONITOR;

typedef struct khe_cal_idle_monitor_rec *KHE_CAL_IDLE_MONITOR;
typedef HA_ARRAY(KHE_CAL_IDLE_MONITOR) ARRAY_KHE_CAL_IDLE_MONITOR;

typedef struct khe_cal_vertex_rec *KHE_CAL_VERTEX;
typedef HA_ARRAY(KHE_CAL_VERTEX) ARRAY_KHE_CAL_VERTEX;

typedef struct khe_cal_edge_rec *KHE_CAL_EDGE;
typedef HA_ARRAY(KHE_CAL_EDGE) ARRAY_KHE_CAL_EDGE;

typedef struct khe_cal_graph_rec *KHE_CAL_GRAPH;


/*****************************************************************************/
/*                                                                           */
/*  KHE_CAL_STATE - the state of exclusion of a time group or time           */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_CAL_OPEN,			/* not yet tested for exclusion              */
  KHE_CAL_CLOSED_NOT_EXCLUDED,	/* tested for exclusion but did not exclude  */
  KHE_CAL_CLOSED_EXCLUDED	/* tested for exclusion and did exclude      */
} KHE_CAL_STATE;

typedef HA_ARRAY(KHE_CAL_STATE) ARRAY_KHE_CAL_STATE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_CAL_TIME_GROUP - a time group, with related info                     */
/*                                                                           */
/*****************************************************************************/

struct khe_cal_time_group_rec {
  KHE_CAL_VERTEX		vertex;			/* encl vertex       */
  KHE_TIME_GROUP		time_group;		/* the time group    */
  ARRAY_KHE_CAL_CLUSTER_MONITOR cluster_monitors;	/* cluster monitors  */
  ARRAY_KHE_CAL_IDLE_MONITOR	idle_monitors;		/* idle monitors     */
  ARRAY_KHE_CAL_STATE		states;			/* state at each time*/
  int				avail_durn;		/* open, not excluded*/
  int				cluster_cost;		/* cost (temporary)  */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_CAL_CLUSTER_MONITOR - a cluster busy times monitor, with info        */
/*                                                                           */
/*****************************************************************************/

struct khe_cal_cluster_monitor_rec {
  KHE_CAL_VERTEX		vertex;			/* encl vertex       */
  KHE_CLUSTER_BUSY_TIMES_MONITOR monitor;		/* the monitor       */
  int				monitor_maximum;	/* max limit         */
  ARRAY_KHE_CAL_TIME_GROUP	time_groups;		/* its time groups   */
  int				exclusions_wanted;	/* no. wanted        */
  /* int			open_time_groups; */	/* open time groups  */
  int				excluded_time_groups;	/* excluded tg's     */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_CAL_IDLE_MONITOR - a limit idle times monitor, with info             */
/*                                                                           */
/*****************************************************************************/

struct khe_cal_idle_monitor_rec {
  KHE_CAL_VERTEX		vertex;			/* encl vertex       */
  KHE_LIMIT_IDLE_TIMES_MONITOR	monitor;		/* the monitor       */
  int				monitor_maximum;	/* max limit         */
  ARRAY_KHE_CAL_TIME_GROUP	time_groups;		/* its time groups   */
  KHE_COST			priqueue_key;		/* key in priqueue   */
  int				priqueue_index;		/* index in priqueue */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_CAL_VERTEX - vertex in cal graph, representing one resource          */
/*                                                                           */
/*****************************************************************************/

struct khe_cal_vertex_rec {
  KHE_CAL_GRAPH			graph;			/* enclosing graph   */
  KHE_RESOURCE_GROUP		resource_group;		/* time-equiv r's    */
  int				preassigned_durn;	/* preassigned durn  */
  int				avail_durn;		/* total avail durn  */
  ARRAY_KHE_CAL_EDGE		edges;			/* adjacent edges    */
  ARRAY_KHE_CAL_CLUSTER_MONITOR cluster_monitors;	/* cluster monitors  */
  ARRAY_KHE_CAL_IDLE_MONITOR	idle_monitors;		/* idle monitors     */
  ARRAY_KHE_CAL_TIME_GROUP	time_groups;		/* their time groups */
  /* int			open_time_groups; */	/* open time groups  */
  int				excluded_time_groups;	/* excluded tg's     */
  int				priqueue_key;		/* key in priqueue   */
  int				priqueue_index;		/* index in priqueue */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_CAL_EDGE - edge in cal graph                                         */
/*                                                                           */
/*****************************************************************************/

struct khe_cal_edge_rec {
  KHE_CAL_VERTEX		endpoint;		/* vertex at end     */
  int				positive_cost;		/* positive edge cost*/
  int				negative_cost;		/* negative edge cost*/
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_CAL_GRAPH - cal graph                                                */
/*                                                                           */
/*****************************************************************************/

struct khe_cal_graph_rec {
  HA_ARENA			arena;			/* arena             */
  KHE_SOLN			soln;			/* enclosing soln    */
  KHE_TIME_EQUIV		time_equiv;		/* time-equiv object */
  KHE_COST			min_cluster_weight;	/* parameter         */
  KHE_COST			min_idle_weight;	/* parameter         */
  float				slack;			/* parameter         */
  KHE_MEET_BOUND_GROUP		meet_bound_group;	/* stores bounds     */
  ARRAY_KHE_CAL_VERTEX		vertices;		/* graph's vertices  */
  KHE_PRIQUEUE			cluster_priqueue;	/* cluster priqueue  */
  KHE_PRIQUEUE			idle_priqueue;		/* idle priqueue     */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "cal time groups"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CAL_TIME_GROUP KheCalTimeGroupMake(KHE_CAL_VERTEX cv,                */
/*    KHE_TIME_GROUP tg)                                                     */
/*                                                                           */
/*  Make an cal time group for tg, initially open.                           */
/*                                                                           */
/*****************************************************************************/

static KHE_CAL_TIME_GROUP KheCalTimeGroupMake(KHE_CAL_VERTEX cv,
  KHE_TIME_GROUP tg, HA_ARENA a)
{
  KHE_CAL_TIME_GROUP res;  int i;
  HaMake(res, a);
  res->vertex = cv;
  res->time_group = tg;
  HaArrayInit(res->cluster_monitors, a);
  HaArrayInit(res->idle_monitors, a);
  HaArrayInit(res->states, a);
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
    HaArrayAddLast(res->states, KHE_CAL_OPEN);
  res->avail_durn = KheTimeGroupTimeCount(tg);
  res->cluster_cost = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalTimeGroupDelete(KHE_CAL_TIME_GROUP ctg)                       */
/*                                                                           */
/*  Delete ctg.                                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheCalTimeGroupDelete(KHE_CAL_TIME_GROUP ctg)
{
  MArrayFree(ctg->states);
  MArrayFree(ctg->cluster_monitors);
  MArrayFree(ctg->idle_monitors);
  MFree(ctg);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheCalTimeGroupIsCompatible(KHE_CAL_TIME_GROUP ctg,                 */
/*    KHE_TIME_GROUP tg)                                                     */
/*                                                                           */
/*  Return true if ctg and tg are compatible, that is, if as time groups     */
/*  they are either equal or disjoint.                                       */
/*                                                                           */
/*****************************************************************************/

static bool KheCalTimeGroupIsCompatible(KHE_CAL_TIME_GROUP ctg,
  KHE_TIME_GROUP tg)
{
  return KheTimeGroupEqual(ctg->time_group, tg) ||
    KheTimeGroupDisjoint(ctg->time_group, tg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalTimeGroupAddClusterMonitor(KHE_CAL_TIME_GROUP ctg,            */
/*    KHE_CAL_CLUSTER_MONITOR ccm)                                           */
/*                                                                           */
/*  Add a cluster monitor to ctg.                                            */
/*                                                                           */
/*****************************************************************************/

static void KheCalTimeGroupAddClusterMonitor(KHE_CAL_TIME_GROUP ctg,
  KHE_CAL_CLUSTER_MONITOR ccm)
{
  HaArrayAddLast(ctg->cluster_monitors, ccm);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalTimeGroupAddIdleMonitor(KHE_CAL_TIME_GROUP ctg,               */
/*    KHE_CAL_IDLE_MONITOR cim)                                              */
/*                                                                           */
/*  Add an idle monitor to ctg.                                              */
/*                                                                           */
/*****************************************************************************/

static void KheCalTimeGroupAddIdleMonitor(KHE_CAL_TIME_GROUP ctg,
  KHE_CAL_IDLE_MONITOR cim)
{
  HaArrayAddLast(ctg->idle_monitors, cim);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalTimeGroupSetClusterCost(KHE_CAL_TIME_GROUP ctg)               */
/*                                                                           */
/*  Set ctg->cluster_cost to its current cluster cost, or 0 if not open      */
/*  or not used by cluster monitors.                                         */
/*                                                                           */
/*****************************************************************************/
static bool KheCalVertexContainsTimeGroup(KHE_CAL_VERTEX cv,
  KHE_TIME_GROUP tg, KHE_CAL_TIME_GROUP *ctg);

static void KheCalTimeGroupSetClusterCost(KHE_CAL_TIME_GROUP ctg)
{
  KHE_CAL_EDGE ce;  int i;  KHE_CAL_TIME_GROUP ctg2;
  ctg->cluster_cost = 0;
  if( HaArrayCount(ctg->cluster_monitors) > 0 &&
      HaArrayCount(ctg->states) > 0 && HaArrayFirst(ctg->states) == KHE_CAL_OPEN )
    HaArrayForEach(ctg->vertex->edges, ce, i)
      if( KheCalVertexContainsTimeGroup(ce->endpoint, ctg->time_group, &ctg2)
	  && HaArrayFirst(ctg2->states) == KHE_CAL_CLOSED_EXCLUDED )
	ctg->cluster_cost += (ce->negative_cost - ce->positive_cost);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCalTimeGroupIncreasingClusterCostCmp(const void *t1,              */
/*    const void *t2)                                                        */
/*                                                                           */
/*  Comparison function for sorting time groups by increasing cluster cost.  */
/*                                                                           */
/*****************************************************************************/

static int KheCalTimeGroupIncreasingClusterCostCmp(const void *t1,
  const void *t2)
{
  KHE_CAL_TIME_GROUP ctg1 = * (KHE_CAL_TIME_GROUP *) t1;
  KHE_CAL_TIME_GROUP ctg2 = * (KHE_CAL_TIME_GROUP *) t2;
  return ctg1->cluster_cost - ctg2->cluster_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  char KheCalStateShow(KHE_CAL_STATE state)                                */
/*                                                                           */
/*  Return the value of state in printable char form.                        */
/*                                                                           */
/*****************************************************************************/

static char KheCalStateShow(KHE_CAL_STATE state)
{
  switch( state )
  {
    case KHE_CAL_OPEN:			return 'O';
    case KHE_CAL_CLOSED_NOT_EXCLUDED:	return 'C';
    case KHE_CAL_CLOSED_EXCLUDED:	return 'E';
    default:				return '?';
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCalTimeGroupClusterExclusionIsOpen(KHE_CAL_TIME_GROUP ctg)       */
/*                                                                           */
/*  Return true if ctg is open for a cluster exclusion:  if it has not been  */
/*  tried, and some cluster monitor would benefit if from the exclusion.     */
/*                                                                           */
/*****************************************************************************/
static bool KheCalClusterMonitorIsOpen(KHE_CAL_CLUSTER_MONITOR ccm);
static bool KheCalVertexIsOpen(KHE_CAL_VERTEX cv, int durn);

static bool KheCalTimeGroupClusterExclusionIsOpen(KHE_CAL_TIME_GROUP ctg)
{
  KHE_CAL_CLUSTER_MONITOR ccm;  int i;
  if( KheCalVertexIsOpen(ctg->vertex, KheTimeGroupTimeCount(ctg->time_group))
      && HaArrayCount(ctg->states) > 0 && HaArrayFirst(ctg->states)==KHE_CAL_OPEN )
    HaArrayForEach(ctg->cluster_monitors, ccm, i)
      if( KheCalClusterMonitorIsOpen(ccm) )
	return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCalTimeGroupLastOpen(KHE_CAL_TIME_GROUP ctg, int *time_pos)      */
/*                                                                           */
/*  If ctg has a last time open to being excluded, set *time_pos to that     */
/*  time's position and return true.  Otherwise return false.                */
/*                                                                           */
/*****************************************************************************/

static bool KheCalTimeGroupLastOpen(KHE_CAL_TIME_GROUP ctg, int *time_pos)
{
  int i;  KHE_CAL_STATE state;
  if( KheCalVertexIsOpen(ctg->vertex, 1) && ctg->avail_durn > 0 )
    for( i = HaArrayCount(ctg->states) - 1;  i >= 0;  i-- )
    {
      state = HaArray(ctg->states, i);
      switch( state )
      {
	case KHE_CAL_OPEN:

	  *time_pos = i;
	  return true;

	case KHE_CAL_CLOSED_NOT_EXCLUDED:

	  /* can't keep searching past this point */
	  *time_pos = -1;
	  return false;

	case KHE_CAL_CLOSED_EXCLUDED:

	  /* OK to keep searching past this point */
	  break;
      }
    }
  *time_pos = -1;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCalTimeGroupFirstOpen(KHE_CAL_TIME_GROUP ctg, int *time_pos)     */
/*                                                                           */
/*  If ctg has a first time open to being excluded, set *time_pos to its     */
/*  position and return true.  Otherwise return false.                       */
/*                                                                           */
/*****************************************************************************/

static bool KheCalTimeGroupFirstOpen(KHE_CAL_TIME_GROUP ctg, int *time_pos)
{
  int i;  KHE_CAL_STATE state;
  if( KheCalVertexIsOpen(ctg->vertex, 1) && ctg->avail_durn > 0 )
    for( i = 0;  i < HaArrayCount(ctg->states);  i++ )
    {
      state = HaArray(ctg->states, i);
      switch( state )
      {
	case KHE_CAL_OPEN:

	  *time_pos = i;
	  return true;

	case KHE_CAL_CLOSED_NOT_EXCLUDED:

	  /* can't keep searching past this point */
	  *time_pos = -1;
	  return false;

	case KHE_CAL_CLOSED_EXCLUDED:

	  /* OK to keep searching past this point */
	  break;
      }
    }
  *time_pos = -1;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalTimeGroupChangeTimeState(KHE_CAL_TIME_GROUP ctg,              */
/*    int time_pos, KHE_CAL_STATE state)                                     */
/*                                                                           */
/*  Change the state of the time at position time_pos within ctg's time      */
/*  group to state.                                                          */
/*                                                                           */
/*****************************************************************************/

static void KheCalTimeGroupChangeTimeState(KHE_CAL_TIME_GROUP ctg,
  int time_pos, KHE_CAL_STATE state)
{
  HnAssert(HaArray(ctg->states, time_pos) == KHE_CAL_OPEN,
    "KheCalTimeGroupChangeState internal error 1");
  HaArrayPut(ctg->states, time_pos, state);
  if( state == KHE_CAL_CLOSED_EXCLUDED )
    ctg->avail_durn--, ctg->vertex->avail_durn--;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalTimeGroupChangeWholeState(KHE_CAL_TIME_GROUP ctg,             */
/*    KHE_CAL_STATE state)                                                   */
/*                                                                           */
/*  Change the state of the whole time group.                                */
/*                                                                           */
/*****************************************************************************/

static void KheCalTimeGroupChangeWholeState(KHE_CAL_TIME_GROUP ctg,
  KHE_CAL_STATE state)
{
  int pos;
  for( pos = 0;  pos < KheTimeGroupTimeCount(ctg->time_group);  pos++ )
    KheCalTimeGroupChangeTimeState(ctg, pos, state);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCalTimeGroupAvailableDurn(KHE_CAL_TIME_GROUP ctg)                 */
/*                                                                           */
/*  Return the number of available (open or not excluded) times in ctg.      */
/*                                                                           */
/*****************************************************************************/

static int KheCalTimeGroupAvailableDurn(KHE_CAL_TIME_GROUP ctg)
{
  return ctg->avail_durn;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalTimeGroupDebug(KHE_CAL_TIME_GROUP ctg, int verbosity,         */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of ctg onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheCalTimeGroupDebug(KHE_CAL_TIME_GROUP ctg, int verbosity,
  int indent, FILE *fp)
{
  int i;  KHE_CAL_STATE state;
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    HaArrayForEach(ctg->states, state, i)
      fprintf(fp, "%c", KheCalStateShow(state));
    fprintf(fp, "(avail %d): ", ctg->avail_durn);
    KheTimeGroupDebug(ctg->time_group, 1, -1, fp);
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "cal cluster monitors"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CAL_CLUSTER_MONITOR KheCalClusterMonitorMake(KHE_CAL_VERTEX cv,      */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m)                                      */
/*                                                                           */
/*  Make an cal cluster monitor with these attributes.                       */
/*                                                                           */
/*****************************************************************************/

static KHE_CAL_CLUSTER_MONITOR KheCalClusterMonitorMake(KHE_CAL_VERTEX cv,
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, HA_ARENA a)
{
  KHE_CAL_CLUSTER_MONITOR res;  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c;
  HaMake(res, a);
  res->vertex = cv;
  res->monitor = m;
  c = KheClusterBusyTimesMonitorConstraint(m);
  res->monitor_maximum = KheClusterBusyTimesConstraintMaximum(c);
  HaArrayInit(res->time_groups, a);
  res->exclusions_wanted = KheClusterBusyTimesConstraintTimeGroupCount(c) -
    res->monitor_maximum;
  res->excluded_time_groups = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalClusterMonitorAddTimeGroup(KHE_CAL_CLUSTER_MONITOR ccm,       */
/*    KHE_CAL_TIME_GROUP ctg)                                                */
/*                                                                           */
/*  Add a time group to ccm.                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheCalClusterMonitorAddTimeGroup(KHE_CAL_CLUSTER_MONITOR ccm,
  KHE_CAL_TIME_GROUP ctg)
{
  HaArrayAddLast(ccm->time_groups, ctg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalClusterMonitorDelete(KHE_CAL_CLUSTER_MONITOR ccm)             */
/*                                                                           */
/*  Delete ccm.                                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheCalClusterMonitorDelete(KHE_CAL_CLUSTER_MONITOR ccm)
{
  MArrayFree(ccm->time_groups);
  MFree(ccm);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheCalClusterMonitorIsOpen(KHE_CAL_CLUSTER_MONITOR ccm)             */
/*                                                                           */
/*  Return true if ccm is open:  if it would benefit from the exclusion of   */
/*  one of its time groups, because it has not yet reached its limit.        */
/*                                                                           */
/*****************************************************************************/

static bool KheCalClusterMonitorIsOpen(KHE_CAL_CLUSTER_MONITOR ccm)
{
  return ccm->excluded_time_groups < ccm->exclusions_wanted;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "cal idle monitors"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CAL_IDLE_MONITOR KheCalIdleMonitorMake(KHE_CAL_VERTEX cv,            */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Make an cal idle monitor with these attributes.                          */
/*                                                                           */
/*****************************************************************************/

static KHE_CAL_IDLE_MONITOR KheCalIdleMonitorMake(KHE_CAL_VERTEX cv,
  KHE_LIMIT_IDLE_TIMES_MONITOR m, HA_ARENA a)
{
  KHE_CAL_IDLE_MONITOR res;
  HaMake(res, a);
  res->vertex = cv;
  res->monitor = m;
  res->monitor_maximum = KheLimitIdleTimesConstraintMaximum(
    KheLimitIdleTimesMonitorConstraint(m));
  HaArrayInit(res->time_groups, a);
  res->priqueue_key = 0;
  res->priqueue_index = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalIdleMonitorAddTimeGroup(KHE_CAL_IDLE_MONITOR cim,             */
/*    KHE_CAL_TIME_GROUP ctg)                                                */
/*                                                                           */
/*  Add a time group to cim.                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheCalIdleMonitorAddTimeGroup(KHE_CAL_IDLE_MONITOR cim,
  KHE_CAL_TIME_GROUP ctg)
{
  HaArrayAddLast(cim->time_groups, ctg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalIdleMonitorDelete(KHE_CAL_IDLE_MONITOR cim)                   */
/*                                                                           */
/*  Delete cim.                                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheCalIdleMonitorDelete(KHE_CAL_IDLE_MONITOR cim)
{
  MArrayFree(cim->time_groups);
  MFree(cim);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheCalIdleMonitorAvailableDurn(KHE_CAL_IDLE_MONITOR cim)             */
/*                                                                           */
/*  Return the number of times available inside the time groups monitored    */
/*  by cim.                                                                  */
/*                                                                           */
/*****************************************************************************/

static int KheCalIdleMonitorAvailableDurn(KHE_CAL_IDLE_MONITOR cim)
{
  int i, res;  KHE_CAL_TIME_GROUP ctg;
  res = 0;
  HaArrayForEach(cim->time_groups, ctg, i)
    res += KheCalTimeGroupAvailableDurn(ctg);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalIdleMonitorSetPriority(KHE_CAL_IDLE_MONITOR cim)              */
/*                                                                           */
/*  Set the priority of cim, and notify the priority queue.  If it is        */
/*  non-zero, the monitor is open to further reduction.                      */
/*                                                                           */
/*    cim_avail - the number of available times within cim's time groups     */
/*    cim_min_durn - the minimum duration of meets that must lie within cim  */
/*    cim_max_dev - an upper bound on the deviation of this monitor          */
/*                                                                           */
/*****************************************************************************/

static void KheCalIdleMonitorSetPriority(KHE_CAL_IDLE_MONITOR cim)
{
  int cim_max_dev, cim_avail, cim_min_durn;
  if( KheCalVertexIsOpen(cim->vertex, 1) )
  {
    cim_avail = KheCalIdleMonitorAvailableDurn(cim);
    HnAssert(cim_avail <= cim->vertex->avail_durn,
      "KheCalIdleMonitorSetPriority internal error");
    cim_min_durn = cim->vertex->preassigned_durn -
      (cim->vertex->avail_durn - cim_avail);
    if( cim_min_durn < 0 ) cim_min_durn = 0;
    cim_max_dev = cim_avail - cim_min_durn - cim->monitor_maximum;
    if( cim_max_dev < 0 ) cim_max_dev = 0;
    cim->priqueue_key = cim_max_dev * KheMonitorCombinedWeight(
      (KHE_MONITOR) cim->monitor);
  }
  else
    cim->priqueue_key = 0;
  if( cim->priqueue_index > 0 )
    KhePriQueueNotifyKeyChange(cim->vertex->graph->idle_priqueue, (void *) cim);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalIdleMonitorPriQueueKey(void *entry)                           */
/*                                                                           */
/*  Retrieve an idle monitor's priqueue index.                               */
/*                                                                           */
/*  NB largest keys first, so at the last moment we negate the key.          */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheCalIdleMonitorPriQueueKey(void *entry)
{
  KHE_CAL_IDLE_MONITOR cim = (KHE_CAL_IDLE_MONITOR) entry;
  return - cim->priqueue_key;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCalIdleMonitorPriQueueIndexGet(void *entry)                       */
/*                                                                           */
/*  Retrieve an idle monitor's priqueue index.                               */
/*                                                                           */
/*****************************************************************************/

static int KheCalIdleMonitorPriQueueIndexGet(void *entry)
{
  KHE_CAL_IDLE_MONITOR cim = (KHE_CAL_IDLE_MONITOR) entry;
  return cim->priqueue_index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalIdleMonitorPriQueueIndexSet(void *entry, int index)           */
/*                                                                           */
/*  Set an idle monitor's priqueue index.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheCalIdleMonitorPriQueueIndexSet(void *entry, int index)
{
  KHE_CAL_IDLE_MONITOR cim = (KHE_CAL_IDLE_MONITOR) entry;
  cim->priqueue_index = index;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCalVertexTrySingletonExclusion(KHE_CAL_VERTEX cv,                */
/*    KHE_CAL_TIME_GROUP ctg, int first_or_last)                             */
/*                                                                           */
/*  Try excluding the first or last time of ctg.  Return true if successful. */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheCalVertexTrySingletonExclusion(KHE_CAL_VERTEX cv,
  KHE_CAL_TIME_GROUP ctg, KHE_TIME time)
{
  int init_defect_count, i, j, junk;  KHE_MARK mark;  KHE_TASK task;
  KHE_MEET meet;  KHE_TIME_GROUP tg;  KHE_MEET_BOUND mb;
  KHE_SOLN soln;  KHE_CAL_IDLE_MONITOR cim;  KHE_RESOURCE r;

  ** try the exlusion **
  if( DEBUG3 )
  {
    fprintf(stderr, "[ TrySingletonExclusion(");
    KheResourceGroupDebug(cv->resource_group, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheTimeGroupDebug(ctg->time_group, 1, -1, stderr);
    fprintf(stderr, ", %s)\n", KheTimeId(time));
  }
  soln = cv->graph->soln;
  init_defect_count = KheSolnMatchingDefectCount(soln);
  KheSolnTi meGroupBegin(soln);
  KheSolnTimeGroupUnion(soln, KheInstanceFullTimeGroup(KheSolnInstance(soln)));
  KheSolnTimeGroupSubTime(soln, time);
  tg = KheSolnTimeGroupEnd(soln);
  mark = KheMarkBegin(soln);
  mb = KheMeetBoundMake(soln, true, tg);
  r = KheResourceGroupResource(cv->resource_group, 0);
  for( i = 0;  i < KheResourceAssignedTaskCount(soln, r);  i++ )
  {
    task = KheResourceAssignedTask(soln, r, i);
    meet = KheMeetFirstMovable(KheTaskMeet(task), &junk);
    if( meet == NULL || !KheMeetAddMeetBound(meet, mb) ||
	KheSolnMatchingDefectCount(soln) != init_defect_count )
      break;
  }
  if( DEBUG5 )
    fprintf(stderr, "  idle %s %s: %s\n", KheResourceId(r),
      KheTimeName(time), i >= KheResourceAssignedTaskCount(soln, r) ?
      "excluded" : "failed to exclude");

  if( i >= KheResourceAssignedTaskCount(soln, r) )
  {
    ** success; end mark without undoing (so mb is still defined) **
    KheMarkEnd(mark, false);

    ** change the state of ctg and its vertex, including updating priorities **
    ** KheCalTimeGroupExcludeExtremeTime(ctg, first_or_last); still to do **
    HaArrayForEach(cv->idle_monitors, cim, j)
      KheCalIdleMonitorSetPriority(cim);

    ** add the new bound to the meet bound group, if any **
    if( cv->graph->meet_bound_group != NULL )
      KheMeetBoundGroupAddMeetBound(cv->graph->meet_bound_group, mb);
    if( DEBUG3 )
      fprintf(stderr, "] TrySingletonExclusion returning true\n");
    return true;
  }
  else
  {
    ** failure; end mark with undoing (so mb is undefined) **
    KheMarkEnd(mark, true);
    if( DEBUG3 )
      fprintf(stderr, "] TrySingletonExclusion returning false\n");
    return false;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCalIdleMonitorTryExclusion(KHE_CAL_TIME_GROUP ctg, int pos)      */
/*                                                                           */
/*  Try excluding the time at position pos in ctg.                           */
/*                                                                           */
/*****************************************************************************/
static bool KheCalVertexTryExclusion(KHE_CAL_VERTEX cv, KHE_TIME_GROUP tg);
static bool KheCalVertexTimeIsFirstOrLastOpen(KHE_CAL_VERTEX cv,
  KHE_TIME time, KHE_CAL_TIME_GROUP *ctg, int *time_pos);

static void KheCalIdleMonitorTryExclusion(KHE_CAL_TIME_GROUP ctg, int pos)
{
  KHE_TIME time;  KHE_CAL_IDLE_MONITOR cim;  int i, time_pos;
  KHE_CAL_EDGE ce;  KHE_CAL_VERTEX cv;  KHE_CAL_TIME_GROUP ctg2;
  time = KheTimeGroupTime(ctg->time_group, pos);
  if( KheCalVertexTryExclusion(ctg->vertex, KheTimeSingletonTimeGroup(time)) )
  {
    /* success */
    KheCalTimeGroupChangeTimeState(ctg, pos, KHE_CAL_CLOSED_EXCLUDED);
    HaArrayForEach(ctg->idle_monitors, cim, i)
      KheCalIdleMonitorSetPriority(cim);

    /* propagate this trial over positive edges */
    HaArrayForEach(ctg->vertex->edges, ce, i)
      if( ce->positive_cost > 0 )
      {
	cv = ce->endpoint;
	if( KheCalVertexTimeIsFirstOrLastOpen(cv, time, &ctg2, &time_pos) )
	{
	  if( DEBUG3 )
	    fprintf(stderr, "  --(+%d)-->", ce->positive_cost);
	  KheCalIdleMonitorTryExclusion(ctg2, time_pos);
	}
      }
  }
  else
  {
    /* failure */
    KheCalTimeGroupChangeTimeState(ctg, pos, KHE_CAL_CLOSED_NOT_EXCLUDED);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalIdleMonitorTryExclusions(KHE_CAL_IDLE_MONITOR cim, int shift) */
/*                                                                           */
/*  Try exclusions for cim, using shift for diversification.                 */
/*                                                                           */
/*****************************************************************************/

static void KheCalIdleMonitorTryExclusions(KHE_CAL_IDLE_MONITOR cim, int shift)
{
  KHE_CAL_TIME_GROUP ctg;  int i, pos, index;  bool progressing;
  /* ***
  if( DEBUG6 )
  {
    fprintf(stderr, "  idle ");
    KheResourceGroupDebug(cim->vertex->resource_group, 1, -1, stderr);
    fprintf(stderr, " (%.5f)\n", KheCostShow(cim->priqueue_key));
  }
  *** */

  /* try removing last times */
  progressing = true;
  while( progressing && cim->priqueue_key > 0 )
  {
    progressing = false;
    for( i = 0;  i < HaArrayCount(cim->time_groups);  i++ )
    {
      index = (i + shift) % HaArrayCount(cim->time_groups);
      ctg = HaArray(cim->time_groups, index);
      if( KheCalTimeGroupLastOpen(ctg, &pos) )
      {
        KheCalIdleMonitorTryExclusion(ctg, pos);
	progressing = true;
      }
    }
  }

  /* try removing first times */
  progressing = true;
  while( progressing && cim->priqueue_key > 0 )
  {
    progressing = false;
    for( i = 0;  i < HaArrayCount(cim->time_groups);  i++ )
    {
      index = (i + shift) % HaArrayCount(cim->time_groups);
      ctg = HaArray(cim->time_groups, index);
      if( KheCalTimeGroupFirstOpen(ctg, &pos) )
      {
        KheCalIdleMonitorTryExclusion(ctg, pos);
	progressing = true;
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "cal graph vertices"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheCalVertexContainsTimeGroup(KHE_CAL_VERTEX cv,                    */
/*    KHE_TIME_GROUP tg, KHE_CAL_TIME_GROUP *ctg)                            */
/*                                                                           */
/*  If cv contains tg, return true and set *ctg to its cal time group,       */
/*  otherwise return false.                                                  */
/*                                                                           */
/*****************************************************************************/

static bool KheCalVertexContainsTimeGroup(KHE_CAL_VERTEX cv,
  KHE_TIME_GROUP tg, KHE_CAL_TIME_GROUP *ctg)
{
  int i;  KHE_CAL_TIME_GROUP ctg2;
  HaArrayForEach(cv->time_groups, ctg2, i)
    if( KheTimeGroupEqual(ctg2->time_group, tg) )
    {
      *ctg = ctg2;
      return true;
    }
  *ctg = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCalVertexClusterMonitorIsCompatible(KHE_CAL_VERTEX cv,           */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm)                                   */
/*                                                                           */
/*  Return true if cbtm is compatible with cv, that is, if                   */
/*                                                                           */
/*    * Its weight is at least cg->min_cluster_weight                        */
/*                                                                           */
/*    * It would benefit from at least one exclusion                         */
/*                                                                           */
/*    * Each of its time groups is either equal to or disjoint from each     */
/*      of the existing time groups of cv.                                   */
/*                                                                           */
/*****************************************************************************/

static bool KheCalVertexClusterMonitorIsCompatible(KHE_CAL_VERTEX cv,
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm)
{
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;  KHE_TIME_GROUP tg;  int i, j, offset;
  KHE_CAL_TIME_GROUP ctg;  KHE_COST combined_weight;  KHE_RESOURCE r;
  KHE_POLARITY po;

  /* check weight */
  cbtc = KheClusterBusyTimesMonitorConstraint(cbtm);
  offset = KheClusterBusyTimesMonitorOffset(cbtm);
  combined_weight = KheMonitorCombinedWeight((KHE_MONITOR) cbtm);
  if( combined_weight == 0 || combined_weight < cv->graph->min_cluster_weight )
    return false;

  /* check would benefit from exclusions */
  if( KheClusterBusyTimesConstraintTimeGroupCount(cbtc) <=
      KheClusterBusyTimesConstraintMaximum(cbtc) )
    return false;
  r = KheResourceGroupResource(cv->resource_group, 0);
  if( KheResourceAssignedTaskCount(cv->graph->soln, r) <=
      KheClusterBusyTimesConstraintMaximum(cbtc) )
    return false;

  /* check time groups */
  for( i = 0;  i < KheClusterBusyTimesConstraintTimeGroupCount(cbtc);  i++ )
  {
    tg = KheClusterBusyTimesConstraintTimeGroup(cbtc, i, offset, &po);
    HaArrayForEach(cv->time_groups, ctg, j)
      if( !KheCalTimeGroupIsCompatible(ctg, tg) )
	return false;
  }

  /* all checks passed */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalVertexMakeAndAddClusterMonitor(KHE_CAL_VERTEX cv,             */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm)                                   */
/*                                                                           */
/*  Update cv to include cbtm, both in the cluster monitor list and in       */
/*  the time group list.                                                     */
/*                                                                           */
/*****************************************************************************/

static void KheCalVertexMakeAndAddClusterMonitor(KHE_CAL_VERTEX cv,
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm, HA_ARENA a)
{
  KHE_CAL_CLUSTER_MONITOR ccm;  KHE_CAL_TIME_GROUP ctg;  KHE_TIME_GROUP tg;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c;  int i, offset;  KHE_POLARITY po;

  /* build and add the basic cluster monitor object */
  ccm = KheCalClusterMonitorMake(cv, cbtm, a);
  HaArrayAddLast(cv->cluster_monitors, ccm);

  /* add cal time groups as required */
  c = KheClusterBusyTimesMonitorConstraint(cbtm);
  offset = KheClusterBusyTimesMonitorOffset(cbtm);
  for( i = 0;  i < KheClusterBusyTimesConstraintTimeGroupCount(c);  i++ )
  {
    tg = KheClusterBusyTimesConstraintTimeGroup(c, i, offset, &po);
    if( !KheCalVertexContainsTimeGroup(cv, tg, &ctg) )
    {
      ctg = KheCalTimeGroupMake(cv, tg, a);
      HaArrayAddLast(cv->time_groups, ctg);
    }
    KheCalClusterMonitorAddTimeGroup(ccm, ctg);
    KheCalTimeGroupAddClusterMonitor(ctg, ccm);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCalVertexIdleMonitorIsCompatible(KHE_CAL_VERTEX cv,              */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR litm)                                     */
/*                                                                           */
/*  Return true if litm is compatible with cv, that is, if                   */
/*                                                                           */
/*    * Its weight is at least cg->min_idle_weight                           */
/*                                                                           */
/*    * Its resource's resource type's demand is all preassigned             */
/*                                                                           */
/*    * Its time groups are disjoint                                         */
/*                                                                           */
/*    * Each of its time groups is either equal to or disjoint from each     */
/*      of the existing time groups of cv.                                   */
/*                                                                           */
/*****************************************************************************/

static bool KheCalVertexIdleMonitorIsCompatible(KHE_CAL_VERTEX cv,
  KHE_LIMIT_IDLE_TIMES_MONITOR litm)
{
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT litc;  KHE_TIME_GROUP tg;  int i, j;
  KHE_CAL_TIME_GROUP ctg;  KHE_COST combined_weight;  KHE_RESOURCE_TYPE rt;
  KHE_RESOURCE r;

  /* check weight */
  litc = KheLimitIdleTimesMonitorConstraint(litm);
  combined_weight = KheMonitorCombinedWeight((KHE_MONITOR) litm);
  if( combined_weight == 0 || combined_weight < cv->graph->min_idle_weight )
    return false;

  /* check resource type */
  rt = KheResourceGroupResourceType(cv->resource_group);
  if( !KheResourceTypeDemandIsAllPreassigned(rt) )
    return false;

  /* must have at least two tasks */
  r = KheResourceGroupResource(cv->resource_group, 0);
  if( KheResourceAssignedTaskCount(cv->graph->soln, r) <= 1 )
    return false;

  /* check time groups disjoint */
  if( !KheLimitIdleTimesConstraintTimeGroupsDisjoint(litc) )
    return false;

  /* check time groups wrt existing time groups */
  for( i = 0;  i < KheLimitIdleTimesConstraintTimeGroupCount(litc);  i++ )
  {
    tg = KheLimitIdleTimesConstraintTimeGroup(litc, i);
    HaArrayForEach(cv->time_groups, ctg, j)
      if( !KheCalTimeGroupIsCompatible(ctg, tg) )
	return false;
  }

  /* all checks passed */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalVertexMakeAndAddIdleMonitor(KHE_CAL_VERTEX cv,                */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR litm)                                     */
/*                                                                           */
/*  Update cv to include litm, both in the idle monitor list and in          */
/*  the time group list.                                                     */
/*                                                                           */
/*****************************************************************************/

static void KheCalVertexMakeAndAddIdleMonitor(KHE_CAL_VERTEX cv,
  KHE_LIMIT_IDLE_TIMES_MONITOR litm, HA_ARENA a)
{
  KHE_CAL_IDLE_MONITOR cim;  KHE_CAL_TIME_GROUP ctg;  KHE_TIME_GROUP tg;
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c;  int i;

  /* build and add the basic idle monitor object */
  cim = KheCalIdleMonitorMake(cv, litm, a);
  HaArrayAddLast(cv->idle_monitors, cim);

  /* add cal time groups as required */
  c = KheLimitIdleTimesMonitorConstraint(litm);
  for( i = 0;  i < KheLimitIdleTimesConstraintTimeGroupCount(c);  i++ )
  {
    tg = KheLimitIdleTimesConstraintTimeGroup(c, i);
    if( !KheCalVertexContainsTimeGroup(cv, tg, &ctg) )
    {
      ctg = KheCalTimeGroupMake(cv, tg, a);
      HaArrayAddLast(cv->time_groups, ctg);
    }
    KheCalIdleMonitorAddTimeGroup(cim, ctg);
    KheCalTimeGroupAddIdleMonitor(ctg, cim);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CAL_VERTEX KheCalVertexMake(KHE_CAL_GRAPH cg, KHE_RESOURCE_GROUP rg) */
/*                                                                           */
/*  Make and return a vertex for time-equivalent resource group rg,          */
/*  including monitors and time groups.                                      */
/*                                                                           */
/*****************************************************************************/

static KHE_CAL_VERTEX KheCalVertexMake(KHE_CAL_GRAPH cg, KHE_RESOURCE_GROUP rg)
{
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;  KHE_CAL_VERTEX res;  KHE_RESOURCE r;
  KHE_LIMIT_IDLE_TIMES_MONITOR litm;  KHE_MONITOR m;  int i, j;  KHE_TASK task;

  /* make the basic object */
  HaMake(res, cg->arena);
  res->graph = cg;
  res->resource_group = rg;
  res->preassigned_durn = 0;
  res->avail_durn = KheInstanceTimeCount(KheSolnInstance(cg->soln));
  HaArrayInit(res->edges, cg->arena);
  HaArrayInit(res->cluster_monitors, cg->arena);
  HaArrayInit(res->idle_monitors, cg->arena);
  HaArrayInit(res->time_groups, cg->arena);
  /* res->open_time_groups = 0; */
  res->excluded_time_groups = 0;
  res->priqueue_key = 0;
  res->priqueue_index = 0;

  /* add in preassigned duration (using any one resource from rg) */
  r = KheResourceGroupResource(rg, 0);
  for( i = 0;  i < KheResourceAssignedTaskCount(cg->soln, r);  i++ )
  {
    task = KheResourceAssignedTask(cg->soln, r, i);
    res->preassigned_durn += KheTaskDuration(task);
  }

  /* add compatible cluster and idle times monitors and their time groups */
  for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
  {
    r = KheResourceGroupResource(rg, i);
    for( j = 0;  j < KheSolnResourceMonitorCount(cg->soln, r);  j++ )
    {
      m = KheSolnResourceMonitor(cg->soln, r, j);
      if( KheMonitorTag(m) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG )
      {
	cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
	if( KheCalVertexClusterMonitorIsCompatible(res, cbtm) )
	  KheCalVertexMakeAndAddClusterMonitor(res, cbtm, cg->arena);
      }
      else if( KheMonitorTag(m) == KHE_LIMIT_IDLE_TIMES_MONITOR_TAG )
      {
	litm = (KHE_LIMIT_IDLE_TIMES_MONITOR) m;
	if( KheCalVertexIdleMonitorIsCompatible(res, litm) )
	  KheCalVertexMakeAndAddIdleMonitor(res, litm, cg->arena);
      }
    }
  }
  /* res->open_time_groups = HaArrayCount(res->time_groups); */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCalVertexIsConnected(KHE_CAL_VERTEX cv,                          */
/*    KHE_CAL_VERTEX endpoint, KHE_CAL_EDGE *ce)                             */
/*                                                                           */
/*  If there is an edge from cv to endpoint, set *ce to that edge and        */
/*  return true.  Otherwise return false.                                    */
/*                                                                           */
/*****************************************************************************/

static bool KheCalVertexIsConnected(KHE_CAL_VERTEX cv,
  KHE_CAL_VERTEX endpoint, KHE_CAL_EDGE *ce)
{
  KHE_CAL_EDGE ce2;  int i;
  HaArrayForEach(cv->edges, ce2, i)
    if( ce2->endpoint == endpoint )
    {
      *ce = ce2;
      return true;
    }
  *ce = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalVertexAddPositiveCost(KHE_CAL_VERTEX cv,                      */
/*    KHE_CAL_VERTEX endpoint, int positive_cost)                            */
/*                                                                           */
/*  Add positive cost to the edge from cv to endpoint.  If there is no       */
/*  such edge, create one first.                                             */
/*                                                                           */
/*****************************************************************************/
static KHE_CAL_EDGE KheCalEdgeMake(KHE_CAL_VERTEX endpoint, HA_ARENA a);

static void KheCalVertexAddPositiveCost(KHE_CAL_VERTEX cv,
  KHE_CAL_VERTEX endpoint, int positive_cost, HA_ARENA a)
{
  KHE_CAL_EDGE ce;
  if( !KheCalVertexIsConnected(cv, endpoint, &ce) )
  {
    ce = KheCalEdgeMake(endpoint, a);
    HaArrayAddLast(cv->edges, ce);
    cv->priqueue_key--;
  }
  ce->positive_cost += positive_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalVertexAddNegativeCost(KHE_CAL_VERTEX cv,                      */
/*    KHE_CAL_VERTEX endpoint, int negative_cost)                            */
/*                                                                           */
/*  Add negative cost to the edge from cv to endpoint.  If there is no       */
/*  such edge, create one first.  We actually max it in, not add it in.      */
/*                                                                           */
/*****************************************************************************/

static void KheCalVertexAddNegativeCost(KHE_CAL_VERTEX cv,
  KHE_CAL_VERTEX endpoint, int negative_cost, HA_ARENA a)
{
  KHE_CAL_EDGE ce;
  if( !KheCalVertexIsConnected(cv, endpoint, &ce) )
  {
    ce = KheCalEdgeMake(endpoint, a);
    HaArrayAddLast(cv->edges, ce);
    cv->priqueue_key--;
  }
  if( negative_cost > ce->negative_cost )
    ce->negative_cost = negative_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalVertexDelete(KHE_CAL_VERTEX cv)                               */
/*                                                                           */
/*  Delete cv, including deleting its edges.                                 */
/*                                                                           */
/*****************************************************************************/
/* ***
static void KheCalEdgeDelete(KHE_CAL_EDGE ce);

static void KheCalVertexDelete(KHE_CAL_VERTEX cv)
{
  while( HaArrayCount(cv->cluster_monitors) > 0 )
    KheCalClusterMonitorDelete(MArrayRemoveLast(cv->cluster_monitors));
  MArrayFree(cv->cluster_monitors);
  while( HaArrayCount(cv->idle_monitors) > 0 )
    KheCalIdleMonitorDelete(MArrayRemoveLast(cv->idle_monitors));
  MArrayFree(cv->idle_monitors);
  while( HaArrayCount(cv->time_groups) > 0 )
    KheCalTimeGroupDelete(MArrayRemoveLast(cv->time_groups));
  MArrayFree(cv->time_groups);
  while( HaArrayCount(cv->edges) > 0 )
    KheCalEdgeDelete(MArrayRemoveLast(cv->edges));
  MArrayFree(cv->edges);
  MFree(cv);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCalVertexPriQueueKey(void *entry)                                */
/*                                                                           */
/*  Retrieve a vertex's priqueue key.  This is initially the negative of the */
/*  degree of the vertex, then it gets smaller as neighbours are coloured.   */
/*                                                                           */
/*****************************************************************************/

int64_t KheCalVertexPriQueueKey(void *entry)
{
  KHE_CAL_VERTEX cv = (KHE_CAL_VERTEX) entry;
  return (int64_t) cv->priqueue_key;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalVertexChangePriQueueKey(KHE_CAL_VERTEX cv, int cost)          */
/*                                                                           */
/*  Change the priqueue key of cv by the given amount; and if cv is          */
/*  currently in the priority queue, change its position there.              */
/*                                                                           */
/*****************************************************************************/

static void KheCalVertexChangePriQueueKey(KHE_CAL_VERTEX cv, int cost)
{
  cv->priqueue_key += cost;
  if( cv->priqueue_index > 0 )
    KhePriQueueNotifyKeyChange(cv->graph->cluster_priqueue, (void *) cv);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCalVertexPriQueueIndexGet(void *entry)                            */
/*                                                                           */
/*  Retrieve a vertex's priqueue index.                                      */
/*                                                                           */
/*****************************************************************************/

static int KheCalVertexPriQueueIndexGet(void *entry)
{
  KHE_CAL_VERTEX cv = (KHE_CAL_VERTEX) entry;
  return cv->priqueue_index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalVertexPriQueueIndexSet(void *entry, int index)                */
/*                                                                           */
/*  Set a vertex's priqueue index.                                           */
/*                                                                           */
/*****************************************************************************/

static void KheCalVertexPriQueueIndexSet(void *entry, int index)
{
  KHE_CAL_VERTEX cv = (KHE_CAL_VERTEX) entry;
  cv->priqueue_index = index;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCalVertexIsOpen(KHE_CAL_VERTEX cv, int durn)                     */
/*                                                                           */
/*  Return true if cv is open to an exclusion which would reduce the         */
/*  available duration by durn.                                              */
/*                                                                           */
/*****************************************************************************/

static bool KheCalVertexIsOpen(KHE_CAL_VERTEX cv, int durn)
{
  return cv->avail_durn - durn >= cv->graph->slack * cv->preassigned_durn;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCalVertexTimeIsFirstOrLastOpen(KHE_CAL_VERTEX cv,                */
/*    KHE_TIME time, KHE_CAL_TIME_GROUP *ctg, int *time_pos)                 */
/*                                                                           */
/*  Return true if time is an open first or last time in cv, setting *ctg    */
/*  to its time group and *pos to its position in that case.                 */
/*                                                                           */
/*****************************************************************************/

static bool KheCalVertexTimeIsFirstOrLastOpen(KHE_CAL_VERTEX cv,
  KHE_TIME time, KHE_CAL_TIME_GROUP *ctg, int *time_pos)
{
  int i, pos;  KHE_TIME_GROUP tg;
  HaArrayForEach(cv->time_groups, *ctg, i)
  {
    tg = (*ctg)->time_group;
    if( KheTimeGroupContains(tg, time, &pos) )
    {
      if( KheCalTimeGroupLastOpen(*ctg, time_pos) &&
	  KheTimeGroupTime(tg, *time_pos) == time )
	return true;
      else if( KheCalTimeGroupFirstOpen(*ctg, time_pos) &&
	  KheTimeGroupTime(tg, *time_pos) == time )
	return true;
      else
	return false;
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCalVertexTryExclusion(KHE_CAL_VERTEX cv, KHE_TIME_GROUP tg)      */
/*                                                                           */
/*  Try excluding tg from cv, and return true if successful.                 */
/*                                                                           */
/*****************************************************************************/

static bool KheCalVertexTryExclusion(KHE_CAL_VERTEX cv, KHE_TIME_GROUP tg)
{
  int init_defect_count, i, junk;  KHE_MARK mark;  KHE_TASK task; KHE_SOLN soln;
  KHE_MEET meet;  KHE_TIME_GROUP compl_tg;  KHE_MEET_BOUND mb;  KHE_RESOURCE r;

  /* try the exlusion */
  if( DEBUG3 )
  {
    fprintf(stderr, "  TryExclusion(");
    KheResourceGroupDebug(cv->resource_group, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheTimeGroupDebug(tg, 1, -1, stderr);
    fprintf(stderr, ")");
  }
  soln = cv->graph->soln;
  init_defect_count = KheSolnMatchingDefectCount(soln);
  KheSolnTimeGroupBegin(soln);
  KheSolnTimeGroupUnion(soln, KheInstanceFullTimeGroup(KheSolnInstance(soln)));
  KheSolnTimeGroupDifference(soln, tg);
  compl_tg = KheSolnTimeGroupEnd(soln);
  mark = KheMarkBegin(soln);
  mb = KheMeetBoundMake(soln, true, compl_tg);
  r = KheResourceGroupResource(cv->resource_group, 0);
  for( i = 0;  i < KheResourceAssignedTaskCount(soln, r);  i++ )
  {
    task = KheResourceAssignedTask(soln, r, i);
    meet = KheMeetFirstMovable(KheTaskMeet(task), &junk);
    if( meet == NULL || !KheMeetAddMeetBound(meet, mb) ||
	KheSolnMatchingDefectCount(soln) != init_defect_count )
    {
      /* failure; end mark with undoing (so mb is undefined) */
      KheMarkEnd(mark, true);
      if( DEBUG3 )
	fprintf(stderr, " = false\n");
      return false;
    }
  }

  /* success */
  KheMarkEnd(mark, false);
  if( cv->graph->meet_bound_group != NULL )
    KheMeetBoundGroupAddMeetBound(cv->graph->meet_bound_group, mb);
  if( DEBUG3 )
    fprintf(stderr, " = true\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCalVertexTryClusterExclusion(KHE_CAL_VERTEX cv,                  */
/*    KHE_CAL_TIME_GROUP ctg)                                                */
/*                                                                           */
/*  As part of cluster handling, try excluding ctg from cv.  Return true     */
/*  if successful.                                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheCalVertexTryClusterExclusion(KHE_CAL_VERTEX cv,
  KHE_CAL_TIME_GROUP ctg)
{
  int i;  KHE_CAL_CLUSTER_MONITOR ccm;  KHE_CAL_EDGE ce;

  /* try the exlusion */
  HnAssert(HaArrayCount(ctg->states) > 0 &&
    HaArrayFirst(ctg->states) == KHE_CAL_OPEN,
    "KheCalVertexTryClusterExclusion internal error");
  if( KheCalVertexTryExclusion(cv, ctg->time_group) )
  {
    /* change the state of cv and its monitors */
    KheCalTimeGroupChangeWholeState(ctg, KHE_CAL_CLOSED_EXCLUDED);
    cv->excluded_time_groups++;
    HaArrayForEach(ctg->cluster_monitors, ccm, i)
      ccm->excluded_time_groups++;

    /* update the priorities of cv's neighbours, if first colour */
    if( cv->excluded_time_groups == 1 )
      HaArrayForEach(cv->edges, ce, i)
	KheCalVertexChangePriQueueKey(ce->endpoint,
	  -4 * (ce->positive_cost + ce->negative_cost));

    /* update cv's own priority */
    KheCalVertexChangePriQueueKey(cv, -100);
    return true;
  }
  else
  {
    /* change the state of cv and its monitors */
    KheCalTimeGroupChangeWholeState(ctg, KHE_CAL_CLOSED_NOT_EXCLUDED);
    return false;
  }
}


/* ***
static bool KheCalVertexTryClusterExclusion(KHE_CAL_VERTEX cv, KHE_CAL_TIME_GROUP ctg)
{
  int init_defect_count, i, junk;  KHE_MARK mark;  KHE_TASK task;
  KHE_MEET meet;  KHE_TIME_GROUP tg;  KHE_MEET_BOUND mb;  KHE_RESOURCE r;
  KHE_CAL_CLUSTER_MONITOR ccm;  KHE_SOLN soln;  KHE_CAL_EDGE ce;

  ** try the exlusion **
  if( DEBUG3 )
  {
    fprintf(stderr, "[ TryExclusion(");
    KheResourceGroupDebug(cv->resource_group, 1, -1, stderr);
    fprintf(stderr, ": %d, ", cv->priqueue_key);
    KheTimeGroupDebug(ctg->time_group, 1, -1, stderr);
    fprintf(stderr, ": %d)\n", ctg->cluster_cost);
  }
  HnAssert(HaArrayCount(ctg->states) > 0 && HaArrayFirst(ctg->states)==KHE_CAL_OPEN,
    "KheCalVertexTryClusterExclusion internal error");
  soln = cv->graph->soln;
  init_defect_count = KheSolnMatchingDefectCount(soln);
  KheSolnTime GroupBegin(soln);
  KheSolnTimeGroupUnion(soln, KheInstanceFullTimeGroup(KheSolnInstance(soln)));
  KheSolnTimeGroupDifference(soln, ctg->time_group);
  tg = KheSolnTimeGroupEnd(soln);
  mark = KheMarkBegin(soln);
  mb = KheMeetBoundMake(soln, true, tg);
  r = KheResourceGroupResource(cv->resource_group, 0);
  for( i = 0;  i < KheResourceAssignedTaskCount(soln, r);  i++ )
  {
    task = KheResourceAssignedTask(soln, r, i);
    meet = KheMeetFirstMovable(KheTaskMeet(task), &junk);
    if( meet == NULL || !KheMeetAddMeetBound(meet, mb) ||
	KheSolnMatchingDefectCount(soln) != init_defect_count )
      break;
  }
  if( DEBUG5 )
  {
    fprintf(stderr, "  cluster %s (%d) %s ", KheResourceId(r),
      cv->priqueue_key, i >= KheResourceAssignedTaskCount(soln, r) ?
      "excluded" : "failed to exclude");
    KheTimeGroupDebug(ctg->time_group, 1, -1, stderr);
    fprintf(stderr, " (%d)\n", ctg->cluster_cost);
  }

  if( i >= KheResourceAssignedTaskCount(soln, r) )
  {
    ** success; end mark without undoing (so mb is still defined) **
    KheMarkEnd(mark, false);

    ** change the state of cv and its monitors **
    KheCalTimeGroupChangeWholeState(ctg, KHE_CAL_CLOSED_EXCLUDED);
    cv->open_time_groups--;
    cv->excluded_time_groups++;
    HaArrayForEach(cv->cluster_monitors, ccm, i)
      KheCalClusterMonitorUpdateTimeGroup(ccm, ctg, KHE_CAL_CLOSED_EXCLUDED);

    ** add the new bound to the meet bound group, if any **
    if( cv->graph->meet_bound_group != NULL )
      KheMeetBoundGroupAddMeetBound(cv->graph->meet_bound_group, mb);

    ** update the priorities of cv's neighbours, if first colour **
    if( cv->excluded_time_groups == 1 )
      HaArrayForEach(cv->edges, ce, i)
	KheCalVertexChangePriQueueKey(ce->endpoint,
	  -4 * (ce->positive_cost + ce->negative_cost));

    ** update cv's own priority **
    KheCalVertexChangePriQueueKey(cv, -100);
    if( DEBUG3 )
      fprintf(stderr, "] TryExclusion returning true\n");
    return true;
  }
  else
  {
    ** failure; end mark with undoing (so mb is undefined) **
    KheMarkEnd(mark, true);

    ** change the state of cv and its monitors **
    KheCalTimeGroupChangeWholeState(ctg, KHE_CAL_CLOSED_NOT_EXCLUDED);
    cv->open_time_groups--;
    HaArrayForEach(cv->cluster_monitors, ccm, i)
      KheCalClusterMonitorUpdateTimeGroup(ccm, ctg,
	KHE_CAL_CLOSED_NOT_EXCLUDED);
    if( DEBUG3 )
      fprintf(stderr, "] TryExclusion returning false\n");
    return false;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCalVertexTryUnavailableClusterExclusions(KHE_CAL_VERTEX cv)      */
/*                                                                           */
/*  Try exclusions caused by unavailable times.                              */
/*                                                                           */
/*****************************************************************************/

static void KheCalVertexTryUnavailableClusterExclusions(KHE_CAL_VERTEX cv)
{
  KHE_TIME_GROUP tg;  KHE_CAL_TIME_GROUP ctg;  int i, j;  KHE_RESOURCE r;
  for( i = 0;  i < KheResourceGroupResourceCount(cv->resource_group);  i++ )
  {
    r = KheResourceGroupResource(cv->resource_group, i);
    tg = KheResourceHardAndSoftUnavailableTimeGroup(r);
    HaArrayForEach(cv->time_groups, ctg, j)
      if( KheTimeGroupSubset(ctg->time_group, tg) &&
	  KheCalTimeGroupClusterExclusionIsOpen(ctg) )
	KheCalVertexTryClusterExclusion(cv, ctg);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCalVertexTryOrdinaryClusterExclusions(KHE_CAL_VERTEX cv)         */
/*                                                                           */
/*  Here cv has just been extracted from the priority queue, so sort its     */
/*  time groups by increasing cost and try each in turn until one succeeds   */
/*  or all are tried.  Return true if cv may still have open time groups.    */
/*                                                                           */
/*****************************************************************************/

static bool KheCalVertexTryOrdinaryClusterExclusions(KHE_CAL_VERTEX cv)
{
  KHE_CAL_TIME_GROUP ctg;  int i;

  /* find the cost of each open time group, and sort by increasing cost */
  HaArrayForEach(cv->time_groups, ctg, i)
    KheCalTimeGroupSetClusterCost(ctg);
  HaArraySort(cv->time_groups, &KheCalTimeGroupIncreasingClusterCostCmp);

  /* try each open one in turn until one succeeds or all have been tried */
  HaArrayForEach(cv->time_groups, ctg, i)
    if( KheCalTimeGroupClusterExclusionIsOpen(ctg) &&
	KheCalVertexTryClusterExclusion(cv, ctg) )
      break;

  /* return true if there is still work to do */
  return i < HaArrayCount(cv->time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalVertexDebug(KHE_CAL_VERTEX cv, int verbosity,                 */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of cv onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/
static void KheCalEdgeDebug(KHE_CAL_EDGE ce, int verbosity,
  int indent, FILE *fp);

static void KheCalVertexDebug(KHE_CAL_VERTEX cv, int verbosity,
  int indent, FILE *fp)
{
  int i;  KHE_CAL_TIME_GROUP ctm;  KHE_CAL_EDGE ce;
  if( indent >= 0 && (verbosity >= 2 ||
      (verbosity >= 1 && HaArrayCount(cv->cluster_monitors) > 0)) )
  {
    fprintf(fp, "%*sVertex ", indent, "");
    KheResourceGroupDebug(cv->resource_group, 2, -1, stderr);
    fprintf(fp, " (durn %d, clusters %d, idles %d, key %d):\n",
      cv->preassigned_durn, HaArrayCount(cv->cluster_monitors),
      HaArrayCount(cv->idle_monitors), cv->priqueue_key);
    HaArrayForEach(cv->time_groups, ctm, i)
      KheCalTimeGroupDebug(ctm, verbosity, indent + 2, fp);
    HaArrayForEach(cv->edges, ce, i)
      KheCalEdgeDebug(ce, verbosity, indent + 2, fp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "edges"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CAL_EDGE KheCalEdgeMake(KHE_CAL_VERTEX endpoint)                     */
/*                                                                           */
/*  Make an edge object with these attributes, but don't link it in.         */
/*                                                                           */
/*****************************************************************************/

static KHE_CAL_EDGE KheCalEdgeMake(KHE_CAL_VERTEX endpoint, HA_ARENA a)
{
  KHE_CAL_EDGE res;
  HaMake(res, a);
  res->endpoint = endpoint;
  res->positive_cost = 0;
  res->negative_cost = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalEdgeDelete(KHE_CAL_EDGE ce)                                   */
/*                                                                           */
/*  Delete ce.                                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheCalEdgeDelete(KHE_CAL_EDGE ce)
{
  MFree(ce);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCalEdgeDebug(KHE_CAL_EDGE ce, int verbosity,                     */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of ce onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheCalEdgeDebug(KHE_CAL_EDGE ce, int verbosity,
  int indent, FILE *fp)
{
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s--(", indent, "");
    if( ce->positive_cost > 0 )
      fprintf(fp, "+%d", ce->positive_cost);
    if( ce->negative_cost > 0 )
      fprintf(fp, "-%d", ce->negative_cost);
    fprintf(fp, ")-> ");
    KheResourceGroupDebug(ce->endpoint->resource_group, 1, 0, stderr);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "graph"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheCalGraphAccumulatePreassignedResourceVertices(KHE_CAL_GRAPH cg,  */
/*    KHE_MEET meet, ARRAY_KHE_CAL_VERTEX *vertices)                         */
/*                                                                           */
/*  Accumulate in *vertices the vertices holding the preassigned resources   */
/*  of the tasks of meet and meets assigned to it, directly or indirectly.   */
/*                                                                           */
/*****************************************************************************/

static void KheCalGraphAccumulatePreassignedResourceVertices(KHE_CAL_GRAPH cg,
  KHE_MEET meet, ARRAY_KHE_CAL_VERTEX *vertices)
{
  int i, pos, index;  KHE_TASK task;  KHE_RESOURCE r;  KHE_MEET child_meet;
  KHE_CAL_VERTEX cv;

  /* accumulate preassigned resources from the tasks of meet itself */
  for( i = 0;  i < KheMeetTaskCount(meet);  i++ )
  {
    task = KheMeetTask(meet, i);
    if( KheTaskIsPreassigned(task, &r) )
    {
      index = KheTimeEquivResourceResourceGroupIndex(cg->time_equiv, r);
      cv = HaArray(cg->vertices, index);
      if( !HaArrayContains(*vertices, cv, &pos) )
	HaArrayAddLast(*vertices, cv);
    }
  }

  /* accumulate preassigned resources from the tasks of the children of meet */
  for( i = 0;  i < KheMeetAssignedToCount(meet);  i++ )
  {
    child_meet = KheMeetAssignedTo(meet, i);
    KheCalGraphAccumulatePreassignedResourceVertices(cg, child_meet, vertices);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CAL_GRAPH KheCalGraphMake(KHE_SOLN soln,                             */
/*    KHE_COST min_cluster_weight, KHE_COST min_idle_weight,                 */
/*    float slack, KHE_MEET_BOUND_GROUP mbg, KHE_OPTIONS options)            */
/*                                                                           */
/*  Make a cal graph for soln, including one vertex for each set of          */
/*  time-equivalent resources, and pairs of opposite edges as required.      */
/*                                                                           */
/*****************************************************************************/

static KHE_CAL_GRAPH KheCalGraphMake(KHE_SOLN soln,
  KHE_COST min_cluster_weight, KHE_COST min_idle_weight,
  float slack, KHE_MEET_BOUND_GROUP mbg, KHE_OPTIONS options, HA_ARENA a)
{
  KHE_CAL_GRAPH res;  int i, j, k, durn, negative_cost;  /* HA_ARENA a; */
  KHE_MEET meet;  KHE_CAL_VERTEX cv, cv1, cv2;  KHE_CAL_EDGE ce1, ce2;
  KHE_RESOURCE_GROUP rg;  ARRAY_KHE_CAL_VERTEX vertices;

  /* make the basic object */
  if( DEBUG4 )
    fprintf(stderr, "[ KheCalGraphMake(soln, ...)\n");
  /* a = HaAr enaMake(); */
  HaMake(res, a);
  res->soln = soln;
  res->time_equiv = KheTimeEquivOption(options, "st_time_equiv", soln);
  /* ***
  res->time_equiv =
    (KHE_TIME_EQUIV) KheOptionsGetObject(options, "st_time_equiv", NULL);
  *** */
  /* res->time_equiv = KheOptionsStructuralTimeEquiv(options); */
  res->min_cluster_weight = min_cluster_weight;
  res->min_idle_weight = min_idle_weight,
  res->slack = slack;
  res->meet_bound_group = mbg;
  HaArrayInit(res->vertices, a);
  res->cluster_priqueue = NULL;
  res->idle_priqueue = NULL;

  /* add one vertex for each time-equivalent resource group */
  if( KheInstanceResourceCount(KheSolnInstance(soln)) > 0 )
    HnAssert(KheTimeEquivResourceGroupCount(res->time_equiv) > 0,
      "KheSolnClusterAndLimitMeetDomains: "
      "KheTimeEquivSolve not called previously");
  for( i = 0;  i < KheTimeEquivResourceGroupCount(res->time_equiv);  i++ )
  {
    rg = KheTimeEquivResourceGroup(res->time_equiv, i);
    HaArrayAddLast(res->vertices, KheCalVertexMake(res, rg));
  }

  /* accumulate positive edge costs */
  HaArrayInit(vertices, a);
  for( i = 0;  i < KheSolnMeetCount(soln);  i++ )
  {
    meet = KheSolnMeet(soln, i);
    if( !KheMeetIsCycleMeet(meet) && KheMeetAsst(meet) == NULL )
    {
      durn = KheMeetDuration(meet);
      HaArrayClear(vertices);
      KheCalGraphAccumulatePreassignedResourceVertices(res, meet, &vertices);
      HaArrayForEach(vertices, cv1, j)
	for( k = j + 1;  k < HaArrayCount(vertices);  k++ )
	{
	  cv2 = HaArray(vertices, k);
          KheCalVertexAddPositiveCost(cv1, cv2, durn, a);
          KheCalVertexAddPositiveCost(cv2, cv1, durn, a);
	}
    }
  }
  HaArrayFree(vertices);

  /* accumulate negative edge costs */
  HaArrayForEach(res->vertices, cv, i)
  {
    for( j = 0;  j < HaArrayCount(cv->edges);  j++ )
    {
      ce1 = HaArray(cv->edges, j);
      cv1 = ce1->endpoint;
      if( HaArrayCount(cv1->cluster_monitors) > 0 )
	for( k = j + 1;  k < HaArrayCount(cv->edges);  k++ )
	{
	  ce2 = HaArray(cv->edges, k);
	  cv2 = ce2->endpoint;
	  if( HaArrayCount(cv2->cluster_monitors) > 0 )
	  {
	    negative_cost = ce1->positive_cost + ce2->positive_cost;
	    KheCalVertexAddNegativeCost(cv1, cv2, negative_cost, a);
	    KheCalVertexAddNegativeCost(cv2, cv1, negative_cost, a);
	  }
	}
    }
  }

  /* reduce edges to those to vertices with cluster monitors only */
  /* ***
  HaArrayForEach(res->vertices, cv, i)
    HaArrayForEach(cv->edges, ce, j)
      if( HaArrayCount(ce->endpoint->cluster_monitors) == 0 )
      {
	KheCalEdgeDelete(ce);
	ce = MArrayRemoveAndPlug(cv->edges, j);
	j--;
      }
  *** */

  /* reduce vertices to those with cluster monitors only */
  /* ***
  HaArrayForEach(res->vertices, cv, i)
    if( HaArrayCount(cv->cluster_monitors) == 0 )
    {
      KheCalVertexDelete(cv);
      HaArrayPut(res->vertices, i, NULL);
    }
  *** */

  /* all done */
  if( DEBUG4 )
    fprintf(stderr, "] KheCalGraphMake returning res\n");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCalGraphDelete(KHE_CAL_GRAPH cg)                                 */
/*                                                                           */
/*  Delete cg.                                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheCalGraphDelete(KHE_CAL_GRAPH cg)
{
  HaAre naDelete(cg->arena);
  ** ***
  KHE_CAL_VERTEX cv;  int i;
  HaArrayForEach(cg->vertices, cv, i)
    KheCalVertexDelete(cv);
  MArrayFree(cg->vertices);
  MFree(cg);
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCalGraphDebug(KHE_CAL_GRAPH cg, int verbosity,                   */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of cg onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheCalGraphDebug(KHE_CAL_GRAPH cg, int verbosity,
  int indent, FILE *fp)
{
  KHE_CAL_VERTEX cv;  int i;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ CalGraph\n", indent, "");
    HaArrayForEach(cg->vertices, cv, i)
      KheCalVertexDebug(cv, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main function"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnClusterAndLimitMeetDomains(KHE_SOLN soln,                    */
/*    KHE_COST min_cluster_weight, KHE_COST min_idle_weight,                 */
/*    float slack, KHE_MEET_BOUND_GROUP mbg, KHE_OPTIONS options)            */
/*                                                                           */
/*  Handle the cluster busy times and limit idle times monitors of soln      */
/*  structurally.                                                            */
/*                                                                           */
/*****************************************************************************/

void KheSolnClusterAndLimitMeetDomains(KHE_SOLN soln,
  KHE_COST min_cluster_weight, KHE_COST min_idle_weight,
  float slack, KHE_MEET_BOUND_GROUP mbg, KHE_OPTIONS options)
{
  KHE_CAL_GRAPH cg;  KHE_CAL_VERTEX cv;  int i, j, shift;
  KHE_CAL_IDLE_MONITOR cim;  HA_ARENA a;

  /* build the cal graph */
  if( DEBUG1 )
    fprintf(stderr, "[ KheSolnClusterAndLimitMeetDomains(soln, ...)\n");
  HnAssert(slack>=1.0, "KheSolnClusterAndLimitMeetDomains: slack out of range");
  a = KheSolnArenaBegin(soln);
  cg = KheCalGraphMake(soln, min_cluster_weight, min_idle_weight,
    slack, mbg, options, a);
  if( DEBUG2 )
    KheCalGraphDebug(cg, 1, 2, stderr);

  /* set up for diversification */
  shift = KheSolnDiversifier(soln);
  /* ***
  if( KheOptionsDiversify(options) )
    shift = KheSolnDiversifier(soln);
  else
    shift = 0;
  *** */

  /* do the cluster stuff */
  cg->cluster_priqueue = KhePriQueueMake(&KheCalVertexPriQueueKey,
    &KheCalVertexPriQueueIndexGet, &KheCalVertexPriQueueIndexSet, a);
  HaArrayForEach(cg->vertices, cv, i)
    if( HaArrayCount(cv->cluster_monitors) > 0 )
      KhePriQueueInsert(cg->cluster_priqueue, (void *) cv);
  if( DEBUG1 )
    fprintf(stderr, "  unavailable times cluster exclusions:\n");
  HaArrayForEach(cg->vertices, cv, i)
    if( HaArrayCount(cv->cluster_monitors) > 0 )
      KheCalVertexTryUnavailableClusterExclusions(cv);
  if( DEBUG1 )
    fprintf(stderr, "  ordinary cluster exclusions:\n");
  while( !KhePriQueueEmpty(cg->cluster_priqueue) )
  {
    cv = (KHE_CAL_VERTEX) KhePriQueueDeleteMin(cg->cluster_priqueue);
    if( KheCalVertexTryOrdinaryClusterExclusions(cv) )
      KhePriQueueInsert(cg->cluster_priqueue, (void *) cv);
  }
  /* KhePriQueueDelete(cg->cluster_priqueue); */
  cg->cluster_priqueue = NULL;

  /* do the idle stuff */
  if( DEBUG1 )
    fprintf(stderr, "  idle exclusions:\n");
  cg->idle_priqueue = KhePriQueueMake(&KheCalIdleMonitorPriQueueKey,
    &KheCalIdleMonitorPriQueueIndexGet, &KheCalIdleMonitorPriQueueIndexSet, a);
  HaArrayForEach(cg->vertices, cv, i)
    HaArrayForEach(cv->idle_monitors, cim, j)
    {
      KheCalIdleMonitorSetPriority(cim);
      KhePriQueueInsert(cg->idle_priqueue, (void *) cim);
    }
  while( !KhePriQueueEmpty(cg->idle_priqueue) )
  {
    cim = (KHE_CAL_IDLE_MONITOR) KhePriQueueDeleteMin(cg->idle_priqueue);
    KheCalIdleMonitorTryExclusions(cim, shift++);
  }
  /* KhePriQueueDelete(cg->idle_priqueue); */
  /* cg->idle_priqueue = NULL; futile */

  /* delete the cal graph and exit */
  KheSolnArenaEnd(soln, a);
  /* KheCalGraphDelete(cg); */
  if( DEBUG1 )
    fprintf(stderr, "] KheSolnClusterAndLimitMeetDomains returning\n");
}

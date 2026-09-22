
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
/*  FILE:         khe_sr_adjustments.c                                       */
/*  DESCRIPTION:  Solution adjustments                                       */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1 0
#define DEBUG3 0


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheSetClusterMonitorMultipliers"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSetClusterMonitorMultipliers(KHE_SOLN_ADJUSTER sa,               */
/*    char *str, int val)                                                    */
/*                                                                           */
/*  For each cluster busy times constraint c whose name or Id matches        */
/*  str, set the multiplier of every monitor derived from c to val.          */
/*                                                                           */
/*****************************************************************************/

void KheSetClusterMonitorMultipliers(KHE_SOLN soln, KHE_SOLN_ADJUSTER sa,
  char *str, int val)
{
  KHE_MONITOR m;  int i;  KHE_CONSTRAINT c;  KHE_COST combined_weight;
  if( DEBUG3 )
    fprintf(stderr, "[ KheSetClusterMonitorMultipliers(soln, \"%s\", %d):\n",
      str, val);
  HnAssert(val >= 0, "KheSetClusterMonitorMultipliers: val < 0");
  for( i = 0;  i < KheSolnMonitorCount(soln);  i++ )
  {
    m = KheSolnMonitor(soln, i);
    if( KheMonitorTag(m) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG )
    {
      c = KheMonitorConstraint(m);
      if( c != NULL )
      {
	if( strstr(KheConstraintId(c), str) != NULL ||
	    strstr(KheConstraintName(c), str) != NULL )
	{
	  combined_weight = KheMonitorCombinedWeight(m);
	  KheSolnAdjusterMonitorSetCombinedWeight(sa, m, combined_weight * val);
	}
      }
    }
  }
  if( DEBUG3 )
    fprintf(stderr, "] KheSetClusterMonitorMultipliers returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheTiltPlateau"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTiltPlateau(KHE_SOLN soln, KHE_SOLN_ADJUSTER sa)                 */
/*                                                                           */
/*  Tilt the plateau.                                                        */
/*                                                                           */
/*****************************************************************************/

void KheTiltPlateau(KHE_SOLN soln, KHE_SOLN_ADJUSTER sa)
{
  KHE_MONITOR m;  int i, time_count, index;  KHE_INSTANCE ins;
  KHE_TIME first_time, last_time;  KHE_COST weight;
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;
  ins = KheSolnInstance(soln);
  time_count = KheInstanceTimeCount(ins);
  for( i = 0;  i < KheSolnMonitorCount(soln);  i++ )
  {
    m = KheSolnMonitor(soln, i);
    weight = KheMonitorCombinedWeight(m);
    if( weight > 0 )
    {
      if( KheMonitorTag(m) == KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG )
      {
	/* tilt a limit active intervals monitor */
	KheSolnAdjusterMonitorSetCombinedWeight(sa, m, weight * time_count);
	laim = (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m;
	KheSolnAdjusterMonitorSetTilt(sa, laim);
      }
      else
      {
	/* tilt any other sort of monitor */
	if( KheMonitorTimeRange(m, &first_time, &last_time) )
	  index = KheTimeIndex(first_time);
	else
	  index = 0;
	KheSolnAdjusterMonitorSetCombinedWeight(sa, m,
	  weight * time_count - index);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KhePropagateUnavailableTimes"                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheDoPropagateResource(KHE_SOLN soln, KHE_RESOURCE r)               */
/*                                                                           */
/*  Propagate unavailable times for resource r.                              */
/*                                                                           */
/*****************************************************************************/

static bool KheDoPropagateResource(KHE_SOLN soln, KHE_RESOURCE r)
{
  KHE_TIME_GROUP unavail_tg;  bool res;  int i, j, junk;  KHE_MONITOR m;
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR autm;
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT autc;
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  KHE_LIMIT_BUSY_TIMES_MONITOR lbtm;
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc;
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;
  KHE_TIME_GROUP tg;  KHE_POLARITY po;

  /* build unavail_tg */
  res = false;
  KheSolnTimeGroupBegin(soln);
  for( i = 0;  i < KheSolnResourceMonitorCount(soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(soln, r, i);
    switch( KheMonitorTag(m) )
    {
      case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

	autm = (KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m;
	autc = KheAvoidUnavailableTimesMonitorConstraint(autm);
	if( KheConstraintWeight((KHE_CONSTRAINT) autc) > 0 )
	{
	  tg = KheAvoidUnavailableTimesConstraintUnavailableTimes(autc);
	  KheSolnTimeGroupUnion(soln, tg);
	}
	break;

      case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

	cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
	cbtc = KheClusterBusyTimesMonitorConstraint(cbtm);
	if( KheConstraintWeight((KHE_CONSTRAINT) cbtc) > 0 &&
	    KheClusterBusyTimesConstraintMaximum(cbtc) == 0 &&
	    !KheClusterBusyTimesConstraintAllNegative(cbtc) )
	{
	  for( j = 0; j < KheClusterBusyTimesMonitorTimeGroupCount(cbtm); j++ )
	  {
            tg = KheClusterBusyTimesMonitorTimeGroup(cbtm, j, &po);
	    if( po == KHE_POSITIVE )
	      KheSolnTimeGroupUnion(soln, tg);
	  }
	}
	break;

      case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

	lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) m;
	lbtc = KheLimitBusyTimesMonitorConstraint(lbtm);
	if( KheConstraintWeight((KHE_CONSTRAINT) lbtc) > 0 &&
	    KheLimitBusyTimesConstraintMaximum(lbtc) == 0 )
	{
	  for( j = 0; j < KheLimitBusyTimesMonitorTimeGroupCount(lbtm); j++ )
	  {
	    tg = KheLimitBusyTimesMonitorTimeGroup(lbtm, j, &junk);
	    KheSolnTimeGroupUnion(soln, tg);
	  }
	}
	break;

      default:

	/* nothing to do in these other cases */
	break;
    }
  }
  unavail_tg = KheSolnTimeGroupEnd(soln);
  if( DEBUG1 )
  {
    fprintf(stderr, "  resource %s: ", KheResourceId(r));
    KheTimeGroupDebug(unavail_tg, 3, 0, stderr);
  }

  /* if unavail_tg is non-empty, search for places to apply it */
  if( KheTimeGroupTimeCount(unavail_tg) > 0 )
  {
    for( i = 0;  i < KheSolnResourceMonitorCount(soln, r);  i++ )
    {
      m = KheSolnResourceMonitor(soln, r, i);
      switch( KheMonitorTag(m) )
      {
	case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

	  cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
	  for( j = 0; j < KheClusterBusyTimesMonitorTimeGroupCount(cbtm); j++ )
	  {
            tg = KheClusterBusyTimesMonitorTimeGroup(cbtm, j, &po);
	    if( KheTimeGroupSubset(tg, unavail_tg) )
	    {
	      KheClusterBusyTimesMonitorSetNotBusyState(cbtm, j,
                po == KHE_NEGATIVE);
	      if( DEBUG1 )
		fprintf(stderr, "  SetNotBusyState(cbtm %s, %d, %s)\n",
		  KheMonitorId((KHE_MONITOR) cbtm), j,
		  bool_show(po == KHE_NEGATIVE));
	      res = true;
	    }
	  }
	  break;

	case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

	  laim = (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m;
	  for( j=0; j < KheLimitActiveIntervalsMonitorTimeGroupCount(laim); j++)
	  {
            tg = KheLimitActiveIntervalsMonitorTimeGroup(laim, j, &po);
	    if( KheTimeGroupSubset(tg, unavail_tg) )
	    {
	      KheLimitActiveIntervalsMonitorSetNotBusyState(laim, j,
                po == KHE_NEGATIVE);
	      if( DEBUG1 )
		fprintf(stderr, "  SetNotBusyState(laim %s, %d, %s)\n",
		  KheMonitorId((KHE_MONITOR) laim), j,
		  bool_show(po == KHE_NEGATIVE));
	      res = true;
	    }
	  }
	  break;

	default:

	  /* nothing to do in these other cases */
	  break;
      }
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDoPropagateUnavailableTimes(KHE_SOLN soln, KHE_RESOURCE_TYPE rt) */
/*                                                                           */
/*  Propagate unavailable times for the resources of resource type rt.       */
/*                                                                           */
/*****************************************************************************/

static bool KheDoPropagateUnavailableTimes(KHE_SOLN soln, KHE_RESOURCE_TYPE rt)
{
  int i;  KHE_RESOURCE r;  bool res;
  res = false;
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    if( KheDoPropagateResource(soln, r) )
      res = true;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePropagateUnavailableTimes(KHE_SOLN soln, KHE_RESOURCE_TYPE rt)   */
/*                                                                           */
/*  For each resource r of type rt (or all resources if rt is NULL),         */
/*  propagate unavailable times to r's cluster busy times and limit          */
/*  active intervals monitors.                                               */
/*                                                                           */
/*****************************************************************************/

bool KhePropagateUnavailableTimes(KHE_SOLN soln, KHE_RESOURCE_TYPE rt)
{
  bool res;  int i;  KHE_INSTANCE ins;
  ins = KheSolnInstance(soln);
  if( DEBUG1 )
    fprintf(stderr, "[ KhePropagateUnavailableTimes(%s, %s)\n",
      KheInstanceId(ins), rt == NULL ? "-" : KheResourceTypeId(rt));
  if( rt != NULL )
    res = KheDoPropagateUnavailableTimes(soln, rt);
  else
  {
    res = false;
    for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
    {
      rt = KheInstanceResourceType(ins, i);
      if( KheDoPropagateUnavailableTimes(soln, rt) )
	res = true;
    }
  }
  if( DEBUG1 )
    fprintf(stderr, "] KhePropagateUnavailableTimes returning %s\n",
      bool_show(res));
  return res;
}

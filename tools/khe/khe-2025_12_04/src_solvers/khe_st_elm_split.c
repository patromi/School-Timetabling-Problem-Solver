
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
/*  FILE:         khe_st_elm_split.c                                         */
/*  DESCRIPTION:  Elm layer matching - splitting supplies                    */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "khe_elm.h"
#include "howard_a.h"
#include "howard_n.h"
#include <limits.h>

#define DEBUG1 0
#define DEBUG3 0
#define DEBUG10 0

typedef HA_ARRAY(KHE_ELM_DEMAND) ARRAY_KHE_ELM_DEMAND;


/*****************************************************************************/
/*                                                                           */
/*  void KheElmEnsureAssignedSupply(KHE_ELM elm, KHE_ELM_DEMAND d)           */
/*                                                                           */
/*  Try to ensure that there is a supply for assigned d in elm.              */
/*                                                                           */
/*****************************************************************************/

static void KheElmEnsureAssignedSupply(KHE_ELM elm, KHE_ELM_DEMAND d)
{
  KHE_ELM_SUPPLY_GROUP sg;  KHE_ELM_SUPPLY s, ls, rs;
  KHE_MEET meet, target_meet;  int offset, durn, i, j, count;

  meet = KheElmDemandMeet(d);
  target_meet = KheMeetAsst(meet);
  HnAssert(target_meet != NULL, "KheElmEnsureAssignedSupply: no target");
  offset = KheMeetAsstOffset(meet);
  durn = KheMeetDuration(meet);
  for( i = 0;  i < KheElmSupplyGroupCount(elm);  i++ )
  {
    sg = KheElmSupplyGroup(elm, i);
    if( KheElmSupplyGroupMeet(sg) == target_meet )
      for( j = 0;  j < KheElmSupplyGroupSupplyCount(sg);  j++ )
      {
	s = KheElmSupplyGroupSupply(sg, j);
	if( KheElmSupplySplit(s, offset, durn, &count, &ls, &rs) )
	{
	  /* split s and record the fact that d is fixed to s */
	  KheElmSupplySetFixedDemand(s, d);
	  return;
	}
      }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheElmSplitInTwo(KHE_ELM_SUPPLY s, int offset, int durn)            */
/*                                                                           */
/*  Return true if splitting s at this offset would split it into two,       */
/*  not three fragments.  The fragment has duration durn.                    */
/*                                                                           */
/*****************************************************************************/

static bool KheElmSplitInTwo(KHE_ELM_SUPPLY s, int offset, int durn)
{
  int count;
  return KheElmSupplySplitCheck(s, offset, durn, &count) && count == 2;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheElmSplitHasZoneType(KHE_ELM_SUPPLY s, int offset,                */
/*    int durn, KHE_ZONE_TYPE zone_type)                                     */
/*                                                                           */
/*  Return true if splitting s at this offset would produce the given        */
/*  zone type.  The fragment has duration durn.                              */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_ZONE_EXACT,
  KHE_ZONE_INEXACT,
  KHE_ZONE_IGNORE
} KHE_ZONE_TYPE;

static bool KheElmSplitHasZoneType(KHE_ELM_SUPPLY s, int offset,
  int durn, KHE_ZONE_TYPE zone_type)
{
  KHE_ZONE zone;  KHE_MEET m;  int i;
  switch( zone_type )
  {
    case KHE_ZONE_EXACT:

      /* ensure s[offset .. offset + durn -1] covers a single zone */
      m = KheElmSupplyMeet(s);
      zone = KheMeetOffsetZone(m, offset);
      for( i = 1;  i < durn;  i++ )
	if( KheMeetOffsetZone(m, offset + i) != zone )
	  return false;

      /* if there is a preceding zone, it must be different */
      if( offset > 0 && KheMeetOffsetZone(m, offset - 1) == zone )
	return false;

      /* if there is a following zone, it must be different */
      if( offset + durn < KheMeetDuration(m) &&
          KheMeetOffsetZone(m, offset + durn) == zone )
	return false;

      /* all in order */
      return true;

    case KHE_ZONE_INEXACT:

      /* return true if not exact */
      return !KheElmSplitHasZoneType(s, offset, durn, KHE_ZONE_EXACT);

    case KHE_ZONE_IGNORE:

      /* ignore zones and return true */
      return true;

    default:

      HnAbort("KheElmSplitHasZoneType internal error");
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheElmZoneTypeShow(KHE_ZONE_TYPE zone_type)                        */
/*                                                                           */
/*  Brief display of a zone type.                                            */
/*                                                                           */
/*****************************************************************************/

static char *KheElmZoneTypeShow(KHE_ZONE_TYPE zone_type)
{
  switch( zone_type )
  {
    case KHE_ZONE_EXACT:	return "zn+";
    case KHE_ZONE_INEXACT:	return "zn-";
    case KHE_ZONE_IGNORE:	return "---";

    default:

      HnAbort("KheElmZoneTypeShow internal error");
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void TrySplit(KHE_ELM_SUPPLY s, int offset, int durn,                    */
/*    bool in_two, KHE_ZONE_TYPE zone_type, int pre_infeas,                  */
/*    KHE_ELM_SUPPLY *best_s, int *best_offset, KHE_COST *best_cost,         */
/*    int *best_unevenness)                                                  */
/*                                                                           */
/*  Try splitting s at offset and durn, but only if in_two and zone_type     */
/*  allow.  Update *best_s, *best_offset, *best_cost, and *best_unevenness   */
/*  if new best.                                                             */
/*                                                                           */
/*****************************************************************************/

static void TrySplit(KHE_ELM_SUPPLY s, int offset, int durn,
  bool in_two, KHE_ZONE_TYPE zone_type, int pre_unmatched,
  KHE_ELM_SUPPLY *best_s, int *best_offset, KHE_COST *best_cost,
  int *best_unevenness)
{
  KHE_ELM_SUPPLY ls, rs;  KHE_COST cost;  KHE_ELM elm;
  int count, post_unmatched, unevenness;

  if( in_two == KheElmSplitInTwo(s, offset, durn) &&
      KheElmSplitHasZoneType(s, offset, durn, zone_type) )
  {
    /* try splitting s at offset */
    if( DEBUG10 )
    {
      fprintf(stderr, "  try %s %s split ", in_two ? "in2" : "in3",
        KheElmZoneTypeShow(zone_type));
      KheElmSupplyDebug(s, 1, -1, stderr);
      fprintf(stderr, " at offset %d: ", offset);
    }
    elm = KheElmSupplyGroupElm(KheElmSupplySupplyGroup(s));
    KheElmSupplySplit(s, offset, durn, &count, &ls, &rs);
    post_unmatched = KheElmBestUnmatched(elm);
    cost = KheElmBestCost(elm);
    unevenness = KheElmUnevenness(elm);
    if( post_unmatched < pre_unmatched && (cost < *best_cost ||
        (cost == *best_cost && unevenness < *best_unevenness)) )
    {
      *best_s = s;
      *best_offset = offset;
      *best_cost = cost;
      *best_unevenness = unevenness;
      if( DEBUG10 )
	fprintf(stderr, "new best (%.5f, %d)\n", KheCostShow(cost), unevenness);
	/* ***
	KheWMatchDebug(s->supply_group->elm->wmatch, 3,2,stderr);
	*** */
    }
    else
    {
      if( DEBUG10 )
      {
	if( post_unmatched >= pre_unmatched )
	  fprintf(stderr, "infeasible\n");
	else
	  fprintf(stderr, "uncompet (%.5f, %d)\n",KheCostShow(cost),unevenness);
      }
    }
    KheElmSupplyMerge(ls, s, rs);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheElmRepair(KHE_ELM elm, KHE_ELM_DEMAND d,                         */
/*    bool in_two, KHE_ZONE_TYPE zone_type, int shift)                       */
/*                                                                           */
/*  Try to repair elm so that d can match, returning true if successful.     */
/*  The repair method is to try each possible way in which a supply          */
/*  can be split into two or three fragments, one of which has the same      */
/*  duration as d, and to retain the best that matches.                      */
/*                                                                           */
/*  If in_two is true, the split must be into two fragments, otherwise it    */
/*  must be in three.  Depending on zone_type, the split must lie within     */
/*  one zone, or not, or don't care.  Parameter shift is used to vary the    */
/*  starting point of the repair.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheElmRepair(KHE_ELM elm, KHE_ELM_DEMAND d,
  bool in_two, KHE_ZONE_TYPE zone_type, int shift)
{
  KHE_ELM_SUPPLY_GROUP sg;  KHE_ELM_SUPPLY s, best_s, ls, rs;
  KHE_COST best_cost;  int i, j, durn, offset;
  int best_offset, best_unevenness, pre_unmatched, sgs, count;

  if( DEBUG10 )
    fprintf(stderr, "[ KheElmRepair(elm, d (durn %d), %s, %s, %d)\n",
      KheMeetDuration(KheElmDemandMeet(d)), in_two ? "true" : "false",
      KheElmZoneTypeShow(zone_type), shift);

  /* try all the possible repairs and remember the best */
  pre_unmatched = KheElmBestUnmatched(elm);
  /* KheWMatchEval(elm->wmatch, &pre_unmatched, &cost); */
  durn = KheMeetDuration(KheElmDemandMeet(d));
  best_cost = KheCostMax;
  best_s = NULL;
  best_offset = -1;
  best_unevenness = INT_MAX;
  sgs = KheElmSupplyGroupCount(elm);
  for( i = 0;  i < sgs;  i++ )
  {
    sg = KheElmSupplyGroup(elm, (i + shift) % sgs);
    for( j = 0;  j < KheElmSupplyGroupSupplyCount(sg);  j++ )
    {
      s = KheElmSupplyGroupSupply(sg, j);
      if( durn < KheElmSupplyDuration(s) && KheElmSupplyFixedDemand(s)==NULL  )
      {
	/* split from the back if durn is 1, from the front otherwise */
	if( durn == 1 )
	{
	  for( offset = KheElmSupplyOffset(s) + KheElmSupplyDuration(s) - durn;
	       offset >= KheElmSupplyOffset(s);  offset-- )
	    TrySplit(s, offset, durn, in_two, zone_type, pre_unmatched,
	      &best_s, &best_offset, &best_cost, &best_unevenness);
	}
	else
	{
	  for( offset = KheElmSupplyOffset(s);
	       offset + durn <= KheElmSupplyOffset(s) + KheElmSupplyDuration(s);
	       offset++ )
	    TrySplit(s, offset, durn, in_two, zone_type, pre_unmatched,
	      &best_s, &best_offset, &best_cost, &best_unevenness);
	}
      }
    }
  }

  /* make the best assignment, if any */
  if( best_s != NULL )
  {
    KheElmSupplySplit(best_s, best_offset, durn, &count, &ls, &rs);
    if( DEBUG10 )
    {
      fprintf(stderr, "  split ");
      KheElmSupplyDebug(best_s, 1, -1, stderr);
      fprintf(stderr, " at offset %d cost (%.5f, %d)\n", best_offset,
	KheCostShow(KheElmBestCost(elm)), best_unevenness);
      fprintf(stderr, "] KheElmRepair returning true\n");
    }
    return true;
  }
  else
  {
    if( DEBUG10 )
      fprintf(stderr, "] KheElmRepair returning false\n");
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmAddDemandToWMatch(KHE_ELM elm, KHE_ELM_DEMAND d, int shift)   */
/*                                                                           */
/*  Add d and its edges to the matching graph, and try to repair if          */
/*  necessary.  Use shift to vary the starting position of the repair.       */
/*                                                                           */
/*  Variable res is included just to keep the compiler happy.                */
/*                                                                           */
/*****************************************************************************/

static void KheElmAddDemandToWMatch(KHE_ELM elm, KHE_ELM_DEMAND d, int shift)
{
  int pre_unmatched, post_unmatched;  bool res;

  /* check the number of unmatched nodes before and after unfixing d */
  pre_unmatched = KheElmBestUnmatched(elm);
  KheMeetAssignUnFix(KheElmDemandMeet(d));
  KheElmDemandHasChanged(d);
  post_unmatched = KheElmBestUnmatched(elm);
  /* ***
  KheWMatchEval(elm->wmatch, &pre_unmatched, &cost);
  d->wmatch_node = KheWMatchDemandNodeMake(elm->wmatch, d,
    d->demand_group->wmatch_category, 1);
  KheWMatchEval(elm->wmatch, &post_unmatched, &cost);
  *** */
  res = false;
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheAddDemand(elm, ");
    KheElmDemandDebug(d, 1, -1, stderr);
    fprintf(stderr, ", %d) pre %d post %d\n", shift, pre_unmatched,
      post_unmatched);
  }
  if( KheMeetAsst(KheElmDemandMeet(d)) != NULL )
    KheElmEnsureAssignedSupply(elm, d);
  else if( post_unmatched >= pre_unmatched )
  {
    if( KheNodeZoneCount(KheLayerParentNode(KheElmLayer(elm))) > 0 )
    {
      /* have zones, so try first to obey them, then not */
      res = KheElmRepair(elm, d, true, KHE_ZONE_EXACT, shift) ||
      KheElmRepair(elm, d, true, KHE_ZONE_INEXACT, shift) ||
      KheElmRepair(elm, d, false, KHE_ZONE_EXACT, shift) ||
      KheElmRepair(elm, d, false, KHE_ZONE_INEXACT, shift);
    }
    else
    {
      /* no zones, so don't take them into account when splitting */
      res = KheElmRepair(elm, d, true, KHE_ZONE_IGNORE, shift) ||
      KheElmRepair(elm, d, false, KHE_ZONE_IGNORE, shift);
    }
  }
  if( DEBUG1 )
    fprintf(stderr, "] res = %s\n", res ? "true" : "false");
}


/*****************************************************************************/
/*                                                                           */
/*  int KheElmDemandDiversifyCmp(const void *t1, const void *t2)             */
/*  int KheElmDemandNoDiversifyCmp(const void *t1, const void *t2)           */
/*                                                                           */
/*  Comparison function for sorting demands such that those with             */
/*  assigned meets come first, then by increasing domain size (counting      */
/*  automatic domains as having size 1), then by decreasing demand.          */
/*                                                                           */
/*  Ties are broken using the meet index, but which way around depends       */
/*  on the diversifier if KheElmDemandDiversifyCmp is called.                */
/*                                                                           */
/*****************************************************************************/

static bool DemandDiffersByAtLeast(int demand1, int demand2, int diff)
{
  if( demand1 < demand2 )
    return demand2 - demand1 >= diff;
  else
    return demand1 - demand2 >= diff;
}

static int KheElmDemandDiversifyCmp(const void *t1, const void *t2)
{
  KHE_ELM_DEMAND d1 = * (KHE_ELM_DEMAND *) t1;
  KHE_ELM_DEMAND d2 = * (KHE_ELM_DEMAND *) t2;
  KHE_MEET meet1 = KheElmDemandMeet(d1);
  KHE_MEET meet2 = KheElmDemandMeet(d2);
  int count1, count2, demand1, demand2;  KHE_SOLN soln;
  if( (KheMeetAsst(meet1) == NULL) != (KheMeetAsst(meet2) == NULL) )
    return KheMeetAsst(meet1) != NULL ? -1 : 1;
  count1 = KheMeetDomain(meet1) == NULL ? 1 :
    KheTimeGroupTimeCount(KheMeetDomain(meet1));
  count2 = KheMeetDomain(meet2) == NULL ? 1 :
    KheTimeGroupTimeCount(KheMeetDomain(meet2));
  if( count1 != count2 )
    return count1 - count2;
  demand1 = KheMeetDemand(meet1);
  demand2 = KheMeetDemand(meet2);
  if( DemandDiffersByAtLeast(demand1, demand2, 3) )
    return demand2 - demand1;
  else
  {
    soln = KheMeetSoln(meet1);
    if( KheSolnDiversifierChoose(soln, 2) == 0 )
      return KheMeetSolnIndex(meet1) - KheMeetSolnIndex(meet2);
    else
      return KheMeetSolnIndex(meet2) - KheMeetSolnIndex(meet1);
  }
}

/* ***
static int KheElmDemandNoDiversifyCmp(const void *t1, const void *t2)
{
  KHE_ELM_DEMAND d1 = * (KHE_ELM_DEMAND *) t1;
  KHE_ELM_DEMAND d2 = * (KHE_ELM_DEMAND *) t2;
  KHE_MEET meet1 = KheElmDemandMeet(d1);
  KHE_MEET meet2 = KheElmDemandMeet(d2);
  int count1, count2;
  if( (KheMeetAsst(meet1) == NULL) != (KheMeetAsst(meet2) == NULL) )
    return KheMeetAsst(meet1) != NULL ? -1 : 1;
  count1 = KheMeetDomain(meet1) == NULL ? 1 :
    KheTimeGroupTimeCount(KheMeetDomain(meet1));
  count2 = KheMeetDomain(meet2) == NULL ? 1 :
    KheTimeGroupTimeCount(KheMeetDomain(meet2));
  if( count1 != count2 )
    return count1 - count2;
  else if( KheMeetDemand(meet2) != KheMeetDemand(meet1) )
    return KheMeetDemand(meet2) - KheMeetDemand(meet1);
  else
    return KheMeetSolnIndex(meet1) - KheMeetSolnIndex(meet2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheElmMaxUnassignedDuration(KHE_ELM elm)                             */
/*                                                                           */
/*  Return the maximum, over all unassigned demands, of the duration.        */
/*                                                                           */
/*****************************************************************************/

static int KheElmMaxUnassignedDuration(KHE_ELM elm)
{
  int res, i, j, durn;  KHE_ELM_DEMAND d;  KHE_ELM_DEMAND_GROUP dg;
  KHE_MEET meet;
  res = 0;
  for( i = 0;  i < KheElmDemandGroupCount(elm);  i++ )
  {
    dg = KheElmDemandGroup(elm, i);
    for( j = 0;  j < KheElmDemandGroupDemandCount(dg);  j++ )
    {
      d = KheElmDemandGroupDemand(dg, j);
      meet = KheElmDemandMeet(d);
      if( KheMeetAsst(meet) == NULL )
      {
	durn = KheMeetDuration(meet);
	if( durn > res )
	  res = durn;
      }
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmSplitSupplies(KHE_ELM elm, KHE_SPREAD_EVENTS_CONSTRAINT sec)  */
/*                                                                           */
/*  Guided by sec if non-NULL, split the supplies of elm to improve the      */
/*  size and cost of the best matching.                                      */
/*                                                                           */
/*****************************************************************************/

void KheElmSplitSupplies(KHE_ELM elm, KHE_SPREAD_EVENTS_CONSTRAINT sec)
{
  int i, j, max_durn, shift, count;
  KHE_ELM_SUPPLY s, ls, rs;  KHE_MEET meet;
  KHE_ELM_DEMAND d;  KHE_ELM_DEMAND_GROUP dg;
  KHE_ELM_SUPPLY_GROUP sg;  KHE_TIME_SPREAD time_spread;
  KHE_LIMITED_TIME_GROUP ltg;  KHE_TIME_GROUP tg;
  ARRAY_KHE_ELM_DEMAND all_demands;  /* KHE_OPTIONS options; */

  if( DEBUG1 || DEBUG3 )
    fprintf(stderr, "[ KheElmSplitSupplies(elm, %s)\n",
      sec != NULL ? "sec" : "-");

  /* detach unwanted resource monitors */
  /* KheElmDetachResourceMonitors(elm); */

  /* add one even time group for each time group of sec */
  /* options = KheElmOptions(elm); */
  if( sec != NULL )
  {
    time_spread = KheSpreadEventsConstraintTimeSpread(sec);
    for( i = 0;  i < KheTimeSpreadLimitedTimeGroupCount(time_spread);  i++ )
    {
      ltg = KheTimeSpreadLimitedTimeGroup(time_spread, i);
      tg = KheLimitedTimeGroupTimeGroup(ltg);
      HnAssert(tg != NULL, "KheElmSplitSupplies internal error");
      KheElmUnevennessTimeGroupAdd(elm, tg);
    }
  }

  /* accumulate and sort the demands */
  HaArrayInit(all_demands, KheElmArena(elm));
  for( i = 0;  i < KheElmDemandGroupCount(elm);  i++ )
  {
    dg = KheElmDemandGroup(elm, i);
    for( j = 0;  j < KheElmDemandGroupDemandCount(dg);  j++ )
    {
      d = KheElmDemandGroupDemand(dg, j);
      HaArrayAddLast(all_demands, d);
    }
  }
  HaArraySort(all_demands, &KheElmDemandDiversifyCmp);
  /* ***
  HaArraySort(all_demands, KheOptionsDiversify(options) ?
    &KheElmDemandDiversifyCmp : &KheElmDemandNoDiversifyCmp);
  *** */

  /* find the maximum duration of unassigned meets, or 0 if none */
  max_durn = KheElmMaxUnassignedDuration(elm);

  /* optimization:  split all supplies if max_durn == 1 */
  if( max_durn == 1 )
  {
    if( DEBUG3 )
      fprintf(stderr, "max_durn == 1 special case\n");
    for( i = 0;  i < KheElmSupplyGroupCount(elm);  i++ )
    {
      sg = KheElmSupplyGroup(elm, i);
      for( j = 0;  j < KheElmSupplyGroupSupplyCount(sg);  j++ )
      {
	s = KheElmSupplyGroupSupply(sg, j);
	if( KheElmSupplyDuration(s) > max_durn &&
	    KheElmSupplyFixedDemand(s) == NULL )
	{
	  if( DEBUG1 )
	  {
	    fprintf(stderr, "  opt splitting ");
	    KheElmSupplyDebug(s, 1, -1, stderr);
	    fprintf(stderr, " (%d > %d)\n", KheElmSupplyDuration(s), max_durn);
	  }
	  KheElmSupplySplit(s, KheElmSupplyOffset(s), max_durn, &count,&ls,&rs);
	}
      }
    }
  }

  /* fix all assignments initially */
  HaArrayForEach(all_demands, d, i)
    KheMeetAssignFix(KheElmDemandMeet(d));

  /* main loop: find supply for each demand */
  shift = KheSolnDiversifier(KheLayerSoln(KheElmLayer(elm)));
  /* ***
  shift = KheOptionsDiversify(options) ?
    KheSolnDiversifier(KheLayerSoln(KheElmLayer(elm))) : 0;
  *** */
  HaArrayForEach(all_demands, d, i)
    KheElmAddDemandToWMatch(elm, d, shift + i);

  /* debug */
  HaArrayForEach(all_demands, d, i)
  {
    meet = KheElmDemandMeet(d);
    if( DEBUG1 )
    {
      fprintf(stderr, "  final meet(durn %d, %s): ",
	KheMeetDuration(meet),
	KheMeetAsst(meet) == NULL ? "unassigned" : "assigned");
      KheMeetDebug(meet, 1, 0, stderr);
      if( KheMeetAsst(meet) != NULL )
        KheMeetDebug(KheMeetAsst(meet), 1, 4, stderr);
    }
  }

  /* further splitting of oversize supplies */
  if( max_durn > 1 )
  {
    if( DEBUG1 )
      fprintf(stderr, "  splitting to max_durn %d\n", max_durn);
    for( i = 0;  i < KheElmSupplyGroupCount(elm);  i++ )
    {
      sg = KheElmSupplyGroup(elm, i);
      for( j = 0;  j < KheElmSupplyGroupSupplyCount(sg);  j++ )
      {
	s = KheElmSupplyGroupSupply(sg, j);
	if( KheElmSupplyDuration(s) > max_durn &&
	    KheElmSupplyFixedDemand(s) == NULL )
	{
	  if( DEBUG1 )
	  {
	    fprintf(stderr, "  splitting ");
	    KheElmSupplyDebug(s, 1, -1, stderr);
	    fprintf(stderr, " (%d > %d)\n", KheElmSupplyDuration(s), max_durn);
	  }
	  KheElmSupplySplit(s, KheElmSupplyOffset(s), max_durn, &count,&ls,&rs);
	}
      }
    }
  }

  if( DEBUG1 || DEBUG3 )
  {
    if( DEBUG1 )
      KheElmDebug(elm, 3, 2, stderr);
    if( DEBUG3 )
      KheElmDebugSegmentation(elm, 1, 2, stderr);
    fprintf(stderr, "] KheElmDoMake returning\n");
  }
}

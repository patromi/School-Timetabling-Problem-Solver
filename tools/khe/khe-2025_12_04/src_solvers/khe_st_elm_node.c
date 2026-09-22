
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
/*  FILE:         khe_st_elm_node.c                                          */
/*  DESCRIPTION:  Elm layer matching - improving node regularity             */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "khe_elm.h"
#include "howard_n.h"
#include "sset.h"
#include <limits.h>

#define DEBUG4 0
#define DEBUG5 0
#define DEBUG7 0
#define DEBUG8 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_RESTRICTION - a restriction to a set of zones                    */
/*                                                                           */
/*****************************************************************************/

/* ***
typedef struct khe_elm_restriction {
  ** LSET		zones_index_set; **	** its zones as an lset      **
  ARRAY_KHE_ZONE	zones;			** its zones as an array     **
  int			irregularity;		** its zones' irregularity   **
} *KHE_ELM_RESTRICTION;
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_REGULARITY_COST - cost of a restriction to a set of zones        */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_elm_regularity_cost_rec {
  int		infeasibility;			/* infeasibility             */
  KHE_COST	without_children_cost;		/* nodes without children    */
  int		zones_cost;			/* cost of zones             */
  KHE_COST	with_children_cost;		/* nodes with children       */
} KHE_ELM_REGULARITY_COST;


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_RESTRICTED_DEMAND_GROUP - a demand group restricted to zones     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_elm_node_solver_rec *KHE_ELM_NODE_SOLVER;
typedef HA_ARRAY(KHE_ZONE) ARRAY_KHE_ZONE;

typedef struct khe_restricted_demand_group_rec {
  KHE_ELM_NODE_SOLVER	node_solver;		/* enclosing solver          */
  KHE_ELM_DEMAND_GROUP	demand_group;		/* the demand group          */
  ARRAY_KHE_ZONE	all_zones;		/* all zones dg joins to     */
  ARRAY_KHE_ZONE	best_zones;		/* best zones during search  */
  /* ARRAY_KHE_ZONE	curr_zones; */		/* curr zones during search  */
  int			curr_irregularity;	/* irreg. of curr_zones      */
} *KHE_ELM_RESTRICTED_DEMAND_GROUP;

typedef HA_ARRAY(KHE_ELM_RESTRICTED_DEMAND_GROUP)
  ARRAY_KHE_ELM_RESTRICTED_DEMAND_GROUP;


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_NODE_SOLVER                                                      */
/*                                                                           */
/*****************************************************************************/

struct khe_elm_node_solver_rec {
  HA_ARENA				arena;
  KHE_ELM				elm;
  ARRAY_KHE_ELM_RESTRICTED_DEMAND_GROUP restricted_demand_groups;
  /* KHE_ELM_REGULARITY_COST		best_reg_cost; */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "regularity cost"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheElmRegularityCostSet(KHE_ELM_REGULARITY_COST *rc,                */
/*    int infeasibility, KHE_COST without_children_cost, int zones_cost,     */
/*    KHE_COST with_children_cost)                                           */
/*                                                                           */
/*  Set *rc to these values.                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheElmRegularityCostSet(KHE_ELM_REGULARITY_COST *rc,
  int infeasibility, KHE_COST without_children_cost, int zones_cost,
  KHE_COST with_children_cost)
{
  rc->infeasibility = infeasibility;
  rc->without_children_cost = without_children_cost;
  rc->zones_cost = zones_cost;
  rc->with_children_cost = with_children_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmRegularityCostInitToLarge(KHE_ELM_REGULARITY_COST *rc,        */
/*    int infeasibility)                                                     */
/*                                                                           */
/*  Assign *rc an initial value which has the given infeasibility and is     */
/*  otherwise very large.                                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmRegularityCostInitToLarge(KHE_ELM_REGULARITY_COST *rc,
  int infeasibility)
{
  rc->infeasibility = infeasibility;
  rc->without_children_cost = KheCostMax;
  rc->zones_cost = INT_MAX;
  rc->with_children_cost = KheCostMax;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheElmRegularityCostLessThan(KHE_ELM_REGULARITY_COST *rc1,          */
/*    KHE_ELM_REGULARITY_COST *rc2)                                          */
/*                                                                           */
/*  Return true if rc1 < rc2.                                                */
/*                                                                           */
/*****************************************************************************/

static bool KheElmRegularityCostLessThan(KHE_ELM_REGULARITY_COST *rc1,
  KHE_ELM_REGULARITY_COST *rc2)
{
  if( rc1->infeasibility != rc2->infeasibility )
    return rc1->infeasibility < rc2->infeasibility;
  else if( rc1->without_children_cost != rc2->without_children_cost )
    return rc1->without_children_cost < rc2->without_children_cost;
  else if( rc1->zones_cost != rc2->zones_cost )
    return rc1->zones_cost < rc2->zones_cost;
  else
    return rc1->with_children_cost < rc2->with_children_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmRegularityCostDebug(KHE_ELM_REGULARITY_COST *rc,              */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of *rc onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheElmRegularityCostDebug(KHE_ELM_REGULARITY_COST *rc,
  int verbosity, int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "(%d, %.5f, %d, %.5f)", rc->infeasibility,
      KheCostShow(rc->without_children_cost), rc->zones_cost,
      KheCostShow(rc->with_children_cost));
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "restrictions"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_RESTRICTION KheElmRestrictionMake(void)                          */
/*                                                                           */
/*  Make a new, empty restriction.                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_ELM_RESTRICTION KheElmRestrictionMake(void)
{
  KHE_ELM_RESTRICTION res;
  MMake(res);
  ** res->zones_index_set = LSetNew(); **
  MArrayInit(res->zones);
  res->irregularity = 0;
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElmRestrictionFree(KHE_ELM_RESTRICTION r)                        */
/*                                                                           */
/*  Free r.                                                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmRestrictionFree(KHE_ELM_RESTRICTION r)
{
  ** LSetFree(r->zones_index_set); **
  MArrayFree(r->zones);
  MFree(r);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElmRestrictionClear(KHE_ELM_RESTRICTION r)                       */
/*                                                                           */
/*  Clear r back to the empty set of zones.                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmRestrictionClear(KHE_ELM_RESTRICTION r)
{
  LSetClear(r->zones_index_set);
  HaArrayClear(r->zones);
  r->irregularity = 0;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheElmRestrictionIsEmpty(KHE_ELM_RESTRICTION r)                     */
/*                                                                           */
/*  Return true if r contains no zones.                                      */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheElmRestrictionIsEmpty(KHE_ELM_RESTRICTION r)
{
  return HaArrayCount(r->zones) == 0;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElmRestrictionAssign(KHE_ELM_RESTRICTION target_r,               */
/*    KHE_ELM_RESTRICTION source_r)                                          */
/*                                                                           */
/*  Copy source_r onto target_r.                                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmRestrictionAssign(KHE_ELM_RESTRICTION target_r,
  KHE_ELM_RESTRICTION source_r)
{
  int i;
  ** LSetAssign(&target_r->zones_index_set, source_r->zones_index_set); **
  HaArrayClear(target_r->zones);
  MArrayAppend(target_r->zones, source_r->zones, i);
  target_r->irregularity = source_r->irregularity;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheElmZoneShiftedIndex(KHE_ZONE zone)                                */
/*                                                                           */
/*  This function returns 0 if zone is NULL, and the zone index plus one     */
/*  otherwise.  Essentially, it allows this module to treat NULL as a zone.  */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheElmZoneShiftedIndex(KHE_ZONE zone)
{
  return zone == NULL ? 0 : KheZoneNodeIndex(zone) + 1;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElmRestrictionPushZone(KHE_ELM_RESTRICTION r, KHE_ZONE zone,     */
/*    int irregularity)                                                      */
/*                                                                           */
/*  Add zone, which has the given irregularity, to r.                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmRestrictionPushZone(KHE_ELM_RESTRICTION r, KHE_ZONE zone,
  int irregularity)
{
  if( DEBUG7 )
  {
    fprintf(stderr, "    PushZone(r, ");
    KheZoneDebug(zone, 1, -1, stderr);
    fprintf(stderr, ", %d)\n", irregularity);
  }
  ** LSetInsert(&r->zones_index_set, KheElmZoneShiftedIndex(zone)); **
  HaArrayAddLast(r->zones, zone);
  r->irregularity += irregularity;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElmRestrictionPopZone(KHE_ELM_RESTRICTION r, KHE_ZONE zone,      */
/*    int irregularity)                                                      */
/*                                                                           */
/*  Pop zone, which has the given irregularity, from r.                      */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmRestrictionPopZone(KHE_ELM_RESTRICTION r, KHE_ZONE zone,
  int irregularity)
{
  ** LSetDelete(r->zones_index_set, KheElmZoneShiftedIndex(zone)); **
  HnAssert(MArrayLast(r->zones) == zone,
    "KheElmRestrictionPopZone internal error");
  MArrayDropLast(r->zones);
  r->irregularity -= irregularity;
  HnAssert(r->irregularity >= 0, "KheElmRestrictionPopZone internal error 2");
  if( DEBUG7 )
  {
    fprintf(stderr, "    PopZone(r, ");
    KheZoneDebug(zone, 1, -1, stderr);
    fprintf(stderr, ", %d)\n", irregularity);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheElmRestrictionContains(KHE_ELM_RESTRICTION r, KHE_ZONE zone)     */
/*                                                                           */
/*  Return true if r contains zone.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheElmRestrictionContains(KHE_ELM_RESTRICTION r, KHE_ZONE zone)
{
  return LSetContains(r->zones_index_set, KheElmZoneShiftedIndex(zone));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "restricted demand groups"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheElmEdge(KHE_ELM_DEMAND d, KHE_ELM_SUPPLY s)                      */
/*                                                                           */
/*  Return true if an edge can be drawn between d and s, not counting        */
/*  cost and not concerned with zone restrictions.                           */
/*                                                                           */
/*****************************************************************************/

static bool KheElmEdge(KHE_ELM_DEMAND d, KHE_ELM_SUPPLY s)
{
  KHE_MEET meet;
  meet = KheElmDemandMeet(d);
  if( KheElmSupplyDuration(s) != KheMeetDuration(meet) )
    return false;
  else if( KheMeetAsst(meet) != NULL )
    return KheElmSupplyMeet(s) == KheMeetAsst(meet) &&
      KheElmSupplyOffset(s) == KheMeetAsstOffset(meet);
  else
    return KheMeetAssignCheck(meet, KheElmSupplyMeet(s), KheElmSupplyOffset(s));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmRestrictedDemandGroupAddZone(                                 */
/*    KHE_ELM_RESTRICTED_DEMAND_GROUP rdg, KHE_ZONE zone)                    */
/*                                                                           */
/*  Add zone to rdg.                                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmRestrictedDemandGroupAddZone(
  KHE_ELM_RESTRICTED_DEMAND_GROUP rdg, KHE_ZONE zone)
{
  int pos;
  if( !HaArrayContains(rdg->all_zones, zone, &pos) )
    HaArrayAddLast(rdg->all_zones, zone);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_RESTRICTED_DEMAND_GROUP KheElmRestrictedDemandGroupMake(         */
/*    KHE_ELM_NODE_SOLVER ns, KHE_ELM_DEMAND_GROUP dg)                       */
/*                                                                           */
/*  Make a restricted demand group with these attributes.                    */
/*                                                                           */
/*****************************************************************************/

static KHE_ELM_RESTRICTED_DEMAND_GROUP KheElmRestrictedDemandGroupMake(
  KHE_ELM_NODE_SOLVER ns, KHE_ELM_DEMAND_GROUP dg)
{
  KHE_ELM_RESTRICTED_DEMAND_GROUP res;  KHE_ELM_DEMAND d;  KHE_ZONE zone;
  int i, j, k, m, pos;  KHE_ELM_SUPPLY_GROUP sg;  KHE_ELM_SUPPLY s;

  /* make the basic object */
  HaMake(res, ns->arena);
  res->node_solver = ns;
  res->demand_group = dg;
  HaArrayInit(res->all_zones, ns->arena);
  HaArrayInit(res->best_zones, ns->arena);
  /* HaArrayInit(res->curr_zones); */
  res->curr_irregularity = 0;
  /* ***
  res->curr_restriction = KheElmRestrictionMake();
  res->best_restriction = KheElmRestrictionMake();
  *** */

  /* fill all_zones with the zones that dg can access */
  for( i = 0;  i < KheElmDemandGroupDemandCount(dg);  i++ )
  {
    d = KheElmDemandGroupDemand(dg, i);
    for( j = 0;  j < KheElmSupplyGroupCount(ns->elm);  j++ )
    {
      sg = KheElmSupplyGroup(ns->elm, j);
      for( k = 0;  k < KheElmSupplyGroupSupplyCount(sg);  k++ )
      {
	s = KheElmSupplyGroupSupply(sg, k);
	if( KheElmEdge(d, s) )
	{
	  /* s's zones get added to rdg's, if not already present */
	  for( m = 0;  m < KheElmSupplyZoneCount(s);  m++ )
	  {
	    zone = KheElmSupplyZone(s, m);
	    if( !HaArrayContains(res->all_zones, zone, &pos) )
	      HaArrayAddLast(res->all_zones, zone);
	  }
	}
      }
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmRestrictedDemandGroupDelete(                                  */
/*    KHE_ELM_RESTRICTED_DEMAND_GROUP rdg)                                   */
/*                                                                           */
/*  Delete rdg.                                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmRestrictedDemandGroupDelete(
  KHE_ELM_RESTRICTED_DEMAND_GROUP rdg)
{
  MArrayFree(rdg->all_zones);
  MArrayFree(rdg->best_zones);
  ** ***
  MArrayFree(rdg->curr_zones);
  KheElmRestrictionFree(rdg->curr_restriction);
  KheElmRestrictionFree(rdg->best_restriction);
  *** **
  MFree(rdg);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElmRestrictedDemandGroupPushZone(                                */
/*    KHE_ELM_RESTRICTED_DEMAND_GROUP rdg, KHE_ZONE zone, int irregularity)  */
/*                                                                           */
/*  Add zone, which has the given irregularity, to r's current zones.        */
/*                                                                           */
/*****************************************************************************/

static void KheElmRestrictedDemandGroupPushZone(
  KHE_ELM_RESTRICTED_DEMAND_GROUP rdg, KHE_ZONE zone, int irregularity)
{
  if( DEBUG7 )
  {
    fprintf(stderr, "    PushZone(rdg, ");
    KheZoneDebug(zone, 1, -1, stderr);
    fprintf(stderr, ", %d)\n", irregularity);
  }
  /* LSetInsert(&r->zones_index_set, KheElmZoneShiftedIndex(zone)); */
  /* HaArrayAddLast(rdg->curr_zones, zone); */
  KheElmDemandGroupAddZone(rdg->demand_group, zone);
  rdg->curr_irregularity += irregularity;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmRestrictedDemandGroupPopZone(                                 */
/*    KHE_ELM_RESTRICTED_DEMAND_GROUP rdg, KHE_ZONE zone, int irregularity)  */
/*                                                                           */
/*  Pop zone, which has the given irregularity, from rdg.                    */
/*                                                                           */
/*****************************************************************************/

static void KheElmRestrictedDemandGroupPopZone(
  KHE_ELM_RESTRICTED_DEMAND_GROUP rdg, KHE_ZONE zone, int irregularity)
{
  /* LSetDelete(r->zones_index_set, KheElmZoneShiftedIndex(zone)); */
  /* ***
  HnAssert(MArrayLast(rdg->curr_zones) == zone,
    "KheElmRestrictedDemandGroupPopZone internal error");
  MArrayDropLast(rdg->curr_zones);
  *** */
  KheElmDemandGroupDeleteZone(rdg->demand_group, zone);
  rdg->curr_irregularity -= irregularity;
  HnAssert(rdg->curr_irregularity >= 0,
    "KheElmRestrictedDemandGroupPopZone internal error 2");
  if( DEBUG7 )
  {
    fprintf(stderr, "    PopZone(rdg, ");
    KheZoneDebug(zone, 1, -1, stderr);
    fprintf(stderr, ", %d)\n", irregularity);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheElmRestrictedDemandGroupZonePreviouslyCurrent(                   */
/*    KHE_ELM_RESTRICTED_DEMAND_GROUP rdg, KHE_ZONE zone)                    */
/*                                                                           */
/*  Return true if zone is already in use in a previous restriction.         */
/*                                                                           */
/*****************************************************************************/

static bool KheElmRestrictedDemandGroupZonePreviouslyCurrent(
  KHE_ELM_RESTRICTED_DEMAND_GROUP rdg, KHE_ZONE zone)
{
  int i /* , index */;  KHE_ELM_RESTRICTED_DEMAND_GROUP rdg2;
  KHE_ELM_NODE_SOLVER ns;
  /* index = KheElmZoneShiftedIndex(zone); */
  ns = rdg->node_solver;
  HaArrayForEach(ns->restricted_demand_groups, rdg2, i)
  {
    if( rdg2 == rdg )
      return false;
    if( KheElmDemandGroupContainsZone(rdg->demand_group, zone) )
      return true;
    /* ***
    if( KheElmRestrictionContains(rdg->curr_restriction, zone) )
      return true;
    *** */
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheElmRestrictedDemandGroupZoneIrregularity(                         */
/*    KHE_ELM_RESTRICTED_DEMAND_GROUP rdg, KHE_ZONE zone)                    */
/*                                                                           */
/*  Return the irregularity of zone in dg.                                   */
/*                                                                           */
/*****************************************************************************/

static int KheElmRestrictedDemandGroupZoneIrregularity(
  KHE_ELM_RESTRICTED_DEMAND_GROUP rdg, KHE_ZONE zone)
{
  return (zone == NULL ? 0 : KheZoneMeetOffsetCount(zone)) +
    (KheElmRestrictedDemandGroupZonePreviouslyCurrent(rdg, zone) ? 0 : 10);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmRestrictedDemandGroupResetCurrRestriction(                    */
/*    KHE_ELM_RESTRICTED_DEMAND_GROUP rdg)                                   */
/*                                                                           */
/*  Reset dg's node set to contain all its nodes.                            */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmRestrictedDemandGroupResetCurrRestriction(
  KHE_ELM_RESTRICTED_DEMAND_GROUP rdg)
{
  KHE_ZONE zone;  int i;
  HnAssert(KheElmRestrictionIsEmpty(rdg->curr_restriction),
    "KheElmRestrictedDemandGroupResetCurrRestriction internal error");
  ** KheElmRestrictionClear(rdg->curr_restriction); **
  HaArrayForEach(rdg->all_zones, zone, i)
  {
    KheElmRestrictionPushZone(rdg->curr_restriction, zone,
      KheElmRestrictedDemandGroupZoneIrregularity(rdg, zone));
    KheElmDemandGroupAddZone(rdg->demand_group, zone);
  }
  ** KheElmDemandGroupHasChanged(rdg->demand_group); **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheElmZoneDecreasingDurationCmp(const void *t1, const void *t2)      */
/*                                                                           */
/*  Comparison function for sorting an array of nodes by decreasing durn.    */
/*                                                                           */
/*****************************************************************************/

static int KheElmZoneDecreasingDurationCmp(const void *t1, const void *t2)
{
  KHE_ZONE zone1 = * (KHE_ZONE *) t1;
  KHE_ZONE zone2 = * (KHE_ZONE *) t2;
  int offs1 = (zone1 == NULL ? 0 : KheZoneMeetOffsetCount(zone1));
  int offs2 = (zone2 == NULL ? 0 : KheZoneMeetOffsetCount(zone2));
  return offs2 - offs1;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmRestrictedDemandGroupRotateLeft(                              */
/*    KHE_ELM_RESTRICTED_DEMAND_GROUP rdg, int i, int j)                     */
/*                                                                           */
/*  Rotate rdg->all_zones[i .. j-1] one place to the left.                   */
/*                                                                           */
/*****************************************************************************/

static void KheElmRestrictedDemandGroupRotateLeft(
  KHE_ELM_RESTRICTED_DEMAND_GROUP rdg, int i, int j)
{
  KHE_ZONE tmp;  int k;
  tmp = HaArray(rdg->all_zones, i);
  for( k = i + 1;  k < j;  k++ )
    HaArrayPut(rdg->all_zones, k-1, HaArray(rdg->all_zones, k));
  HaArrayPut(rdg->all_zones, k-1, tmp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDemandGroupRotateZones(KHE_ELM_DEMAND_GROUP dg,               */
/*    int i, int j, int r)                                                   */
/*                                                                           */
/*  Rotate rdg->all_zones[i .. j-1] r places to the left.                    */
/*                                                                           */
/*****************************************************************************/

static void KheElmRestrictedDemandGroupRotateZones(
  KHE_ELM_RESTRICTED_DEMAND_GROUP rdg, int i, int j, int r)
{
  int k;
  for( k = 0;  k < r;  k++ )
    KheElmRestrictedDemandGroupRotateLeft(rdg, i, j);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDemandGroupDiversify(KHE_ELM_DEMAND_GROUP dg)                 */
/*                                                                           */
/*  Diversify dg by diversifing its zones.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheElmRestrictedDemandGroupDiversify(
  KHE_ELM_RESTRICTED_DEMAND_GROUP rdg)
{
  KHE_SOLN soln;  int i, j, offsi, offsj;  KHE_ZONE zonei, zonej;
  if( HaArrayCount(rdg->all_zones) > 0 )
  {
    HaArraySort(rdg->all_zones, &KheElmZoneDecreasingDurationCmp);
    soln = KheLayerSoln(KheElmLayer(KheElmDemandGroupElm(rdg->demand_group)));
    for( i = 0;  i < HaArrayCount(rdg->all_zones);  i = j )
    {
      zonei = HaArray(rdg->all_zones, i);
      offsi = (zonei == NULL ? 0 : KheZoneMeetOffsetCount(zonei));
      for( j = i + 1;  j < HaArrayCount(rdg->all_zones);  j++ )
      {
	zonej = HaArray(rdg->all_zones, j);
        offsj = (zonej == NULL ? 0 : KheZoneMeetOffsetCount(zonej));
	if( offsj != offsi )
	  break;
      }
      if( DEBUG8 )
	fprintf(stderr, "  diversifier %d given %d choices: %d\n",
	  KheSolnDiversifier(soln), (j - i), KheSolnDiversifier(soln)%(j - i));
      KheElmRestrictedDemandGroupRotateZones(rdg, i, j,
	KheSolnDiversifierChoose(soln, j - i));
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheElmDemandGroupCmp(const void *t1, const void *t2)                 */
/*                                                                           */
/*  Comparison function for sorting an array of demand groups by             */
/*  decreasing duration, then increasing number of child nodes.              */
/*                                                                           */
/*****************************************************************************/

static int KheElmRestrictedDemandGroupCmp(const void *t1, const void *t2)
{
  KHE_ELM_RESTRICTED_DEMAND_GROUP rdg1 = *(KHE_ELM_RESTRICTED_DEMAND_GROUP *)t1;
  KHE_ELM_RESTRICTED_DEMAND_GROUP rdg2 = *(KHE_ELM_RESTRICTED_DEMAND_GROUP *)t2;
  KHE_NODE node1 = KheElmDemandGroupNode(rdg1->demand_group);
  KHE_NODE node2 = KheElmDemandGroupNode(rdg2->demand_group);
  if( KheNodeDuration(node1) != KheNodeDuration(node2) )
    return KheNodeDuration(node2) - KheNodeDuration(node1);
  else if( KheNodeChildCount(node1) != KheNodeChildCount(node2) )
    return KheNodeChildCount(node1) - KheNodeChildCount(node2);
  else
    return KheNodeSolnIndex(node1) - KheNodeSolnIndex(node2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmRestrictedDemandGroupSetRegularityCost(                       */
/*    KHE_ELM_RESTRICTED_DEMAND_GROUP rdg, KHE_ELM_REGULARITY_COST *rc)      */
/*                                                                           */
/*  Set *rc to the regularity cost of rdg.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheElmRestrictedDemandGroupSetRegularityCost(
  KHE_ELM_RESTRICTED_DEMAND_GROUP rdg, KHE_ELM_REGULARITY_COST *rc)
{
  int i, j;  KHE_COST cost;  KHE_ELM_DEMAND_GROUP dg2;  KHE_ELM elm;
  KHE_ELM_DEMAND d;  KHE_ELM_SUPPLY s;  KHE_MEET meet;
  elm = KheElmDemandGroupElm(rdg->demand_group);
  /* KheElmDemandGroupHasChanged(rdg->demand_group); */
  rc->infeasibility = KheElmBestUnmatched(elm);
  rc->without_children_cost = rc->with_children_cost = 0;
  for( i = 0;  i < KheElmDemandGroupCount(elm);  i++ )
  {
    dg2 = KheElmDemandGroup(elm, i);
    for( j = 0;  j < KheElmDemandGroupDemandCount(dg2);  j++ )
    {
      d = KheElmDemandGroupDemand(dg2, j);
      meet = KheElmDemandMeet(d);
      if( KheMeetAsst(meet) == NULL && KheElmDemandBestSupply(d, &s, &cost) )
      {
	if( KheNodeChildCount(KheElmDemandGroupNode(dg2)) == 0 )
	  rc->without_children_cost += cost;
	else
	  rc->with_children_cost += cost;
      }
    }
  }
  rc->zones_cost = rdg->curr_irregularity;
  /* rc->zones_cost = rdg->curr_restriction->irregularity; */
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "node solvers"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeSolverAllGroupsHaveZones(KHE_ELM_NODE_SOLVER ns)             */
/*                                                                           */
/*  Return true if all restricted demand groups have zones.                  */
/*                                                                           */
/*****************************************************************************/

static bool KheNodeSolverAllGroupsHaveZones(KHE_ELM_NODE_SOLVER ns)
{
  KHE_ELM_RESTRICTED_DEMAND_GROUP rdg;  int i;
  HaArrayForEach(ns->restricted_demand_groups, rdg, i)
    if( HaArrayCount(rdg->all_zones) == 0 )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheElmAddRestrictions(KHE_ELM elm)                                  */
/*                                                                           */
/*  Add zone restrictions to elm's demand groups.   Return true if           */
/*  all restrictions are non-empty.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheElmAddRestrictions(KHE_ELM elm)
{
  KHE_ELM_DEMAND_GROUP dg;  KHE_ELM_DEMAND d;  int i, j, k, n, m;
  KHE_ELM_SUPPLY_GROUP sg;  KHE_ELM_SUPPLY s;  KHE_ZONE zone;  bool res;
  int init_infeas, final_infeas;  KHE_COST init_cost, final_cost;
  res = true;
  init_infeas = KheElmBestUnmatched(elm);
  init_cost = KheElmBestCost(elm);
  HaArrayForEach(elm->demand_groups, dg, i)
  {
    ** add restrictions to dg **
    HnAssert(dg->curr_restriction == NULL,
      "KheElmAddRestrictions internal error 1");
    dg->curr_restriction = KheElmRestrictionMake();
    dg->best_restriction = KheElmRestrictionMake();

    ** add zones to dg's demands **
    HaArrayForEach(dg->demands, d, j)
      HaArrayForEach(elm->supply_groups, sg, k)
        HaArrayForEach(sg->supplies, s, n)
	  if( KheElmEdge(d, s) )
	  {
	    ** s's zones get added to dg's, if not already present **
	    HaArrayForEach(s->zones, zone, m)
	      KheElmDemandGroupAddZone(dg, zone);
	  }
    if( HaArrayCount(dg->all_zones) == 0 )
      res = false;

    ** initialize curr restriction to everything **
    KheElmRestrictedDemandGroupResetCurrRestriction(dg);
  }
  final_infeas = KheElmBestUnmatched(elm);
  final_cost = KheElmBestCost(elm);
  HnAssert(init_infeas==final_infeas, "KheElmAddRestrictions internal error 2");
  HnAssert(init_cost == final_cost, "KheElmAddRestrictions internal error 3");
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "node solvers"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_NODE_SOLVER KheElmNodeSolverMake(KHE_ELM elm)                    */
/*                                                                           */
/*  Make a node solver for elm.                                              */
/*                                                                           */
/*****************************************************************************/

static KHE_ELM_NODE_SOLVER KheElmNodeSolverMake(KHE_ELM elm)
{
  KHE_ELM_NODE_SOLVER res;  KHE_ELM_DEMAND_GROUP dg;
  KHE_ELM_RESTRICTED_DEMAND_GROUP rdg;  int i;  HA_ARENA a;
  /* ***
  int init_infeas, final_infeas;  KHE_COST init_cost, final_cost;
  init_infeas = KheElmBestUnmatched(elm);
  init_cost = KheElmBestCost(elm);
  *** */
  a = KheElmArena(elm);
  HaMake(res, a);
  res->arena = a;
  res->elm = elm;
  HaArrayInit(res->restricted_demand_groups, a);
  for( i = 0;  i < KheElmDemandGroupCount(elm);  i++ )
  {
    dg = KheElmDemandGroup(elm, i);
    rdg = KheElmRestrictedDemandGroupMake(res, dg);
    HaArrayAddLast(res->restricted_demand_groups, rdg);
  }
  /* ***
  final_infeas = KheElmBestUnmatched(elm);
  final_cost = KheElmBestCost(elm);
  HnAssert(init_infeas==final_infeas, "KheElmNodeSolverMake internal error 1");
  HnAssert(init_cost == final_cost, "KheElmNodeSolverMake internal error 2");
  KheElmRegularityCostInitToLarge(&res->best_reg_cost, 0);
  KheElmRegularityCostSet(&res->best_reg_cost, 0, KheCostMax, INT_MAX,
    KheCostMax);
  *** */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmNodeSolverDelete(KHE_ELM_NODE_SOLVER ns)                      */
/*                                                                           */
/*  Delete ns.                                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmNodeSolverDelete(KHE_ELM_NODE_SOLVER ns)
{
  while( HaArrayCount(ns->restricted_demand_groups) > 0 )
    KheElmRestrictedDemandGroupDelete(
      HaArrayDeleteLast(ns->restricted_demand_groups));
  MArrayFree(ns->restricted_demand_groups);
  MFree(ns);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElmSaveCurrZonesToBest(KHE_ELM_RESTRICTED_DEMAND_GROUP rdg)      */
/*                                                                           */
/*  Save the current zones as best.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheElmSaveCurrZonesToBest(KHE_ELM_RESTRICTED_DEMAND_GROUP rdg)
{
  int i;  KHE_ZONE zone;
  HaArrayClear(rdg->best_zones);
  for( i = 0;  i < KheElmDemandGroupZoneCount(rdg->demand_group);  i++ )
  {
    zone = KheElmDemandGroupZone(rdg->demand_group, i);
    HaArrayAddLast(rdg->best_zones, zone);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTrialDebug(KHE_ELM_RESTRICTED_DEMAND_GROUP rdg,                  */
/*    KHE_ELM_REGULARITY_COST *rc, KHE_ELM_REGULARITY_COST *best_rc)         */
/*                                                                           */
/*  Debug print of one trial of a set of zones.                              */
/*                                                                           */
/*****************************************************************************/

static void KheTrialDebug(KHE_ELM_RESTRICTED_DEMAND_GROUP rdg,
  KHE_ELM_REGULARITY_COST *rc, KHE_ELM_REGULARITY_COST *best_rc)
{
  int i;  KHE_ZONE zone;
  fprintf(stderr, "    trying {");
  KheElmRegularityCostDebug(rc, 1, -1, stderr);
  fprintf(stderr, ": ");
  for( i = 0;  i < KheElmDemandGroupZoneCount(rdg->demand_group);  i++ )
  {
    zone = KheElmDemandGroupZone(rdg->demand_group, i);
    if( i > 0 )
      fprintf(stderr, ", ");
    if( zone == NULL )
      fprintf(stderr, "NULL");
    else
      fprintf(stderr, "%d", KheZoneNodeIndex(zone));
  }
  fprintf(stderr, "}%s\n", KheElmRegularityCostLessThan(rc, best_rc) ?
    " (new best)" : "");
}


/*****************************************************************************/
/*                                                                           */
/*  void DoTrySetsOfSize(KHE_ELM_RESTRICTED_DEMAND_GROUP rdg,                */
/*    int zone_index, int count, KHE_ELM_REGULARITY_COST *best_rc)           */
/*                                                                           */
/*  Try all subsets of size count from zone_index onwards.                   */
/*                                                                           */
/*****************************************************************************/

static void DoTrySetsOfSize(KHE_ELM_RESTRICTED_DEMAND_GROUP rdg,
  int zone_index, int count, KHE_ELM_REGULARITY_COST *best_rc)
{
  KHE_ZONE zone;  int irregularity;  KHE_ELM_REGULARITY_COST rc;
  if( zone_index + count > HaArrayCount(rdg->all_zones) )
  {
    /* not enough zones left to make a set of size count, so do nothing */
    return;
  }
  else if( count == 0 )
  {
    /* only subset now is the empty set, so time to test */
    KheElmRestrictedDemandGroupSetRegularityCost(rdg, &rc);
    if( DEBUG5 )
    {
      KheTrialDebug(rdg, &rc, best_rc);
      /* ***
      fprintf(stderr, "    trying {");
      KheElmRegularityCostDebug(&rc, 1, -1, stderr);
      fprintf(stderr, ": ");
      ** HaArrayForEach(rdg->curr_restriction->zones, zone, i) **
      ** HaArrayForEach(rdg->curr_zones, zone, i) **
      for( i = 0;  i < KheElmDemandGroupZoneCount(rdg->demand_group);  i++ )
      {
	zone = KheElmDemandGroupZone(rdg->demand_group, i);
	if( i > 0 )
	  fprintf(stderr, ", ");
	** KheZoneDebug(zone, 1, -1, stderr); **
	if( zone == NULL )
	  fprintf(stderr, "NULL");
	else
	  fprintf(stderr, "%d", KheZoneNodeIndex(zone));
      }
      fprintf(stderr, "}%s\n", KheElmRegularityCostLessThan(&rc, best_rc) ?
	  " (new best)" : "");
      ** ***
      fprintf(stderr, "}%s\n",
        KheElmRegularityCostLessThan(&rc, &rdg->node_solver->best_reg_cost) ?
	  " (new best)" : "");
      *** **
      *** */
    }
    /* if( KheElmRegularityCostLessThan(&rc,&rdg->node_solver->best_reg_cost))*/
    if( KheElmRegularityCostLessThan(&rc, best_rc) )
    {
      /* ***
      KheElmRestrictionAssign(rdg->best_restriction, rdg->curr_restriction);
      HaArrayClear(rdg->best_zones);
      MArrayAppend(rdg->best_zones, rdg->curr_zones, i);
      *** */
      KheElmSaveCurrZonesToBest(rdg);
      /* rdg->node_solver->best_reg_cost = rc; */
      *best_rc = rc;
    }
  }
  else
  {
    /* try with zone, which the previous tests prove must exist */
    zone = HaArray(rdg->all_zones, zone_index);
    irregularity = KheElmRestrictedDemandGroupZoneIrregularity(rdg, zone);
    KheElmRestrictedDemandGroupPushZone(rdg, zone, irregularity);
    /* KheElmRestrictionPushZone(rdg->curr_restriction, zone, irregularity); */
    DoTrySetsOfSize(rdg, zone_index + 1, count - 1, best_rc);
    KheElmRestrictedDemandGroupPopZone(rdg, zone, irregularity);
    /* KheElmRestrictionPopZone(rdg->curr_restriction, zone, irregularity); */

    /* try without zone */
    DoTrySetsOfSize(rdg, zone_index + 1, count, best_rc);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void TryFullSet(KHE_ELM_RESTRICTED_DEMAND_GROUP rdg,                     */
/*    KHE_ELM_REGULARITY_COST *best_rc)                                      */
/*                                                                           */
/*  As a backstop, try the full set of demand group for rdg.                 */
/*                                                                           */
/*****************************************************************************/

static void TryFullSet(KHE_ELM_RESTRICTED_DEMAND_GROUP rdg,
  KHE_ELM_REGULARITY_COST *best_rc)
{
  KHE_ELM_REGULARITY_COST rc;  KHE_ZONE zone;  int i, irregularity;

  /* push all rdg's zones */
  HnAssert(KheElmDemandGroupZoneCount(rdg->demand_group) == 0,
    "TryFullSet internal error");
  /* ***
  HnAssert(KheElmRestrictionIsEmpty(rdg->curr_restriction),
    "KheElmRestrictedDemandGroupResetCurrRestriction internal error");
  *** */
  /* KheElmRestrictionClear(rdg->curr_restriction); */
  HaArrayForEach(rdg->all_zones, zone, i)
  {
    irregularity = KheElmRestrictedDemandGroupZoneIrregularity(rdg, zone);
    KheElmRestrictedDemandGroupPushZone(rdg, zone, irregularity);
    /* ***
    KheElmRestrictionPushZone(rdg->curr_restriction, zone,
      KheElmRestrictedDemandGroupZoneIrregularity(rdg, zone));
    KheElmDemandGroupAddZone(rdg->demand_group, zone);
    *** */
  }

  /* check the cost */
  KheElmRestrictedDemandGroupSetRegularityCost(rdg, &rc);
  if( DEBUG5 )
  {
    KheTrialDebug(rdg, &rc, best_rc);
    /* ***
    fprintf(stderr, "    trying full ");
    KheElmRegularityCostDebug(&rc, 1, -1, stderr);
    fprintf(stderr, ": %s\n", KheElmRegularityCostLessThan(&rc, best_rc) ?
	" (new best)" : "");
    ** ***
    fprintf(stderr, ": %s%s\n",
      ** LSetShow(rdg->curr_restriction->zones_index_set), ** ""
      KheElmRegularityCostLessThan(&rc, &rdg->node_solver->best_reg_cost) ?
	" (new best)" : "");
    *** **
    *** */
  }
  /* if( KheElmRegularityCostLessThan(&rc, &rdg->node_solver->best_reg_cost) )*/
  if( KheElmRegularityCostLessThan(&rc, best_rc) )
  {
    /* ***
    KheElmRestrictionAssign(rdg->best_restriction, rdg->curr_restriction);
    HaArrayClear(rdg->best_zones);
    MArrayAppend(rdg->best_zones, rdg->curr_zones, i);
    *** */
    KheElmSaveCurrZonesToBest(rdg);
    /* rdg->node_solver->best_reg_cost = rc; */
    *best_rc = rc;
  }

  /* pop all rdg's zones */
  HaArrayForEachReverse(rdg->all_zones, zone, i)
  {
    irregularity = KheElmRestrictedDemandGroupZoneIrregularity(rdg, zone);
    KheElmRestrictedDemandGroupPopZone(rdg, zone, irregularity);
    /* ***
    KheElmRestrictionPopZone(rdg->curr_restriction, zone,
      KheElmRestrictedDemandGroupZoneIrregularity(rdg, zone));
    KheElmDemandGroupDeleteZone(rdg->demand_group, zone);
    *** */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmImproveNodeRegularity(KHE_ELM elm)                            */
/*                                                                           */
/*  Improve the node regularity of elm with respect to zones.                */
/*                                                                           */
/*****************************************************************************/

void KheElmImproveNodeRegularity(KHE_ELM elm)
{
  KHE_ELM_RESTRICTED_DEMAND_GROUP rdg;  int i, j, count;
  KHE_ZONE zone;  KHE_NODE node, parent_node;  KHE_ELM_NODE_SOLVER ns;
  KHE_ELM_REGULARITY_COST best_rc;
  if( DEBUG4 )
  {
    fprintf(stderr, "[ KheElmImproveNodeRegularity(elm)\n");
    fprintf(stderr, "  infeas a %d\n", KheElmBestUnmatched(elm));
  }
  parent_node = KheLayerParentNode(KheElmLayer(elm));
  if( KheNodeZoneCount(parent_node) > 0 )
  {
    /* add zones and zone index sets to the supply and demands of elm */
    KheElmSpecialModeBegin(elm);
    ns = KheElmNodeSolverMake(elm);
    if( KheNodeSolverAllGroupsHaveZones(ns) )
    /* if( KheElmAddRestrictions(elm) ) */
    {
      if( DEBUG4 )
	fprintf(stderr, "  infeas b %d\n", KheElmBestUnmatched(elm));

      /* sort restricted demand groups by decreasing durn, increasing childrn */
      HaArraySort(ns->restricted_demand_groups,&KheElmRestrictedDemandGroupCmp);

      /* diversify */
      HaArrayForEach(ns->restricted_demand_groups, rdg, i)
	KheElmRestrictedDemandGroupDiversify(rdg);

      /* optionally diversify ***
      if( KheOptionsDiversify(KheElmOptions(elm)) )
	HaArrayForEach(ns->restricted_demand_groups, rdg, i)
	  KheElmRestrictedDemandGroupDiversify(rdg);
      *** */

      /* search */
      if( DEBUG4 )
	fprintf(stderr, "  infeas c %d\n", KheElmBestUnmatched(elm));
      HaArrayForEach(ns->restricted_demand_groups, rdg, i)
      {
	HnAssert(HaArrayCount(rdg->best_zones) == 0,
	  "KheElmImproveNodeRegularity internal error 1");
	HnAssert(KheElmDemandGroupZoneCount(rdg->demand_group) == 0,
	  "KheElmImproveNodeRegularity internal error 2");
	/* ***
	HnAssert(KheElmRestrictionIsEmpty(rdg->best_restriction),
	  "KheElmImproveNodeRegularity internal error 1");
	KheElmRestrictionClear(rdg->best_restriction);
        HnAssert(HaArrayCount(rdg->curr_zones) == 0,
	  "KheElmImproveNodeRegularity internal error 2");
	HnAssert(KheElmRestrictionIsEmpty(rdg->curr_restriction),
	  "KheElmImproveNodeRegularity internal error 2");
	KheElmRestrictionClear(rdg->curr_restriction);
	KheElmRegularityCostInitToLarge(&ns->best_reg_cost, infeas);
	KheElmRegularityCostSet(&ns->best_reg_cost, infeas, KheCostMax,
	  INT_MAX, KheCostMax);
	*** */
	KheElmRegularityCostSet(&best_rc, KheElmBestUnmatched(elm),
	  KheCostMax, INT_MAX, KheCostMax);
	if( DEBUG4 )
	{
	  fprintf(stderr, "  [ restricting ");
	  node = KheElmDemandGroupNode(rdg->demand_group);
	  KheNodeDebug(node, 1, -1, stderr);
	  fprintf(stderr, " (%d children)\n", KheNodeChildCount(node));
	  fprintf(stderr, "    init ");
	  /* KheElmRegularityCostDebug(&ns->best_reg_cost, 1, -1, stderr); */
	  KheElmRegularityCostDebug(&best_rc, 1, -1, stderr);
	  fprintf(stderr, "\n");
	}
	for( count = 1;  count <= 2;  count++ )
	  DoTrySetsOfSize(rdg, 0, count, &best_rc);
	TryFullSet(rdg, &best_rc);
	HnAssert(HaArrayCount(rdg->best_zones) > 0,
	  "KheElmImproveNodeRegularity internal error");
	/* ***
	HnAssert(!KheElmRestrictionIsEmpty(rdg->best_restriction),
	  "KheElmImproveNodeRegularity internal error");
	KheElmRestrictionAssign(rdg->curr_restriction, rdg->best_restriction);
	HaArrayClear(rdg->curr_zones);
	MArrayAppend(rdg->curr_zones, rdg->best_zones, j);
	HaArrayForEach(rdg->curr_restriction->zones, zone, j)
	  KheElmDemandGroupAddZone(rdg->demand_group, zone);
	KheElmDemandGroupHasChanged(rdg->demand_group);
	*** */
	HaArrayForEach(rdg->best_zones, zone, j)
	  KheElmDemandGroupAddZone(rdg->demand_group, zone);
	if( DEBUG4 )
	{
	  fprintf(stderr, "  ] best ");
	  /* KheElmRegularityCostDebug(&ns->best_reg_cost, 1, -1, stderr); */
	  KheElmRegularityCostDebug(&best_rc, 1, -1, stderr);
	  fprintf(stderr, ": ");
	    /* LSetShow(rdg->curr_restriction->zones_index_set)); */
	  HaArrayForEach(rdg->best_zones, zone, j)
	  {
	    if( j > 0 )
	      fprintf(stderr, ", ");
	    if( zone == NULL )
	      fprintf(stderr, "NULL");
	    else
	      fprintf(stderr, "%d", KheZoneNodeIndex(zone));
	  }
	  fprintf(stderr, "\n");
	}
      }
    }
    /* KheElmDeleteZones(elm); */
    KheElmSpecialModeEnd(elm);
    /* KheElmNodeSolverDelete(ns); */
  }
  if( DEBUG4 )
  {
    fprintf(stderr, "  infeas d %d\n", KheElmBestUnmatched(elm));
    fprintf(stderr, "] KheElmImproveNodeRegularity returning\n");
  }
}

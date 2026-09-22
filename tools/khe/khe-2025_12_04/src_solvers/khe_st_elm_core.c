
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
/*  FILE:         khe_st_elm_core.c                                          */
/*  DESCRIPTION:  Elm layer matching - core module                           */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "khe_elm.h"
#include "howard_a.h"
#include "howard_n.h"
#include "khe_wmatch.h"
#include "sset.h"
#include <limits.h>

#define CHILDLESS_MULTIPLIER 10

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0
#define DEBUG9 0
#define DEBUG10 0
#define DEBUG11 0
#define DEBUG12 0
#define DEBUG13 0

typedef HA_ARRAY(KHE_ELM_DEMAND_GROUP) ARRAY_KHE_ELM_DEMAND_GROUP;
typedef HA_ARRAY(KHE_ELM_DEMAND) ARRAY_KHE_ELM_DEMAND;
typedef HA_ARRAY(KHE_ELM_SUPPLY_GROUP) ARRAY_KHE_ELM_SUPPLY_GROUP;
typedef HA_ARRAY(KHE_ELM_SUPPLY) ARRAY_KHE_ELM_SUPPLY;

typedef struct khe_elm_even_time_group_rec *KHE_ELM_EVEN_TIME_GROUP;
typedef HA_ARRAY(KHE_ELM_EVEN_TIME_GROUP) ARRAY_KHE_ELM_EVEN_TIME_GROUP;

typedef struct khe_elm_even_time_group_set_rec *KHE_ELM_EVEN_TIME_GROUP_SET;
typedef HA_ARRAY(KHE_ELM_EVEN_TIME_GROUP_SET) ARRAY_KHE_ELM_EVEN_TIME_GROUP_SET;

typedef HA_ARRAY(KHE_NODE) ARRAY_KHE_NODE;
typedef HA_ARRAY(KHE_LAYER) ARRAY_KHE_LAYER;
typedef HA_ARRAY(KHE_ZONE) ARRAY_KHE_ZONE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_EVEN_TIME_GROUP (private) - helps calculate the unevenness       */
/*                                                                           */
/*****************************************************************************/

struct khe_elm_even_time_group_rec {
  KHE_ELM		elm;			/* enclosing layer match     */
  KHE_TIME_GROUP	time_group;		/* the time group            */
  int			supply_count;		/* supplies touching tg      */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_EVEN_TIME_GROUP_SET - a set of even time groups                  */
/*                                                                           */
/*****************************************************************************/

struct khe_elm_even_time_group_set_rec {
  KHE_TIME			time;		        /* one time          */
  int				supply_count;		/* starting here     */
  ARRAY_KHE_ELM_EVEN_TIME_GROUP	even_time_groups;	/* etgs at this time */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_SUPPLY_GROUP - all segments of a parent meet                     */
/*                                                                           */
/*****************************************************************************/

struct khe_elm_supply_group_rec {
  KHE_ELM		elm;			/* enclosing layer match     */
  KHE_MEET		target_meet;		/* the originating meet      */
  ARRAY_KHE_ELM_SUPPLY	supplies;		/* supplies                  */
  /* KHE_ELM_EVEN_TIME_GROUP even_time_group;*/	/* evenness time group       */
  KHE_WMATCH_CATEGORY	wmatch_category;	/* category of this group    */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_SUPPLY - one segment of a parent meet                            */
/*                                                                           */
/*****************************************************************************/

struct khe_elm_supply_rec {
  KHE_ELM_SUPPLY_GROUP	supply_group;		/* enclosing group           */
  bool			removed;		/* if temporarily removed    */
  int			offset;			/* offset in sg's meet       */
  int			duration;		/* duration                  */
  ARRAY_KHE_ZONE	zones;			/* zones of supply           */
  SSET			zone_index_set;		/* zone indexes              */
  KHE_WMATCH_NODE	wmatch_node;		/* supply node in wmatch     */
  KHE_ELM_DEMAND	fixed_demand;		/* if known to be assigned   */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_RESTRICTION - a restriction to a set of zones                    */
/*                                                                           */
/*****************************************************************************/

/* ***
typedef struct khe_restriction {
  SSET			lset;			** its zones as an lset      **
  ARRAY_KHE_ZONE	array;			** its zones as an array     **
  int			irregularity;		** its zones' irregularity   **
} *KHE_ELM_RESTRICTION;
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_DEMAND_GROUP - one group of demands                              */
/*                                                                           */
/*****************************************************************************/

struct khe_elm_demand_group_rec {
  KHE_ELM		elm;			/* enclosing layer match     */
  KHE_NODE		node;			/* the originating node      */
  ARRAY_KHE_ELM_DEMAND	demands;		/* members of the group      */
  ARRAY_KHE_ZONE	zones;			/* restricted to these zones */
  SSET			zone_index_set;		/* indexes of zones' zones   */
  /* ARRAY_KHE_ZONE	all_zones; */		/* all zones they join to    */
  /* KHE_ELM_RESTRICTION curr_restriction; */	/* the current restriction   */
  /* KHE_ELM_RESTRICTION best_restriction; */	/* the best restriction      */
  KHE_WMATCH_CATEGORY	wmatch_category;	/* category of this group    */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_DEMAND - one child meet                                          */
/*                                                                           */
/*****************************************************************************/

struct khe_elm_demand_rec {
  KHE_ELM_DEMAND_GROUP	demand_group;		/* enclosing demand group    */
  KHE_MEET		meet;			/* child meet                */
  KHE_WMATCH_NODE	wmatch_node;		/* demand node in wmatch     */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_REGULARITY_COST - cost of a restriction to a set of zones        */
/*                                                                           */
/*****************************************************************************/

/* ***
typedef struct khe_elm_regularity_cost_rec {
  int		infeasibility;			** infeasibility             **
  KHE_COST	without_children_cost;		** nodes without children    **
  int		zones_cost;			** cost of zones             **
  KHE_COST	with_children_cost;		** nodes with children       **
} KHE_ELM_REGULARITY_COST;
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM - one layer matching                                             */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_MONITOR) ARRAY_KHE_MONITOR;
typedef HA_ARRAY(KHE_CLUSTER_BUSY_TIMES_MONITOR)
  ARRAY_KHE_CLUSTER_BUSY_TIMES_MONITOR;

struct khe_elm_rec {
  HA_ARENA			arena;
  KHE_LAYER			layer;		     /* matching's layer     */
  KHE_OPTIONS			options;	     /* matching's options   */
  ARRAY_KHE_MONITOR		irregular_monitors;  /* irregular monitors   */
  /* ARRAY_KHE_MONITOR		detached_monitors; */ /* detached monitors   */
  /* ARRAY_KHE_CLUSTER_BUSY_TIMES_MONITOR cluster_monitors; */
  ARRAY_KHE_ELM_SUPPLY_GROUP	supply_groups;	     /* supply groups        */
  ARRAY_KHE_ELM_DEMAND_GROUP	demand_groups;	     /* demand groups        */
  ARRAY_KHE_ELM_EVEN_TIME_GROUP	even_time_groups;    /* even time groups     */
  ARRAY_KHE_ELM_EVEN_TIME_GROUP_SET even_time_group_sets;  /* etg sets       */
  int				unevenness;          /* unevenness of tg's   */
  KHE_WMATCH			wmatch;		     /* the matching itself  */

  /* used by node regularity code */
  /* KHE_ELM_REGULARITY_COST  best_reg_cost; */ /* best node regularity cost */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "even time group sets" (private)                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_EVEN_TIME_GROUP_SET KheElmEvenTimeGroupSetMake(KHE_TIME time)    */
/*                                                                           */
/*  Make a new even time group set for this time.                            */
/*                                                                           */
/*****************************************************************************/

KHE_ELM_EVEN_TIME_GROUP_SET KheElmEvenTimeGroupSetMake(KHE_TIME time,
  HA_ARENA a)
{
  KHE_ELM_EVEN_TIME_GROUP_SET res;
  HaMake(res, a);
  res->time = time;
  res->supply_count = 0;
  HaArrayInit(res->even_time_groups, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmEvenTimeGroupSetDelete(KHE_ELM_EVEN_TIME_GROUP_SET etgs)      */
/*                                                                           */
/*  Delete etgs.                                                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmEvenTimeGroupSetDelete(KHE_ELM_EVEN_TIME_GROUP_SET etgs)
{
  MArrayFree(etgs->even_time_groups);
  MFree(etgs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "even time groups" (private)                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_EVEN_TIME_GROUP KheElmEvenTimeGroupMake(KHE_ELM elm,             */
/*    KHE_TIME_GROUP tg)                                                     */
/*                                                                           */
/*  Make an even time group with these attributes.                           */
/*                                                                           */
/*****************************************************************************/

static KHE_ELM_EVEN_TIME_GROUP KheElmEvenTimeGroupMake(KHE_ELM elm,
  KHE_TIME_GROUP tg)
{
  KHE_ELM_EVEN_TIME_GROUP res;
  HaMake(res, elm->arena);
  res->elm = elm;
  res->time_group = tg;
  res->supply_count = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmEvenTimeGroupAddSupply(KHE_ELM_EVEN_TIME_GROUP etg)           */
/*                                                                           */
/*  Inform etg that it has an extra supply now.                              */
/*                                                                           */
/*****************************************************************************/

static void KheElmEvenTimeGroupAddSupply(KHE_ELM_EVEN_TIME_GROUP etg)
{
  etg->supply_count++;
  etg->elm->unevenness += etg->supply_count;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmEvenTimeGroupDeleteSupply(KHE_ELM_EVEN_TIME_GROUP etg)        */
/*                                                                           */
/*  Inform etg that is has one less supply now.                              */
/*                                                                           */
/*****************************************************************************/

static void KheElmEvenTimeGroupDeleteSupply(KHE_ELM_EVEN_TIME_GROUP etg)
{
  etg->elm->unevenness -= etg->supply_count;
  etg->supply_count--;
  HnAssert(etg->supply_count >= 0,
    "KheElmEvenTimeGroupDeleteSupply internal error");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmEvenTimeGroupFree(KHE_ELM_EVEN_TIME_GROUP etg)                */
/*                                                                           */
/*  Free etg.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmEvenTimeGroupFree(KHE_ELM_EVEN_TIME_GROUP etg)
{
  MFree(etg);
}
*** */


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
  res->lset = LSetNew();
  MArrayInit(res->array);
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
  LSetFree(r->lset);
  MArrayFree(r->array);
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
  SSetClear(r->lset);
  HaArrayClear(r->array);
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
  return HaArrayCount(r->array) == 0;
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
  LSetAssign(&target_r->lset, source_r->lset);
  HaArrayClear(target_r->array);
  MArrayAppend(target_r->array, source_r->array, i);
  target_r->irregularity = source_r->irregularity;
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
  SSetInsert(&r->lset, KheElmZoneShiftedIndex(zone));
  HaArrayAddLast(r->array, zone);
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
  LSetDelete(r->lset, KheElmZoneShiftedIndex(zone));
  HnAssert(HaArrayLast(r->array) == zone,
    "KheElmRestrictionPopZone internal error");
  MArrayDropLast(r->array);
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
  return SSetContains(r->lset, KheElmZoneShiftedIndex(zone));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "demand groups - basic operations"                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_DEMAND_GROUP KheElmDemandGroupMake(KHE_ELM elm, KHE_NODE node)   */
/*                                                                           */
/*  Make a new demand group for the demands of node; do not add it to elm.   */
/*                                                                           */
/*****************************************************************************/

static KHE_ELM_DEMAND_GROUP KheElmDemandGroupMake(KHE_ELM elm, KHE_NODE node)
{
  KHE_ELM_DEMAND_GROUP res;
  HaMake(res, elm->arena);
  res->elm = elm;
  res->node = node;
  HaArrayInit(res->demands, elm->arena);
  HaArrayInit(res->zones, elm->arena);
  SSetInit(res->zone_index_set, elm->arena);
  /* res->zone_index_set = SSetNew(); */
  res->wmatch_category = KheWMatchNewCategory(elm->wmatch);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDemandGroupAddDemand(KHE_ELM_DEMAND_GROUP dg,                 */
/*    KHE_ELM_DEMAND d)                                                      */
/*                                                                           */
/*  Add d to dg.                                                             */
/*                                                                           */
/*****************************************************************************/

static void KheElmDemandGroupAddDemand(KHE_ELM_DEMAND_GROUP dg,
  KHE_ELM_DEMAND d)
{
  HaArrayAddLast(dg->demands, d);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDemandGroupDeleteDemand(KHE_ELM_DEMAND_GROUP dg,              */
/*    KHE_ELM_DEMAND d)                                                      */
/*                                                                           */
/*  Delete d from dg.                                                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmDemandGroupDeleteDemand(KHE_ELM_DEMAND_GROUP dg,
  KHE_ELM_DEMAND d)
{
  KHE_ELM_DEMAND d2;  int i;
  HaArrayForEachReverse(dg->demands, d2, i)
    if( d2 == d )
    {
      HaArrayDeleteAndShift(dg->demands, i);
      return;
    }
  HnAbort("KheElmDemandGroupDeleteDemand internal error");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDemandGroupDelete(KHE_ELM_DEMAND_GROUP dg)                    */
/*                                                                           */
/*  Delete dg and its demands, but do not delete dg from the match.          */
/*                                                                           */
/*****************************************************************************/
/* ***
static void KheElmDemandDelete(KHE_ELM_DEMAND d);

static void KheElmDemandGroupDelete(KHE_ELM_DEMAND_GROUP dg)
{
  while( HaArrayCount(dg->demands) > 0 )
    KheElmDemandDelete(HaArrayLast(dg->demands));
  MArrayFree(dg->demands);
  MArrayFree(dg->zones);
  LSetFree(dg->zone_index_set);
  MFree(dg);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM KheElmDemandGroupElm(KHE_ELM_DEMAND_GROUP dg)                    */
/*                                                                           */
/*  Return dg's enclosing elm.                                               */
/*                                                                           */
/*****************************************************************************/

KHE_ELM KheElmDemandGroupElm(KHE_ELM_DEMAND_GROUP dg)
{
  return dg->elm;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE KheElmDemandGroupNode(KHE_ELM_DEMAND_GROUP dg)                  */
/*                                                                           */
/*  Return the node that dg is derived from.                                 */
/*                                                                           */
/*****************************************************************************/

KHE_NODE KheElmDemandGroupNode(KHE_ELM_DEMAND_GROUP dg)
{
  return dg->node;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheElmDemandGroupDemandCount(KHE_ELM_DEMAND_GROUP dg)                */
/*                                                                           */
/*  Return the number of demands in dg.                                      */
/*                                                                           */
/*****************************************************************************/

int KheElmDemandGroupDemandCount(KHE_ELM_DEMAND_GROUP dg)
{
  return HaArrayCount(dg->demands);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_DEMAND KheElmDemandGroupDemand(KHE_ELM_DEMAND_GROUP dg, int i)   */
/*                                                                           */
/*  Return the ith demand of dg.                                             */
/*                                                                           */
/*****************************************************************************/

KHE_ELM_DEMAND KheElmDemandGroupDemand(KHE_ELM_DEMAND_GROUP dg, int i)
{
  return HaArray(dg->demands, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDemandGroupHasChanged(KHE_ELM_DEMAND_GROUP dg)                */
/*                                                                           */
/*  Notify wmatch that every node of this demand group is now dirty.         */
/*                                                                           */
/*****************************************************************************/

void KheElmDemandGroupHasChanged(KHE_ELM_DEMAND_GROUP dg)
{
  int i;  KHE_ELM_DEMAND d;
  HaArrayForEach(dg->demands, d, i)
    KheElmDemandHasChanged(d);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDemandGroupDebug(KHE_ELM_DEMAND_GROUP dg,                     */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of dg onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

void KheElmDemandGroupDebug(KHE_ELM_DEMAND_GROUP dg,
  int verbosity, int indent, FILE *fp)
{
  int i;  KHE_ZONE zone;
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    KheNodeDebug(dg->node, 1, -1, fp);
    if( verbosity > 1 )
    {
      fprintf(fp, ":");
      HaArrayForEach(dg->zones, zone, i)
      {
	if( indent >= 0 )
	  fprintf(fp, "\n%*s  ", indent, "");
	else
	  fprintf(fp, " ");
	KheZoneDebug(zone, 1, -1, fp);
      }
    }
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "demand groups - zones"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheElmZoneShiftedIndex(KHE_ZONE zone)                                */
/*                                                                           */
/*  This function returns 0 if zone is NULL, and the zone index plus one     */
/*  otherwise.  Essentially, it allows this module to treat NULL as a zone.  */
/*                                                                           */
/*****************************************************************************/

static int KheElmZoneShiftedIndex(KHE_ZONE zone)
{
  return zone == NULL ? 0 : KheZoneNodeIndex(zone) + 1;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDemandGroupAddZone(KHE_ELM_DEMAND_GROUP dg, KHE_ZONE zone)    */
/*                                                                           */
/*  Add zone to dg; it must not be already present.                          */
/*                                                                           */
/*****************************************************************************/

void KheElmDemandGroupAddZone(KHE_ELM_DEMAND_GROUP dg, KHE_ZONE zone)
{
  int shifted_index, pos;
  shifted_index = KheElmZoneShiftedIndex(zone);
  HnAssert(!SSetContains(dg->zone_index_set, shifted_index, &pos),
    "KheElmDemandGroupAddZone: zone already present");
  HaArrayAddLast(dg->zones, zone);
  SSetInsert(dg->zone_index_set, shifted_index);
  KheElmDemandGroupHasChanged(dg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDemandGroupDeleteZone(KHE_ELM_DEMAND_GROUP dg, KHE_ZONE zone) */
/*                                                                           */
/*  Delete zone from dg; it must be present.                                 */
/*                                                                           */
/*****************************************************************************/

void KheElmDemandGroupDeleteZone(KHE_ELM_DEMAND_GROUP dg, KHE_ZONE zone)
{
  int shifted_index, i, pos;
  shifted_index = KheElmZoneShiftedIndex(zone);
  HnAssert(SSetContains(dg->zone_index_set, shifted_index, &pos),
    "KheElmDemandGroupDeleteZone: zone not present");
  for( i=HaArrayCount(dg->zones)-1; i>=0 && HaArray(dg->zones,i)!=zone; i-- );
  HnAssert(i >= 0, "KheElmDemandGroupDeleteZone internal error");
  HaArrayDeleteAndShift(dg->zones, i);
  SSetDelete(dg->zone_index_set, shifted_index);
  KheElmDemandGroupHasChanged(dg);
}


/*****************************************************************************/
/*                                                                           */
/* bool KheElmDemandGroupContainsZone(KHE_ELM_DEMAND_GROUP dg, KHE_ZONE zone)*/
/*                                                                           */
/*  Return true if dg contains zone.                                         */
/*                                                                           */
/*****************************************************************************/

bool KheElmDemandGroupContainsZone(KHE_ELM_DEMAND_GROUP dg, KHE_ZONE zone)
{
  int pos;
  return SSetContains(dg->zone_index_set, KheElmZoneShiftedIndex(zone), &pos);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheElmDemandGroupZoneCount(KHE_ELM_DEMAND_GROUP dg)                  */
/*                                                                           */
/*  Return the number of zones of dg.                                        */
/*                                                                           */
/*****************************************************************************/

int KheElmDemandGroupZoneCount(KHE_ELM_DEMAND_GROUP dg)
{
  return HaArrayCount(dg->zones);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ZONE KheElmDemandGroupZone(KHE_ELM_DEMAND_GROUP dg, int i)           */
/*                                                                           */
/*  Return the ith zone of dg.                                               */
/*                                                                           */
/*****************************************************************************/

KHE_ZONE KheElmDemandGroupZone(KHE_ELM_DEMAND_GROUP dg, int i)
{
  return HaArray(dg->zones, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheElmZonePreviouslyCurrent(KHE_ELM_DEMAND_GROUP dg, KHE_ZONE zone) */
/*                                                                           */
/*  Return true if zone is already in use in a previous restriction.         */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheElmZonePreviouslyCurrent(KHE_ELM_DEMAND_GROUP dg, KHE_ZONE zone)
{
  int i, index;  KHE_ELM_DEMAND_GROUP dg2;
  index = KheElmZoneShiftedIndex(zone);
  HaArrayForEach(dg->elm->demand_groups, dg2, i)
  {
    if( dg2 == dg )
      return false;
    if( KheElmRestrictionContains(dg->curr_restriction, zone) )
      return true;
  }
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheElmDemandGroupZoneIrregularity(KHE_ELM_DEMAND_GROUP dg,           */
/*    KHE_ZONE zone)                                                         */
/*                                                                           */
/*  Return the irregularity of zone in dg.                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheElmDemandGroupZoneIrregularity(KHE_ELM_DEMAND_GROUP dg,
  KHE_ZONE zone)
{
  return (zone == NULL ? 0 : KheZoneMeetOffsetCount(zone)) +
    (KheElmZonePreviouslyCurrent(dg, zone) ? 0 : 10);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDemandGroupResetCurrRestriction(KHE_ELM_DEMAND_GROUP dg)      */
/*                                                                           */
/*  Reset dg's node set to contain all its nodes.                            */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmDemandGroupResetCurrRestriction(KHE_ELM_DEMAND_GROUP dg)
{
  KHE_ZONE zone;  int i;
  KheElmRestrictionClear(dg->curr_restriction);
  HaArrayForEach(dg->all_zones, zone, i)
    KheElmRestrictionPushZone(dg->curr_restriction, zone,
      KheElmDemandGroupZoneIrregularity(dg, zone));
  KheElmDemandGroupHasChanged(dg);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheElmZoneDecreasingDurationCmp(const void *t1, const void *t2)      */
/*                                                                           */
/*  Comparison function for sorting an array of nodes by decreasing durn.    */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheElmZoneDecreasingDurationCmp(const void *t1, const void *t2)
{
  KHE_ZONE zone1 = * (KHE_ZONE *) t1;
  KHE_ZONE zone2 = * (KHE_ZONE *) t2;
  int offs1 = (zone1 == NULL ? 0 : KheZoneMeetOffsetCount(zone1));
  int offs2 = (zone2 == NULL ? 0 : KheZoneMeetOffsetCount(zone2));
  return offs2 - offs1;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDemandGroupRotateLeft(KHE_ELM_DEMAND_GROUP dg, int i, int j)  */
/*                                                                           */
/*  Rotate dg->zones[i .. j-1] one place to the left.                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmDemandGroupRotateLeft(KHE_ELM_DEMAND_GROUP dg, int i, int j)
{
  KHE_ZONE tmp;  int k;
  tmp = HaArray(dg->all_zones, i);
  for( k = i + 1;  k < j;  k++ )
    MArrayPut(dg->all_zones, k-1, HaArray(dg->all_zones, k));
  MArrayPut(dg->all_zones, k-1, tmp);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDemandGroupRotateZones(KHE_ELM_DEMAND_GROUP dg,               */
/*    int i, int j, int r)                                                   */
/*                                                                           */
/*  Rotate dg->zones[i .. j-1] r places to the left.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmDemandGroupRotateZones(KHE_ELM_DEMAND_GROUP dg,
  int i, int j, int r)
{
  int k;
  for( k = 0;  k < r;  k++ )
    KheElmDemandGroupRotateLeft(dg, i, j);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDemandGroupDiversify(KHE_ELM_DEMAND_GROUP dg)                 */
/*                                                                           */
/*  Diversify dg by diversifing its zones.                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmDemandGroupDiversify(KHE_ELM_DEMAND_GROUP dg)
{
  KHE_SOLN soln;  int i, j, offsi, offsj;  KHE_ZONE zonei, zonej;
  if( HaArrayCount(dg->all_zones) > 0 )
  {
    HaArraySort(dg->all_zones, &KheElmZoneDecreasingDurationCmp);
    soln = KheLayerSoln(dg->elm->layer);
    for( i = 0;  i < HaArrayCount(dg->all_zones);  i = j )
    {
      zonei = HaArray(dg->all_zones, i);
      offsi = (zonei == NULL ? 0 : KheZoneMeetOffsetCount(zonei));
      for( j = i + 1;  j < HaArrayCount(dg->all_zones);  j++ )
      {
	zonej = HaArray(dg->all_zones, j);
        offsj = (zonej == NULL ? 0 : KheZoneMeetOffsetCount(zonej));
	if( offsj != offsi )
	  break;
      }
      if( DEBUG8 )
	fprintf(stderr, "  diversifier %d given %d choices: %d\n",
	  KheSolnDiversifier(soln), (j - i), KheSolnDiversifier(soln)%(j - i));
      KheElmDemandGroupRotateZones(dg, i, j,
	KheSolnDiversifierChoose(soln, j - i));
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheElmDemandGroupCmp(const void *t1, const void *t2)                 */
/*                                                                           */
/*  Comparison function for sorting an array of demand groups by             */
/*  decreasing duration, then increasing number of child nodes.              */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheElmDemandGroupCmp(const void *t1, const void *t2)
{
  KHE_ELM_DEMAND_GROUP dg1 = * (KHE_ELM_DEMAND_GROUP *) t1;
  KHE_ELM_DEMAND_GROUP dg2 = * (KHE_ELM_DEMAND_GROUP *) t2;
  if( KheNodeDuration(dg1->node) != KheNodeDuration(dg2->node) )
    return KheNodeDuration(dg2->node) - KheNodeDuration(dg1->node);
  else if( KheNodeChildCount(dg1->node) != KheNodeChildCount(dg2->node) )
    return KheNodeChildCount(dg1->node) - KheNodeChildCount(dg2->node);
  else
    return KheNodeSolnIndex(dg1->node) - KheNodeSolnIndex(dg2->node);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "demands"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_DEMAND KheElmDemandMake(KHE_ELM_DEMAND_GROUP dg, KHE_MEET meet)  */
/*                                                                           */
/*  Make a new demand with these attributes and add it to dg.                */
/*                                                                           */
/*****************************************************************************/

static KHE_ELM_DEMAND KheElmDemandMake(KHE_ELM_DEMAND_GROUP dg, KHE_MEET meet,
  HA_ARENA a)
{
  KHE_ELM_DEMAND res;
  HaMake(res, a);
  res->demand_group = dg;
  res->meet = meet;
  res->wmatch_node = KheWMatchDemandNodeMake(dg->elm->wmatch, res,
    dg->wmatch_category, 1);
  KheElmDemandGroupAddDemand(dg, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDemandDelete(KHE_ELM_DEMAND d)                                */
/*                                                                           */
/*  Delete d, including deleting it from its demand group.                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmDemandDelete(KHE_ELM_DEMAND d)
{
  KheElmDemandGroupDeleteDemand(d->demand_group, d);
  KheWMatchDemandNodeDelete(d->wmatch_node);
  MFree(d);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheElmDemandEdgeFn(void *demand_back, void *supply_back,            */
/*    KHE_COST *cost)                                                        */
/*                                                                           */
/*  Edge function called by wmatch to find out what edges there are.         */
/*                                                                           */
/*****************************************************************************/

static bool KheElmDemandEdgeFn(void *demand_back, void *supply_back,
  KHE_COST *cost)
{
  KHE_SOLN soln;  bool res;
  KHE_ELM_DEMAND d = (KHE_ELM_DEMAND) demand_back;
  KHE_ELM_SUPPLY s = (KHE_ELM_SUPPLY) supply_back;
  KHE_ELM_DEMAND_GROUP dg = d->demand_group;
  KHE_ELM_SUPPLY_GROUP sg = s->supply_group;
  if( DEBUG6 )
  {
    fprintf(stderr, "  KheElmDemandEdgeFn(");
    KheElmDemandDebug(d, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheElmSupplyDebug(s, 1, -1, stderr);
    fprintf(stderr, " = ");
  }
  if( cost == NULL )
  {
    if( s->removed )
    {
      if( DEBUG6 )
	fprintf(stderr, "false (removed)\n");
      return false;
    }
    else if( s->duration != KheMeetDuration(d->meet) )
    {
      if( DEBUG6 )
	fprintf(stderr, "false (durns)\n");
      return false;
    }
    else if( KheMeetAsst(d->meet) != NULL )
    {
      res = sg->target_meet == KheMeetAsst(d->meet) &&
	s->offset == KheMeetAsstOffset(d->meet);
      if( DEBUG6 )
	fprintf(stderr, "%s (assigned)\n", res ? "true" : "false");
      return res;
    }
    else if( HaArrayCount(dg->zones) > 0 &&
      !SSetSubset(s->zone_index_set, dg->zone_index_set) )
    {
      if( DEBUG6 )
	fprintf(stderr, "false (zones)\n");
      return false;
    }
    else
    {
      res = KheMeetAssignCheck(d->meet, sg->target_meet, s->offset);
      if( DEBUG6 )
	fprintf(stderr, "%s (assign check)\n", res ? "true" : "false");
      return res;
    }
  }
  else
  {
    *cost = 0L;
    if( s->removed )
    {
      if( DEBUG6 )
	fprintf(stderr, "false (removed)\n");
      return false;
    }
    else if( s->duration != KheMeetDuration(d->meet) )
    {
      if( DEBUG6 )
	fprintf(stderr, "false (durn)\n");
      return false;
    }
    else if( KheMeetAsst(d->meet) != NULL )
    {
      res = sg->target_meet == KheMeetAsst(d->meet) &&
	s->offset == KheMeetAsstOffset(d->meet);
      if( DEBUG6 )
	fprintf(stderr, "%s (assigned)\n", res ? "true" : "false");
      return res;
    }
    else if( HaArrayCount(dg->zones) > 0 &&
      !SSetSubset(s->zone_index_set, dg->zone_index_set) )
    {
      if( DEBUG6 )
	fprintf(stderr, "false (zones)\n");
      return false;
    }
    else
    {
      soln = KheMeetSoln(d->meet);
      if( KheMeetAssign(d->meet, sg->target_meet, s->offset) )
      {
	*cost = KheSolnCost(soln);
	if( KheNodeChildCount(d->demand_group->node) == 0 )
	  *cost *= CHILDLESS_MULTIPLIER;
	KheMeetUnAssign(d->meet);  /* OK; undoes an experimental asst */
	if( DEBUG6 )
	  fprintf(stderr, "true (cost %.5f%s)\n", KheCostShow(*cost),
	    KheNodeChildCount(d->demand_group->node) == 0 ?
	    " childless" : "");
	return true;
      }
      else
      {
	if( DEBUG6 )
	  fprintf(stderr, "false (assign check)\n");
	return false;
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_DEMAND_GROUP KheElmDemandDemandGroup(KHE_ELM_DEMAND d)           */
/*                                                                           */
/*  Return the demand group containing d.                                    */
/*                                                                           */
/*****************************************************************************/

KHE_ELM_DEMAND_GROUP KheElmDemandDemandGroup(KHE_ELM_DEMAND d)
{
  return d->demand_group;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheElmDemandMeet(KHE_ELM_DEMAND d)                              */
/*                                                                           */
/*  Return the meet that gave rise to d.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_MEET KheElmDemandMeet(KHE_ELM_DEMAND d)
{
  return d->meet;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDemandHasChanged(KHE_ELM_DEMAND d)                            */
/*                                                                           */
/*  Inform elm that d has changed in a way that can invalidate its edges.    */
/*                                                                           */
/*****************************************************************************/

void KheElmDemandHasChanged(KHE_ELM_DEMAND d)
{
  KheWMatchDemandNodeNotifyDirty(d->wmatch_node);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheElmDemandBestSupply(KHE_ELM_DEMAND d,                            */
/*    KHE_ELM_SUPPLY *s, KHE_COST *cost)                                     */
/*                                                                           */
/*  If d is matched with a supply in the best matching, set *s to that       */
/*  supply, set *cost to the cost of the edge joining d and *s, and          */
/*  return true.  Otherwise return false.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheElmDemandBestSupply(KHE_ELM_DEMAND d, KHE_ELM_SUPPLY *s, KHE_COST *cost)
{
  *s = (KHE_ELM_SUPPLY) KheWMatchDemandNodeAssignedTo(d->wmatch_node, cost);
  return *s != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDemandDebug(KHE_ELM_DEMAND d, int verbosity,                  */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of d onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheElmDemandDebug(KHE_ELM_DEMAND d, int verbosity,
  int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "N%d:", KheNodeSolnIndex(d->demand_group->node));
    KheMeetDebug(d->meet, 1, -1, fp);
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "supply groups"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_SUPPLY_GROUP KheElmSupplyGroupMake(KHE_ELM elm,                  */
/*    KHE_MEET target_meet)                                                  */
/*                                                                           */
/*  Make a new supply group with these attributes; do not add it to elm.     */
/*                                                                           */
/*****************************************************************************/

static KHE_ELM_SUPPLY_GROUP KheElmSupplyGroupMake(KHE_ELM elm,
  KHE_MEET target_meet)
{
  KHE_ELM_SUPPLY_GROUP res;
  HaMake(res, elm->arena);
  res->elm = elm;
  res->target_meet = target_meet;
  HaArrayInit(res->supplies, elm->arena);
  res->wmatch_category = KheWMatchNewCategory(elm->wmatch);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmSupplyGroupAddSupply(KHE_ELM_SUPPLY_GROUP sg,                 */
/*    KHE_ELM_SUPPLY s)                                                      */
/*                                                                           */
/*  Add s to sg.                                                             */
/*                                                                           */
/*****************************************************************************/

static void KheElmSupplyGroupAddSupply(KHE_ELM_SUPPLY_GROUP sg,
  KHE_ELM_SUPPLY s)
{
  HaArrayAddLast(sg->supplies, s);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmSupplyGroupDeleteSupply(KHE_ELM_SUPPLY_GROUP sg,              */
/*    KHE_ELM_SUPPLY s)                                                      */
/*                                                                           */
/*  Delete s from sg.                                                        */
/*                                                                           */
/*****************************************************************************/

static void KheElmSupplyGroupDeleteSupply(KHE_ELM_SUPPLY_GROUP sg,
  KHE_ELM_SUPPLY s)
{
  KHE_ELM_SUPPLY s2;  int i;
  HaArrayForEachReverse(sg->supplies, s2, i)
    if( s2 == s )
    {
      HaArrayDeleteAndShift(sg->supplies, i);
      return;
    }
  HnAbort("KheElmSupplyGroupDeleteSupply internal error");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmSupplyGroupDelete(KHE_ELM_SUPPLY_GROUP sg)                    */
/*                                                                           */
/*  Delete sg and its supplies.                                              */
/*                                                                           */
/*****************************************************************************/
/* ***
static void KheElmSupplyDelete(KHE_ELM_SUPPLY s);

static void KheElmSupplyGroupDelete(KHE_ELM_SUPPLY_GROUP sg)
{
  while( HaArrayCount(sg->supplies) > 0 )
    KheElmSupplyDelete(HaArrayLast(sg->supplies));
  MArrayFree(sg->supplies);
  MFree(sg);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM KheElmSupplyGroupElm(KHE_ELM_SUPPLY_GROUP sg)                    */
/*                                                                           */
/*  Return the enclosing elm of sg.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_ELM KheElmSupplyGroupElm(KHE_ELM_SUPPLY_GROUP sg)
{
  return sg->elm;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheElmSupplyGroupMeet(KHE_ELM_SUPPLY_GROUP sg)                  */
/*                                                                           */
/*  Return the parent meet that sg is derived from.                          */
/*                                                                           */
/*****************************************************************************/

KHE_MEET KheElmSupplyGroupMeet(KHE_ELM_SUPPLY_GROUP sg)
{
  return sg->target_meet;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheElmSupplyGroupSupplyCount(KHE_ELM_SUPPLY_GROUP sg)                */
/*                                                                           */
/*  Return the number of supplies of sg.                                     */
/*                                                                           */
/*****************************************************************************/

int KheElmSupplyGroupSupplyCount(KHE_ELM_SUPPLY_GROUP sg)
{
  return HaArrayCount(sg->supplies);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_SUPPLY KheElmSupplyGroupSupply(KHE_ELM_SUPPLY_GROUP sg, int i)   */
/*                                                                           */
/*  Return the ith supply of sg.                                             */
/*                                                                           */
/*****************************************************************************/

KHE_ELM_SUPPLY KheElmSupplyGroupSupply(KHE_ELM_SUPPLY_GROUP sg, int i)
{
  return HaArray(sg->supplies, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmSupplyGroupDebug(KHE_ELM_SUPPLY_GROUP sg,                     */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug printf of sg onto fp.                                              */
/*                                                                           */
/*****************************************************************************/

void KheElmSupplyGroupDebug(KHE_ELM_SUPPLY_GROUP sg,
  int verbosity, int indent, FILE *fp)
{
  KHE_ELM_SUPPLY s;  int i;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ SupplyGroup(", indent, "");
    KheMeetDebug(sg->target_meet, 1, -1, fp);
    fprintf(fp, ")\n");
    HaArrayForEach(sg->supplies, s, i)
      KheElmSupplyDebug(s, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "supplies"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheElmSupplyUpdateZones(KHE_ELM_SUPPLY s)                           */
/*                                                                           */
/*  Bring the zones and zone_index_set fields of s up to date.               */
/*                                                                           */
/*****************************************************************************/

static void KheElmSupplyUpdateZones(KHE_ELM_SUPPLY s)
{
  int k, shifted_index, pos;  KHE_ZONE zone;
  HaArrayClear(s->zones);
  SSetClear(s->zone_index_set);
  for( k = 0;  k < s->duration;  k++ )
  {
    zone = KheMeetOffsetZone(s->supply_group->target_meet, s->offset + k);
    shifted_index = KheElmZoneShiftedIndex(zone);
    /* if( !MArrayContains(s->zones, zone, &pos) ) */
    if( !SSetContains(s->zone_index_set, shifted_index, &pos) )
    {
      HaArrayAddLast(s->zones, zone);
      SSetInsert(s->zone_index_set, shifted_index);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_SUPPLY KheElmSupplyMake(KHE_ELM_SUPPLY_GROUP sg,                 */
/*    int offset, int duration)                                              */
/*                                                                           */
/*  Make a new supply with these attributes and add it to sg.                */
/*                                                                           */
/*****************************************************************************/
static void KheElmUnevennessAddSupply(KHE_ELM elm, KHE_ELM_SUPPLY s);

static KHE_ELM_SUPPLY KheElmSupplyMake(KHE_ELM_SUPPLY_GROUP sg,
  int offset, int duration, HA_ARENA a)
{
  KHE_ELM_SUPPLY res;
  HaMake(res, a);
  res->supply_group = sg;
  res->removed = false;
  res->offset = offset;
  res->duration = duration;
  res->fixed_demand = NULL;
  HaArrayInit(res->zones, a);
  SSetInit(res->zone_index_set, a);
  /* res->zone_index_set = LSetNew(); */
  KheElmSupplyUpdateZones(res);
  res->wmatch_node = KheWMatchSupplyNodeMake(sg->elm->wmatch, res,
    sg->wmatch_category);
  KheElmSupplyGroupAddSupply(sg, res);
  KheElmUnevennessAddSupply(sg->elm, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmSupplyDelete(KHE_ELM_SUPPLY s)                                */
/*                                                                           */
/*  Delete s, including deleting it from its supply group.                   */
/*                                                                           */
/*****************************************************************************/
static void KheElmUnevennessDeleteSupply(KHE_ELM elm, KHE_ELM_SUPPLY s);

static void KheElmSupplyDelete(KHE_ELM_SUPPLY s)
{
  KheElmUnevennessDeleteSupply(s->supply_group->elm, s);
  KheElmSupplyGroupDeleteSupply(s->supply_group, s);
  KheWMatchSupplyNodeDelete(s->wmatch_node);
  /* ***
  MArrayFree(s->zones);
  LSetFree(s->zone_index_set);
  MFree(s);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_SUPPLY_GROUP KheElmSupplySupplyGroup(KHE_ELM_SUPPLY s)           */
/*                                                                           */
/*  Return the supply group of s.                                            */
/*                                                                           */
/*****************************************************************************/

KHE_ELM_SUPPLY_GROUP KheElmSupplySupplyGroup(KHE_ELM_SUPPLY s)
{
  return s->supply_group;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheElmSupplyMeet(KHE_ELM_SUPPLY s)                              */
/*                                                                           */
/*  Return the meet of s's supply group.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_MEET KheElmSupplyMeet(KHE_ELM_SUPPLY s)
{
  return s->supply_group->target_meet;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheElmSupplyOffset(KHE_ELM_SUPPLY s)                                 */
/*                                                                           */
/*  Return the offset of s.                                                  */
/*                                                                           */
/*****************************************************************************/

int KheElmSupplyOffset(KHE_ELM_SUPPLY s)
{
  return s->offset;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheElmSupplyDuration(KHE_ELM_SUPPLY s)                               */
/*                                                                           */
/*  Return the duration of s.                                                */
/*                                                                           */
/*****************************************************************************/

int KheElmSupplyDuration(KHE_ELM_SUPPLY s)
{
  return s->duration;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheElmSupplyZoneCount(KHE_ELM_SUPPLY s)                              */
/*                                                                           */
/*  Return the number of zones of s.                                         */
/*                                                                           */
/*****************************************************************************/

int KheElmSupplyZoneCount(KHE_ELM_SUPPLY s)
{
  return HaArrayCount(s->zones);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ZONE KheElmSupplyZone(KHE_ELM_SUPPLY s, int i)                       */
/*                                                                           */
/*  Return the i'th zone of s.                                               */
/*                                                                           */
/*****************************************************************************/

KHE_ZONE KheElmSupplyZone(KHE_ELM_SUPPLY s, int i)
{
  return HaArray(s->zones, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmSupplySetFixedDemand(KHE_ELM_SUPPLY s, KHE_ELM_DEMAND d)      */
/*                                                                           */
/*  Set s's fixed demand, possibly to NULL.                                  */
/*                                                                           */
/*****************************************************************************/

void KheElmSupplySetFixedDemand(KHE_ELM_SUPPLY s, KHE_ELM_DEMAND d)
{
  if( s->fixed_demand != d )
  {
    s->fixed_demand = d;
    KheElmDemandHasChanged(d);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_DEMAND KheElmSupplyFixedDemand(KHE_ELM_SUPPLY s)                 */
/*                                                                           */
/*  Return s's fixed demand, or NULL if none.                                */
/*                                                                           */
/*****************************************************************************/

KHE_ELM_DEMAND KheElmSupplyFixedDemand(KHE_ELM_SUPPLY s)
{
  return s->fixed_demand;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmSupplyRemove(KHE_ELM_SUPPLY s)                                */
/*                                                                           */
/*  Temporarily remove s from the graph.                                     */
/*                                                                           */
/*****************************************************************************/

void KheElmSupplyRemove(KHE_ELM_SUPPLY s)
{
  HnAssert(!s->removed, "KheElmSupplyRemove: s is already removed");
  s->removed = true;
  KheWMatchSupplyNodeNotifyDirty(s->wmatch_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmSupplyUnRemove(KHE_ELM_SUPPLY s)                              */
/*                                                                           */
/*  Unremove s from the graph.                                               */
/*                                                                           */
/*****************************************************************************/

void KheElmSupplyUnRemove(KHE_ELM_SUPPLY s)
{
  HnAssert(s->removed, "KheElmSupplyUnRemove: s is not currently removed");
  s->removed = false;
  KheWMatchSupplyNodeNotifyDirty(s->wmatch_node);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheElmSupplyIsRemoved(KHE_ELM_SUPPLY s)                             */
/*                                                                           */
/*  Return true if s is currently removed.                                   */
/*                                                                           */
/*****************************************************************************/

bool KheElmSupplyIsRemoved(KHE_ELM_SUPPLY s)
{
  return s->removed;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheElmSupplySplitCheck(KHE_ELM_SUPPLY s, int offset, int durn,      */
/*    int *count)                                                            */
/*                                                                           */
/*  Return true if s is splittable at this offset and durn, also setting     */
/*  *count to the number of fragments in that case.                          */
/*                                                                           */
/*****************************************************************************/

bool KheElmSupplySplitCheck(KHE_ELM_SUPPLY s, int offset, int durn,
  int *count)
{
  if( s->fixed_demand == NULL && s->offset <= offset &&
      offset + durn <= s->offset + s->duration )
  {
    *count = 1;
    if( s->offset < offset )
      (*count)++;
    if( offset + durn < s->offset + s->duration )
      (*count)++;
    return true;
  }
  else
  {
    *count = 0;
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmSupplyUpdate(KHE_ELM_SUPPLY s, int offset, int durn)          */
/*                                                                           */
/*  Change the offset and durn of s to these values.                         */
/*                                                                           */
/*****************************************************************************/
static void KheElmUnevennessDeleteSupply(KHE_ELM elm, KHE_ELM_SUPPLY s);

static void KheElmSupplyUpdate(KHE_ELM_SUPPLY s, int offset, int durn)
{
  KheElmUnevennessDeleteSupply(s->supply_group->elm, s);
  s->offset = offset;
  s->duration = durn;
  KheElmUnevennessAddSupply(s->supply_group->elm, s);
  KheWMatchSupplyNodeNotifyDirty(s->wmatch_node);
  KheElmSupplyUpdateZones(s);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheElmSupplySplit(KHE_ELM_SUPPLY s, int offset, int durn,           */
/*    int *count, KHE_ELM_SUPPLY *ls, KHE_ELM_SUPPLY *rs)                    */
/*                                                                           */
/*  Split s as required to ensure that there is a supply for s's             */
/*  target_meet with this offset and duration.                               */
/*                                                                           */
/*****************************************************************************/

bool KheElmSupplySplit(KHE_ELM_SUPPLY s, int offset, int durn,
  int *count, KHE_ELM_SUPPLY *ls, KHE_ELM_SUPPLY *rs)
{
  HA_ARENA a;
  a = s->supply_group->elm->arena;

  /* split to left of [offset, offset + durn] if required */
  if( !KheElmSupplySplitCheck(s, offset, durn, count) )
    return false;
  if( s->offset < offset )
  {
    *ls = KheElmSupplyMake(s->supply_group, s->offset, offset - s->offset, a);
    KheElmSupplyUpdate(s, offset, s->duration - (offset - s->offset));
  }
  else
    *ls = NULL;

  /* split to right of [offset, offset + durn] if required */
  if( offset + durn < s->offset + s->duration )
  {
    *rs = KheElmSupplyMake(s->supply_group, offset + durn,
      (s->offset + s->duration) - (offset + durn), a);
    KheElmSupplyUpdate(s, s->offset, durn);
  }
  else
    *rs = NULL;

  if( DEBUG2 )
  {
    fprintf(stderr, "  KheElmSupplySplit: ");
    if( *ls == NULL )
      fprintf(stderr, "-");
    else
      KheElmSupplyDebug(*ls, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheElmSupplyDebug(s, 1, -1, stderr);
    fprintf(stderr, ", ");
    if( *rs == NULL )
      fprintf(stderr, "-");
    else
      KheElmSupplyDebug(*rs, 1, -1, stderr);
    fprintf(stderr, "\n");
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmSupplyMerge(KHE_ELM_SUPPLY ls, KHE_ELM_SUPPLY s,              */
/*    KHE_ELM_SUPPLY rs)                                                     */
/*                                                                           */
/*  Merge ls, s, and rs.  Either or both of ls and rs could be NULL.         */
/*                                                                           */
/*****************************************************************************/

void KheElmSupplyMerge(KHE_ELM_SUPPLY ls, KHE_ELM_SUPPLY s,
  KHE_ELM_SUPPLY rs)
{
  int s_offset, s_duration;

  /* merge ls into s if present */
  s_offset = s->offset;
  s_duration = s->duration;
  if( ls != NULL )
  {
    HnAssert(s->fixed_demand == NULL,
      "KheElmSupplyMerge: s has fixed demand");
    HnAssert(ls->supply_group == s->supply_group,
      "KheElmSupplyMerge: ls and s come from different supply groups");
    HnAssert(ls->fixed_demand == NULL,
      "KheElmSupplyMerge: ls has fixed demand");
    HnAssert(ls->offset + ls->duration == s->offset,
      "KheElmSupplyMerge: ls and s have incompatible offsets and durations");
    s_offset = ls->offset;
    s_duration += ls->duration;
    KheElmSupplyDelete(ls);
  }

  /* merge rs into s if present */
  if( rs != NULL )
  {
    HnAssert(s->fixed_demand == NULL,
      "KheElmSupplyMerge: s has fixed demand");
    HnAssert(rs->supply_group == s->supply_group,
      "KheElmSupplyMerge: s and rs come from different supply groups");
    HnAssert(rs->fixed_demand == NULL,
      "KheElmSupplyMerge: rs has fixed demand");
    HnAssert(s->offset + s->duration == rs->offset,
      "KheElmSupplyMerge: s and rs have incompatible offsets and durations");
    s_duration += rs->duration;
    KheElmSupplyDelete(rs);
  }

  if( s_offset != s->offset || s_duration != s->duration )
    KheElmSupplyUpdate(s, s_offset, s_duration);

  if( DEBUG2 )
  {
    fprintf(stderr, "  KheElmSupplyMerge: ");
    KheElmSupplyDebug(s, 1, -1, stderr);
    fprintf(stderr, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmSupplyDebug(KHE_ELM_SUPPLY s, int verbosity,                  */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of s onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheElmSupplyDebug(KHE_ELM_SUPPLY s, int verbosity, int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    KheMeetDebug(s->supply_group->target_meet, 1, -1, fp);
    fprintf(fp, "+%dd%d", s->offset, s->duration);
    /* if( s->zone_index_set != NULL && !SSetEmpty(s->zone_index_set) ) */
    if( !SSetEmpty(s->zone_index_set) )
      fprintf(fp, " z%s", SSetShow(s->zone_index_set));
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "regularity cost"                                              */
/*                                                                           */
/*****************************************************************************/

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

/* ***
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
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElmRegularityCostDebug(KHE_ELM_REGULARITY_COST *rc,              */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of *rc onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

/* ***
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
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time groups sets and unevenness"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheElmUnevennessAddSupply(KHE_ELM elm, KHE_ELM_SUPPLY s)            */
/*                                                                           */
/*  Add supply to the unevenness record of elm.                              */
/*                                                                           */
/*****************************************************************************/

static void KheElmUnevennessAddSupply(KHE_ELM elm, KHE_ELM_SUPPLY s)
{
  KHE_TIME time;  KHE_ELM_EVEN_TIME_GROUP_SET etgs;
  KHE_ELM_EVEN_TIME_GROUP etg;  int i;
  time = KheMeetAsstTime(s->supply_group->target_meet);
  if( time != NULL )
  {
    time = KheTimeNeighbour(time, s->offset);
    etgs = HaArray(elm->even_time_group_sets, KheTimeIndex(time));
    etgs->supply_count++;
    HaArrayForEach(etgs->even_time_groups, etg, i)
      KheElmEvenTimeGroupAddSupply(etg);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmUnevennessDeleteSupply(KHE_ELM elm, KHE_ELM_SUPPLY s)         */
/*                                                                           */
/*  Delete supply from the unevenness record of elm.                         */
/*                                                                           */
/*****************************************************************************/

static void KheElmUnevennessDeleteSupply(KHE_ELM elm, KHE_ELM_SUPPLY s)
{
  KHE_TIME time;  KHE_ELM_EVEN_TIME_GROUP_SET etgs;
  KHE_ELM_EVEN_TIME_GROUP etg;  int i;
  time = KheMeetAsstTime(s->supply_group->target_meet);
  if( time != NULL )
  {
    time = KheTimeNeighbour(time, s->offset);
    etgs = HaArray(elm->even_time_group_sets, KheTimeIndex(time));
    etgs->supply_count--;
    HnAssert(etgs->supply_count >= 0,
      "KheElmUnevennessDeleteSupply internal errir");
    HaArrayForEach(etgs->even_time_groups, etg, i)
      KheElmEvenTimeGroupDeleteSupply(etg);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmUnevennessTimeGroupAdd(KHE_ELM elm, KHE_TIME_GROUP tg)        */
/*                                                                           */
/*  Add an evenness time group representing tg to elm.                       */
/*                                                                           */
/*****************************************************************************/

void KheElmUnevennessTimeGroupAdd(KHE_ELM elm, KHE_TIME_GROUP tg)
{
  KHE_ELM_EVEN_TIME_GROUP etg;  KHE_TIME time;  int i, j;
  KHE_ELM_EVEN_TIME_GROUP_SET etgs;
  etg = KheElmEvenTimeGroupMake(elm, tg);
  HaArrayAddLast(elm->even_time_groups, etg);
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    time = KheTimeGroupTime(tg, i);
    etgs = HaArray(elm->even_time_group_sets, KheTimeIndex(time));
    HaArrayAddLast(etgs->even_time_groups, etg);
    for( j = 0;  j < etgs->supply_count;  j++ )
      KheElmEvenTimeGroupAddSupply(etg);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheElmUnevenness(KHE_ELM elm)                                        */
/*                                                                           */
/*  Return the evenness of elm.                                              */
/*                                                                           */
/*****************************************************************************/

int KheElmUnevenness(KHE_ELM elm)
{
  return elm->unevenness;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "layer matching construction"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheElmBestCostDebug(void *back, KHE_COST cost, FILE *fp)            */
/*                                                                           */
/*  Debug print of cost onto fp.                                             */
/*                                                                           */
/*****************************************************************************/

static void KheElmBestCostDebug(void *back, KHE_COST cost, FILE *fp)
{
  fprintf(fp, "%.5f", KheCostShow(cost));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDetachResourceMonitors(KHE_ELM elm)                           */
/*                                                                           */
/*  Detach unwanted resource monitors and store them for re-attaching later. */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmDetachResourceMonitors(KHE_ELM elm)
{
  KHE_RESOURCE r;  int i, j;  KHE_SOLN soln;  KHE_MONITOR m;
  soln = KheLayerSoln(elm->layer);
  for( i = 0;  i < KheLayerResourceCount(elm->layer);  i++ )
  {
    r = KheLayerResource(elm->layer, i);
    for( j = 0;  j < KheSolnResourceMonitorCount(soln, r);  j++ )
    {
      m = KheSolnResourceMonitor(soln, r, j);
      if( KheMonitorAttachedToSoln(m) ) switch( KheMonitorTag(m) )
      {
	case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
	case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
	case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

	  KheMonitorDe tachFromSoln(m);
	  HaArrayAddLast(elm->detached_monitors, m);
	  break;

	default:

	  ** others are fine **
	  break;
      }
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElmReAttachResourceMonitors(KHE_ELM elm)                         */
/*                                                                           */
/*  Reattach the monitors detached by KheElmDetachResourceMonitors.          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmReAttachResourceMonitors(KHE_ELM elm)
{
  KHE_MONITOR m;  int i;
  HaArrayForEach(elm->detached_monitors, m, i)
    KheMonitorAttachToSoln(m);
  HaArrayClear(elm->detached_monitors);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElmFindIrregularMonitors(KHE_ELM elm)                            */
/*                                                                           */
/*  Add the irregular monitors of elm to elm->irregular_monitors.            */
/*                                                                           */
/*****************************************************************************/

static void KheElmFindIrregularMonitors(KHE_ELM elm)
{
  KHE_RESOURCE r;  int i, j;  KHE_SOLN soln;  KHE_MONITOR m;
  if( DEBUG12 )
  {
    fprintf(stderr, "[ KheElmFindIrregularMonitors(");
    KheLayerDebug(elm->layer, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }
  soln = KheLayerSoln(elm->layer);
  for( i = 0;  i < KheLayerResourceCount(elm->layer);  i++ )
  {
    r = KheLayerResource(elm->layer, i);
    if( DEBUG12 )
      fprintf(stderr, "  %s:\n", KheResourceId(r));
    for( j = 0;  j < KheSolnResourceMonitorCount(soln, r);  j++ )
    {
      m = KheSolnResourceMonitor(soln, r, j);
      if( DEBUG12 )
	KheMonitorDebug(m, 1, 4, stderr);
      if( KheMonitorAttachedToSoln(m) ) switch( KheMonitorTag(m) )
      {
	case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
	case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
	case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

	  HaArrayAddLast(elm->irregular_monitors, m);
	  break;

	default:

	  break;
      }
    }
  }
  if( DEBUG12 )
    fprintf(stderr, "] KheElmFindIrregularMonitors returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM KheElmMake(KHE_LAYER layer, KHE_OPTIONS options, HA_ARENA a)     */
/*                                                                           */
/*  Make a new elm with these attributes.                                    */
/*                                                                           */
/*****************************************************************************/

KHE_ELM KheElmMake(KHE_LAYER layer, KHE_OPTIONS options, HA_ARENA a)
{
  KHE_ELM res;  int i, j, /* max_durn, */ shift, index;
  KHE_NODE n;  KHE_MEET meet;  KHE_INSTANCE ins;
  KHE_ELM_DEMAND d;  KHE_ELM_DEMAND_GROUP dg;  /* HA_ARENA a; */
  KHE_ELM_SUPPLY_GROUP sg;  KHE_ELM_EVEN_TIME_GROUP_SET etgs;

  if( DEBUG1 || DEBUG3 )
  {
    fprintf(stderr, "[ KheElmMake(");
    KheLayerDebug(layer, 1, -1, stderr);
    fprintf(stderr, ", options)\n");
  }

  /* make the basic object */
  /* a = HaAren aMake(); */
  HaMake(res, a);
  res->arena = a;
  res->layer = layer;
  res->options = options;
  HaArrayInit(res->irregular_monitors, a);
  KheElmFindIrregularMonitors(res);
  /* HaArrayInit(res->detached_monitors); */
  /* HaArrayInit(res->cluster_monitors); */
  HaArrayInit(res->supply_groups, a);
  HaArrayInit(res->demand_groups, a);
  HaArrayInit(res->even_time_groups, a);
  HaArrayInit(res->even_time_group_sets, a);
  res->unevenness = 0;
  res->wmatch = KheWMatchMake(layer, (GENERIC_DEBUG_FN) &KheLayerDebug,
    (GENERIC_DEBUG_FN) &KheElmDemandDebug,
    (GENERIC_DEBUG_FN) &KheElmSupplyDebug,
    &KheElmDemandEdgeFn, &KheElmBestCostDebug, KheCost(1, 0), a);
  /* KheElmRegularityCostInitToLarge(&res->best_reg_cost, 0); */

  /* add one evenness time group set for each time */
  ins = KheSolnInstance(KheLayerSoln(layer));
  for( i = 0;  i < KheInstanceTimeCount(ins);  i++ )
  {
    etgs = KheElmEvenTimeGroupSetMake(KheInstanceTime(ins, i), a);
    HaArrayAddLast(res->even_time_group_sets, etgs);
  }

  /* add one supply group and one supply for each parent meet */
  shift = KheSolnDiversifier(KheLayerSoln(layer));
  /* ***
  shift = KheOptionsDiversify(options) ?
    KheSolnDiversifier(KheLayerSoln(layer)) : 0;
  *** */
  n = KheLayerParentNode(layer);
  for( j = 0;  j < KheNodeMeetCount(n);  j++ )
  {
    index = (j + shift * 7) % KheNodeMeetCount(n);
    meet = KheNodeMeet(n, index);
    sg = KheElmSupplyGroupMake(res, meet);
    HaArrayAddLast(res->supply_groups, sg);
    KheElmSupplyMake(sg, 0, KheMeetDuration(meet), a);
  }

  /* make one demand group for each child node, and one demand */
  /* for each child meet */
  /* max_durn = 0; */
  for( i = 0;  i < KheLayerChildNodeCount(layer);  i++ )
  {
    index = (i + shift * 3) % KheLayerChildNodeCount(layer);
    n = KheLayerChildNode(layer, index);
    dg = KheElmDemandGroupMake(res, n);
    HaArrayAddLast(res->demand_groups, dg);
    for( j = 0;  j < KheNodeMeetCount(n);  j++ )
    {
      index = (j + shift * 5) % KheNodeMeetCount(n);
      meet = KheNodeMeet(n, index);
      /* ***
      if( KheMeetDuration(meet) > max_durn )
        max_durn = KheMeetDuration(meet);
      *** */
      d = KheElmDemandMake(dg, meet, a);
      if( DEBUG1 )
      {
	fprintf(stderr, "  initial meet(durn %d, %s): ",
	  KheMeetDuration(d->meet),
	  KheMeetAsst(d->meet) == NULL ? "unassigned" : "assigned");
	KheMeetDebug(d->meet, 1, 0, stderr);
	if( KheMeetAsst(d->meet) != NULL )
	  KheMeetDebug(KheMeetAsst(d->meet), 1, 4, stderr);
      }
    }
  }

  if( DEBUG1 || DEBUG3 )
    fprintf(stderr, "] KheElmDoMake returning\n");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheElmDemandGroupCount(KHE_ELM elm)                                  */
/*                                                                           */
/*  Return the number of demand groups in elm.                               */
/*                                                                           */
/*****************************************************************************/

int KheElmDemandGroupCount(KHE_ELM elm)
{
  return HaArrayCount(elm->demand_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_DEMAND_GROUP KheElmDemandGroup(KHE_ELM elm, int i)               */
/*                                                                           */
/*  Return the ith demand group of elm.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_ELM_DEMAND_GROUP KheElmDemandGroup(KHE_ELM elm, int i)
{
  return HaArray(elm->demand_groups, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheElmSupplyGroupCount(KHE_ELM elm)                                  */
/*                                                                           */
/*  Return the number of supply groups of elm.                               */
/*                                                                           */
/*****************************************************************************/

int KheElmSupplyGroupCount(KHE_ELM elm)
{
  return HaArrayCount(elm->supply_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ELM_SUPPLY_GROUP KheElmSupplyGroup(KHE_ELM elm, int i)               */
/*                                                                           */
/*  Return the ith supply group of elm.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_ELM_SUPPLY_GROUP KheElmSupplyGroup(KHE_ELM elm, int i)
{
  return HaArray(elm->supply_groups, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheElmBestUnmatched(KHE_ELM elm)                                     */
/*                                                                           */
/*  Return the number of unmatched demand nodes in elm's best matching.      */
/*                                                                           */
/*****************************************************************************/

int KheElmBestUnmatched(KHE_ELM elm)
{
  int res;  KHE_COST cost;
  KheWMatchEval(elm->wmatch, &res, &cost);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheElmBestCost(KHE_ELM elm)                                     */
/*                                                                           */
/*  Return the cost of elm's best matching.  This is a sum of solution       */
/*  costs, so it is not very meaningful.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheElmBestCost(KHE_ELM elm)
{
  int junk;  KHE_COST cost;
  KheWMatchEval(elm->wmatch, &junk, &cost);
  return cost;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheElmBestAssignMeets(KHE_ELM elm)                                  */
/*                                                                           */
/*  Carry out the assignments indicated by the edges of elm's best matching. */
/*  Return true if every meet is assigned afterwards.                        */
/*                                                                           */
/*****************************************************************************/

bool KheElmBestAssignMeets(KHE_ELM elm)
{
  bool res;  int i, j /* , junk */;  KHE_COST cost;  KHE_ELM_SUPPLY s;
  KHE_MEET meet;  KHE_ELM_DEMAND_GROUP dg;  KHE_ELM_DEMAND d;
  if( DEBUG9 )
    fprintf(stderr, "[ KheElmBestAssignMeets(elm)\n");
  /* KheWMatchEval(elm->wmatch, &junk, &cost); */
  res = true;
  HaArrayForEach(elm->demand_groups, dg, i)
    HaArrayForEach(dg->demands, d, j)
    {
      meet = KheElmDemandMeet(d);
      if( KheMeetAsst(meet) == NULL && KheElmDemandBestSupply(d, &s, &cost) )
      {
	if( !KheMeetAssign(meet, KheElmSupplyMeet(s), KheElmSupplyOffset(s)) )
	  HnAbort("KheElmBestAssignMeets internal error");
	if( DEBUG9 )
	{
	  fprintf(stderr, "  KheMeetAssign(");
	  KheMeetDebug(meet, 1, -1, stderr);
	  fprintf(stderr, ", ");
	  KheMeetDebug(KheElmSupplyMeet(s), 1, -1, stderr);
	  fprintf(stderr, ", %d)\n", KheElmSupplyOffset(s));
	}
      }
      if( KheMeetAsst(meet) == NULL )
      {
	if( DEBUG9 )
	{
	  fprintf(stderr, "  no best edge for ");
	  KheMeetDebug(meet, 1, -1, stderr);
	  fprintf(stderr, "\n");
	}
	res = false;
      }
    }
  if( DEBUG9 )
  {
    if( !res )
      KheElmDebug(elm, 4, 2, stderr);
    fprintf(stderr, "] KheElmBestAssignMeets returning %s\n",
      res ? "true" : "false");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmSpecialModeBegin(KHE_ELM elm)                                 */
/*                                                                           */
/*  Begin special mode.                                                      */
/*                                                                           */
/*****************************************************************************/

void KheElmSpecialModeBegin(KHE_ELM elm)
{
  KheWMatchSpecialModeBegin(elm->wmatch);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmSpecialModeEnd(KHE_ELM elm)                                   */
/*                                                                           */
/*  End special mode.                                                        */
/*                                                                           */
/*****************************************************************************/

void KheElmSpecialModeEnd(KHE_ELM elm)
{
  KheWMatchSpecialModeEnd(elm->wmatch);
}


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA KheElmArena(KHE_ELM elm)                                        */
/*                                                                           */
/*  Return elm's arena.                                                      */
/*                                                                           */
/*****************************************************************************/

HA_ARENA KheElmArena(KHE_ELM elm)
{
  return elm->arena;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDelete(KHE_ELM elm)                                           */
/*                                                                           */
/*  Delete elm, freeing the memory it consumed.                              */
/*                                                                           */
/*****************************************************************************/

/* *** now done when the caller deletes the arena
void KheElmDelete(KHE_ELM elm)
{
  HaArena Delete(elm->arena);
  ** ***
  MArrayFree(elm->irregular_monitors);
  ** MArrayFree(elm->detached_monitors); **
  ** MArrayFree(elm->cluster_monitors); **
  while( HaArrayCount(elm->supply_groups) > 0 )
    KheElmSupplyGroupDelete(MArrayRemoveLast(elm->supply_groups));
  MArrayFree(elm->supply_groups);
  while( HaArrayCount(elm->demand_groups) > 0 )
    KheElmDemandGroupDelete(MArrayRemoveLast(elm->demand_groups));
  MArrayFree(elm->demand_groups);
  while( HaArrayCount(elm->even_time_group_sets) > 0 )
    KheElmEvenTimeGroupSetDelete(MArrayRemoveLast(elm->even_time_group_sets));
  while( HaArrayCount(elm->even_time_groups) > 0 )
    KheElmEvenTimeGroupFree(MArrayRemoveLast(elm->even_time_groups));
  KheWMatchDelete(elm->wmatch);
  MFree(elm);
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_LAYER KheElmLayer(KHE_ELM elm)                                       */
/*                                                                           */
/*  Return the layer attribute of elm.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_LAYER KheElmLayer(KHE_ELM elm)
{
  return elm->layer;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_OPTIONS KheElmOptions(KHE_ELM elm)                                   */
/*                                                                           */
/*  Return the options attribute of elm.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_OPTIONS KheElmOptions(KHE_ELM elm)
{
  return elm->options;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheElmDebug(KHE_ELM elm, int verbosity, int indent, FILE *fp)       */
/*                                                                           */
/*  Debug print of elm with the given verbosity and indent.                  */
/*                                                                           */
/*****************************************************************************/

void KheElmDebug(KHE_ELM elm, int verbosity, int indent, FILE *fp)
{
  int i, infeasibility;  KHE_COST badness;  KHE_ELM_DEMAND_GROUP dg;
  KHE_ELM_SUPPLY_GROUP sg;  KHE_ELM_EVEN_TIME_GROUP etg;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ Layer Match for ", indent, "");
    KheLayerDebug(elm->layer, 2, 0, stderr);
    HaArrayForEach(elm->supply_groups, sg, i)
      KheElmSupplyGroupDebug(sg, verbosity, indent + 2, fp);
    HaArrayForEach(elm->demand_groups, dg, i)
      KheElmDemandGroupDebug(dg, verbosity, indent + 2, fp);
    if( HaArrayCount(elm->even_time_groups) > 0 )
    {
      fprintf(fp, "%*s  unevenness (", indent, "");
      HaArrayForEach(elm->even_time_groups, etg, i)
      {
	if( i > 0 )
	  fprintf(fp, ", ");
	fprintf(fp, "%s:%d", KheTimeGroupId(etg->time_group),
	  etg->supply_count);
      }
      fprintf(fp, ") %d\n", elm->unevenness);
    }
    if( verbosity >= 2 )
    {
      KheWMatchEval(elm->wmatch, &infeasibility, &badness);
      KheWMatchDebug(elm->wmatch, verbosity, indent + 2, fp);
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDebugSegmentation(KHE_ELM elm,                                */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug the segmentation of elm.                                           */
/*                                                                           */
/*****************************************************************************/

void KheElmDebugSegmentation(KHE_ELM elm, int verbosity, int indent, FILE *fp)
{
  int i, j, durn, count, max_durn;  KHE_ELM_SUPPLY_GROUP sg;
  KHE_ELM_SUPPLY s;  KHE_TIME time;
  if( verbosity >= 1 && indent >= 0 )
  {
    /* print the segmentation and find max_durn */
    fprintf(stderr, "%*ssegmentation: ", indent, "");
    max_durn = 0;
    HaArrayForEach(elm->supply_groups, sg, i)
      HaArrayForEach(sg->supplies, s, j)
      {
	if( i + j > 0 )
	  fprintf(stderr, ", ");
	if( KheMeetIsCycleMeet(sg->target_meet) )
	{
	  time = KheTimeNeighbour(KheMeetAsstTime(sg->target_meet), s->offset);
	  fprintf(stderr, "%s", KheTimeId(time));
	}
	else
	{
	  KheMeetDebug(sg->target_meet, 1, -1, stderr);
	  fprintf(stderr, "+%d", s->offset);
	}
	fprintf(stderr, "d%d", s->duration);
	if( s->duration > max_durn )
	  max_durn = s->duration;
      }
    fprintf(stderr, "\n");

    /* print the number of segments of each duration */
    fprintf(stderr, "%*sdurations: ", indent, "");
    for( durn = max_durn;  durn >= 1;  durn-- )
    {
      count = 0;
      HaArrayForEach(elm->supply_groups, sg, i)
	HaArrayForEach(sg->supplies, s, j)
	  if( s->duration == durn )
	    count++;
      if( durn != max_durn )
	fprintf(stderr, ", ");
      fprintf(stderr, "%d x d%d", count, durn);
    }
    fprintf(stderr, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "irregular monitors"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheElmIrregularMonitorCount(KHE_ELM elm)                             */
/*                                                                           */
/*  Return the number of irregular monitors of elm.                          */
/*                                                                           */
/*****************************************************************************/

int KheElmIrregularMonitorCount(KHE_ELM elm)
{
  return HaArrayCount(elm->irregular_monitors);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheElmIrregularMonitor(KHE_ELM elm, int i)                   */
/*                                                                           */
/*  Return the ith irregular monitor of elm.                                 */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR KheElmIrregularMonitor(KHE_ELM elm, int i)
{
  return HaArray(elm->irregular_monitors, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElmSortIrregularMonitors(KHE_ELM elm,                            */
/*    int(*compar)(const void *, const void *))                              */
/*                                                                           */
/*  Sort elm's irregular monitors.                                           */
/*                                                                           */
/*****************************************************************************/

void KheElmSortIrregularMonitors(KHE_ELM elm,
  int(*compar)(const void *, const void *))
{
  HaArraySort(elm->irregular_monitors, compar);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheElmIrregularMonitorsAttached(KHE_ELM elm)                        */
/*                                                                           */
/*  Return true if all of elm's irregular monitors are currently attached.   */
/*                                                                           */
/*****************************************************************************/

bool KheElmIrregularMonitorsAttached(KHE_ELM elm)
{
  KHE_MONITOR m;  int i;
  for( i = 0;  i < KheElmIrregularMonitorCount(elm);  i++ )
  {
    m = KheElmIrregularMonitor(elm, i);
    if( !KheMonitorAttachedToSoln(m) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "taking cluser busy times constraints into account"            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheElmClusterCmp(const void *t1, const void *t2)                     */
/*                                                                           */
/*  Comparison function for sorting an array of cluster busy times monitors  */
/*  by increasing number of time groups.                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheElmClusterCmp(const void *t1, const void *t2)
{
  KHE_CLUSTER_BUSY_TIMES_MONITOR m1 = * (KHE_CLUSTER_BUSY_TIMES_MONITOR *) t1;
  KHE_CLUSTER_BUSY_TIMES_MONITOR m2 = * (KHE_CLUSTER_BUSY_TIMES_MONITOR *) t2;
  return KheClusterBusyTimesMonitorTimeGroupMonitorCount(m1) -
    KheClusterBusyTimesMonitorTimeGroupMonitorCount(m2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElmGatherClusterMonitors(KHE_ELM elm)                            */
/*                                                                           */
/*  Make a list of the cluster busy times monitors of elm, sorted by         */
/*  increasing number of time groups                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmGatherClusterMonitors(KHE_ELM elm)
{
  KHE_RESOURCE r;  int i, j;  KHE_SOLN soln;  KHE_MONITOR m;
  soln = KheLayerSoln(elm->layer);
  HaArrayClear(elm->cluster_monitors);
  for( i = 0;  i < KheLayerResourceCount(elm->layer);  i++ )
  {
    r = KheLayerResource(elm->layer, i);
    for( j = 0;  j < KheSolnResourceMonitorCount(soln, r);  j++ )
    {
      m = KheSolnResourceMonitor(soln, r, j);
      if( KheMonitorTag(m) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG )
	HaArrayAddLast(elm->cluster_monitors, (KHE_CLUSTER_BUSY_TIMES_MONITOR)m);
    }
  }
  HaArraySort(elm->cluster_monitors, &KheElmClusterCmp);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "node regularity and main solver function"                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheElmAddZones(KHE_ELM lm)                                          */
/*                                                                           */
/*  Add zones and zone index sets to the supplies of elm, showing their      */
/*  overlaps with zones.                                                     */
/*                                                                           */
/*****************************************************************************/

/* *** zones are now kept up to date automatically
static void KheElmAddZones(KHE_ELM elm)
{
  int i, j, k, pos;  KHE_ELM_SUPPLY s;  KHE_ELM_SUPPLY_GROUP sg;
  KHE_ZONE zone;
  HaArrayForEach(elm->supply_groups, sg, i)
    HaArrayForEach(sg->supplies, s, j)
    {
      ** add a zone index set and zones to s **
      HnAssert(s->zone_index_set==NULL,"KheElmAddZones internal error");
      s->zone_index_set = LSetNew();
      for( k = 0;  k < s->duration;  k++ )
      {
	zone = KheMeetOffsetZone(sg->target_meet, s->offset + k);
	if( !MArrayContains(s->zones, zone, &pos) )
	{
	  HaArrayAddLast(s->zones, zone);
	  SSetInsert(&s->zone_index_set, KheElmZoneShiftedIndex(zone));
	}
      }
    }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElmDeleteZones(KHE_ELM elm)                                      */
/*                                                                           */
/*  Delete zones and zone index sets from the supplies of elm.               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElmDeleteZones(KHE_ELM elm)
{
  int i, j;  KHE_ELM_SUPPLY s;  KHE_ELM_SUPPLY_GROUP sg;
  HaArrayForEach(elm->supply_groups, sg, i)
    HaArrayForEach(sg->supplies, s, j)
    {
      ** delete the zone index set and zones of s **
      HnAssert(s->zone_index_set != NULL,
	"KheElmDeleteZones internal error");
      LSetFree(s->zone_index_set);
      s->zone_index_set = NULL;
      HaArrayClear(s->zones);
    }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheElmEdge(KHE_ELM_DEMAND d, KHE_ELM_SUPPLY s)                      */
/*                                                                           */
/*  Return true if an edge can be drawn between d and s, not counting        */
/*  cost and not concerned with zone restrictions.                           */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheElmEdge(KHE_ELM_DEMAND d, KHE_ELM_SUPPLY s)
{
  if( s->duration != KheMeetDuration(d->meet) )
    return false;
  else if( KheMeetAsst(d->meet) != NULL )
    return s->supply_group->target_meet == KheMeetAsst(d->meet) &&
      s->offset == KheMeetAsstOffset(d->meet);
  else
    return KheMeetAssignCheck(d->meet, s->supply_group->target_meet, s->offset);
}
*** */


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
  KheWMatchEval(elm->wmatch, &init_infeas, &init_cost);
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
    KheElmDemandGroupResetCurrRestriction(dg);
  }
  KheWMatchEval(elm->wmatch, &final_infeas, &final_cost);
  HnAssert(init_infeas==final_infeas, "KheElmAddRestrictions internal error 2");
  HnAssert(init_cost == final_cost, "KheElmAddRestrictions internal error 3");
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void SetRegularityCost(KHE_ELM_DEMAND_GROUP dg,                          */
/*    KHE_ELM_REGULARITY_COST *rc)                                           */
/*                                                                           */
/*  Set *rc to the regularity cost of dg.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
static void SetRegularityCost(KHE_ELM_DEMAND_GROUP dg,
  KHE_ELM_REGULARITY_COST *rc)
{
  int i, j;  KHE_COST junk, cost;  KHE_ELM_DEMAND_GROUP dg2;
  KHE_ELM_DEMAND d;  KHE_MEET meet;  KHE_ELM elm;
  KHE_ELM_SUPPLY s;
  elm = dg->elm;
  KheElmDemandGroupHasChanged(dg);
  KheWMatchEval(elm->wmatch, &rc->infeasibility, &junk);
  rc->without_children_cost = rc->with_children_cost = 0;
  HaArrayForEach(elm->demand_groups, dg2, i)
    HaArrayForEach(dg2->demands, d, j)
    {
      meet = KheElmDemandMeet(d);
      if( KheMeetAsst(meet) == NULL &&
	KheElmDemandBestSupply(d, &s, &cost) )
      {
	if( KheNodeChildCount(d->demand_group->node) == 0 )
	  rc->without_children_cost += cost;
	else
	  rc->with_children_cost += cost;
      }
    }
  rc->zones_cost = dg->curr_restriction->irregularity;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void DoTrySetsOfSize(KHE_ELM_DEMAND_GROUP dg, int zone_index, int count) */
/*                                                                           */
/*  Try all subsets of size count from zone_index onwards.                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static void DoTrySetsOfSize(KHE_ELM_DEMAND_GROUP dg, int zone_index, int count)
{
  KHE_ZONE zone;  int irregularity, i;  KHE_ELM_REGULARITY_COST rc;
  if( zone_index + count > HaArrayCount(dg->all_zones) )
  {
    ** not enough zones left to make a set of size count, so do nothing **
    return;
  }
  else if( count == 0 )
  {
    ** only subset now is the empty set, so time to test **
    SetRegularityCost(dg, &rc);
    if( DEBUG5 )
    {
      fprintf(stderr, "    trying {");
      KheElmRegularityCostDebug(&rc, 1, -1, stderr);
      fprintf(stderr, ": ");
      HaArrayForEach(dg->curr_restriction->array, zone, i)
      {
	if( i > 0 )
	  fprintf(stderr, ", ");
	** KheZoneDebug(zone, 1, -1, stderr); **
	fprintf(stderr, "%d", KheElmZoneShiftedIndex(zone));
      }
      fprintf(stderr, "}%s\n",
        KheElmRegularityCostLessThan(&rc, &dg->elm->best_reg_cost) ?
	  " (new best)" : "");
    }
    if( KheElmRegularityCostLessThan(&rc, &dg->elm->best_reg_cost) )
    {
      KheElmRestrictionAssign(dg->best_restriction, dg->curr_restriction);
      dg->elm->best_reg_cost = rc;
    }
  }
  else
  {
    ** try with zone, which the previous tests prove must exist **
    zone = HaArray(dg->all_zones, zone_index);
    irregularity = KheElmDemandGroupZoneIrregularity(dg, zone);
    KheElmRestrictionPushZone(dg->curr_restriction, zone, irregularity);
    DoTrySetsOfSize(dg, zone_index + 1, count - 1);
    KheElmRestrictionPopZone(dg->curr_restriction, zone, irregularity);

    ** try without zone **
    DoTrySetsOfSize(dg, zone_index + 1, count);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void TryFullSet(KHE_ELM_DEMAND_GROUP dg)                                 */
/*                                                                           */
/*  As a backstop, try the full set of demand group for dg.                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static void TryFullSet(KHE_ELM_DEMAND_GROUP dg)
{
  KHE_ELM_REGULARITY_COST rc;
  KheElmDemandGroupResetCurrRestriction(dg);
  SetRegularityCost(dg, &rc);
  if( DEBUG5 )
  {
    fprintf(stderr, "    trying full ");
    KheElmRegularityCostDebug(&rc, 1, -1, stderr);
    fprintf(stderr, ": %s%s\n", LSetShow(dg->curr_restriction->lset),
      KheElmRegularityCostLessThan(&rc, &dg->elm->best_reg_cost) ?
	" (new best)" : "");
  }
  if( KheElmRegularityCostLessThan(&rc, &dg->elm->best_reg_cost) )
  {
    KheElmRestrictionAssign(dg->best_restriction, dg->curr_restriction);
    dg->elm->best_reg_cost = rc;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElmImproveNodeRegularity(KHE_ELM elm)                            */
/*                                                                           */
/*  Improve the node regularity of elm with respect to zones.                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheOldElmImproveNodeRegularity(KHE_ELM elm)
{
  KHE_ELM_DEMAND_GROUP dg;  int i, j, count, infeas;  KHE_COST junk;
  KHE_ZONE zone;
  if( DEBUG4 )
  {
    fprintf(stderr, "[ KheElmImproveNodeRegularity(elm)\n");
    KheWMatchEval(elm->wmatch, &infeas, &junk);
    fprintf(stderr, "  infeas a %d\n", infeas);
  }
  if( KheNodeZoneCount(KheLayerParentNode(elm->layer)) > 0 )
  {
    ** add zones and zone index sets to the supply and demands of elm **
    KheWMatchSpecialModeBegin(elm->wmatch);
    KheElmAddZones(elm);
    if( KheElmAddRestrictions(elm) )
    {
      if( DEBUG4 )
      {
	KheWMatchEval(elm->wmatch, &infeas, &junk);
	fprintf(stderr, "  infeas b %d\n", infeas);
      }

      ** sort demand groups by decreasing duration, increasing children **
      HaArraySort(elm->demand_groups, &KheElmDemandGroupCmp);

      ** optionally diversify **
      if( KheOptionsDiversify(elm->options) )
	HaArrayForEach(elm->demand_groups, dg, i)
	  KheElmDemandGroupDiversify(dg);

      ** search **
      KheWMatchEval(elm->wmatch, &infeas, &junk);
      if( DEBUG4 )
	fprintf(stderr, "  infeas c %d\n", infeas);
      HaArrayForEach(elm->demand_groups, dg, i)
      {
	KheElmRestrictionClear(dg->best_restriction);
	KheElmRestrictionClear(dg->curr_restriction);
	KheElmRegularityCostInitToLarge(&elm->best_reg_cost, infeas);
	if( DEBUG4 )
	{
	  fprintf(stderr, "  [ restricting ");
	  KheNodeDebug(dg->node, 1, -1, stderr);
	  fprintf(stderr, " (%d children)\n", KheNodeChildCount(dg->node));
	  fprintf(stderr, "    init ");
	  KheElmRegularityCostDebug(&elm->best_reg_cost, 1, -1, stderr);
	  fprintf(stderr, "\n");
	}
	for( count = 1;  count <= 2;  count++ )
	  DoTrySetsOfSize(dg, 0, count);
	TryFullSet(dg);
	HnAssert(!KheElmRestrictionIsEmpty(dg->best_restriction),
	  "KheElmImproveNodeRegularity internal error");
	KheElmRestrictionAssign(dg->curr_restriction, dg->best_restriction);
	KheElmDemandGroupHasChanged(dg);
	if( DEBUG4 )
	{
	  fprintf(stderr, "  ] best ");
	  KheElmRegularityCostDebug(&elm->best_reg_cost, 1, -1, stderr);
	  fprintf(stderr, ": %s ", LSetShow(dg->curr_restriction->lset));
	  HaArrayForEach(dg->curr_restriction->array, zone, j)
	  {
	    if( j > 0 )
	      fprintf(stderr, ", ");
	    fprintf(stderr, "%d", KheElmZoneShiftedIndex(zone));
	  }
	  fprintf(stderr, "\n");
	}
      }
    }
    ** KheElmDeleteZones(elm); **
    KheWMatchSpecialModeEnd(elm->wmatch);
  }
  if( DEBUG4 )
  {
    KheWMatchEval(elm->wmatch, &infeas, &junk);
    fprintf(stderr, "  infeas d %d\n", infeas);
    fprintf(stderr, "] KheElmImproveNodeRegularity returning\n");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheElmLayerAssign(KHE_LAYER layer,                                  */
/*    KHE_SPREAD_EVENTS_CONSTRAINT sec, KHE_OPTIONS options)                 */
/*                                                                           */
/*  Assign layer using layer matching.  If present, sec influences the       */
/*  assignment to spread itself evenly through the time groups of sec.       */
/*                                                                           */
/*****************************************************************************/

bool KheElmLayerAssign(KHE_LAYER layer,
  KHE_SPREAD_EVENTS_CONSTRAINT sec, KHE_OPTIONS options)
{
  KHE_ELM elm;  bool res;  KHE_COST cost;  int infeas;  KHE_SOLN soln;
  HA_ARENA a;
  /* KHE_MEET_BOUND_GROUP mbg; */
  if( DEBUG11 || DEBUG13 )
  {
    fprintf(stderr, "[ KheElmLayerAssign(");
    KheLayerDebug(layer, 1, -1, stderr);
    fprintf(stderr, ", %s, options)\n", sec != NULL ? "sec" : "NULL");
  }
  soln = KheLayerSoln(layer);
  a = KheSolnArenaBegin(soln);
  elm = KheElmMake(layer, options, a);
  KheElmDetachIrregularMonitors(elm);
  KheElmSplitSupplies(elm, sec);
  if( DEBUG11 )
  {
    KheWMatchEval(elm->wmatch, &infeas, &cost);
    fprintf(stderr, "  after KheElmMake and Split: (%d, %.5f)\n",
      infeas, KheCostShow(cost));
    KheElmDebug(elm, 2, 2, stderr);
  }
  /* ***
  mbg = KheElmHandleIrregularMonitors(elm);
  if( DEBUG11 )
  {
    KheWMatchEval(elm->wmatch, &infeas, &cost);
    fprintf(stderr, "  after KheElmHandleIrregularMonitors: (%d, %.5f)\n",
      infeas, KheCostShow(cost));
    KheElmDebug(elm, 2, 2, stderr);
  }
  *** */
  KheElmReduceIrregularMonitors(elm);
  if( DEBUG11 )
  {
    KheWMatchEval(elm->wmatch, &infeas, &cost);
    fprintf(stderr, "  after KheElmReduceIrregularMonitors: (%d, %.5f)\n",
      infeas, KheCostShow(cost));
    KheElmDebug(elm, 2, 2, stderr);
  }
  KheElmImproveNodeRegularity(elm);
  if( DEBUG11 )
  {
    KheWMatchEval(elm->wmatch, &infeas, &cost);
    fprintf(stderr, "  after KheElmImproveNodeRegularity: (%d, %.5f)\n",
      infeas, KheCostShow(cost));
  }
  res = KheElmBestAssignMeets(elm);
  if( DEBUG11 )
  {
    KheWMatchEval(elm->wmatch, &infeas, &cost);
    fprintf(stderr, "  after KheElmAssignBest: (%d, %.5f)\n",
      infeas, KheCostShow(cost));
  }
  /* ***
  if( !KheMeetBoundGroupDelete(mbg) )
    HnAbort("KheElmLayerAssign internal error");
  *** */
  KheElmAttachIrregularMonitors(elm);
  /* KheElmDelete(elm); */
  KheSolnArenaEnd(soln, a);
  if( DEBUG11 || DEBUG13 )
    fprintf(stderr, "] KheElmLayerAssign returning %s\n",
      res ? "true" : "false");
  return res;
}

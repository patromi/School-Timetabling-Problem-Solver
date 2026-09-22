
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
/*  FILE:         khe_st_layer_node_match.c                                  */
/*  DESCRIPTION:  Layer node matchings                                       */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"
#include "khe_wmatch.h"
#include <limits.h>

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0

/* #define COST_ADJUST (KheCostMax >> 10) */

/*****************************************************************************/
/*                                                                           */
/*  KHE_ASST - one assignment.                                               */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_asst_rec {
  KHE_MEET		target_meet;		/* the meet assigned to      */
  int			target_offset;		/* offset in target_meet     */
  int			durn;			/* assigned meet's duration  */
} *KHE_ASST;

typedef HA_ARRAY(KHE_ASST) ARRAY_KHE_ASST;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SUPPLY_NODE (private) - the assignments of one node                  */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_supply_node_rec {
  ARRAY_KHE_ASST	assts;			/* assignments of node       */
  KHE_WMATCH_NODE	wmatch_node;		/* supply node in wmatch     */
} *KHE_SUPPLY_NODE;

typedef HA_ARRAY(KHE_SUPPLY_NODE) ARRAY_KHE_SUPPLY_NODE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_DEMAND_NODE (private) - one node requiring assignment                */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_MEET) ARRAY_KHE_MEET;

typedef struct khe_demand_node_rec {
  KHE_NODE			node;		/* the node demanding asst   */
  ARRAY_KHE_MEET		meets;		/* the meets of node, sorted */
  KHE_WMATCH_NODE		wmatch_node;	/* demand node in wmatch     */
} *KHE_DEMAND_NODE;

typedef HA_ARRAY(KHE_DEMAND_NODE) ARRAY_KHE_DEMAND_NODE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_PART_LAYER - set of compatible nodes requiring assignment            */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_part_layer_rec {
  ARRAY_KHE_DEMAND_NODE	demand_nodes;		/* demand nodes              */
  ARRAY_KHE_SUPPLY_NODE	supply_nodes;		/* supply nodes              */
  KHE_WMATCH		wmatch;			/* the matching graph        */
} *KHE_PART_LAYER;

typedef HA_ARRAY(KHE_PART_LAYER) ARRAY_KHE_PART_LAYER;


/*****************************************************************************/
/*                                                                           */
/*  KHE_LAYER_NODE_SOLVER - a solver for this entire operation               */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_layer_node_solver_rec {
  HA_ARENA		arena;
  ARRAY_KHE_PART_LAYER	part_layers;		/* part-layers to solve      */
} *KHE_LAYER_NODE_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "assignments"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ASST KheAsstMake(void)                                               */
/*                                                                           */
/*  Make an asst object with placeholder attributes; these will be changed   */
/*  to real ones later by KheAsstAssign().                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_ASST KheAsstMake(HA_ARENA a)
{
  KHE_ASST res;
  HaMake(res, a);
  res->target_meet = NULL;
  res->target_offset = 0;
  res->durn = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAsstAssign(KHE_ASST asst, KHE_MEET target_meet,                  */
/*    int target_offset, int durn)                                           */
/*                                                                           */
/*  Assign these values to the attributes of asst.                           */
/*                                                                           */
/*****************************************************************************/

static void KheAsstAssign(KHE_ASST asst, KHE_MEET target_meet,
  int target_offset, int durn)
{
  asst->target_meet = target_meet;
  asst->target_offset = target_offset;
  asst->durn = durn;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAsstDelete(KHE_ASST asst)                                        */
/*                                                                           */
/*  Free the memory consumed by asst.                                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheAsstDelete(KHE_ASST asst)
{
  MFree(asst);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheAsstDebug(KHE_ASST asst, FILE *fp)                               */
/*                                                                           */
/*  Debug print of asst onto fp.                                             */
/*                                                                           */
/*****************************************************************************/

static void KheAsstDebug(KHE_ASST asst, FILE *fp)
{
  KHE_TIME t;  KHE_INSTANCE ins;
  if( asst->target_meet == NULL )
    fprintf(fp, "(not loaded)");
  else if( KheMeetIsCycleMeet(asst->target_meet) )
  {
    t = KheMeetAsstTime(asst->target_meet);
    ins = KheTimeInstance(t);
    t = KheInstanceTime(ins, KheTimeIndex(t) + asst->target_offset);
    fprintf(fp, "/%s/d%d", KheTimeId(t) == NULL ? "-" : KheTimeId(t),
      asst->durn);
  }
  else
  {
    KheMeetDebug(asst->target_meet, 1, -1, fp);
    fprintf(fp, "+%dd%d", asst->target_offset, asst->durn);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "supply nodes"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SUPPLY_NODE KheSupplyNodeMake(int asst_count, KHE_WMATCH wmatch)     */
/*                                                                           */
/*  Make a supply node.  Its has asst_count assignments, and its wmatch      */
/*  node goes into wmatch.                                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_SUPPLY_NODE KheSupplyNodeMake(int asst_count, KHE_WMATCH wmatch,
  HA_ARENA a)
{
  KHE_SUPPLY_NODE res;  int i;
  HaMake(res, a);
  HaArrayInit(res->assts, a);
  for( i = 0;  i < asst_count;  i++ )
    HaArrayAddLast(res->assts, KheAsstMake(a));
  res->wmatch_node = KheWMatchSupplyNodeMake(wmatch, res, NULL);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSupplyNodeLoad(KHE_SUPPLY_NODE sn, KHE_DEMAND_NODE dn)           */
/*                                                                           */
/*  Load the current assignments of the meets of dn into sn.                 */
/*                                                                           */
/*****************************************************************************/

static void KheSupplyNodeLoad(KHE_SUPPLY_NODE sn, KHE_DEMAND_NODE dn)
{
  KHE_MEET meet;  KHE_ASST asst;  int i;
  HnAssert(HaArrayCount(sn->assts) == HaArrayCount(dn->meets),
    "KheSupplyNodeLoad internal error 1");
  HaArrayForEach(dn->meets, meet, i)
  {
    asst = HaArray(sn->assts, i);
    HnAssert(KheMeetAsst(meet) != NULL, "KheSupplyNodeLoad internal error 2");
    KheAsstAssign(asst, KheMeetAsst(meet), KheMeetAsstOffset(meet),
      KheMeetDuration(meet));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSupplyNodeDelete(KHE_SUPPLY_NODE sn)                             */
/*                                                                           */
/*  Delete sn.  Its wmatch node will be deleted separately.                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSupplyNodeDelete(KHE_SUPPLY_NODE sn)
{
  while( HaArrayCount(sn->assts) > 0 )
    KheAsstDelete(MArrayRemoveLast(sn->assts));
  MFree(sn);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSupplyNodeDebug(KHE_SUPPLY_NODE sn, int verbosity,               */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug function for printing one supply node.                             */
/*                                                                           */
/*****************************************************************************/

static void KheSupplyNodeDebug(KHE_SUPPLY_NODE sn, int verbosity,
  int indent, FILE *fp)
{
  KHE_ASST asst;  int i;
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[");
    HaArrayForEach(sn->assts, asst, i)
    {
      if( i > 0 )
	fprintf(fp, ", ");
      KheAsstDebug(asst, fp);
    }
    fprintf(fp, "]");
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "demand nodes"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DEMAND_NODE KheDemandNodeMake(KHE_NODE node)                         */
/*                                                                           */
/*  Make a demand node.  Its meets are the meets of node, sorted by          */
/*  decreasing duration; its wmatch node is NULL initially.                  */
/*                                                                           */
/*****************************************************************************/

static KHE_DEMAND_NODE KheDemandNodeMake(KHE_NODE node, HA_ARENA a)
{
  KHE_DEMAND_NODE res;  int i;
  HaMake(res, a);
  res->node = node;
  HaArrayInit(res->meets, a);
  for( i = 0;  i < KheNodeMeetCount(node);  i++ )
    HaArrayAddLast(res->meets, KheNodeMeet(node, i));
  HaArraySort(res->meets, &KheMeetDecreasingDurationCmp);
  res->wmatch_node = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMaxLayerIndexInFirstButNotSecond(KHE_NODE node1, KHE_NODE node2)  */
/*                                                                           */
/*  Return the maximum layer index over all parent layers of node1 that are  */
/*  not parent layers of node2.  Return -1 if there are no such layers.      */
/*                                                                           */
/*****************************************************************************/

static int KheMaxLayerIndexInFirstButNotSecond(KHE_NODE node1, KHE_NODE node2)
{
  KHE_LAYER ly1;  int res, i;
  res = -1;
  for( i = 0;  i < KheNodeParentLayerCount(node1);  i++ )
  {
    ly1 = KheNodeParentLayer(node1, i);
    if( KheLayerParentNodeIndex(ly1) > res && !KheLayerContains(ly1, node2) )
      res = KheLayerParentNodeIndex(ly1);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDemandNodeCompatibleCmp(KHE_DEMAND_NODE dn1, KHE_DEMAND_NODE dn2) */
/*                                                                           */
/*  Comparison function which returns 0 when dn1 and dn2 are compatible,     */
/*  and a non-zero value suitable for sorting demand nodes so that           */
/*  compatible ones are brought together otherwise.  Two nodes are           */
/*  compatible if they lie in the same parent layers and have meets of       */
/*  the same durations.                                                      */
/*                                                                           */
/*  We don't worry about meet domains here; the edge function takes care     */
/*  of them.                                                                 */
/*                                                                           */
/*****************************************************************************/
static void KheDemandNodeDebug(KHE_DEMAND_NODE dn, int verbosity,
  int indent, FILE *fp);

static int KheDemandNodeCompatibleCmp(KHE_DEMAND_NODE dn1, KHE_DEMAND_NODE dn2)
{
  int max_layer1_index, max_layer2_index, i;  KHE_MEET meet1, meet2;

  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheDemandNodeCompatibleCmp(dn1, dn2)\n");
    KheDemandNodeDebug(dn1, 2, 2, stderr);
    KheDemandNodeDebug(dn2, 2, 2, stderr);
  }

  /* check number of parent layers */
  if( KheNodeParentLayerCount(dn1->node) != KheNodeParentLayerCount(dn2->node) )
  {
    if( DEBUG2 )
      fprintf(stderr, "] differed parent layer counts: %d vs %d\n",
	KheNodeParentLayerCount(dn1->node), KheNodeParentLayerCount(dn2->node));
    return KheNodeParentLayerCount(dn1->node) -
      KheNodeParentLayerCount(dn2->node);
  }

  /* check max index of different layers */
  max_layer1_index = KheMaxLayerIndexInFirstButNotSecond(dn1->node, dn2->node);
  max_layer2_index = KheMaxLayerIndexInFirstButNotSecond(dn2->node, dn1->node);
  if( max_layer1_index != max_layer2_index )
  {
    if( DEBUG2 )
      fprintf(stderr, "] different max layer indexes: %d vs %d\n",
	max_layer1_index, max_layer2_index);
    return max_layer1_index - max_layer2_index;
  }

  /* check number of meets */
  if( HaArrayCount(dn1->meets) != HaArrayCount(dn2->meets) )
  {
    if( DEBUG2 )
      fprintf(stderr, "] different number of meets: %d vs %d\n",
	HaArrayCount(dn1->meets), HaArrayCount(dn2->meets));
    return HaArrayCount(dn1->meets) - HaArrayCount(dn2->meets);
  }

  /* check durations of meets (nb meets are sorted by decreasing duration) */
  for( i = 0;  i < HaArrayCount(dn1->meets);  i++ )
  {
    meet1 = HaArray(dn1->meets, i);
    meet2 = HaArray(dn2->meets, i);
    if( KheMeetDuration(meet1) != KheMeetDuration(meet2) )
    {
      if( DEBUG2 )
	fprintf(stderr, "] different duration of meet %d: %d vs %d\n",
	  i, KheMeetDuration(meet1), KheMeetDuration(meet2));
      return KheMeetDuration(meet1) - KheMeetDuration(meet2);
    }
  }

  /* same parent layers, same meet durations - these nodes are compatible */
  if( DEBUG2 )
    fprintf(stderr, "] compatible, returning 0\n");
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDemandNodeCmp(const void *t1, const void *t2)                     */
/*                                                                           */
/*  Comparison function for sorting an array of demand nodes so that         */
/*  compatible ones are brought together.                                    */
/*                                                                           */
/*****************************************************************************/

static int KheDemandNodeCmp(const void *t1, const void *t2)
{
  KHE_DEMAND_NODE dn1 = * (KHE_DEMAND_NODE *) t1;
  KHE_DEMAND_NODE dn2 = * (KHE_DEMAND_NODE *) t2;
  return KheDemandNodeCompatibleCmp(dn1, dn2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDemandNodeUnAssign(KHE_DEMAND_NODE dn)                           */
/*                                                                           */
/*  Unassign the meets of dn.                                                */
/*                                                                           */
/*****************************************************************************/

static void KheDemandNodeUnAssign(KHE_DEMAND_NODE dn)
{
  KHE_MEET meet;  int i;
  HaArrayForEach(dn->meets, meet, i)
    KheMeetUnAssign(meet);  /* layer matching, not sure about this */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDemandNodeAssign(KHE_DEMAND_NODE dn, KHE_SUPPLY_NODE sn)         */
/*                                                                           */
/*  Assign the meets of dn as dictated by sn.                                */
/*                                                                           */
/*****************************************************************************/

static void KheDemandNodeAssign(KHE_DEMAND_NODE dn, KHE_SUPPLY_NODE sn)
{
  KHE_MEET meet;  KHE_ASST asst;  int i;
  HnAssert(HaArrayCount(sn->assts) == HaArrayCount(dn->meets),
    "KheDemandNodeAssign internal error 1");
  HaArrayForEach(dn->meets, meet, i)
  {
    asst = HaArray(sn->assts, i);
    HnAssert(asst->durn == KheMeetDuration(meet),
      "KheDemandNodeAssign internal error 2");
    if( !KheMeetAssign(meet, asst->target_meet, asst->target_offset) )
      HnAbort("KheDemandNodeAssign internal error 3");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDemandNodeAssignFromMatching(KHE_DEMAND_NODE dn)                 */
/*                                                                           */
/*  Assign the meets of dn, using the matching to find the assignments.      */
/*                                                                           */
/*****************************************************************************/

static void KheDemandNodeAssignFromMatching(KHE_DEMAND_NODE dn)
{
  KHE_COST cost;  KHE_SUPPLY_NODE sn;
  sn = (KHE_SUPPLY_NODE) KheWMatchDemandNodeAssignedTo(dn->wmatch_node, &cost);
  KheDemandNodeAssign(dn, sn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDemandNodeDelete(KHE_DEMAND_NODE dn)                             */
/*                                                                           */
/*  Free dn.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheDemandNodeDelete(KHE_DEMAND_NODE dn)
{
  MArrayFree(dn->meets);
  MFree(dn);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheDemandNodeEdgeFn(KHE_DEMAND_NODE dn,                             */
/*    KHE_SUPPLY_NODE sn, KHE_COST *cost)                                    */
/*                                                                           */
/*  Edge function defining whether dn and sn are connected, and if so        */
/*  what the edge cost is.                                                   */
/*                                                                           */
/*****************************************************************************/

static bool KheDemandNodeEdgeFn(KHE_DEMAND_NODE dn,
  KHE_SUPPLY_NODE sn, KHE_COST *cost)
{
  int i, j;  KHE_MEET meet;  KHE_ASST asst;  KHE_COST cost_after;
  KHE_SOLN soln;
  if( cost == NULL )
  {
    /* just need to know whether assignable, not the cost */
    HaArrayForEach(dn->meets, meet, i)
    {
      asst = HaArray(sn->assts, i);
      HnAssert(KheMeetDuration(meet) == asst->durn,
	"KheDemandNodeEdgeFn internal error 1");
      if( !KheMeetAssignCheck(meet, asst->target_meet, asst->target_offset) )
	return false;
    }
    return true;
  }
  else if( HaArrayCount(dn->meets) == 0 )
  {
    /* no meets, cost is 0 (will probably never happen) */
    *cost = 0L /* + COST_ADJUST */;
    return true;
  }
  else
  {

    /* assign as much as we can */
    soln = KheMeetSoln(HaArrayFirst(dn->meets));
    /* cost_before = KheSolnCost(soln);*/
    HaArrayForEach(dn->meets, meet, i)
    {
      asst = HaArray(sn->assts, i);
      HnAssert(KheMeetDuration(meet) == asst->durn,
	"KheDemandNodeEdgeFn internal error 2");
      if( !KheMeetAssign(meet, asst->target_meet, asst->target_offset) )
	break;
    }
    cost_after = KheSolnCost(soln);
    if( DEBUG4 && i == HaArrayCount(dn->meets) )
    {
      fprintf(stderr, "  cost of ");
      KheDemandNodeDebug(dn, 1, -1, stderr);
      fprintf(stderr, " --> ");
      KheSupplyNodeDebug(sn, 1, -1, stderr);
      fprintf(stderr, "  is %.5f\n", KheCostShow(cost_after));
      /* ***
      fprintf(stderr, "  is %.5f - %.5f\n",
	KheCostShow(cost_after), KheCostShow(cost_before));
      *** */
    }

    /* unassign everything that was assigned and return true if all */
    for( j = 0;  j < i;  j++ )
      KheMeetUnAssign(HaArray(dn->meets, j));  /* OK, unassigns prev asst */
    /* *cost = (cost_after - cost_before) + COST_ADJUST; */
    *cost = cost_after;
    return i == HaArrayCount(dn->meets);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDemandNodeDebug(KHE_DEMAND_NODE dn, int verbosity,               */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug function for printing one demand node.                             */
/*                                                                           */
/*****************************************************************************/

static void KheDemandNodeDebug(KHE_DEMAND_NODE dn, int verbosity,
  int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    if( HaArrayCount(dn->meets) > 0 )
      KheMeetDebug(HaArrayFirst(dn->meets), 1, -1, fp);
    else
      fprintf(fp, "Node %d", KheNodeSolnIndex(dn->node));
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "part-layers"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KhePartLayerDebug(KHE_PART_LAYER pl, int verbosity,                 */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug function for printing one part-layer.                              */
/*                                                                           */
/*****************************************************************************/

static void KhePartLayerDebug(KHE_PART_LAYER pl, int verbosity,
  int indent, FILE *fp)
{
  KHE_DEMAND_NODE dn;  KHE_SUPPLY_NODE sn;  int i;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ Part Layer\n", indent, "");
    HaArrayForEach(pl->demand_nodes, dn, i)
      KheDemandNodeDebug(dn, verbosity, indent + 2, fp);
    HaArrayForEach(pl->supply_nodes, sn, i)
      KheSupplyNodeDebug(sn, verbosity, indent + 2, fp);
    KheWMatchDebug(pl->wmatch, verbosity, indent + 2, stderr);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartLayerCostDebug(void *back, KHE_COST cost, FILE *fp)          */
/*                                                                           */
/*  Cost debug function.                                                     */
/*                                                                           */
/*****************************************************************************/

static void KhePartLayerCostDebug(void *back, KHE_COST cost, FILE *fp)
{
  fprintf(fp, "%.5f", KheCostShow(cost));
  /* ***
  int hard_cost, soft_cost;
  if( cost == 0 )
    fprintf(fp, "0");
  else
  {
    hard_cost = KheHardCost(cost - COST_ADJUST);
    soft_cost = KheSoftCost(cost - COST_ADJUST);
    fprintf(fp, "(%d,%d)", hard_cost, soft_cost);
  }
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PART_LAYER KhePartLayerMake(void)                                    */
/*                                                                           */
/*  Make a new part-layer object.                                            */
/*                                                                           */
/*****************************************************************************/

static KHE_PART_LAYER KhePartLayerMake(HA_ARENA a)
{
  KHE_PART_LAYER res;
  HaMake(res, a);
  HaArrayInit(res->demand_nodes, a);
  HaArrayInit(res->supply_nodes, a);
  res->wmatch = KheWMatchMake(res, (GENERIC_DEBUG_FN) &KhePartLayerDebug,
    (GENERIC_DEBUG_FN) &KheDemandNodeDebug,
    (GENERIC_DEBUG_FN) &KheSupplyNodeDebug,
    (KHE_WMATCH_EDGE_FN) &KheDemandNodeEdgeFn, &KhePartLayerCostDebug, 0L, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartLayerAddDemandNode(KHE_PART_LAYER pl, KHE_DEMAND_NODE dn)    */
/*                                                                           */
/*  Add dn to pl, and also add a corresponding supply node, and add a        */
/*  wmatch node to dn.                                                       */
/*                                                                           */
/*****************************************************************************/

static void KhePartLayerAddDemandNode(KHE_PART_LAYER pl, KHE_DEMAND_NODE dn,
  HA_ARENA a)
{
  KHE_SUPPLY_NODE sn;
  HaArrayAddLast(pl->demand_nodes, dn);
  sn = KheSupplyNodeMake(HaArrayCount(dn->meets), pl->wmatch, a);
  HaArrayAddLast(pl->supply_nodes, sn);
  dn->wmatch_node = KheWMatchDemandNodeMake(pl->wmatch, dn, NULL, 0);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartLayerSolve(KHE_PART_LAYER pl)                                */
/*                                                                           */
/*  Solve pl and return true if there was a cost decrease.                   */
/*                                                                           */
/*****************************************************************************/

static bool KhePartLayerSolve(KHE_PART_LAYER pl)
{
  KHE_DEMAND_NODE dn;  KHE_SUPPLY_NODE sn;  int i;  KHE_COST cost, junk;
  KHE_SOLN soln;

  /* find soln and get its cost */
  if( DEBUG3 )
    fprintf(stderr, "[ KhePartLayerSolve(pl)\n");
  HnAssert(HaArrayCount(pl->demand_nodes) > 0,
    "KhePartLayerSolve internal error 1");
  dn = HaArrayFirst(pl->demand_nodes);
  soln = KheNodeSoln(dn->node);
  cost = KheSolnCost(soln);

  /* load current assignments of demand nodes into supply nodes, and deassign */
  HaArrayForEach(pl->demand_nodes, dn, i)
  {
    sn = HaArray(pl->supply_nodes, i);
    KheSupplyNodeLoad(sn, dn);
    KheWMatchSupplyNodeNotifyDirty(sn->wmatch_node);
    KheDemandNodeUnAssign(dn);
  }

  /* find a maximum matching of minimum cost */
  if( DEBUG3 )
  {
    fprintf(stderr, "  before solving:\n");
    KhePartLayerDebug(pl, 4, 4, stderr);
  }
  if( !KheWMatchFullyMatchable(pl->wmatch, &junk, 0) )
    HnAbort("KhePartLayerSolve internal error 2");
  if( DEBUG3 )
  {
    fprintf(stderr, "  after solving:\n");
    KhePartLayerDebug(pl, 4, 4, stderr);
  }

  /* install the min-cost matching */
  HaArrayForEach(pl->demand_nodes, dn, i)
    KheDemandNodeAssignFromMatching(dn);

  if( KheSolnCost(soln) < cost )
  {
    /* improvement, so stay with this */
    if( DEBUG3 || DEBUG5 )
      fprintf(stderr, "%c KhePartLayerSolve ret true (pre %.5f, post %.5f)\n",
	DEBUG3 ? ']' : ' ', KheCostShow(cost), KheCostShow(KheSolnCost(soln)));
    return true;
  }
  else
  {
    /* inferior, so return to original */
    HaArrayForEach(pl->demand_nodes, dn, i)
      KheDemandNodeUnAssign(dn);
    HaArrayForEach(pl->demand_nodes, dn, i)
      KheDemandNodeAssign(dn, HaArray(pl->supply_nodes, i));
    HnAssert(KheSolnCost(soln) == cost, "KhePartLayerSolve internal error");
    if( DEBUG3 || DEBUG5 )
      fprintf(stderr, "%c KhePartLayerSolve ret false\n", DEBUG3 ? ']' : ' ');
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartLayerDelete(KHE_PART_LAYER pl)                               */
/*                                                                           */
/*  Delete pl.                                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KhePartLayerDelete(KHE_PART_LAYER pl)
{
  while( HaArrayCount(pl->demand_nodes) > 0 )
    KheDemandNodeDelete(MArrayRemoveLast(pl->demand_nodes));
  while( HaArrayCount(pl->supply_nodes) > 0 )
    KheSupplyNodeDelete(MArrayRemoveLast(pl->supply_nodes));
  KheWMatchDelete(pl->wmatch);
  MFree(pl);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "layer node solver"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeIsFullyAssigned(KHE_NODE node)                               */
/*                                                                           */
/*  Return true if every meet of node is assigned.                           */
/*                                                                           */
/*  Implementation node.  Could just compare NodeDuration with               */
/*  NodeAssignedDuration here.                                               */
/*                                                                           */
/*****************************************************************************/

static bool KheNodeIsFullyAssigned(KHE_NODE node)
{
  KHE_MEET meet;  int i;
  for( i = 0;  i < KheNodeMeetCount(node);  i++ )
  {
    meet = KheNodeMeet(node, i);
    if( KheMeetAsst(meet) == NULL )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LAYER_NODE_SOLVER KheLayerNodeSolverMake(KHE_NODE parent_node,       */
/*    KHE_LAYER layer)                                                       */
/*                                                                           */
/*  Make a layer node solver for parent_node, assuming it has child layers.  */
/*                                                                           */
/*****************************************************************************/

static KHE_LAYER_NODE_SOLVER KheLayerNodeSolverMake(KHE_NODE parent_node,
  KHE_LAYER layer, HA_ARENA a)
{
  ARRAY_KHE_DEMAND_NODE demand_nodes;  KHE_DEMAND_NODE dn, prev_dn;
  KHE_LAYER_NODE_SOLVER res;  int i;  KHE_PART_LAYER pl;  KHE_NODE child_node;
  /* HA_ARENA a; */

  if( DEBUG1 )
  {
    if( parent_node != NULL )
      fprintf(stderr, "[ KheLayerNodeSolverMake(Node %d)\n",
	KheNodeSolnIndex(parent_node));
    else
      fprintf(stderr, "[ KheLayerNodeSolverMake(Layer %d)\n",
	KheLayerParentNodeIndex(layer));
  }

  /* make the layer node solver object */
  HaMake(res, a);
  res->arena = a;
  HaArrayInit(res->part_layers, a);

  /* build one demand node for each fully assigned child node */
  HaArrayInit(demand_nodes, a);
  if( parent_node != NULL )
    for( i = 0;  i < KheNodeChildCount(parent_node);  i++ )
    {
      child_node = KheNodeChild(parent_node, i);
      if( KheNodeIsFullyAssigned(child_node) )
	HaArrayAddLast(demand_nodes, KheDemandNodeMake(child_node, a));
    }
  else
    for( i = 0;  i < KheLayerChildNodeCount(layer);  i++ )
    {
      child_node = KheLayerChildNode(layer, i);
      if( KheNodeIsFullyAssigned(child_node) )
	HaArrayAddLast(demand_nodes, KheDemandNodeMake(child_node, a));
    }

  /* sort the demand nodes to bring compatible ones together */
  HaArraySort(demand_nodes, &KheDemandNodeCmp);

  /* make one part layer for each run of compatible demand nodes */
  pl = NULL;  prev_dn = NULL;
  HaArrayForEach(demand_nodes, dn, i)
  {
    /* start a new part layer if required */
    if( prev_dn == NULL || KheDemandNodeCompatibleCmp(prev_dn, dn) != 0 )
    {
      pl = KhePartLayerMake(a);
      HaArrayAddLast(res->part_layers, pl);
    }

    /* add dn to pl */
    KhePartLayerAddDemandNode(pl, dn, a);
    prev_dn = dn;
  }
  HaArrayFree(demand_nodes);

  /* remove part layers with only one demand node */
  HaArrayForEach(res->part_layers, pl, i)
    if( HaArrayCount(pl->demand_nodes) == 1 )
    {
      /* KhePartLayerDelete(pl); */
      HaArrayDeleteAndShift(res->part_layers, i);
      i--;
    }

  if( DEBUG1 )
  {
    HaArrayForEach(res->part_layers, pl, i)
      KhePartLayerDebug(pl, 2, 2, stderr);
    fprintf(stderr, "] KheLayerNodeSolverMake returning\n");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerNodeSolverSolve(KHE_LAYER_NODE_SOLVER lns)                  */
/*                                                                           */
/*  Solve lns, by repeatedly solving its part-layers until no improvement.   */
/*                                                                           */
/*****************************************************************************/

static void KheLayerNodeSolverSolve(KHE_LAYER_NODE_SOLVER lns)
{
  int i, fail_count;
  fail_count = i = 0;
  while( fail_count < HaArrayCount(lns->part_layers) )
  {
    i = (i + 1) % HaArrayCount(lns->part_layers);
    if( KhePartLayerSolve(HaArray(lns->part_layers, i)) )
      fail_count = 0;
    else
      fail_count++;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerNodeSolverDelete(KHE_LAYER_NODE_SOLVER lns)                 */
/*                                                                           */
/*  Delete lns.                                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheLayerNodeSolverDelete(KHE_LAYER_NODE_SOLVER lns)
{
  HaAre naDelete(lns->arena);
  ** ***
  while( HaArrayCount(lns->part_layers) > 0 )
    KhePartLayerDelete(MArrayRemoveLast(lns->part_layers));
  MFree(lns);
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "the solver"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerNodeMatchingNodeRepairTimes(KHE_NODE parent_node,           */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Repair the assignments of the child nodes of parent_node by repeatedly   */
/*  reassigning compatible nodes using weighted bipartite matching.          */
/*                                                                           */
/*  Return true if the cost of the solution improved.                        */
/*                                                                           */
/*****************************************************************************/

bool KheLayerNodeMatchingNodeRepairTimes(KHE_NODE parent_node,
  KHE_OPTIONS options)
{
  KHE_LAYER_NODE_SOLVER lns;  bool add_layers;  KHE_SOLN soln;
  KHE_COST initial_cost, final_cost;  HA_ARENA a;
  soln = KheNodeSoln(parent_node);
  a = KheSolnArenaBegin(soln);
  initial_cost = KheSolnCost(soln);
  if( DEBUG1 )
    fprintf(stderr, "[ KheLayerNodeMatchingNodeRepairTimes(Node %d): %.5f\n",
      KheNodeSolnIndex(parent_node), KheCostShow(initial_cost));

  /* make child layers if required, and build a layer node solver */
  add_layers = (KheNodeChildLayerCount(parent_node) == 0);
  if( add_layers )
    KheNodeChildLayersMake(parent_node);
  lns = KheLayerNodeSolverMake(parent_node, NULL, a);

  /* solve lns */
  KheLayerNodeSolverSolve(lns);

  /* clean up and exit */
  KheSolnArenaEnd(soln, a);
  /* KheLayerNodeSolverDelete(lns); */
  if( add_layers )
    KheNodeChildLayersDelete(parent_node);
  final_cost = KheSolnCost(soln);
  if( DEBUG1 )
    fprintf(stderr,"] KheLayerNodeMatchingNodeRepairTimes returning %s: %.5f\n",
      final_cost < initial_cost ? "true" : "false", KheCostShow(final_cost));
  return final_cost < initial_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerNodeMatchingLayerRepairTimes(KHE_LAYER layer,               */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Like the above, but on the child nodes of layer rather than on the       */
/*  child nodes of parent_node.                                              */
/*                                                                           */
/*****************************************************************************/

bool KheLayerNodeMatchingLayerRepairTimes(KHE_LAYER layer, KHE_OPTIONS options)
{
  KHE_LAYER_NODE_SOLVER lns;  KHE_SOLN soln;
  KHE_COST initial_cost, final_cost;  HA_ARENA a;
  soln = KheLayerSoln(layer);
  a = KheSolnArenaBegin(soln);
  initial_cost = KheSolnCost(soln);
  if( DEBUG1 )
    fprintf(stderr, "[ KheLayerNodeMatchingLayerRepairTimes(Layer %d): %.5f\n",
      KheLayerParentNodeIndex(layer), KheCostShow(initial_cost));

  /* build a layer node solver */
  lns = KheLayerNodeSolverMake(NULL, layer, a);

  /* solve lns */
  KheLayerNodeSolverSolve(lns);

  /* clean up and exit */
  KheSolnArenaEnd(soln, a);
  /* KheLayerNodeSolverDelete(lns); */

  final_cost = KheSolnCost(soln);;
  if( DEBUG1 )
    fprintf(stderr,
      "] KheLayerNodeMatchingLayerRepairTimes returning %s: %.5f\n",
      final_cost < initial_cost ? "true" : "false", KheCostShow(final_cost));
  return final_cost < initial_cost;
}

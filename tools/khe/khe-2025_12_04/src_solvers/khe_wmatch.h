
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
/*  FILE:         khe_wmatch.h                                               */
/*  MODULE:       Weighted bipartite matching                                */
/*                                                                           */
/*  This module offers weighted bipartite matching.                          */
/*                                                                           */
/*  Instead of operations to insert and delete edges and change the cost     */
/*  of an edge, one passes in an "edge function" at the start which the      */
/*  module calls to find out whether there is an edge between two nodes,     */
/*  and if so what its cost is.  The caller may notify the module that a     */
/*  node is dirty; this causes the module to call the edge function later    */
/*  when it needs to bring the graph up to date prior to solving it.         */
/*                                                                           */
/*  Each node may have a category, and demand nodes may have an integer      */
/*  category strength.  If the tabu_cost parameter of KheWMatchNew is        */
/*  non-zero, then this module tries to ensure that the matching never       */
/*  contains two or more edges whose endpoints have the same categories      */
/*  as each other, and whose demand nodes have non-zero total strength,      */
/*  by penalizing paths that introduce such edges by tabu_cost.              */
/*                                                                           */
/*  An edge of the matching may be fixed, meaning that the two nodes must    */
/*  be assigned to each other, and later it may be unfixed.  A fixed edge    */
/*  is considered to have zero cost when this is done.                       */
/*                                                                           */
/*  Node dirtying and edge fixing operate in two modes.  In the ordinary     */
/*  mode, when a node is marked dirty or its edge is fixed, it is assumed    */
/*  that all its incident edges and their costs are garbage and will have    */
/*  to be recalculated from scratch when the matching is brought up to date. */
/*                                                                           */
/*  In the special mode, when a node is marked dirty or its edge is fixed,   */
/*  it is assumed that all the edges and their costs are still valid, but    */
/*  that some are to be ignored temporarily, and brought back later.  The    */
/*  edge function is still consulted, but only to say whether each of the    */
/*  existing edges is in or out, not to determine the cost of any edges.     */
/*  (The call on the edge function will pass a NULL cost parameter in this   */
/*  case.)  This saves a lot of time when calculating the cost is expensive. */
/*                                                                           */
/*  Two demand nodes may be declared to be equivalent.  This means that      */
/*  the edges of one are always the same as the edges of the other, and so   */
/*  it is not necessary to calculate them twice when building the matching.  */
/*                                                                           */
/*****************************************************************************/
#ifndef KHE_WMATCH_HEADER_FILE
#define KHE_WMATCH_HEADER_FILE
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "howard_a.h"

/* object type declarations */
typedef struct khe_wmatch_category_rec	*KHE_WMATCH_CATEGORY;
typedef struct khe_wmatch_node_rec	*KHE_WMATCH_NODE;
typedef struct khe_wmatch_rec		*KHE_WMATCH;

/* function type declarations */
typedef void (*GENERIC_DEBUG_FN)(void *value, int verbosity,
  int indent, FILE *fp);
typedef void (*COST_DEBUG_FN)(void *back, int64_t cost, FILE *fp);
typedef bool (*KHE_WMATCH_EDGE_FN)(void *demand_back,
  void *supply_back, int64_t *cost);

/* categories */
extern KHE_WMATCH_CATEGORY KheWMatchNewCategory(KHE_WMATCH m);
extern KHE_WMATCH KheWMatchCategoryWMatch(KHE_WMATCH_CATEGORY category);
extern void KheWMatchCategoryDelete(KHE_WMATCH_CATEGORY category);

/* demand nodes */
extern KHE_WMATCH_NODE KheWMatchDemandNodeMake(KHE_WMATCH m, void *back,
  KHE_WMATCH_CATEGORY category, int category_strength);
extern KHE_WMATCH KheWMatchDemandNodeWMatch(KHE_WMATCH_NODE demand_node);
extern void *KheWMatchDemandNodeBack(KHE_WMATCH_NODE demand_node);
extern KHE_WMATCH_CATEGORY KheWMatchDemandNodeCategory(
  KHE_WMATCH_NODE demand_node);
extern int KheWMatchDemandNodeCategoryStrength(KHE_WMATCH_NODE demand_node);
extern void KheWMatchDemandNodeNotifyDirty(KHE_WMATCH_NODE demand_node);
extern void KheWMatchDemandNodeDelete(KHE_WMATCH_NODE demand_node);
extern void KheWMatchDemandNodeAddToCategory(KHE_WMATCH_NODE demand_node,
  KHE_WMATCH_CATEGORY category, int category_strength);
extern void KheWMatchDemandNodeDeleteFromCategory(KHE_WMATCH_NODE demand_node);
extern void KheWMatchDemandNodeEquivalent(KHE_WMATCH_NODE demand_node1,
  KHE_WMATCH_NODE demand_node2);
extern void KheWMatchDemandNodeDeleteEquivalent(KHE_WMATCH_NODE demand_node);

/* supply nodes */
extern KHE_WMATCH_NODE KheWMatchSupplyNodeMake(KHE_WMATCH m, void *back,
  KHE_WMATCH_CATEGORY category);
extern KHE_WMATCH KheWMatchSupplyNodeWMatch(KHE_WMATCH_NODE supply_node);
extern void *KheWMatchSupplyNodeBack(KHE_WMATCH_NODE supply_node);
extern KHE_WMATCH_CATEGORY KheWMatchSupplyNodeCategory(
  KHE_WMATCH_NODE supply_node);
extern void KheWMatchSupplyNodeNotifyDirty(KHE_WMATCH_NODE supply_node);
extern void KheWMatchSupplyNodeDelete(KHE_WMATCH_NODE supply_node);
extern void KheWMatchSupplyNodeAddToCategory(KHE_WMATCH_NODE supply_node,
  KHE_WMATCH_CATEGORY category);
extern void KheWMatchSupplyNodeDeleteFromCategory(KHE_WMATCH_NODE supply_node);

/* edge fixing */
extern void KheWMatchEdgeFix(KHE_WMATCH_NODE demand_node,
  KHE_WMATCH_NODE supply_node);
extern void KheWMatchEdgeUnFix(KHE_WMATCH_NODE demand_node);

/* edge freezing */
extern bool KheWMatchEdgeFreeze(KHE_WMATCH_NODE demand_node,
  KHE_WMATCH_NODE supply_node);
extern void KheWMatchEdgeUnFreeze(KHE_WMATCH_NODE demand_node);

/* solving and result reporting */
extern bool KheWMatchFullyMatchable(KHE_WMATCH m, int64_t *cost, int indent);
extern void KheWMatchEval(KHE_WMATCH m, int *infeasibility, int64_t *badness);
extern int64_t KheWMatchDemandNodeAverageEdgeCost(KHE_WMATCH_NODE demand_node);
extern void *KheWMatchDemandNodeAssignedTo(KHE_WMATCH_NODE demand_node,
  int64_t *cost);

/* special mode */
extern void KheWMatchSpecialModeBegin(KHE_WMATCH m);
extern void KheWMatchSpecialModeEnd(KHE_WMATCH m);

/* forced assignments */
extern void KheWMatchForcedAsstTestBegin(KHE_WMATCH m);
extern void KheWMatchForcedAsstTestAddNode(KHE_WMATCH m,
  KHE_WMATCH_NODE demand_node);
extern bool KheWMatchForcedAsstTestEnd(KHE_WMATCH m);

/* Hall sets */
extern int KheWMatchHallSetCount(KHE_WMATCH m);
extern int KheWMatchHallSetDemandNodeCount(KHE_WMATCH m, int i);
extern void *KheWMatchHallSetDemandNode(KHE_WMATCH m, int i, int j);
extern int KheWMatchHallSetSupplyNodeCount(KHE_WMATCH m, int i);
extern void *KheWMatchHallSetSupplyNode(KHE_WMATCH m, int i, int j);
extern void KheWMatchHallSetDebug(KHE_WMATCH m, int verbosity,
  int indent, FILE *fp);

/* matchings */
extern KHE_WMATCH KheWMatchMake(void *back, GENERIC_DEBUG_FN back_debug,
  GENERIC_DEBUG_FN demand_back_debug, GENERIC_DEBUG_FN supply_back_debug,
  KHE_WMATCH_EDGE_FN edge_fn, COST_DEBUG_FN cost_debug, int64_t tabu_cost,
  HA_ARENA a);
extern void KheWMatchClear(KHE_WMATCH m);
/* extern void KheWMatchDelete(KHE_WMATCH m); */
extern bool KheWMatchUseCategories(KHE_WMATCH m);

/* debugging and testing */
extern void KheWMatchDebug(KHE_WMATCH m, int verbosity, int indent, FILE *fp);
extern void KheWMatchTest(FILE *fp);

#endif

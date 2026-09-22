
/*****************************************************************************/
/*                                                                           */
/*  THE KTS TIMETABLING SYSTEM                                               */
/*  COPYRIGHT (C) 2004, 2010 Jeffrey H. Kingston                             */
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
/*  FILE:         khe_matching.h                                             */
/*  MODULE:       Unweighted bipartite matching                              */
/*                                                                           */
/*  This module implements unweighted bipartite matching of demand nodes     */
/*  to supply nodes.                                                         */
/*                                                                           */
/*  It has some special features useful for timetabling, notably a way to    */
/*  organize demand nodes and their domains to make it easy to do what is    */
/*  required when solution events are assigned, deassigned, split, and       */
/*  merged.  It also offers diagnosis of failure to match, in the form       */
/*  of Hall sets of unmatchable demand and supply nodes.                     */
/*                                                                           */
/*  Type KHE_MATCHING represents one bipartite graph, consisting of supply   */
/*  nodes, demand nodes, edges connecting them, and a maximum matching.      */
/*                                                                           */
/*  Type KHE_MATCHING_SUPPLY_CHUNK represents a set of supply nodes of a     */
/*  graph, in practice with contiguous index numbers.                        */
/*                                                                           */
/*  Type KHE_MATCHING_SUPPLY_NODE represents one supply node of a graph.     */
/*  Each supply node lies in exactly one supply chunk.                       */
/*                                                                           */
/*  Type KHE_MATCHING_DEMAND_CHUNK represents a set of demand nodes,         */
/*  with (base, offset, domain) attributes that help to determine the        */
/*  domains of their demand nodes, as described below.                       */
/*                                                                           */
/*  Type KHE_MATCHING_DEMAND_NODE represents one demand node of a graph.     */
/*  Each demand node lies in exactly one demand chunk.                       */
/*                                                                           */
/*  Type KHE_MATCHING_HALL_SET represents one Hall set of demand and         */
/*  supply nodes that failed to match.                                       */
/*                                                                           */
/*  How domains are calculated                                               */
/*  --------------------------                                               */
/*                                                                           */
/*  Given demand node dn with domain(dn), and its enclosing demand chunk     */
/*  dc with base(dc), increment(dc), and domain(dc), the domain of the       */
/*  demand node is the set of all numbers y calculated like this:            */
/*                                                                           */
/*     for each element a of domain(dc)                                      */
/*        x := base(dc) + increment(dc) * a;                                 */
/*        for each element b of domain(dn)                                   */
/*           y := x + b;                                                     */
/*                                                                           */
/*  This makes it easy to relocate entire domain chunks, by changing         */
/*  base(dc), and it allows for global tixel matching domains by setting     */
/*  increment(dc) to the number of resources in the instance.                */
/*                                                                           */
/*****************************************************************************/
#ifndef KHE_MATCHING_HEADER_FILE
#define KHE_MATCHING_HEADER_FILE
#include "howard_a.h"
#include <stdio.h>

#define NO_PREV_ASST SHRT_MAX


/*****************************************************************************/
/*                                                                           */
/*  Type declarations                                                        */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_matching_supply_chunk_rec *KHE_MATCHING_SUPPLY_CHUNK;
typedef HA_ARRAY(KHE_MATCHING_SUPPLY_CHUNK) ARRAY_KHE_MATCHING_SUPPLY_CHUNK;

typedef struct khe_matching_supply_node_rec *KHE_MATCHING_SUPPLY_NODE;
typedef HA_ARRAY(KHE_MATCHING_SUPPLY_NODE) ARRAY_KHE_MATCHING_SUPPLY_NODE;

typedef struct khe_matching_demand_chunk_rec *KHE_MATCHING_DEMAND_CHUNK;
typedef HA_ARRAY(KHE_MATCHING_DEMAND_CHUNK) ARRAY_KHE_MATCHING_DEMAND_CHUNK;

typedef struct khe_matching_demand_node_rec *KHE_MATCHING_DEMAND_NODE;
typedef HA_ARRAY(KHE_MATCHING_DEMAND_NODE) ARRAY_KHE_MATCHING_DEMAND_NODE;

typedef struct khe_matching_rec *KHE_MATCHING;
typedef HA_ARRAY(KHE_MATCHING) ARRAY_KHE_MATCHING;

typedef struct khe_matching_hall_set_rec *KHE_MATCHING_HALL_SET;
typedef HA_ARRAY(KHE_MATCHING_HALL_SET) ARRAY_KHE_MATCHING_HALL_SET;

typedef void (*KHE_MATCHING_SUPPLY_NODE_DEBUG_FN)
  (KHE_MATCHING_SUPPLY_NODE sn, int verbosity, int indent, FILE *fp);
typedef void (*KHE_MATCHING_DEMAND_NODE_DEBUG_FN)
  (KHE_MATCHING_DEMAND_NODE dn, int verbosity, int indent, FILE *fp);

typedef enum {
  KHE_MATCHING_DOMAIN_CHANGE_TO_SUBSET,
  KHE_MATCHING_DOMAIN_CHANGE_TO_SUPERSET,
  KHE_MATCHING_DOMAIN_CHANGE_TO_OTHER
} KHE_MATCHING_DOMAIN_CHANGE_TYPE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_SUPPLY_CHUNK                                                */
/*                                                                           */
/*****************************************************************************/

/* construction and query */
extern KHE_MATCHING_SUPPLY_CHUNK KheMatchingSupplyChunkMake(KHE_MATCHING m,
  void *impl);
extern void KheMatchingSupplyChunkDelete(KHE_MATCHING_SUPPLY_CHUNK sc);
extern KHE_MATCHING KheMatchingSupplyChunkMatching(
  KHE_MATCHING_SUPPLY_CHUNK sc);
extern void *KheMatchingSupplyChunkImpl(KHE_MATCHING_SUPPLY_CHUNK sc);
extern void KheMatchingSupplyChunkSetImpl(KHE_MATCHING_SUPPLY_CHUNK sc,
  void *impl);
extern int KheMatchingSupplyChunkBase(KHE_MATCHING_SUPPLY_CHUNK sc);
extern KHE_MATCHING_SUPPLY_CHUNK KheMatchingSupplyChunkCopyPhase1(
  KHE_MATCHING_SUPPLY_CHUNK sc, HA_ARENA a);
extern void KheMatchingSupplyChunkCopyPhase2(KHE_MATCHING_SUPPLY_CHUNK sc);

/* supply nodes */
extern int KheMatchingSupplyChunkSupplyNodeCount(KHE_MATCHING_SUPPLY_CHUNK sc);
extern KHE_MATCHING_SUPPLY_NODE KheMatchingSupplyChunkSupplyNode(
  KHE_MATCHING_SUPPLY_CHUNK sc, int i);


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_SUPPLY_NODE                                                 */
/*                                                                           */
/*****************************************************************************/

/* construction and query */
extern KHE_MATCHING_SUPPLY_NODE KheMatchingSupplyNodeMake(
  KHE_MATCHING_SUPPLY_CHUNK sc, void *impl);
extern KHE_MATCHING_SUPPLY_CHUNK KheMatchingSupplyNodeChunk(
  KHE_MATCHING_SUPPLY_NODE sn);
extern KHE_MATCHING KheMatchingSupplyNodeMatching(KHE_MATCHING_SUPPLY_NODE sn);
extern void *KheMatchingSupplyNodeImpl(KHE_MATCHING_SUPPLY_NODE sn);
extern void KheMatchingSupplyNodeSetImpl(KHE_MATCHING_SUPPLY_NODE sn,
  void *impl);
extern int KheMatchingSupplyNodeIndex(KHE_MATCHING_SUPPLY_NODE sn);
extern KHE_MATCHING_SUPPLY_NODE KheMatchingSupplyNodeCopyPhase1(
  KHE_MATCHING_SUPPLY_NODE sn, HA_ARENA a);
extern void KheMatchingSupplyNodeCopyPhase2(KHE_MATCHING_SUPPLY_NODE sn);


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_DEMAND_CHUNK                                                */
/*                                                                           */
/*****************************************************************************/

/* construction and query */
extern KHE_MATCHING_DEMAND_CHUNK KheMatchingDemandChunkMake(KHE_MATCHING m,
  void *impl, int base, int increment, SSET *domain);
extern KHE_MATCHING KheMatchingDemandChunkMatching(
  KHE_MATCHING_DEMAND_CHUNK dc);
extern void *KheMatchingDemandChunkImpl(KHE_MATCHING_DEMAND_CHUNK dc);
extern void KheMatchingDemandChunkSetImpl(KHE_MATCHING_DEMAND_CHUNK dc,
  void *impl);
extern void KheMatchingDemandChunkDelete(KHE_MATCHING_DEMAND_CHUNK dc);
extern KHE_MATCHING_DEMAND_CHUNK KheMatchingDemandChunkCopyPhase1(
  KHE_MATCHING_DEMAND_CHUNK dc, HA_ARENA a);
extern void KheMatchingDemandChunkCopyPhase2(KHE_MATCHING_DEMAND_CHUNK dc);

/* base, increment, and domain */
extern int KheMatchingDemandChunkBase(KHE_MATCHING_DEMAND_CHUNK dc);
extern int KheMatchingDemandChunkIncrement(KHE_MATCHING_DEMAND_CHUNK dc);
extern SSET *KheMatchingDemandChunkDomain(KHE_MATCHING_DEMAND_CHUNK dc);
extern void KheMatchingDemandChunkSetBase(KHE_MATCHING_DEMAND_CHUNK dc,
  int base);
extern void KheMatchingDemandChunkSetIncrement(KHE_MATCHING_DEMAND_CHUNK dc,
  int increment);
extern void KheMatchingDemandChunkSetDomain(KHE_MATCHING_DEMAND_CHUNK dc,
  SSET *domain, KHE_MATCHING_DOMAIN_CHANGE_TYPE domain_change_type);

/* demand nodes */
extern int KheMatchingDemandChunkNodeCount(KHE_MATCHING_DEMAND_CHUNK dc);
extern KHE_MATCHING_DEMAND_NODE KheMatchingDemandChunkNode(
  KHE_MATCHING_DEMAND_CHUNK dc, int i);


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_DEMAND_NODE                                                 */
/*                                                                           */
/*****************************************************************************/

/* construction and query */
extern void KheMatchingDemandNodeAdd(KHE_MATCHING_DEMAND_NODE dn);
extern void KheMatchingDemandNodeDelete(KHE_MATCHING_DEMAND_NODE dn);
extern KHE_MATCHING_DEMAND_CHUNK KheMatchingDemandNodeChunk(
  KHE_MATCHING_DEMAND_NODE dn);
extern KHE_MATCHING_DEMAND_NODE KheMatchingDemandNodeCopyPhase1(
  KHE_MATCHING_DEMAND_NODE dn, HA_ARENA a);
extern void KheMatchingDemandNodeCopyPhase2(KHE_MATCHING_DEMAND_NODE dn);

/* domains */
extern KHE_SET *KheMatchingDemandNodeDomain(KHE_MATCHING_DEMAND_NODE dn);
extern void KheMatchingDemandNodeSetDomain(KHE_MATCHING_DEMAND_NODE dn,
  KHE_SET *domain, KHE_MATCHING_DOMAIN_CHANGE_TYPE domain_change_type);


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING                                                             */
/*                                                                           */
/*****************************************************************************/

/* construction and query */
extern KHE_MATCHING KheMatchingMake(KHE_SOLN soln);
/* extern void *KheMatchingImpl(KHE_MATCHING m); */
/* extern void KheMatchingSetImpl(KHE_MATCHING m, void *impl); */
extern void KheMatchingDelete(KHE_MATCHING m);
extern KHE_MATCHING KheMatchingCopyPhase1(KHE_MATCHING m, HA_ARENA a);
extern void KheMatchingCopyPhase2(KHE_MATCHING m);
extern bool KheMatchingIsActive(KHE_MATCHING m);

/* node counts */
extern int KheMatchingSupplyNodeCount(KHE_MATCHING m);
extern int KheMatchingDemandNodeCount(KHE_MATCHING m);

/* supply chunks */
extern int KheMatchingSupplyChunkCount(KHE_MATCHING m);
extern KHE_MATCHING_SUPPLY_CHUNK KheMatchingSupplyChunk(KHE_MATCHING m, int i);

/* demand chunks */
extern int KheMatchingDemandChunkCount(KHE_MATCHING m);
extern KHE_MATCHING_DEMAND_CHUNK KheMatchingDemandChunk(KHE_MATCHING m, int i);

/* solving and cost */
extern int KheMatchingUnmatchedDemandNodeCount(KHE_MATCHING m);
extern KHE_MATCHING_DEMAND_NODE KheMatchingUnmatchedDemandNode(KHE_MATCHING m,
  int i);
extern void KheMatchingMarkBegin(KHE_MATCHING m);
extern void KheMatchingMarkCheck(KHE_MATCHING m);
extern void KheMatchingMarkEnd(KHE_MATCHING m, bool undo);

/* Hall sets */
extern int KheMatchingHallSetCount(KHE_MATCHING m);
extern KHE_MATCHING_HALL_SET KheMatchingHallSet(KHE_MATCHING m, int i);

/* competitors */
extern void KheMatchingSetCompetitors(KHE_MATCHING m,
  KHE_MATCHING_DEMAND_NODE dn);
extern int KheMatchingCompetitorCount(KHE_MATCHING m);
extern KHE_MATCHING_DEMAND_NODE KheMatchingCompetitor(KHE_MATCHING m, int i);

/* debug (arbitrary subset of all demand chunks) */
extern void KheMatchingDebugBegin(KHE_MATCHING m,
  KHE_MATCHING_SUPPLY_NODE_DEBUG_FN supply_node_debug_fn,
  KHE_MATCHING_DEMAND_NODE_DEBUG_FN demand_node_debug_fn,
  int verbosity, int indent, FILE *fp);
extern void KheMatchingDebugDemandChunk(KHE_MATCHING_DEMAND_CHUNK dc,
  KHE_MATCHING_SUPPLY_NODE_DEBUG_FN supply_node_debug_fn,
  KHE_MATCHING_DEMAND_NODE_DEBUG_FN demand_node_debug_fn,
  int verbosity, int indent, FILE *fp);
extern void KheMatchingDebugEnd(KHE_MATCHING m,
  KHE_MATCHING_SUPPLY_NODE_DEBUG_FN supply_node_debug_fn,
  KHE_MATCHING_DEMAND_NODE_DEBUG_FN demand_node_debug_fn,
  int verbosity, int indent, FILE *fp);

/* debug (whole matching) */
extern void KheMatchingDebug(KHE_MATCHING m,
  KHE_MATCHING_SUPPLY_NODE_DEBUG_FN supply_node_debug_fn,
  KHE_MATCHING_DEMAND_NODE_DEBUG_FN demand_node_debug_fn,
  int verbosity, int indent, FILE *fp);


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_HALL_SET                                                    */
/*                                                                           */
/*****************************************************************************/

/* construction and query */
extern KHE_MATCHING_HALL_SET KheMatchingHallSetCopyPhase1(
  KHE_MATCHING_HALL_SET hs, HA_ARENA a);
extern void KheMatchingHallSetCopyPhase2(KHE_MATCHING_HALL_SET hs);

/* supply nodes */
extern int KheMatchingHallSetSupplyNodeCount(KHE_MATCHING_HALL_SET hs);
extern KHE_MATCHING_SUPPLY_NODE KheMatchingHallSetSupplyNode(
  KHE_MATCHING_HALL_SET hs, int i);

/* demand nodes */
extern int KheMatchingHallSetDemandNodeCount(KHE_MATCHING_HALL_SET hs);
extern KHE_MATCHING_DEMAND_NODE KheMatchingHallSetDemandNode(
  KHE_MATCHING_HALL_SET hs, int i);

/* debug */
extern void KheMatchingHallSetDebug(KHE_MATCHING_HALL_SET hs,
  KHE_MATCHING_SUPPLY_NODE_DEBUG_FN supply_node_debug_fn,
  KHE_MATCHING_DEMAND_NODE_DEBUG_FN demand_node_debug_fn,
  int verbosity, int indent, FILE *fp);

#endif

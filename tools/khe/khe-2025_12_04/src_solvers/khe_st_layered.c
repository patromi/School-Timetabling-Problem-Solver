
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
/*  FILE:         khe_st_layered.c                                           */
/*  DESCRIPTION:  Layer time assignment                                      */
/*                                                                           */
/*****************************************************************************/
#include <limits.h>
#include "khe_solvers.h"
#include "khe_wmatch.h"

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

typedef HA_ARRAY(KHE_MEET) ARRAY_KHE_MEET;
typedef HA_ARRAY(KHE_NODE) ARRAY_KHE_NODE;

typedef struct khe_layer_info_rec *KHE_LAYER_INFO;
typedef HA_ARRAY(KHE_LAYER_INFO) ARRAY_KHE_LAYER_INFO;


/*****************************************************************************/
/*                                                                           */
/*  KHE_LAYER_REPAIR_TYPE                                                    */
/*                                                                           */
/*  The kind of layer repair to try, from the "ts_layer_repair" option:      */
/*                                                                           */
/*     layer_repair      KHE_LAYER_REPAIR_TYPE                               */
/*     ---------------------------------------                               */
/*     none              KHE_LAYER_REPAIR_NONE                               */
/*     exp               KHE_LAYER_REPAIR_EXP                                */
/*     all               KHE_LAYER_REPAIR_ALL                                */
/*     ---------------------------------------                               */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_LAYER_REPAIR_NONE,
  KHE_LAYER_REPAIR_EXP,
  KHE_LAYER_REPAIR_ALL
} KHE_LAYER_REPAIR_TYPE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_LAYER_INFO - information about one soln layer.                       */
/*                                                                           */
/*****************************************************************************/

struct khe_layer_info_rec {
  KHE_LAYER		layer;			/* the soln layer            */
  int			assigned_count;		/* no of assigned meets      */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "paralleling preassignments" (private)                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheNodeAssignedMeetCount(KHE_NODE node)                              */
/*                                                                           */
/*  Return the number of assigned meets in node.                             */
/*                                                                           */
/*****************************************************************************/

static int KheNodeAssignedMeetCount(KHE_NODE node)
{
  int i, res;  KHE_MEET meet;
  res = 0;
  for( i = 0;  i < KheNodeMeetCount(node);  i++ )
  {
    meet = KheNodeMeet(node, i);
    if( KheMeetAsst(meet) != NULL )
      res++;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLayerAssignedMeetCount(KHE_LAYER layer)                           */
/*                                                                           */
/*  Return the no. of assigned meets in layer.                               */
/*                                                                           */
/*****************************************************************************/

static int KheLayerAssignedMeetCount(KHE_LAYER layer)
{
  int i, res;  KHE_NODE child_node;
  res = 0;
  for( i = 0;  i < KheLayerChildNodeCount(layer);  i++ )
  {
    child_node = KheLayerChildNode(layer, i);
    res += KheNodeAssignedMeetCount(child_node);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LAYER_INFO KheLayerInfoMake(KHE_LAYER layer)                         */
/*                                                                           */
/*  Make a new soln layer info object for layer.                             */
/*                                                                           */
/*****************************************************************************/

static KHE_LAYER_INFO KheLayerInfoMake(KHE_LAYER layer, HA_ARENA a)
{
  KHE_LAYER_INFO res;
  HaMake(res, a);
  res->layer = layer;
  res->assigned_count = KheLayerAssignedMeetCount(layer);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLayerInfoCmp(const void *t1, const void *t2)                      */
/*                                                                           */
/*  Comparison function for sorting soln layer info records into order of    */
/*  decreasing number of assigned meets.                                     */
/*                                                                           */
/*****************************************************************************/

static int KheLayerInfoCmp(const void *t1, const void *t2)
{
  KHE_LAYER_INFO si1 = * (KHE_LAYER_INFO *) t1;
  KHE_LAYER_INFO si2 = * (KHE_LAYER_INFO *) t2;
  if( si2->assigned_count != si1->assigned_count )
    return si2->assigned_count - si1->assigned_count;
  else
    return KheLayerParentNodeIndex(si1->layer) -
      KheLayerParentNodeIndex(si2->layer);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheOverlaps(int offset1, int duration1, int offset2, int duration2) */
/*                                                                           */
/*  Return true if two meets with these offsets and durations, assigned to   */
/*  the same meet, would overlap in time.                                    */
/*                                                                           */
/*****************************************************************************/

static bool KheOverlaps(int offset1, int duration1, int offset2, int duration2)
{
  if( offset1 + duration1 <= offset2 )
    return false;
  if( offset2 + duration2 <= offset1 )
    return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetsMatch(KHE_MEET meet1, KHE_MEET meet2)                       */
/*                                                                           */
/*  Assuming that meet1 and meet2 are both assigned, return true if they     */
/*  have the same duration and are assigned to the same meet at the same     */
/*  offset.                                                                  */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetsMatch(KHE_MEET meet1, KHE_MEET meet2)
{
  return KheMeetAsst(meet1) == KheMeetAsst(meet2) &&
    KheMeetAsstOffset(meet1) == KheMeetAsstOffset(meet2) &&
    KheMeetDuration(meet1) == KheMeetDuration(meet2);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeMatches(KHE_NODE layer_node, KHE_NODE other_node,            */
/*    ARRAY_KHE_MEET *match)                                                 */
/*                                                                           */
/*  Return true if layer_node matches other_node, that is, if the two        */
/*  nodes are distinct, have the same duration, and the assigned meets of    */
/*  other_node can be matched up with meets of layer_node, preferably with   */
/*  meets with the same assignment, but otherwise with unassigned meets.     */
/*  If successful, set *match to the match.                                  */
/*                                                                           */
/*****************************************************************************/

static bool KheNodeMatches(KHE_NODE layer_node, KHE_NODE other_node,
  ARRAY_KHE_MEET *match)
{
  int i, j, pos;  KHE_MEET other_meet, layer_meet;  bool done;
  if( DEBUG5 )
    fprintf(stderr, "  [ KheNodeMatches(Node %d, Node %d)\n",
      KheNodeSolnIndex(layer_node), KheNodeSolnIndex(other_node));

  /* make sure the two nodes are distinct with equal durations */
  if( other_node == layer_node )
  {
    if( DEBUG5 )
      fprintf(stderr, "  ] KheNodeMatches returning false (identical nodes)\n");
    return false;
  }
  if( KheNodeDuration(other_node) != KheNodeDuration(layer_node) )
  {
    if( DEBUG5 )
      fprintf(stderr, "  ] KheNodeMatches returning false (node durations)\n");
    return false;
  }

  /* match each assigned meet of other_node with a meet of layer_node */
  HaArrayClear(*match);
  for( i = 0;  i < KheNodeMeetCount(other_node);  i++ )
  {
    other_meet = KheNodeMeet(other_node, i);
    if( KheMeetAsst(other_meet) != NULL )
    {
      /* search for an unused meet of layer_node assigned like other_meet */
      done = false;
      for( j = 0;  !done && j < KheNodeMeetCount(layer_node);  j++ )
      {
	layer_meet = KheNodeMeet(layer_node, j);
	if( KheMeetAsst(layer_meet) != NULL &&
	    KheMeetsMatch(layer_meet, other_meet) &&
	    !HaArrayContains(*match, layer_meet, &pos) )
	{
	  HaArrayAddLast(*match, layer_meet);
	  done = true;
	}
      }

      /* else, search for an unused assignable meet of layer_node */
      for( j = 0;  !done && j < KheNodeMeetCount(layer_node);  j++ )
      {
	layer_meet = KheNodeMeet(layer_node, j);
	if( KheMeetAsst(layer_meet) == NULL &&
	    KheMeetDuration(layer_meet) == KheMeetDuration(other_meet) &&
	    KheMeetAssignCheck(layer_meet, KheMeetAsst(other_meet),
	      KheMeetAsstOffset(other_meet)) &&
	    !HaArrayContains(*match, layer_meet, &pos) )
	{
	  HaArrayAddLast(*match, layer_meet);
	  done = true;
	}
      }

      /* else fail now */
      if( !done )
      {
	if( DEBUG5 )
	{
	  fprintf(stderr, "  ] KheNodeMatches returning false at ");
	  KheMeetDebug(other_meet, 1, -1, stderr);
	  fprintf(stderr, "\n");
	}
	return false;
      }
    }
  }
  if( DEBUG5 )
  {
    HaArrayForEach(*match, layer_meet, i)
      KheMeetDebug(layer_meet, 2, 4, stderr);
    fprintf(stderr, "  ] KheNodeMatches returning true\n");
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeMatch(KHE_NODE layer_node, KHE_NODE other_node,              */
/*    ARRAY_KHE_MEET *match)                                                 */
/*                                                                           */
/*  Assuming KheNodeMatches above returned true for these parameters,        */
/*  carry out the assignments that give layer_node the assignments           */
/*  of other_node, using *match as a guide, and return true.                 */
/*                                                                           */
/*****************************************************************************/

static bool KheNodeMatch(KHE_NODE layer_node, KHE_NODE other_node,
  ARRAY_KHE_MEET *match)
{
  int i, j;  KHE_MEET layer_meet, other_meet;
  j = 0;
  for( i = 0;  i < KheNodeMeetCount(other_node);  i++ )
  {
    other_meet = KheNodeMeet(other_node, i);
    if( KheMeetAsst(other_meet) != NULL )
    {
      layer_meet = HaArray(*match, j);
      if( KheMeetAsst(layer_meet) == NULL )
      {
	if( DEBUG2 )
	{
	  fprintf(stderr, "  calling KheMeetAssign(");
	  KheMeetDebug(layer_meet, 1, -1, stderr);
	  fprintf(stderr, ", ");
	  KheMeetDebug(KheMeetAsst(other_meet), 1, -1, stderr);
	  fprintf(stderr, ", %d)\n", KheMeetAsstOffset(other_meet));
	}
	if( !KheMeetAssign(layer_meet, KheMeetAsst(other_meet),
	      KheMeetAsstOffset(other_meet)) )
	  HnAbort("KheNodeMatch internal error");
      }
      j++;
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetsOverlap(KHE_MEET meet1, KHE_MEET meet2)                     */
/*                                                                           */
/*  Assuming that meet1 and meet2 are both assigned, return true if they     */
/*  are assigned to the same meet in such a way as to overlap in time.       */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetsOverlap(KHE_MEET meet1, KHE_MEET meet2)
{
  return KheMeetAsst(meet1) == KheMeetAsst(meet2) &&
    KheOverlaps(KheMeetAsstOffset(meet1), KheMeetDuration(meet1),
      KheMeetAsstOffset(meet2), KheMeetDuration(meet2));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodesOverlap(KHE_NODE node1, KHE_NODE node2)                     */
/*                                                                           */
/*  Return true if node1 and node2 contain meets that overlap in time.       */
/*                                                                           */
/*****************************************************************************/

static bool KheNodesOverlap(KHE_NODE node1, KHE_NODE node2)
{
  KHE_MEET meet1, meet2;  int i, j;
  for( i = 0;  i < KheNodeMeetCount(node1);  i++ )
  {
    meet1 = KheNodeMeet(node1, i);
    if( KheMeetAsst(meet1) != NULL )
      for( j = 0;  j < KheNodeMeetCount(node2);  j++ )
      {
	meet2 = KheNodeMeet(node2, j);
	if( KheMeetAsst(meet2) != NULL && KheMeetsOverlap(meet1, meet2) )
	  return true;
      }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeParallelAssignTimes(KHE_NODE child_node, KHE_LAYER layer)    */
/*                                                                           */
/*  Here child_node is known to have at least one preassigned meet.  Try     */
/*  to match up the node with a node of layer, assigning some meets of the   */
/*  node of layer the preassigned times.                                     */
/*                                                                           */
/*****************************************************************************/

static bool KheNodeParallelAssignTimes(KHE_NODE child_node, KHE_LAYER layer)
{
  int i, j;  bool res;  ARRAY_KHE_MEET match;  HA_ARENA a;
  ARRAY_KHE_NODE nodes;  KHE_NODE best_node, node;  KHE_SOLN soln;
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheNodeParallelAssignTimes(Node %d)\n",
      KheNodeSolnIndex(child_node));
    KheNodeDebug(child_node, 2, 2, stderr);
  }

  /* find the nodes of layer that overlap child_node */
  soln = KheNodeSoln(child_node);
  a = KheSolnArenaBegin(soln);
  /* a = HaArena Make(); */
  HaArrayInit(nodes, a);
  for( i = 0;  i < KheLayerChildNodeCount(layer);  i++ )
  {
    node = KheLayerChildNode(layer, i);
    if( !HaArrayContains(nodes, node, &j) && KheNodesOverlap(node, child_node) )
      HaArrayAddLast(nodes, node);
  }
  if( DEBUG2 )
  {
    fprintf(stderr, "  overlaps %d nodes", HaArrayCount(nodes));
    HaArrayForEach(nodes, node, i)
      fprintf(stderr, "%s Node %d", i == 0 ? ":" : ",", KheNodeSolnIndex(node));
    fprintf(stderr, "\n");
  }

  res = false;
  if( HaArrayCount(nodes) == 0 )
  {
    /* no node overlaps at present, so any matching node will do */
    best_node = NULL;
    HaArrayInit(match, a);
    for( i = 0;  i < KheLayerChildNodeCount(layer);  i++ )
    {
      node = KheLayerChildNode(layer, j);
      if( KheNodeMatches(node, child_node, &match) )
      {
	if( best_node == NULL ||
	    KheNodeDuration(node) < KheNodeDuration(best_node) ||
	    (KheNodeDuration(node) == KheNodeDuration(best_node) &&
	     KheNodeAssignedMeetCount(node) <
	     KheNodeAssignedMeetCount(best_node)) )
	  best_node = node;
      }
    }

    /* match the best node if there is one */
    if( best_node != NULL )
    {
      if( !KheNodeMatches(best_node, child_node, &match) )
	HnAbort("KheNodeParallelAssignTimes internal error");
      res = KheNodeMatch(best_node, child_node, &match);
    }
    HaArrayFree(match);
  }
  else if( HaArrayCount(nodes) == 1 )
  {
    /* try to match child_node with this one node */
    HaArrayInit(match, a);
    best_node = HaArrayFirst(nodes);
    if( KheNodeMatches(best_node, child_node, &match) )
      res = KheNodeMatch(best_node, child_node, &match);
    HaArrayFree(match);
  }
  else
  {
    /* preassignments already span two nodes, so abandon paralleling here */
  }

  KheSolnArenaEnd(soln, a);
  /* HaAre naDelete(a); */
  /* MArrayFree(nodes); */
  if( DEBUG2 )
    fprintf(stderr, "] KheNodeParallelAssignTimes returning %s\n",
      res ? "true" : "false");
  return res;
}



/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerParallelAssignTimes(KHE_LAYER layer, KHE_OPTIONS options)   */
/*                                                                           */
/*  Make assignments to the meets of layer which parallel preassignments     */
/*  in layer's sibling layers, as far as possible.  Return true if every     */
/*  assigned meet in every sibling layer has a parallel meet in layer.       */
/*                                                                           */
/*****************************************************************************/

bool KheLayerParallelAssignTimes(KHE_LAYER layer, KHE_OPTIONS options)
{
  ARRAY_KHE_LAYER_INFO layer_infos;  KHE_LAYER_INFO layer_info;
  KHE_LAYER layer2;  KHE_NODE parent_node, child_node;  int i, j;
  bool res;  HA_ARENA a;  KHE_SOLN soln;
  if( DEBUG2 )
    fprintf(stderr, "[ KheLayerParallelAssignTimes(layer)\n");

  /* get the layer_infos and sort them by decreasing no of preassigned meets */
  soln = KheLayerSoln(layer);
  a = KheSolnArenaBegin(soln);
  /* a = HaAr enaMake(); */
  HaArrayInit(layer_infos, a);
  parent_node = KheLayerParentNode(layer);
  for( i = 0;  i < KheNodeChildLayerCount(parent_node);  i++ )
  {
    layer2 = KheNodeChildLayer(parent_node, i);
    if( layer2 != layer )
      HaArrayAddLast(layer_infos, KheLayerInfoMake(layer2, a));
  }
  HaArraySort(layer_infos, &KheLayerInfoCmp);

  /* make parallel assignments for each layer_info in turn */
  res = true;
  HaArrayForEach(layer_infos, layer_info, i)
    for( j = 0;  j < KheLayerChildNodeCount(layer_info->layer);  j++ )
    {
      child_node = KheLayerChildNode(layer_info->layer, j);
      if( KheNodeAssignedMeetCount(child_node) > 0 )
	res &= KheNodeParallelAssignTimes(child_node, layer);
    }

  /* free the array of soln layer info objects and the objects themselves */
  KheSolnArenaEnd(soln, a);
  /* HaArena Delete(a); */
  /* ***
  HaArrayForEach(layer_infos, layer_info, j)
    MFree(layer_info);
  MArrayFree(layer_infos);
  *** */

  if( DEBUG2 )
    fprintf(stderr, "] KheLayerParallelAssignTimes returning %s\n",
      res ? "true" : "false");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "layered time assignment"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheNodeLayeredLayerCmp(const void *t1, const void *t2)               */
/*                                                                           */
/*  Sort soln layers so that visited ones come first, and the rest are       */
/*  sorted according to the saturation degree heuristic, roughly.            */
/*                                                                           */
/*****************************************************************************/

int KheNodeLayeredLayerCmp(const void *t1, const void *t2)
{
  KHE_LAYER layer1 = * (KHE_LAYER *) t1;
  KHE_LAYER layer2 = * (KHE_LAYER *) t2;
  int value1, value2, demand1, demand2;
  if( KheLayerVisitNum(layer1) != KheLayerVisitNum(layer2) )
    return KheLayerVisitNum(layer2) - KheLayerVisitNum(layer1);
  value1 = 3 * KheLayerDuration(layer1) - KheLayerMeetCount(layer1) +
    KheLayerAssignedDuration(layer1);
  value2 = 3 * KheLayerDuration(layer2) - KheLayerMeetCount(layer2) +
    KheLayerAssignedDuration(layer2);
  if( value1 != value2 )
    return value2 - value1;
  demand1 = KheLayerDemand(layer1);
  demand2 = KheLayerDemand(layer2);
  if( DEBUG7 )
    fprintf(stderr, "  KheNodeLayeredLayerCmp(): demand1 %d, demand2 %d\n",
      demand1, demand2);
  if( demand1 != demand2 )
    return demand2 - demand1;
  return KheLayerParentNodeIndex(layer1) - KheLayerParentNodeIndex(layer2);
}

/*****************************************************************************/
/*                                                                           */
/*  int KheNodeLayeredLayerDiversifyCmp(const void *t1, const void *t2)      */
/*                                                                           */
/*  Sort soln layers so that visited ones come first, and the rest are       */
/*  sorted according to the saturation degree heuristic, roughly, with       */
/*  diversification.                                                         */
/*                                                                           */
/*****************************************************************************/

static bool DemandDiffersByAtLeast(int demand1, int demand2, int diff)
{
  if( demand1 < demand2 )
    return demand2 - demand1 >= diff;
  else
    return demand1 - demand2 >= diff;
}

int KheNodeLayeredLayerDiversifyCmp(const void *t1, const void *t2)
{
  KHE_LAYER layer1 = * (KHE_LAYER *) t1;
  KHE_LAYER layer2 = * (KHE_LAYER *) t2;
  int value1, value2, demand1, demand2;  KHE_SOLN soln;
  if( KheLayerVisitNum(layer1) != KheLayerVisitNum(layer2) )
    return KheLayerVisitNum(layer2) - KheLayerVisitNum(layer1);
  value1 = 3 * KheLayerDuration(layer1) - KheLayerMeetCount(layer1) +
    KheLayerAssignedDuration(layer1);
  value2 = 3 * KheLayerDuration(layer2) - KheLayerMeetCount(layer2) +
    KheLayerAssignedDuration(layer2);
  if( value1 != value2 )
    return value2 - value1;
  demand1 = KheLayerDemand(layer1);
  demand2 = KheLayerDemand(layer2);
  /* if( demand1 != demand2 ) */
  if( DemandDiffersByAtLeast(demand1, demand2, 6) )
    return demand2 - demand1;
  if( DEBUG7 )
    fprintf(stderr, "  KheNodeLayeredLayerCmp() diversifying\n");
  soln = KheLayerSoln(layer1);
  if( (KheLayerParentNodeIndex(layer1) + KheLayerParentNodeIndex(layer2) +
      KheSolnDiversifier(soln)) % 2 == 0 )
    return KheLayerParentNodeIndex(layer1) - KheLayerParentNodeIndex(layer2);
  else
    return KheLayerParentNodeIndex(layer2) - KheLayerParentNodeIndex(layer1);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLayerSaturationDegreeCmp(const void *t1, const void *t2)          */
/*                                                                           */
/*  Sort soln layers so that visited ones come first, and the rest are       */
/*  sorted according to the saturation degree heuristic.                     */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static int KheLayerSaturationDegreeCmp(const void *t1, const void *t2)
{
  KHE_LAYER layer1 = * (KHE_LAYER *) t1;
  KHE_LAYER layer2 = * (KHE_LAYER *) t2;
  int demand1, demand2;
  int assigned_duration1, assigned_duration2;
  if( KheLayerVisitNum(layer1) != KheLayerVisitNum(layer2) )
    return KheLayerVisitNum(layer2) - KheLayerVisitNum(layer1);
  if( KheLayerDuration(layer1) != KheLayerDuration(layer2) )
    return KheLayerDuration(layer2) - KheLayerDuration(layer1);
  assigned_duration1 = KheLayerAssignedDuration(layer1);
  assigned_duration2 = KheLayerAssignedDuration(layer2);
  if( assigned_duration1 != assigned_duration2 )
    return assigned_duration2 - assigned_duration1;
  demand1 = KheLayerDemand(layer1);
  demand2 = KheLayerDemand(layer2);
  if( demand1 != demand2 )
    return demand2 - demand1;
  else
    return KheLayerParentNodeIndex(layer1) - KheLayerParentNodeIndex(layer2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_SPREAD_EVENTS_CONSTRAINT BestSpreadEventsConstraint(KHE_INSTANCE ins)*/
/*                                                                           */
/*  Return the best spread events constraint of ins, or NULL if none.        */
/*                                                                           */
/*****************************************************************************/

static KHE_SPREAD_EVENTS_CONSTRAINT BestSpreadEventsConstraint(KHE_INSTANCE ins)
{
  KHE_CONSTRAINT c, best_c;  int i;
  best_c = NULL;
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintTag(c) == KHE_SPREAD_EVENTS_CONSTRAINT_TAG &&
	(best_c == NULL ||
	 KheConstraintAppliesToCount(c) > KheConstraintAppliesToCount(best_c)) )
      best_c = c;
  }
  return (KHE_SPREAD_EVENTS_CONSTRAINT) best_c;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnDebugDefectsWithTag(KHE_SOLN soln, KHE_MONITOR_TAG tag)      */
/*                                                                           */
/*  Helper function for printing defects with the given tag.                 */
/*                                                                           */
/*****************************************************************************/

/* *** used below but usage is currently commented out
static void KheSolnDebugDefectsWithTag(KHE_SOLN soln, KHE_MONITOR_TAG tag)
{
  KHE_MONITOR m;  int i;
  for( i = 0;  i < KheSolnDefectCount(soln);  i++ )
  {
    m = KheSolnDefect(soln, i);
    if( KheMonitorTag(m) == tag )
      KheMonitorDebug(m, 2, 4, stderr);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerDebugUnassigned(KHE_LAYER layer, int verbosity,             */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of the unassigned children of layer.                         */
/*                                                                           */
/*****************************************************************************/

static void KheLayerDebugUnassigned(KHE_LAYER layer, int verbosity,
  int indent, FILE *fp)
{
  int i, j;  KHE_NODE child_node;  KHE_MEET meet;
  fprintf(stderr, "%*s[ unassigned meets of layer:\n", indent, "");
  for( i = 0;  i < KheLayerChildNodeCount(layer);  i++ )
  {
    child_node = KheLayerChildNode(layer, i);
    for( j = 0;  j < KheNodeMeetCount(child_node);  j++ )
    {
      meet = KheNodeMeet(child_node, j);
      if( KheMeetAsst(meet) == NULL )
	KheMeetDebug(meet, verbosity, indent + 2, fp);
    }
  }
  fprintf(stderr, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerRepair(KHE_LAYER layer, KHE_LAYER_REPAIR_TYPE repair_type,  */
/*    KHE_OPTIONS options, KHE_BACKOFF bk)                                   */
/*                                                                           */
/*  Carry out the repair of layer requested by repair_type.                  */
/*                                                                           */
/*****************************************************************************/

static void KheLayerRepair(KHE_LAYER layer, KHE_LAYER_REPAIR_TYPE repair_type,
  KHE_OPTIONS options, KHE_BACKOFF bk)
{
  if( DEBUG10 )
  {
    fprintf(stderr, "[ KheLayerRepair(layer, ...), layer:\n");
    KheLayerDebugUnassigned(layer, 2, 2, stderr);
  }
  switch( repair_type )
  {
    case KHE_LAYER_REPAIR_NONE:

      /* do nothing */
      break;

    case KHE_LAYER_REPAIR_EXP:

      /* repair the layer with exponential backoff */
      if( KheBackoffAcceptOpportunity(bk) )
	KheBackoffResult(bk,
	  KheEjectionChainLayerRepairTimes(layer, options, NULL));
      break;

    case KHE_LAYER_REPAIR_ALL:

      /* repair the layer without exponential backoff */
      KheEjectionChainLayerRepairTimes(layer, options, NULL);
      break;

    default:

      HnAbort("KheLayerRepair: invalid repair_type (%d)", repair_type);
  }
  if( DEBUG10 )
  {
    fprintf(stderr, "  at end of KheLayerRepair:\n");
    KheLayerDebugUnassigned(layer, 2, 2, stderr);
    fprintf(stderr, "] KheLayerRepair returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerRepair(KHE_LAYER layer, KHE_OPTIONS options, KHE_BACKOFF bk)*/
/*                                                                           */
/*  Carry out the repair of layer requested by options.                      */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete
static void KheLayerRepair(KHE_LAYER layer, KHE_OPTIONS options, KHE_BACKOFF bk)
{
  if( !KheOptionsTimeLayerRepair(options) )
  {
    ** do nothing **
  }
  else if( KheOptionsTimeLayerRepairBackoff(options) )
  {
    ** repair the layer with exponential backoff **
    if( KheBackoffAcceptOpportunity(bk) )
      KheBackoffResult(bk, KheEjectionChainLayerRepairTimes(layer, options));
  }
  else
  {
    ** repair the layer without exponential backoff **
    KheEjectionChainLayerRepairTimes(layer, options);
  }
}
*** */


/* ***
static void KheLayerRepair(KHE_LAYER layer, KHE_OPTIONS options, KHE_BACKOFF bk)
{
  switch( KheOptionsTimeLayerRepair(options) )
  {
    case KHE_OPTIONS_TIME_LAYER_REPAIR_NONE:

      ** do nothing **
      break;

    case KHE_OPTIONS_TIME_LAYER_REPAIR_LAYER:

      ** repair just the layer **
      KheEjectionChainLaye rRepairTimes(layer, options);
      break;

    case KHE_OPTIONS_TIME_LAYER_REPAIR_NODE:

      ** repair the entire parent node **
      KheEjectionChain NodeRepairTimes(KheLayerParentNode(layer), options);
      break;

    case KHE_OPTIONS_TIME_LAYER_REPAIR_LAYER_BACKOFF:

      ** repair the layer with exponential backoff **
      if( KheBackoffAcceptOpportunity(bk) )
	KheBackoffResult(bk, KheEjectionChain LayerRepairTimes(layer, options));
      break;

    case KHE_OPTIONS_TIME_LAYER_REPAIR_NODE_BACKOFF:

      ** repair the entire parent node with exponential backoff **
      if( KheBackoffAcceptOpportunity(bk) )
	KheBackoffResult(bk,
	  KheEjection ChainNodeRepairTimes(KheLayerParentNode(layer), options));
      break;

    default:

      HnAbort("KheLayerRepair: illegal time_layer_repair option");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheOneLayerAsstDebug(KHE_NODE parent_node, int i,                   */
/*    bool reversed, FILE *fp)                                               */
/*                                                                           */
/*  Debug print of layer i of parent_node onto fp after assignment.          */
/*                                                                           */
/*****************************************************************************/

static void KheOneLayerAsstDebug(KHE_NODE parent_node, int i,
  bool reversed, FILE *fp)
{
  KHE_LAYER layer;  int j, k;  KHE_NODE child_node;  KHE_MEET meet;
  KHE_SOLN soln;  bool all_assigned;
  soln = KheNodeSoln(parent_node);
  layer = KheNodeChildLayer(parent_node, i);
  all_assigned = (KheLayerAssignedDuration(layer) >= KheLayerDuration(layer));
  fprintf(stderr, "  after assigning %slayer %d of %d (%d assigned of %d%s):\n",
    reversed ? "reversed " : "", i, KheNodeChildLayerCount(parent_node),
    KheLayerAssignedDuration(layer), KheLayerDuration(layer),
    all_assigned ? "" : " - not all assigned");
  KheLayerDebug(layer, all_assigned ? 1 : 3, 4, stderr);
  for( j = 0;  j < KheLayerChildNodeCount(layer);  j++ )
  {
    child_node = KheLayerChildNode(layer, j);
    for( k = 0;  k < KheNodeMeetCount(child_node);  k++ )
    {
      meet = KheNodeMeet(child_node, k);
      if( KheMeetAsst(meet) == NULL )
      {
	fprintf(stderr, "    unassigned meet ");
	KheMeetDebug(meet, 3, 0, stderr);
      }
    }
  }
  KheSolnCostByTypeDebug(soln, 2, 4, stderr);
  /* ***
  fflush(stderr);
  KheSolnDebugDefectsWithTag(soln, KHE_ORDINARY_DEMAND_MONITOR_TAG);
  KheSolnDebugDefectsWithTag(soln, KHE_SPREAD_EVENTS_MONITOR_TAG);
  KheSolnDebugDefectsWithTag(soln, KHE_EVENNESS_MONITOR_TAG);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOneLayerRepairDebug(KHE_NODE parent_node, int i, FILE *fp)       */
/*                                                                           */
/*  Debug print of layer i of parent_node onto fp after repair.              */
/*                                                                           */
/*****************************************************************************/

static void KheOneLayerRepairDebug(KHE_NODE parent_node, int i, FILE *fp)
{
  KHE_LAYER layer;  int j, k;  KHE_NODE child_node;  KHE_MEET meet;
  KHE_SOLN soln;
  soln = KheNodeSoln(parent_node);
  layer = KheNodeChildLayer(parent_node, i);
  fprintf(stderr, "  after repairing layer %d of %d (%d assigned of %d):\n",
    i, KheNodeChildLayerCount(parent_node),
    KheLayerAssignedDuration(layer), KheLayerDuration(layer));
  KheLayerDebug(layer, 1, 4, stderr);
  for( j = 0;  j < KheLayerChildNodeCount(layer);  j++ )
  {
    child_node = KheLayerChildNode(layer, j);
    for( k = 0;  k < KheNodeMeetCount(child_node);  k++ )
    {
      meet = KheNodeMeet(child_node, k);
      if( KheMeetAsst(meet) == NULL )
      {
	fprintf(stderr, "    unassigned meet ");
	KheMeetDebug(meet, 1, 0, stderr);
      }
    }
  }
  KheSolnCostByTypeDebug(soln, 2, 4, stderr);
  fflush(stderr);
  /* ***
  KheSolnDebugDefectsWithTag(soln, KHE_ORDINARY_DEMAND_MONITOR_TAG);
  KheSolnDebugDefectsWithTag(soln, KHE_SPREAD_EVENTS_MONITOR_TAG);
  KheSolnDebugDefectsWithTag(soln, KHE_EVENNESS_MONITOR_TAG);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerDetachNonLocalResourceMonitors(KHE_LAYER layer)             */
/*                                                                           */
/*  Detach the non-local resource monitors of the resources of layer.        */
/*                                                                           */
/*****************************************************************************/

static void KheLayerDetachNonLocalResourceMonitors(KHE_LAYER layer)
{
  KHE_RESOURCE r;  int i, j;  KHE_SOLN soln;  KHE_MONITOR m;
  soln = KheLayerSoln(layer);
  for( i = 0;  i < KheLayerResourceCount(layer);  i++ )
  {
    r = KheLayerResource(layer, i);
    for( j = 0;  j < KheSolnResourceMonitorCount(soln, r);  j++ )
    {
      m = KheSolnResourceMonitor(soln, r, j);
      if( KheMonitorAttachedToSoln(m) ) switch( KheMonitorTag(m) )
      {
	case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
	case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
	/* ***
	case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
	*** */

	  KheMonitorDetachFromSoln(m);
	  break;

	default:

	  break;
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerAttachNonLocalResourceMonitors(KHE_LAYER layer)             */
/*                                                                           */
/*  Detach the limit idle times monitors of the resources of layer.          */
/*                                                                           */
/*****************************************************************************/

static void KheLayerAttachNonLocalResourceMonitors(KHE_LAYER layer)
{
  KHE_RESOURCE r;  int i, j;  KHE_SOLN soln;  KHE_MONITOR m;
  soln = KheLayerSoln(layer);
  for( i = 0;  i < KheLayerResourceCount(layer);  i++ )
  {
    r = KheLayerResource(layer, i);
    for( j = 0;  j < KheSolnResourceMonitorCount(soln, r);  j++ )
    {
      m = KheSolnResourceMonitor(soln, r, j);
      if( !KheMonitorAttachedToSoln(m) ) switch( KheMonitorTag(m) )
      {
	case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
	case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
	/* ***
	case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
	*** */

	  KheMonitorAttachToSoln(m);
	  break;

	default:

	  break;
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LAYER_REPAIR_TYPE KheLayerRepairType(KHE_OPTIONS options,            */
/*    KHE_BACKOFF *bk)                                                       */
/*                                                                           */
/*  Consult the "ts_layer_repair" option of options to decide what kind of   */
/*  layer repair is needed.  Return that value, and if it is exponential     */
/*  backoff, set *bk to a backoff object, else set *bk to NULL.              */
/*                                                                           */
/*****************************************************************************/

static KHE_LAYER_REPAIR_TYPE KheLayerRepairType(KHE_OPTIONS options,
  KHE_BACKOFF *bk, HA_ARENA a)
{
  char *opt;
  opt = KheOptionsGet(options, "ts_layer_repair", "all");
  if( strcmp(opt, "none") == 0 )
  {
    *bk = NULL;
    return KHE_LAYER_REPAIR_NONE;
  }
  else if( strcmp(opt, "exp") == 0 )
  {
    *bk = KheBackoffBegin(KHE_BACKOFF_EXPONENTIAL, a);
    return KHE_LAYER_REPAIR_EXP;
  }
  else if( strcmp(opt, "all") == 0 )
  {
    *bk = NULL;
    return KHE_LAYER_REPAIR_ALL;
  }
  else
  {
    HnAbort("KheLayerRepairType: unknown layer_repair option \"%s\"", opt);
    *bk = NULL;
    return KHE_LAYER_REPAIR_ALL;   /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeLayeredAssignTimes(KHE_NODE parent_node, KHE_OPTIONS options)*/
/*                                                                           */
/*  Assign times to the meets of the child nodes of parent_node, layer       */
/*  by layer.  If no_node_regularity is false, try for node regularity.      */
/*                                                                           */
/*****************************************************************************/

bool KheNodeLayeredAssignTimes(KHE_NODE parent_node, KHE_OPTIONS options)
{
  KHE_SPREAD_EVENTS_CONSTRAINT best_sec;  KHE_LAYER_REPAIR_TYPE repair_type;
  KHE_BACKOFF bk;  KHE_LAYER layer;  int i;  KHE_SOLN soln;  KHE_INSTANCE ins;
  KHE_LAYER_ASST this_layer_asst1, next_layer_asst1;  HA_ARENA a;
  KHE_LAYER_ASST this_layer_asst2, next_layer_asst2;
  bool added_layers, no_node_regularity, layer_swap, save_vizier_node;
  float layer_time_limit;  KHE_TIMER layer_timer;
  /* ***
  KHE_OPTIONS_TIME_LAYER_REPAIR time_layer_repair =
    KheOptionsTimeLayerRepair(options);
  *** */
  /* bool time_vizier_node = KheOptionsEjectorVizierNode(options); */
  /* bool time_repair_backoff = KheOptionsTimeRepairBackoff(options); */

  /* options used by this solver */
  no_node_regularity = KheOptionsGetBool(options, "ts_no_node_regularity",
    false);
  layer_swap = KheOptionsGetBool(options, "ts_layer_swap", false);
  layer_time_limit = KheTimeFromString(KheOptionsGet(options,
    "ts_layer_time_limit", "-"));

  /* boilerplate */
  if( DEBUG4 )
    fprintf(stderr, "[ KheNodeLayeredAssignTimes(Node %d, %s)\n",
      KheNodeSolnIndex(parent_node), no_node_regularity ? "true" : "false");
  soln = KheNodeSoln(parent_node);
  a = KheSolnArenaBegin(soln);
  /* a = HaAre naMake(); */
  ins = KheSolnInstance(soln);
  best_sec = no_node_regularity ? NULL : BestSpreadEventsConstraint(ins);
  /* ***
  if( time_layer_repair == KHE_OPTIONS_TIME_LAYER_REPAIR_LAYER_BACKOFF ||
      time_layer_repair == KHE_OPTIONS_TIME_LAYER_REPAIR_NODE_BACKOFF )
  *** */
  if( layer_swap )
  {
    this_layer_asst1 = KheLayerAsstMake(soln);
    this_layer_asst2 = KheLayerAsstMake(soln);
    next_layer_asst1 = KheLayerAsstMake(soln);
    next_layer_asst2 = KheLayerAsstMake(soln);
  }
  else
  {
    this_layer_asst1 = NULL;
    this_layer_asst2 = NULL;
    next_layer_asst1 = NULL;
    next_layer_asst2 = NULL;
  }
  repair_type = KheLayerRepairType(options, &bk, a);
  /* ***
  if( KheOptionsTimeLayerRepair(options) &&
      KheOptionsTimeLayerRepairBackoff(options) )
    bk = KheBackoffBegin(KHE_BACKOFF_EXPONENTIAL);
  else
    bk = NULL;
  *** */

  /* build layers and detach their limit idles times monitors */
  added_layers = (KheNodeChildLayerCount(parent_node) == 0);
  if( added_layers )
    KheNodeChildLayersMake(parent_node);
  KheNodeChildLayersSort(parent_node, &KheNodeLayeredLayerDiversifyCmp);
  /* KheNodeChildLayersReduce(parent_node); on balance, no */
  for( i = 0;  i < KheNodeChildLayerCount(parent_node);  i++ )
  {
    layer = KheNodeChildLayer(parent_node, i);
    KheLayerDetachNonLocalResourceMonitors(layer);
  }
  if( DEBUG4 )
  {
    fprintf(stderr, "  layers after sorting:\n");
    for( i = 0;  i < KheNodeChildLayerCount(parent_node);  i++ )
    {
      layer = KheNodeChildLayer(parent_node, i);
      KheLayerDebug(layer, 1, 2, stderr);
    }
  }

  /* install layer time limit if required */
  if( layer_time_limit >= 0.0 )
  {
    layer_timer = KheOptionsAddTimer(options, "layer", layer_time_limit);
    if( DEBUG9 )
      fprintf(stderr, "  KheNodeLayeredAssignTimes layer_time_limit %.1f\n",
        layer_time_limit);
  }
  else
    layer_timer = NULL;

  /* assign each layer */
  /* ***
  if( time_node_regularity )
    KheZonesMake(parent_node, best_sec, options);
  *** */
  for( i = 0;  i < KheNodeChildLayerCount(parent_node);  i++ )
  {
    /* start layer time limit, if requested */
    if( layer_time_limit >= 0.0 )
      KheTimerResetStartTime(layer_timer);

    layer = KheNodeChildLayer(parent_node, i);
    if( i == 0 )
    {
      /* first layer */
      if( !no_node_regularity )
	KheLayerParallelAssignTimes(layer, options);
      KheLayerAttachNonLocalResourceMonitors(layer);
      KheElmLayerAssign(layer, best_sec, options);
      if( DEBUG4 )
	KheOneLayerAsstDebug(parent_node, i, false, stderr);
      save_vizier_node = KheOptionsGetBool(options, "es_vizier_node", false);
      KheOptionsSetBool(options, "es_vizier_node", false);
      /* save_time_vizier_node = KheOptionsEjectorVizierNode(options); */
      /* KheOptionsSetEjectorVizierNode(options, false); */
      KheLayerRepair(layer, repair_type, options, bk);
      KheOptionsSetBool(options, "es_vizier_node", save_vizier_node);
      /* KheOptionsSetEjectorVizierNode(options, save_time_vizier_node); */
      if( !no_node_regularity )
      {
	KheLayerInstallZonesInParent(layer);
	KheNodeExtendZones(parent_node);
      }
      KheLayerSetVisitNum(layer, 1);
      if( DEBUG4 )
	KheOneLayerRepairDebug(parent_node, i, stderr);
    }
    else if( KheLayerAssignedDuration(layer) >= KheLayerDuration(layer) )
    {
      /* repair only for this layer */
      KheLayerAttachNonLocalResourceMonitors(layer);
      KheLayerRepair(layer, repair_type, options, bk);
      KheLayerSetVisitNum(layer, 1);
      if( DEBUG4 )
	KheOneLayerRepairDebug(parent_node, i, stderr);
    }
    else if( layer_swap && i < KheNodeChildLayerCount(parent_node) - 1 )
    {
      /* try two layers in each order and see which is best */
      KHE_LAYER next_layer;  KHE_COST cost1, cost2;

      /* assign this layer then the next layer and record cost */
      KheLayerAttachNonLocalResourceMonitors(layer);
      KheLayerAsstBegin(this_layer_asst1, layer);
      KheElmLayerAssign(layer, NULL, options);
      KheLayerRepair(layer, repair_type, options, bk);
      KheLayerAsstEnd(this_layer_asst1);

      next_layer = KheNodeChildLayer(parent_node, i + 1);
      KheLayerAttachNonLocalResourceMonitors(next_layer);
      KheLayerAsstBegin(next_layer_asst1, next_layer);
      KheElmLayerAssign(next_layer, NULL, options);
      KheLayerRepair(next_layer, repair_type, options, bk);
      KheLayerAsstEnd(next_layer_asst1);
      cost1 = KheSolnCost(soln);

      /* undo the assignments just made */
      KheLayerAsstUndo(next_layer_asst1);
      KheLayerAsstUndo(this_layer_asst1);
      KheLayerDetachNonLocalResourceMonitors(layer);
      KheLayerDetachNonLocalResourceMonitors(next_layer);

      /* assign next layer then this layer and record cost */
      KheLayerAttachNonLocalResourceMonitors(next_layer);
      KheLayerAsstBegin(next_layer_asst2, next_layer);
      KheElmLayerAssign(next_layer, NULL, options);
      KheLayerRepair(next_layer, repair_type, options, bk);
      KheLayerAsstEnd(next_layer_asst2);

      KheLayerAttachNonLocalResourceMonitors(layer);
      KheLayerAsstBegin(this_layer_asst2, layer);
      KheElmLayerAssign(layer, NULL, options);
      KheLayerRepair(layer, repair_type, options, bk);
      KheLayerAsstEnd(this_layer_asst2);
      cost2 = KheSolnCost(soln);

      if( cost1 < cost2 )
      {
	/* first was better, save the first layer it assigned */
	KheLayerAsstUndo(this_layer_asst2);
	KheLayerAsstUndo(next_layer_asst2);
	KheLayerAsstRedo(this_layer_asst1);
	KheLayerDetachNonLocalResourceMonitors(next_layer);
	KheLayerSetVisitNum(layer, 1);
	if( DEBUG4 )
	  KheOneLayerAsstDebug(parent_node, i, false, stderr);
      }
      else
      {
	/* second was better, save the first layer it assigned */
	KheLayerAsstUndo(this_layer_asst2);
	KheLayerDetachNonLocalResourceMonitors(layer);
	KheLayerSetVisitNum(next_layer, 1);
	if( DEBUG4 )
	  KheOneLayerAsstDebug(parent_node, i + 1, true, stderr);
      }
    }
    else
    {
      /* just do one layer */
      KheLayerAttachNonLocalResourceMonitors(layer);
      KheElmLayerAssign(layer, NULL, options);
      if( DEBUG4 )
	KheOneLayerAsstDebug(parent_node, i, false, stderr);
      KheLayerRepair(layer, repair_type, options, bk);
      KheLayerSetVisitNum(layer, 1);
      if( DEBUG4 )
	KheOneLayerRepairDebug(parent_node, i, stderr);
    }
    KheNodeChildLayersSort(parent_node, &KheNodeLayeredLayerDiversifyCmp);
  }
  if( layer_time_limit > 0.0 )
    KheOptionsDeleteTimer(options, layer_timer);
  if( added_layers )
    KheNodeChildLayersDelete(parent_node);
  if( DEBUG4 )
  {
    if( bk != NULL )
      KheBackoffDebug(bk, 2, 2, stderr);
    fprintf(stderr, "] KheNodeLayeredAssignTimes returning %s\n",
      KheNodeAllChildMeetsAssigned(parent_node) ? "true" : "false");
  }
  KheSolnArenaEnd(soln, a);
  /* HaArena Delete(a); */
  /* ***
  if( bk != NULL )
    KheBackoffEnd(bk);
  *** */
  return KheNodeAllChildMeetsAssigned(parent_node);
}


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
/*  FILE:         khe_ss_layers.c                                            */
/*  DESCRIPTION:  Layer solvers                                              */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0
#define DEBUG8_NODE 376
#define DEBUG9 0


/*****************************************************************************/
/*                                                                           */
/*  Submodule "layer construction"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE KheEnclosingNode(KHE_MEET meet, KHE_NODE parent_node)           */
/*                                                                           */
/*  Return the child node of parent_node that meet lies within or below,     */
/*  or NULL if none.                                                         */
/*                                                                           */
/*****************************************************************************/

static KHE_NODE KheEnclosingNode(KHE_MEET meet, KHE_NODE parent_node)
{
  KHE_NODE res;
  while( KheMeetNode(meet) == NULL && KheMeetAsst(meet) != NULL )
    meet = KheMeetAsst(meet);
  if( KheMeetNode(meet) == NULL )
    return NULL;
  res = KheMeetNode(meet);
  while( KheNodeParent(res) != NULL && KheNodeParent(res) != parent_node )
    res = KheNodeParent(res);
  return KheNodeParent(res) == parent_node ? res : NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LAYER KheLayerMakeFromResource(KHE_NODE parent_node, KHE_RESOURCE r) */
/*                                                                           */
/*  Make a new layer containing the nodes that contain r.                    */
/*                                                                           */
/*****************************************************************************/

KHE_LAYER KheLayerMakeFromResource(KHE_NODE parent_node, KHE_RESOURCE r)
{
  KHE_LAYER res;  KHE_SOLN soln;  KHE_EVENT e;  KHE_MEET meet;
  KHE_NODE node;  int i, j;
  HnAssert(r != NULL, "KheLayerMakeFromResource: r is NULL");
  res = KheLayerMake(parent_node);
  KheLayerAddResource(res, r);
  soln = KheNodeSoln(parent_node);
  for( i = 0;  i < KheResourceLayerEventCount(r);  i++ )
  {
    e = KheResourceLayerEvent(r, i);
    for( j = 0;  j < KheEventMeetCount(soln, e);  j++ )
    {
      meet = KheEventMeet(soln, e, j);
      node = KheEnclosingNode(meet, parent_node);
      if( node != NULL && !KheLayerContains(res, node) )
      {
	KheLayerAddChildNode(res, node);
	if( DEBUG8 && KheNodeSolnIndex(node) == DEBUG8_NODE )
	{
	  fprintf(stderr, "  KheLayerMakeFromResource(%s) adding node:\n",
	    KheResourceId(r));
	  fprintf(stderr, "    due to meet: ");
	  KheMeetDebug(meet, 1, 0, stderr);
	  KheNodeDebug(node, 4, 2, stderr);
	}
      }
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerIsSubset(KHE_NODE parent_node, KHE_LAYER layer,             */
/*    KHE_LAYER *layer2)                                                     */
/*                                                                           */
/*  If layer is a subset of another layer of parent_node, set *layer2 to     */
/*  the first such layer and return true, otherwise return false.            */
/*                                                                           */
/*****************************************************************************/

static bool KheLayerIsSubset(KHE_NODE parent_node, KHE_LAYER layer,
  KHE_LAYER *layer2)
{
  int i;  KHE_LAYER l2;
  for( i = 0;  i < KheNodeChildLayerCount(parent_node);  i++ )
  {
    l2 = KheNodeChildLayer(parent_node, i);
    if( l2 != layer && KheLayerSubset(layer, l2) )
    {
      *layer2 = l2;
      return true;
    }
  }
  *layer2 = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerPrune(KHE_NODE parent_node, KHE_LAYER *layer)               */
/*                                                                           */
/*  Merge into *layer any layers of parent_node that are subsets of *layer.  */
/*                                                                           */
/*****************************************************************************/

static void KheLayerPrune(KHE_NODE parent_node, KHE_LAYER *layer)
{
  KHE_LAYER layer2;  int i;
  for( i = 0;  i < KheNodeChildLayerCount(parent_node);  i++ )
  {
    layer2 = KheNodeChildLayer(parent_node, i);
    if( layer2 != *layer && KheLayerSubset(layer2, *layer) )
    {
      KheLayerMerge(*layer, layer2, layer);
      i--;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLayerDefaultCmp(const void *t1, const void *t2)                   */
/*                                                                           */
/*  Comparison functions for sorting an array of layers by decreasing size.  */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheLayerDefaultCmp(const void *t1, const void *t2)
{
  KHE_LAYER layer1 = * (KHE_LAYER *) t1;
  KHE_LAYER layer2 = * (KHE_LAYER *) t2;
  int key1 = KheLayerDuration(layer1) - KheLayerMeetCount(layer1);
  int key2 = KheLayerDuration(layer2) - KheLayerMeetCount(layer2);
  if( key1 != key2 )
    return key2 - key1;
  else if( KheLayerDuration(layer1) != KheLayerDuration(layer2) )
    return KheLayerDuration(layer2) - KheLayerDuration(layer1);
  else if( KheLayerChildNodeCount(layer1) != KheLayerChildNodeCount(layer2) )
    return KheLayerChildNodeCount(layer1) - KheLayerChildNodeCount(layer2);
  else
    return KheLayerIndex(layer1) - KheLayerIndex(layer2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeChildLayersMake(KHE_NODE parent_node)                        */
/*                                                                           */
/*  Make a the usual child layers for this parent_node.                      */
/*                                                                           */
/*****************************************************************************/

void KheNodeChildLayersMake(KHE_NODE parent_node)
{
  KHE_SOLN soln;  KHE_INSTANCE ins;  int i;
  KHE_RESOURCE r;  KHE_LAYER layer, layer2;  KHE_NODE child_node;
  if( DEBUG7 )
  {
    fprintf(stderr, "[ KheNodeChildLayersMake(");
    KheNodeDebug(parent_node, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }

  /* delete all existing child layers */
  KheNodeChildLayersDelete(parent_node);

  /* add layers based on all resources */
  soln = KheNodeSoln(parent_node);
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    r = KheInstanceResource(ins, i);
    if( KheResourceLayerEventCount(r) > 0 )
    {
      layer = KheLayerMakeFromResource(parent_node, r);
      if( KheLayerChildNodeCount(layer) == 0 )
	KheLayerDelete(layer);
      else if( KheLayerIsSubset(parent_node, layer, &layer2) )
	KheLayerMerge(layer2, layer, &layer2);
      else
      {
	/* remove any existing layers that are subsets of layer */
        KheLayerPrune(parent_node, &layer);
      }
    }
  }

  /* add one layer for each child node not otherwise included */
  for( i = 0;  i < KheNodeChildCount(parent_node);  i++ )
  {
    child_node = KheNodeChild(parent_node, i);
    if( KheNodeParentLayerCount(child_node) == 0 )
    {
      /* child_node missed out, so add one layer just for it */
      layer = KheLayerMake(parent_node);
      KheLayerAddChildNode(layer, child_node);
    }
  }

  /* sort by decreasing duration and increasing number of nodes */
  /* *** no, leave this to the user
  KheNodeChildLayersSort(parent_node, &KheLayerDefaultCmp);
  *** */

  if( DEBUG7 )
  {
    fprintf(stderr, "  KheNodeChildLayersMake final layers:\n");
    for( i = 0;  i < KheNodeChildLayerCount(parent_node);  i++ )
    {
      layer = KheNodeChildLayer(parent_node, i);
      KheLayerDebug(layer, 2, 2, stderr);
    }
    fprintf(stderr, "] KheNodeChildLayersMake returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "layer reduction"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_LAYER KheNodeFirstAssignedLayer(KHE_NODE child_node)                 */
/*                                                                           */
/*  Return the layer within which child node will first be assigned:  the    */
/*  layer of minimum index among all the layers containing child_node.       */
/*  There must be at least one such layer.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_LAYER KheNodeFirstAssignedLayer(KHE_NODE child_node)
{
  int i, index, min_index;  KHE_LAYER min_layer, layer;
  HnAssert(KheNodeParentLayerCount(child_node) >= 1,
    "KheNodeFirstLayer internal error");
  min_layer = KheNodeParentLayer(child_node, 0);
  min_index = KheLayerParentNodeIndex(min_layer);
  for( i = 1;  i < KheNodeParentLayerCount(child_node);  i++ )
  {
    layer = KheNodeParentLayer(child_node, i);
    index = KheLayerParentNodeIndex(layer);
    if( index < min_index )
    {
      min_index = index;
      min_layer = layer;
    }
  }
  return min_layer;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LAYER KheLayerFirstAllAssignedLayer(KHE_LAYER layer)                 */
/*                                                                           */
/*  Find the first layer at or before which all of layer's nodes will be     */
/*  assigned.  This will either be layer itself or some earlier layer.       */
/*                                                                           */
/*****************************************************************************/

static KHE_LAYER KheLayerFirstAllAssignedLayer(KHE_LAYER layer)
{
  int i, max_index, min_index, layer_index;  KHE_NODE child_node;
  KHE_LAYER max_layer, min_layer;
  if( KheLayerChildNodeCount(layer) == 0 )
    return layer;  /* not likely in practice; empty layers are deleted fast */
  layer_index = KheLayerParentNodeIndex(layer);
  child_node = KheLayerChildNode(layer, 0);
  max_layer = KheNodeFirstAssignedLayer(child_node);
  max_index = KheLayerParentNodeIndex(max_layer);
  for( i=1; i < KheLayerChildNodeCount(layer) && max_index < layer_index; i++ )
  {
    child_node = KheLayerChildNode(layer, i);
    min_layer = KheNodeFirstAssignedLayer(child_node);
    min_index = KheLayerParentNodeIndex(min_layer);
    if( min_index > max_index )
    {
      max_layer = min_layer;
      max_index = min_index;
    }
  }
  HnAssert(max_index <= layer_index,
    "KheLayerFirstAllAssignedLayer internal error");
  return max_layer;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeChildLayersReduce(KHE_NODE parent_node)                      */
/*                                                                           */
/*  Reduce the number of layers of parent_node by identifying layers         */
/*  whose nodes all appear in earlier layers, and deleting those layers      */
/*  while moving their resources to the appropriate earlier layers           */
/*                                                                           */
/*****************************************************************************/

void KheNodeChildLayersReduce(KHE_NODE parent_node)
{
  KHE_LAYER layer, prev_layer;  int i, j, count;  KHE_RESOURCE r;

  if( DEBUG9 )
    fprintf(stderr, "[ KheNodeChildLayersReduce(parent_node)\n");
  count = 0;
  for( i = KheNodeChildLayerCount(parent_node) - 1;  i >= 0;  i-- )
  {
    layer = KheNodeChildLayer(parent_node, i);
    prev_layer = KheLayerFirstAllAssignedLayer(layer);
    if( prev_layer != layer )
    {
      if( DEBUG9 )
      {
	fprintf(stderr, "  merging layer %d: ", KheLayerParentNodeIndex(layer));
	KheLayerDebug(layer, 1, 0, stderr);
	fprintf(stderr,"  into layer %d: ",KheLayerParentNodeIndex(prev_layer));
	KheLayerDebug(prev_layer, 1, 0, stderr);
      }
      for( j = 0;  j < KheLayerResourceCount(layer);  j++ )
      {
	r = KheLayerResource(layer, j);
	KheLayerAddResource(prev_layer, r);
      }
      KheLayerDelete(layer);
      count++;
    }
  }
  if( DEBUG9 )
    fprintf(stderr, "] KheNodeChildLayersReduce ret. (deleted %d layers)\n",
      count);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "layer duration reduction" (private)                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE_REDUCE - a node plus info about its node durations              */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_node_reduce_rec {
  KHE_NODE			node;			/* the node          */
  HA_ARRAY_INT			node_durations;		/* node durations    */
  int				total_node_duration;	/* sum of above      */
} *KHE_NODE_REDUCE;

typedef HA_ARRAY(KHE_NODE_REDUCE) ARRAY_KHE_NODE_REDUCE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE_REDUCE KheNodeReduceMake(KHE_NODE node)                         */
/*                                                                           */
/*  Make a node reduce object, initially with no durations.                  */
/*                                                                           */
/*****************************************************************************/

static KHE_NODE_REDUCE KheNodeReduceMake(KHE_NODE node, HA_ARENA a)
{
  KHE_NODE_REDUCE res;
  HaMake(res, a);
  res->node = node;
  HaArrayInit(res->node_durations, a);
  res->total_node_duration = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeReduceDelete(KHE_NODE_REDUCE nr)                             */
/*                                                                           */
/*  Free nr.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheNodeReduceDelete(KHE_NODE_REDUCE nr)
{
  MArrayFree(nr->node_durations);
  MFree(nr);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeReduceAddDuration(KHE_NODE_REDUCE nr, int durn)              */
/*                                                                           */
/*  Add one duration to nr.                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheNodeReduceAddDuration(KHE_NODE_REDUCE nr, int durn)
{
  HaArrayAddLast(nr->node_durations, durn);
  nr->total_node_duration += durn;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheNodeReduceCmp(const void *t1, const void *t2)                     */
/*                                                                           */
/*  Comparison function for sorting node reduce objects into increasing      */
/*  total duration order.                                                    */
/*                                                                           */
/*****************************************************************************/

static int KheNodeReduceCmp(const void *t1, const void *t2)
{
  KHE_NODE_REDUCE nr1 = * (KHE_NODE_REDUCE *) t1;
  KHE_NODE_REDUCE nr2 = * (KHE_NODE_REDUCE *) t2;
  if( nr1->total_node_duration != nr2->total_node_duration )
    return nr1->total_node_duration - nr2->total_node_duration;
  else
    return KheNodeSolnIndex(nr1->node) - KheNodeSolnIndex(nr2->node);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeReduceCanGoUnder(KHE_NODE_REDUCE nr1, KHE_NODE_REDUCE nr2)   */
/*                                                                           */
/*  Return true if nr1 can go under nr2.                                     */
/*                                                                           */
/*****************************************************************************/

static bool KheNodeReduceCanGoUnder(KHE_NODE_REDUCE nr1, KHE_NODE_REDUCE nr2)
{
  int i, durn1, durn2;
  HnAssert(HaArrayCount(nr1->node_durations)==HaArrayCount(nr2->node_durations),
    "KheNodeReduceCanGoUnder internal error");
  for( i = 0;  i < HaArrayCount(nr1->node_durations);  i++ )
  {
    durn1 = HaArray(nr1->node_durations, i);
    durn2 = HaArray(nr2->node_durations, i);
    if( durn1 + durn2 > KheNodeDuration(nr2->node) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeCanBeSubsumed(ARRAY_KHE_NODE_REDUCE *node_reduces,           */
/*    KHE_NODE_REDUCE nr, KHE_NODE_REDUCE *nr2)                              */
/*                                                                           */
/*  Search node_reduces for the first node that nr can go under, and         */
/*  return true, setting *nr2 to that node, if found.                        */
/*                                                                           */
/*****************************************************************************/

static bool KheNodeCanBeSubsumed(ARRAY_KHE_NODE_REDUCE *node_reduces,
  KHE_NODE_REDUCE nr, KHE_NODE_REDUCE *nr2)
{
  int i;  KHE_NODE_REDUCE nr1;
  HaArrayForEach(*node_reduces, nr1, i)
    if( nr != nr1 && KheNodeReduceCanGoUnder(nr, nr1) )
    {
      *nr2 = nr1;
      return true;
    }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeReduceMergeInto(KHE_NODE_REDUCE nr1, KHE_NODE_REDUCE nr2)    */
/*                                                                           */
/*  Merge nr1 into nr2, removing all trace of nr1.                           */
/*                                                                           */
/*****************************************************************************/

static void KheNodeReduceMergeInto(KHE_NODE_REDUCE nr1, KHE_NODE_REDUCE nr2)
{
  int i, durn1, durn2;
  for( i = 0;  i < HaArrayCount(nr1->node_durations);  i++ )
  {
    durn1 = HaArray(nr1->node_durations, i);
    durn2 = HaArray(nr2->node_durations, i);
    HaArrayPut(nr2->node_durations, i, durn1 + durn2);
  }
  nr2->total_node_duration += nr1->total_node_duration;
  if( DEBUG2 )
  {
    fprintf(stderr, "      reducing Node %d to child of Node %d\n",
      KheNodeSolnIndex(nr1->node), KheNodeSolnIndex(nr2->node));
    KheNodeDebug(nr1->node, 2, 6, stderr);
    KheNodeDebug(nr2->node, 2, 6, stderr);
  }
  KheNodeMove(nr1->node, nr2->node);
  /* KheNodeReduceDelete(nr1); */
  if( DEBUG5 )
    KheNodeDebug(nr2->node, 3, 4, stderr);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeReduceDebug(KHE_NODE_REDUCE nr, int indent, FILE *fp)        */
/*                                                                           */
/*  One-line debug print of nr.                                              */
/*                                                                           */
/*****************************************************************************/

static void KheNodeReduceDebug(KHE_NODE_REDUCE nr, int indent, FILE *fp)
{
  int durn, i;
  fprintf(stderr, "%*sNode %3d (duration %d, resource durn %2d:", indent, "",
    KheNodeSolnIndex(nr->node), KheNodeDuration(nr->node),
    nr->total_node_duration);
  HaArrayForEach(nr->node_durations, durn, i)
    fprintf(stderr, " %d", durn);
  fprintf(stderr, ")\n");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCoordinateReduceLayerDuration(KHE_LAYER layer, int over)         */
/*                                                                           */
/*  Try to reduce the duration of layer.  It currently exceeds the duration  */
/*  of its parent node by over.  Return true if anything changed.            */
/*                                                                           */
/*****************************************************************************/

static bool KheCoordinateReduceLayerDuration(KHE_LAYER layer, int over,
  HA_ARENA a)
{
  KHE_NODE /* parent_node, */ node;  KHE_RESOURCE r;  int i, j;  bool res;
  ARRAY_KHE_NODE_REDUCE node_reduces;  KHE_NODE_REDUCE nr1, nr2;
  if( DEBUG5 )
  {
    fprintf(stderr, "[ KheCoordinateReduceLayerDuration(layer, %d):\n", over);
    KheLayerDebug(layer, 2, 2, stderr);
  }

  /* build and sort the node reduce objects */
  /* parent_node = KheLayerParentNode(layer); */
  HaArrayInit(node_reduces, a);
  for( i = 0;  i < KheLayerChildNodeCount(layer);  i++ )
  {
    node = KheLayerChildNode(layer, i);
    nr1 = KheNodeReduceMake(node, a);
    for( j = 0;  j < KheLayerResourceCount(layer);  j++ )
    {
      r = KheLayerResource(layer, j);
      KheNodeReduceAddDuration(nr1, KheNodeResourceDuration(node, r));
    }
    HaArrayAddLast(node_reduces, nr1);
  }
  HaArraySort(node_reduces, &KheNodeReduceCmp);
  if( DEBUG5 )
  {
    fprintf(stderr, "  resources: ");
    for( j = 0;  j < KheLayerResourceCount(layer);  j++ )
    {
      r = KheLayerResource(layer, j);
      fprintf(stderr, "%s%s", j > 0 ? ", " : "", KheResourceId(r));
    }
    fprintf(stderr, "\n");
    HaArrayForEach(node_reduces, nr1, i)
      KheNodeReduceDebug(nr1, 2, stderr);
  }

  /* try each node in turn until all tried or overload fixed */
  res = false;
  for( i = 0;  i < HaArrayCount(node_reduces) && over > 0;  i++ )
  {
    nr1 = HaArray(node_reduces, i);
    if( KheNodeCanBeSubsumed(&node_reduces, nr1, &nr2) )
    {
      if( DEBUG5 )
      {
	fprintf(stderr, "    moving node:\n");
	KheNodeReduceDebug(nr1, 6, stderr);
	KheNodeReduceDebug(nr2, 6, stderr);
      }
      over -= KheNodeDuration(nr1->node);
      KheNodeReduceMergeInto(nr1, nr2);
      HaArrayDeleteAndShift(node_reduces, i);
      i--;
      res = true;
    }
  }

  /* clean up and return */
  /* ***
  HaArrayForEach(node_reduces, nr1, i)
    KheNodeReduceDelete(nr1);
  MArrayFree(node_reduces);
  *** */
  if( DEBUG5 )
    fprintf(stderr, "] KheCoordinateReduceLayerDuration returning %s\n",
      res ? "true" : "false");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "layer coordination"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool LayerIsOfInterest(KHE_LAYER layer)                                  */
/*                                                                           */
/*  Return true of layer is of interest (is a candidate for coordinating).   */
/*                                                                           */
/*****************************************************************************/

static bool LayerIsOfInterest(KHE_LAYER layer)
{
  return KheResourcePartition(KheLayerResource(layer, 0)) != NULL &&
    KheLayerDuration(layer) >= 0.9*KheNodeDuration(KheLayerParentNode(layer));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerDominates(KHE_LAYER layer1, KHE_LAYER layer2,               */
/*    int similar_count)                                                     */
/*                                                                           */
/*  When this function is called, node layers layer1 and layer2 have been    */
/*  subjected to KheLayerSimilar, which returned false with the given        */
/*  similar_count.  That being the case, return true if layer1 dominates     */
/*  layer2.                                                                  */
/*                                                                           */
/*  Soln layer layer1 dominates soln layer layer2 if the duration of         */
/*  layer1 is equal to the duration of the parent node, and all but one of   */
/*  the nodes of layer1 is counted in similar_count.  In that case, the      */
/*  leftover nodes of layer2 might as well be moved to below the one         */
/*  leftover node of layer1.                                                 */
/*                                                                           */
/*****************************************************************************/

static bool KheLayerDominates(KHE_LAYER layer1, KHE_LAYER layer2,
  int similar_count)
{
  return KheLayerDuration(layer1)==KheNodeDuration(KheLayerParentNode(layer1))
    && similar_count == KheLayerChildNodeCount(layer1) - 1;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCoordinateTwoLayers(KHE_LAYER layer1, KHE_LAYER layer2,          */
/*    int similar_count)                                                     */
/*                                                                           */
/*  Coordinate layer1 and layer2, whose first similar_count nodes are known  */
/*  to be similar; the remaining layer2 nodes are to be made children of     */
/*  the last layer1 node.                                                    */
/*                                                                           */
/*****************************************************************************/

static void KheCoordinateTwoLayers(KHE_LAYER layer1, KHE_LAYER layer2,
  int similar_count)
{
  KHE_NODE node1, node2;  int i1, i2;
  if( DEBUG2 )
    fprintf(stderr, "  [ KheCoordinateTwoLayers(%s:%d, %s:%d, %d):\n",
      KheResourceId(KheLayerResource(layer1, 0)) != NULL ?
	KheResourceId(KheLayerResource(layer1, 0)) : "-",
      KheLayerChildNodeCount(layer1),
      KheResourceId(KheLayerResource(layer2, 0)) != NULL ?
        KheResourceId(KheLayerResource(layer2, 0)) : "-",
      KheLayerChildNodeCount(layer2), similar_count);

  /* move the first similar_count layer2 nodes to be children of layer1 nodes */
  for( i1 = i2 = 0;  i1 < similar_count;  i1++, i2++ )
  {
    node1 = KheLayerChildNode(layer1, i1);
    node2 = KheLayerChildNode(layer2, i2);
    if( node1 != node2 )
    {
      if( DEBUG2 )
      {
	fprintf(stderr, "      moving Node %d to child of Node %d\n",
	  KheNodeSolnIndex(node2), KheNodeSolnIndex(node1));
	KheNodeDebug(node2, 2, 6, stderr);
	KheNodeDebug(node1, 2, 6, stderr);
      }
      if( !KheNodeMove(node2, node1) )
	HnAbort("KheCoordinateTwoLayers internal error 3");
      i2--;
    }
  }

  /* move the remaining layer2 nodes to be children of the last layer1 node */
  if( similar_count < KheLayerChildNodeCount(layer1) )
  {
    node1 = KheLayerChildNode(layer1, similar_count);
    for( ;  i2 < KheLayerChildNodeCount(layer2);  i2++ )
    {
      node2 = KheLayerChildNode(layer2, i2);
      HnAssert(node1 != node2, "KheCoordinateTwoLayers internal error 4");
      if( DEBUG2 )
      {
	fprintf(stderr, "      moving extra Node %d to child of Node %d\n",
	  KheNodeSolnIndex(node2), KheNodeSolnIndex(node1));
	KheNodeDebug(node2, 2, 6, stderr);
	KheNodeDebug(node1, 2, 6, stderr);
      }
      if( !KheNodeMove(node2, node1) )
	HnAbort("KheCoordinateTwoLayers internal error 5");
      i2--;
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "  ] KheCoordinateTwoLayers returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCoordinateResources(KHE_RESOURCE r1, KHE_RESOURCE r2,            */
/*    KHE_NODE parent_node, bool with_domination)                            */
/*                                                                           */
/*  Coordinate resources r1 and r2.  These are known to lie in the same      */
/*  resource partition, and the duration of their preassigned events is      */
/*  enough to suggest that they might be of interest.                        */
/*                                                                           */
/*****************************************************************************/

static void KheCoordinateResources(KHE_RESOURCE r1, KHE_RESOURCE r2,
  KHE_NODE parent_node, bool with_domination)
{
  KHE_LAYER layer1, layer2;  int similar_count;
  if( DEBUG1 )
  {
    fprintf(stderr, "  [ KheCoordinateResources(");
    KheResourceDebug(r1, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheResourceDebug(r2, 1, -1, stderr);
    fprintf(stderr, ", Node %d, %s)\n", KheNodeSolnIndex(parent_node),
      with_domination ? "true" : "false");
  }

  /* make the two soln layers and return if they are not interesting */
  layer1 = KheLayerMakeFromResource(parent_node, r1);
  if( !LayerIsOfInterest(layer1) )
  {
    KheLayerDelete(layer1);
    if( DEBUG1 )
      fprintf(stderr, "  ] KheCoordinateResources returning (r1 no use)\n");
    return;
  }
  layer2 = KheLayerMakeFromResource(parent_node, r2);
  if( !LayerIsOfInterest(layer2) )
  {
    KheLayerDelete(layer1);
    KheLayerDelete(layer2);
    if( DEBUG1 )
      fprintf(stderr, "  ] KheCoordinateResources returning (r2 no use)\n");
    return;
  }

  /* try to match up the nodes of the two layers */
  if( KheLayerSimilar(layer1, layer2, &similar_count) )
    KheCoordinateTwoLayers(layer1, layer2, similar_count);
  else if( with_domination && KheLayerDominates(layer1, layer2, similar_count) )
    KheCoordinateTwoLayers(layer1, layer2, similar_count);
  else if( with_domination && KheLayerDominates(layer2, layer1, similar_count) )
    KheCoordinateTwoLayers(layer2, layer1, similar_count);

  /* finished with the node layer objects */
  KheLayerDelete(layer1);
  KheLayerDelete(layer2);
  if( DEBUG1 )
    fprintf(stderr, "  ] KheCoordinateResources returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCoordinateResourcePartition(KHE_RESOURCE_GROUP rg,               */
/*    KHE_NODE parent_node, bool with_domination)                            */
/*                                                                           */
/*  Coordinate the resources of partition rg.                                */
/*                                                                           */
/*****************************************************************************/

static void KheCoordinateResourcePartition(KHE_RESOURCE_GROUP rg,
  KHE_NODE parent_node, bool with_domination)
{
  int i1, i2;  KHE_RESOURCE r1, r2;
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheCoordinateResourcePartition(");
    KheResourceGroupDebug(rg, 1, -1, stderr);
    fprintf(stderr, ", Node %d, %s)\n", KheNodeSolnIndex(parent_node),
      with_domination ? "true" : "false");
  }
  for( i1 = 0;  i1 < KheResourceGroupResourceCount(rg);  i1++ )
  {
    r1 = KheResourceGroupResource(rg, i1);
    if( KheResourceLayerDuration(r1) >= 0.9 * KheNodeDuration(parent_node) )
    {
      for( i2 = i1 + 1;  i2 < KheResourceGroupResourceCount(rg);  i2++ )
      {
	r2 = KheResourceGroupResource(rg, i2);
	if( KheResourceLayerDuration(r2) >= 0.9 * KheNodeDuration(parent_node) )
	  KheCoordinateResources(r1, r2, parent_node, with_domination);
      }
    }
  }
  if( DEBUG1 )
    fprintf(stderr, "] KheCoordinateResourcePartition returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCoordinateLayers(KHE_NODE parent_node, bool with_domination)     */
/*                                                                           */
/*  Solver for coordinating the soln layers of parent_node.                  */
/*                                                                           */
/*****************************************************************************/

void KheCoordinateLayers(KHE_NODE parent_node, bool with_domination)
{
  int i, j;  KHE_RESOURCE_TYPE rt;  KHE_RESOURCE_GROUP rg;  KHE_INSTANCE ins;
  KHE_LAYER layer;  HA_ARENA a;  KHE_SOLN soln;
  if( DEBUG1 )
    fprintf(stderr, "[ KheCoordinateLayers(Node %d)\n",
      KheNodeSolnIndex(parent_node));

  /* do the coordinating proper */
  soln = KheNodeSoln(parent_node);
  a = KheSolnArenaBegin(soln);
  /* a = HaArena Make(); */
  ins = KheSolnInstance(KheNodeSoln(parent_node));
  for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
  {
    rt = KheInstanceResourceType(ins, i);
    if( KheResourceTypeHasPartitions(rt) )
      for( j = 0;  j < KheResourceTypeResourceGroupCount(rt);  j++ )
      {
	rg = KheResourceTypeResourceGroup(rt, j);
	if( KheResourceGroupIsPartition(rg) )
	  KheCoordinateResourcePartition(rg, parent_node, with_domination);
      }
  }

  /* try to reduce the duration of oversize layers */
  KheNodeChildLayersMake(parent_node);
  for( i = 0;  i < KheNodeChildLayerCount(parent_node);  i++ )
  {
    layer = KheNodeChildLayer(parent_node, i);
    if( KheLayerDuration(layer) > KheNodeDuration(parent_node) )
    {
      if( KheCoordinateReduceLayerDuration(layer,
	  KheLayerDuration(layer) - KheNodeDuration(parent_node), a) )
      {
	/* if successful, the layers change and need reconstructing */
	KheNodeChildLayersMake(parent_node);
      }
    }
  }
  KheNodeChildLayersDelete(parent_node);

  if( DEBUG1 )
  {
    fprintf(stderr, "  at end:\n");
    KheNodeDebug(parent_node, 3, 2, stderr);
    fprintf(stderr, "] KheCoordinateLayers returning\n");
  }
  KheSolnArenaEnd(soln, a);
  /* HaArena Delete(a); */
}

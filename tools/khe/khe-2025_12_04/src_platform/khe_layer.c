
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
/*  FILE:         khe_layer.c                                                */
/*  DESCRIPTION:  Solution layers and layerings                              */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_LAYER                                                                */
/*                                                                           */
/*****************************************************************************/

struct khe_layer_rec {
  void			*back;			/* back pointer              */
  int			visit_num;		/* visit number              */
  int			reference_count;	/* reference count           */
  KHE_NODE		parent_node;		/* the parent node           */
  int			parent_node_index;	/* index in parent node      */
  ARRAY_KHE_NODE	child_nodes;		/* the child nodes           */
  SSET			child_nodes_sset;	/* child node indexes        */
  ARRAY_KHE_RESOURCE	resources;		/* resources                 */
  int			duration;		/* duration of nodes         */
  int			meet_count;		/* number of meets in nodes  */
  KHE_LAYER		copy;			/* used when copying         */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "back pointers"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLayerKernelSetBack(KHE_LAYER layer, void *back)                  */
/*                                                                           */
/*  Set the back pointer of layer to back, assuming all is well.             */
/*                                                                           */
/*****************************************************************************/

void KheLayerKernelSetBack(KHE_LAYER layer, void *back)
{
  KheSolnOpLayerSetBack(KheLayerSoln(layer), layer,
    layer->back, back);
  layer->back = back;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerKernelSetBackUndo(KHE_LAYER layer, void *old_back,          */
/*    void *new_back)                                                        */
/*                                                                           */
/*  Undo KheLayerKernelSetBack.                                              */
/*                                                                           */
/*****************************************************************************/

void KheLayerKernelSetBackUndo(KHE_LAYER layer, void *old_back, void *new_back)
{
  layer->back = old_back;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerSetBack(KHE_LAYER layer, void *back)                        */
/*                                                                           */
/*  Set the back pointer of layer.                                           */
/*                                                                           */
/*****************************************************************************/

void KheLayerSetBack(KHE_LAYER layer, void *back)
{
  if( back != layer->back )
    KheLayerKernelSetBack(layer, back);
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheLayerBack(KHE_LAYER layer)                                      */
/*                                                                           */
/*  Return the back pointer of layer.                                        */
/*                                                                           */
/*****************************************************************************/

void *KheLayerBack(KHE_LAYER layer)
{
  return layer->back;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "visit numbers"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLayerSetVisitNum(KHE_LAYER layer, int num)                       */
/*                                                                           */
/*  Set the visit number of layer.                                           */
/*                                                                           */
/*****************************************************************************/

void KheLayerSetVisitNum(KHE_LAYER layer, int num)
{
  layer->visit_num = num;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLayerVisitNum(KHE_LAYER layer)                                    */
/*                                                                           */
/*  Return the visit number of layer.                                        */
/*                                                                           */
/*****************************************************************************/

int KheLayerVisitNum(KHE_LAYER layer)
{
  return layer->visit_num;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerVisited(KHE_LAYER layer, int slack)                         */
/*                                                                           */
/*  Return true if layer has been visited recently.                          */
/*                                                                           */
/*****************************************************************************/

bool KheLayerVisited(KHE_LAYER layer, int slack)
{
  return KheSolnGlobalVisitNum(KheLayerSoln(layer)) - layer->visit_num <= slack;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerVisit(KHE_LAYER layer)                                      */
/*                                                                           */
/*  Visit layer.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheLayerVisit(KHE_LAYER layer)
{
  layer->visit_num = KheSolnGlobalVisitNum(KheLayerSoln(layer));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerUnVisit(KHE_LAYER layer)                                    */
/*                                                                           */
/*  Unvisit layer.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheLayerUnVisit(KHE_LAYER layer)
{
  layer->visit_num = KheSolnGlobalVisitNum(KheLayerSoln(layer)) - 1;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "duration, assigned duration, and demand"                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLayerAddDuration(KHE_LAYER layer, int durn)                      */
/*                                                                           */
/*  Add durn to the duration of layer.  Since this is called when a meet     */
/*  is added to one of layer's nodes, also increment meet_count.             */
/*                                                                           */
/*****************************************************************************/

void KheLayerAddDuration(KHE_LAYER layer, int durn)
{
  layer->duration += durn;
  layer->meet_count++;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerSubtractDuration(KHE_LAYER layer, int durn)                 */
/*                                                                           */
/*  Subtract durn from the duration of layer.                                */
/*                                                                           */
/*****************************************************************************/

void KheLayerSubtractDuration(KHE_LAYER layer, int durn)
{
  layer->duration -= durn;
  layer->meet_count--;
  HnAssert(layer->duration >= 0, "KheLayerSubtractDuration internal error 1");
  HnAssert(layer->meet_count >= 0, "KheLayerSubtractDuration internal error 2");
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLayerDuration(KHE_LAYER layer)                                    */
/*                                                                           */
/*  Return the total duration of the nodes of layer.                         */
/*                                                                           */
/*****************************************************************************/

int KheLayerDuration(KHE_LAYER layer)
{
  return layer->duration;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLayerAssignedDuration(KHE_LAYER layer)                            */
/*                                                                           */
/*  Return the total assigned duration of the nodes of layer.                */
/*                                                                           */
/*****************************************************************************/

int KheLayerAssignedDuration(KHE_LAYER layer)
{
  int i, res;  KHE_NODE n;
  res = 0;
  HaArrayForEach(layer->child_nodes, n, i)
    res += KheNodeAssignedDuration(n);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLayerDemand(KHE_LAYER layer)                                      */
/*                                                                           */
/*  Return the total demand of the nodes of layer.                           */
/*                                                                           */
/*****************************************************************************/

int KheLayerDemand(KHE_LAYER layer)
{
  int i, res;  KHE_NODE n;
  res = 0;
  HaArrayForEach(layer->child_nodes, n, i)
    res += KheNodeDemand(n);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "other simple attributes"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE KheLayerParentNode(KHE_LAYER layer)                             */
/*                                                                           */
/*  Return the parent node of layer.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_NODE KheLayerParentNode(KHE_LAYER layer)
{
  return layer->parent_node;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerSetParentNode(KHE_LAYER layer, KHE_NODE parent_node)        */
/*                                                                           */
/*  Set the parent_node attribute of layer.                                  */
/*                                                                           */
/*****************************************************************************/

void KheLayerSetParentNode(KHE_LAYER layer, KHE_NODE parent_node)
{
  layer->parent_node = parent_node;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLayerParentNodeIndex(KHE_LAYER layer)                             */
/*                                                                           */
/*  Return the index number of layer in its parent node.                     */
/*                                                                           */
/*****************************************************************************/

int KheLayerParentNodeIndex(KHE_LAYER layer)
{
  return layer->parent_node_index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerSetParentNodeIndex(KHE_LAYER layer, int parent_node_index)  */
/*                                                                           */
/*  Set the parent_node_index attribute of layer.                            */
/*                                                                           */
/*****************************************************************************/

void KheLayerSetParentNodeIndex(KHE_LAYER layer, int parent_node_index)
{
  layer->parent_node_index = parent_node_index;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheLayerSoln(KHE_LAYER layer)                                   */
/*                                                                           */
/*  Return the solution of layer's parent node.                              */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheLayerSoln(KHE_LAYER layer)
{
  return KheNodeSoln(layer->parent_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerChangeChildNodeIndex(KHE_LAYER layer,                       */
/*    int old_index, int new_index)                                          */
/*                                                                           */
/*  The index number of one of the child nodes of layer has changed from     */
/*  old_index to new_index, so change layer's lset accordingly.              */
/*                                                                           */
/*****************************************************************************/

void KheLayerChangeChildNodeIndex(KHE_LAYER layer,
  int old_index, int new_index)
{
  SSetDelete(layer->child_nodes_sset, old_index);
  SSetInsert(layer->child_nodes_sset, new_index);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLayerMeetCount(KHE_LAYER layer)                                   */
/*                                                                           */
/*  Return the number of meets in the nodes of layer.                        */
/*                                                                           */
/*****************************************************************************/

int KheLayerMeetCount(KHE_LAYER layer)
{
  return layer->meet_count;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "creation and deletion"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_LAYER KheLayerDoMake(HA_ARENA a)                                     */
/*                                                                           */
/*  Obtain a new layer from a; initialize its arrays and sset.               */
/*                                                                           */
/*****************************************************************************/

static KHE_LAYER KheLayerDoMake(HA_ARENA a)
{
  KHE_LAYER res;
  HaMake(res, a);
  HaArrayInit(res->child_nodes, a);
  SSetInit(res->child_nodes_sset, a);
  HaArrayInit(res->resources, a);
  if( DEBUG4 )
    fprintf(stderr, "KheLayerDoMake() = %p (%d after)\n", (void *) res,
      HaArrayCount(res->resources));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerUnMake(KHE_LAYER layer)                                     */
/*                                                                           */
/*  Undo KheLayerDoMake, returning layer's memory to the memory allocator.   */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheLayerUnMake(KHE_LAYER layer)
{
  if( DEBUG4 )
    fprintf(stderr, "KheLayerUnMake(%p) (%d before)\n", (void *) layer,
      HaArrayCount(layer->resources));
  MArrayFree(layer->child_nodes);
  LSetFree(layer->child_nodes_sset);
  MArrayFree(layer->resources);
  MFree(layer);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_LAYER KheLayerDoGet(KHE_SOLN soln)                                   */
/*                                                                           */
/*  Get a layer object, either from soln's free list or allocated.           */
/*                                                                           */
/*****************************************************************************/

static KHE_LAYER KheLayerDoGet(KHE_SOLN soln)
{
  KHE_LAYER res;
  res = KheSolnGetLayerFromFreeList(soln);
  if( res == NULL )
    res = KheLayerDoMake(KheSolnArena(soln));
  res->reference_count = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerUnGet(KHE_LAYER layer)                                      */
/*                                                                           */
/*  Undo KheLayerDoGet, adding layer to its soln's free list.                */
/*                                                                           */
/*****************************************************************************/

static void KheLayerUnGet(KHE_LAYER layer)
{
  KheSolnAddLayerToFreeList(KheLayerSoln(layer), layer);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerReferenceCountIncrement(KHE_LAYER layer)                    */
/*                                                                           */
/*  Increment layer's reference count.                                       */
/*                                                                           */
/*****************************************************************************/

void KheLayerReferenceCountIncrement(KHE_LAYER layer)
{
  layer->reference_count++;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerReferenceCountDecrement(KHE_LAYER layer)                    */
/*                                                                           */
/*  Decrement layer's reference count, and possibly add it to the free list. */
/*                                                                           */
/*****************************************************************************/

void KheLayerReferenceCountDecrement(KHE_LAYER layer)
{
  HnAssert(layer->reference_count >= 1,
    "KheLayerReferenceCountDecrement internal error");
  if( --layer->reference_count == 0 )
    KheLayerUnGet(layer);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerDoAdd(KHE_LAYER layer, KHE_SOLN soln)                       */
/*                                                                           */
/*  Initialize layer and add it to parent_node.                              */
/*                                                                           */
/*****************************************************************************/

static void KheLayerDoAdd(KHE_LAYER layer, KHE_NODE parent_node)
{
  layer->back = NULL;
  layer->visit_num = 0;
  KheLayerReferenceCountIncrement(layer);
  KheNodeAddChildLayer(parent_node, layer);  /* sets parent_node and index */
  HaArrayClear(layer->child_nodes);
  SSetClear(layer->child_nodes_sset);
  if( DEBUG4 )
    fprintf(stderr, "KheLayerDoAdd(%p)\n", (void *) layer);
  HaArrayClear(layer->resources);
  layer->duration = 0;
  layer->meet_count = 0;
  layer->copy = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerUnAdd(KHE_LAYER layer)                                      */
/*                                                                           */
/*  Undo KheLayerDoAdd, leaving layer unlinked from the solution.            */
/*                                                                           */
/*****************************************************************************/

static void KheLayerUnAdd(KHE_LAYER layer)
{
  /* delete from parent node */
  KheNodeDeleteChildLayer(layer->parent_node, layer);

  /* layer is now not referenced from solution (this call may free layer) */
  KheLayerReferenceCountDecrement(layer);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerKernelAdd(KHE_LAYER layer, KHE_NODE parent_node)            */
/*                                                                           */
/*  Kernel operation which adds layer to parent_node (but does not make it). */
/*                                                                           */
/*****************************************************************************/

void KheLayerKernelAdd(KHE_LAYER layer, KHE_NODE parent_node)
{
  KheLayerDoAdd(layer, parent_node);
  KheSolnOpLayerAdd(KheNodeSoln(parent_node), layer, parent_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerKernelAddUndo(KHE_LAYER layer)                              */
/*                                                                           */
/*  Undo KheLayerKernelAdd.                                                  */
/*                                                                           */
/*****************************************************************************/

void KheLayerKernelAddUndo(KHE_LAYER layer)
{
  KheLayerUnAdd(layer);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerKernelDelete(KHE_LAYER layer)                               */
/*                                                                           */
/*  Kernel operation which deletes layer (but does not free it).             */
/*                                                                           */
/*****************************************************************************/

void KheLayerKernelDelete(KHE_LAYER layer)
{
  KheSolnOpLayerDelete(KheLayerSoln(layer), layer, layer->parent_node);
  KheLayerUnAdd(layer);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerKernelDeleteUndo(KHE_LAYER layer, KHE_NODE parent_node)     */
/*                                                                           */
/*  Undo KheLayerKernelDelete.                                               */
/*                                                                           */
/*****************************************************************************/

void KheLayerKernelDeleteUndo(KHE_LAYER layer, KHE_NODE parent_node)
{
  KheLayerDoAdd(layer, parent_node);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LAYER KheLayerMake(KHE_NODE parent_node)                             */
/*                                                                           */
/*  Make a new, empty child layer for parent_node.                           */
/*                                                                           */
/*****************************************************************************/

KHE_LAYER KheLayerMake(KHE_NODE parent_node)
{
  KHE_LAYER res;

  /* make and initialize a new layer object from scratch */
  res = KheLayerDoGet(KheNodeSoln(parent_node));

  /* carry out the kernel add operation */
  KheLayerKernelAdd(res, parent_node);

  /* return it */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerDelete(KHE_LAYER layer)                                     */
/*                                                                           */
/*  Delete and free layer.                                                   */
/*                                                                           */
/*****************************************************************************/

void KheLayerDelete(KHE_LAYER layer)
{
  /* delete resources from layer */
  while( HaArrayCount(layer->resources) > 0 )
    KheLayerDeleteResource(layer, HaArrayLast(layer->resources));

  /* detach layer from child nodes */
  while( HaArrayCount(layer->child_nodes) > 0 )
    KheLayerDeleteChildNode(layer, HaArrayLast(layer->child_nodes));

  /* carry out the kernel delete operation (may free layer) */
  KheLayerKernelDelete(layer);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "copy"                                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_LAYER KheLayerCopyPhase1(KHE_LAYER layer, HA_ARENA a)                */
/*                                                                           */
/*  Carry out Phase 1 of copying layer.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_LAYER KheLayerCopyPhase1(KHE_LAYER layer, HA_ARENA a)
{
  KHE_LAYER copy;  KHE_NODE child_node;  int i;
  if( layer->copy == NULL )
  {
    HaMake(copy, a);
    copy->back = layer->back;
    copy->visit_num = layer->visit_num;
    copy->reference_count = 1;  /* no paths, and layer is linked in */
    copy->parent_node = KheNodeCopyPhase1(layer->parent_node, a);
    copy->parent_node_index = layer->parent_node_index;
    HaArrayInit(copy->child_nodes, a);
    HaArrayForEach(layer->child_nodes, child_node, i)
      HaArrayAddLast(copy->child_nodes, KheNodeCopyPhase1(child_node, a));
    SSetCopy(copy->child_nodes_sset, layer->child_nodes_sset, a);
    HaArrayInit(copy->resources, a);
    HaArrayAppend(copy->resources, layer->resources, i);
    copy->duration = layer->duration;
    copy->meet_count = layer->meet_count;
    layer->copy = copy;
  }
  return layer->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerCopyPhase2(KHE_LAYER layer)                                 */
/*                                                                           */
/*  Carry out Phase 2 of copying layer.                                      */
/*                                                                           */
/*****************************************************************************/

void KheLayerCopyPhase2(KHE_LAYER layer)
{
  KHE_NODE child_node;  int i;
  if( layer->copy != NULL )
  {
    layer->copy = NULL;
    KheNodeCopyPhase2(layer->parent_node);
    HaArrayForEach(layer->child_nodes, child_node, i)
      KheNodeCopyPhase2(child_node);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "child nodes"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLayerDoAddChildNode(KHE_LAYER layer, KHE_NODE child_node)        */
/*                                                                           */
/*  Add child_node to layer.                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheLayerDoAddChildNode(KHE_LAYER layer, KHE_NODE child_node)
{
  HnAssert(!KheLayerContains(layer, child_node),
    "KheLayerAddChildNode: child_node already in layer");
  HaArrayAddLast(layer->child_nodes, child_node);
  layer->duration += KheNodeDuration(child_node);
  layer->meet_count += KheNodeMeetCount(child_node);
  SSetInsert(layer->child_nodes_sset, KheNodeSolnIndex(child_node));
  KheNodeAddParentLayer(child_node, layer);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerDoDeleteChildNode(KHE_LAYER layer, KHE_NODE child_node)     */
/*                                                                           */
/*  Delete child_node from layer.                                            */
/*                                                                           */
/*****************************************************************************/

static void KheLayerDoDeleteChildNode(KHE_LAYER layer, KHE_NODE child_node)
{
  int pos;
  if( !HaArrayContains(layer->child_nodes, child_node, &pos) )
    HnAbort("KheLayerDeleteChildNode: layer does not contain n");
  KheNodeDeleteParentLayer(child_node, layer);
  SSetDelete(layer->child_nodes_sset, KheNodeSolnIndex(child_node));
  layer->duration -= KheNodeDuration(child_node);
  layer->meet_count -= KheNodeMeetCount(child_node);
  HaArrayDeleteAndShift(layer->child_nodes, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerKernelAddChildNode(KHE_LAYER layer, KHE_NODE child_node)    */
/*                                                                           */
/*  Kernel operation for adding child_node to layer.                         */
/*                                                                           */
/*****************************************************************************/

void KheLayerKernelAddChildNode(KHE_LAYER layer, KHE_NODE child_node)
{
  KheSolnOpLayerAddChildNode(KheLayerSoln(layer), layer, child_node);
  KheLayerDoAddChildNode(layer, child_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerKernelAddChildNodeUndo(KHE_LAYER layer, KHE_NODE child_node)*/
/*                                                                           */
/*  Undo KheLayerKernelAddChildNode.                                         */
/*                                                                           */
/*****************************************************************************/

void KheLayerKernelAddChildNodeUndo(KHE_LAYER layer, KHE_NODE child_node)
{
  KheLayerDoDeleteChildNode(layer, child_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerKernelDeleteChildNode(KHE_LAYER layer, KHE_NODE child_node) */
/*                                                                           */
/*  Kernel operation for deleting child_node from layer.                     */
/*                                                                           */
/*****************************************************************************/

void KheLayerKernelDeleteChildNode(KHE_LAYER layer, KHE_NODE child_node)
{
  KheSolnOpLayerDeleteChildNode(KheLayerSoln(layer), layer, child_node);
  KheLayerDoDeleteChildNode(layer, child_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerKernelDeleteChildNodeUndo(KHE_LAYER layer,                  */
/*    KHE_NODE child_node)                                                   */
/*                                                                           */
/*  Undo KheLayerKernelDeleteChildNode.                                      */
/*                                                                           */
/*****************************************************************************/

void KheLayerKernelDeleteChildNodeUndo(KHE_LAYER layer, KHE_NODE child_node)
{
  KheLayerDoAddChildNode(layer, child_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerAddChildNode(KHE_LAYER layer, KHE_NODE child_node)          */
/*                                                                           */
/*  Add child_node to layer.                                                 */
/*                                                                           */
/*****************************************************************************/

void KheLayerAddChildNode(KHE_LAYER layer, KHE_NODE child_node)
{
  KheLayerKernelAddChildNode(layer, child_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerDeleteChildNode(KHE_LAYER layer, KHE_NODE child_node)       */
/*                                                                           */
/*  Delete child_node from layer.                                            */
/*                                                                           */
/*****************************************************************************/

void KheLayerDeleteChildNode(KHE_LAYER layer, KHE_NODE child_node)
{
  KheLayerKernelDeleteChildNode(layer, child_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerChildNodesSort(KHE_LAYER layer,                             */
/*    int(*compar)(const void *, const void *))                              */
/*                                                                           */
/*  Sort the child nodes of layer.                                           */
/*                                                                           */
/*****************************************************************************/

void KheLayerChildNodesSort(KHE_LAYER layer,
  int(*compar)(const void *, const void *))
{
  HaArraySort(layer->child_nodes, compar);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLayerChildNodeCount(KHE_LAYER layer)                              */
/*                                                                           */
/*  Return the number of child nodes in layer.                               */
/*                                                                           */
/*****************************************************************************/

int KheLayerChildNodeCount(KHE_LAYER layer)
{
  return HaArrayCount(layer->child_nodes);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE KheLayerChildNode(KHE_LAYER layer, int i)                       */
/*                                                                           */
/*  Return the ith child node of layer.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_NODE KheLayerChildNode(KHE_LAYER layer, int i)
{
  return HaArray(layer->child_nodes, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resources"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLayerDoAddResource(KHE_LAYER layer, KHE_RESOURCE r)              */
/*                                                                           */
/*  Add r to layer, assuming all is in order.                                */
/*                                                                           */
/*****************************************************************************/

static void KheLayerDoAddResource(KHE_LAYER layer, KHE_RESOURCE r)
{
  HaArrayAddLast(layer->resources, r);
  if( DEBUG4 )
    fprintf(stderr, "KheLayerDoAddResource(%p, %s) (%d after)\n",
      (void *) layer, KheResourceId(r), HaArrayCount(layer->resources));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerDoDeleteResource(KHE_LAYER layer, KHE_RESOURCE r)           */
/*                                                                           */
/*  Delete r from layer, assuming all is in order.                           */
/*                                                                           */
/*****************************************************************************/

static void KheLayerDoDeleteResource(KHE_LAYER layer, KHE_RESOURCE r)
{
  int pos;
  if( DEBUG4 )
  {
    KHE_RESOURCE r2;  int i;
    fprintf(stderr, "KheLayerDoDeleteResource(%p, %s) (%d before)\n",
      (void *) layer, KheResourceId(r), HaArrayCount(layer->resources));
    HaArrayForEach(layer->resources, r2, i)
      fprintf(stderr, "  %s\n", KheResourceId(r2));
  }
  if( !HaArrayContains(layer->resources, r, &pos) )
    HnAbort("KheLayerDeleteResource: layer does not contain r");
  HaArrayDeleteAndShift(layer->resources, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerKernelAddResource(KHE_LAYER layer, KHE_RESOURCE resource)   */
/*                                                                           */
/*  Kernel operation for adding a resource to layer.                         */
/*                                                                           */
/*****************************************************************************/

void KheLayerKernelAddResource(KHE_LAYER layer, KHE_RESOURCE resource)
{
  KheSolnOpLayerAddResource(KheLayerSoln(layer), layer, resource);
  KheLayerDoAddResource(layer, resource);
}


/*****************************************************************************/
/*                                                                           */
/* void KheLayerKernelAddResourceUndo(KHE_LAYER layer, KHE_RESOURCE resource)*/
/*                                                                           */
/*  Undo KheLayerKernelAddResource.                                          */
/*                                                                           */
/*****************************************************************************/

void KheLayerKernelAddResourceUndo(KHE_LAYER layer, KHE_RESOURCE resource)
{
  KheLayerDoDeleteResource(layer, resource);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerKernelDeleteResource(KHE_LAYER layer, KHE_RESOURCE resource)*/
/*                                                                           */
/*  Kernel operation for deleting a resource from layer.                     */
/*                                                                           */
/*****************************************************************************/

void KheLayerKernelDeleteResource(KHE_LAYER layer, KHE_RESOURCE resource)
{
  KheSolnOpLayerDeleteResource(KheLayerSoln(layer), layer, resource);
  KheLayerDoDeleteResource(layer, resource);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerKernelDeleteResourceUndo(KHE_LAYER layer,                   */
/*    KHE_RESOURCE resource)                                                 */
/*                                                                           */
/*  Undo KheLayerKernelDeleteResource.                                       */
/*                                                                           */
/*****************************************************************************/

void KheLayerKernelDeleteResourceUndo(KHE_LAYER layer,
  KHE_RESOURCE resource)
{
  KheLayerDoAddResource(layer, resource);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerAddResource(KHE_LAYER layer, KHE_RESOURCE r)                */
/*                                                                           */
/*  Add r to layer.                                                          */
/*                                                                           */
/*****************************************************************************/

void KheLayerAddResource(KHE_LAYER layer, KHE_RESOURCE r)
{
  int pos;
  if( !HaArrayContains(layer->resources, r, &pos) )
    KheLayerKernelAddResource(layer, r);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerDeleteResource(KHE_LAYER layer, KHE_RESOURCE r)             */
/*                                                                           */
/*  Delete r from layer.                                                     */
/*                                                                           */
/*****************************************************************************/

void KheLayerDeleteResource(KHE_LAYER layer, KHE_RESOURCE r)
{
  KheLayerKernelDeleteResource(layer, r);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLayerResourceCount(KHE_LAYER layer)                               */
/*                                                                           */
/*  Return the number of resources in layer.                                 */
/*                                                                           */
/*****************************************************************************/

int KheLayerResourceCount(KHE_LAYER layer)
{
  return HaArrayCount(layer->resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheLayerResource(KHE_LAYER layer, int i)                    */
/*                                                                           */
/*  Return the i'th resource of layer.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheLayerResource(KHE_LAYER layer, int i)
{
  return HaArray(layer->resources, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "set operations"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerEqual(KHE_LAYER layer1, KHE_LAYER layer2)                   */
/*                                                                           */
/*  Return true if layer1 and layer2 contain the same nodes.                 */
/*                                                                           */
/*****************************************************************************/

bool KheLayerEqual(KHE_LAYER layer1, KHE_LAYER layer2)
{
  return SSetEqual(layer1->child_nodes_sset, layer2->child_nodes_sset);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerSubset(KHE_LAYER layer1, KHE_LAYER layer2)                  */
/*                                                                           */
/*  Return true of all of layer1's nodes are nodes of layer2.                */
/*                                                                           */
/*****************************************************************************/

bool KheLayerSubset(KHE_LAYER layer1, KHE_LAYER layer2)
{
  return SSetSubset(layer1->child_nodes_sset, layer2->child_nodes_sset);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerDisjoint(KHE_LAYER layer1, KHE_LAYER layer2)                */
/*                                                                           */
/*  Return true if layer1 and layer2 have no nodes in common.                */
/*                                                                           */
/*****************************************************************************/

bool KheLayerDisjoint(KHE_LAYER layer1, KHE_LAYER layer2)
{
  return SSetDisjoint(layer1->child_nodes_sset, layer2->child_nodes_sset);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerContains(KHE_LAYER layer1, KHE_NODE n)                      */
/*                                                                           */
/*  Return true if layer1 contains n.                                        */
/*                                                                           */
/*****************************************************************************/

bool KheLayerContains(KHE_LAYER layer1, KHE_NODE n)
{
  int pos;
  return SSetContains(layer1->child_nodes_sset, KheNodeSolnIndex(n), &pos);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheNodeDecreasingDurationCmp(const void *t1, const void *t2)         */
/*                                                                           */
/*  Comparison function for sorting nodes into decreasing duration order,    */
/*  with ties broken by increasing number of meets.                          */
/*                                                                           */
/*****************************************************************************/

static int KheNodeDecreasingDurationCmp(const void *t1, const void *t2)
{
  KHE_NODE node1 = * (KHE_NODE *) t1;
  KHE_NODE node2 = * (KHE_NODE *) t2;
  if( KheNodeDuration(node1) != KheNodeDuration(node2) )
    return KheNodeDuration(node2) - KheNodeDuration(node1);
  else
    return KheNodeMeetCount(node1) - KheNodeMeetCount(node2);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerAlign(KHE_LAYER layer1, KHE_LAYER layer2,                   */
/*    bool (*node_equiv)(KHE_NODE node1, KHE_NODE node2), int *count)        */
/*                                                                           */
/*  Align equivalent nodes of layer1 and layer2, defined according to        */
/*  node_equiv, and setting *count to the number that aligned in this way.   */
/*                                                                           */
/*****************************************************************************/

bool KheLayerAlign(KHE_LAYER layer1, KHE_LAYER layer2,
  bool (*node_equiv)(KHE_NODE node1, KHE_NODE node2), int *count)
{
  int i, j;  KHE_NODE node1, node2, tmp;
  *count = HaArrayCount(layer1->child_nodes);
  if( layer1 == layer2 )
    return true;
  HaArraySort(layer1->child_nodes, &KheNodeDecreasingDurationCmp);
  HaArraySort(layer2->child_nodes, &KheNodeDecreasingDurationCmp);
  for( i = 0;  i < *count;  i++ )
  {
    node1 = HaArray(layer1->child_nodes, i);
    for( j = i;  j < HaArrayCount(layer2->child_nodes);  j++ )
    {
      node2 = HaArray(layer2->child_nodes, j);
      if( node_equiv(node1, node2) )
	break;
    }
    if( j < HaArrayCount(layer2->child_nodes) )
    {
      /* node1 and node2 are similar */
      HaArraySwap(layer2->child_nodes, i, j, tmp);
    }
    else
    {
      /* node1 is not similar to any node of layer2 */
      (*count)--;
      HaArraySwap(layer1->child_nodes, *count, i, tmp);
      i--;
    }
  }
  return HaArrayCount(layer1->child_nodes) == HaArrayCount(layer2->child_nodes) &&
    HaArrayCount(layer1->child_nodes) == *count;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerSame(KHE_LAYER layer1, KHE_LAYER layer2, int *same_count)   */
/*                                                                           */
/*  Align nodes of layer1 and layer2 that are the exact same node.           */
/*                                                                           */
/*****************************************************************************/

static bool KheDoNodeSame(KHE_NODE node1, KHE_NODE node2)
{
  return node1 == node2;
}

bool KheLayerSame(KHE_LAYER layer1, KHE_LAYER layer2, int *same_count)
{
  return KheLayerAlign(layer1, layer2, &KheDoNodeSame, same_count);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerSimilar(KHE_LAYER layer1, KHE_LAYER layer2,                 */
/*    int *similar_count)                                                    */
/*                                                                           */
/*  Compare layer1 and layer2 for similarity, reordering the nodes so that   */
/*  the similar ones come first in both node layers, and setting             */
/*  *similar_count to the number of similarities.                            */
/*                                                                           */
/*****************************************************************************/

bool KheLayerSimilar(KHE_LAYER layer1, KHE_LAYER layer2, int *similar_count)
{
  return KheLayerAlign(layer1, layer2, &KheNodeSimilar, similar_count);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerRegular(KHE_LAYER layer1, KHE_LAYER layer2,                 */
/*    int *regular_count)                                                    */
/*                                                                           */
/*  Return true when layer1 and layer2 are regular, setting *regular_count   */
/*  to the number of pairs of regular nodes.                                 */
/*                                                                           */
/*****************************************************************************/

static bool KheDoNodeRegular(KHE_NODE node1, KHE_NODE node2)
{
  int junk;
  return KheNodeRegular(node1, node2, &junk);
}

bool KheLayerRegular(KHE_LAYER layer1, KHE_LAYER layer2, int *regular_count)
{
  return KheLayerAlign(layer1, layer2, &KheDoNodeRegular, regular_count);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerMerge(KHE_LAYER layer1, KHE_LAYER layer2, KHE_LAYER *res)   */
/*                                                                           */
/*  Merge layer1 and layer2, producing *res (solver, not kernel).            */
/*                                                                           */
/*****************************************************************************/

void KheLayerMerge(KHE_LAYER layer1, KHE_LAYER layer2, KHE_LAYER *res)
{
  KHE_NODE child_node;  KHE_RESOURCE r;  int i, pos;
  HnAssert(layer1 != layer2,
    "KheLayerMerge: layer1 and layer2 are the same layer");
  HnAssert(layer1->parent_node == layer2->parent_node,
    "KheLayerMerge: layer1 and layer2 have different parent nodes");
  HaArrayForEach(layer2->child_nodes, child_node, i)
    if( !KheLayerContains(layer1, child_node) )
      KheLayerAddChildNode(layer1, child_node);
  HaArrayForEach(layer2->resources, r, i)
    if( !HaArrayContains(layer1->resources, r, &pos) )
      KheLayerAddResource(layer1, r);
  KheLayerDelete(layer2);
  *res = layer1;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerAssignedDurationDebug(KHE_LAYER layer, int verbosity,       */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of layer's assigned duration.                                */
/*                                                                           */
/*****************************************************************************/

void KheLayerAssignedDurationDebug(KHE_LAYER layer, int verbosity,
  int indent, FILE *fp)
{
  int i;  KHE_NODE n;
  fprintf(fp, "%*s[ layer assigned duration:\n", indent, "");
  HaArrayForEach(layer->child_nodes, n, i)
    KheNodeAssignedDurationDebug(n, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerDebug(KHE_LAYER layer, int verbosity, int indent, FILE *fp) */
/*                                                                           */
/*  Debug print of layer onto fp.                                            */
/*                                                                           */
/*****************************************************************************/

void KheLayerDebug(KHE_LAYER layer, int verbosity, int indent, FILE *fp)
{
  KHE_RESOURCE r;  KHE_NODE node;  int i;  bool oversize;
  if( verbosity > 0 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    oversize = (layer->duration > KheNodeDuration(layer->parent_node));
    fprintf(fp, "[ Layer: %d nodes, durn %d meet %d adur %d %s(",
      HaArrayCount(layer->child_nodes), KheLayerDuration(layer),
      KheLayerMeetCount(layer), KheLayerAssignedDuration(layer),
      oversize ? "[OVERSIZE] " : "");
    if( verbosity <= 2 && HaArrayCount(layer->resources) > 5 )
    {
      for( i = 0;  i < 4;  i++ )
      {
	r = HaArray(layer->resources, i);
	fprintf(fp, "%s%s", i == 0 ? "" : ", ",
	KheResourceId(r) == NULL ? "-" : KheResourceId(r));
      }
      r = HaArrayLast(layer->resources);
      fprintf(fp, ", ... , %s",
	KheResourceId(r) == NULL ? "-" : KheResourceId(r));
    }
    else
    {
      HaArrayForEach(layer->resources, r, i)
	fprintf(fp, "%s%s", i == 0 ? "" : ", ",
	KheResourceId(r) == NULL ? "-" : KheResourceId(r));
    }
    fprintf(fp, ")");
    if( indent >= 0 && verbosity > 2 && KheLayerAssignedDuration(layer) > 1 )
    {
      fprintf(fp, "\n");
      KheLayerAssignedDurationDebug(layer, verbosity, indent + 2, fp);
    }
    if( indent >= 0 && (oversize || verbosity > 2) )
    {
      fprintf(fp, "\n");
      fprintf(fp, "%*sparent:\n", indent + 2, "");
      KheNodeDebug(layer->parent_node, 1, indent + 4, fp);
      if( HaArrayCount(layer->child_nodes) > 0 )
      {
	fprintf(fp, "%*s%s:\n", indent + 2, "",
	  HaArrayCount(layer->child_nodes) == 1 ? "child" : "children");
	HaArrayForEach(layer->child_nodes, node, i)
	  KheNodeDebug(node, verbosity, indent + 4, fp);
      }
      fprintf(fp, "%*s]\n", indent, "");
    }
    else
    {
      fprintf(fp, " ]");
      if( indent >= 0 )
	fprintf(fp, "\n");
    }
  }
}

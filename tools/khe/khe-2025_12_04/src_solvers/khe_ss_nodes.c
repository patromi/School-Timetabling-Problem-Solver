
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
/*  FILE:         khe_ss_nodes.c                                             */
/*  DESCRIPTION:  Rearranging nodes                                          */
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

typedef HA_ARRAY(KHE_MEET) ARRAY_KHE_MEET;

typedef struct {
  KHE_MEET		meet;
  KHE_MEET		target_meet;
  int			offset;
} KHE_MEET_ASST;

typedef HA_ARRAY(KHE_MEET_ASST) ARRAY_MEET_ASST;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "meet assignment saver"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAndDomainDebug(KHE_MEET meet, FILE *fp)                      */
/*                                                                           */
/*  Debug print of meet and its domain onto fp.                              */
/*                                                                           */
/*****************************************************************************/

static void KheMeetAndDomainDebug(KHE_MEET meet, FILE *fp)
{
  KHE_TIME_GROUP tg;
  KheMeetDebug(meet, 1, -1, fp);
  tg = KheMeetDomain(meet);
  fprintf(fp, " : ");
  if( tg == NULL )
    fprintf(fp, "NULL");
  else
    KheTimeGroupDebug(tg, 2, -1, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAsstChainDebug(KHE_MEET meet, int indent, FILE *fp)          */
/*                                                                           */
/*  Debug the chain of assignments leading out of meet.                      */
/*                                                                           */
/*****************************************************************************/

static void KheMeetAsstChainDebug(KHE_MEET meet, int indent, FILE *fp)
{
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  KheMeetAndDomainDebug(meet, fp);
  while( KheMeetAsst(meet) != NULL )
  {
    fprintf(fp, " --%d-> ", KheMeetAsstOffset(meet));
    meet = KheMeetAsst(meet);
    KheMeetAndDomainDebug(meet, fp);
  }
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAsstDebug(KHE_MEET_ASST asst, int indent, FILE *fp)          */
/*                                                                           */
/*  Debug print of asst onto fp.                                             */
/*                                                                           */
/*****************************************************************************/

static void KheMeetAsstDebug(KHE_MEET_ASST asst, int indent, FILE *fp)
{
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "asst ");
  KheMeetAndDomainDebug(asst.meet, fp);
  fprintf(fp, " --%d-> ", asst.offset);
  KheMeetAsstChainDebug(asst.target_meet, -1, fp);
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAsstSave(ARRAY_MEET_ASST *assts, KHE_MEET meet)              */
/*                                                                           */
/*  Save meet and its current assignment in *assts.                          */
/*                                                                           */
/*****************************************************************************/

static void KheMeetAsstSave(ARRAY_MEET_ASST *assts, KHE_MEET meet)
{
  KHE_MEET_ASST asst;
  HnAssert(KheMeetAsst(meet) != NULL, "KheMeetAsstSave internal error");
  asst.meet = meet;
  asst.target_meet = KheMeetAsst(meet);
  asst.offset = KheMeetAsstOffset(meet);
  if( DEBUG4 )
    KheMeetAsstDebug(asst, 2, stderr);
  HaArrayAddLast(*assts, asst);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAsstSaveFull(ARRAY_MEET_ASST *assts, KHE_MEET meet,          */
/*    KHE_MEET target_meet, int offset)                                      */
/*                                                                           */
/*  Save meet and the given assignment (not necessarily meet's current       */
/*  assignment, but the one we want to restore) in *assts.                   */
/*                                                                           */
/*****************************************************************************/

static void KheMeetAsstSaveFull(ARRAY_MEET_ASST *assts, KHE_MEET meet,
  KHE_MEET target_meet, int offset)
{
  KHE_MEET_ASST asst;
  HnAssert(target_meet != NULL, "KheMeetAsstSaveFull internal error");
  asst.meet = meet;
  asst.target_meet = target_meet;
  asst.offset = offset;
  if( DEBUG4 )
    KheMeetAsstDebug(asst, 2, stderr);
  HaArrayAddLast(*assts, asst);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAsstsRestore(ARRAY_MEET_ASST *assts)                         */
/*                                                                           */
/*  Restore the assts stored in meet.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheMeetAsstsRestore(ARRAY_MEET_ASST *assts)
{
  KHE_MEET_ASST asst;  int i;
  HaArrayForEach(*assts, asst, i)
    if( !KheMeetAssign(asst.meet, asst.target_meet, asst.offset) )
    {
      if( DEBUG2 )
      {
	fprintf(stderr, "  failing KheMeetAssign(");
	KheMeetDebug(asst.meet, 1, -1, stderr);
	fprintf(stderr, ", ");
	KheMeetDebug(asst.target_meet, 1, -1, stderr);
	fprintf(stderr, ", %d)%s%s\n", asst.offset,
	  KheMeetAssignIsFixed(asst.meet) ? " fixed" : "",
	  KheMeetAsst(asst.meet) != NULL ? " assigned" : "");
      }
      HnAbort("KheMeetAsstsRestore failed to assign meet");
    }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "node merging and splitting"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeMergeCheck(KHE_NODE node1, KHE_NODE node2)                   */
/*                                                                           */
/*  Check whether node1 and node2 can be merged.                             */
/*                                                                           */
/*****************************************************************************/

bool KheNodeMergeCheck(KHE_NODE node1, KHE_NODE node2)
{
  /* must have same parent node, or no parent node */
  return KheNodeParent(node1) == KheNodeParent(node2);
    /* && KheNodeSameParentLayers(node1, node2); */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeMerge(KHE_NODE node1, KHE_NODE node2, KHE_NODE *res)         */
/*                                                                           */
/*  Merge node1 and node2 into *res.                                         */
/*                                                                           */
/*****************************************************************************/

bool KheNodeMerge(KHE_NODE node1, KHE_NODE node2, KHE_NODE *res)
{
  KHE_NODE child_node;  KHE_MEET meet, child_meet;  ARRAY_MEET_ASST assts;
  int i;  HA_ARENA a;  KHE_SOLN soln;
  if( DEBUG3 )
    fprintf(stderr, "[ KheNodeMerge(Node %d, Node %d, &res)\n",
      KheNodeSolnIndex(node1), KheNodeSolnIndex(node2));

  if( !KheNodeMergeCheck(node1, node2) )
    return false;

  /* move the children of node2 to node1, saving assignments first */
  soln = KheNodeSoln(node1);
  a = KheSolnArenaBegin(soln);
  /* a = HaAr enaMake(); */
  HaArrayInit(assts, a);
  while( KheNodeChildCount(node2) > 0 )
  {
    child_node = KheNodeChild(node2, 0);
    for( i = 0;  i < KheNodeMeetCount(child_node);  i++ )
    {
      child_meet = KheNodeMeet(child_node, i);
      if( KheMeetAsst(child_meet) != NULL )
	KheMeetAsstSave(&assts, child_meet);
    }
    KheNodeDeleteParent(child_node);
    KheNodeAddParent(child_node, node1);
  }

  /* move the meets of node2 to node1 */
  while( KheNodeMeetCount(node2) > 0 )
  {
    meet = KheNodeMeet(node2, 0);
    KheNodeDeleteMeet(node2, meet);
    KheNodeAddMeet(node1, meet);
  }

  /* restore any assignments previously deleted */
  KheMeetAsstsRestore(&assts);
  /* MArrayFree(assts); */
  /* HaArenaD elete(a); */
  KheSolnArenaEnd(soln, a);

  /* delete and free node2 (must work: it has no children or meets) */
  KheNodeDelete(node2);
  *res = node1;
  if( DEBUG3 )
    fprintf(stderr, "] KheNodeMerge returning\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeSplitCheck(KHE_NODE node, int meet_count1, int child_count1) */
/*                                                                           */
/*  Check whether it is safe to split node into two with these attributes.   */
/*                                                                           */
/*****************************************************************************/

/* *** unused and not very useful, so omitted now
bool KheNodeSplitCheck(KHE_NODE node, int meet_count1, int child_count1)
{
  KHE_NODE child_node;  KHE_MEET meet, target_meet;  int i, j, pos;

  HnAssert(0 <= meet_count1 && meet_count1 <= KheNodeMeetCount(node),
    "KheNodeSplitCheck: meet_count1 (%d) out of range 0..%d", meet_count1,
    KheNodeMeetCount(node));
  HnAssert(0 <= child_count1 && child_count1 <= KheNodeChildCount(node),
    "KheNodeSplitCheck: child_count1 (%d) out of range 0..%d", child_count1,
    KheNodeChildCount(node));
  HaArrayForEach(node->child_nodes, child_node, i)
    HaArrayForEach(child_node->meets, meet, j)
    {
      target_meet = KheMeetAsst(meet);
      if( target_meet != NULL )
      {
	if( !MArrayContains(node->meets, target_meet, &pos) )
	  HnAbort("KheNodeSplitCheck internal error");
	if( pos < meet_count1 && j >= child_count1 )
	  return false;
      }
    }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeSplit(KHE_NODE node, int meet_count1, int child_count1,      */
/*    KHE_NODE *res1, KHE_NODE *res2)                                        */
/*                                                                           */
/*  Split node into *res1 and *res2.                                         */
/*                                                                           */
/*****************************************************************************/

/* *** unused and not very useful, so omitted now
bool KheNodeSplit(KHE_NODE node, int meet_count1, int child_count1,
  KHE_NODE *res1, KHE_NODE *res2)
{
  KHE_NODE res, child_node;  KHE_MEET meet;  KHE_LAYER layer;  int i;

  ** check safe to split **
  if( !KheNodeSplitCheck(node, meet_count1, child_count1) )
    return false;

  ** make res, a new node with the same parent and parent layers as node **
  res = KheNodeMake(node->soln);
  if( node->parent_node != NULL )
    KheNodeAddParent(res, node->parent_node);
  HaArrayForEach(node->parent_layers, layer, i)
    KheLayerAddChildNode(layer, res);

  ** move the children at and after index child_count1 to res **
  while( child_count1 < MArraySize(node->child_nodes) )
  {
    child_node = MArrayGet(node->child_nodes, child_count1);
    KheNodeDoDeleteParent(child_node, node);
    KheNodeDoAddParent(child_node, res);
  }

  ** move the meets at and after index meet_count1 to res **
  while( meet_count1 < MArraySize(node->meets) )
  {
    meet = MArrayGet(node->meets, meet_count1);
    KheNodeUncheckedDeleteMeet(node, meet_count1);
    KheMeetSetNodeInternal(meet, res);
    KheNodeUncheckedAddMeet(res, meet);
  }

  ** set return parameters and return **
  *res1 = node;
  *res2 = res;
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "node meet merging and splitting"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheNodeMeetSplit(KHE_NODE node, bool recursive)                     */
/*                                                                           */
/*  Split the meets of node as much as possible.                             */
/*                                                                           */
/*  Implementation note.  KheMeetSplit causes the new meet to be added       */
/*  to the end of node's meets.  This function gets around to it there.      */
/*                                                                           */
/*****************************************************************************/

void KheNodeMeetSplit(KHE_NODE node, bool recursive)
{
  KHE_MEET meet, junk;  int i, durn;
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheNodeMeetSplit(node, %s)\n",
      recursive ? "true" : "false");
    KheNodeDebug(node, 2, 2, stderr);
  }
  for( i = 0;  i < KheNodeMeetCount(node);  i++ )
  {
    meet = KheNodeMeet(node, i);
    for( durn = 1;  durn < KheMeetDuration(meet);  durn++ )
      KheMeetSplit(meet, durn, recursive, &meet, &junk);
  }
  if( DEBUG1 )
  {
    fprintf(stderr, "after splitting:\n");
    KheNodeDebug(node, 2, 2, stderr);
    fprintf(stderr, "  KheNodeMeetSplit returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeMeetMerge(KHE_NODE node, bool recursive)                     */
/*                                                                           */
/*  Merge the meets of node as much as possible.  The recursive parameter    */
/*  is passed to each call to KheMeetMerge.                                  */
/*                                                                           */
/*  Implementation note.  KheMeetMerge causes the departing meet to be       */
/*  removed from the node's meets without otherwise reordering them.         */
/*                                                                           */
/*****************************************************************************/

void KheNodeMeetMerge(KHE_NODE node, bool recursive)
{
  KHE_MEET meet;  int i;
  if( DEBUG1 )
  {
    fprintf(stderr, "  KheNodeMeetMerge(node)\n");
    KheNodeDebug(node, 2, 2, stderr);
  }
  KheNodeMeetSort(node, &KheMeetIncreasingAsstCmp);
  for( i = 0;  i < KheNodeMeetCount(node);  i++ )
  {
    meet = KheNodeMeet(node, i);
    while( i + 1 < KheNodeMeetCount(node) &&
      KheMeetMerge(meet, KheNodeMeet(node, i + 1), recursive, &meet) );
  }
  if( DEBUG1 )
  {
    fprintf(stderr, "after merging:\n");
    KheNodeDebug(node, 2, 2, stderr);
    fprintf(stderr, "] KheNodeMeetMerge returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "moving"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeMoveCheck(KHE_NODE child_node, KHE_NODE parent_node)         */
/*                                                                           */
/*  Check that it is safe to move child_node so that its parent node is      */
/*  parent_node.                                                             */
/*                                                                           */
/*****************************************************************************/

bool KheNodeMoveCheck(KHE_NODE child_node, KHE_NODE parent_node)
{
  return !KheNodeIsDescendant(parent_node, child_node);
  /* ***
  KHE_NODE node;
  ** check for cycles, including a node being its own parent **
  for( node = parent_node;  node != NULL;  node = KheNodeParent(node) )
    if( node == child_node )
      return false;
  return true;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetFindNodeTarget(KHE_MEET meet, KHE_NODE node,                 */
/*    KHE_MEET *target_meet, int *offset)                                    */
/*                                                                           */
/*  If meet is assigned, directly or indirectly, to a meet lying             */
/*  in node, then set *target_meet to that meet and *offset                  */
/*  to meet's offset in *target_meet; else set them to NULL and -1.          */
/*                                                                           */
/*****************************************************************************/

static void KheMeetFindNodeTarget(KHE_MEET meet, KHE_NODE node,
  KHE_MEET *target_meet, int *offset)
{
  int offs;
  offs = 0;
  while( KheMeetNode(meet) != node && KheMeetAsst(meet) != NULL )
  {
    offs += KheMeetAsstOffset(meet);
    meet = KheMeetAsst(meet);
  }
  if( KheMeetNode(meet) == node )
  {
    *target_meet = meet;
    *offset = offs;
  }
  else
  {
    *target_meet = NULL;
    *offset = -1;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeMove(KHE_NODE child_node, KHE_NODE parent_node)              */
/*                                                                           */
/*  Move child_node so that its parent node is parent_node.                  */
/*                                                                           */
/*****************************************************************************/

bool KheNodeMove(KHE_NODE child_node, KHE_NODE parent_node)
{
  KHE_NODE n;  KHE_MEET meet, target_meet;  int i, offset;
  ARRAY_MEET_ASST assts;  HA_ARENA a;  KHE_SOLN soln;

  if( !KheNodeMoveCheck(child_node, parent_node) )
    return false;

  n = KheNodeParent(child_node);
  for( ;  n != NULL && n != parent_node;  n = KheNodeParent(n) );
  if( n == NULL )
  {
    /* parent_node not ancestor of child_node's parent; unassign meets */
    for( i = 0;  i < KheNodeMeetCount(child_node);  i++ )
    {
      meet = KheNodeMeet(child_node, i);
      if( KheMeetAsst(meet) != NULL )
	KheMeetUnAssign(meet);  /* part of node move, so should be OK */
    }

    /* move child_node to parent_node */
    if( !KheNodeDeleteParent(child_node) )
      HnAbort("KheNodeMove internal error 1");
    if( !KheNodeAddParent(child_node, parent_node) )
      HnAbort("KheNodeMove internal error 2");
  }
  else
  {
    /* parent_node is ancestor; unassign meets, remembering targets */
    soln = KheNodeSoln(child_node);
    a = KheSolnArenaBegin(soln);
    /* a = HaAren aMake(); */
    HaArrayInit(assts, a);
    for( i = 0;  i < KheNodeMeetCount(child_node);  i++ )
    {
      meet = KheNodeMeet(child_node, i);
      KheMeetFindNodeTarget(meet, parent_node, &target_meet, &offset);
      if( target_meet != NULL )
      {
	HnAssert(KheMeetNode(target_meet) == parent_node,
	  "KheNodeMove internal error 3");
        KheMeetAsstSaveFull(&assts, meet, target_meet, offset);
      }
      if( KheMeetAsst(meet) != NULL )
	KheMeetUnAssign(meet);   /* part of node move; will be reassigned */
    }

    /* move child_node to parent_node */
    if( !KheNodeDeleteParent(child_node) )
      HnAbort("KheNodeMove internal error 4");
    if( !KheNodeAddParent(child_node, parent_node) )
      HnAbort("KheNodeMove internal error 5");

    /* reassign child_node's meets */
    KheMeetAsstsRestore(&assts);
    /* MArrayFree(assts); */
    KheSolnArenaEnd(soln, a);
    /* HaArena Delete(a); */
  }
  return true;
}

/* *** old version that does not use KHE_MEET_ASST
bool KheNodeMove(KHE_NODE child_node, KHE_NODE parent_node)
{
  KHE_NODE n;  KHE_MEET meet, target_meet;  int i, target_offset;

  if( !KheNodeMoveCheck(child_node, parent_node) )
    return false;

  n = KheNodeParent(child_node);
  for( ;  n != NULL && n != parent_node;  n = KheNodeParent(n) );
  if( n == NULL )
  {
    ** parent_node not ancestor of child_node's parent; unassign meets **
    for( i = 0;  i < KheNodeMeetCount(child_node);  i++ )
    {
      meet = KheNodeMeet(child_node, i);
      if( KheMeetAsst(meet) != NULL )
	KheMeetUn Assign(meet);
    }

    ** move child_node to parent_node **
    if( !KheNodeDeleteParent(child_node) )
      HnAbort("KheNodeMove internal error 1");
    if( !KheNodeAddParent(child_node, parent_node) )
      HnAbort("KheNodeMove internal error 2");
  }
  else
  {
    ** parent_node is ancestor; unassign meets, remembering targets **
    ARRAY_KHE_MEET target_meets;  ARRAY_INT target_offsets;
    MArrayInit(target_meets);
    MArrayInit(target_offsets);
    for( i = 0;  i < KheNodeMeetCount(child_node);  i++ )
    {
      meet = KheNodeMeet(child_node, i);
      KheMeetFindNodeTarget(meet, parent_node, &target_meet, &target_offset);
      HaArrayAddLast(target_meets, target_meet);
      HaArrayAddLast(target_offsets, target_offset);
      if( KheMeetAsst(meet) != NULL )
	KheMeetUn Assign(meet);
    }

    ** move child_node to parent_node **
    if( !KheNodeDeleteParent(child_node) )
      HnAbort("KheNodeMove internal error 3");
    if( !KheNodeAddParent(child_node, parent_node) )
      HnAbort("KheNodeMove internal error 4");

    ** reassign child_node's meets **
    for( i = 0;  i < KheNodeMeetCount(child_node);  i++ )
    {
      meet = KheNodeMeet(child_node, i);
      target_meet = MArrayGet(target_meets, i);
      target_offset = MArrayGet(target_offsets, i);
      if( target_meet != NULL &&
	  !KheMeetAssign(meet, target_meet, target_offset) )
	HnAbort("KheNodeMove internal error 5");
    }
    MArrayFree(target_meets);
    MArrayFree(target_offsets);
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "vizier nodes"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheVizierDebugZones(KHE_NODE node, int indent, FILE *fp)            */
/*                                                                           */
/*  Debug print of the zones of node, for use in testing vizier splits.      */
/*                                                                           */
/*****************************************************************************/

static void KheVizierDebugZones(KHE_NODE node, int indent, FILE *fp)
{
  KHE_MEET meet;  int i, offset, max_offset;  KHE_ZONE zone;
  max_offset = 10;
  fprintf(fp, "%*s[ KheVizierDebugZones(", indent, "");
  KheNodeDebug(node, 1, -1, fp);
  fprintf(fp, ")\n");
  for( i = 0;  i < KheNodeMeetCount(node);  i++ )
  {
    meet = KheNodeMeet(node, i);
    fprintf(fp, "%*s  ", indent, "");
    for( offset = 0;  offset < max_offset;  offset++ )
    {
      if( offset > 0 )
	fprintf(fp, " ");
      if( offset < KheMeetDuration(meet) )
      {
	zone = KheMeetOffsetZone(meet, offset);
	if( zone == NULL )
	  fprintf(fp, "---");
	else
	  fprintf(fp, "Z%02d", KheZoneNodeIndex(zone));
      }
      else
	fprintf(fp, "   ");
    }
    KheMeetDebug(meet, 1, 1, fp);
  }
  fprintf(fp, "%*s] KheVizierDebugZones returning\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE KheNodeVizierMake(KHE_NODE parent_node)                         */
/*                                                                           */
/*  Add a vizier node below parent_node, and return the vizier node.         */
/*                                                                           */
/*****************************************************************************/

KHE_NODE KheNodeVizierMake(KHE_NODE parent_node)
{
  KHE_NODE vizier_node;  KHE_MEET parent_meet, vizier_meet, child_meet;
  int i, j;  KHE_ZONE parent_zone, vizier_zone;  KHE_SOLN soln;
  ARRAY_MEET_ASST meet_assts;  HA_ARENA a;
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheNodeVizierMake(");
    KheNodeDebug(parent_node, 1, -1, stderr);
    fprintf(stderr, ")\n");
    KheVizierDebugZones(parent_node, 2, stderr);
  }

  /* make vizier_node and its meets; save assignments to vizier */
  soln = KheNodeSoln(parent_node);
  a = KheSolnArenaBegin(soln);
  /* a = HaAren aMake(); */
  HaArrayInit(meet_assts, a);
  vizier_node = KheNodeMake(soln);
  for( i = 0;  i < KheNodeMeetCount(parent_node);  i++ )
  {
    parent_meet = KheNodeMeet(parent_node, i);
    vizier_meet = KheMeetMake(soln, KheMeetDuration(parent_meet), NULL);
    KheMeetSetAutoDomain(vizier_meet, true);
    KheNodeAddMeet(vizier_node, vizier_meet);
    for( j = 0;  j < KheMeetAssignedToCount(parent_meet);  j++ )
    {
      child_meet = KheMeetAssignedTo(parent_meet, j);
      if( DEBUG4 )
        KheMeetAsstChainDebug(child_meet, 2, stderr);
      if( KheMeetNode(child_meet) != NULL )
	KheMeetAsstSaveFull(&meet_assts, child_meet, vizier_meet,
	  KheMeetAsstOffset(child_meet));
    }
  }

  /* move parent_node's child nodes and layers to vizier_node; unassign meets */
  KheNodeSwapChildNodesAndLayers(vizier_node, parent_node);

  /* make vizier_node a child of parent_node and assign its meets */
  KheNodeAddParent(vizier_node, parent_node);
  for( i = 0;  i < KheNodeMeetCount(parent_node);  i++ )
  {
    parent_meet = KheNodeMeet(parent_node, i);
    vizier_meet = KheNodeMeet(vizier_node, i);
    KheMeetAssign(vizier_meet, parent_meet, 0);
  }

  /* reassign the meets of the child nodes of parent_node */
  KheMeetAsstsRestore(&meet_assts);
  HaArrayFree(meet_assts);

  if( KheNodeZoneCount(parent_node) > 0 )
  {
    /* make zones in vizier_node corresponding to zones in parent_node */
    for( i = 0;  i < KheNodeZoneCount(parent_node);  i++ )
      KheZoneMake(vizier_node);

    /* vizier node meet zone offsets corr. to parent node meet zone offsets */
    for( i = 0;  i < KheNodeMeetCount(parent_node);  i++ )
    {
      parent_meet = KheNodeMeet(parent_node, i);
      vizier_meet = KheNodeMeet(vizier_node, i);
      for( j = 0;  j < KheMeetDuration(parent_meet);  j++ )
      {
	/* the vizier zone is the one with the same index as the parent zone */
	parent_zone = KheMeetOffsetZone(parent_meet, j);
	if( parent_zone != NULL )
	{
	  vizier_zone = KheNodeZone(vizier_node, KheZoneNodeIndex(parent_zone));
	  KheZoneAddMeetOffset(vizier_zone, vizier_meet, j);
	  /* KheMeetOffsetAddZone(vizier_meet, j, vizier_zone); */
	}
      }
    }

    /* remove zones from the parent node */
    KheNodeDeleteZones(parent_node);
  }

  if( DEBUG1 )
  {
    fprintf(stderr, "] KheNodeVizierMake returning ");
    KheNodeDebug(vizier_node, 1, -1, stderr);
    fprintf(stderr, "\n");
    KheVizierDebugZones(vizier_node, 2, stderr);
  }
  KheSolnArenaEnd(soln, a);
  /* HaArenaD elete(a); */
  return vizier_node;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeVizierDelete(KHE_NODE parent_node)                           */
/*                                                                           */
/*  Delete the vizier node from below parent_node.                           */
/*                                                                           */
/*****************************************************************************/

void KheNodeVizierDelete(KHE_NODE parent_node)
{
  KHE_NODE vizier_node;  int i, j, parent_offset;  KHE_SOLN soln;
  KHE_MEET vizier_meet, parent_meet, child_meet;  HA_ARENA a;
  KHE_ZONE vizier_zone, parent_zone;  ARRAY_MEET_ASST meet_assts;

  /* parent_node must have no child layers, no zones, and one child node */
  HnAssert(KheNodeChildLayerCount(parent_node) == 0,
    "KheNodeVizierDelete: parent_node has child layers");
  HnAssert(KheNodeZoneCount(parent_node) == 0,
    "KheNodeVizierDelete: parent_node has zones");
  HnAssert(KheNodeChildCount(parent_node) == 1,
    "KheNodeVizierDelete: parent_node does not have exactly one child node");
  vizier_node = KheNodeChild(parent_node, 0);
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheNodeVizierDelete(");
    KheNodeDebug(parent_node, 1, -1, stderr);
    fprintf(stderr, ")\n");
    KheVizierDebugZones(vizier_node, 2, stderr);
  }

  /* add zones to parent_node, based on vizier_node's zones and assignments */
  if( KheNodeZoneCount(vizier_node) > 0 )
  {
    /* make zones in parent_node corresponding to zones in vizier_node */
    for( i = 0;  i < KheNodeZoneCount(vizier_node);  i++ )
      KheZoneMake(parent_node);

    /* add parent node meet offsets to the new zones */
    for( i = 0;  i < KheNodeMeetCount(vizier_node);  i++ )
    {
      vizier_meet = KheNodeMeet(vizier_node, i);
      parent_meet = KheMeetAsst(vizier_meet);
      if( parent_meet != NULL )
      {
	parent_offset = KheMeetAsstOffset(vizier_meet);
	for( j = 0;  j < KheMeetDuration(vizier_meet);  j++ )
	{
	  vizier_zone = KheMeetOffsetZone(vizier_meet, j);
	  if( vizier_zone != NULL )
	  {
	    parent_zone = KheNodeZone(parent_node,
	      KheZoneNodeIndex(vizier_zone));
	    if( DEBUG5 )
	    {
	      fprintf(stderr, "  calling KheZoneAddMeetOffset(");
	      KheZoneDebug(parent_zone, 1, -1, stderr);
	      fprintf(stderr, ", ");
	      KheMeetDebug(parent_meet, 1, -1, stderr);
	      fprintf(stderr, ", %d)\n", parent_offset + j);
	    }
	    if( KheMeetOffsetZone(parent_meet, parent_offset + j) == NULL )
	      KheZoneAddMeetOffset(parent_zone, parent_meet, parent_offset + j);
	  }
	}
      }
    }

    /* remove zones from vizier node */
    KheNodeDeleteZones(vizier_node);
  }

  /* save the assignments of the meets of the child nodes of vizier_node */
  soln = KheNodeSoln(vizier_node);
  a = KheSolnArenaBegin(soln);
  /* a = HaAren aMake(); */
  HaArrayInit(meet_assts, a);
  for( i = 0;  i < KheNodeMeetCount(vizier_node);  i++ )
  {
    vizier_meet = KheNodeMeet(vizier_node, i);
    parent_meet = KheMeetAsst(vizier_meet);
    if( parent_meet != NULL )
    {
      for( j = 0;  j < KheMeetAssignedToCount(vizier_meet);  j++ )
      {
	child_meet = KheMeetAssignedTo(vizier_meet, j);
	if( KheMeetNode(child_meet) != NULL )
	  KheMeetAsstSaveFull(&meet_assts, child_meet, parent_meet,
	    KheMeetAsstOffset(vizier_meet) + KheMeetAsstOffset(child_meet));
      }
      KheMeetUnAssign(vizier_meet);  /* vizier meet, will be deleted */
    }
  }

  /* move vizier_node's child nodes and layers to parent_node; unassign meets */
  KheNodeDeleteParent(vizier_node);
  KheNodeSwapChildNodesAndLayers(vizier_node, parent_node);

  /* reassign the meets of the child nodes of vizier_node */
  KheMeetAsstsRestore(&meet_assts);
  /* MArrayFree(meet_assts); */
  KheSolnArenaEnd(soln, a);
  /* HaAre naDelete(a); */

  /* delete vizier_node */
  KheNodeDelete(vizier_node);
  if( DEBUG1 )
  {
    KheVizierDebugZones(parent_node, 2, stderr);
    fprintf(stderr, "] KheNodeVizierDelete\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheFindVizierTarget(KHE_MEET meet, KHE_NODE vizier_node)        */
/*                                                                           */
/*  Find the meet of vizier_node such that assigning meet to it leaves       */
/*  meet's assignment in the parent node unchanged.                          */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static KHE_MEET KheFindVizierTarget(KHE_MEET meet, KHE_NODE vizier_node)
{
  int i;  KHE_MEET vizier_meet;
  if( KheMeetAsst(meet) == NULL )
    return NULL;
  for( i = 0;  i < KheNodeMeetCount(vizier_node);  i++ )
  {
    vizier_meet = KheNodeMeet(vizier_node, i);
    if( KheMeetAsst(vizier_meet) == KheMeetAsst(meet) )
      return vizier_meet;
  }
  HnAbort("KheFindVizierTarget internal error");
  return NULL;  ** keep compiler happy **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMoveChildNode(KHE_NODE child_node, KHE_NODE vizier_node)         */
/*                                                                           */
/*  Move child_node to be a child of vizier_node.  This includes saving      */
/*  the assignments of its meets, and bringing them back after the move.     */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static void KheMoveChildNode(KHE_NODE child_node, KHE_NODE vizier_node)
{
  int i, vizier_offset;  KHE_MEET child_meet, vizier_meet;
  ARRAY_KHE_MEET vizier_meets;  ARRAY_INT vizier_offsets;
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheMoveChildNode(");
    KheNodeDebug(child_node, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheNodeDebug(vizier_node, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }

  ** store the current assignments and ensure all meets are unassigned **
  MArrayInit(vizier_meets);
  MArrayInit(vizier_offsets);
  for( i = 0;  i < KheNodeMeetCount(child_node);  i++ )
  {
    child_meet = KheNodeMeet(child_node, i);
    vizier_meet = KheFindVizierTarget(child_meet, vizier_node);
    HaArrayAddLast(vizier_meets, vizier_meet);
    HaArrayAddLast(vizier_offsets, KheMeetAsstOffset(child_meet));
    if( DEBUG2 )
    {
      fprintf(stderr, "  saving child_meet: ");
      KheMeetDebug(child_meet, 3, 0, stderr);
      fprintf(stderr, "  vizier_meet: ");
      if( vizier_meet == NULL )
	fprintf(stderr, "null\n");
      else
	KheMeetDebug(vizier_meet, 3, 0, stderr);
    }
    if( vizier_meet != NULL )
      KheMeetUnAss ign(child_meet);
  }

  ** move the node **
  if( !KheNodeDeleteParent(child_node) )
    HnAbort("KheMoveChildNode internal error 1");
  if( !KheNodeAddParent(child_node, vizier_node) )
    HnAbort("KheMoveChildNode internal error 2");

  ** assign the meets as recorded **
  for( i = 0;  i < KheNodeMeetCount(child_node);  i++ )
  {
    child_meet = KheNodeMeet(child_node, i);
    vizier_meet = MArrayGet(vizier_meets, i);
    vizier_offset = MArrayGet(vizier_offsets, i);
    if( DEBUG2 )
    {
      fprintf(stderr, "  restoring child_meet: ");
      KheMeetDebug(child_meet, 3, 0, stderr);
      fprintf(stderr, "  vizier_meet: ");
      if( vizier_meet == NULL )
	fprintf(stderr, "null\n");
      else
	KheMeetDebug(vizier_meet, 3, 0, stderr);
    }
    if( vizier_meet != NULL &&
	!KheMeetAssign(child_meet, vizier_meet, vizier_offset) )
      HnAbort("KheMoveChildNode internal error 3");
  }
  MArrayFree(vizier_meets);
  MArrayFree(vizier_offsets);
  if( DEBUG2 )
    fprintf(stderr, "] KheMoveChildNode returning\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE KheNodeInsertVizierNode(KHE_NODE parent_node)                   */
/*                                                                           */
/*  Insert a vizier node directly below parent_node; return the new node.    */
/*                                                                           */
/*****************************************************************************/

/* replaced by KheNodeVizierMake ***
KHE_NODE KheNodeInsertVizierNode(KHE_NODE parent_node)
{
  KHE_MEET parent_meet, vizier_meet;  KHE_NODE vizier_node, child_node;
  KHE_SOLN soln;  int i, offset;  KHE_ZONE parent_zone, vizier_zone;
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheNodeInsertVizierNode(");
    KheNodeDebug(parent_node, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }

  ** build the new vizier node and its meets **
  soln = KheNodeSoln(parent_node);
  vizier_node = KheNodeMake(soln);
  if( !KheNodeAddParent(vizier_node, parent_node) )
    HnAbort("KheNodeInsertVizierNode internal error 1");
  for( i = 0;  i < KheNodeMeetCount(parent_node);  i++ )
  {
    parent_meet = KheNodeMeet(parent_node, i);
    vizier_meet = KheMeetMake(soln, KheMeetDuration(parent_meet), NULL);
    if( !KheMeetSetDomain(vizier_meet, NULL) )
      HnAbort("KheNodeInsertVizierNode internal error 2");
    if( !KheNodeAddMeet(vizier_node, vizier_meet) )
      HnAbort("KheNodeInsertVizierNode internal error 3");
    if( !KheMeetAssign(vizier_meet, parent_meet, 0) )
      HnAbort("KheNodeInsertVizierNode internal error 4");
  }
  if( DEBUG1 )
    KheNodeDebug(vizier_node, 4, 2, stderr);

  ** add parent_node's zones to vizier_node **
  for( i = 0;  i < KheNodeZoneCount(parent_node);  i++ )
    KheZoneMake(vizier_node);
  for( i = 0;  i < KheNodeMeetCount(vizier_node);  i++ )
  {
    vizier_meet = KheNodeMeet(vizier_node, i);
    parent_meet = KheMeetAsst(vizier_meet);
    for( offset = 0;  offset < KheMeetDuration(vizier_meet);  offset++ )
    {
      parent_zone = KheMeetOffsetZone(parent_meet, offset);
      if( parent_zone != NULL )
      {
	vizier_zone = KheNodeZone(vizier_node, KheZoneNodeIndex(parent_zone));
	KheZoneAddMeetOffset(vizier_zone, vizier_meet, offset);
      }
    }
  }

  ** delete parent_node's zones **
  while( KheNodeZoneCount(parent_node) > 0 )
    KheZoneDelete(KheNodeZone(parent_node, 0));

  ** move every child node of parent_node except vizier_node to vizier_node **
  for( i = 0;  i < KheNodeChildCount(parent_node);  i++ )
  {
    child_node = KheNodeChild(parent_node, i);
    if( child_node != vizier_node )
    {
      KheMoveChildNode(child_node, vizier_node);
      i--;
    }
  }
  if( DEBUG1 )
    fprintf(stderr, "] KheNodeInsertVizierNode returning\n");
  return vizier_node;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeRemoveVizierNode(KHE_NODE vizier_node)                       */
/*                                                                           */
/*  Remove vizier_node, including moving its zones to its parent.            */
/*                                                                           */
/*****************************************************************************/

/* replaced by KheNodeVizierDelete ***
void KheNodeRemoveVizierNode(KHE_NODE vizier_node)
{
  KHE_NODE parent_node;  int i, parent_offset, offset;
  KHE_MEET vizier_meet, parent_meet;  KHE_ZONE vizier_zone, parent_zone;
  if( DEBUG1 )
    fprintf(stderr, "[ KheNodeRemoveVizierNode(Node %d)\n",
      KheNodeSolnIndex(vizier_node));
  parent_node = KheNodeParent(vizier_node);
  HnAssert(parent_node != NULL,
    "KheNodeRemoveVizierNode: vizier_node has no parent");

  ** remove parent_node's zones and add vizier_node's zones to parent_node **
  while( KheNodeZoneCount(parent_node) > 0 )
    KheZoneDelete(KheNodeZone(parent_node, 0));
  for( i = 0;  i < KheNodeZoneCount(vizier_node);  i++ )
    KheZoneMake(parent_node);
  for( i = 0;  i < KheNodeMeetCount(vizier_node);  i++ )
  {
    vizier_meet = KheNodeMeet(vizier_node, i);
    parent_meet = KheMeetAsst(vizier_meet);
    if( parent_meet != NULL )
    {
      parent_offset = KheMeetAsstOffset(vizier_meet);
      for( offset = 0;  offset < KheMeetDuration(vizier_meet);  offset++ )
      {
	vizier_zone = KheMeetOffsetZone(vizier_meet, offset);
	if( vizier_zone != NULL )
	{
	  parent_zone = KheNodeZone(parent_node, KheZoneNodeIndex(vizier_zone));
	  KheZoneAddMeetOffset(parent_zone, parent_meet, parent_offset+offset);
	}
      }
    }
  }

  ** bypass vizier_node, delete its meets, and delete it **
  KheNodeBypass(vizier_node);
  while( KheNodeMeetCount(vizier_node) > 0 )
    KheMeetDelete(KheNodeMeet(vizier_node, 0));
  KheNodeDelete(vizier_node);
  if( DEBUG1 )
    fprintf(stderr, "] KheNodeRemoveVizierNode returning\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "flattening"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheNodeBypass(KHE_NODE node)                                        */
/*                                                                           */
/*  Move the children of node to be children of its parent.                  */
/*                                                                           */
/*****************************************************************************/

void KheNodeBypass(KHE_NODE node)
{
  HnAssert(KheNodeParent(node) != NULL, "KheNodeBypass: node has no parent");
  while( KheNodeChildCount(node) > 0 )
    if( !KheNodeMove(KheNodeChild(node, 0), KheNodeParent(node)) )
      HnAbort("KheNodeBypass internal error");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeFlatten(KHE_NODE parent_node)                                */
/*                                                                           */
/*  Flatten the subtree rooted at parent_node.                               */
/*                                                                           */
/*  Implementation note.  The implementation assumes that new children       */
/*  will go on the end.  The loop invariant is "all nodes at indexes         */
/*  less than i have no children, and all proper descendants are still       */
/*  descendants."                                                            */
/*                                                                           */
/*****************************************************************************/

void KheNodeFlatten(KHE_NODE parent_node)
{
  int i;
  for( i = 0;  i < KheNodeChildCount(parent_node);  i++ )
    KheNodeBypass(KheNodeChild(parent_node, i));
}

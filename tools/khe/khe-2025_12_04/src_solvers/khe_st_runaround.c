
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
/*  FILE:         khe_st_runaround.c                                         */
/*  DESCRIPTION:  Time solver for assigning times to runaround nodes         */
/*                                                                           */
/*****************************************************************************/
#include <limits.h>
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0

typedef struct khe_asst_rec *KHE_ASST;
typedef HA_ARRAY(KHE_ASST) ARRAY_KHE_ASST;

typedef struct khe_child_meet_rec *KHE_CHILD_MEET;
typedef HA_ARRAY(KHE_CHILD_MEET) ARRAY_KHE_CHILD_MEET;

typedef struct khe_child_node_rec *KHE_CHILD_NODE;
typedef HA_ARRAY(KHE_CHILD_NODE) ARRAY_KHE_CHILD_NODE;

typedef struct khe_child_layer_rec *KHE_CHILD_LAYER;
typedef HA_ARRAY(KHE_CHILD_LAYER) ARRAY_KHE_CHILD_LAYER;

typedef struct khe_runaround_solver_rec *KHE_RUNAROUND_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  KHE_ASST - one possible assignment to a parent meet                      */
/*                                                                           */
/*****************************************************************************/

struct khe_asst_rec {
  KHE_MEET		meet;			/* the meet to assign to     */
  int			offset;			/* the offset to start at    */
  int			durn;			/* the available duration    */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_CHILD_MEET - a solver object representing a child meet               */
/*                                                                           */
/*****************************************************************************/

struct khe_child_meet_rec {
  KHE_CHILD_NODE	child_node;		/* the enclosing child node  */
  KHE_MEET		meet;			/* the meet represented      */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_CHILD_NODE - a solver object representing a child node               */
/*                                                                           */
/*****************************************************************************/

struct khe_child_node_rec {
  KHE_RUNAROUND_SOLVER  runaround_solver;	/* the enclosing solver      */
  ARRAY_KHE_CHILD_LAYER	child_layers;		/* enclosing child layers    */
  KHE_NODE		node;			/* the node represented      */
  ARRAY_KHE_CHILD_MEET	child_meets;		/* the child meets           */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_CHILD_LAYER - a solver object representing one child layer           */
/*                                                                           */
/*****************************************************************************/

struct khe_child_layer_rec {
  KHE_RUNAROUND_SOLVER  runaround_solver;	/* the enclosing solver      */
  KHE_LAYER		layer;			/* the layer represented     */
  ARRAY_KHE_CHILD_NODE	child_nodes;		/* the child nodes           */
  ARRAY_KHE_ASST	assts;			/* the available assignments */
  int			it_durn;		/* iterator: duration        */
  bool			it_across;		/* iterator: across parent   */
  int			it_shift;		/* iterator: shift           */
  int			it_index;		/* iterator: asst index      */
  int			it_offset;		/* iterator: offset in asst  */
  int			it_variant;		/* iterator: when to stop    */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_RUNAROUND_SOLVER - a solver for timetabling runarounds               */
/*                                                                           */
/*****************************************************************************/

struct khe_runaround_solver_rec {
  HA_ARENA		arena;			/* holds the lot             */
  KHE_NODE		parent_node;		/* the parent node           */
  ARRAY_KHE_CHILD_NODE	child_nodes;		/* one for each child node   */
  ARRAY_KHE_CHILD_LAYER	child_layers;		/* the layers to be solved   */
  int			max_unassigned_durn;	/* max unassigned durn       */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "assignments"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ASST KheAsstMake(KHE_MEET meet, int offset, int durn)                */
/*                                                                           */
/*  Make and return a new asst object with these attributes.                 */
/*                                                                           */
/*****************************************************************************/

static KHE_ASST KheAsstMake(KHE_MEET meet, int offset, int durn, HA_ARENA a)
{
  KHE_ASST res;
  HaMake(res, a);
  res->meet = meet;
  res->offset = offset;
  res->durn = durn;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAsstDelete(KHE_ASST asst)                                        */
/*                                                                           */
/*  Delete asst.                                                             */
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
/*  int KheAsstCmp(const void *t1, const void *t2)                           */
/*                                                                           */
/*  Comparison function for sorting an array of assts into canonical order.  */
/*                                                                           */
/*****************************************************************************/

static int KheAsstCmp(const void *t1, const void *t2)
{
  KHE_ASST asst1 = * (KHE_ASST *) t1;
  KHE_ASST asst2 = * (KHE_ASST *) t2;
  if( asst1->meet != asst2->meet )
    return KheMeetSolnIndex(asst1->meet) - KheMeetSolnIndex(asst2->meet);
  else if( asst1->offset != asst2->offset )
    return asst1->offset - asst2->offset;
  else
  {
    HnAbort("KheAsstCmp internal error");
    return 0;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "child meets"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CHILD_MEET KheChildMeetMake(KHE_CHILD_NODE child_node, KHE_MEET meet)*/
/*                                                                           */
/*  Make and return a new child meet with these attributes.                  */
/*                                                                           */
/*****************************************************************************/

static KHE_CHILD_MEET KheChildMeetMake(KHE_CHILD_NODE child_node,
  KHE_MEET meet, HA_ARENA a)
{
  KHE_CHILD_MEET res;
  HaMake(res, a);
  res->child_node = child_node;
  res->meet = meet;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheChildMeetDelete(KHE_CHILD_MEET cm)                               */
/*                                                                           */
/*  Delete cm.                                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheChildMeetDelete(KHE_CHILD_MEET cm)
{
  MFree(cm);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheChildMeetDebug(KHE_CHILD_MEET cm, int verbosity,                 */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of cm onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheChildMeetDebug(KHE_CHILD_MEET cm, int verbosity,
  int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    KheMeetDebug(cm->meet, verbosity, -1, fp);
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "child nodes"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CHILD_NODE KheChildNodeMake(KHE_RUNAROUND_SOLVER rs, KHE_NODE node)  */
/*                                                                           */
/*  Make a child node with these attributes.                                 */
/*                                                                           */
/*****************************************************************************/

static KHE_CHILD_NODE KheChildNodeMake(KHE_RUNAROUND_SOLVER rs,
  KHE_NODE node, HA_ARENA a)
{
  KHE_CHILD_NODE res;  int i;
  HaMake(res, a);
  res->runaround_solver = rs;
  HaArrayInit(res->child_layers, a);
  res->node = node;
  HaArrayInit(res->child_meets, a);
  for( i = 0;  i < KheNodeMeetCount(node);  i++ )
    HaArrayAddLast(res->child_meets,
      KheChildMeetMake(res, KheNodeMeet(node, i), a));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheChildNodeDelete(KHE_CHILD_NODE cn)                               */
/*                                                                           */
/*  Delete cn.                                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheChildNodeDelete(KHE_CHILD_NODE cn)
{
  MArrayFree(cn->child_layers);
  while( HaArrayCount(cn->child_meets) > 0 )
    KheChildMeetDelete(MArrayRemoveLast(cn->child_meets));
  MArrayFree(cn->child_meets);
  MFree(cn);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheChildNodeRecordAsst(KHE_CHILD_NODE cn, KHE_MEET meet,            */
/*    int offset, int durn)                                                  */
/*                                                                           */
/*  Tell each layer containing cn that it contains a meet assigned to        */
/*  meet at offset and durn.                                                 */
/*                                                                           */
/*****************************************************************************/
static void KheChildLayerRecordAsst(KHE_CHILD_LAYER cl, KHE_MEET meet,
  int offset, int durn, HA_ARENA a);

static void KheChildNodeRecordAsst(KHE_CHILD_NODE cn, KHE_MEET meet,
  int offset, int durn, HA_ARENA a)
{
  KHE_CHILD_LAYER cl;  int i;
  HaArrayForEach(cn->child_layers, cl, i)
    KheChildLayerRecordAsst(cl, meet, offset, durn, a);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheChildNodeCmp(const void *t1, const void *t2)                      */
/*                                                                           */
/*  Comparison function for sorting an array of child nodes so that          */
/*  nodes with more meets come before nodes with fewer meets.                */
/*                                                                           */
/*****************************************************************************/

static int KheChildNodeCmp(const void *t1, const void *t2)
{
  KHE_CHILD_NODE cn1 = * (KHE_CHILD_NODE *) t1;
  KHE_CHILD_NODE cn2 = * (KHE_CHILD_NODE *) t2;
  if( KheNodeMeetCount(cn1->node) != KheNodeMeetCount(cn2->node) )
    return KheNodeMeetCount(cn2->node) - KheNodeMeetCount(cn1->node);
  else
    return KheNodeSolnIndex(cn1->node) - KheNodeSolnIndex(cn2->node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheChildNodeDebug(KHE_CHILD_NODE cn, int verbosity,                 */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug printf of cn onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheChildNodeDebug(KHE_CHILD_NODE cn, int verbosity,
  int indent, FILE *fp)
{
  KHE_CHILD_MEET cm;  int i;
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ Child Node ");
    KheNodeDebug(cn->node, 1, -1, fp);
    if( verbosity >= 2 )
    {
      fprintf(fp, ": ");
      HaArrayForEach(cn->child_meets, cm, i)
      {
	if( i > 0 )
	  fprintf(fp, ", ");
	KheChildMeetDebug(cm, 1, -1, fp);
      }
    }
    fprintf(fp, " ]");
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "asst iteration" - these functions iterate over open assts     */
/*                                                                           */
/*  Implementation note.   The iterator has three parameters:                */
/*                                                                           */
/*    it_durn          Return only positions suited to a meet of this durn   */
/*    it_across        If true, return multiple positions across the asst    */
/*    it_shift         Start at the it_shift'th asst, not at the first       */
/*                                                                           */
/*  It has two values that define the position of the iterator:              */
/*                                                                           */
/*    it_index         Index of the current assignment in cl->assts          */
/*    it_offset        The offset within that current assignment             */
/*                                                                           */
/*  And it has a value that tells it when to stop:                           */
/*                                                                           */
/*    it_variant        Stop when this drops to zero                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSetOffset(KHE_CHILD_LAYER cl, KHE_ASST asst)                     */
/*                                                                           */
/*  This function is private to the iterator.  It has just entered asst;     */
/*  set the offset appropriately for the first spot in that asst.            */
/*                                                                           */
/*****************************************************************************/

static void KheSetOffset(KHE_CHILD_LAYER cl, KHE_ASST asst)
{
  if( cl->it_across )
    cl->it_offset = asst->offset;
  else
    cl->it_offset = asst->offset + (cl->it_shift * cl->it_durn) % asst->durn;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNextAsst(KHE_CHILD_LAYER cl)                                     */
/*                                                                           */
/*  This function is private to the iterator.  We are finished with the      */
/*  current asst, so move the iterator to the start of the next asst with    */
/*  sufficient duration, or finish off the iteration if there is no such.    */
/*                                                                           */
/*****************************************************************************/

static void KheNextAsst(KHE_CHILD_LAYER cl)
{
  KHE_ASST asst;  int stop_index;
  cl->it_variant--;
  if( cl->it_variant <= 0 )
    return;
  stop_index = cl->it_index;
  do
  {
    cl->it_index = (cl->it_index + 1) % HaArrayCount(cl->assts);
    asst = HaArray(cl->assts, cl->it_index);
  }
  while( cl->it_index != stop_index && asst->durn < cl->it_durn );
  if( cl->it_index == stop_index )
    cl->it_variant = 0;
  else
    KheSetOffset(cl, asst);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheChildLayerIteratorNext(KHE_CHILD_LAYER cl)                       */
/*                                                                           */
/*  Assuming cl's iterator has been set up by KheChildLayerIteratorFirst     */
/*  and that we are not off the end, move to the next position.              */
/*                                                                           */
/*****************************************************************************/

static void KheChildLayerIteratorNext(KHE_CHILD_LAYER cl, bool check_stop)
{
  KHE_ASST asst;
  if( cl->it_across )
  {
    /* we are returning every it_durn'th position across the asst */
    cl->it_offset += cl->it_durn;
    asst = HaArray(cl->assts, cl->it_index);
    if( cl->it_offset + cl->it_durn > asst->offset + asst->durn )
      KheNextAsst(cl);
  }
  else
  {
    /* we are returning only one position, depending on the shift */
    KheNextAsst(cl);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheChildLayerIteratorFirst(KHE_CHILD_LAYER cl, int durn,            */
/*    bool across, int shift)                                                */
/*                                                                           */
/*  Set up the iterator and move shift positions along it.                   */
/*                                                                           */
/*****************************************************************************/

static void KheChildLayerIteratorFirst(KHE_CHILD_LAYER cl, int durn,
  bool across, int shift)
{
  KHE_ASST asst;  int i, count;

  /* save the parameters */
  cl->it_durn = durn;
  cl->it_across = across;
  cl->it_shift = shift;

  /* find the initial value of the variant, which is the number of assts */
  /* with sufficient duration; return immediately if variant is zero     */
  cl->it_variant = 0;
  HaArrayForEach(cl->assts, asst, i)
    if( asst->durn >= durn )
      cl->it_variant++;
  if( cl->it_variant == 0 )
    return;

  /* find the first value of the iterator, which is at some offset */
  /* in the shift'th asst with sufficient duration */
  count = shift % cl->it_variant;
  HaArrayForEach(cl->assts, asst, i)
    if( asst->durn >= durn )
    {
      if( count == 0 )
	break;
      count--;
    }
  HnAssert(count == 0, "KheChildLayerIteratorFirst internal error");
  cl->it_index = i;
  KheSetOffset(cl, asst);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheChildLayerIteratorCurr(KHE_CHILD_LAYER cl, KHE_MEET *meet,       */
/*    int *offset)                                                           */
/*                                                                           */
/*  If the assignment iterator is not off the end, set *meet to the          */
/*  current asst's meet and *offset to the current offset within that meet,  */
/*  and return true.  Otherwise return false.                                */
/*                                                                           */
/*****************************************************************************/

static bool KheChildLayerIteratorCurr(KHE_CHILD_LAYER cl, KHE_MEET *meet,
  int *offset)
{
  KHE_ASST asst;
  if( cl->it_variant <= 0 )
    return false;
  asst = HaArray(cl->assts, cl->it_index);
  *meet = asst->meet;
  *offset = cl->it_offset;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  ChildLayerForEachAsst(KHE_CHILD_LAYER cl, int durn, bool across,         */
/*    int shift, KHE_MEET *meet, int *offset)                                */
/*                                                                           */
/*  Iterator which successively returns all the suitable meets and offsets   */
/*  within those meets for assigning a meet of the given duration, starting  */
/*  shift positions along from the first.  If across is true, positsions     */
/*  right across each parent meet are returned; if false, only one position  */
/*  is returned, determined by the shift.                                    */
/*                                                                           */
/*****************************************************************************/

#define ChildLayerForEachAsst(cl, durn, across, shift, meet, offset)	\
  for( KheChildLayerIteratorFirst(cl, durn, across, shift);		\
       KheChildLayerIteratorCurr(cl, meet, offset);			\
       KheChildLayerIteratorNext(cl, true) )


/*****************************************************************************/
/*                                                                           */
/*  void KheChildLayerIteratorDebug(KHE_CHILD_LAYER cl, int durn,            */
/*    bool across, int shift, int verbosity, int indent, FILE *fp)           */
/*                                                                           */
/*  Debug print of the members of iterator                                   */
/*                                                                           */
/*    ChildLayerForEachAsst(cl, durn, across, shift, meet, offset)           */
/*                                                                           */
/*****************************************************************************/

static void KheChildLayerIteratorDebug(KHE_CHILD_LAYER cl, int durn,
  bool across, int shift, int verbosity, int indent, FILE *fp)
{
  KHE_MEET meet;  int offset;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ KheChildLayerIterator(durn %d, across %s, shift %d):\n",
      indent, "", durn, across ? "true" : "false", shift);
    ChildLayerForEachAsst(cl, durn, across, shift, &meet, &offset)
    {
      fprintf(fp, "%*s  ", indent, "");
      KheMeetDebug(meet, 1, -1, fp);
      fprintf(fp, " + %d\n", offset);
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "child layers"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CHILD_LAYER KheChildLayerMake(KHE_RUNAROUND_SOLVER rs,               */
/*    KHE_LAYER layer)                                                       */
/*                                                                           */
/*  Make a child layer with these attributes, but no nodes to begin with.    */
/*                                                                           */
/*****************************************************************************/

static KHE_CHILD_LAYER KheChildLayerMake(KHE_RUNAROUND_SOLVER rs,
  KHE_LAYER layer, HA_ARENA a)
{
  KHE_CHILD_LAYER res;  int i;  KHE_MEET meet;
  HaMake(res, a);
  res->runaround_solver = rs;
  res->layer = layer;
  HaArrayInit(res->child_nodes, a);
  /* ***
  for( i = 0;  i < KheLayerChildNodeCount(layer);  i++ )
    HaArrayAddLast(res->child_nodes,
      KheChildNodeMake(res, KheLayerChildNode(layer, i)));
  *** */
  HaArrayInit(res->assts, a);
  for( i = 0;  i < KheNodeMeetCount(rs->parent_node);  i++ )
  {
    meet = KheNodeMeet(rs->parent_node, i);
    HaArrayAddLast(res->assts, KheAsstMake(meet, 0, KheMeetDuration(meet), a));
  }
  res->it_durn = -1;  /* all these iterator values undefined originally */
  res->it_across = false;
  res->it_shift = -1;
  res->it_index = -1;
  res->it_offset = -1;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheChildLayerAddChildNode(KHE_CHILD_LAYER cl, KHE_CHILD_NODE cn)    */
/*                                                                           */
/*  Add cn to cl (and cl to cn).                                             */
/*                                                                           */
/*****************************************************************************/

static void KheChildLayerAddChildNode(KHE_CHILD_LAYER cl, KHE_CHILD_NODE cn)
{
  HaArrayAddLast(cl->child_nodes, cn);
  HaArrayAddLast(cn->child_layers, cl);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheChildLayerSortChildNodes(KHE_CHILD_LAYER cl)                     */
/*                                                                           */
/*  Sort the child nodes of cl so that nodes with more meets come before     */
/*  nodes with fewer meets.                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheChildLayerSortChildNodes(KHE_CHILD_LAYER cl)
{
  HaArraySort(cl->child_nodes, &KheChildNodeCmp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheChildLayerDelete(KHE_CHILD_LAYER cl)                             */
/*                                                                           */
/*  Delete cl, but not the child nodes; they are deleted separately.         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheChildLayerDelete(KHE_CHILD_LAYER cl)
{
  ** ***
  while( HaArrayCount(cl->child_nodes) > 0 )
    KheChildNodeDelete(MArrayRemoveLast(cl->child_nodes));
  *** **
  MArrayFree(cl->child_nodes);
  while( HaArrayCount(cl->assts) > 0 )
    KheAsstDelete(MArrayRemoveLast(cl->assts));
  MArrayFree(cl->assts);
  MFree(cl);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheOverlaps(KHE_MEET meet1, int offset1, int durn1,                 */
/*    KHE_MEET meet2, int offset2, int durn2)                                */
/*                                                                           */
/*  Return true if these two assignments overlap.                            */
/*                                                                           */
/*****************************************************************************/

static bool KheOverlaps(KHE_MEET meet1, int offset1, int durn1,
  KHE_MEET meet2, int offset2, int durn2)
{
  if( meet1 != meet2 )
    return false;
  if( offset1 + durn1 <= offset2 )
    return false;
  if( offset2 + durn2 <= offset1 )
    return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheContains(int offset1, int durn1, int offset2, int durn2)         */
/*                                                                           */
/*  Return true if offset1..durn1 contains offset2..durn2, given that        */
/*  the two intervals are already known to overlap.                          */
/*                                                                           */
/*****************************************************************************/

static bool KheContains(int offset1, int durn1, int offset2, int durn2)
{
  return offset2 >= offset1 && offset2 + durn2 <= offset1 + durn1;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheChildLayerRecordAsst(KHE_CHILD_LAYER cl, KHE_MEET meet,          */
/*    int offset, int durn)                                                  */
/*                                                                           */
/*  Record in cl's assts the fact that parent meet has been assigned a       */
/*  child meet of this durn with this offset.                                */
/*                                                                           */
/*****************************************************************************/

static void KheChildLayerRecordAsst(KHE_CHILD_LAYER cl, KHE_MEET meet,
  int offset, int durn, HA_ARENA a)
{
  KHE_ASST asst, asst2;  int i;
  HaArrayForEach(cl->assts, asst, i)
    if( KheOverlaps(asst->meet, asst->offset, asst->durn, meet, offset, durn) )
    {
      HnAssert(KheContains(asst->offset, asst->durn, offset, durn),
	"KheChildLayerRecordAsst internal error");
      if( asst->offset == offset )
      {
	asst->offset += durn;
	asst->durn -= durn;
	if( asst->durn == 0 )
	{
	  /* KheAsstDelete(asst); */
	  HaArrayDeleteAndShift(cl->assts, i);
	  i--;
	}
      }
      else if( asst->offset + asst->durn == offset + durn )
	asst->durn -= durn;
      else
      {
	asst->durn = (offset - asst->offset);
        asst2 = KheAsstMake(meet, offset + durn,
	  (asst->offset + asst->durn) - (offset + durn), a);
	HaArrayAddLast(cl->assts, asst2);
      }
    }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheChildLayerSortAssts(KHE_CHILD_LAYER cl)                          */
/*                                                                           */
/*  Sort the assignments of cl into canonical order.                         */
/*                                                                           */
/*****************************************************************************/

static void KheChildLayerSortAssts(KHE_CHILD_LAYER cl)
{
  HaArraySort(cl->assts, &KheAsstCmp);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheChildLayerCompare(KHE_CHILD_LAYER cl1, KHE_CHILD_LAYER cl2)       */
/*                                                                           */
/*  Comparison function for sorting an array of child layers so as to        */
/*  bring together child layers with equal assignments, assuming that        */
/*  the assignments themselves have already been sorted.                     */
/*                                                                           */
/*****************************************************************************/

static int KheChildLayerCompare(KHE_CHILD_LAYER cl1, KHE_CHILD_LAYER cl2)
{
  int i;  KHE_ASST asst1, asst2;
  if( HaArrayCount(cl1->assts) != HaArrayCount(cl2->assts) )
    return HaArrayCount(cl1->assts) - HaArrayCount(cl2->assts);
  for( i = 0;  i < HaArrayCount(cl1->assts);  i++ )
  {
    asst1 = HaArray(cl1->assts, i);
    asst2 = HaArray(cl2->assts, i);
    if( asst1->meet != asst2->meet )
      return KheMeetSolnIndex(asst1->meet) - KheMeetSolnIndex(asst2->meet);
    else if( asst1->offset != asst2->offset )
      return asst1->offset - asst2->offset;
    else if( asst1->durn != asst2->durn )
      return asst1->durn - asst2->durn;
  }
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheChildLayerCmp(const void *t1, const void *t2)                     */
/*                                                                           */
/*  Comparison function for sorting an array of child layers.                */
/*                                                                           */
/*****************************************************************************/

static int KheChildLayerCmp(const void *t1, const void *t2)
{
  KHE_CHILD_LAYER cl1 = * (KHE_CHILD_LAYER *) t1;
  KHE_CHILD_LAYER cl2 = * (KHE_CHILD_LAYER *) t2;
  return KheChildLayerCompare(cl1, cl2);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTargetMeetUsed(KHE_MEET target_meet, KHE_NODE node)              */
/*                                                                           */
/*  Return true if target_meet is already in use as the target of one        */
/*  of the meets of node.                                                    */
/*                                                                           */
/*****************************************************************************/

static bool KheTargetMeetUsed(KHE_MEET target_meet, KHE_NODE node)
{
  int i;  KHE_MEET meet;
  for( i = 0;  i < KheNodeMeetCount(node);  i++ )
  {
    meet = KheNodeMeet(node, i);
    if( KheMeetAsst(meet) == target_meet )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTryAsst(KHE_MEET meet, KHE_MEET target_meet, int target_offset,  */
/*    bool avoid_dups)                                                       */
/*                                                                           */
/*  Try assigning meet to target_meet at target_offset.  If this can be      */
/*  done, and with no increase in soln cost, then leave it in place and      */
/*  return true; otherwise leave it undone and return false.                 */
/*                                                                           */
/*  If avoid_dups is true, only succeed if target_meet is not the            */
/*  target of some other meet of meet's node.                                */
/*                                                                           */
/*****************************************************************************/

static bool KheTryAsst(KHE_MEET meet, KHE_MEET target_meet, int target_offset,
  bool avoid_dups)
{
  KHE_SOLN soln;  KHE_COST init_cost;
  if( !avoid_dups || !KheTargetMeetUsed(target_meet, KheMeetNode(meet)) )
  {
    soln = KheMeetSoln(meet);
    init_cost = KheSolnCost(soln);
    if( KheMeetAssign(meet, target_meet, target_offset) )
    {
      if( KheSolnCost(soln) <= init_cost )
      {
	if( DEBUG4 )
	{
	  fprintf(stderr, "    assigning ");
	  KheMeetDebug(meet, 1, -1, stderr);
	  fprintf(stderr, " to ");
	  KheMeetDebug(target_meet, 1, -1, stderr);
	  fprintf(stderr, " + %d%s\n", target_offset,
	    avoid_dups ? "" : " (dups allowed)");
	}
	return true;
      }
      KheMeetUnAssign(meet);  /* OK; undoes previous asst */
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheChildLayerAssign(KHE_CHILD_LAYER cl, int durn, int shift)        */
/*                                                                           */
/*  Assign the unassigned meets of layer cl of duration durn, starting not   */
/*  at the first available spot but rather at the shift'th.  Return true if  */
/*  all these meets were successfully assigned.                              */
/*                                                                           */
/*****************************************************************************/
static void KheChildLayerDebug(KHE_CHILD_LAYER cl, int verbosity,
  int indent, FILE *fp);

static bool KheChildLayerAssign(KHE_CHILD_LAYER cl, int durn, int shift,
  HA_ARENA a)
{
  int i, j, offset;  KHE_CHILD_NODE cn;  KHE_CHILD_MEET cm;  KHE_MEET meet;
  if( DEBUG4 )
  {
    fprintf(stderr, "[ KheChildLayerAssign(cl, durn %d, shift %d)\n",
      durn, shift);
    KheChildLayerDebug(cl, 1, 2, stderr);
    KheChildLayerIteratorDebug(cl, durn, false, shift, 2, 2, stderr);
  }
  HaArrayForEach(cl->child_nodes, cn, i)
    HaArrayForEach(cn->child_meets, cm, j)
      if( KheMeetAsst(cm->meet) == NULL && KheMeetDuration(cm->meet) == durn )
      {
	/* assign cm->meet */
        ChildLayerForEachAsst(cl, durn, false, shift, &meet, &offset)
	  if( KheTryAsst(cm->meet, meet, offset, true) )
	  {
            KheChildNodeRecordAsst(cn, meet, offset, durn, a);
	    break;
	  }
	if( KheMeetAsst(cm->meet) == NULL )
	{
	  /* failed, so try again more forgivingly */
	  ChildLayerForEachAsst(cl, durn, true, shift, &meet, &offset)
	    if( KheTryAsst(cm->meet, meet, offset, false) )
	    {
	      KheChildNodeRecordAsst(cn, meet, offset, durn, a);
	      break;
	    }
	  if( KheMeetAsst(cm->meet) == NULL )
	  {
	    if( DEBUG4 )
	    {
	      fprintf(stderr, "] KheChildLayerAssign returning false at ");
	      KheMeetDebug(cm->meet, 1, 0, stderr);
	    }
	    return false;
	  }
	}
      }
  if( DEBUG4 )
    fprintf(stderr, "] KheChildLayerAssign returning true\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheChildLayerDebug(KHE_CHILD_LAYER cl, int verbosity,               */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of cl with the given verbosity and indent.                   */
/*                                                                           */
/*****************************************************************************/

static void KheChildLayerDebug(KHE_CHILD_LAYER cl, int verbosity,
  int indent, FILE *fp)
{
  KHE_CHILD_NODE cn;  int i;  KHE_ASST asst;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ Child Layer for ", indent, "");
    KheLayerDebug(cl->layer, 2, 0, fp);
    HaArrayForEach(cl->child_nodes, cn, i)
      KheChildNodeDebug(cn, verbosity, indent + 2, fp);
    if( HaArrayCount(cl->assts) > 0 )
    {
      fprintf(fp, "%*s  %d assts: ", indent, "", HaArrayCount(cl->assts));
      HaArrayForEach(cl->assts, asst, i)
      {
	if( i > 0 )
	  fprintf(fp, ", ");
	KheMeetDebug(asst->meet, 1, -1, fp);
	fprintf(fp, "+%dd%d", asst->offset, asst->durn);
      }
      fprintf(fp, "\n");
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "runaround solvers"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RUNAROUND_SOLVER KheRunaroundSolverMake(KHE_NODE parent_node)        */
/*                                                                           */
/*  Make a runaround solver with these attributes.                           */
/*                                                                           */
/*****************************************************************************/

static KHE_RUNAROUND_SOLVER KheRunaroundSolverMake(KHE_NODE parent_node,
  HA_ARENA a)
{
  KHE_RUNAROUND_SOLVER res;  int i, j, durn;  KHE_CHILD_NODE cn;  KHE_NODE node;
  KHE_LAYER layer;  KHE_CHILD_LAYER cl;  KHE_MEET meet;  /* HA_ARENA a; */

  /* build the basic object, including the child layers */
  /* ***
  a = HaArena Make();
  *** */
  HaMake(res, a);
  res->arena = a;
  res->parent_node = parent_node;
  HaArrayInit(res->child_layers, a);
  for( i = 0;  i < KheNodeChildLayerCount(parent_node);  i++ )
    HaArrayAddLast(res->child_layers,
      KheChildLayerMake(res, KheNodeChildLayer(parent_node, i), a));
  HaArrayInit(res->child_nodes, a);
  res->max_unassigned_durn = 0;

  /* add child nodes */
  for( i = 0;  i < KheNodeChildCount(parent_node);  i++ )
  {
    node = KheNodeChild(parent_node, i);
    for( j = 0;  j < KheNodeMeetCount(node);  j++ )
    {
      meet = KheNodeMeet(node, j);
      durn = KheMeetDuration(meet);
      if( KheMeetAsst(meet) == NULL && durn > res->max_unassigned_durn )
        res->max_unassigned_durn = durn;
    }
    cn = KheChildNodeMake(res, node, a);
    HaArrayAddLast(res->child_nodes, cn);
    for( j = 0;  j < KheNodeParentLayerCount(node);  j++ )
    {
      layer = KheNodeParentLayer(node, j);
      cl = HaArray(res->child_layers, KheLayerParentNodeIndex(layer));
      KheChildLayerAddChildNode(cl, cn);
    }
  }

  /* sort each child layer's nodes so that nodes with more meets come first */
  HaArrayForEach(res->child_layers, cl, i)
    KheChildLayerSortChildNodes(cl);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRunaroundSolverDelete(KHE_RUNAROUND_SOLVER rs)                   */
/*                                                                           */
/*  Delete rs.                                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheRunaroundSolverDelete(KHE_RUNAROUND_SOLVER rs)
{
  HaAren aDelete(rs->arena);
  ** ***
  while( HaArrayCount(rs->child_layers) > 0 )
    KheChildLayerDelete(MArrayRemoveLast(rs->child_layers));
  MArrayFree(rs->child_layers);
  while( HaArrayCount(rs->child_nodes) > 0 )
    KheChildNodeDelete(MArrayRemoveLast(rs->child_nodes));
  MArrayFree(rs->child_nodes);
  MFree(rs);
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheRunaroundSolverSortChildLayers(KHE_RUNAROUND_SOLVER rs)          */
/*                                                                           */
/*  Sort the child layers of rs to bring those with equal assignments        */
/*  together.                                                                */
/*                                                                           */
/*****************************************************************************/

static void KheRunaroundSolverSortChildLayers(KHE_RUNAROUND_SOLVER rs)
{
  KHE_CHILD_LAYER cl;  int i;

  /* sort the assignments in each child layer */
  HaArrayForEach(rs->child_layers, cl, i)
    KheChildLayerSortAssts(cl);

  /* sort the child layers */
  HaArraySort(rs->child_layers, &KheChildLayerCmp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRunaroundSolverMakeExistingAssignments(KHE_RUNAROUND_SOLVER rs)  */
/*                                                                           */
/*  Update the assignments to reflect assignments existing from the start.   */
/*                                                                           */
/*****************************************************************************/

static void KheRunaroundSolverMakeExistingAssignments(KHE_RUNAROUND_SOLVER rs)
{
  int i, j;  KHE_CHILD_NODE cn;  KHE_CHILD_MEET cm;
  HaArrayForEach(rs->child_nodes, cn, i)
    HaArrayForEach(cn->child_meets, cm, j)
      if( KheMeetAsst(cm->meet) != NULL )
        KheChildNodeRecordAsst(cn, KheMeetAsst(cm->meet),
	  KheMeetAsstOffset(cm->meet), KheMeetDuration(cm->meet), rs->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRunaroundSolverSolve(KHE_RUNAROUND_SOLVER rs)                    */
/*                                                                           */
/*  Solve rs, returning true if all meets are assigned successfully.         */
/*                                                                           */
/*****************************************************************************/

static bool KheRunaroundSolverSolve(KHE_RUNAROUND_SOLVER rs)
{
  int durn, i;  KHE_CHILD_LAYER cl;
  if( DEBUG2 )
    fprintf(stderr, "[ KheRunaroundSolverSolve(rs)\n");

  /* debug iterators */
  if( DEBUG3 && HaArrayCount(rs->child_layers) > 0 )
  {
    cl = HaArrayFirst(rs->child_layers);
    for( durn = rs->max_unassigned_durn;  durn >= 1;  durn-- )
      for( i = 0;  i <= 4;  i++ )
	KheChildLayerIteratorDebug(cl, durn, true, i, 2, 2, stderr);
  }

  /* do the solving */
  for( durn = rs->max_unassigned_durn;  durn >= 1;  durn-- )
  {
    KheRunaroundSolverSortChildLayers(rs);
    if( DEBUG2 )
    {
      fprintf(stderr, "  KheRunaroundSolverSolve beginning durn %d:\n", durn);
      KheNodePrintTimetable(rs->parent_node, 15, 2, stderr);
    }
    HaArrayForEach(rs->child_layers, cl, i)
      if( !KheChildLayerAssign(cl, durn, i, rs->arena) )
      {
	if( DEBUG2 )
	  fprintf(stderr, "] KheRunaroundSolverSolve ret. false (%d)\n", i);
	return false;
      }
  }
  if( DEBUG2 )
  {
    KheNodePrintTimetable(rs->parent_node, 15, 2, stderr);
    fprintf(stderr, "] KheRunaroundSolverSolve returning true\n");
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRunaroundSolverDebug(KHE_RUNAROUND_SOLVER rs,                    */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of runaround solver rs onto fp with the given verbosity      */
/*  and indent.                                                              */
/*                                                                           */
/*****************************************************************************/

static void KheRunaroundSolverDebug(KHE_RUNAROUND_SOLVER rs,
  int verbosity, int indent, FILE *fp)
{
  KHE_CHILD_NODE cn;  KHE_CHILD_LAYER cl;  int i;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ Runaround Solver for ", indent, "");
    KheNodeDebug(rs->parent_node, 1, 0, fp);
    fprintf(fp, "%*s  max_unassigned_durn %d\n", indent, "",
      rs->max_unassigned_durn);
    HaArrayForEach(rs->child_nodes, cn, i)
      KheChildNodeDebug(cn, verbosity, indent + 2, fp);
    HaArrayForEach(rs->child_layers, cl, i)
      KheChildLayerDebug(cl, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "solving"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheRunaroundNodeAssignTimes(KHE_NODE parent_node,                   */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Assign times to runaround node parent_node.                              */
/*                                                                           */
/*****************************************************************************/

bool KheRunaroundNodeAssignTimes(KHE_NODE parent_node, KHE_OPTIONS options)
{
  KHE_LAYER first_layer, layer;  int similar_count, i;
  KHE_RUNAROUND_SOLVER rs;  bool success;  KHE_MARK mark;
  KHE_SOLN soln;  HA_ARENA a;

  /* return immediately if there are no child nodes, since nothing to do */
  if( KheNodeChildCount(parent_node) == 0 )
    return true;

  /* build the child layers and bring out their similarities */
  KheNodeChildLayersMake(parent_node);
  HnAssert(KheNodeChildLayerCount(parent_node) > 0,
    "KheRunaroundNodeAssignTimes internal error");
  first_layer = KheNodeChildLayer(parent_node, 0);
  for( i = 1;  i < KheNodeChildLayerCount(parent_node);  i++ )
  {
    layer = KheNodeChildLayer(parent_node, i);
    KheLayerSimilar(first_layer, layer, &similar_count);
  }

  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheRunaroundNodeAssignTimes(");
    KheNodeDebug(parent_node, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }

  /* build a runaround solver for this problem */
  soln = KheNodeSoln(parent_node);
  a = KheSolnArenaBegin(soln);
  rs = KheRunaroundSolverMake(parent_node, a);
  if( DEBUG1 )
    KheRunaroundSolverDebug(rs, 2, 2, stderr);

  /* record existing assignments, then solve */
  mark = KheMarkBegin(KheNodeSoln(parent_node));
  KheRunaroundSolverMakeExistingAssignments(rs);
  success = KheRunaroundSolverSolve(rs);
  KheMarkEnd(mark, !success);

  /* finished with runaround solver and layers now */
  KheSolnArenaEnd(soln, a);
  /* KheRunaroundSolverDelete(rs); */
  KheNodeChildLayersDelete(parent_node);

  /* take the last resort, if necessary */
  if( !success )
  {
    /* NB time_ejecting_not_basic must be false, because ejecting moves */
    /* can unassign times, which is not wanted here                     */
    bool save_vizier_node = KheOptionsGetBool(options, "es_vizier_node", false);
    bool save_no_node_regularity =
      KheOptionsGetBool(options, "ts_no_node_regularity", false);
    bool save_no_ejecting_moves =
      KheOptionsGetBool(options, "es_no_ejecting_moves", false);
    KheOptionsSetBool(options, "es_vizier_node", false);
    KheOptionsSetBool(options, "ts_no_node_regularity", true);
    KheOptionsSetBool(options, "es_no_ejecting_moves", false);
    success = KheNodeLayeredAssignTimes(parent_node, options);
    KheOptionsSetBool(options, "es_vizier_node", save_vizier_node);
    KheOptionsSetBool(options, "ts_no_node_regularity",save_no_node_regularity);
    KheOptionsSetBool(options, "es_no_ejecting_moves", save_no_ejecting_moves);
  }

  /* take the last resort, if necessary ***
  if( !success )
  {
    ** NB time_ejecting_not_basic must be false, because ejecting moves **
    ** can unassign times, which is not wanted here                     **
    bool save_vizier_node = KheOptionsEjectorVizierNode(options);
    bool save_node_regularity = KheOptionsTimeNodeRegularity(options);
    bool save_ejecting_not_basic = KheOptionsEjectorEjectingNotBasic(options);
    ** KHE_NODE save_node_limit = KheOptionsEjectorNode Limit(options); **
    KheOptionsSetEjectorVizierNode(options, false);
    KheOptionsSetTimeNodeRegularity(options, false);
    KheOptionsSetEjectorEjectingNotBasic(options, false);
    ** KheOptionsSetEjectorNode Limit(options, parent_node); **
    success = KheNodeLayeredAssignTimes(parent_node, options);
    KheOptionsSetEjectorVizierNode(options, save_vizier_node);
    KheOptionsSetTimeNodeRegularity(options, save_node_regularity);
    KheOptionsSetEjectorEjectingNotBasic(options, save_ejecting_not_basic);
    ** KheOptionsSetEjectorNode Limit(options, save_node_limit); **
  }
  *** */
  if( DEBUG1 )
    fprintf(stderr, "] KheRunaroundNodeAssignTimes returning %s\n",
      success ? "true" : "false");
  return success;
}


/* old version, uses transactions
bool KheRunaroundNodeAssignTimes(KHE_NODE parent_node, KHE_OPTIONS options)
{
  KHE_LAYER first_layer, layer;  int similar_count, i;
  KHE_RUNAROUND_SOLVER rs;  bool res;  KHE_TRANSACTION t;

  ** return immediately if there are no child nodes, since nothing to do **
  if( KheNodeChildCount(parent_node) == 0 )
    return true;

  ** build the child layers and bring out their similarities **
  KheNodeChildLayersMake(parent_node);
  HnAssert(KheNodeChildLayerCount(parent_node) > 0,
    "KheRunaroundNodeAssignTimes internal error");
  first_layer = KheNodeChildLayer(parent_node, 0);
  for( i = 1;  i < KheNodeChildLayerCount(parent_node);  i++ )
  {
    layer = KheNodeChildLayer(parent_node, i);
    KheLayerSimilar(first_layer, layer, &similar_count);
  }

  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheRunaroundNodeAssignTimes(");
    KheNodeDebug(parent_node, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }

  ** build a runaround solver for this problem **
  rs = KheRunaroundSolverMake(parent_node);
  if( DEBUG1 )
    KheRunaroundSolverDebug(rs, 2, 2, stderr);

  ** record existing assignments, then solve **
  t = KheTransactionMake(KheNodeSoln(parent_node));
  KheTransactionBegin(t);
  KheRunaroundSolverMakeExistingAssignments(rs);
  res = KheRunaroundSolverSolve(rs);
  KheTransactionEnd(t);

  ** finished with runaround solver and layers now **
  KheRunaroundSolverDelete(rs);
  KheNodeChildLayersDelete(parent_node);

  ** take the last resort, if necessary **
  if( !res )
  {
    ** NB time_ejecting_not_basic must be false, because ejecting moves **
    ** can unassign times, which is not wanted here                     **
    bool save_vizier_node = KheOptionsEjectorVizierNode(options);
    bool save_node_regularity = KheOptionsTimeNodeRegularity(options);
    bool save_ejecting_not_basic = KheOptionsEjectorEjectingNotBasic(options);
    ** KHE_NODE save_node_limit = KheOptionsEjectorNode Limit(options); **
    KheTransactionUndo(t);
    KheOptionsSetEjectorVizierNode(options, false);
    KheOptionsSetTimeNodeRegularity(options, false);
    KheOptionsSetEjectorEjectingNotBasic(options, false);
    ** KheOptionsSetEjectorNode Limit(options, parent_node); **
    res = KheNodeLayeredAssignTimes(parent_node, options);
    KheOptionsSetEjectorVizierNode(options, save_vizier_node);
    KheOptionsSetTimeNodeRegularity(options, save_node_regularity);
    KheOptionsSetEjectorEjectingNotBasic(options, save_ejecting_not_basic);
    ** KheOptionsSetEjectorNode Limit(options, save_node_limit); **
  }
  if( DEBUG1 )
    fprintf(stderr, "] KheRunaroundNodeAssignTimes returning %s\n",
      res ? "true" : "false");
  KheTransactionDelete(t);
  return res;
}
*** */

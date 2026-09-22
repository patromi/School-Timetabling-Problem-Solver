
/*****************************************************************************/
/*                                                                           */
/*  THE KTS TIMETABLING SYSTEM                                               */
/*  COPYRIGHT (C) 2004, 2008 Jeffrey H. Kingston                             */
/*                                                                           */
/*  Jeffrey H. Kingston (jeff@it.usyd.edu.au)                                */
/*  School of Information Technologies                                       */
/*  The University of Sydney 2006                                            */
/*  AUSTRALIA                                                                */
/*                                                                           */
/*  FILE:         khe_mmatch.c                                               */
/*  MODULE:       Weighted bipartite matching with multi-nodes               */
/*                                                                           */
/*  This implementation of weighted bipartite matching uses min-cost         */
/*  flow, with shortest paths found by Dijksta's algorithm, using edge       */
/*  cost adjustment.                                                         */
/*                                                                           */
/*  Unlike khe_wmatch.c, which offers many features, this code is very       */
/*  basic, except that it offers multi-nodes (nodes with multiplicity)       */
/*  and 96-bit edge costs, in the form of (int, int, int) triples.           */
/*                                                                           */
/*****************************************************************************/
#include "khe_mmatch.h"
#include "howard_a.h"
#include "howard_n.h"
#include <limits.h>
#include <string.h>
#include <sys/time.h>

#define DEBUG1 0
#define DEBUG4 0
#define DEBUG_EDGE_CHECK 1


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_COST - a cost, in the form of an (int, int, int) triple       */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_mmatch_cost_rec {
  int			cost1;
  int			cost2;
  int			cost3;
} KHE_MMATCH_COST;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_EDGE - one edge in the flow network (private)                 */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_mmatch_edge_rec *KHE_MMATCH_EDGE;

struct khe_mmatch_edge_rec {
  KHE_MMATCH_EDGE	next_edge;		/* next edge out of node     */
  KHE_MMATCH_NODE	endpoint;		/* endpoint of this edge     */
  KHE_MMATCH_COST	cost;			/* cost of this edge         */
  int			capacity;		/* capacity of this edge     */
  int			flow;			/* current flow on this edge */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_EDGE_LIST - a list of edges                                   */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_mmatch_edge_list_rec {
  KHE_MMATCH_EDGE	first_edge;
  KHE_MMATCH_EDGE	last_edge;
} KHE_MMATCH_EDGE_LIST;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_EDGE_PAIR - a pair of opposite edges, allocated together      */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_mmatch_edge_pair_rec {
  struct khe_mmatch_edge_rec	forward_edge_rec;	/* goes forwards     */
  struct khe_mmatch_edge_rec	backward_edge_rec;	/* goes backwards    */
} *KHE_MMATCH_EDGE_PAIR;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_NODE - one node in the flow network.                          */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  NODE_SOURCE,
  NODE_SINK,
  NODE_SUPPLY,
  NODE_DEMAND
} NODE_TYPE;

struct khe_mmatch_node_rec {
  KHE_MMATCH_NODE	next_node;		/* next node in node list    */
  KHE_MMATCH		mmatch;			/* enclosing mmatch          */
  void			*back;			/* original of this node     */
  NODE_TYPE		type;			/* source/sink/supply/demand */
  int			heap_index;		/* back index into heap      */
  int			visit_num;		/* visited flag (int form)   */
  KHE_MMATCH_COST	curr_adjusted_distance;	/* adjusted cost to here     */
  KHE_MMATCH_COST	prev_true_distance;	/* true cost on previous run */
  int			node_flow;		/* flow through this node    */
  KHE_MMATCH_EDGE	parent_edge;		/* incoming edge from parent */
  KHE_MMATCH_EDGE_LIST	edge_list;		/* list of outgoing edges    */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_NODE_LIST - a list of nodes                                   */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_mmatch_node_list_rec {
  KHE_MMATCH_NODE	first_node;
  KHE_MMATCH_NODE	last_node;
} KHE_MMATCH_NODE_LIST;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH - the network flow graph as a whole                           */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_MMATCH_NODE) HEAP;

struct khe_mmatch_rec {
  HA_ARENA		arena;			/* holds the lot             */
  void			*back;			/* original		     */
  MMATCH_DEBUG_FN	mmatch_back_debug;	/* debug mmatch original     */
  MMATCH_DEBUG_FN	demand_back_debug;	/* debug demand original     */
  MMATCH_DEBUG_FN	supply_back_debug;	/* debug supply original     */
  MMATCH_COST_DEBUG_FN	cost_debug;		/* cost debug function       */
  KHE_MMATCH_COST	curr_cost;		/* current cost              */
  int			visit_num;		/* visited flag (int form)   */
  KHE_MMATCH_NODE_LIST	demand_node_list;	/* demand nodes              */
  KHE_MMATCH_NODE_LIST	supply_node_list;	/* supply nodes              */
  int			demand_total_capacity;	/* capacity through demands  */
  int			total_flow;		/* total flow if up to date  */
  HEAP			heap;			/* heap for shortest paths   */
  KHE_MMATCH_NODE_LIST	free_node_list;		/* free list of nodes        */
  KHE_MMATCH_EDGE_LIST	free_edge_list;		/* free list of edges        */
  KHE_MMATCH_NODE	source;			/* source node               */
  KHE_MMATCH_NODE	sink;			/* sink node                 */
  KHE_MMATCH_NODE	it_node;		/* iterator to return result */
  KHE_MMATCH_EDGE	it_edge;		/* iterator to return result */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "costs" (private)                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KheMMatchCostParts(mc)                                                   */
/*                                                                           */
/*  A macro which returns the three parts of mc, separated by commas.        */
/*                                                                           */
/*****************************************************************************/

#define KheMMatchCostParts(mc) (mc).cost1, (mc).cost2, (mc).cost3


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_COST KheMMatchCost(int cost1, int cost2, int cost3)           */
/*                                                                           */
/*  Return a match cost with these attributes.                               */
/*                                                                           */
/*****************************************************************************/

static KHE_MMATCH_COST KheMMatchCost(int cost1, int cost2, int cost3)
{
  KHE_MMATCH_COST res;
  res.cost1 = cost1;
  res.cost2 = cost2;
  res.cost3 = cost3;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_COST KheMMatchCostZero(void)                                  */
/*                                                                           */
/*  Return a zero match cost.                                                */
/*                                                                           */
/*****************************************************************************/

static KHE_MMATCH_COST KheMMatchCostZero(void)
{
  return KheMMatchCost(0, 0, 0);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_COST KheMMatchCostAdd(KHE_MMATCH_COST mc1,KHE_MMATCH_COST mc2)*/
/*                                                                           */
/*  Return mc1 + mc2.                                                        */
/*                                                                           */
/*****************************************************************************/

static KHE_MMATCH_COST KheMMatchCostAdd(KHE_MMATCH_COST mc1,
  KHE_MMATCH_COST mc2)
{
  KHE_MMATCH_COST res;
  res.cost1 = mc1.cost1 + mc2.cost1;
  res.cost2 = mc1.cost2 + mc2.cost2;
  res.cost3 = mc1.cost3 + mc2.cost3;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_COST KheMMatchCostSub(KHE_MMATCH_COST mc1,KHE_MMATCH_COST mc2)*/
/*                                                                           */
/*  Return mc1 - mc2.                                                        */
/*                                                                           */
/*****************************************************************************/

static KHE_MMATCH_COST KheMMatchCostSub(KHE_MMATCH_COST mc1,
  KHE_MMATCH_COST mc2)
{
  KHE_MMATCH_COST res;
  res.cost1 = mc1.cost1 - mc2.cost1;
  res.cost2 = mc1.cost2 - mc2.cost2;
  res.cost3 = mc1.cost3 - mc2.cost3;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_COST KheMMatchCostMultiply(KHE_MMATCH_COST mc, int factor)    */
/*                                                                           */
/*  Return mc * factor.                                                      */
/*                                                                           */
/*****************************************************************************/

KHE_MMATCH_COST KheMMatchCostMultiply(KHE_MMATCH_COST mc, int factor)
{
  KHE_MMATCH_COST res;
  res.cost1 = mc.cost1 * factor;
  res.cost2 = mc.cost2 * factor;
  res.cost3 = mc.cost3 * factor;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_COST KheMMatchCostNegative(KHE_MMATCH_COST mc)                */
/*                                                                           */
/*  Return - mc.                                                             */
/*                                                                           */
/*****************************************************************************/

KHE_MMATCH_COST KheMMatchCostNegative(KHE_MMATCH_COST mc)
{
  KHE_MMATCH_COST res;
  res.cost1 = - mc.cost1;
  res.cost2 = - mc.cost2;
  res.cost3 = - mc.cost3;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMMatchCostIsZero(KHE_MMATCH_COST mc)                             */
/*                                                                           */
/*  Return true if mc is zero.                                               */
/*                                                                           */
/*****************************************************************************/

static bool KheMMatchCostIsZero(KHE_MMATCH_COST mc)
{
  return mc.cost1 == 0 && mc.cost2 == 0 && mc.cost3 == 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMMatchCostEqual(KHE_MMATCH_COST mc1, KHE_MMATCH_COST mc2)        */
/*                                                                           */
/*  Return true if mc1 and mc2 are equal.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheMMatchCostEqual(KHE_MMATCH_COST mc1, KHE_MMATCH_COST mc2)
{
  return mc1.cost1 == mc2.cost1 && mc1.cost2 == mc2.cost2 &&
    mc1.cost3 == mc2.cost3;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMMatchCostLT(KHE_MMATCH_COST mc1, KHE_MMATCH_COST mc2)           */
/*                                                                           */
/*  Return true if mc1 < mc2.                                                */
/*                                                                           */
/*****************************************************************************/

static bool KheMMatchCostLT(KHE_MMATCH_COST mc1, KHE_MMATCH_COST mc2)
{
  return mc1.cost1 < mc2.cost1 || (mc1.cost1 == mc2.cost1 &&
        (mc1.cost2 < mc2.cost2 || (mc1.cost2 == mc2.cost2 &&
         mc1.cost3 < mc2.cost3)));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMMatchCostLE(KHE_MMATCH_COST mc1, KHE_MMATCH_COST mc2)           */
/*                                                                           */
/*  Return true if mc1 <= mc2.                                               */
/*                                                                           */
/*****************************************************************************/

static bool KheMMatchCostLE(KHE_MMATCH_COST mc1, KHE_MMATCH_COST mc2)
{
  return mc1.cost1 < mc2.cost1 || (mc1.cost1 == mc2.cost1 &&
        (mc1.cost2 < mc2.cost2 || (mc1.cost2 == mc2.cost2 &&
         mc1.cost3 <= mc2.cost3)));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "edge lists" (private)                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_EDGE_LIST EdgeListEmpty(void)                                 */
/*                                                                           */
/*  Return an empty edge list.                                               */
/*                                                                           */
/*****************************************************************************/

static KHE_MMATCH_EDGE_LIST EdgeListEmpty(void)
{
  KHE_MMATCH_EDGE_LIST res;
  res.first_edge = res.last_edge = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void EdgeListAddEdge(KHE_MMATCH_EDGE_LIST *edge_list,                    */
/*    KHE_MMATCH_EDGE edge)                                                  */
/*                                                                           */
/*  Add edge to the end of *edge_list.                                       */
/*                                                                           */
/*****************************************************************************/

static void EdgeListAddEdge(KHE_MMATCH_EDGE_LIST *edge_list,
  KHE_MMATCH_EDGE edge)
{
  if( edge_list->first_edge == NULL )
    edge_list->first_edge = edge;
  else
    edge_list->last_edge->next_edge = edge;
  edge_list->last_edge = edge;
  edge->next_edge = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void EdgeListAppend(KHE_MMATCH_EDGE_LIST *dest_edge_list,                */
/*    KHE_MMATCH_EDGE_LIST src_edge_list)                                    */
/*                                                                           */
/*  Append src_edge_list to the end of dest_edge_list.                       */
/*                                                                           */
/*****************************************************************************/

static void EdgeListAppend(KHE_MMATCH_EDGE_LIST *dest_edge_list,
  KHE_MMATCH_EDGE_LIST *src_edge_list)
{
  if( src_edge_list->first_edge != NULL )
  {
    if( dest_edge_list->first_edge == NULL )
      dest_edge_list->first_edge = src_edge_list->first_edge;
    else
      dest_edge_list->last_edge->next_edge = src_edge_list->first_edge;
    dest_edge_list->last_edge = src_edge_list->last_edge;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_EDGE EdgeListDeleteFirst(KHE_MMATCH_EDGE_LIST *edge_list)     */
/*                                                                           */
/*  Delete and return the first edge of edge_list, or return NULL if none.   */
/*                                                                           */
/*****************************************************************************/

static KHE_MMATCH_EDGE EdgeListDeleteFirst(KHE_MMATCH_EDGE_LIST *edge_list)
{
  KHE_MMATCH_EDGE res;
  if( edge_list->first_edge == NULL )
    return NULL;
  res = edge_list->first_edge;
  edge_list->first_edge = res->next_edge;
  if( edge_list->first_edge == NULL )
    edge_list->last_edge = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  EdgeListForEach(KHE_MMATCH_EDGE elist, EDGE e)                           */
/*                                                                           */
/*  Iterator over all edges e of elist.                                      */
/*                                                                           */
/*****************************************************************************/

#define EdgeListForEach(elist, e)					\
for( (e) = (elist).first_edge;  (e) != NULL;  (e) = (e)->next_edge )


/*****************************************************************************/
/*                                                                           */
/*  void EdgeListCheckAllForward(KHE_MMATCH_EDGE_LIST *edge_list)            */
/*                                                                           */
/*  Check that every edge on *edge_list is forward.                          */
/*                                                                           */
/*****************************************************************************/
static bool EdgeIsForward(KHE_MMATCH_EDGE e);

static void EdgeListCheckAllForward(KHE_MMATCH_EDGE_LIST *edge_list)
{
  KHE_MMATCH_EDGE edge;
  EdgeListForEach(*edge_list, edge)
    HnAssert(EdgeIsForward(edge), "EdgeListCheckAllForward internal error");
}


/*****************************************************************************/
/*                                                                           */
/*  void EdgeListFree(KHE_MMATCH_EDGE edge_list)                             */
/*                                                                           */
/*  Free an entire list of edges.  These are supposed to be forward.        */
/*                                                                           */
/*****************************************************************************/

static void EdgeListFree(KHE_MMATCH m, KHE_MMATCH_EDGE_LIST *edge_list)
{
  if( DEBUG_EDGE_CHECK )
    EdgeListCheckAllForward(edge_list);
  EdgeListAppend(&m->free_edge_list, edge_list);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "edges" (private)                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void EdgeGetPair(KHE_MMATCH m, KHE_MMATCH_EDGE *forward_edge,            */
/*    KHE_MMATCH_EDGE *backward_edge)                                        */
/*                                                                           */
/*  Get a new pair of edges, either from m's free list or from its arena.    */
/*                                                                           */
/*****************************************************************************/

static void EdgeGetPair(KHE_MMATCH m, KHE_MMATCH_EDGE *forward_edge,
  KHE_MMATCH_EDGE *backward_edge)
{
  KHE_MMATCH_EDGE pedge;   KHE_MMATCH_EDGE_PAIR edge_pair;
  pedge = EdgeListDeleteFirst(&m->free_edge_list);
  if( pedge == NULL )
    HaMake(edge_pair, m->arena);
  else
    edge_pair = (KHE_MMATCH_EDGE_PAIR) pedge;
  *forward_edge = &edge_pair->forward_edge_rec;
  *backward_edge = &edge_pair->backward_edge_rec;
}


/*****************************************************************************/
/*                                                                           */
/*  void InsertEdge(KHE_MMATCH_NODE from, KHE_MMATCH_NODE to,                */
/*    int multiplicity, KHE_MMATCH_COST cost)                                */
/*                                                                           */
/*  Insert an edge from dn to sn of capacity multiplicity and cost as        */
/*  given.  Also add its opposite edge.                                      */
/*                                                                           */
/*****************************************************************************/

static void InsertEdge(KHE_MMATCH_NODE from, KHE_MMATCH_NODE to,
  int multiplicity, KHE_MMATCH_COST cost)
{
  KHE_MMATCH_EDGE forward_edge, backward_edge;
  HnAssert(multiplicity > 0, "InsertEdge: multiplicity <= 0");
  HnAssert(multiplicity < INT_MAX, "InsertEdge: multiplicity == INT_MAX");

  /* get two edge records, forward_edge and backward_edge */
  EdgeGetPair(from->mmatch, &forward_edge, &backward_edge);

  /* initialize forward_edge to go from from to to */
  EdgeListAddEdge(&from->edge_list, forward_edge);
  forward_edge->endpoint = to;
  forward_edge->capacity = multiplicity;
  forward_edge->cost = cost;
  forward_edge->flow = 0;

  /* initialize backward_edge to go from to to from */
  EdgeListAddEdge(&to->edge_list, backward_edge);
  backward_edge->endpoint = from;
  backward_edge->capacity = - multiplicity;
  backward_edge->cost = KheMMatchCostNegative(cost);
  backward_edge->flow = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMMatchAddEdge(KHE_MMATCH_NODE dn, KHE_MMATCH_NODE sn,            */
/*    int multiplicity, int c1, int c2, int c3)                              */
/*                                                                           */
/*  Add an edge with these attributes from demand node dn to supply node sn. */
/*                                                                           */
/*****************************************************************************/

void KheMMatchAddEdge(KHE_MMATCH_NODE dn, KHE_MMATCH_NODE sn,
  int multiplicity, int c1, int c2, int c3)
{
  KHE_MMATCH_COST cost;
  HnAssert(dn->type == NODE_DEMAND,"KheMMatchAddEdge: dn is not a demand node");
  HnAssert(sn->type == NODE_SUPPLY,"KheMMatchAddEdge: sn is not a supply node");
  cost = KheMMatchCost(c1, c2, c3);
  HnAssert(KheMMatchCostLE(KheMMatchCostZero(), cost),
    "KheMMatchAddEdge: negative cost");
  InsertEdge(dn, sn, multiplicity, cost);
}


/*****************************************************************************/
/*                                                                           */
/*  bool EdgeIsForward(KHE_MMATCH_EDGE e)                                    */
/*                                                                           */
/*  Return true if e is a forward edge.                                      */
/*                                                                           */
/*****************************************************************************/

static bool EdgeIsForward(KHE_MMATCH_EDGE e)
{
  return e->capacity > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_EDGE EdgeOpposite(KHE_MMATCH_EDGE edge)                       */
/*                                                                           */
/*  Return the opposite edge to edge.  This is stored either just after      */
/*  it (if edge is forward) or just before it (if edge is backward).        */
/*                                                                           */
/*****************************************************************************/

static KHE_MMATCH_EDGE EdgeOpposite(KHE_MMATCH_EDGE edge)
{
  KHE_MMATCH_EDGE_PAIR pair;
  if( EdgeIsForward(edge) )
  {
    pair = (KHE_MMATCH_EDGE_PAIR) edge;
    return &pair->backward_edge_rec;
  }
  else
  {
    pair = (KHE_MMATCH_EDGE_PAIR)
      ((char *) edge - sizeof(struct khe_mmatch_edge_rec));
    return &pair->forward_edge_rec;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_NODE EdgeBeginPoint(KHE_MMATCH_EDGE edge)                     */
/*                                                                           */
/*  Return the begin point of edge (the node it starts from).  This is       */
/*  found by finding the endpoint of its opposite edge.                      */
/*                                                                           */
/*****************************************************************************/

static KHE_MMATCH_NODE EdgeBeginPoint(KHE_MMATCH_EDGE edge)
{
  return EdgeOpposite(edge)->endpoint;
}


/*****************************************************************************/
/*                                                                           */
/*  void EdgeFree(KHE_MMATCH m, KHE_MMATCH_EDGE edge)                        */
/*                                                                           */
/*  Free edge.  It is supposed to be forward.                                */
/*                                                                           */
/*****************************************************************************/

static void EdgeFree(KHE_MMATCH m, KHE_MMATCH_EDGE edge)
{
  if( DEBUG_EDGE_CHECK )
    HnAssert(EdgeIsForward(edge), "EdgeFree internal error");
  EdgeListAddEdge(&m->free_edge_list, edge);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "node lists" (private)                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_NODE_LIST NodeListEmpty(void)                                 */
/*                                                                           */
/*  Return an empty node list.                                               */
/*                                                                           */
/*****************************************************************************/

static KHE_MMATCH_NODE_LIST NodeListEmpty(void)
{
  KHE_MMATCH_NODE_LIST res;
  res.first_node = res.last_node = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NodeListAddNode(KHE_MMATCH_NODE_LIST *node_list,                    */
/*    KHE_MMATCH_NODE node)                                                  */
/*                                                                           */
/*  Add node to the end of *node_list.                                       */
/*                                                                           */
/*****************************************************************************/

static void NodeListAddNode(KHE_MMATCH_NODE_LIST *node_list,
  KHE_MMATCH_NODE node)
{
  if( node_list->first_node == NULL )
    node_list->first_node = node;
  else
    node_list->last_node->next_node = node;
  node_list->last_node = node;
  node->next_node = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void NodeListAppend(KHE_MMATCH_NODE_LIST *dest_node_list,                */
/*    KHE_MMATCH_NODE_LIST src_node_list)                                    */
/*                                                                           */
/*  Append src_node_list to the end of dest_node_list.                       */
/*                                                                           */
/*****************************************************************************/

static void NodeListAppend(KHE_MMATCH_NODE_LIST *dest_node_list,
  KHE_MMATCH_NODE_LIST *src_node_list)
{
  if( src_node_list->first_node != NULL )
  {
    if( dest_node_list->first_node == NULL )
      dest_node_list->first_node = src_node_list->first_node;
    else
      dest_node_list->last_node->next_node = src_node_list->first_node;
    dest_node_list->last_node = src_node_list->last_node;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_NODE NodeListDeleteFirst(KHE_MMATCH_NODE_LIST *node_list)     */
/*                                                                           */
/*  Delete and return the first node of node_list, or NULL if empty.         */
/*                                                                           */
/*****************************************************************************/

static KHE_MMATCH_NODE NodeListDeleteFirst(KHE_MMATCH_NODE_LIST *node_list)
{
  KHE_MMATCH_NODE res;
  if( node_list->first_node == NULL )
    return NULL;
  res = node_list->first_node;
  node_list->first_node = res->next_node;
  if( node_list->first_node == NULL )
    node_list->last_node = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NodeListForEach(KHE_MMATCH_NODE list, NODE x)                            */
/*                                                                           */
/*  Iterator over all nodes x of list.                                       */
/*                                                                           */
/*****************************************************************************/

#define NodeListForEach(list, x)					\
for( (x) = (list).first_node;  (x) != NULL;  (x) = (x)->next_node )


/*****************************************************************************/
/*                                                                           */
/*  void NodeListFree(KHE_MMATCH m, KHE_MMATCH_NODE_LIST *node_list)         */
/*                                                                           */
/*  Free an entire list of nodes.                                            */
/*                                                                           */
/*****************************************************************************/

static void NodeListFree(KHE_MMATCH m, KHE_MMATCH_NODE_LIST *node_list)
{
  NodeListAppend(&m->free_node_list, node_list);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "nodes" (private)                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_NODE NodeMake(KHE_MMATCH m, void *back, NODE_TYPE type)       */
/*                                                                           */
/*  Get a node, either from m's free list or from malloc, and initialize it. */
/*                                                                           */
/*****************************************************************************/

static KHE_MMATCH_NODE NodeMake(KHE_MMATCH m, void *back, NODE_TYPE type)
{
  KHE_MMATCH_NODE res;

  /* get the node */
  res = NodeListDeleteFirst(&m->free_node_list);
  if( res == NULL )
    HaMake(res, m->arena);

  /* initialize it */
  res->next_node = NULL;
  res->mmatch = m;
  res->back = back;
  res->type = type;
  res->heap_index = 0;
  res->visit_num = 0;
  res->curr_adjusted_distance = KheMMatchCostZero();
  res->prev_true_distance = KheMMatchCostZero();
  res->node_flow = 0;
  res->parent_edge = NULL;
  res->edge_list = EdgeListEmpty();;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NodeFree(KHE_MMATCH_NODE n)                                         */
/*                                                                           */
/*  Free n.                                                                  */
/*                                                                           */
/*****************************************************************************/

static void NodeFree(KHE_MMATCH_NODE n)
{
  KHE_MMATCH m = n->mmatch;
  NodeListAddNode(&m->free_node_list, n);
}


/*****************************************************************************/
/*                                                                           */
/*  void NodeDebug(KHE_MMATCH_NODE v, FILE *fp)                              */
/*                                                                           */
/*  Debug print of v onto fp.   It knows what sort of node it is.            */
/*                                                                           */
/*****************************************************************************/

static void NodeDebug(KHE_MMATCH_NODE v, FILE *fp)
{
  switch( v->type )
  {
    case NODE_SOURCE:

      fprintf(fp, "Source");
      break;

    case NODE_SINK:

      fprintf(fp, "Sink");
      break;

    case NODE_SUPPLY:

      v->mmatch->supply_back_debug(v->back, 1, -1, fp);
      break;

    case NODE_DEMAND:

      v->mmatch->demand_back_debug(v->back, 1, -1, fp);
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "demand nodes"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_NODE KheMMatchDemandNodeMake(KHE_MMATCH m,                    */
/*    int multiplicity, void *back)                                          */
/*                                                                           */
/*  Add and return one demand node.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_MMATCH_NODE KheMMatchDemandNodeMake(KHE_MMATCH m,
  int multiplicity, void *back)
{
  KHE_MMATCH_NODE res;
  res = NodeMake(m, back, NODE_DEMAND);
  NodeListAddNode(&m->demand_node_list, res);
  InsertEdge(m->source, res, multiplicity, KheMMatchCostZero());
  m->demand_total_capacity += multiplicity;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH KheMMatchDemandNodeMMatch(KHE_MMATCH_NODE dn)                 */
/*                                                                           */
/*  Return the KHE_MMATCH containing dn.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_MMATCH KheMMatchDemandNodeMMatch(KHE_MMATCH_NODE dn)
{
  return dn->mmatch;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMMatchDemandNodeMultiplicity(KHE_MMATCH_NODE dn)                  */
/*                                                                           */
/*  Return the multiplicity attribute of dn.                                 */
/*                                                                           */
/*****************************************************************************/

/* *** not easily implementable, so leaving out until proved necessary
int KheMMatchDemandNodeMultiplicity(KHE_MMATCH_NODE dn)
{
  HnAbort("KheMMatchDemandNodeMultiplicity sti ll to do");
  return 0;  ** keep compiler happy **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void *KheMMatchDemandNodeBack(KHE_MMATCH_NODE dn)                        */
/*                                                                           */
/*  Return the back pointer attribute of dn.                                 */
/*                                                                           */
/*****************************************************************************/

void *KheMMatchDemandNodeBack(KHE_MMATCH_NODE dn)
{
  return dn->back;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "supply nodes"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_NODE KheMMatchSupplyNodeMake(KHE_MMATCH m,                    */
/*    int multiplicity, void *back)                                          */
/*                                                                           */
/*  Add and return one supply node.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_MMATCH_NODE KheMMatchSupplyNodeMake(KHE_MMATCH m,
  int multiplicity, void *back)
{
  KHE_MMATCH_NODE res;
  res = NodeMake(m, back, NODE_SUPPLY);
  InsertEdge(res, m->sink, multiplicity, KheMMatchCostZero());
  NodeListAddNode(&m->supply_node_list, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH KheMMatchSupplyNodeMMatch(KHE_MMATCH_NODE sn)                 */
/*                                                                           */
/*  Return the KHE_MMATCH containing sn.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_MMATCH KheMMatchSupplyNodeMMatch(KHE_MMATCH_NODE sn)
{
  return sn->mmatch;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMMatchSupplyNodeMultiplicity(KHE_MMATCH_NODE sn)                  */
/*                                                                           */
/*  Return the multiplicity attribute of sn.                                 */
/*                                                                           */
/*****************************************************************************/

/* *** this one is easy to implement, but omit it since demand one is omitted
int KheMMatchSupplyNodeMultiplicity(KHE_MMATCH_NODE sn)
{
  HnAbort("KheMMatchSupplyNodeMultiplicity s till to do");
  return 0;  ** keep compiler happy **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void *KheMMatchSupplyNodeBack(KHE_MMATCH_NODE sn)                        */
/*                                                                           */
/*  Return the back pointer of sn.                                           */
/*                                                                           */
/*****************************************************************************/

void *KheMMatchSupplyNodeBack(KHE_MMATCH_NODE sn)
{
  return sn->back;
}


/*****************************************************************************/
/*                                                                           */
/*  void EdgeDebug(KHE_MMATCH_EDGE e, FILE *fp)                              */
/*                                                                           */
/*  Debug print of e onto fp.                                                */
/*                                                                           */
/*****************************************************************************/
static void DefaultCostDebugFn(int c1, int c2, int c3, FILE *fp);

static void EdgeDebug(KHE_MMATCH_EDGE e, FILE *fp)
{
  KHE_MMATCH m = e->endpoint->mmatch;
  fprintf(fp, "--c%df%d(", e->capacity, e->flow);
  m->cost_debug(KheMMatchCostParts(e->cost), fp);
  fprintf(fp, ")->");
}


/*****************************************************************************/
/*                                                                           */
/*  void EdgeFullDebug(KHE_MMATCH_EDGE e, FILE *fp)                          */
/*                                                                           */
/*  Debug print of e, including its endpoints.                               */
/*                                                                           */
/*****************************************************************************/

static void EdgeFullDebug(KHE_MMATCH_EDGE e, FILE *fp)
{
  NodeDebug(EdgeBeginPoint(e), fp);
  fprintf(fp, " ");
  EdgeDebug(e, fp);
  fprintf(fp, " ");
  NodeDebug(e->endpoint, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Floyd-Williams heap" (private)                                */
/*                                                                           */
/*****************************************************************************/
#define HEAP_ENTRY KHE_MMATCH_NODE
#define key(x) ((x)->curr_adjusted_distance)
#define key_lt(x, y) KheMMatchCostLT(key(x), key(y))
#define key_le(x, y) KheMMatchCostLE(key(x), key(y))
#define set_heap(heap, i, x) HaArrayPut(heap, i, x), (x)->heap_index = (i)

static void add_leaf(HEAP *heap, int pos)
{
  int i, j;  HEAP_ENTRY x;
  x = HaArray(*heap, pos);
  i = pos;
  j = i / 2;
  while( j > 0 && key_lt(x, HaArray(*heap, j)) )
  {
    set_heap(*heap, i, HaArray(*heap, j));
    i = j;
    j = i / 2;
  }
  set_heap(*heap, i, x);
}

static void add_root(HEAP *heap, int i)
{
  int j;  HEAP_ENTRY x;
  x = HaArray(*heap, i);
  for( ;; )
  {
    j = 2 * i;
    if( j >= HaArrayCount(*heap) )
      break;
    if( j < HaArrayCount(*heap)-1 &&
	key_lt(HaArray(*heap, j+1), HaArray(*heap, j)) )
      j++;
    if( key_le(x, HaArray(*heap, j)) )
      break;
    set_heap(*heap, i, HaArray(*heap, j));
    i = j;
  }
  set_heap(*heap, i, x);
}


/*****************************************************************************/
/*                                                                           */
/*  void HeapClear(HEAP *heap)                                               */
/*                                                                           */
/*  Reset existing heap to empty.  Since heaps are indexed from 1 but our    */
/*  arrays are indexed from 0, we need to add a dummy null value to begin.   */
/*                                                                           */
/*****************************************************************************/

static void HeapClear(HEAP *heap)
{
  HaArrayClear(*heap);
  HaArrayAddLast(*heap, NULL);
}


/*****************************************************************************/
/*                                                                           */
/*  bool HeapIsEmpty(HEAP *heap)                                             */
/*                                                                           */
/*  Return true if heap is empty (except for the dummy element at position   */
/*  0, which isn't really an element at all).                                */
/*                                                                           */
/*****************************************************************************/

static bool HeapIsEmpty(HEAP *heap)
{
  return HaArrayCount(*heap) == 1;
}


/*****************************************************************************/
/*                                                                           */
/*  void HeapInsert(HEAP *heap, HEAP_ENTRY x)                                */
/*                                                                           */
/*  Add a new node x to the heap.                                            */
/*                                                                           */
/*****************************************************************************/

static void HeapInsert(HEAP *heap, HEAP_ENTRY x)
{
  HaArrayAddLast(*heap, x);
  add_leaf(heap, HaArrayCount(*heap) - 1);
}


/*****************************************************************************/
/*                                                                           */
/*  HEAP_ENTRY HeapFindMin(HEAP *heap)                                       */
/*                                                                           */
/*  Return the minimum entry of heap;  it must be non-empty.                 */
/*                                                                           */
/*****************************************************************************/

/* *** correct, but not used
static HEAP_ENTRY HeapFindMin(HEAP *heap)
{
  HnAssert(HaArrayCount(*heap) >= 2, "");
  return HaArray(*heap, 1);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  HEAP_ENTRY HeapDeleteMin(HEAP *heap)                                     */
/*                                                                           */
/*  Find and delete a minimum entry of heap.                                 */
/*                                                                           */
/*****************************************************************************/

static HEAP_ENTRY HeapDeleteMin(HEAP *heap)
{
  HEAP_ENTRY res, x;
  HnAssert(HaArrayCount(*heap) >= 2, "HeapDeleteMin internal error");
  res = HaArray(*heap, 1);
  x = HaArrayLastAndDelete(*heap);
  if( HaArrayCount(*heap) >= 2 )
  {
    HaArrayPut(*heap, 1, x);
    add_root(heap, 1);
  }
  res->heap_index = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void HeapDecreaseKey(HEAP *heap, HEAP_ENTRY x)                           */
/*                                                                           */
/*  The key of x has just been decreased by the application.  Move x         */
/*  through heap to its new position.                                        */
/*                                                                           */
/*****************************************************************************/

static void HeapDecreaseKey(HEAP *heap, HEAP_ENTRY x)
{
  add_leaf(heap, x->heap_index);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "solver" (private)                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int EdgeAvailableExtraFlow(KHE_MMATCH_EDGE e)                            */
/*                                                                           */
/*  Return the amount of extra flow available along e.                       */
/*                                                                           */
/*****************************************************************************/

static int EdgeAvailableExtraFlow(KHE_MMATCH_EDGE e)
{
  return (EdgeIsForward(e) ? e->capacity - e->flow : - e->flow);
}


/*****************************************************************************/
/*                                                                           */
/*  void SetFlowToZero(KHE_MMATCH m)                                         */
/*                                                                           */
/*  Set the flow to zero.                                                    */
/*                                                                           */
/*****************************************************************************/

static void SetFlowToZeroAtNode(KHE_MMATCH_NODE v)
{
  KHE_MMATCH_EDGE e;
  v->node_flow = 0;
  EdgeListForEach(v->edge_list, e)
    e->flow = 0;
}

static void SetFlowToZero(KHE_MMATCH m)
{
  KHE_MMATCH_NODE v;
  NodeListForEach(m->demand_node_list, v)
    SetFlowToZeroAtNode(v);
  NodeListForEach(m->supply_node_list, v)
    SetFlowToZeroAtNode(v);
  SetFlowToZeroAtNode(m->source);
  SetFlowToZeroAtNode(m->sink);
}


/*****************************************************************************/
/*                                                                           */
/*  void InitPrevTrueDistances(KHE_MMATCH m)                                 */
/*                                                                           */
/*  Set the curr_distance fields in all nodes to 0.  These values will be    */
/*  copied into the prev_distance fields as soon as the nodes are touched;   */
/*  from there, they are used to adjust edge costs.                          */
/*                                                                           */
/*****************************************************************************/

static void InitPrevTrueDistances(KHE_MMATCH m)
{
  KHE_MMATCH_NODE v;
  NodeListForEach(m->demand_node_list, v)
    v->prev_true_distance = KheMMatchCostZero();
  NodeListForEach(m->supply_node_list, v)
    v->prev_true_distance = KheMMatchCostZero();
  m->source->prev_true_distance = KheMMatchCostZero();
  m->sink->prev_true_distance = KheMMatchCostZero();
}


/*****************************************************************************/
/*                                                                           */
/*  void UpdatePrevTrueDistances(KHE_MMATCH m)                               */
/*                                                                           */
/*  Update the prev_true_distance fields to reflect the true distances       */
/*  at the end of a Dijkstra run.                                            */
/*                                                                           */
/*****************************************************************************/

static void UpdatePrevTrueDistances(KHE_MMATCH m)
{
  KHE_MMATCH_NODE v;
  NodeListForEach(m->demand_node_list, v)
    v->prev_true_distance =
      KheMMatchCostAdd(v->prev_true_distance, v->curr_adjusted_distance);
  NodeListForEach(m->supply_node_list, v)
    v->prev_true_distance =
      KheMMatchCostAdd(v->prev_true_distance, v->curr_adjusted_distance);
  m->source->prev_true_distance = KheMMatchCostAdd(
    m->source->prev_true_distance, m->source->curr_adjusted_distance);
  m->sink->prev_true_distance = KheMMatchCostAdd(
    m->sink->prev_true_distance, m->sink->curr_adjusted_distance);
}


/*****************************************************************************/
/*                                                                           */
/*  void CheckNodeDistances(KHE_MMATCH_NODE v, KHE_MMATCH m)                 */
/*                                                                           */
/*  Carry out the specification of CheckDistances at one node v, which must  */
/*  be visited.  This function is called only when debugging.                */
/*                                                                           */
/*****************************************************************************/

static void CheckNodeDistances(KHE_MMATCH_NODE v, KHE_MMATCH m)
{
  KHE_MMATCH_EDGE e;  KHE_MMATCH_NODE w;

  /* only check visited vertices */
  if( v->visit_num < m->visit_num )
    return;

  /* make sure the true distance of v can be justified */
  if( v == m->source )
  {
    if( v->parent_edge != NULL )
    {
      fprintf(stderr, "CheckNodeDistances: source has parent_edge\n");
      abort();
    }
    /* if( v->prev_true_distance != 0 ) */
    if( !KheMMatchCostIsZero(v->prev_true_distance) )
    {
      fprintf(stderr, "CheckNodeDistances: source has prev_true_distance ");
      m->cost_debug(KheMMatchCostParts(v->prev_true_distance), stderr);
      fprintf(stderr, "\n");
      abort();
    }
  }
  else
  {
    if( v->parent_edge == NULL )
    {
      fprintf(stderr, "CheckNodeDistances: visited node ");
      NodeDebug(v, stderr);
      fprintf(stderr, " has no parent\n");
      abort();
    }
    e = v->parent_edge;
    w = EdgeBeginPoint(e);
    /* if( w->prev_true_distance + e->cost != v->prev_true_distance ) */
    if( !KheMMatchCostEqual(KheMMatchCostAdd(w->prev_true_distance, e->cost),
          v->prev_true_distance) )
    {
      fprintf(stderr, "CheckNodeDistances: inconsistent parent edge ");
      NodeDebug(w, stderr);
      fprintf(stderr, " (");
      DefaultCostDebugFn(KheMMatchCostParts(w->prev_true_distance), stderr);
      fprintf(stderr, ")");
      EdgeDebug(e, stderr);
      fprintf(stderr, " ");
      NodeDebug(v, stderr);
      fprintf(stderr, " (");
      DefaultCostDebugFn(KheMMatchCostParts(v->prev_true_distance), stderr);
      fprintf(stderr, ")\n");
      HnAbort("CheckNodeDistances found inconsistency");
    }
  }

  /* make sure all v's outgoing edges are consistent */
  EdgeListForEach(v->edge_list, e)
    if( EdgeAvailableExtraFlow(e) > 0 )
    {
      w = e->endpoint;
      if( w->visit_num < m->visit_num )
	continue;
      /* if( w->prev_true_distance > v->prev_true_distance + e->cost ) */
      if( KheMMatchCostLT(KheMMatchCostAdd(v->prev_true_distance, e->cost),
	    w->prev_true_distance) )
      {
	fprintf(stderr, "CheckNodeDistances: inconsistent edge ");
	EdgeFullDebug(e, stderr);
	fprintf(stderr, "\n");
	abort();
      }
    }
}


/*****************************************************************************/
/*                                                                           */
/*  void CheckDistances(KHE_MMATCH m)                                        */
/*                                                                           */
/*  This function is called only when debugging.  It checks that the         */
/*  distances stored in each vertex do represent a shortest path             */
/*  spanning tree.  It must be called after UpdatePrevTrueDistances,         */
/*  since it relies on the prev_true_distance fields being set properly.     */
/*                                                                           */
/*****************************************************************************/

static void CheckDistances(KHE_MMATCH m)
{
  KHE_MMATCH_NODE v;
  NodeListForEach(m->demand_node_list, v)
    CheckNodeDistances(v, m);
  NodeListForEach(m->supply_node_list, v)
    CheckNodeDistances(v, m);
  CheckNodeDistances(m->source, m);
  CheckNodeDistances(m->sink, m);
}


/*****************************************************************************/
/*                                                                           */
/*  void UpdateFlowThroughNode(KHE_MMATCH_NODE v, int extra_flow,            */
/*    KHE_MMATCH_EDGE incoming_edge, KHE_MMATCH_EDGE outgoing_edge)          */
/*                                                                           */
/*  Here v is a vertex on an augmenting path from the source to the sink,    */
/*  not equal to the source or sink; extra_flow is an amount of extra flow   */
/*  that is being pushed along that path, incoming_edge is the edge entering */
/*  v on the path, and outgoing_edge is the edge leaving v.  Update the      */
/*  node_flow field of v to record the change in flow through v.             */
/*                                                                           */
/*  If both the incoming edge and the outgoing edge are forward edges, then  */
/*  the augment represents an increase in the flow through v.  If both       */
/*  edges are backward edges, it represents a decrease in flow through v.    */
/*  Otherwise there is no change in the flow through v.                      */
/*                                                                           */
/*****************************************************************************/

static void UpdateFlowThroughNode(KHE_MMATCH_NODE v, int extra_flow,
  KHE_MMATCH_EDGE incoming_edge, KHE_MMATCH_EDGE outgoing_edge)
{
  if( EdgeIsForward(incoming_edge) )
  {
    if( EdgeIsForward(outgoing_edge) )
      v->node_flow += extra_flow;
  }
  else
  {
    if( !EdgeIsForward(outgoing_edge) )
      v->node_flow -= extra_flow;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AugmentPath(KHE_MMATCH m, int *extra_flow)                          */
/*                                                                           */
/*  Augment the path from source to sink found by a previous successful      */
/*  call to any function which has set a chain of parent_edge fields from    */
/*  the sink back to the source.  Also set *extra_flow to the amount of      */
/*  extra flow introduced by this augment.                                   */
/*                                                                           */
/*****************************************************************************/
#define prev_edge_on_path(e) (EdgeBeginPoint(e)->parent_edge)

static void AugmentPath(KHE_MMATCH m, int *extra_flow)
{
  KHE_MMATCH_EDGE e, prev_e;

  /* do a first traversal to work out the amount of augmenting flow */
  e = m->sink->parent_edge;
  *extra_flow = EdgeAvailableExtraFlow(e);
  e = prev_edge_on_path(e);
  while( e != NULL )
  {
    if( EdgeAvailableExtraFlow(e) < *extra_flow )
      *extra_flow = EdgeAvailableExtraFlow(e);
    e = prev_edge_on_path(e);
  }

  /* do a second traversal to insert this flow */
  m->sink->node_flow += *extra_flow;
  prev_e = NULL;
  e = m->sink->parent_edge;
  while( e != NULL )
  {
    if( prev_e != NULL )
      UpdateFlowThroughNode(e->endpoint, *extra_flow, e, prev_e);
    e->flow += *extra_flow;
    EdgeOpposite(e)->flow = - e->flow;
    prev_e = e;
    e = prev_edge_on_path(e);
  }
  m->source->node_flow += *extra_flow;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH_COST EdgeAdjustedCost(KHE_MMATCH_EDGE e, KHE_MMATCH m)        */
/*                                                                           */
/*  Return the adjusted cost of e.  Check that it is non-negative.           */
/*                                                                           */
/*****************************************************************************/

static KHE_MMATCH_COST EdgeAdjustedCost(KHE_MMATCH_EDGE e, KHE_MMATCH m)
{
  KHE_MMATCH_COST res;
  res = KheMMatchCostSub(
    KheMMatchCostAdd(e->cost, EdgeBeginPoint(e)->prev_true_distance),
    e->endpoint->prev_true_distance);
  HnAssert(KheMMatchCostLE(KheMMatchCostZero(), res),
    "EdgeAdjustedCost: found negative cost %d,%d,%d", KheMMatchCostParts(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool FindMinCostAugmentingPath(KHE_MMATCH m)                             */
/*                                                                           */
/*  If an augmenting path exists in f, find one of minimum cost and store    */
/*  it in the flow's parent_edge fields.                                     */
/*                                                                           */
/*  Implementation note.  This function is an implementation of Dijkstra's   */
/*  shortest path algorithm, with edge cost adjustment to avoid negative     */
/*  cost edges.  The current adjusted distance from the source vertex to     */
/*  any visited vertex v is stored in v->curr_adjusted_distance.             */
/*                                                                           */
/*****************************************************************************/

static bool FindMinCostAugmentingPath(KHE_MMATCH m)
{
  KHE_MMATCH_NODE v, w;  KHE_MMATCH_EDGE e;  KHE_MMATCH_COST path_cost;
  m->visit_num++;
  HeapClear(&m->heap);
  m->source->visit_num = m->visit_num;
  m->source->prev_true_distance = KheMMatchCostZero();
  m->source->curr_adjusted_distance = KheMMatchCostZero();
  m->source->parent_edge = NULL;
  HeapInsert(&m->heap, m->source);
  while( !HeapIsEmpty(&m->heap) )
  {
    v = HeapDeleteMin(&m->heap);
    EdgeListForEach(v->edge_list, e)
      if( EdgeAvailableExtraFlow(e) > 0 )
      {
	w = e->endpoint;
	path_cost = KheMMatchCostAdd(v->curr_adjusted_distance,
          EdgeAdjustedCost(e, m));
	if( w->visit_num < m->visit_num )
	{
	  w->visit_num = m->visit_num;
	  w->curr_adjusted_distance = path_cost;
	  w->parent_edge = e;
	  HeapInsert(&m->heap, w);
	}
	else if( w->heap_index != 0 &&
	  KheMMatchCostLT(path_cost, w->curr_adjusted_distance) )
	{
	  w->curr_adjusted_distance = path_cost;
	  w->parent_edge = e;
	  HeapDecreaseKey(&m->heap, w);
	}
      }
  }
  return m->sink->visit_num == m->visit_num;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMMatchSolve(KHE_MMATCH m)                                        */
/*                                                                           */
/*  Run a min-cost max-flow algorithm.  Return true if every node was        */
/*  matched; but a false value is not a problem, it just means that some     */
/*  calls to KheMMatchResultEdge will return false with *sn == NULL.         */
/*                                                                           */
/*****************************************************************************/

bool KheMMatchSolve(KHE_MMATCH m)
{
  int extra_flow;
  InitPrevTrueDistances(m);
  SetFlowToZero(m);
  m->total_flow = 0;
  m->curr_cost = KheMMatchCostZero();
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheMMatchSolve, initial state is:\n");
    KheMMatchDebug(m, 3, 2, stderr);
  }
  while( FindMinCostAugmentingPath(m) )
  {
    UpdatePrevTrueDistances(m);
    if( DEBUG4 )
      CheckDistances(m);
    AugmentPath(m, &extra_flow);
    m->total_flow += extra_flow;
    m->curr_cost = KheMMatchCostAdd(m->curr_cost,
      KheMMatchCostMultiply(m->sink->prev_true_distance, extra_flow));
    if( DEBUG1 )
    {
      fprintf(stderr, "  after augment, state is:\n");
      KheMMatchDebug(m, 3, 2, stderr);
    }
  }
  m->it_node = NULL;
  m->it_edge = NULL;
  if( DEBUG1 )
    fprintf(stderr, "] KheMMatchSolve returning %s\n",
      m->total_flow == m->demand_total_capacity ? "true" : "false");
  return m->total_flow == m->demand_total_capacity;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMMatchResultEdge(KHE_DEMAND_NODE dn, KHE_SUPPLY_NODE *sn,        */
/*    int *multiplicity, int *c1, int *c2, int *c3)                          */
/*                                                                           */
/*  Visit the supply nodes connected to dn by edges of non-zero flow.        */
/*                                                                           */
/*****************************************************************************/

bool KheMMatchResultEdge(KHE_MMATCH_NODE dn, KHE_MMATCH_NODE *sn,
  int *multiplicity, int *c1, int *c2, int *c3)
{
  KHE_MMATCH m;

  /* set m->it_edge to the next edge with flow, or NULL if none */
  m = dn->mmatch;
  if( m->it_node != dn )
  {
    /* starting a traverse, we want the first edge with positive flow */
    HnAssert(dn->type == NODE_DEMAND,
      "KheMMatchResultEdge: dn is not a demand node");
    m->it_node = dn;
    m->it_edge = dn->edge_list.first_edge;
    while( m->it_edge != NULL && m->it_edge->flow <= 0 )
      m->it_edge = m->it_edge->next_edge;
  }
  else
  {
    /* continuing a traverse, we want the next edge with positive flow */
    do
      m->it_edge = m->it_edge->next_edge;
    while( m->it_edge != NULL && m->it_edge->flow <= 0 );
  }

  /* return appropriate values, depending on whether we have an edge or not */
  if( m->it_edge == NULL )
  {
    /* finished this traverse */
    m->it_node = NULL;
    *sn = NULL;
    *c1 = *c2 = *c3 = -1;
    *multiplicity = -1;
    return false;
  }
  else
  {
    /* found another edge */
    *sn = m->it_edge->endpoint;
    *c1 = m->it_edge->cost.cost1;
    *c2 = m->it_edge->cost.cost2;
    *c3 = m->it_edge->cost.cost3;
    *multiplicity = m->it_edge->flow;
    HnAssert((*sn)->type == NODE_SUPPLY, "KheMMatchResultEdge internal error");
    return true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheMMatchDemandNodeAssignedTo(KHE_MMATCH_NODE demand_node)         */
/*                                                                           */
/*  Find any node that receives a non-zero amount of flow directly from      */
/*  demand_node and return its original; or return NULL if no such node.     */
/*                                                                           */
/*  Alternatively, if demand_node is fixed to some supply node, return       */
/*  that node.                                                               */
/*                                                                           */
/*****************************************************************************/

/* *** not doing it this way now
void *KheMMatchDemandNodeAssignedTo(KHE_MMATCH_NODE demand_node, int64_t *cost)
{
  HnAbort("still to do");
  return NULL;
  ** *** still to do here
  KHE_MMATCH_EDGE edge;
  KheMMatchMakeClean(demand_node->mmatch);
  EdgeListForEach(demand_node->edge_list, edge)
    if( edge->flow > 0 )
    {
      if( DEBUG9 )
      {
	fprintf(stderr, "KheMMatchDemandNodeAssignedTo(");
	NodeDebug(demand_node, stderr);
	fprintf(stderr, ") returning ");
	NodeDebug(edge->endpoint, stderr);
	fprintf(stderr, ":\n");
      }
      *cost = edge->cost;
      return edge->endpoint->back;
    }
  if( DEBUG9 )
  {
    fprintf(stderr, "KheMMatchDemandNodeAssignedTo(");
    NodeDebug(demand_node, stderr);
    fprintf(stderr, ") returning NULL:\n");
    KheMMatchDebug(demand_node->mmatch, 2, 2, stderr);
  }
  return NULL;
  *** **
}
*** */



/*****************************************************************************/
/*                                                                           */
/*  Submodule "matchings".                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void DefaultDebugFn(void *back, int verbosity, int indent, FILE *fp)     */
/*                                                                           */
/*  Use this function when the user has not supplied a debug function.       */
/*                                                                           */
/*****************************************************************************/

static void DefaultDebugFn(void *back, int verbosity, int indent, FILE *fp)
{
  fprintf(fp, "-");
}


/*****************************************************************************/
/*                                                                           */
/*  void DefaultCostDebugFn(int c1, int c2, int c3, FILE *fp)                */
/*                                                                           */
/*  Use this function when the user has not supplied a cost debug function.  */
/*                                                                           */
/*****************************************************************************/

static void DefaultCostDebugFn(int c1, int c2, int c3, FILE *fp)
{
  fprintf(fp, "%d,%d,%d", c1, c2, c3);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MMATCH KheMMatchMake(void *back, MMATCH_DEBUG_FN back_debug,         */
/*    MMATCH_DEBUG_FN demand_back_debug, MMATCH_DEBUG_FN supply_back_debug,  */
/*    MMATCH_COST_DEBUG_FN cost_debug, HA_ARENA a)                           */
/*                                                                           */
/*  Return a new mmatch with just the source and sink nodes, not connected.  */
/*                                                                           */
/*****************************************************************************/

KHE_MMATCH KheMMatchMake(void *back, MMATCH_DEBUG_FN back_debug,
  MMATCH_DEBUG_FN demand_back_debug, MMATCH_DEBUG_FN supply_back_debug,
  MMATCH_COST_DEBUG_FN cost_debug, HA_ARENA a)
{
  KHE_MMATCH res;
  HaMake(res, a);
  res->arena = a;
  res->back = back;
  res->mmatch_back_debug =
    (back_debug == NULL ? &DefaultDebugFn : back_debug);
  res->demand_back_debug =
    (demand_back_debug == NULL ? &DefaultDebugFn : demand_back_debug);
  res->supply_back_debug =
    (supply_back_debug == NULL ? &DefaultDebugFn : supply_back_debug);
  res->cost_debug = (cost_debug == NULL ? DefaultCostDebugFn : cost_debug);
  res->curr_cost = KheMMatchCostZero();
  res->visit_num = 0;
  res->demand_total_capacity = 0;
  res->demand_node_list = NodeListEmpty();
  res->supply_node_list = NodeListEmpty();
  res->total_flow = 0;
  HaArrayInit(res->heap, a);
  res->free_node_list = NodeListEmpty();
  res->free_edge_list = EdgeListEmpty();
  res->source = NodeMake(res, NULL, NODE_SOURCE);
  res->sink = NodeMake(res, NULL, NODE_SINK);
  res->it_node = NULL;
  res->it_edge = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMMatchClear(KHE_MMATCH m)                                        */
/*                                                                           */
/*  Clear out m, ready for a fresh start.  But save its free lists!          */
/*                                                                           */
/*  Implementation note.  It is important to only free forward edges,        */
/*  because they carry the backward edges along with them.  The comments     */
/*  below indicate where the forward edges are.                              */
/*                                                                           */
/*****************************************************************************/

void KheMMatchClear(KHE_MMATCH m)
{
  KHE_MMATCH_NODE node;  KHE_MMATCH_EDGE edge;

  /* the edges out of the source node are forward edges */
  EdgeListFree(m, &m->source->edge_list);

  /* the edges out of each demand node are forward edges, except for */
  /* the first edge, which is a backward edge back to the source */
  NodeListForEach(m->demand_node_list, node)
  {
    edge = EdgeListDeleteFirst(&node->edge_list);
    HnAssert(edge != NULL && edge->endpoint == m->source,
      "KheMMatchClear internal error 1");
    EdgeListFree(m, &node->edge_list);
  }

  /* the edges out of each supply node are backward edges, except for */
  /* the first edge, which is a forward edge to the source */
  NodeListForEach(m->supply_node_list, node)
  {
    edge = EdgeListDeleteFirst(&node->edge_list);
    HnAssert(edge != NULL && edge->endpoint == m->sink,
      "KheMMatchClear internal error 2");
    EdgeFree(m, edge);
  }

  /* the edges out of the sink are all backward edges */

  /* free nodes */
  NodeFree(m->source);
  NodeListFree(m, &m->demand_node_list);
  NodeListFree(m, &m->supply_node_list);
  NodeFree(m->sink);

  /* reset m's various fields */
  m->curr_cost = KheMMatchCostZero();
  m->visit_num = 0;
  m->demand_total_capacity = 0;
  m->demand_node_list = NodeListEmpty();
  m->supply_node_list = NodeListEmpty();
  m->total_flow = 0;
  HaArrayClear(m->heap);
  m->source = NodeMake(m, NULL, NODE_SOURCE);
  m->sink = NodeMake(m, NULL, NODE_SINK);
  m->it_node = NULL;
  m->it_edge = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debugging and testing".                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMMatchDebug(KHE_MMATCH m, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug printf of m onto fp with the given verbosity and indent.           */
/*                                                                           */
/*    Verbosity 1: show infeasibility and cost                               */
/*    Verbosity 2: add display of matching demand nodes                      */
/*    Verbosity 3: add display of matching edges and supply nodes            */
/*    Verbosity 4: all kinds of extra stuff                                  */
/*                                                                           */
/*****************************************************************************/

void KheMMatchDebug(KHE_MMATCH m, int verbosity, int indent, FILE *fp)
{
  KHE_MMATCH_NODE demand, supply;  KHE_MMATCH_EDGE de, se, xe;
  int source_out_capacity, sink_in_capacity;

  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ KheMMatch ");
    m->mmatch_back_debug(m->back, 1, -1, fp);
    fprintf(fp, " ");
    m->cost_debug(KheMMatchCostParts(m->curr_cost), fp);
    fprintf(fp, ")");
    if( verbosity == 1 || indent < 0 )
    {
      fprintf(fp, " ]");
    }
    else if( verbosity == 2 )
    {
      fprintf(fp, ":\n");
      EdgeListForEach(m->source->edge_list, de)
      {
	demand = de->endpoint;
	EdgeListForEach(demand->edge_list, se)
	  if( se->flow > 0 && KheMMatchCostLT(KheMMatchCostZero(), se->cost) )
	  {
	    fprintf(fp, "%*s  ", indent, "");
	    NodeDebug(demand, fp);
	    fprintf(fp, "\n");
	  }
      }
      fprintf(fp, "%*s]\n", indent, "");
    }
    else if( verbosity == 3 )
    {
      fprintf(fp, ":\n");
      EdgeListForEach(m->source->edge_list, de)
      {
	demand = de->endpoint;
	fprintf(fp, "%*s  ", indent, "");
	NodeDebug(demand, fp);
	EdgeListForEach(demand->edge_list, se)
	{
	  supply = se->endpoint;
	  if( se->flow > 0 )
	  {
	    fprintf(fp, " -");
	    m->cost_debug(KheMMatchCostParts(se->cost), fp);
	    fprintf(fp, "-> ");
	    NodeDebug(supply, fp);
	  }
	}
	fprintf(fp, "\n");
      }
      fprintf(fp, "%*s]\n", indent, "");
    }
    else /* verbosity >= 4 */
    {
      /* work out total capacity out of source */
      source_out_capacity = 0;
      EdgeListForEach(m->source->edge_list, de)
      {
	HnAssert(de->capacity >= 0, "KheMMatchDebug internal error 1");
	source_out_capacity += de->capacity;
      }

      /* work out total capacity into sink */
      sink_in_capacity = 0;
      EdgeListForEach(m->sink->edge_list, se)
      {
	HnAssert(se->capacity <= 0, "KheMMatchDebug internal error 2");
	sink_in_capacity -= se->capacity;
      }

      /* header */
      fprintf(fp, ":\n");
      fprintf(fp, "%*s  ", indent, "");
      NodeDebug(m->source, fp);
      fprintf(fp, "\n");
      EdgeListForEach(m->source->edge_list, de)
      {
	demand = de->endpoint;
	fprintf(fp, "%*s    ", indent, "");
	EdgeDebug(de, fp);
	fprintf(fp, " ");
	NodeDebug(demand, fp);
	fprintf(fp, "\n");
	EdgeListForEach(demand->edge_list, se)
	{
	  supply = se->endpoint;
	  if( supply != m->source )
	  {
	    fprintf(fp, "%*s      ", indent, "");
	    EdgeDebug(se, fp);
	    fprintf(fp, " ");
	    NodeDebug(supply, fp);
            EdgeListForEach(supply->edge_list, xe)
	      if( xe->endpoint == m->sink )
	      {
		fprintf(fp, " ");
		EdgeDebug(xe, fp);
		fprintf(fp, " ");
		NodeDebug(xe->endpoint, fp);
	      }
	    fprintf(fp, "\n");
	  }
	}
      }
      fprintf(fp, "%*s]\n", indent, "");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMMatchTest(FILE *fp)                                             */
/*                                                                           */
/*  Test this module, reporting results to fp.                               */
/*                                                                           */
/*****************************************************************************/

typedef struct test_node {
  char *		name;		/* node name                         */
  KHE_MMATCH_NODE	node;		/* the node                          */
} *TEST_NODE;

static TEST_NODE MakeTestDemandNode(KHE_MMATCH m, char *name)
{
  TEST_NODE res;
  HaMake(res, m->arena);
  res->name = name;
  res->node = KheMMatchDemandNodeMake(m, 1, (void *) res);
  return res;
}

static TEST_NODE MakeTestSupplyNode(KHE_MMATCH m, char *name)
{
  TEST_NODE res;
  HaMake(res, m->arena);
  res->name = name;
  res->node = KheMMatchSupplyNodeMake(m, 1, (void *) res);
  return res;
}

static void node_debug(void *node, int verbosity, int indent, FILE *fp)
{
  fprintf(stderr, "%s", ((TEST_NODE) node)->name);
}

/* *** currently not used
static bool edge_fn(void *demand, void *supply, int64_t *cost)
{
  ** silly function which declares need for an edge when the two names **
  ** contain a character in common, and uses 0-1-2-0-1-2 etc. for cost **
  int i, j;
  static int64_t silly_cost = 0;
  TEST_NODE tdemand = (TEST_NODE) demand;
  TEST_NODE tsupply = (TEST_NODE) supply;
  for( i = 0;  tdemand->name[i] != '\0';  i++ )
    for( j = 0;  tsupply->name[j] != '\0';  j++ )
      if( tdemand->name[i] == tsupply->name[j] )
      {
	*cost = (silly_cost++) % 3;
	return true;
      }
  return false;
}
*** */

static void KheMMatchTest1(FILE *fp)
{
  HA_ARENA a;  KHE_MMATCH m;  HA_ARENA_SET as;
  as = HaArenaSetMake();
  a = HaArenaMake(as);  /* this arena is used for testing only */
  m = KheMMatchMake(NULL, NULL, &node_debug, &node_debug, NULL, a);
  /* TEST_NODE ta, tb, tc, td */;
  MakeTestDemandNode(m, "A1234");
  MakeTestDemandNode(m, "B12");

  MakeTestSupplyNode(m, "1");
  MakeTestSupplyNode(m, "2");
  MakeTestSupplyNode(m, "3");
  MakeTestSupplyNode(m, "4");

  KheMMatchSolve(m);
  fprintf(fp, "After solving:\n");
  KheMMatchDebug(m, 2, 0, fp);
  fprintf(fp, "In brief:\n");
  KheMMatchDebug(m, 1, 0, fp);
  HaArenaDelete(a);  /* this arena is used for testing only */
}


void KheMMatchTest2(FILE *fp)
{
  /* nothing just at present */
}


void KheMMatchTest(FILE *fp)
{
  KheMMatchTest1(fp);
  KheMMatchTest2(fp);
}

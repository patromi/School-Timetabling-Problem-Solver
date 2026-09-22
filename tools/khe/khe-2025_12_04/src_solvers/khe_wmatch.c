
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
/*  FILE:         khe_wmatch.c                                               */
/*  MODULE:       Updateable weighted matching                               */
/*                                                                           */
/*  This implementation of weighted bipartite matching uses min-cost         */
/*  flow, with shortest paths found by Dijksta's algorithm, using edge       */
/*  cost adjustment.                                                         */
/*                                                                           */
/*  A node may be in one of four states: clean, fixed, partly dirty, and     */
/*  fully dirty.  The graph itself may be clean or dirty.                    */
/*                                                                           */
/*  A node may be deleted at a time when it is an endpoint of a fixed edge;  */
/*  in that case, the edge will be unfixed before the node is deleted.       */
/*                                                                           */
/*****************************************************************************/
#include "khe_wmatch.h"
#include "howard_a.h"
#include "howard_n.h"
#include <limits.h>
#include <string.h>
#include <sys/time.h>
#define DEBUG1 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG9 0
#define DEBUG10 0
/* #define DEBUG11 0 */
#define min(a, b) ((a) < (b) ? (a) : (b))
#define beginpoint opposite_edge->endpoint
#define	TIDY_HALL_SETS 1

/*****************************************************************************/
/*                                                                           */
/*  time_taken - if DEBUG11, time spent in KheWMatchGraphMakeClean, in musecs*/
/*                                                                           */
/*****************************************************************************/

/* static int64_t time_taken = 0; */


/*****************************************************************************/
/*                                                                           */
/*  HALL_SET (private)                                                       */
/*                                                                           */
/*****************************************************************************/

typedef struct hall_set_rec *HALL_SET;

struct hall_set_rec {
  HALL_SET		parent_set;		/* parent, or NULL if node   */
  HA_ARRAY_VOIDP	demand_originals;	/* originals of demand nodes */
  HA_ARRAY_VOIDP	supply_originals;	/* originals of supply nodes */
};

typedef HA_ARRAY(HALL_SET) ARRAY_HALL_SET;


/*****************************************************************************/
/*                                                                           */
/*  KHE_WMATCH_EDGE - one edge in the flow network (private)                 */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_wmatch_edge_rec *KHE_WMATCH_EDGE;

struct khe_wmatch_edge_rec {
  KHE_WMATCH_EDGE	prev_edge;		/* prev edge out of node     */
  KHE_WMATCH_EDGE	next_edge;		/* next edge out of node     */
  KHE_WMATCH_NODE	endpoint;		/* endpoint of this edge     */
  KHE_WMATCH_EDGE	opposite_edge;		/* edge opposite to this one */
  int64_t		cost;			/* cost of this edge         */
  int			capacity;		/* capacity of this edge     */
  int			flow;			/* current flow on this edge */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_WMATCH_CATEGORY - a set of related nodes                             */
/*                                                                           */
/*****************************************************************************/

struct khe_wmatch_category_rec {
  int			index;			/* index of category         */
  KHE_WMATCH_CATEGORY	next_category;		/* next category in list     */
  KHE_WMATCH		wmatch;			/* the wmatch this is in     */
  KHE_WMATCH_NODE	first_node;		/* first node in category    */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_WMATCH_NODE - one node in the flow network.                          */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  NODE_SOURCE,
  NODE_SINK,
  NODE_SUPPLY,
  NODE_DEMAND
} NODE_TYPE;

typedef enum {
  NODE_CLEAN,
  NODE_DIRTY,
  NODE_FIXED
} NODE_STATE;

struct khe_wmatch_node_rec {
  KHE_WMATCH_NODE	next_node;		/* next node in node list    */
  KHE_WMATCH_NODE	prev_node;		/* prev node in node list    */
  KHE_WMATCH		wmatch;			/* enclosing wmatch          */
  void			*back;			/* original of this node     */
  KHE_WMATCH_CATEGORY	category;		/* category                  */
  int			category_strength;	/* non-zero means worse      */
  KHE_WMATCH_NODE	category_next;		/* category list (circular)  */
  KHE_WMATCH_NODE	category_prev;		/* category list (circular)  */
  NODE_TYPE		type;			/* source/sink/supply/demand */
  NODE_STATE		state;			/* clean/dirty/fixed         */
  int			heap_index;		/* back index into heap      */
  int			visit_num;		/* visited flag (int form)   */
  int64_t		curr_adjusted_distance;	/* adjusted cost to here     */
  int64_t		prev_true_distance;	/* true cost on previous run */
  int			node_flow;		/* flow through this node    */
  KHE_WMATCH_EDGE	parent_edge;		/* incoming edge from parent */
  KHE_WMATCH_EDGE	edge_list;		/* list of outgoing edges    */
  KHE_WMATCH_EDGE	special_edge_list;	/* outgoing special edges    */
  KHE_WMATCH_NODE	fixed_to;		/* when fixed to other node  */
  int64_t		forced_test_weight;	/* used by forced asst test  */
  HALL_SET		hall_set;		/* Hall set, if wanted       */
  KHE_WMATCH_NODE	equiv_next;		/* equivalent list (circular)*/
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_WMATCH - the network flow graph as a whole.                          */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(HA_ARRAY_INT) ARRAY_ARRAY_INT;
typedef HA_ARRAY(KHE_WMATCH_NODE) ARRAY_KHE_WMATCH_NODE;
#define HEAP ARRAY_KHE_WMATCH_NODE

struct khe_wmatch_rec {
  HA_ARENA		arena;			/* arena holding the lot     */
  void			*back;			/* original		     */
  GENERIC_DEBUG_FN	wmatch_back_debug;	/* debug wmatch original     */
  GENERIC_DEBUG_FN	demand_back_debug;	/* debug demand original     */
  GENERIC_DEBUG_FN	supply_back_debug;	/* debug supply original     */
  bool			is_dirty;		/* true if dirty             */
  int			curr_cost1;		/* current infeasibility     */
  int64_t		curr_cost2;		/* current badness           */
  KHE_WMATCH_EDGE_FN	edge_fn;		/* edge function             */
  COST_DEBUG_FN		cost_debug;		/* cost debug function       */
  bool			special_mode;		/* special mode in force     */
  bool			use_categories;		/* true if using tabu cats   */
  int64_t		category_cost;		/* category violation penalty*/
  KHE_WMATCH_CATEGORY	category_list;		/* all categories of m       */
  int			next_category_index;	/* next avail category index */
  int			visit_num;		/* visited flag (int form)   */
  int			demand_count;		/* unfixed demand node count */
  KHE_WMATCH_NODE	demand_list;		/* unfixed demand nodes      */
  int			supply_count;		/* unfixed supply node count */
  KHE_WMATCH_NODE	supply_list;		/* unfixed supply nodes      */
  int			total_flow;		/* total flow if up to date  */
  int			dirty_node_count;	/* number of dirty nodes     */
  HEAP			heap;			/* heap for shortest paths   */
  KHE_WMATCH_NODE	source;			/* source node               */
  KHE_WMATCH_NODE	sink;			/* sink node                 */
  bool			forced_test_active;	/* currently testing for it  */
  ARRAY_KHE_WMATCH_NODE	forced_test_nodes;	/* testing for forced assts  */
  ARRAY_HALL_SET	hall_sets;		/* Hall sets                 */
  KHE_WMATCH_NODE	node_free_list;		/* free list of nodes        */
  KHE_WMATCH_EDGE	edge_free_list;		/* free list of edges        */
  KHE_WMATCH_CATEGORY	category_free_list;	/* free list of categories   */
  HALL_SET		hall_set_free_list;	/* free list of Hall sets    */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "edge management" (private)                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_WMATCH_EDGE EdgeMake(KHE_WMATCH m)                                   */
/*                                                                           */
/*  Get a new edge record, either from m's free list or from malloc.         */
/*                                                                           */
/*****************************************************************************/

static KHE_WMATCH_EDGE EdgeMake(KHE_WMATCH m)
{
  KHE_WMATCH_EDGE res;
  if( m->edge_free_list != NULL )
  {
    res = m->edge_free_list;
    m->edge_free_list = m->edge_free_list->next_edge;
  }
  else
  {
    HaMake(res, m->arena);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void EdgeFree(KHE_WMATCH_EDGE e)                                         */
/*                                                                           */
/*  Free e (add it to the matching's edge free list).                        */
/*                                                                           */
/*****************************************************************************/

static void EdgeFree(KHE_WMATCH_EDGE e)
{
  KHE_WMATCH m = e->endpoint->wmatch;
  e->next_edge = m->edge_free_list;
  m->edge_free_list = e;
}


/*****************************************************************************/
/*                                                                           */
/*  void EdgeListFree(KHE_WMATCH_EDGE edge_list)                             */
/*                                                                           */
/*  Free an entire list of edges, whose first element is edge_list.          */
/*                                                                           */
/*****************************************************************************/

static void EdgeListFree(KHE_WMATCH_EDGE edge_list)
{
  KHE_WMATCH_EDGE last_edge;  KHE_WMATCH m;
  if( edge_list != NULL )
  {
    last_edge = edge_list->prev_edge;
    m = last_edge->endpoint->wmatch;
    last_edge->next_edge = m->edge_free_list;
    m->edge_free_list = edge_list;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void EdgeListAddEdge(KHE_WMATCH_EDGE *edge_list, KHE_WMATCH_EDGE edge)   */
/*                                                                           */
/*  Add edge to the end of *edge_list.                                       */
/*                                                                           */
/*****************************************************************************/

static void EdgeListAddEdge(KHE_WMATCH_EDGE *edge_list, KHE_WMATCH_EDGE edge)
{
  if( *edge_list == NULL )
  {
    /* edge is sole occupant of *edge_list */
    *edge_list = edge;
    edge->prev_edge = edge->next_edge = edge;
  }
  else
  {
    /* edge is not sole occupant of *edge_list */
    edge->prev_edge = (*edge_list)->prev_edge;
    edge->next_edge = *edge_list;
    edge->prev_edge->next_edge = edge;
    edge->next_edge->prev_edge = edge;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void EdgeListAppend(KHE_WMATCH_EDGE *edge_list,                          */
/*    KHE_WMATCH_EDGE *other_edge_list)                                      */
/*                                                                           */
/*  Append *other_edge_list to *edge_list, clearing *other_edge_list.        */
/*                                                                           */
/*****************************************************************************/

static void EdgeListAppend(KHE_WMATCH_EDGE *edge_list,
  KHE_WMATCH_EDGE *other_edge_list)
{
  KHE_WMATCH_EDGE last_edge, other_last_edge;
  if( *other_edge_list != NULL )
  {
    if( *edge_list == NULL )
      *edge_list = *other_edge_list;
    else
    {
      last_edge = (*edge_list)->prev_edge;
      other_last_edge = (*other_edge_list)->prev_edge;
      (*edge_list)->prev_edge = other_last_edge;
      other_last_edge->next_edge = *edge_list;
      (*other_edge_list)->prev_edge = last_edge;
      last_edge->next_edge = *other_edge_list;
    }
    *other_edge_list = NULL;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void EdgeListDeleteEdge(KHE_WMATCH_EDGE *edge_list, KHE_WMATCH_EDGE edge)*/
/*                                                                           */
/*  Delete edge from edge_list.                                              */
/*                                                                           */
/*****************************************************************************/

static void EdgeListDeleteEdge(KHE_WMATCH_EDGE *edge_list, KHE_WMATCH_EDGE edge)
{
  if( edge->next_edge == edge )
  {
    /* edge is sole occupant of *edge_list */
    HnAssert(*edge_list == edge, "EdgeListDeleteEdge internal error");
    *edge_list = NULL;
  }
  else
  {
    /* edge is not sole occupant of *edge_list */
    if( *edge_list == edge )
      *edge_list = edge->next_edge;
    edge->prev_edge->next_edge = edge->next_edge;
    edge->next_edge->prev_edge = edge->prev_edge;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void InsertEdge(KHE_WMATCH_NODE from, KHE_WMATCH_NODE to, int64_t cost)  */
/*                                                                           */
/*  Insert an edge from from to to of capacity 1 and cost as given.  Also    */
/*  add its opposite edge.                                                   */
/*                                                                           */
/*****************************************************************************/

static void InsertEdge(KHE_WMATCH_NODE from, KHE_WMATCH_NODE to, int64_t cost)
{
  KHE_WMATCH_EDGE edge, opposite_edge;
  HnAssert(cost >= 0, "InsertEdge internal error");

  /* get two edge records, edge and its opposite_edge */
  edge = EdgeMake(from->wmatch);
  opposite_edge = EdgeMake(from->wmatch);

  /* initialize edge to go from from to to */
  EdgeListAddEdge(&from->edge_list, edge);
  edge->endpoint = to;
  edge->opposite_edge = opposite_edge;
  edge->capacity = 1;
  edge->cost = cost;
  edge->flow = 0;

  /* initialize opposite_edge to go from to to from */
  EdgeListAddEdge(&to->edge_list, opposite_edge);
  opposite_edge->endpoint = from;
  opposite_edge->opposite_edge = edge;
  opposite_edge->capacity = -1;
  opposite_edge->cost = - cost;
  opposite_edge->flow = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void DeleteEdgePairs(KHE_WMATCH_EDGE *edge_list)                         */
/*                                                                           */
/*  Delete all the edges and their matching opposite edges from *edge_list.  */
/*                                                                           */
/*****************************************************************************/

static void DeleteEdgePairs(KHE_WMATCH_EDGE *edge_list)
{
  KHE_WMATCH_EDGE e;
  while( *edge_list != NULL )
  {
    e = *edge_list;
    EdgeListDeleteEdge(&e->endpoint->edge_list, e->opposite_edge);
    EdgeFree(e->opposite_edge);
    EdgeListDeleteEdge(edge_list, e);
    EdgeFree(e);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  EdgeListForEach(KHE_WMATCH_EDGE elist, EDGE e)                           */
/*                                                                           */
/*  Iterator over all edges e of elist.                                      */
/*                                                                           */
/*****************************************************************************/

#define EdgeListForEach(elist, e)					\
for( e = (elist); e != NULL; e = e->next_edge == (elist) ? NULL : e->next_edge)


/*****************************************************************************/
/*                                                                           */
/*  bool EdgeIsForward(KHE_WMATCH_EDGE e)                                    */
/*                                                                           */
/*  Return true if e is a forward edge.                                      */
/*                                                                           */
/*****************************************************************************/

#define EdgeIsForward(e) ((e)->capacity > 0)


/*****************************************************************************/
/*                                                                           */
/*  int EdgeListCount(KHE_WMATCH_EDGE edge_list)                             */
/*                                                                           */
/*  Count the number of edges on edge_list.                                  */
/*                                                                           */
/*****************************************************************************/

static int EdgeListCount(KHE_WMATCH_EDGE edge_list)
{
  int res;  KHE_WMATCH_EDGE e;
  res = 0;
  EdgeListForEach(edge_list, e)
    res++;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void MoveEdgesFromOrdinaryToSpecial(KHE_WMATCH_NODE node)                */
/*                                                                           */
/*  Move all the ordinary edges of node to the special list; do it for       */
/*  their opposite edges also.                                               */
/*                                                                           */
/*****************************************************************************/

static void MoveEdgesFromOrdinaryToSpecial(KHE_WMATCH_NODE node)
{
  KHE_WMATCH_EDGE e;

  /* move the opposite edges to their appropriate special edge lists */
  EdgeListForEach(node->edge_list, e)
  {
    EdgeListDeleteEdge(&e->endpoint->edge_list, e->opposite_edge);
    EdgeListAddEdge(&e->endpoint->special_edge_list, e->opposite_edge);
  }

  /* move the edges themselves (this will clear node->edge_list) */
  EdgeListAppend(&node->special_edge_list, &node->edge_list);
}


/*****************************************************************************/
/*                                                                           */
/*  void MoveEdgeFromSpecialToOrdinary(KHE_WMATCH_EDGE e)                    */
/*                                                                           */
/*  Move e and its opposite edge from the special list where they are        */
/*  now to the ordinary list.                                                */
/*                                                                           */
/*****************************************************************************/

static void MoveEdgeFromSpecialToOrdinary(KHE_WMATCH_EDGE e)
{
  /* do it for e */
  EdgeListDeleteEdge(&e->beginpoint->special_edge_list, e);
  EdgeListAddEdge(&e->beginpoint->edge_list, e);
  e->flow = 0;

  /* do it for e's opposite edge */
  EdgeListDeleteEdge(&e->endpoint->special_edge_list, e->opposite_edge);
  EdgeListAddEdge(&e->endpoint->edge_list, e->opposite_edge);
  e->opposite_edge->flow = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "categories"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_WMATCH_CATEGORY CategoryMake(KHE_WMATCH m)                           */
/*                                                                           */
/*  Make a new category object, either from m's free list or from malloc.    */
/*                                                                           */
/*****************************************************************************/

static KHE_WMATCH_CATEGORY CategoryMake(KHE_WMATCH m)
{
  KHE_WMATCH_CATEGORY res;
  if( m->category_free_list != NULL )
  {
    res = m->category_free_list;
    m->category_free_list = res->next_category;
  }
  else
    HaMake(res, m->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void CategoryFree(KHE_WMATCH_CATEGORY c)                                 */
/*                                                                           */
/*  Free c.                                                                  */
/*                                                                           */
/*****************************************************************************/

static void CategoryFree(KHE_WMATCH_CATEGORY c)
{
  KHE_WMATCH m = c->wmatch;
  c->next_category = m->category_free_list;
  m->category_free_list = c;
}


/*****************************************************************************/
/*                                                                           */
/*  void CategoryListFree(KHE_WMATCH_CATEGORY category)                      */
/*                                                                           */
/*  Add a whole list of categories, beginning with category, to the          */
/*  free list.                                                               */
/*                                                                           */
/*****************************************************************************/

static void CategoryListFree(KHE_WMATCH_CATEGORY category_list)
{
  KHE_WMATCH_CATEGORY last_category;  KHE_WMATCH m;
  if( category_list != NULL )
  {
    last_category = category_list;
    m = last_category->wmatch;
    while( last_category->next_category != NULL )
      last_category = last_category->next_category;
    last_category->next_category = m->category_free_list;
    m->category_free_list = category_list;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void CategoryAddNode(KHE_WMATCH_CATEGORY category, KHE_WMATCH_NODE node) */
/*                                                                           */
/*  Add node to category.                                                    */
/*                                                                           */
/*****************************************************************************/

static void CategoryAddNode(KHE_WMATCH_CATEGORY category, KHE_WMATCH_NODE node)
{
  HnAssert(category != NULL, "CategoryAddNode internal error 1");
  HnAssert(node->category == NULL, "CategoryAddNode internal error 2");
  node->category = category;
  if( category->first_node == NULL )
  {
    category->first_node = node;
    node->category_next = node;
    node->category_prev = node;
  }
  else
  {
    node->category_next = category->first_node;
    node->category_prev = category->first_node->category_prev;
    node->category_prev->category_next = node;
    node->category_next->category_prev = node;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void CategoryDeleteNode(KHE_WMATCH_CATEGORY category,                    */
/*    KHE_WMATCH_NODE node)                                                  */
/*                                                                           */
/*  Delete node from category.   The node must currently be in this          */
/*  category.                                                                */
/*                                                                           */
/*****************************************************************************/

static void CategoryDeleteNode(KHE_WMATCH_CATEGORY category,
  KHE_WMATCH_NODE node)
{
  HnAssert(category != NULL, "CategoryDeleteNode internal error 1");
  HnAssert(node->category == category, "CategoryDeleteNode internal error 2");
  if( node->category_next == node )
  {
    HnAssert(category->first_node==node, "CategoryDeleteNode internal error 3");
    category->first_node = NULL;
  }
  else
  {
    if( category->first_node == node )
      category->first_node = node->category_next;
    node->category_next->category_prev = node->category_prev;
    node->category_prev->category_next = node->category_next;
    node->category_prev = node->category_next = node;
  }
  node->category = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void CategoryDebug(KHE_WMATCH_NODE node, KHE_WMATCH m, FILE *fp)         */
/*                                                                           */
/*  Debug print of node category and weight onto fp.                         */
/*                                                                           */
/*****************************************************************************/

static void CategoryDebug(KHE_WMATCH_NODE node, KHE_WMATCH m, FILE *fp)
{
  if( m->use_categories && node->category != NULL )
  {
    if( node->category_strength > 0 )
      fprintf(fp, " c%ds%d", node->category->index, node->category_strength);
    else
      fprintf(fp, " c%d", node->category->index);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_WMATCH_CATEGORY KheWMatchNewCategory(KHE_WMATCH m)                   */
/*                                                                           */
/*  Return a new, empty category for m.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_WMATCH_CATEGORY KheWMatchNewCategory(KHE_WMATCH m)
{
  KHE_WMATCH_CATEGORY res;
  HnAssert(m->use_categories, "KheWMatchNewCategory: categories not in use");
  res = CategoryMake(m);
  res->next_category = m->category_list;
  res->index = m->next_category_index++;
  res->first_node = NULL;
  res->wmatch = m;
  m->category_list = res;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchCategoryDelete(KHE_WMATCH_CATEGORY category)               */
/*                                                                           */
/*  Remove all trace of category from wmatch, including freeing the          */
/*  category.  Any nodes currently in the category will revert to            */
/*  being in no category.                                                    */
/*                                                                           */
/*****************************************************************************/

void KheWMatchCategoryDelete(KHE_WMATCH_CATEGORY category)
{
  KHE_WMATCH_CATEGORY prev_category;

  /* remove all nodes from the category */
  while( category->first_node == NULL )
    CategoryDeleteNode(category, category->first_node);

  /* remove the category from its wmatch */
  if( category->wmatch->category_list == category )
    category->wmatch->category_list = NULL;
  else
  {
    prev_category = category->wmatch->category_list;
    while( prev_category->next_category != category )
      prev_category = prev_category->next_category;
    prev_category->next_category = category->next_category;
  }

  /* free the category */
  CategoryFree(category);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_WMATCH_CATEGORY KheWMatchCategory(KHE_WMATCH_NODE node)              */
/*                                                                           */
/*  Return the category of node.  This will be non-NULL if and only if       */
/*  categories are in use.                                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_WMATCH_CATEGORY KheWMatchCategory(KHE_WMATCH_NODE node)
{
  return node->category;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_WMATCH KheWMatchCategoryWMatch(KHE_WMATCH_CATEGORY category)         */
/*                                                                           */
/*  Return the category containing this wmatch.                              */
/*                                                                           */
/*****************************************************************************/

KHE_WMATCH KheWMatchCategoryWMatch(KHE_WMATCH_CATEGORY category)
{
  return category->wmatch;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "node management" (private)                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_WMATCH_NODE NodeMake(KHE_WMATCH m)                                   */
/*                                                                           */
/*  Get a node, either from m's free list or from malloc, and initialize it. */
/*                                                                           */
/*****************************************************************************/

static KHE_WMATCH_NODE NodeMake(KHE_WMATCH m, void *back,
  KHE_WMATCH_CATEGORY category, int category_strength, NODE_TYPE type,
  NODE_STATE state)
{
  KHE_WMATCH_NODE res;

  /* get the node */
  if( m->node_free_list != NULL )
  {
    res = m->node_free_list;
    m->node_free_list = res->next_node;
  }
  else
    HaMake(res, m->arena);

  /* initialize it */
  res->next_node = res->prev_node = NULL;
  res->wmatch = m;
  res->back = back;
  res->category = NULL;
  if( category != NULL )
  {
    HnAssert(m->use_categories, "NodeMake: categories not in use");
    CategoryAddNode(category, res);
  }
  else
  {
    res->category = NULL;
    res->category_next = res->category_prev = res;
  }
  res->category_strength = category_strength;
  res->type = type;
  res->state = state;
  res->heap_index = 0;
  res->visit_num = 0;
  res->curr_adjusted_distance = 0;
  res->prev_true_distance = 0;
  res->node_flow = 0;
  res->parent_edge = NULL;
  res->edge_list = NULL;
  res->special_edge_list = NULL;
  res->fixed_to = NULL;
  res->forced_test_weight = 0;
  res->hall_set = NULL;
  res->equiv_next = res;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NodeFree(KHE_WMATCH_NODE n)                                         */
/*                                                                           */
/*  Free n.                                                                  */
/*                                                                           */
/*****************************************************************************/

static void NodeFree(KHE_WMATCH_NODE n)
{
  KHE_WMATCH m = n->wmatch;
  n->next_node = m->node_free_list;
  m->node_free_list = n;
}


/*****************************************************************************/
/*                                                                           */
/*  void NodeListFree(KHE_WMATCH_NODE node_list)                             */
/*                                                                           */
/*  Free an entire list of nodes.                                            */
/*                                                                           */
/*****************************************************************************/

static void NodeListFree(KHE_WMATCH_NODE node_list)
{
  KHE_WMATCH_NODE last_node;  KHE_WMATCH m;
  if( node_list != NULL )
  {
    last_node = node_list->prev_node;
    m = last_node->wmatch;
    last_node->next_node = m->node_free_list;
    m->node_free_list = node_list;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  NodeListForEach(KHE_WMATCH_NODE list, NODE x)                            */
/*                                                                           */
/*  Iterator over all nodes x of list.                                       */
/*                                                                           */
/*****************************************************************************/

#define NodeListForEach(list, x)					\
for(x = (list); x != NULL; x = (x->next_node == (list) ? NULL : x->next_node))


/*****************************************************************************/
/*                                                                           */
/*  void NodeListAddNode(KHE_WMATCH_NODE *node_list, KHE_WMATCH_NODE node)   */
/*                                                                           */
/*  Add node to the end of *node_list.                                       */
/*                                                                           */
/*****************************************************************************/

static void NodeListAddNode(KHE_WMATCH_NODE *node_list, KHE_WMATCH_NODE node)
{
  if( *node_list == NULL )
  {
    /* node is sole occupant of *node_list */
    *node_list = node;
    node->prev_node = node->next_node = node;
  }
  else
  {
    /* node is not sole occupant of *node_list */
    node->prev_node = (*node_list)->prev_node;
    node->next_node = *node_list;
    node->prev_node->next_node = node;
    node->next_node->prev_node = node;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NodeListDeleteNode(KHE_WMATCH_NODE *node_list, KHE_WMATCH_NODE node)*/
/*                                                                           */
/*  Delete node from node_list.                                              */
/*                                                                           */
/*****************************************************************************/

static void NodeListDeleteNode(KHE_WMATCH_NODE *node_list, KHE_WMATCH_NODE node)
{
  if( node->next_node == node )
  {
    /* node is sole occupant of *node_list */
    HnAssert(*node_list == node, "NodeListDeleteNode internal error");
    *node_list = NULL;
  }
  else
  {
    /* node is not sole occupant of *node_list */
    if( *node_list == node )
      *node_list = node->next_node;
    node->prev_node->next_node = node->next_node;
    node->next_node->prev_node = node->prev_node;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NodeListAppend(KHE_WMATCH_NODE *node_list1,                         */
/*    KHE_WMATCH_NODE *node_list2)                                           */
/*                                                                           */
/*  Append *node_list2 to the end of *node_list1, clearing *node_list2.      */
/*                                                                           */
/*****************************************************************************/

/* *** unused
static void NodeListAppend(KHE_WMATCH_NODE *node_list1, KHE_WMATCH_NODE *node_list2)
{
  KHE_WMATCH_NODE last1, last2;
  if( *node_list2 != NULL )
  {
    if( *node_list1 == NULL )
    {
      ** result is just *node_list2 **
      *node_list1 = *node_list2;
    }
    else
    {
      ** need to concatenate two non-empty lists **
      last1 = (*node_list1)->prev_node;
      last2 = (*node_list2)->prev_node;
      (*node_list1)->prev_node = last2;
      last2->next_node = *node_list1;
      (*node_list2)->prev_node = last1;
      last1->next_node = *node_list2;
    }
    *node_list2 = NULL;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void CheckDirtyNodeCount(KHE_WMATCH m)                                   */
/*                                                                           */
/*  It is an invariant that the dirty node count of m counts the number      */
/*  of supply and demand nodes in the NODE_DIRTY or NODE_CLEANING state.     */
/*  This function checks this condition and aborts if it fails.              */
/*                                                                           */
/*****************************************************************************/

/* *** correct but not currently needed
static void CheckDirtyNodeCount(KHE_WMATCH m)
{
  KHE_WMATCH_NODE d, s;  int count;
  count = 0;
  NodeListForEach(m->demand_list, d)
  {
    assert(d->state == NODE_CLEAN || d->state == NODE_CLEANING ||
      d->state == NODE_FIXED || d->state == NODE_DIRTY);
    if( d->state == NODE_DIRTY || d->state == NODE_CLEANING )
      count++;
  }
  NodeListForEach(m->supply_list, s)
  {
    assert(s->state == NODE_CLEAN || s->state == NODE_FIXED ||
      s->state == NODE_DIRTY);
    if( s->state == NODE_DIRTY )
      count++;
  }
  if( m->dirty_node_count != count )
  {
    fprintf(stderr, "CheckDirtyNodeCount m->dirty_node_count %d != count %d\n",
      m->dirty_node_count, count);
    assert(false);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void MakeDirty(KHE_WMATCH m, KHE_WMATCH_NODE node)                       */
/*                                                                           */
/*  Ensure that node is marked dirty, and that m is kept informed about      */
/*  how many dirty nodes it has.                                             */
/*                                                                           */
/*  In ordinary mode, a dirty node's edges have to be recalculated from      */
/*  scratch, so we delete them here.  In special mode, their cost is         */
/*  still valid and only whether they are in use or not is in doubt, so      */
/*  we just move them to the special list.                                   */
/*                                                                           */
/*****************************************************************************/

static void MakeDirty(KHE_WMATCH m, KHE_WMATCH_NODE node)
{
  if( node->state == NODE_CLEAN )
  {
    if( m->special_mode )
      MoveEdgesFromOrdinaryToSpecial(node);
    else
      DeleteEdgePairs(&node->edge_list);
    m->dirty_node_count++;
    node->state = NODE_DIRTY;
    /* MonitorDeclareDirty((MONITOR) m); */
    m->is_dirty = true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MakeNotDirty(KHE_WMATCH m, KHE_WMATCH_NODE node)                    */
/*                                                                           */
/*  Ensure that node is marked not dirty, and that m is kept informed about  */
/*  how many dirty nodes it has.                                             */
/*                                                                           */
/*****************************************************************************/

static void MakeNotDirty(KHE_WMATCH m, KHE_WMATCH_NODE node)
{
  HnAssert(node->state != NODE_FIXED, "MakeNotDirty internal error");
  if( node->state != NODE_CLEAN )
  {
    m->dirty_node_count--;
    node->state = NODE_CLEAN;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  ForEachOtherEquivalent(KHE_WMATCH_NODE demand_node, KHE_WMATCH_NODE d)   */
/*                                                                           */
/*  Iterate over each demand node d equivalent to demand_node, except        */
/*  do not visit demand_node itself.                                         */
/*                                                                           */
/*****************************************************************************/

#define ForEachOtherEquivalent(demand_node, d)				    \
  for( d = demand_node->equiv_next;  d != demand_node;  d = d->equiv_next)


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchMakeNodesClean(KHE_WMATCH m)                               */
/*                                                                           */
/*  Make the nodes of m clean.                                               */
/*                                                                           */
/*****************************************************************************/

static void KheWMatchMakeNodesClean(KHE_WMATCH m)
{
  KHE_WMATCH_NODE demand, d, supply;  int64_t cost;  KHE_WMATCH_EDGE e, ne;
  if( m->dirty_node_count > 0 )
  {
    if( m->special_mode )
    {
      /* clean up special mode */
      NodeListForEach(m->demand_list, demand)
	if( demand->state == NODE_DIRTY )
	{
	  for( e = demand->special_edge_list;  e != NULL;  e = ne )
	  {
	    ne = e->next_edge != demand->special_edge_list ? e->next_edge:NULL;
	    if( e->endpoint->back == NULL ||
		m->edge_fn(demand->back, e->endpoint->back, NULL) )
	      MoveEdgeFromSpecialToOrdinary(e);
	    e = ne;
	  }
	  MakeNotDirty(m, demand);
        }
      NodeListForEach(m->supply_list, supply)
	if( supply->state == NODE_DIRTY )
	{
	  for( e = supply->special_edge_list;  e != NULL;  e = ne )
	  {
	    ne = e->next_edge != supply->special_edge_list ? e->next_edge:NULL;
	    if( e->endpoint->back == NULL ||
	        m->edge_fn(e->endpoint->back, supply->back, NULL) )
	      MoveEdgeFromSpecialToOrdinary(e);
	    e = ne;
	  }
	  MakeNotDirty(m, supply);
	}
    }
    else
    {
      /* visit every demand node once */
      m->visit_num++;
      NodeListForEach(m->demand_list, demand)
	if( demand->visit_num < m->visit_num )
	{
	  if( demand->state == NODE_CLEAN )
	  {
	    /* check that equivalent nodes are clean, and mark them visited */
	    demand->visit_num = m->visit_num;
	    ForEachOtherEquivalent(demand, d)
	    {
	      HnAssert(d->state == NODE_CLEAN,
		"KheWMatchMakeNodesClean internal error 1");
	      d->visit_num = m->visit_num;
	    }

	    /* rematch these clean nodes with dirty supply nodes only */
	    NodeListForEach(m->supply_list, supply)
	      if( supply->state == NODE_DIRTY &&
		  m->edge_fn(demand->back, supply->back, &cost) )
	      {
		InsertEdge(demand, supply, cost);
		ForEachOtherEquivalent(demand, d)
		  InsertEdge(d, supply, cost);
	      }
	  }
	  else if( demand->state == NODE_DIRTY )
	  {
	    /* check that equivalent nodes are dirty, mark them visited, */
	    /* make them clean, and link the source to them              */
	    demand->visit_num = m->visit_num;
	    MakeNotDirty(m, demand);
	    InsertEdge(m->source, demand, 0);
	    ForEachOtherEquivalent(demand, d)
	    {
	      HnAssert(d->state == NODE_DIRTY,
		"KheWMatchMakeNodesClean internal error 2");
	      d->visit_num = m->visit_num;
	      MakeNotDirty(m, d);
	      InsertEdge(m->source, d, 0);
	    }

	    /* rematch with all supply nodes, except fixed ones */
	    NodeListForEach(m->supply_list, supply)
	    {
	      if( supply->state != NODE_FIXED &&
		  m->edge_fn(demand->back, supply->back, &cost) )
	      {
		InsertEdge(demand, supply, cost);
		ForEachOtherEquivalent(demand, d)
		  InsertEdge(d, supply, cost);
	      }
	    }
	  }
	}

      /* insert edges from dirty supply nodes to sink, and make clean */
      NodeListForEach(m->supply_list, supply)
	if( supply->state == NODE_DIRTY )
	{
	  MakeNotDirty(m, supply);
	  InsertEdge(supply, m->sink, 0);
	}
      HnAssert(m->dirty_node_count == 0,
	"KheWMatchMakeNodesClean internal error 3");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NodeDebug(KHE_WMATCH_NODE v, FILE *fp)                              */
/*                                                                           */
/*  Debug print of v onto fp.   It knows what sort of node it is.            */
/*                                                                           */
/*****************************************************************************/

static void NodeDebug(KHE_WMATCH_NODE v, FILE *fp)
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

      v->wmatch->supply_back_debug(v->back, 1, -1, fp);
      break;

    case NODE_DEMAND:

      v->wmatch->demand_back_debug(v->back, 1, -1, fp);
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
/*  KHE_WMATCH_NODE KheWMatchDemandNodeMake(KHE_WMATCH m, void *back,        */
/*    KHE_WMATCH_CATEGORY category, int category_strength)                   */
/*                                                                           */
/*  Add and return one demand node.  Initially it is dirty, since we         */
/*  have no idea what edges should be linked to it.                          */
/*                                                                           */
/*  Dirty demand nodes are not linked to any nodes, not even to the source.  */
/*  They are however stored in the matching's list of demand nodes.          */
/*                                                                           */
/*  If category != NULL, this node is in that category.  Otherwise it lies   */
/*  in a category of its own, if categories are in use at all.               */
/*                                                                           */
/*****************************************************************************/

KHE_WMATCH_NODE KheWMatchDemandNodeMake(KHE_WMATCH m, void *back,
  KHE_WMATCH_CATEGORY category, int category_strength)
{
  KHE_WMATCH_NODE res;
  HnAssert(!m->special_mode, "KheWMatchDemandNodeMake internal error 1");
  res = NodeMake(m, back, category, category_strength, NODE_DEMAND, NODE_CLEAN);
  NodeListAddNode(&m->demand_list, res);
  m->demand_count++;
  MakeDirty(m, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_WMATCH KheWMatchDemandNodeWMatch(KHE_WMATCH_NODE demand_node)        */
/*                                                                           */
/*  Return the KHE_WMATCH containing demand_node.                            */
/*                                                                           */
/*****************************************************************************/

KHE_WMATCH KheWMatchDemandNodeWMatch(KHE_WMATCH_NODE demand_node)
{
  return demand_node->wmatch;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheWMatchDemandNodeBack(KHE_WMATCH_NODE demand_node)               */
/*                                                                           */
/*  Return the back pointer attribute of demand_node.                        */
/*                                                                           */
/*****************************************************************************/

void *KheWMatchDemandNodeBack(KHE_WMATCH_NODE demand_node)
{
  return demand_node->back;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_WMATCH_CATEGORY KheWMatchDemandNodeCategory(                         */
/*    KHE_WMATCH_NODE demand_node)                                           */
/*                                                                           */
/*  Return the category attribute of demand_node, or NULL if none.           */
/*                                                                           */
/*****************************************************************************/

KHE_WMATCH_CATEGORY KheWMatchDemandNodeCategory(KHE_WMATCH_NODE demand_node)
{
  return demand_node->category;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheWMatchDemandNodeCategoryStrength(KHE_WMATCH_NODE demand_node)     */
/*                                                                           */
/*  Return the category strength attribute of demand_node.                   */
/*                                                                           */
/*****************************************************************************/

int KheWMatchDemandNodeCategoryStrength(KHE_WMATCH_NODE demand_node)
{
  return demand_node->category_strength;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchDemandNodeNotifyDirty(KHE_WMATCH_NODE demand_node)         */
/*                                                                           */
/*  Notify its wmatch that demand_node is dirty.                             */
/*                                                                           */
/*****************************************************************************/

void KheWMatchDemandNodeNotifyDirty(KHE_WMATCH_NODE demand_node)
{
  MakeDirty(demand_node->wmatch, demand_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchDemandNodeFix(KHE_WMATCH_NODE demand_node,                 */
/*    KHE_WMATCH_NODE supply_node)                                           */
/*                                                                           */
/*  Fix demand_node to supply_node.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheWMatchDemandNodeFix(KHE_WMATCH_NODE demand_node,
  KHE_WMATCH_NODE supply_node)
{
  KHE_WMATCH m;
  m = demand_node->wmatch;
  KheWMatchDemandNodeDeleteEquivalent(demand_node);
  m->demand_count--;
  MakeNotDirty(m, demand_node);
  /* MonitorDeclareDirty((MONITOR) m); */
  m->is_dirty = true;
  if( m->special_mode )
    MoveEdgesFromOrdinaryToSpecial(demand_node);
  else
    DeleteEdgePairs(&demand_node->edge_list);
  NodeListDeleteNode(&m->demand_list, demand_node);
  demand_node->state = NODE_FIXED;
  demand_node->fixed_to = supply_node;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchDemandNodeUnFix(KHE_WMATCH_NODE demand_node)               */
/*                                                                           */
/*  Unfix demand_node.                                                       */
/*                                                                           */
/*****************************************************************************/

static void KheWMatchDemandNodeUnFix(KHE_WMATCH_NODE demand_node)
{
  KHE_WMATCH m = demand_node->wmatch;
  KheWMatchDemandNodeDeleteEquivalent(demand_node);
  demand_node->state = NODE_CLEAN;
  NodeListAddNode(&m->demand_list, demand_node);
  demand_node->curr_adjusted_distance = 0;
  demand_node->prev_true_distance = 0;
  demand_node->node_flow = 0;
  HnAssert(demand_node->edge_list == NULL,
    "KheWMatchDemandNodeUnFix internal error");
  demand_node->fixed_to = NULL;
  m->demand_count++;
  MakeDirty(m, demand_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchDemandNodeDelete(KHE_WMATCH_NODE demand_node)              */
/*                                                                           */
/*  Delete node and all adjacent edges.  If it's fixed, unfix it.            */
/*                                                                           */
/*****************************************************************************/

void KheWMatchDemandNodeDelete(KHE_WMATCH_NODE demand_node)
{
  KHE_WMATCH m;
  m = demand_node->wmatch;
  HnAssert(!m->special_mode, "KheWMatchDemandNodeDelete internal error");
  KheWMatchDemandNodeDeleteEquivalent(demand_node);
  if( demand_node->state == NODE_FIXED )
    KheWMatchEdgeUnFix(demand_node);
  m->demand_count--;
  MakeNotDirty(m, demand_node);
  /* MonitorDeclareDirty((MONITOR) m); */
  m->is_dirty = true;
  DeleteEdgePairs(&demand_node->edge_list);
  NodeListDeleteNode(&m->demand_list, demand_node);
  if( demand_node->category != NULL )
    CategoryDeleteNode(demand_node->category, demand_node);
  demand_node->state = NODE_FIXED;
  NodeFree(demand_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchDemandNodeAddToCategory(KHE_WMATCH_NODE demand_node,       */
/*    KHE_WMATCH_CATEGORY category, int category_strength)                   */
/*                                                                           */
/*  Add demand_node to category.  The node must initially not be in any      */
/*  category.                                                                */
/*                                                                           */
/*****************************************************************************/

void KheWMatchDemandNodeAddToCategory(KHE_WMATCH_NODE demand_node,
  KHE_WMATCH_CATEGORY category, int category_strength)
{
  demand_node->category_strength = category_strength;
  CategoryAddNode(category, demand_node);
  KheWMatchDemandNodeNotifyDirty(demand_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchDemandNodeDeleteFromCategory(KHE_WMATCH_NODE demand_node)  */
/*                                                                           */
/*  Delete demand_node from its category.  It must currently be in one.      */
/*                                                                           */
/*****************************************************************************/

void KheWMatchDemandNodeDeleteFromCategory(KHE_WMATCH_NODE demand_node)
{
  CategoryDeleteNode(demand_node->category, demand_node);
  KheWMatchDemandNodeNotifyDirty(demand_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchDemandNodeEquivalent(KHE_WMATCH_NODE demand_node1,         */
/*    KHE_WMATCH_NODE demand_node2)                                          */
/*                                                                           */
/*  Record the fact that these demand nodes are equivalent, in the sense     */
/*  that they have the same adjacent edges with the same costs, and always   */
/*  get marked dirty at the same time.  Consequently, the edge set of one    */
/*  can serve as the edge set of the others.                                 */
/*                                                                           */
/*****************************************************************************/

void KheWMatchDemandNodeEquivalent(KHE_WMATCH_NODE demand_node1,
  KHE_WMATCH_NODE demand_node2)
{
  KHE_WMATCH_NODE d;

  /* make sure that neither is on the list of the other */
  HnAssert(demand_node1!=demand_node2,
    "KheWMatchDemandNodeEquivalent internal error");
  ForEachOtherEquivalent(demand_node1, d)
    if( d == demand_node2 )
      return;

  /* meld the two lists */
  d = demand_node1->equiv_next;
  demand_node1->equiv_next = demand_node2->equiv_next;
  demand_node2->equiv_next = d;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchDemandNodeDeleteEquivalent(KHE_WMATCH_NODE demand_node)    */
/*                                                                           */
/*  Ensure that demand_node is not equivalent to anything.                   */
/*                                                                           */
/*****************************************************************************/

void KheWMatchDemandNodeDeleteEquivalent(KHE_WMATCH_NODE demand_node)
{
  KHE_WMATCH_NODE d;
  if( demand_node->equiv_next != demand_node )
  {
    d = demand_node->equiv_next;
    while( d->equiv_next != demand_node )
      d = d->equiv_next;
    d->equiv_next = demand_node->equiv_next;
    demand_node->equiv_next = demand_node;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "supply nodes"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_WMATCH_NODE KheWMatchSupplyNodeMake(KHE_WMATCH m, void *back,        */
/*    KHE_WMATCH_CATEGORY category)                                          */
/*                                                                           */
/*  Add and return one supply node.  Initially it is dirty, since we         */
/*  have no idea what edges should be linked to it.                          */
/*                                                                           */
/*  Dirty supply nodes are not linked to any nodes, not even to the sink.    */
/*  They are however stored in the matching's list of supply nodes.          */
/*                                                                           */
/*  If category != NULL, this node is in the same tabu category              */
/*  as category.  Otherwise it lies in a category of its own.                */
/*                                                                           */
/*****************************************************************************/

KHE_WMATCH_NODE KheWMatchSupplyNodeMake(KHE_WMATCH m, void *back,
  KHE_WMATCH_CATEGORY category)
{
  KHE_WMATCH_NODE res;
  HnAssert(!m->special_mode, "KheWMatchSupplyNodeMake internal error 1");
  res = NodeMake(m, back, category, 0, NODE_SUPPLY, NODE_CLEAN);
  NodeListAddNode(&m->supply_list, res);
  m->supply_count++;
  MakeDirty(m, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_WMATCH KheWMatchSupplyNodeWMatch(KHE_WMATCH_NODE supply_node)        */
/*                                                                           */
/*  Return the KHE_WMATCH containing supply_node.                            */
/*                                                                           */
/*****************************************************************************/

KHE_WMATCH KheWMatchSupplyNodeWMatch(KHE_WMATCH_NODE supply_node)
{
  return supply_node->wmatch;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheWMatchSupplyNodeBack(KHE_WMATCH_NODE supply_node)               */
/*                                                                           */
/*  Return the back pointer of supply_node.                                  */
/*                                                                           */
/*****************************************************************************/

void *KheWMatchSupplyNodeBack(KHE_WMATCH_NODE supply_node)
{
  return supply_node->back;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_WMATCH_CATEGORY KheWMatchSupplyNodeCategory(                         */
/*    KHE_WMATCH_NODE supply_node)                                           */
/*                                                                           */
/*  Return the category of supply_node, possibly NULL.                       */
/*                                                                           */
/*****************************************************************************/

KHE_WMATCH_CATEGORY KheWMatchSupplyNodeCategory(KHE_WMATCH_NODE supply_node)
{
  return supply_node->category;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchSupplyNodeNotifyDirty(KHE_WMATCH_NODE supply_node)         */
/*                                                                           */
/*  Notify its wmatch that supply_node is dirty.                             */
/*                                                                           */
/*****************************************************************************/

void KheWMatchSupplyNodeNotifyDirty(KHE_WMATCH_NODE supply_node)
{
  MakeDirty(supply_node->wmatch, supply_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchSupplyNodeFix(KHE_WMATCH_NODE supply_node,                 */
/*    KHE_WMATCH_NODE demand_node)                                           */
/*                                                                           */
/*  Fix supply_node to demand_node.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheWMatchSupplyNodeFix(KHE_WMATCH_NODE supply_node,
  KHE_WMATCH_NODE demand_node)
{
  KHE_WMATCH m = supply_node->wmatch;
  m->supply_count--;
  MakeNotDirty(m, supply_node);
  /* MonitorDeclareDirty((MONITOR) m); */
  m->is_dirty = true;
  if( m->special_mode )
    MoveEdgesFromOrdinaryToSpecial(supply_node);
  else
    DeleteEdgePairs(&supply_node->edge_list);
  NodeListDeleteNode(&m->supply_list, supply_node);
  supply_node->state = NODE_FIXED;
  supply_node->fixed_to = demand_node;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchSupplyNodeUnFix(KHE_WMATCH_NODE supply_node)               */
/*                                                                           */
/*  Unfix supply_node.                                                       */
/*                                                                           */
/*****************************************************************************/

static void KheWMatchSupplyNodeUnFix(KHE_WMATCH_NODE supply_node)
{
  KHE_WMATCH m = supply_node->wmatch;
  NodeListAddNode(&m->supply_list, supply_node);
  supply_node->state = NODE_CLEAN;
  supply_node->curr_adjusted_distance = 0;
  supply_node->prev_true_distance = 0;
  supply_node->node_flow = 0;
  HnAssert(supply_node->edge_list == NULL,
    "KheWMatchSupplyNodeUnFix internal error");
  supply_node->fixed_to = NULL;
  m->supply_count++;
  MakeDirty(m, supply_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchSupplyNodeDelete(KHE_WMATCH_NODE supply_node)              */
/*                                                                           */
/*  Delete supply_node and all adjacent edges.                               */
/*                                                                           */
/*****************************************************************************/

void KheWMatchSupplyNodeDelete(KHE_WMATCH_NODE supply_node)
{
  KHE_WMATCH m = supply_node->wmatch;
  HnAssert(!m->special_mode, "KheWMatchSupplyNodeDelete internal error");
  if( supply_node->state == NODE_FIXED )
    KheWMatchEdgeUnFix(supply_node->fixed_to);
  m->supply_count--;
  MakeNotDirty(m, supply_node);
  /* MonitorDeclareDirty((MONITOR) m); */
  m->is_dirty = true;
  DeleteEdgePairs(&supply_node->edge_list);
  NodeListDeleteNode(&m->supply_list, supply_node);
  if( supply_node->category != NULL )
    CategoryDeleteNode(supply_node->category, supply_node);
  supply_node->state = NODE_FIXED;
  NodeFree(supply_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchSupplyNodeAddToCategory(KHE_WMATCH_NODE supply_node,       */
/*    KHE_WMATCH_CATEGORY category)                                          */
/*                                                                           */
/*  Add supply_node to category.  The node must initially not be in any      */
/*  category.                                                                */
/*                                                                           */
/*****************************************************************************/

void KheWMatchSupplyNodeAddToCategory(KHE_WMATCH_NODE supply_node,
  KHE_WMATCH_CATEGORY category)
{
  CategoryAddNode(category, supply_node);
  KheWMatchSupplyNodeNotifyDirty(supply_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchSupplyNodeDeleteFromCategory(KHE_WMATCH_NODE supply_node)  */
/*                                                                           */
/*  Delete supply_node from its category.  It must currently be in one.      */
/*                                                                           */
/*****************************************************************************/

void KheWMatchSupplyNodeDeleteFromCategory(KHE_WMATCH_NODE supply_node)
{
  CategoryDeleteNode(supply_node->category, supply_node);
  KheWMatchSupplyNodeNotifyDirty(supply_node);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "edge fixing"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchEdgeFix(KHE_WMATCH_NODE demand_node,                       */
/*    KHE_WMATCH_NODE supply_node)                                           */
/*                                                                           */
/*  Fix demand_node to supply_node; or do nothing if already fixed.          */
/*                                                                           */
/*****************************************************************************/

void KheWMatchEdgeFix(KHE_WMATCH_NODE demand_node, KHE_WMATCH_NODE supply_node)
{
  if( demand_node->state != NODE_FIXED )
  {
    KheWMatchDemandNodeFix(demand_node, supply_node);
    KheWMatchSupplyNodeFix(supply_node, demand_node);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchEdgeUnFix(KHE_WMATCH_NODE demand_node,                     */
/*    KHE_WMATCH_NODE supply_node)                                           */
/*                                                                           */
/*  Unfix demand_node from whatever it is currently fixed to; or do nothing  */
/*  if already unfixed.                                                      */
/*                                                                           */
/*****************************************************************************/

void KheWMatchEdgeUnFix(KHE_WMATCH_NODE demand_node)
{
  KHE_WMATCH_NODE supply_node;
  if( demand_node->state == NODE_FIXED )
  {
    supply_node = demand_node->fixed_to;
    HnAssert(supply_node->state == NODE_FIXED,
      "KheWMatchEdgeUnFix internal error 1");
    HnAssert(supply_node->fixed_to == demand_node,
      "KheWMatchEdgeUnFix internal error 2");
    KheWMatchDemandNodeUnFix(demand_node);
    KheWMatchSupplyNodeUnFix(supply_node);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "edge freezing"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchEdgeFreeze(KHE_WMATCH_NODE demand_node,                    */
/*    KHE_WMATCH_NODE supply_node)                                           */
/*                                                                           */
/*  Freeze demand_node to supply_node.                                       */
/*                                                                           */
/*****************************************************************************/

bool KheWMatchEdgeFreeze(KHE_WMATCH_NODE demand_node,
  KHE_WMATCH_NODE supply_node)
{
  KHE_WMATCH m;
  KHE_WMATCH_EDGE e, e1, e2;

  /* fail if there is no such edge */
  m = demand_node->wmatch;
  e1 = e2 = NULL;
  EdgeListForEach(demand_node->edge_list, e)
  {
    if( e->endpoint == supply_node )
      e1 = e;
    else if( e->endpoint == m->source )
      e2 = e;
  }
  if( e1 == NULL || e2 == NULL || e1 == e2 )
    return false;

  /* move every edge except e1 and e2 to the specials list */
  m->is_dirty = true;
  MakeNotDirty(m, demand_node);
  MakeNotDirty(m, supply_node);
  MoveEdgesFromOrdinaryToSpecial(demand_node);
  MoveEdgeFromSpecialToOrdinary(e1);
  MoveEdgeFromSpecialToOrdinary(e2);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchEdgeUnFreeze(KHE_WMATCH_NODE demand_node)                  */
/*                                                                           */
/*  Unfreeze demand_node.                                                    */
/*                                                                           */
/*****************************************************************************/

void KheWMatchEdgeUnFreeze(KHE_WMATCH_NODE demand_node)
{
  KHE_WMATCH m = demand_node->wmatch;
  MakeNotDirty(m, demand_node);
  m->is_dirty = true;
  while( demand_node->special_edge_list != NULL )
    MoveEdgeFromSpecialToOrdinary(demand_node->special_edge_list);
}


/*****************************************************************************/
/*                                                                           */
/*  void EdgeDebug(KHE_WMATCH_EDGE e, FILE *fp)                              */
/*                                                                           */
/*  Debug print of e onto fp.                                                */
/*                                                                           */
/*****************************************************************************/

static void EdgeDebug(KHE_WMATCH_EDGE e, FILE *fp)
{
  KHE_WMATCH m = e->endpoint->wmatch;
  fprintf(fp, "--c%df%d(", e->capacity, e->flow);
  if( m->cost_debug != NULL )
    m->cost_debug(m->back, e->cost, fp);
  else
    fprintf(fp, "%ld", e->cost);
  fprintf(fp, ")->");
}


/*****************************************************************************/
/*                                                                           */
/*  void EdgeFullDebug(KHE_WMATCH_EDGE e, FILE *fp)                          */
/*                                                                           */
/*  Debug print of e, including its endpoints.                               */
/*                                                                           */
/*****************************************************************************/

static void EdgeFullDebug(KHE_WMATCH_EDGE e, FILE *fp)
{
  NodeDebug(e->beginpoint, fp);
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
#define HEAP_ENTRY KHE_WMATCH_NODE
#define key(x) ((x)->curr_adjusted_distance)
#define set_heap(heap, i, x) HaArrayPut(heap, i, x), (x)->heap_index = (i)

static void add_leaf(HEAP *heap, int pos)
{
  int i, j;  HEAP_ENTRY x;
  x = HaArray(*heap, pos);
  i = pos;
  j = i / 2;
  while( j > 0 && key(HaArray(*heap, j)) > key(x) )
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
	key(HaArray(*heap, j)) > key(HaArray(*heap, j+1)) )
      j++;
    if( key(x) <= key(HaArray(*heap, j)) )
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
/*  int EdgeAvailableExtraFlow(KHE_WMATCH_EDGE e)                            */
/*                                                                           */
/*  Return the amount of extra flow available along e.                       */
/*                                                                           */
/*****************************************************************************/

static int EdgeAvailableExtraFlow(KHE_WMATCH_EDGE e)
{
  return (EdgeIsForward(e) ? e->capacity - e->flow : - e->flow);
}


/*****************************************************************************/
/*                                                                           */
/*  void SetFlowToZero(KHE_WMATCH m)                                         */
/*                                                                           */
/*  Set the flow to zero.                                                    */
/*                                                                           */
/*****************************************************************************/

static void SetFlowToZeroAtNode(KHE_WMATCH_NODE v)
{
  KHE_WMATCH_EDGE e;
  v->node_flow = 0;
  EdgeListForEach(v->edge_list, e)
    e->flow = 0;
}

static void SetFlowToZero(KHE_WMATCH m)
{
  KHE_WMATCH_NODE v;
  NodeListForEach(m->demand_list, v)
    SetFlowToZeroAtNode(v);
  NodeListForEach(m->supply_list, v)
    SetFlowToZeroAtNode(v);
  SetFlowToZeroAtNode(m->source);
  SetFlowToZeroAtNode(m->sink);
}


/*****************************************************************************/
/*                                                                           */
/*  void InitPrevTrueDistances(KHE_WMATCH m)                                 */
/*                                                                           */
/*  Set the curr_distance fields in all nodes to 0.  These values will be    */
/*  copied into the prev_distance fields as soon as the nodes are touched;   */
/*  from there, they are used to adjust edge costs.                          */
/*                                                                           */
/*****************************************************************************/

static void InitPrevTrueDistances(KHE_WMATCH m)
{
  KHE_WMATCH_NODE v;
  NodeListForEach(m->demand_list, v)
    v->prev_true_distance = 0;
  NodeListForEach(m->supply_list, v)
    v->prev_true_distance = 0;
  m->source->prev_true_distance = 0;
  m->sink->prev_true_distance = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void UpdatePrevTrueDistances(KHE_WMATCH m)                               */
/*                                                                           */
/*  Update the prev_true_distance fields to reflect the true distances       */
/*  at the end of a Dijkstra run.                                            */
/*                                                                           */
/*****************************************************************************/

static void UpdatePrevTrueDistances(KHE_WMATCH m)
{
  KHE_WMATCH_NODE v;
  NodeListForEach(m->demand_list, v)
    v->prev_true_distance += v->curr_adjusted_distance;
  NodeListForEach(m->supply_list, v)
    v->prev_true_distance += v->curr_adjusted_distance;
  m->source->prev_true_distance += m->source->curr_adjusted_distance;
  m->sink->prev_true_distance += m->sink->curr_adjusted_distance;
}


/*****************************************************************************/
/*                                                                           */
/*  void CheckNodeDistances(KHE_WMATCH_NODE v, KHE_WMATCH m)                 */
/*                                                                           */
/*  Carry out the specification of CheckDistances at one node v, which must  */
/*  be visited.  This function is called only when debugging.                */
/*                                                                           */
/*****************************************************************************/

static void CheckNodeDistances(KHE_WMATCH_NODE v, KHE_WMATCH m)
{
  KHE_WMATCH_EDGE e;  KHE_WMATCH_NODE w;

  /* only check visited vertices, and only check if not using categories */
  if( v->visit_num < m->visit_num || m->use_categories )
    return;

  /* make sure the true distance of v can be justified */
  if( v == m->source )
  {
    if( v->parent_edge != NULL )
    {
      fprintf(stderr, "CheckNodeDistances: source has parent_edge\n");
      abort();
    }
    if( v->prev_true_distance != 0 )
    {
      fprintf(stderr, "CheckNodeDistances: source has prev_true_distance ");
      m->cost_debug(m->back, v->prev_true_distance, stderr);
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
    w = e->beginpoint;
    if( w->prev_true_distance + e->cost != v->prev_true_distance )
    {
      fprintf(stderr, "CheckNodeDistances: inconsistent parent edge ");
      NodeDebug(w, stderr);
      fprintf(stderr, " ");
      EdgeDebug(e, stderr);
      fprintf(stderr, "\n");
      abort();
    }
  }

  /* make sure all v's outgoing edges are consistent */
  EdgeListForEach(v->edge_list, e)
    if( EdgeAvailableExtraFlow(e) > 0 )
    {
      w = e->endpoint;
      if( w->visit_num < m->visit_num )
	continue;
      if( w->prev_true_distance > v->prev_true_distance + e->cost )
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
/*  void CheckDistances(KHE_WMATCH m)                                        */
/*                                                                           */
/*  This function is called only when debugging.  It checks that the         */
/*  distances stored in each vertex do represent a shortest path             */
/*  spanning tree.  It must be called after UpdatePrevTrueDistances,         */
/*  since it relies on the prev_true_distance fields being set properly.     */
/*                                                                           */
/*****************************************************************************/

static void CheckDistances(KHE_WMATCH m)
{
  KHE_WMATCH_NODE v;
  NodeListForEach(m->demand_list, v)
    CheckNodeDistances(v, m);
  NodeListForEach(m->supply_list, v)
    CheckNodeDistances(v, m);
  CheckNodeDistances(m->source, m);
  CheckNodeDistances(m->sink, m);
}


/*****************************************************************************/
/*                                                                           */
/*  void UpdateFlowThroughNode(KHE_WMATCH_NODE v, int extra_flow,            */
/*    KHE_WMATCH_EDGE incoming_edge, KHE_WMATCH_EDGE outgoing_edge)          */
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

static void UpdateFlowThroughNode(KHE_WMATCH_NODE v, int extra_flow,
  KHE_WMATCH_EDGE incoming_edge, KHE_WMATCH_EDGE outgoing_edge)
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
/*  void AugmentPath(KHE_WMATCH m, int *extra_flow)                          */
/*                                                                           */
/*  Augment the path from source to sink found by a previous successful      */
/*  call to any function which has set a chain of parent_edge fields from    */
/*  the sink back to the source.  Also set *extra_flow to the amount of      */
/*  extra flow introduced by this augment.                                   */
/*                                                                           */
/*****************************************************************************/
#define prev_edge_on_path(e) ((e)->beginpoint->parent_edge)

static void AugmentPath(KHE_WMATCH m, int *extra_flow)
{
  KHE_WMATCH_EDGE e, prev_e;

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
    e->opposite_edge->flow = - e->flow;
    prev_e = e;
    e = prev_edge_on_path(e);
  }
  m->source->node_flow += *extra_flow;
}


/*****************************************************************************/
/*                                                                           */
/*  bool FindBFSAugmentingPath(KHE_WMATCH f)                                 */
/*                                                                           */
/*  Find an augmenting path from source to sink if one exists, using         */
/*  breadth-first search.                                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool FindBFSAugmentingPath(KHE_WMATCH f)
{
  KHE_WMATCH_NODE v;  KHE_WMATCH_EDGE e;
  for( v = bfs_first_node(f); v!=NULL && v!=f->sink; v = bfs_next_node(f, v) )
    EdgeListForEach(v->edge_list, e)
      if( EdgeAvailableExtraFlow(e) > 0 )
	bfs_follow_edge(f, e);
  return v == f->sink;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void MaxKheWMatch(KHE_WMATCH f)                                          */
/*                                                                           */
/*  Run a max-flow algorithm (Ford-Fulkerson using simple breadth-first      */
/*  search augmenting), unless the flow is already up to date.               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void MaxKheWMatch(KHE_WMATCH f)
{
  int extra_flow;
  if( !f->flow_up_to_date )
  {
    SetFlowToZero(f);
    if( DEBUG1 )
    {
      fprintf(stderr, "[ MaxKheWMatch, initial state is:\n");
      KheWMatchDebug(f, 2, stderr);
    }
    while( FindBFSAugmentingPath(f) )
    {
      AugmentPath(f, &extra_flow);
      f->total_flow += extra_flow;
      if( DEBUG1 )
      {
	fprintf(stderr, "  after augment, state is:\n");
	KheWMatchDebug(f, 2, stderr);
      }
    }
    if( DEBUG1 )
      fprintf(stderr, "] MaxKheWMatch\n");
    f->flow_up_to_date = true;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void Extend(KHE_WMATCH_NODE v, bool *still_firing)                       */
/*                                                                           */
/*  Extend the shortest path through v to all its successors, and set        */
/*  *still_firing to true if any of them were interested.                    */
/*                                                                           */
/*****************************************************************************/

/* ***
static void Extend(KHE_WMATCH_NODE v, bool *still_firing)
{
  KHE_WMATCH_EDGE e;  KHE_WMATCH_NODE v2;
  EdgeListForEach(v->edge_list, e)
    if( EdgeAvailableExtraFlow(e) > 0 )
    {
      v2 = e->endpoint;
      if( v->distance + e->cost < v2->distance )
      {
	v2->distance = v->distance + e->cost;
	v2->parent_edge = e;
	*still_firing = true;
      }
    }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool EdgeInNonTrivialCategories(KHE_WMATCH_EDGE e,                       */
/*    KHE_WMATCH_CATEGORY *demand_category                                   */
/*    KHE_WMATCH_CATEGORY *supply_category, int *count, int *strength)       */
/*                                                                           */
/*  Return true if e requires category handling, because it connects a       */
/*  demand node to a supply node, and the categories of both nodes           */
/*  contain more than one vertex, so there is a risk of a clash.             */
/*  Also return the demand and supply categories in that case, the           */
/*  flow increment (+1 for a forward edge, -1 for a backward edge),          */
/*  and the strength (non-negative for a forward edge, non-positive for      */
/*  a backward one).                                                         */
/*                                                                           */
/*  Otherwise return false.                                                  */
/*                                                                           */
/*****************************************************************************/

static bool EdgeInNonTrivialCategories(KHE_WMATCH_EDGE e,
  KHE_WMATCH_CATEGORY *demand_category, KHE_WMATCH_CATEGORY *supply_category,
  int *count, int *strength)
{
  KHE_WMATCH_NODE v, w;
  v = e->beginpoint;
  w = e->endpoint;
  if( v->category_next == v || w->category_next == w )
    return false;
  else
  {
    if( EdgeIsForward(e) )
    {
      *demand_category = v->category;
      *supply_category = w->category;
      *count = 1;
      *strength = v->category_strength;
    }
    else
    {
      *demand_category = w->category;
      *supply_category = v->category;
      *count = -1;
      *strength = - v->category_strength;
    }
    return true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void DebugEdgeForTabu(KHE_WMATCH_EDGE e, KHE_WMATCH m, FILE *fp)         */
/*                                                                           */
/*  Debug print of edge e.                                                   */
/*                                                                           */
/*****************************************************************************/

static void DebugEdgeForTabu(KHE_WMATCH_EDGE e, KHE_WMATCH m, FILE *fp)
{
  fprintf(fp, "(");
  NodeDebug(e->beginpoint, fp);
  fprintf(fp, " ");
  CategoryDebug(e->beginpoint, m, fp);
  fprintf(fp, " -> ");
  NodeDebug(e->endpoint, fp);
  fprintf(fp, " ");
  CategoryDebug(e->endpoint, m, fp);
  fprintf(fp, ")");
}


/*****************************************************************************/
/*                                                                           */
/*  bool EdgeIsTabu(KHE_WMATCH_EDGE e)                                       */
/*                                                                           */
/*  Assuming that categories are in use and e is a forward edge, so that     */
/*  it is potentially tabu, return true if e is in fact tabu, because the    */
/*  assignment represented by the path to e, if carried out, would cause     */
/*  two or more assignments between the same categories of nodes, from       */
/*  demand nodes of positive total strength.                                 */
/*                                                                           */
/*  The algorithm for this is as follows.  First, make sure that e has       */
/*  non-trivial categories at each endpoint:  if one of the endpoints is     */
/*  in a category by itself, there is no possibility of two assignments      */
/*  clashing, so e cannot be tabu.  Since the source and sink have           */
/*  trivial categories, this test disposes of them as well.                  */
/*                                                                           */
/*  Next, check the categories of the endpoints of every edge on the         */
/*  current path to e, for those with the same endpoint categories as e.     */
/*  Each such edge either adds an assignment between these categories        */
/*  (if it is a forward edge) of a certain strength, or else it takes        */
/*  one away.  Count up the number of additions minus subtractions,          */
/*  including e itself as an addition, to find total_count, the number       */
/*  of extra assignments between these categories that this path causes,     */
/*  and total_strength, the total extra strength of those assignments.       */
/*                                                                           */
/*  If total_count is 0, e cannot cause a problem so return false; if it     */
/*  is 2 or more and total_strength is positive, e is definitely causing     */
/*  a problem so return true.  If total_count is 1, or more but with zero    */
/*  total_strength, we need to continue the search for existing assignments  */
/*  through the nodes of the demand category of e, including fixed edges.    */
/*  So search for such an assignment of positive strength and return true    */
/*  if one is found.                                                         */
/*                                                                           */
/*****************************************************************************/

static bool EdgeIsTabu(KHE_WMATCH_EDGE e, KHE_WMATCH m)
{
  int total_count, count, total_strength, strength;
  KHE_WMATCH_NODE cv;  KHE_WMATCH_EDGE e2;
  KHE_WMATCH_CATEGORY e_dcat, e_scat, dcat, scat;
  if( EdgeInNonTrivialCategories(e, &e_dcat, &e_scat, &total_count,
    &total_strength) )
  {
    /* find the tabu assignments on the path to e inclusive */
    if( DEBUG6 )
    {
      fprintf(stderr, "[ EdgeIsTabu");
      DebugEdgeForTabu(e, m, stderr);
    }
    e2 = prev_edge_on_path(e);
    while( e2 != NULL )
    {
      if( EdgeInNonTrivialCategories(e2, &dcat, &scat, &count, &strength) &&
	  dcat == e_dcat && scat == e_scat )
      {
	total_count += count;
	total_strength += strength;
	if( DEBUG6 )
	  DebugEdgeForTabu(e2, m, stderr);
      }
      e2 = prev_edge_on_path(e2);
    }

    /* if total effect is 0, we're safe */
    if( total_count <= 0 )
    {
      if( DEBUG6 )
	fprintf(stderr, " = false ]\n");
      return false;
    }

    /* if 2 or more and non-zero strength, we're tabu already */
    if( total_count >= 2 && total_strength > 0 )
    {
      if( DEBUG6 )
	fprintf(stderr, " = true ]\n");
      return true;
    }

    /* otherwise have to check for existing clashing assignments */
    cv = e->beginpoint->category_next;
    while( cv != e->beginpoint )
    {
      if( DEBUG6 )
	fprintf(stderr, ".");
      if( total_strength + cv->category_strength > 0 )
      {
	if( cv->fixed_to != NULL )
	{
	  /* cv is the beginpoint of a fixed edge from an e_dcat */
	  if( cv->fixed_to->category == e_scat )
	  {
	    if( DEBUG6 )
	    {
	      fprintf(stderr, " = true (fixed ");
	      NodeDebug(cv, stderr);
	      fprintf(stderr, ") ]\n");
	    }
	    return true;
	  }
	}
	else if( cv->node_flow > 0 )
	{
	  /* find the edge carrying flow out of cv */
	  EdgeListForEach(cv->edge_list, e2)
	    if( EdgeIsForward(e2) && e2->flow > 0 )
	      break;
	  HnAssert(e2 != NULL, "EdgeIsTabu internal error");

	  /* if this edge takes flow to an e_scat, that's bad unless it   */
	  /* goes to e's endpoint, which would have to be deassigned next */
	  /* on the path if e was to be used                              */
	  if( e2->endpoint->category==e_scat && e2->endpoint!=e->endpoint )
	  {
	    if( DEBUG6 )
	      fprintf(stderr, " = true ]\n");
	    return true;
	  }
	}
      }
      cv = cv->category_next;
    }
    if( DEBUG6 )
      fprintf(stderr, " = false ]\n");
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  int64_t EdgeAdjustedCost(KHE_WMATCH_EDGE e, KHE_WMATCH m)                */
/*                                                                           */
/*  Return the adjusted cost of e.  Check that it is non-negative, unless    */
/*  categories are in use (these may lead to negative cost edges).           */
/*                                                                           */
/*****************************************************************************/

int64_t EdgeAdjustedCost(KHE_WMATCH_EDGE e, KHE_WMATCH m)
{
  int64_t res;
  res = e->cost + e->beginpoint->prev_true_distance -
    e->endpoint->prev_true_distance;
  if( !m->use_categories && res < 0 )
  {
    if( DEBUG4 )
    {
      fprintf(stderr, "EdgeAdjustedCost failing at ");
      EdgeFullDebug(e, stderr);
      fprintf(stderr, ":\n");
      KheWMatchDebug(m, 2, 2, stderr);
    }
    HnAbort("EdgeAdjustedCost internal error");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int64_t EdgeAdjustedCostUnchecked(KHE_WMATCH_EDGE e)                     */
/*                                                                           */
/*  Like EdgeAdjustedCost above, but without category checking.  Used in     */
/*  debug prints when we just want the number, right or wrong.               */
/*                                                                           */
/*****************************************************************************/

int64_t EdgeAdjustedCostUnchecked(KHE_WMATCH_EDGE e)
{
  return e->cost + e->beginpoint->prev_true_distance -
    e->endpoint->prev_true_distance;
}


/*****************************************************************************/
/*                                                                           */
/*  bool FindMinCostAugmentingPath(KHE_WMATCH m)                             */
/*                                                                           */
/*  If an augmenting path exists in f, find one of minimum cost and store    */
/*  it in the flow's parent_edge fields.                                     */
/*                                                                           */
/*  Implementation note.  This function is an implementation of Dijkstra's   */
/*  shortest path algorithm, with edge cost adjustment to avoid negative     */
/*  cost edges.  The current adjusted distance from the source vertex to     */
/*  any visited vertex v is stored in v->curr_adjusted_distance.             */
/*                                                                           */
/*  In addition to the usual kind, a second kind of edge cost adjustment     */
/*  is carried out here to discourage edges between categories that are      */
/*  already connected.  Paths containing such edges, called tabu edges,      */
/*  are penalized by an extra amount stored in m->tabu_cost, like this:      */
/*                                                                           */
/*      path_cost = v->curr_adjusted_distance + EdgeAdjustedCost(e, m);      */
/*      if( m->use_categories && EdgeIsForward(e) && EdgeIsTabu(e) )         */
/*        path_cost += m->category_cost;                                     */
/*                                                                           */
/*  Working out whether an edge is tabu is slow, so the code below is tuned  */
/*  so that the tabu edge calculation is avoided when there is no chance     */
/*  that the edge will enter the tree, even if it turns out to be not tabu.  */
/*                                                                           */
/*  This second kind of adjustment increases edge costs in one round, but    */
/*  can lead to negative edge costs in the next.  This is because the tabu   */
/*  penalty is not added permanently to the edge; it is retracted at the     */
/*  end of the round, in effect.  So this implementation includes a test,    */
/*  not usually required in Dijkistra's algorithm, to check whether w is     */
/*  still in the heap when visited.  If not, the edge leading to w has       */
/*  arrived too late and is ignored.  Clearly, when categories are in use    */
/*  this algorithm is not optimal in general.                                */
/*                                                                           */
/*****************************************************************************/

static bool FindMinCostAugmentingPath(KHE_WMATCH m)
{
  KHE_WMATCH_NODE v, w;  KHE_WMATCH_EDGE e;  int64_t path_cost;
  m->visit_num++;
  HeapClear(&m->heap);
  m->source->visit_num = m->visit_num;
  m->source->prev_true_distance = 0;
  m->source->curr_adjusted_distance = 0;
  m->source->parent_edge = NULL;
  HeapInsert(&m->heap, m->source);
  while( !HeapIsEmpty(&m->heap) )
  {
    v = HeapDeleteMin(&m->heap);
    EdgeListForEach(v->edge_list, e)
      if( EdgeAvailableExtraFlow(e) > 0 )
      {
	w = e->endpoint;
	path_cost = v->curr_adjusted_distance + EdgeAdjustedCost(e, m);
	if( w->visit_num < m->visit_num )
	{
	  w->visit_num = m->visit_num;
	  if( m->use_categories && EdgeIsForward(e) && EdgeIsTabu(e, m) )
	    path_cost += m->category_cost;
	  w->curr_adjusted_distance = path_cost;
	  w->parent_edge = e;
	  HeapInsert(&m->heap, w);
	}
	else if( w->heap_index != 0 && path_cost < w->curr_adjusted_distance )
	{
	  if( !m->use_categories || !EdgeIsForward(e) || !EdgeIsTabu(e, m) ||
	      (path_cost += m->category_cost) < w->curr_adjusted_distance )
	  {
	    w->curr_adjusted_distance = path_cost;
	    w->parent_edge = e;
	    HeapDecreaseKey(&m->heap, w);
	  }
	}
      }
  }
  return m->sink->visit_num == m->visit_num;
}

/* *** old version using Bellman-Ford
static bool FindMinCostAugmentingPath(KHE_WMATCH m)
{
  KHE_WMATCH_NODE v;  int i;  bool still_firing;

  ** initialize the node distances and parents **
  NodeListForEach(m->demand_list, v)
  {
    v->distance = INT_MAX / 2;
    v->parent_edge = NULL;
  }
  NodeListForEach(m->supply_list, v)
  {
    v->distance = INT_MAX / 2;
    v->parent_edge = NULL;
  }
  m->sink->distance = INT_MAX / 2;
  m->sink->parent_edge = NULL;
  m->source->distance = 0;

  ** run the main loop NB m->node_count no longer kept **
  still_firing = true;
  for( i = 0;  i <= m->node_count && still_firing;  i++ )
  {
    still_firing = false;
    Extend(m->source, &still_firing);
    NodeListForEach(m->demand_list, v)
      Extend(v, &still_firing);
    NodeListForEach(m->supply_list, v)
      Extend(v, &still_firing);
    Extend(m->sink, &still_firing);
  }
  assert(i <= m->node_count);
  return m->sink->parent_edge != NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void FindMinCostWMatch(KHE_WMATCH m, int indent)                         */
/*                                                                           */
/*  Run a min-cost max-flow algorithm.  Parameter indent is for debugging.   */
/*  The algorithm is the standard one of starting from an empty flow and     */
/*  repeatedly augmenting along a path of minimum cost.  Edge cost           */
/*  adjustment is used to ensure that edges have non-negative cost,          */
/*  permitting Dijkstra's algorithm to be used.                              */
/*                                                                           */
/*****************************************************************************/

static void FindMinCostWMatch(KHE_WMATCH m, int indent)
{
  int extra_flow;
  InitPrevTrueDistances(m);
  SetFlowToZero(m);
  m->total_flow = 0;
  m->curr_cost2 = 0;
  if( DEBUG1 && indent >= 0 )
  {
    fprintf(stderr, "%*s[ FindMinCostWMatch, initial state is:\n", indent, "");
    KheWMatchDebug(m, 2, indent + 2, stderr);
  }
  while( FindMinCostAugmentingPath(m) )
  {
    UpdatePrevTrueDistances(m);
    if( DEBUG4 )
      CheckDistances(m);
    AugmentPath(m, &extra_flow);
    m->total_flow += extra_flow;
    m->curr_cost2 += extra_flow * m->sink->prev_true_distance;
    if( DEBUG1 && indent >= 0 )
    {
      fprintf(stderr, "  after augment, state is:\n");
      KheWMatchDebug(m, 2, indent + 2, stderr);
    }
  }
  m->curr_cost1 = m->demand_count - m->total_flow;
  if( DEBUG1 && indent >= 0 )
    fprintf(stderr, "%*s] FindMinCostWMatch\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchMakeClean(KHE_WMATCH m)                                    */
/*                                                                           */
/*  Ensure that the matching is up to date.                                  */
/*                                                                           */
/*  If there is at least one dirty node, we need to run the edge function    */
/*  between all (demand node, supply node) pairs with at least one dirty     */
/*  element, then re-match.  If the matching itself is marked dirty, we      */
/*  need to re-match but not necessarily to run the edge function.           */
/*                                                                           */
/*  Equivalent sets of demand nodes are either all clean or all dirty,       */
/*  so the main algorithm handles one such set at a time.  If all are        */
/*  clean, they are rematched only with dirty supply nodes.  If all are      */
/*  dirty, they are rematched with all supply nodes.                         */
/*                                                                           */
/*****************************************************************************/
#define msec(tv) ((tv).tv_sec * 1000000 + (tv).tv_usec)

static void KheWMatchMakeClean(KHE_WMATCH m)
{
  /* struct timeval pre_tv, post_tv; */
  if( DEBUG5 )
    fprintf(stderr, "[ KheWMatchMakeClean(m)\n");
  if( m->is_dirty )
  {
    KheWMatchMakeNodesClean(m);
    /* ***
    if( DEBUG11 )
      gettimeofd ay(&pre_tv, NULL);
    *** */
    HaArrayClear(m->hall_sets);
    /* m->hall_sets = NULL; */
    FindMinCostWMatch(m, 2);
    /* MonitorDeclareClean((MONITOR) m); */
    m->is_dirty = false;
    /* ***
    if( DEBUG11 )
    {
      gettimeo fday(&post_tv, NULL);
      time_taken += msec(post_tv) - msec(pre_tv);
    }
    *** */
  }
  if( DEBUG5 )
  {
    fprintf(stderr, "] KheWMatchMakeClean returning, final m is:\n");
    KheWMatchDebug(m, 2, 2, stderr);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AugmentCycle(KHE_WMATCH_NODE from, int *extra_flow)                 */
/*                                                                           */
/*  Augment the cycle including node from, found by a previous successful    */
/*  call to any function which has set a chain of parent_edge fields from    */
/*  the from node back around to itself.  Also set *extra_flow to the amount */
/*  of extra flow introduced by this augment.                                */
/*                                                                           */
/*****************************************************************************/

/* ***
static void AugmentCycle(KHE_WMATCH_NODE from, int *extra_flow)
{
  KHE_WMATCH_NODE v;  KHE_WMATCH_EDGE e;

  ** do a first traversal to work out the amount of augmenting flow **
  v = parent(from, &e);
  *extra_flow = EdgeAvailableExtraFlow(e);
  while( v != NULL && v != from )
  {
    v = parent(v, &e);
    if( EdgeAvailableExtraFlow(e) < *extra_flow )
      *extra_flow = EdgeAvailableExtraFlow(e);
  }
  assert(v != NULL);
  assert(*extra_flow > 0);

  ** do a second traversal to insert this flow **
  v = parent(from, &e);
  e->flow += *extra_flow;
  e->opposite_edge->flow = - e->flow;
  while( v != from )
  {
    v = parent(v, &e);
    e->flow += *extra_flow;
    e->opposite_edge->flow = - e->flow;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool FindMinCostCycleFrom(KHE_WMATCH m, KHE_WMATCH_NODE from)            */
/*                                                                           */
/*  Find a simple cycle of minimum cost that includes from and that allows   */
/*  a non-zero amount of flow to be pushed around itself.  In a min-cost     */
/*  flow this cycle must have non-negative cost.  We intend to use it to     */
/*  find the second minimum-cost flow.                                       */
/*                                                                           */
/*  The algorithm is to make the successors of from into starting points     */
/*  and then find shortest paths from them using Bellman-Ford.  If there is  */
/*  a cycle, then at termination from->parent_edge will lead to it and       */
/*  from->distance will be its total cost.                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
bool FindMinCostCycleFrom(KHE_WMATCH m, KHE_WMATCH_NODE from)
{
  KHE_WMATCH_NODE v, v2;  KHE_WMATCH_EDGE e;  int i;  bool still_firing;

  ** initialize *all* node distances to infinite, and clear parents **
  NodeListForEach(m->demand_list, v)
  {
    v->distance = INT_MAX / 2;
    v->parent_edge = NULL;
  }
  NodeListForEach(m->supply_list, v)
  {
    v->distance = INT_MAX / 2;
    v->parent_edge = NULL;
  }
  m->source->distance = INT_MAX / 2;
  m->source->parent_edge = NULL;
  m->sink->distance = INT_MAX / 2;
  m->sink->parent_edge = NULL;

  ** initialize *successors* of from to their distance from from **
  EdgeListForEach(from->edge_list, e)
    if( EdgeAvailableExtraFlow(e) > 0 )
    {
      v2 = e->endpoint;
      v2->distance = e->cost;
      v2->parent_edge = e;
    }

  ** from itself is now a goal, rather than a start point; run the main loop **
  still_firing = true;
  for( i = 0;  i <= m->node_count && still_firing;  i++ )
  {
    still_firing = false;
    Extend(m->source, &still_firing);
    NodeListForEach(m->demand_list, v)
      Extend(v, &still_firing);
    NodeListForEach(m->supply_list, v)
      Extend(v, &still_firing);
    Extend(m->sink, &still_firing);
  }
  assert(i <= f->node_count);
  return from->parent_edge != NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool FindMinCostCycle(KHE_WMATCH m, KHE_WMATCH_NODE *best_from)          */
/*                                                                           */
/*  Find a simple cycle of minimum cost that includes at least one of the    */
/*  successors of m's source, and that allows a non-zero amount of flow      */
/*  to be pushed around itself.                                              */
/*                                                                           */
/*  If successful, true is returned and *best_from is set to one node on     */
/*  the cycle.  The cycle is reached as (*best_from)->parent_edge etc.       */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool FindMinCostCycle(KHE_WMATCH m, KHE_WMATCH_NODE *best_from)
{
  KHE_WMATCH_NODE from;  KHE_WMATCH_EDGE e;  int best_distance;  bool res;
  if( DEBUG3 )
    fprintf(stderr, "[ FindMinCostCycle(f)\n");
  *best_from = NULL;
  EdgeListForEach(m->source->edge_list, e)
  {
    from = e->endpoint;
    if( FindMinCostCycleFrom(m, from) )
    {
      if( *best_from == NULL || from->distance < best_distance )
      {
	*best_from = from;
	best_distance = from->distance;
      }
    }
  }
  res = *best_from != NULL && FindMinCostCycleFrom(m, *best_from);
  if( DEBUG3 )
    fprintf(stderr, "] FindMinCostCycle returning %s (best_from = %s)\n",
      bool(res), res ? UStringToUTF8((*best_from)->name) : "-");
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_WMATCH KheWMatchNodeWMatch(KHE_WMATCH_NODE node)                     */
/*                                                                           */
/*  Return the wmatch that node lies in.                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_WMATCH KheWMatchNodeWMatch(KHE_WMATCH_NODE node)
{
  return node->wmatch;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWMatchFullyMatchable(KHE_WMATCH m, int64_t *cost)                */
/*                                                                           */
/*  Return true if the maximum matching of m touches every demand node,      */
/*  and set *cost to the cost of that matching.                              */
/*                                                                           */
/*****************************************************************************/

bool KheWMatchFullyMatchable(KHE_WMATCH m, int64_t *cost, int indent)
{
  KheWMatchMakeClean(m);
  *cost = m->curr_cost2;
  /* ***
  if( *cost > 2000 )
    KheWMatchDebug(m, indent, stderr);
  *** */
  return m->total_flow == m->demand_count;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchEval(KHE_WMATCH m, int *cost1, int64_t *cost2)             */
/*                                                                           */
/*  Return the current cost1 (number of unmatchable nodes) and               */
/*  cost2 (cost of maximum matching) of m.                                   */
/*                                                                           */
/*****************************************************************************/

void KheWMatchEval(KHE_WMATCH m, int *cost1, int64_t *cost2)
{
  KheWMatchMakeClean(m);
  *cost1 = m->demand_count - m->total_flow;
  *cost2 = m->curr_cost2;
}


/*****************************************************************************/
/*                                                                           */
/*  int64_t KheWMatchDemandNodeAverageEdgeCost(KHE_WMATCH_NODE demand_node)  */
/*                                                                           */
/*  Return the average cost of the edges touching demand_node, or 0 if none. */
/*                                                                           */
/*****************************************************************************/

int64_t KheWMatchDemandNodeAverageEdgeCost(KHE_WMATCH_NODE demand_node)
{
  int64_t res;  int count;  KHE_WMATCH_EDGE e;
  if( demand_node->wmatch->is_dirty )
    KheWMatchMakeClean(demand_node->wmatch);
  res = count = 0;
  EdgeListForEach(demand_node->edge_list, e)
    if( EdgeIsForward(e) )
      count++, res += e->cost;
  return count == 0 ? 0 : res/count;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheWMatchDemandNodeAssignedTo(KHE_WMATCH_NODE demand_node)         */
/*                                                                           */
/*  Find any node that receives a non-zero amount of flow directly from      */
/*  demand_node and return its original; or return NULL if no such node.     */
/*                                                                           */
/*  Alternatively, if demand_node is fixed to some supply node, return       */
/*  that node.                                                               */
/*                                                                           */
/*****************************************************************************/

void *KheWMatchDemandNodeAssignedTo(KHE_WMATCH_NODE demand_node, int64_t *cost)
{
  KHE_WMATCH_EDGE edge;
  if( demand_node->fixed_to != NULL )
  {
    *cost = 0;
    return demand_node->fixed_to->back;
  }
  KheWMatchMakeClean(demand_node->wmatch);
  EdgeListForEach(demand_node->edge_list, edge)
    if( edge->flow > 0 )
    {
      if( DEBUG9 )
      {
	fprintf(stderr, "KheWMatchDemandNodeAssignedTo(");
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
    fprintf(stderr, "KheWMatchDemandNodeAssignedTo(");
    NodeDebug(demand_node, stderr);
    fprintf(stderr, ") returning NULL:\n");
    KheWMatchDebug(demand_node->wmatch, 2, 2, stderr);
    /* KheWMatchDebugEdges(demand_node->wmatch, 2, stderr); */
  }
  return NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "special mode"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchSpecialModeBegin(KHE_WMATCH m)                             */
/*                                                                           */
/*  Begin special mode.  This includes bringing the nodes up to date.        */
/*                                                                           */
/*****************************************************************************/

void KheWMatchSpecialModeBegin(KHE_WMATCH m)
{
  HnAssert(!m->special_mode,
    "KheWMatchSpecialModeBegin: already in special mode");
  KheWMatchMakeNodesClean(m);
  m->special_mode = true;
  if( DEBUG10 )
  {
    fprintf(stderr, "[ KheWMatchSpecialModeBegin(m):\n");
    KheWMatchDebug(m, 2, 2, stderr);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchSpecialModeEnd(KHE_WMATCH m)                               */
/*                                                                           */
/*  End special mode.  This includes bringing the nodes up to date.          */
/*                                                                           */
/*****************************************************************************/

void KheWMatchSpecialModeEnd(KHE_WMATCH m)
{
  if( DEBUG10 )
  {
    fprintf(stderr, "] KheWMatchSpecialModeEnd(m):\n");
    KheWMatchDebug(m, 2, 2, stderr);
  }
  HnAssert(m->special_mode, "KheWMatchSpecialModeEnd: not in special mode");
  KheWMatchMakeNodesClean(m);
  m->special_mode = false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "forced assignments"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchForcedAsstTestBegin(KHE_WMATCH m)                          */
/*                                                                           */
/*  Begin a test for whether the current assignments to a set of demand      */
/*  nodes are forced or not.                                                 */
/*                                                                           */
/*****************************************************************************/

void KheWMatchForcedAsstTestBegin(KHE_WMATCH m)
{
  if( DEBUG7 )
  {
    fprintf(stderr, "[ KheWMatchForcedAsstTestBegin(");
    m->wmatch_back_debug(m->back, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }
  HnAssert(!m->forced_test_active,
    "KheWMatchForcedAsstTestBegin: already begun");
  m->forced_test_active = true;
  HaArrayClear(m->forced_test_nodes);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchForcedAsstTestAddNode(KHE_WMATCH m,                        */
/*    KHE_WMATCH_NODE demand_node)                                           */
/*                                                                           */
/*  Add demand_node to the set of nodes for which we are currently testing   */
/*  for forced assignments.                                                  */
/*                                                                           */
/*****************************************************************************/

void KheWMatchForcedAsstTestAddNode(KHE_WMATCH m, KHE_WMATCH_NODE demand_node)
{
  if( DEBUG7 )
  {
    fprintf(stderr, "  KheWMatchForcedAsstTestAddNode(");
    m->wmatch_back_debug(m->back, 1, -1, stderr);
    fprintf(stderr, ", ");
    NodeDebug(demand_node, stderr);
    fprintf(stderr, ")\n");
  }
  HnAssert(m->forced_test_active,
    "KheWMatchForcedAsstTestAddNode: forced test not begun");
  HaArrayAddLast(m->forced_test_nodes, demand_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchForcedAsstTestCleanup(KHE_WMATCH m)                        */
/*                                                                           */
/*  After completing a force assignment test, successful or otherwise,       */
/*  this code cleans up.                                                     */
/*                                                                           */
/*****************************************************************************/

static void KheWMatchForcedAsstTestCleanup(KHE_WMATCH m)
{
  KHE_WMATCH_NODE demand_node;  KHE_WMATCH_EDGE e;  int i;
  HaArrayForEach(m->forced_test_nodes, demand_node, i)
  {
    demand_node->forced_test_weight = 0;
    EdgeListForEach(demand_node->edge_list, e)
      if( EdgeIsForward(e) && e->flow > 0 )
	e->endpoint->forced_test_weight = 0;
  }
  HaArrayClear(m->forced_test_nodes);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWMatchForcedAsstTestEnd(KHE_WMATCH m)                            */
/*                                                                           */
/*  End a test for forced assignments, returning true if the nodes are       */
/*  indeed forced.                                                           */
/*                                                                           */
/*  To save time, we do this inexactly; if true is returned the assignment   */
/*  is certainly forced, but if false is returned it is possible that the    */
/*  assignment is really forced but a complex rematch, for which we don't    */
/*  have time, is needed to prove it.                                        */
/*                                                                           */
/*****************************************************************************/

bool KheWMatchForcedAsstTestEnd(KHE_WMATCH m)
{
  KHE_WMATCH_NODE demand_node;  KHE_WMATCH_EDGE e;  bool has_asst;  int i;
  HnAssert(m->forced_test_active,
    "KheWMatchForcedAsstTestEnd: forced test not begun");
  m->forced_test_active = false;

  if( HaArrayCount(m->forced_test_nodes) == 0 )
  {
    /* if no nodes were inserted, we return false, meaning nothing to do */
    if( DEBUG7 )
      fprintf(stderr, "] KheWMatchForcedAsstTestEnd returning false (0)\n");
    return false;
  }
  else
  {
    /* mark all the supply nodes touched by the demand nodes, and */
    /* record in each demand node the cost of its outgoing edge;  */
    /* quit early if the node does not have an assignment at all  */
    KheWMatchMakeClean(m);
    HaArrayForEach(m->forced_test_nodes, demand_node, i)
    {
      if( DEBUG7 )
      {
	fprintf(stderr, "  from ");
	NodeDebug(demand_node, stderr);
	fprintf(stderr, "\n");
      }
      has_asst = false;
      EdgeListForEach(demand_node->edge_list, e)
	if( EdgeIsForward(e) && e->flow > 0 )
	{
	  if( DEBUG7 )
	  {
	    fprintf(stderr, "    mark ");
	    NodeDebug(e->endpoint, stderr);
	    fprintf(stderr, "\n");
	  }
	  e->endpoint->forced_test_weight = 1;
	  demand_node->forced_test_weight = e->cost;
	  has_asst = true;
	}
      if( !has_asst )
      {
        KheWMatchForcedAsstTestCleanup(m);
	if( DEBUG7 )
	  fprintf(stderr, "] KheWMatchForcedAsstTestEnd returning false (1)\n");
	return false;
      }
    }

    /* search for a cheap edge pointing out of the current assts */
    HaArrayForEach(m->forced_test_nodes, demand_node, i)
      EdgeListForEach(demand_node->edge_list, e)
	if( EdgeIsForward(e) && e->endpoint->forced_test_weight == 0
	    && e->cost <= demand_node->forced_test_weight )
	{
	  if( DEBUG7 )
	  {
	    fprintf(stderr, "  outgoing edge ");
	    EdgeFullDebug(e, stderr);
	    fprintf(stderr, "\n");
	    fprintf(stderr, "] KheWMatchForcedAsstTestEnd returning false\n");
	  }
	  KheWMatchForcedAsstTestCleanup(m);
	  return false;
	}

    /* no cheap edges point outside the current assts, so must be forced */
    KheWMatchForcedAsstTestCleanup(m);
    if( DEBUG7 )
      fprintf(stderr, "] KheWMatchForcedAsstTestEnd returning true\n");
    return true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Hall sets"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void HallSetFree(HALL_SET hs)                                            */
/*                                                                           */
/*  Free hs.                                                                 */
/*                                                                           */
/*****************************************************************************/

static void HallSetFree(HALL_SET hs, KHE_WMATCH m)
{
  hs->parent_set = m->hall_set_free_list;
  m->hall_set_free_list = hs;
}


/*****************************************************************************/
/*                                                                           */
/*  HALL_SET HallSetMake()                                                   */
/*                                                                           */
/*  Return a new, empty Hall set, not yet loaded into m.                     */
/*                                                                           */
/*  When there is no parent set, the easiest arrangement is to let the       */
/*  parent_set field point to self rather than be NULL.                      */
/*                                                                           */
/*****************************************************************************/

static HALL_SET HallSetMake(KHE_WMATCH m)
{
  HALL_SET res;
  if( m->hall_set_free_list != NULL )
  {
    res = m->hall_set_free_list;
    m->hall_set_free_list = res->parent_set;
    HaArrayClear(res->demand_originals);
    HaArrayClear(res->supply_originals);
  }
  else
  {
    HaMake(res, m->arena);
    HaArrayInit(res->demand_originals, m->arena);
    HaArrayInit(res->supply_originals, m->arena);
  }
  res->parent_set = res;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  HALL_SET HallSetRoot(HALL_SET hs)                                        */
/*                                                                           */
/*  Return the root hall set of hs.                                          */
/*                                                                           */
/*****************************************************************************/

static HALL_SET HallSetRoot(HALL_SET hs)
{
  HnAssert(hs != NULL, "HallSetRoot internal error");
  while( hs->parent_set != hs )
    hs = hs->parent_set;
  return hs;
}


/*****************************************************************************/
/*                                                                           */
/*  void HallSetCompressPath(HALL_SET *hall_set)                             */
/*                                                                           */
/*  Compress the path from *hall_set to its root Hall set.                   */
/*                                                                           */
/*****************************************************************************/

static void HallSetCompressPath(HALL_SET *hall_set)
{
  HALL_SET hs, hs_parent, root;
  root = HallSetRoot(*hall_set);
  hs = *hall_set;
  *hall_set = root;
  while( hs->parent_set != root )
  {
    hs_parent = hs->parent_set;
    hs->parent_set = root;
    hs = hs_parent;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void HallSetUnion(HALL_SET root, HALL_SET *hall_set)                     */
/*                                                                           */
/*  Union *hall_set into root.                                               */
/*                                                                           */
/*****************************************************************************/

static void HallSetUnion(HALL_SET root, HALL_SET *hall_set)
{
  HALL_SET other_root;
  other_root = HallSetRoot(*hall_set);
  other_root->parent_set = root;
  HallSetCompressPath(hall_set);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_WMATCH_NODE DemandNodeAssignedFrom(KHE_WMATCH_NODE supply_node)      */
/*                                                                           */
/*  Return the demand node that supply_node is supplied from, or NULL        */
/*  if none.                                                                 */
/*                                                                           */
/*****************************************************************************/

static KHE_WMATCH_NODE DemandNodeAssignedFrom(KHE_WMATCH_NODE supply_node)
{
  KHE_WMATCH_EDGE e;
  EdgeListForEach(supply_node->edge_list, e)
    if( !EdgeIsForward(e) && e->flow < 0 )
      return e->endpoint;
  return NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_WMATCH_NODE SupplyNodeNodeAssignedTo(KHE_WMATCH_NODE demand_node)    */
/*                                                                           */
/*  Return the demand node that supply_node is supplied from, or NULL        */
/*  if none.                                                                 */
/*                                                                           */
/*****************************************************************************/

static KHE_WMATCH_NODE SupplyNodeNodeAssignedTo(KHE_WMATCH_NODE demand_node)
{
  KHE_WMATCH_EDGE e;
  EdgeListForEach(demand_node->edge_list, e)
    if( EdgeIsForward(e) && e->flow > 0 )
      return e->endpoint;
  return NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void HallSetAssign(BMATCH m, KHE_WMATCH_NODE demand_node, HALL_SET root) */
/*                                                                           */
/*  Explore the matching graph from demand_node, exactly as Assign()         */
/*  would do, and add all the nodes encountered to root, either directly     */
/*  or by changing the root of their own Hall set to point to this one.      */
/*                                                                           */
/*  This traversal goes exactly where Assign() goes, and since it starts     */
/*  at an unmatched demand node at a time when the matching is maximal, it   */
/*  can never encounter an unassigned supply node.  This is checked.         */
/*                                                                           */
/*  Instead of visit_num, this code uses the presence of a non-NULL          */
/*  Hall set in a node (demand or supply) to determine whether the node      */
/*  has been visited.  If there is a Hall set, the search can terminate,     */
/*  although the Hall set does need to be unioned with root.                 */
/*                                                                           */
/*****************************************************************************/

static void HallSetAssign(KHE_WMATCH m, KHE_WMATCH_NODE demand_node,
  HALL_SET root)
{
  KHE_WMATCH_EDGE e;  KHE_WMATCH_NODE supply_node, d;
  if( TIDY_HALL_SETS && demand_node->state == NODE_FIXED )
  {
    /* ignore this node, it's fixed so basically not here at all */
  }
  else if( demand_node->hall_set != NULL )
  {
    /* demand_node already visited; just make sure Hall sets are unioned */
    if( demand_node->hall_set != root )
      HallSetUnion(root, &demand_node->hall_set);
  }
  else
  {
    /* add demand_node to root and recurse to all supply nodes */
    demand_node->hall_set = root;
    EdgeListForEach(demand_node->edge_list, e)
      if( EdgeIsForward(e) )
      {
	supply_node = e->endpoint;
	if( TIDY_HALL_SETS && supply_node->state == NODE_FIXED )
	{
	  /* ignore this node, it's fixed so basically not here at all */
	}
	else if( supply_node->hall_set != NULL )
	{
	  /* supply_node already visited; make sure Hall sets are unioned */
	  if( supply_node->hall_set != root )
	    HallSetUnion(root, &supply_node->hall_set);
	}
	else
	{
	  /* add supply_node to root and recurse to assigned demand node */
	  supply_node->hall_set = root;
          d = DemandNodeAssignedFrom(supply_node);
	  HnAssert(d != NULL, "HallSetAssign internal error");
	  HallSetAssign(m, d, root);
	}
      }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void BuildHallSets(KHE_WMATCH m)                                         */
/*                                                                           */
/*  Build the Hall sets.                                                     */
/*                                                                           */
/*****************************************************************************/

static void BuildHallSets(KHE_WMATCH m)
{
  HALL_SET hall_set;  KHE_WMATCH_NODE d, s;  int i;
  static ARRAY_HALL_SET all_hall_sets;

  /* clear out the Hall set fields */
  NodeListForEach(m->demand_list, d)
    d->hall_set = NULL;
  NodeListForEach(m->supply_list, s)
    s->hall_set = NULL;
  HaArrayClear(m->hall_sets);

  /* assign nodes to Hall sets (including merging overlapping sets) */
  HaArrayInit(all_hall_sets, m->arena);
  NodeListForEach(m->demand_list, d)
    if( SupplyNodeNodeAssignedTo(d) == NULL && d->hall_set == NULL )
    {
      hall_set = HallSetMake(m);
      HaArrayAddLast(all_hall_sets, hall_set);
      HallSetAssign(m, d, hall_set);
    }

  /* compress the path at each node, and add each node to its root set */
  NodeListForEach(m->demand_list, d)
    if( d->hall_set != NULL )
    {
      HallSetCompressPath(&d->hall_set);
      HaArrayAddLast(d->hall_set->demand_originals, d->back);
    }
  NodeListForEach(m->supply_list, s)
    if( s->hall_set != NULL )
    {
      HallSetCompressPath(&s->hall_set);
      HaArrayAddLast(s->hall_set->supply_originals, s->back);
    }

  /* separate the active Hall sets from the bypassed ones */
  HaArrayForEach(all_hall_sets, hall_set, i)
    if( hall_set->parent_set == hall_set )
    {
      HnAssert(HaArrayCount(hall_set->demand_originals) > 0,
	"BuildHallSets internal error 1");
      HaArrayAddLast(m->hall_sets, hall_set);
    }
    else
    {
      HnAssert(HaArrayCount(hall_set->demand_originals) == 0,
	"BuildHallSets internal error 2");
      HnAssert(HaArrayCount(hall_set->supply_originals) == 0,
	"BuildHallSets internal error 3");
      HallSetFree(hall_set, m);
    }
  HaArrayFree(all_hall_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheWMatchHallSetCount(KHE_WMATCH m)                                  */
/*                                                                           */
/*  Return the number of Hall sets in m.                                     */
/*                                                                           */
/*****************************************************************************/

int KheWMatchHallSetCount(KHE_WMATCH m)
{
  KheWMatchMakeClean(m);
  if( HaArrayCount(m->hall_sets) == 0 )
    BuildHallSets(m);
  return HaArrayCount(m->hall_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheWMatchHallSetDemandNodeCount(KHE_WMATCH m, int i)                 */
/*                                                                           */
/*  Return the number of demand nodes in the i'th Hall set of m.             */
/*  This assumes that KheWMatchHallSetCount has been called recently.        */
/*                                                                           */
/*****************************************************************************/

int KheWMatchHallSetDemandNodeCount(KHE_WMATCH m, int i)
{
  HALL_SET hs;
  hs = HaArray(m->hall_sets, i);
  return HaArrayCount(hs->demand_originals);
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheWMatchHallSetDemandNode(KHE_WMATCH m, int i, int j)             */
/*                                                                           */
/*  Return the j'th demand node of the i'th Hall set of m.                   */
/*  This assumes that KheWMatchHallSetCount has been called recently.        */
/*                                                                           */
/*****************************************************************************/

void *KheWMatchHallSetDemandNode(KHE_WMATCH m, int i, int j)
{
  HALL_SET hs;
  hs = HaArray(m->hall_sets, i);
  return HaArray(hs->demand_originals, j);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheWMatchHallSetSupplyNodeCount(KHE_WMATCH m, int i)                 */
/*                                                                           */
/*  Return the number of demand nodes in the i'th Hall set of m.             */
/*  This assumes that KheWMatchHallSetCount has been called recently.        */
/*                                                                           */
/*****************************************************************************/

int KheWMatchHallSetSupplyNodeCount(KHE_WMATCH m, int i)
{
  HALL_SET hs;
  hs = HaArray(m->hall_sets, i);
  return HaArrayCount(hs->supply_originals);
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheWMatchHallSetSupplyNode(KHE_WMATCH m, int i, int j)             */
/*                                                                           */
/*  Return the j'th supply node of the i'th Hall set of m.                   */
/*  This assumes that KheWMatchHallSetCount has been called recently.        */
/*                                                                           */
/*****************************************************************************/

void *KheWMatchHallSetSupplyNode(KHE_WMATCH m, int i, int j)
{
  HALL_SET hs;
  hs = HaArray(m->hall_sets, i);
  return HaArray(hs->supply_originals, j);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchHallSetDebug(KHE_WMATCH m, int verbosity,                  */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of Hall sets of m.                                           */
/*                                                                           */
/*****************************************************************************/

void KheWMatchHallSetDebug(KHE_WMATCH m, int verbosity,
  int indent, FILE *fp)
{
  int i, j;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ Hall Sets (%d)\n", indent, "", KheWMatchHallSetCount(m));
    for( i = 0;  i < KheWMatchHallSetCount(m);  i++ )
    {
      for( j = 0;  j < KheWMatchHallSetDemandNodeCount(m, i);  j++ )
      {
	fprintf(fp, "%*s  %dD ", indent, "", i + 1);
	m->demand_back_debug(KheWMatchHallSetDemandNode(m, i, j), 1, 0, fp);
      }
      for( j = 0;  j < KheWMatchHallSetSupplyNodeCount(m, i);  j++ )
      {
	fprintf(fp, "%*s  %dS ", indent, "", i + 1);
	m->supply_back_debug(KheWMatchHallSetSupplyNode(m, i, j), 1, 0, fp);
      }
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}


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
/*  KHE_WMATCH KheWMatchMake(void *back, GENERIC_DEBUG_FN back_debug,        */
/*    GENERIC_DEBUG_FN demand_back_debug, GENERIC_DEBUG_FN supply_back_debug,*/
/*    KHE_WMATCH_EDGE_FN edge_fn, COST_DEBUG_FN cost_debug,int64_t tabu_cost,*/
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Return a new wmatch with just the source and sink nodes, not connected.  */
/*                                                                           */
/*****************************************************************************/

KHE_WMATCH KheWMatchMake(void *back, GENERIC_DEBUG_FN back_debug,
  GENERIC_DEBUG_FN demand_back_debug, GENERIC_DEBUG_FN supply_back_debug,
  KHE_WMATCH_EDGE_FN edge_fn, COST_DEBUG_FN cost_debug, int64_t tabu_cost,
  HA_ARENA a)
{
  KHE_WMATCH res;  /* HA_ARENA a; */

  /* get a new wmatch record and initialize its fields */
  /* a = HaAren aMake(); */
  HaMake(res, a);
  res->arena = a;
  res->back = back;
  res->wmatch_back_debug =
    (back_debug == NULL ? &DefaultDebugFn : back_debug);
  res->demand_back_debug =
    (demand_back_debug == NULL ? &DefaultDebugFn : demand_back_debug);
  res->supply_back_debug =
    (supply_back_debug == NULL ? &DefaultDebugFn : supply_back_debug);
  res->is_dirty = false;
  res->curr_cost1 = 0;
  res->curr_cost2 = 0;

  res->edge_fn = edge_fn;
  res->cost_debug = cost_debug;
  res->special_mode = false;
  res->use_categories = tabu_cost > 0;
  res->category_cost = tabu_cost;
  res->category_list = NULL;
  res->next_category_index = 0;
  res->visit_num = 0;
  res->demand_count = 0;
  res->demand_list = NULL;
  res->supply_count = 0;
  res->supply_list = NULL;
  res->total_flow = 0;
  res->dirty_node_count = 0;
  HaArrayInit(res->heap, a);
  res->forced_test_active = false;
  HaArrayInit(res->forced_test_nodes, a);
  HaArrayInit(res->hall_sets, a);
  res->node_free_list = NULL;
  res->edge_free_list = NULL;
  res->category_free_list = NULL;
  res->hall_set_free_list = NULL;

  /* initialize source and sink nodes */
  res->source = NodeMake(res, NULL, NULL, 0, NODE_SOURCE, NODE_CLEAN);
  res->sink = NodeMake(res, NULL, NULL, 0, NODE_SINK, NODE_CLEAN);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchClear(KHE_WMATCH m)                                        */
/*                                                                           */
/*  Clear out m, ready for a fresh start.  But save its free lists!          */
/*                                                                           */
/*****************************************************************************/

void KheWMatchClear(KHE_WMATCH m)
{
  KHE_WMATCH_NODE v;

  /* free the nodes and the edges out of them */
  EdgeListFree(m->source->edge_list);
  NodeFree(m->source);
  EdgeListFree(m->sink->edge_list);
  NodeFree(m->sink);
  NodeListForEach(m->demand_list, v)
    EdgeListFree(v->edge_list);
  NodeListFree(m->demand_list);
  NodeListForEach(m->supply_list, v)
    EdgeListFree(v->edge_list);
  NodeListFree(m->supply_list);

  /* free the categories */
  CategoryListFree(m->category_list);

  /* reset m's various fields */
  m->is_dirty = false;
  m->curr_cost1 = 0;
  m->curr_cost2 = 0;

  m->special_mode = false;
  m->category_list = NULL;
  m->next_category_index = 0;
  m->visit_num = 0;
  m->demand_count = 0;
  m->demand_list = NULL;
  m->supply_count = 0;
  m->supply_list = NULL;
  m->total_flow = 0;
  m->dirty_node_count = 0;
  HaArrayClear(m->heap);
  m->forced_test_active = false;
  HaArrayClear(m->forced_test_nodes);
  HaArrayClear(m->hall_sets);

  /* initialize source and sink nodes */
  m->source = NodeMake(m, NULL, NULL, 0, NODE_SOURCE, NODE_CLEAN);
  m->sink = NodeMake(m, NULL, NULL, 0, NODE_SINK, NODE_CLEAN);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEdgeFreeListRelease(KHE_WMATCH m)                                */
/*                                                                           */
/*  Free the edge free list (really free it, back to malloc).                */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheEdgeFreeListRelease(KHE_WMATCH m)
{
  KHE_WMATCH_EDGE e, next_e;
  e = m->edge_free_list;
  while( e != NULL )
  {
    next_e = e->next_edge;
    MFree(e);
    e = next_e;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeFreeListRelease(KHE_WMATCH m)                                */
/*                                                                           */
/*  Free the node free list (really free it, back to malloc).                */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheNodeFreeListRelease(KHE_WMATCH m)
{
  KHE_WMATCH_NODE n, next_n;
  n = m->node_free_list;
  while( n != NULL )
  {
    next_n = n->next_node;
    MFree(n);
    n = next_n;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCategoryFreeListRelease(KHE_WMATCH m)                            */
/*                                                                           */
/*  Free the category free list (really free it, back to malloc).            */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheCategoryFreeListRelease(KHE_WMATCH m)
{
  KHE_WMATCH_CATEGORY c, next_c;
  c = m->category_free_list;
  while( c != NULL )
  {
    next_c = c->next_category;
    MFree(c);
    c = next_c;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheHallSetFreeListRelease(KHE_WMATCH m)                             */
/*                                                                           */
/*  Free the Hall set free list (really free it, back to malloc).            */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheHallSetFreeListRelease(KHE_WMATCH m)
{
  HALL_SET h, next_h;
  h = m->hall_set_free_list;
  while( h != NULL )
  {
    next_h = h->parent_set;
    MFree(h);
    h = next_h;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchDelete(KHE_WMATCH m)                                       */
/*                                                                           */
/*  Get rid of m, including all its nodes and edges; recycle its memory.     */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheWMatchDelete(KHE_WMATCH m)
{
  HaArenaDel ete(m->arena);
  ** ***
  KHE_WMATCH_NODE v;

  ** free the nodes and the edges out of them **
  EdgeListFree(m->source->edge_list);
  NodeFree(m->source);
  EdgeListFree(m->sink->edge_list);
  NodeFree(m->sink);
  NodeListForEach(m->demand_list, v)
    EdgeListFree(v->edge_list);
  NodeListFree(m->demand_list);
  NodeListForEach(m->supply_list, v)
    EdgeListFree(v->edge_list);
  NodeListFree(m->supply_list);

  ** free the categories **
  CategoryListFree(m->category_list);

  ** free the free lists **
  KheEdgeFreeListRelease(m);
  KheNodeFreeListRelease(m);
  KheCategoryFreeListRelease(m);
  KheHallSetFreeListRelease(m);

  ** free the matching record **
  MArrayFree(m->forced_test_nodes);
  MArrayFree(m->hall_sets);
  MFree(m);
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWMatchUseCategories(KHE_WMATCH m)                                */
/*                                                                           */
/*  Return true if categories are in use in m.                               */
/*                                                                           */
/*****************************************************************************/

bool KheWMatchUseCategories(KHE_WMATCH m)
{
  return m->use_categories;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debugging and testing".                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void BFSPathDebug(KHE_WMATCH m, KHE_WMATCH_NODE v, FILE *fp)             */
/*                                                                           */
/*  Debug print of the BFS path to v.                                        */
/*                                                                           */
/*****************************************************************************/

/*  ***
static void BFSPathDebug(KHE_WMATCH m, KHE_WMATCH_NODE v, FILE *fp)
{
  KHE_WMATCH_EDGE junk;
  if( parent(v, &junk) != NULL )
  {
    v = parent(v, &junk);
    fprintf(fp, " (from %s", UStringToUTF8(v->name));
    v = parent(v, &junk);
    while( v != NULL )
    {
      fprintf(fp, ", from %s", UStringToUTF8(v->name));
      v = parent(v, &junk);
    }
    fprintf(fp, ")");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void CycleDebug(KHE_WMATCH m, KHE_WMATCH_NODE from, int indent, FILE *fp)*/
/*                                                                           */
/*  Debug print of cycle starting at from.                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static void CycleDebug(KHE_WMATCH m, KHE_WMATCH_NODE from, int indent, FILE *fp)
{
  KHE_WMATCH_NODE v, oldv;  KHE_WMATCH_EDGE e;
  v = parent(from, &e);
  fprintf(stderr, "%*s%s --(%d)--> %s\n", indent, "",
    UStringToUTF8(from->name), EdgeAvailableExtraFlow(e),
    UStringToUTF8(v->name));
  while( v != from )
  {
    oldv = v;
    v = parent(v, &e);
    fprintf(stderr, "%*s%s --(%d)--> %s\n", indent, "",
      UStringToUTF8(oldv->name), EdgeAvailableExtraFlow(e),
      UStringToUTF8(v->name));
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchDebugEdges(KHE_WMATCH m, int indent, FILE *fp)             */
/*                                                                           */
/*  Debug print explaining why the edges are as they are.                    */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheWMatchDebugEdges(KHE_WMATCH m, int indent, FILE *fp)
{
  KHE_WMATCH_NODE demand, supply;
  fprintf(fp, "%*s[ KheWMatchDebugEdges:\n", indent, "");
  NodeListForEach(m->demand_list, demand)
  {
    fprintf(fp, "%*s  %s:\n", indent, "", m->demand_show(demand->original));
    NodeListForEach(m->supply_list, supply)
      fprintf(fp, "%*s    %s: %s\n", indent, "",
	m->supply_show(supply->original),
	m->edge_show(demand->original, supply->original));
  }
  fprintf(fp, "%*s]\n", indent, "");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *NodeShow(KHE_WMATCH_NODE x, bool supply, KHE_WMATCH m)             */
/*                                                                           */
/*  Show one node of m.                                                      */
/*                                                                           */
/*****************************************************************************/

/* ***
static char *NodeShow(KHE_WMATCH_NODE x, bool supply, KHE_WMATCH m)
{
  static char buff[200];
  ** *** what is this?
  if( x->visit_num == m->visit_num )
    sprintf(buff, "[%dp%d --%d-> %s]", x->curr_adjusted_distance,
      x->prev_true_distance, x->node_flow,
      x == m->source ? "source" : x == m->sink ? "sink" :
      supply ? m->supply_show(x->original) : m->demand_show(x->original));
  else
    sprintf(buff, "[.p%d --%d-> %s]", x->prev_true_distance, x->node_flow,
      x == m->source ? "source" : x == m->sink ? "sink" :
      supply ? m->supply_show(x->original) : m->demand_show(x->original));
  *** **
  sprintf(buff, "%s", x == m->source ? "source" : x == m->sink ? "sink" :
    x->original == NULL ? "null" : supply ? m->supply_show(x->original) :
    m->demand_show(x->original));
  return buff;

}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *EdgeShow(KHE_WMATCH_EDGE e)                                        */
/*                                                                           */
/*  Show one edge.                                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
static char *EdgeShow(KHE_WMATCH_EDGE e)
{
  static char buff[200];
  sprintf(buff, "%c-%ld(%ld)->", e->flow > 0 ? '*' : ' ', e->cost,
    EdgeAdjustedCostUnchecked(e));
  return buff;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int NodeEquivCount(KHE_WMATCH_NODE node)                                 */
/*                                                                           */
/*  Return the number of nodes on node's equivalence list.                   */
/*                                                                           */
/*****************************************************************************/

static int NodeEquivCount(KHE_WMATCH_NODE node)
{
  KHE_WMATCH_NODE d;  int res;
  res = 1;
  ForEachOtherEquivalent(node, d)
    res++;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NodeEquivShow(KHE_WMATCH_NODE node)                                */
/*                                                                           */
/*  Show the equivalence list of node very briefly (just its length).        */
/*                                                                           */
/*****************************************************************************/

static void NodeEquivDebug(KHE_WMATCH_NODE node, FILE *fp)
{
  int len;
  len = NodeEquivCount(node);
  if( len != 1 )
    fprintf(fp, " E%d", len);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchDebug(KHE_WMATCH m, int indent, FILE *fp)                  */
/*                                                                           */
/*  Debug print of flow m onto file fp with the given indent, showing        */
/*  the nodes by name in breadth-first order and the flows and costs         */
/*  along their edges.                                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheWMatchDebug(KHE_WMATCH m, int indent, FILE *fp)
{
  KHE_WMATCH_NODE demand, supply;  KHE_WMATCH_EDGE de, se;
  int source_out_capacity, sink_in_capacity;

  ** work out total capacity out of source **
  source_out_capacity = 0;
  EdgeListForEach(m->source->edge_list, de)
  {
    HnAssert(de->capacity >= 0, "KheWMatchDebug internal error 1");
    source_out_capacity += de->capacity;
  }

  ** work out total capacity into sink **
  sink_in_capacity = 0;
  EdgeListForEach(m->sink->edge_list, se)
  {
    HnAssert(se->capacity <= 0, "KheWMatchDebug internal error 2");
    sink_in_capacity -= se->capacity;
  }

  ** header **
  fprintf(fp, "%*s[ KheWMatch%s ", indent, "", m->is_dirty ? " !DIRTY!" : "");
  m->wmatch_back_debug(m->back, 1, -1, fp);
  fprintf(fp, " (%s%s %d, %s %d, %si%ds%ld)\n",
    m->use_categories ? "with categories, " : "",
    "total demand", source_out_capacity, "total supply", sink_in_capacity,
    m->special_mode ? "special_mode " : "", m->curr_cost1, m->curr_cost2);

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
    CategoryDebug(demand, m, fp);
    NodeEquivDebug(demand, fp);
    fprintf(fp, " s%d:\n", EdgeListCount(demand->special_edge_list));
    EdgeListForEach(demand->edge_list, se)
    {
      supply = se->endpoint;
      if( supply != m->source )
      {
	fprintf(fp, "%*s      ", indent, "");
	EdgeDebug(se, fp);
	fprintf(fp, " ");
	NodeDebug(supply, fp);
	CategoryDebug(supply, m, fp);
	fprintf(fp, " s%d\n", EdgeListCount(supply->special_edge_list));
      }
    }
  }
  fprintf(fp, "%*s]\n", indent, "");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int CapacityThroughNode(KHE_WMATCH_NODE v)                               */
/*                                                                           */
/*  Return the capacity passing through v, being the minimum of the          */
/*  incoming capacity and the outgoing capacity.                             */
/*                                                                           */
/*****************************************************************************/

/* ***
int CapacityThroughNode(KHE_WMATCH_NODE v)
{
  KHE_WMATCH_EDGE e;
  int incap, outcap;
  incap = outcap = 0;
  EdgeListForEach(v->edge_list, e)
  {
    if( e->capacity > 0 )
      outcap += e->capacity;
    else
      incap += e->opposite_edge->capacity;
  }
  return min(incap, outcap);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheWMatchThroughNode(KHE_WMATCH_NODE v)                              */
/*                                                                           */
/*  Return the amount of flow passing through node v.                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheWMatchThroughNode(KHE_WMATCH_NODE v)
{
  KHE_WMATCH_EDGE e;
  int res = 0;
  EdgeListForEach(v->edge_list, e)
    if( e->flow > 0 )
      res += e->flow;
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchDebugMedium(KHE_WMATCH m, int indent, FILE *fp)            */
/*                                                                           */
/*  Like KheWMatchDebug but briefer, showing only the matching edges.        */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheWMatchDebugMedium(KHE_WMATCH m, int indent, FILE *fp)
{
  KHE_WMATCH_NODE demand, supply;  KHE_WMATCH_EDGE de, se;
  fprintf(fp, "%*s[ KheWMatch ", indent, "");
  m->wmatch_back_debug(m->back, 1, -1, fp);
  fprintf(fp, " (%si%ds%ld):\n",
    m->special_mode ? "special_mode " : "", m->curr_cost1, m->curr_cost2);
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
	fprintf(fp, " -%ld-> ", se->cost);
        NodeDebug(supply, fp);
	** ***
	fprintf(fp, " -%ld-> %s", se->cost,
	  m->supply_show(supply->original));
	*** **
      }
    }
    fprintf(fp, "\n");
  }
  fprintf(fp, "%*s]\n", indent, "");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchDebugBrief(KHE_WMATCH m, int indent, FILE *fp)             */
/*                                                                           */
/*  Like KheWMatchDebug but briefer, showing only the unmatched nodes.       */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheWMatchDebugBrief(KHE_WMATCH m, int indent, FILE *fp)
{
  KHE_WMATCH_NODE demand;  KHE_WMATCH_EDGE de, se;
  fprintf(fp, "%*s[ KheWMatch ", indent, "");
  m->wmatch_back_debug(m->back, 1, -1, fp);
  fprintf(fp, " (%si%ds%ld):\n",
    m->special_mode ? "special_mode " : "", m->curr_cost1, m->curr_cost2);
  EdgeListForEach(m->source->edge_list, de)
  {
    demand = de->endpoint;
    EdgeListForEach(demand->edge_list, se)
      if( se->flow > 0 && se->cost > 0 )
      {
	fprintf(fp, "%*s  ", indent, "");
	NodeDebug(demand, fp);
	fprintf(fp, "\n");
      }
  }
  fprintf(fp, "%*s]\n", indent, "");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWMatchDebug(KHE_WMATCH m, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug printf of m onto fp with the given verbosity and indent.           */
/*                                                                           */
/*    Verbosity 1: show infeasibility and cost                               */
/*    Verbosity 2: add display of matching demand nodes                      */
/*    Verbosity 3: add display of matching edges and supply nodes            */
/*    Verbosity 4: all kinds of extra stuff                                  */
/*                                                                           */
/*****************************************************************************/

void KheWMatchDebug(KHE_WMATCH m, int verbosity, int indent, FILE *fp)
{
  KHE_WMATCH_NODE demand, supply;  KHE_WMATCH_EDGE de, se;
  int source_out_capacity, sink_in_capacity;

  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ KheWMatch ");
    m->wmatch_back_debug(m->back, 1, -1, fp);
    fprintf(fp, " (%s%si%ds", m->is_dirty ? "DIRTY " : "",
      m->special_mode ? "special_mode " : "", m->curr_cost1);
    m->cost_debug(m->back, m->curr_cost2, stderr);
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
	  if( se->flow > 0 && se->cost > 0 )
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
	    m->cost_debug(m->back, se->cost, fp);
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
	HnAssert(de->capacity >= 0, "KheWMatchDebug internal error 1");
	source_out_capacity += de->capacity;
      }

      /* work out total capacity into sink */
      sink_in_capacity = 0;
      EdgeListForEach(m->sink->edge_list, se)
      {
	HnAssert(se->capacity <= 0, "KheWMatchDebug internal error 2");
	sink_in_capacity -= se->capacity;
      }

      /* header */
      fprintf(fp, ":\n");
      /* ***
      fprintf(fp, " (%s%s %d, %s %d, %si%ds%ld)\n",
	m->use_categories ? "with categories, " : "",
	"total demand", source_out_capacity, "total supply", sink_in_capacity,
	m->special_mode ? "special_mode " : "", m->curr_cost1, m->curr_cost2);
      *** */

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
	CategoryDebug(demand, m, fp);
	NodeEquivDebug(demand, fp);
	if( EdgeListCount(demand->special_edge_list) > 0 )
	  fprintf(fp, " s%d:\n", EdgeListCount(demand->special_edge_list));
	else
	  fprintf(fp, ":\n");
	EdgeListForEach(demand->edge_list, se)
	{
	  supply = se->endpoint;
	  if( supply != m->source )
	  {
	    fprintf(fp, "%*s      ", indent, "");
	    EdgeDebug(se, fp);
	    fprintf(fp, " ");
	    NodeDebug(supply, fp);
	    CategoryDebug(supply, m, fp);
	    if( EdgeListCount(supply->special_edge_list) > 0 )
	      fprintf(fp, " s%d\n", EdgeListCount(supply->special_edge_list));
	    else
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
/*  void KheWMatchTest(FILE *fp)                                             */
/*                                                                           */
/*  Test this module, reporting results to fp.                               */
/*                                                                           */
/*****************************************************************************/

typedef struct test_node {
  char *		name;		/* node name                         */
  KHE_WMATCH_NODE	node;		/* the node                          */
} *TEST_NODE;

static TEST_NODE MakeTestDemandNode(KHE_WMATCH m, char *name,
  KHE_WMATCH_CATEGORY category)
{
  TEST_NODE res;
  HaMake(res, m->arena);
  res->name = name;
  res->node = KheWMatchDemandNodeMake(m, (void *) res, category, 0);
  return res;
}

static TEST_NODE MakeTestSupplyNode(KHE_WMATCH m, char *name,
  KHE_WMATCH_CATEGORY category)
{
  TEST_NODE res;
  HaMake(res, m->arena);
  res->name = name;
  res->node = KheWMatchSupplyNodeMake(m, (void *) res, category);
  return res;
}

static void node_debug(void *node, int verbosity, int indent, FILE *fp)
{
  fprintf(stderr, "%s", ((TEST_NODE) node)->name);
}

static bool edge_fn(void *demand, void *supply, int64_t *cost)
{
  /* silly function which declares need for an edge when the two names */
  /* contain a character in common, and uses 0-1-2-0-1-2 etc. for cost */
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

static void cost_debug(void *back, int64_t cost, FILE *fp)
{
  fprintf(fp, "%ld", cost);
}


static void KheWMatchTest1(FILE *fp)
{
  HA_ARENA a;  KHE_WMATCH m;  TEST_NODE ta, tb /* , tc, td */;
  HA_ARENA_SET as;
  as = HaArenaSetMake();
  a = HaArenaMake(as);  /* this arena is used for testing only */
  m = KheWMatchMake(NULL, NULL, &node_debug, &node_debug, &edge_fn,
    &cost_debug, 10, a);
  ta = MakeTestDemandNode(m, "A1234", NULL);
  tb = MakeTestDemandNode(m, "B12", NULL);
  /* tc = MakeTestDemandNode(m, "C2", NULL); */
  /* td = MakeTestDemandNode(m, "D234", NULL); */

  MakeTestSupplyNode(m, "1", NULL);
  MakeTestSupplyNode(m, "2", NULL);
  MakeTestSupplyNode(m, "3", NULL);
  MakeTestSupplyNode(m, "4", NULL);

  KheWMatchMakeClean(m);
  fprintf(fp, "After solving:\n");
  KheWMatchDebug(m, 2, 0, fp);
  fprintf(fp, "In brief:\n");
  KheWMatchDebug(m, 1, 0, fp);
  fprintf(fp, "Forced asst test:\n");
  KheWMatchForcedAsstTestBegin(m);
  KheWMatchForcedAsstTestAddNode(m, ta->node);
  KheWMatchForcedAsstTestAddNode(m, tb->node);
  /* KheWMatchForcedAsstTestAddNode(m, tc->node); */
  /* KheWMatchForcedAsstTestAddNode(m, td->node); */
  KheWMatchForcedAsstTestEnd(m);
  HaArenaDelete(a);  /* this arena is used for testing only */
}


void KheWMatchTest2(FILE *fp)
{
  /* nothing just at present */
}


void KheWMatchTest(FILE *fp)
{
  KheWMatchTest1(fp);
  KheWMatchTest2(fp);
}


/*****************************************************************************/
/*                                                                           */
/*  int64_t KheWMatchTimeTaken(void)                                         */
/*                                                                           */
/*  Return the total time taken on bringing UBM graphs up to date, in        */
/*  microseconds.                                                            */
/*                                                                           */
/*****************************************************************************/

/* ***
int64_t KheWMatchTimeTaken(void)
{
  return time_taken;
}
*** */

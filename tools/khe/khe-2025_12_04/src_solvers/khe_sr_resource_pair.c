
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
/*  FILE:         khe_sr_resource_pair.c                                     */
/*  DESCRIPTION:  Resource pair resource repair                              */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "sset.h"
#include "khe_part_graph.h"
#include <limits.h>

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0

#define KHE_RPR_MAX_NODES 512

typedef enum {
  KHE_OPTIONS_RESOURCE_PAIR_NONE,
  KHE_OPTIONS_RESOURCE_PAIR_SPLITS,
  KHE_OPTIONS_RESOURCE_PAIR_PARTITIONS,
  KHE_OPTIONS_RESOURCE_PAIR_ALL
} KHE_OPTIONS_RESOURCE_PAIR;

typedef HA_ARRAY(KHE_RESOURCE) ARRAY_KHE_RESOURCE;
typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK;
typedef HA_ARRAY(KHE_EVENT_RESOURCE) ARRAY_KHE_EVENT_RESOURCE;
typedef HA_ARRAY(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR)
  ARRAY_KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR;

typedef struct khe_rpr_node_rec *KHE_RPR_NODE;
typedef HA_ARRAY(KHE_RPR_NODE) ARRAY_KHE_RPR_NODE;

typedef struct khe_rpr_component_rec *KHE_RPR_COMPONENT;
typedef HA_ARRAY(KHE_RPR_COMPONENT) ARRAY_KHE_RPR_COMPONENT;

typedef struct khe_rpr_graph_rec *KHE_RPR_GRAPH;
typedef HA_ARRAY(KHE_RPR_GRAPH) ARRAY_KHE_RPR_GRAPH;


/*****************************************************************************/
/*                                                                           */
/*  KHE_RPR_NODE - a node in the clash graph                                 */
/*                                                                           */
/*****************************************************************************/

struct khe_rpr_node_rec {
  KHE_RPR_GRAPH			graph;		/* enclosing graph           */
  KHE_RPR_COMPONENT		component;	/* enclosing comp (if any)   */
  ARRAY_KHE_TASK		tasks;		/* the tasks                 */
  ARRAY_KHE_EVENT_RESOURCE	event_resources; /* all event resources      */
  SSET				times;		/* their assigned times      */
  float				workload;	/* their total workload      */
  bool				assignable[2];	/* true if resource[i] ass.  */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_RPR_COMPONENT - a component of the clash graph                       */
/*                                                                           */
/*  If the component is partitionable, then after partitioning it, nodes     */
/*  will be empty and parts[0..1] will be non-NULL.                          */
/*                                                                           */
/*****************************************************************************/

struct khe_rpr_component_rec {
  KHE_RPR_GRAPH		graph;			/* enclosing graph           */
  KHE_RPR_NODE		parts[2];		/* the parts of component    */
  int			assign_count;		/* number of possible assts  */
  bool			assign_choice[2];	/* assign choices            */
  bool			rp_symmetrical;		/* true if rp-symmetrical    */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_RPR_GRAPH - a clash graph for resource repair                        */
/*                                                                           */
/*****************************************************************************/

struct khe_rpr_graph_rec {
  HA_ARENA		arena;
  KHE_SOLN		soln;			/* enclosing solution        */
  KHE_RESOURCE		resources[2];		/* the resources             */
  bool			fix_splits;		/* true if want this         */
  ARRAY_KHE_RPR_NODE	nodes;			/* nodes of the clash graph  */
  ARRAY_KHE_RPR_COMPONENT components;		/* components of clash graph */
  KHE_COST		init_cost;		/* initial soln cost         */
  int			init_defect_count;	/* initial soln defect count */

  /* used when searching */
  KHE_MARK		root_mark;		/* mark for root of tree     */
  /* KHE_COST		best_cost; */		/* best cost when searching  */
  int			node_count;		/* node count when searching */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "nodes"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheRprNodeAddInfo(KHE_RPR_NODE node, KHE_TASK task)                 */
/*                                                                           */
/*  Add info about task and its descendants to node.                         */
/*                                                                           */
/*****************************************************************************/

static void KheRprNodeAddInfo(KHE_RPR_NODE node, KHE_TASK task)
{
  KHE_MEET meet;  int index, durn, i, pos;  KHE_TASK child_task;
  KHE_EVENT_RESOURCE er;

  /* add event resource info about task */
  er = KheTaskEventResource(task);
  if( er != NULL && !HaArrayContains(node->event_resources, er, &pos) )
    HaArrayAddLast(node->event_resources, er);

  /* add assigned times info about task */
  meet = KheTaskMeet(task);
  if( meet != NULL && KheMeetAsstTime(meet) != NULL )
  {
    durn = KheMeetDuration(meet);
    index = KheTimeIndex(KheMeetAsstTime(meet));
    for( i = 0;  i < durn;  i++ )
      SSetInsert(node->times, index + i);
  }

  /* add workload info about task */
  node->workload += KheTaskWorkload(task);

  /* add info about the tasks assigned to task, directly and indirectly */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    KheRprNodeAddInfo(node, child_task);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RPR_NODE KheRprNodeMake(KHE_RPR_GRAPH graph, KHE_RPR_COMPONENT comp) */
/*                                                                           */
/*  Make an initially empty node of graph.                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_RPR_NODE KheRprNodeMake(KHE_RPR_GRAPH graph,
  KHE_RPR_COMPONENT comp)
{
  KHE_RPR_NODE res;
  HaMake(res, graph->arena);
  res->graph = graph;
  res->component = comp;
  HaArrayInit(res->tasks, graph->arena);
  HaArrayInit(res->event_resources, graph->arena);
  SSetInit(res->times, graph->arena);
  /* res->times = LSetNew(); */
  res->workload = 0.0;
  res->assignable[0] = res->assignable[1] = true;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskAssignable(KHE_TASK task, KHE_RESOURCE r)                    */
/*                                                                           */
/*  Return true if r is assignable to task, or is already assigned to it     */
/*  in the case of a fixed task.                                             */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskAssignable(KHE_TASK task, KHE_RESOURCE r)
{
  if( KheTaskAssignIsFixed(task) )
    return KheTaskAsstResource(task) == r;
  else
    return KheResourceGroupContains(KheTaskDomain(task), r);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRprNodeAddTask(KHE_RPR_NODE node, KHE_TASK task)                 */
/*                                                                           */
/*  Add task to node.                                                        */
/*                                                                           */
/*****************************************************************************/

static void KheRprNodeAddTask(KHE_RPR_NODE node, KHE_TASK task)
{
  HaArrayAddLast(node->tasks, task);
  KheRprNodeAddInfo(node, task);
  if( !KheTaskAssignable(task, node->graph->resources[0]) )
    node->assignable[0] = false;
  if( !KheTaskAssignable(task, node->graph->resources[1]) )
    node->assignable[1] = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRprNodeDelete(KHE_RPR_NODE node)                                 */
/*                                                                           */
/*  Delete node, freeing its memory.                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheRprNodeDelete(KHE_RPR_NODE node)
{
  MArrayFree(node->tasks);
  MArrayFree(node->event_resources);
  LSetFree(node->times);
  MFree(node);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheRprNodeMerge(KHE_RPR_NODE node1, KHE_RPR_NODE node2)             */
/*                                                                           */
/*  Merge node2 into node1.  Do not delete node2, that happens separately.   */
/*                                                                           */
/*  Implementation note.  The nodes' tasks must be disjoint, but their       */
/*  event resources need not be.                                             */
/*                                                                           */
/*****************************************************************************/

static void KheRprNodeMerge(KHE_RPR_NODE node1, KHE_RPR_NODE node2)
{
  int i, pos;  KHE_EVENT_RESOURCE er;
  HaArrayAppend(node1->tasks, node2->tasks, i);
  HaArrayForEach(node2->event_resources, er, i)
    if( !HaArrayContains(node1->event_resources, er, &pos) )
      HaArrayAddLast(node1->event_resources, er);
  SSetUnion(node1->times, node2->times);
  node1->workload += node2->workload;
  node1->assignable[0] = node1->assignable[0] && node2->assignable[0];
  node1->assignable[1] = node1->assignable[1] && node2->assignable[1];
  /* KheRprNodeDelete(node2); */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRprNodeAssignableSameWay(KHE_RPR_NODE node1, KHE_RPR_NODE node2) */
/*                                                                           */
/*  Return true if node1 and node2 are assignable in the same way.           */
/*                                                                           */
/*****************************************************************************/

static bool KheRprNodeAssignableSameWay(KHE_RPR_NODE node1, KHE_RPR_NODE node2)
{
  if( node1->assignable[0] && node2->assignable[0] )
    return true;
  if( node1->assignable[1] && node2->assignable[1] )
    return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRprNodeAssignableDifferentWay(KHE_RPR_NODE node1,                */
/*    KHE_RPR_NODE node2)                                                    */
/*                                                                           */
/*  Return true if node1 and node2 are assignable in a different way.        */
/*                                                                           */
/*****************************************************************************/

static bool KheRprNodeAssignableDifferentWay(KHE_RPR_NODE node1,
  KHE_RPR_NODE node2)
{
  if( node1->assignable[0] && node2->assignable[1] )
    return true;
  if( node1->assignable[1] && node2->assignable[0] )
    return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventResourcesShareAvoidSplitMonitor(KHE_SOLN soln,              */
/*    KHE_EVENT_RESOURCE er1, KHE_EVENT_RESOURCE er2)                        */
/*                                                                           */
/*  Return true if er1 and er2 share an avoid split assignments monitor of   */
/*  non-zero weight in soln.                                                 */
/*                                                                           */
/*****************************************************************************/

static bool KheEventResourcesShareAvoidSplitMonitor(KHE_SOLN soln,
  KHE_EVENT_RESOURCE er1, KHE_EVENT_RESOURCE er2)
{
  KHE_MONITOR m1, m2;  int i, j;
  for( i = 0;  i < KheSolnEventResourceMonitorCount(soln, er1);  i++ )
  {
    m1 = KheSolnEventResourceMonitor(soln, er1, i);
    if( KheMonitorTag(m1) == KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG &&
	KheConstraintWeight(KheMonitorConstraint(m1)) > 0 )
      for( j = 0;  j < KheSolnEventResourceMonitorCount(soln, er2);  j++ )
      {
	m2 = KheSolnEventResourceMonitor(soln, er2, j);
	if( m1 == m2 )
	  return true;
      }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRprNodesShareAvoidSplitMonitor(KHE_RPR_NODE node1,               */
/*    KHE_RPR_NODE node2)                                                    */
/*                                                                           */
/*  Return true if node1 and node2 contain event resources that share an     */
/*  avoid split assignments monitor of non-zero weight in soln.              */
/*                                                                           */
/*****************************************************************************/

static bool KheRprNodesShareAvoidSplitMonitor(KHE_RPR_NODE node1,
  KHE_RPR_NODE node2)
{
  int i1, i2;  KHE_EVENT_RESOURCE er1, er2;  KHE_SOLN soln;
  soln = node1->graph->soln;
  HaArrayForEach(node1->event_resources, er1, i1)
    HaArrayForEach(node2->event_resources, er2, i2)
      if( KheEventResourcesShareAvoidSplitMonitor(soln, er1, er2) )
	return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PART_GRAPH_REL KheRprFirstPartGraphRelFn(void *node1, void *node2)   */
/*                                                                           */
/*  Say whether node1 and node2 must be coloured the same colour, or         */
/*  different colours, or unconstrained.  This is the relation function      */
/*  for what the documentation calls the first partition graph.              */
/*                                                                           */
/*****************************************************************************/

static KHE_PART_GRAPH_REL KheRprFirstPartGraphRelFn(void *node1, void *node2)
{
  KHE_RPR_NODE n1 = (KHE_RPR_NODE) node1;
  KHE_RPR_NODE n2 = (KHE_RPR_NODE) node2;
  HnAssert(n1->assignable[0] || n1->assignable[1],
    "KheRprFirstPartGraphRelFn internal error 1");
  HnAssert(n2->assignable[0] || n2->assignable[1],
    "KheRprFirstPartGraphRelFn internal error 2");
  if( !KheRprNodeAssignableSameWay(n1, n2) )
    return KHE_PART_GRAPH_DIFFERENT;
  else if( !KheRprNodeAssignableDifferentWay(n1, n2) )
    return KHE_PART_GRAPH_SAME;
  else if( n1->graph->fix_splits && KheRprNodesShareAvoidSplitMonitor(n1, n2) )
    return KHE_PART_GRAPH_SAME;
  else if( !SSetDisjoint(n1->times, n2->times) )
    return KHE_PART_GRAPH_DIFFERENT;
  else
    return KHE_PART_GRAPH_UNRELATED;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PART_GRAPH_REL KheRprSecondPartGraphRelFn(void *node1, void *node2)  */
/*                                                                           */
/*  Say whether node1 and node2 must be coloured the same colour, or         */
/*  different colours, or unconstrained.  This is the relation function      */
/*  for what the documentation calls the second partition graph.             */
/*                                                                           */
/*****************************************************************************/

static KHE_PART_GRAPH_REL KheRprSecondPartGraphRelFn(void *node1, void *node2)
{
  KHE_RPR_NODE n1 = (KHE_RPR_NODE) node1;
  KHE_RPR_NODE n2 = (KHE_RPR_NODE) node2;
  if( n1->component == n2->component )
    return KHE_PART_GRAPH_DIFFERENT;
  else if( KheRprNodesShareAvoidSplitMonitor(n1, n2) )
    return KHE_PART_GRAPH_SAME;
  else
    return KHE_PART_GRAPH_UNRELATED;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRprNodeIsPSymmetrical(KHE_RPR_NODE node)                         */
/*                                                                           */
/*  Return true if node is p-symmetrical.                                    */
/*                                                                           */
/*****************************************************************************/

static bool KheRprNodeIsPSymmetrical(KHE_RPR_NODE node)
{
  KHE_EVENT_RESOURCE er;  int i, j;  KHE_CONSTRAINT c;
  KHE_PREFER_RESOURCES_CONSTRAINT prc;  KHE_RESOURCE_GROUP rg;
  KHE_RESOURCE r1, r2;
  r1 = node->graph->resources[0];
  r2 = node->graph->resources[1];
  HaArrayForEach(node->event_resources, er, i)
    for( j = 0;  j < KheEventResourceConstraintCount(er);  j++ )
    {
      c = KheEventResourceConstraint(er, j);
      if( c == NULL ||
	  KheConstraintTag(c) == KHE_PREFER_RESOURCES_CONSTRAINT_TAG )
      {
	if( c != NULL )
	{
	  prc = (KHE_PREFER_RESOURCES_CONSTRAINT) c;
	  rg = KhePreferResourcesConstraintDomain(prc);  /* NB prc != NULL */
	  if( KheResourceGroupContains(rg,r1)!=KheResourceGroupContains(rg,r2) )
	    return false;
	}
	else
	{
	  /* nothing to do here, rg is empty which is symmetrical */
	}
      }
    }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskEnsureAssigned(KHE_TASK task, KHE_RESOURCE r)                */
/*                                                                           */
/*  Ensure that task is assigned r.  The task may be fixed; if not, it is    */
/*  unassigned.  This operation is only attempted when it must succeed.      */
/*                                                                           */
/*****************************************************************************/

static void KheTaskEnsureAssigned(KHE_TASK task, KHE_RESOURCE r)
{
  if( !KheTaskAssignIsFixed(task) )
    KheTaskAssignResource(task, r);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskEnsureUnAssigned(KHE_TASK task)                              */
/*                                                                           */
/*  Ensure that task is unassigned, unless it is fixed.                      */
/*                                                                           */
/*****************************************************************************/

static void KheTaskEnsureUnAssigned(KHE_TASK task)
{
  if( DEBUG8 )
  {
    fprintf(stderr, "  KheTaskEnsureUnAssigned(");
    KheTaskDebug(task, 2, -1, stderr);
    fprintf(stderr, ")\n");
  }
  if( !KheTaskAssignIsFixed(task) )
    KheTaskUnAssign(task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRprNodeAssign(KHE_RPR_NODE node, int choice)                     */
/*                                                                           */
/*  Ensure that all of node's tasks are assigned resource[choice].  This     */
/*  is only attempted when it is known to be doable, so it never fails.      */
/*                                                                           */
/*****************************************************************************/
static void KheRprNodeDebug(KHE_RPR_NODE node, int verbosity,
  int indent, FILE *fp);

static void KheRprNodeAssign(KHE_RPR_NODE node, int choice)
{
  KHE_TASK task;  int i;  KHE_RESOURCE r;
  if( DEBUG6 )
  {
    fprintf(stderr, "[ KheRprNodeAssign(node, %d), node =\n", choice);
    KheRprNodeDebug(node, 2, 2, stderr);
  }
  HnAssert(node->assignable[choice], "KheRprNodeAssign internal error");
  r = node->graph->resources[choice];
  HaArrayForEach(node->tasks, task, i)
    KheTaskEnsureAssigned(task, r);
  if( DEBUG6 )
    fprintf(stderr, "] KheRprNodeAssign returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRprNodeUnAssign(KHE_RPR_NODE node)                               */
/*                                                                           */
/*  Ensure that node's tasks are all unassigned.                             */
/*                                                                           */
/*  NB it is vital to unassign in reverse order, so that transactions can    */
/*  see that previous assignments are being undone.                          */
/*                                                                           */
/*****************************************************************************/

static void KheRprNodeUnAssign(KHE_RPR_NODE node)
{
  KHE_TASK task;  int i;
  if( DEBUG6 )
  {
    fprintf(stderr, "[ KheRprNodeUnAssign(node), node =\n");
    KheRprNodeDebug(node, 2, 2, stderr);
  }
  HaArrayForEachReverse(node->tasks, task, i)
    KheTaskEnsureUnAssigned(task);
  if( DEBUG6 )
    fprintf(stderr, "] KheRprNodeUnAssign returning\n");
}


/*****************************************************************************/
/*                                                                           */
/* void KheRprNodeDebug(KHE_RPR_NODE node, int verbosity,                    */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of node onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

static void KheRprNodeDebug(KHE_RPR_NODE node, int verbosity,
  int indent, FILE *fp)
{
  KHE_TASK task;  int i;  KHE_RESOURCE r;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ RprNode [%s, %s] wk%.1f %s", indent, "",
      node->assignable[0] ? "true" : "false",
      node->assignable[1] ? "true" : "false",
      node->workload, SSetShow(node->times));
    if( HaArrayCount(node->tasks) > 0 )
    {
      task = HaArrayFirst(node->tasks);
      r = KheTaskAsstResource(task);
      if( r != NULL )
	fprintf(fp, " := %s", KheResourceId(r)==NULL ? "-" : KheResourceId(r));
    }
    fprintf(fp, "\n");
    if( verbosity >= 2 )
      HaArrayForEach(node->tasks, task, i)
	KheTaskDebug(task, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "components"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheRprComponentConsistent(KHE_RPR_COMPONENT comp)                   */
/*                                                                           */
/*  Check that comp's assignment choices are consistent with its nodes'      */
/*  assignment choices.                                                      */
/*                                                                           */
/*****************************************************************************/
static void KheRprComponentDebug(KHE_RPR_COMPONENT comp, int verbosity,
  int indent, FILE *fp);

static void KheRprComponentConsistent(KHE_RPR_COMPONENT comp)
{
  bool consistent;
  consistent = true;
  if( DEBUG5 )
  {
    if( comp->assign_count != comp->assign_choice[0]+comp->assign_choice[1] )
      consistent = false;
    if( comp->assign_choice[0] )
    {
      if( comp->parts[0] != NULL && !comp->parts[0]->assignable[0] )
	consistent = false;
      if( comp->parts[1] != NULL && !comp->parts[1]->assignable[1] )
	consistent = false;
    }
    if( comp->assign_choice[1] )
    {
      if( comp->parts[0] != NULL && !comp->parts[0]->assignable[1] )
	consistent = false;
      if( comp->parts[1] != NULL && !comp->parts[1]->assignable[0] )
	consistent = false;
    }
    if( !consistent )
      KheRprComponentDebug(comp, 2, 2, stderr);
  }

  HnAssert(comp->assign_count == comp->assign_choice[0]+comp->assign_choice[1],
    "KheRprComponentConsistent: assign_count");
  if( comp->assign_choice[0] )
  {
    HnAssert(comp->parts[0] == NULL || comp->parts[0]->assignable[0],
      "KheRprComponentConsistent: comp->parts[0]->assignable[0]");
    HnAssert(comp->parts[1] == NULL || comp->parts[1]->assignable[1],
      "KheRprComponentConsistent: comp->parts[1]->assignable[1]");
  }
  if( comp->assign_choice[1] )
  {
    HnAssert(comp->parts[0] == NULL || comp->parts[0]->assignable[1],
      "KheRprComponentConsistent: comp->parts[0]->assignable[1]");
    HnAssert(comp->parts[1] == NULL || comp->parts[1]->assignable[0],
      "KheRprComponentConsistent: comp->parts[1]->assignable[1]");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRprComponentSetAssign(KHE_RPR_COMPONENT comp)                    */
/*                                                                           */
/*  Set the assign_count and assign_choice fields of comp from its parts.    */
/*                                                                           */
/*****************************************************************************/

static void KheRprComponentSetAssign(KHE_RPR_COMPONENT comp)
{
  comp->assign_count = 0;
  comp->assign_choice[0] = comp->assign_choice[1] = false;
  if( comp->parts[0]->assignable[0] && comp->parts[1]->assignable[1] )
  {
    comp->assign_count++;
    comp->assign_choice[0] = true;
  }
  if( comp->parts[0]->assignable[1] && comp->parts[1]->assignable[0] )
  {
    comp->assign_count++;
    comp->assign_choice[1] = true;
  }
  if( DEBUG5 && comp->assign_count == 0 )
    KheRprComponentDebug(comp, 2, 2, stderr);
  HnAssert(comp->assign_count >= 1, "KheRprComponentSetAssign internal error");
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RPR_COMPONENT KheRprComponentMake(KHE_RPR_GRAPH graph,               */
/*    KHE_PART_GRAPH_COMPONENT pgc)                                          */
/*                                                                           */
/*  Make a new component of graph, based on pgc.                             */
/*                                                                           */
/*****************************************************************************/

static KHE_RPR_COMPONENT KheRprComponentMake(KHE_RPR_GRAPH graph,
  KHE_PART_GRAPH_COMPONENT pgc)
{
  KHE_RPR_COMPONENT res;   KHE_RPR_NODE node;  int i, j;
  KHE_PART_GRAPH_PART parts[2];
  HaMake(res, graph->arena);
  res->graph = graph;
  KhePartGraphComponentFindParts(pgc);
  if( KhePartGraphComponentParts(pgc, &parts[0], &parts[1]) )
  {
    /* merge the parts into two nodes */
    for( i = 0;  i < 2;  i++ )
    {
      res->parts[i] = KheRprNodeMake(res->graph, res);
      HaArrayAddLast(res->graph->nodes, res->parts[i]);
      for( j = 0;  j < KhePartGraphPartNodeCount(parts[i]);  j++ )
      {
	node = (KHE_RPR_NODE) KhePartGraphPartNode(parts[i], j);
        KheRprNodeMerge(res->parts[i], node);
      }
    }

    /* work out how many assignments are open, and which ones */
    KheRprComponentSetAssign(res);

    /* work out whether this component is rp-symmetrical */
    res->rp_symmetrical = (res->assign_count == 2 &&
       res->parts[0]->workload == res->parts[1]->workload &&
       SSetEqual(res->parts[0]->times, res->parts[1]->times) &&
       KheRprNodeIsPSymmetrical(res->parts[0]) &&
       KheRprNodeIsPSymmetrical(res->parts[1]));
  }
  else
  {
    /* forcibly merge all the nodes into one part; declare unassignable */
    res->parts[0] = KheRprNodeMake(res->graph, res);
    HaArrayAddLast(res->graph->nodes, res->parts[0]);
    res->parts[1] = NULL;
    for( j = 0;  j < KhePartGraphComponentNodeCount(pgc);  j++ )
    {
      node = (KHE_RPR_NODE) KhePartGraphComponentNode(pgc, j);
      KheRprNodeMerge(res->parts[0], node);
    }
    res->assign_count = 0;
    res->assign_choice[0] = res->assign_choice[1] = false;
    res->rp_symmetrical = false;
  }
  KheRprComponentConsistent(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRprComponentDelete(KHE_RPR_COMPONENT comp)                       */
/*                                                                           */
/*  Delete comp.                                                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheRprComponentDelete(KHE_RPR_COMPONENT comp)
{
  ** individual nodes are deleted separately **
  MFree(comp);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheRprComponentAssignCheck(KHE_RPR_COMPONENT comp, int choice)      */
/*                                                                           */
/*  Return true if choice may be assigned to comp.                           */
/*                                                                           */
/*****************************************************************************/

static bool KheRprComponentAssignCheck(KHE_RPR_COMPONENT comp, int choice)
{
  return comp->assign_choice[choice];
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRprComponentIsAssignable(KHE_RPR_COMPONENT comp)                 */
/*                                                                           */
/*  Return true if comp is assignable:  if at least one choice works.        */
/*                                                                           */
/*****************************************************************************/

static bool KheRprComponentIsAssignable(KHE_RPR_COMPONENT comp)
{
  return comp->assign_count >= 1;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRprComponentAssign(KHE_RPR_COMPONENT comp, int choice)           */
/*                                                                           */
/*  Assign choice to comp, assuming that AssignCheck said it was OK.         */
/*                                                                           */
/*****************************************************************************/

static void KheRprComponentAssign(KHE_RPR_COMPONENT comp, int choice)
{
  KheRprNodeAssign(comp->parts[0], choice);
  if( comp->parts[1] != NULL )
    KheRprNodeAssign(comp->parts[1], 1 - choice);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRprComponentUnAssign(KHE_RPR_COMPONENT comp)                     */
/*                                                                           */
/*  Ensure that all the tasks of all the nodes of comp are unassigned.       */
/*                                                                           */
/*  This comment made obsolete by marks: it is vital to unassign in reverse  */
/*  order, so transactions can see that previous assignments are undone.     */
/*                                                                           */
/*****************************************************************************/

static void KheRprComponentUnAssign(KHE_RPR_COMPONENT comp)
{
  if( DEBUG6 )
  {
    fprintf(stderr, "[ KheRprComponentUnAssign(comp)\n");
    KheRprComponentDebug(comp, 2, 2, stderr);
  }
  if( comp->parts[1] != NULL )
    KheRprNodeUnAssign(comp->parts[1]);
  KheRprNodeUnAssign(comp->parts[0]);
  if( DEBUG6 )
    fprintf(stderr, "] KheRprComponentUnAssign returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  int KheRprComponentAvailAsstsCmp(const void *t1, const void *t2)         */
/*                                                                           */
/*  Comparison function for sorting components by increasing number of       */
/*  available assignments.                                                   */
/*                                                                           */
/*****************************************************************************/

static int KheRprComponentAvailAsstsCmp(const void *t1, const void *t2)
{
  KHE_RPR_COMPONENT comp1 = * (KHE_RPR_COMPONENT *) t1;
  KHE_RPR_COMPONENT comp2 = * (KHE_RPR_COMPONENT *) t2;
  return comp1->assign_count - comp2->assign_count;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRprComponentDebug(KHE_RPR_COMPONENT comp, int verbosity,         */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of comp onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

static void KheRprComponentDebug(KHE_RPR_COMPONENT comp, int verbosity,
  int indent, FILE *fp)
{
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ Component: [%s, %s]\n", indent, "",
      KheRprComponentAssignCheck(comp, 0) ? "true" : "false",
      KheRprComponentAssignCheck(comp, 1) ? "true" : "false");
    if( comp->parts[0] != NULL || comp->parts[1] != NULL )
    {
      if( comp->parts[0] != NULL )
	KheRprNodeDebug(comp->parts[0], verbosity, indent + 2, fp);
      if( comp->parts[1] != NULL )
	KheRprNodeDebug(comp->parts[1], verbosity, indent + 2, fp);
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "graph"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RPR_GRAPH KheRprGraphMake(KHE_SOLN soln, KHE_RESOURCE r1,            */
/*    KHE_RESOURCE r2, bool resource_invariant, bool fix_splits)             */
/*                                                                           */
/*  Make a clash graph with these attributes.                                */
/*                                                                           */
/*****************************************************************************/

static KHE_RPR_GRAPH KheRprGraphMake(KHE_SOLN soln, KHE_RESOURCE r1,
  KHE_RESOURCE r2, bool resource_invariant, bool fix_splits, HA_ARENA a)
{
  KHE_RPR_GRAPH res;  int i, j;  KHE_RPR_NODE node;  KHE_TASK task;
  KHE_PART_GRAPH part_graph;  KHE_PART_GRAPH_COMPONENT pgc;  KHE_RESOURCE r;
  HnAssert(r1 != r2, "KheRprGraphMake: r1 == r2");

  /* make the basic object */
  /* a = HaAre naMake(); */
  HaMake(res, a);
  res->soln = soln;
  res->resources[0] = r1;
  res->resources[1] = r2;
  res->fix_splits = fix_splits;
  HaArrayInit(res->nodes, a);
  HaArrayInit(res->components, a);
  res->init_cost = KheSolnCost(soln);
  res->init_defect_count =
    (resource_invariant ? KheSolnMatchingDefectCount(soln) : INT_MAX);
  res->root_mark = NULL;
  /* res->best_cost = KheSolnCost(soln);  trying to improve on this */
  res->node_count = 0;

  /* add one node for each task assigned directly to r1 or r2 */
  part_graph = KhePartGraphMake(&KheRprFirstPartGraphRelFn, a);
  for( i = 0;  i < 2;  i++ )
  {
    r = res->resources[i];
    for( j = 0;  j < KheResourceAssignedTaskCount(soln, r);  j++ )
    {
      task = KheResourceAssignedTask(soln, r, j);
      if( KheTaskIsCycleTask(KheTaskAsst(task)) )
      {
	node = KheRprNodeMake(res, NULL);
	KheRprNodeAddTask(node, task);
	HaArrayAddLast(res->nodes, node);
	KhePartGraphAddNode(part_graph, (void *) node);
      }
    }
  }

  /* add one component for each part_graph component */
  KhePartGraphFindConnectedComponents(part_graph);
  for( i = 0;  i < KhePartGraphComponentCount(part_graph);  i++ )
  {
    pgc = KhePartGraphComponent(part_graph, i);
    HaArrayAddLast(res->components, KheRprComponentMake(res, pgc));
  }
  /* KhePartGraphDelete(part_graph); */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRprGraphDelete(KHE_RPR_GRAPH graph)                              */
/*                                                                           */
/*  Delete graph and everything in it.  This is also the place where         */
/*  all nodes are deleted.                                                   */
/*                                                                           */
/*****************************************************************************/

/* graph will be deleted when the arena is deleted
static void KheRprGraphDelete(KHE_RPR_GRAPH graph)
{
  Ha ArenaDelete(graph->arena);
  ** ***
  while( HaArrayCount(graph->nodes) > 0 )
    KheRprNodeDelete(MArrayRemoveLast(graph->nodes));
  MArrayFree(graph->nodes);
  while( HaArrayCount(graph->components) > 0 )
    KheRprComponentDelete(MArrayRemoveLast(graph->components));
  MArrayFree(graph->components);
  MFree(graph);
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheRprComponentsTryMerge(KHE_PART_GRAPH_COMPONENT pgc,              */
/*    KHE_RPR_GRAPH graph)                                                   */
/*                                                                           */
/*  If pgc's old components are mergeable, merge them, and if the resulting  */
/*  merged component is rpc-symmetrical, remove one of its assignments.      */
/*                                                                           */
/*  The old components are mergeable if all of them are assignable, at most  */
/*  one of them is not rp-symmetrical, and pgc is partitionable.             */
/*                                                                           */
/*****************************************************************************/
#define swap(a, b, tmp) ((tmp) = (a), (a) = (b), (b) = (tmp))

static void KheRprComponentsTryMerge(KHE_PART_GRAPH_COMPONENT pgc,
  KHE_RPR_GRAPH graph)
{
  KHE_RPR_NODE node, tmp;  int i, j, pos;  KHE_PART_GRAPH_PART parts[2];
  KHE_RPR_COMPONENT non_rp_comp, first_comp;  bool tmpb;

  /* return without merging if pgc contains an unassignable old component */
  /* or more than one old component which is not rp-symmetrical */
  non_rp_comp = NULL;
  for( i = 0;  i < KhePartGraphComponentNodeCount(pgc);  i++ )
  {
    node = (KHE_RPR_NODE) KhePartGraphComponentNode(pgc, i);
    if( node->component->assign_count == 0 )
      return;
    if( !node->component->rp_symmetrical )
    {
      if( non_rp_comp == NULL )
        non_rp_comp = node->component;
      else if( node->component != non_rp_comp )
        return;
    }
  }

  /* return without merging if pgc is not partitionable */
  KhePartGraphComponentFindParts(pgc);
  if( !KhePartGraphComponentParts(pgc, &parts[0], &parts[1]) )
    return;

  /* mergeable; find first_comp, an arbitrary old component of pgc, and */
  /* work out which part of pgc goes into which part of first_comp      */
  HnAssert(KhePartGraphPartNodeCount(parts[0]) > 0,
    "KheRprComponentsMerge internal error 1");
  node = (KHE_RPR_NODE) KhePartGraphPartNode(parts[0], 0);
  first_comp = node->component;
  if( first_comp->parts[0] != node )
  {
    HnAssert(first_comp->parts[1] == node,
      "KheRprComponentsMerge internal error 2");
    swap(first_comp->parts[0], first_comp->parts[1], tmp);
    swap(first_comp->assign_choice[0], first_comp->assign_choice[1], tmpb);
  }

  /* delete every old component of pgc except first_comp.  Since old */
  /* components are known to have one node in parts[0] and one in    */
  /* parts[1], the old components can be found via nodes of parts[0] */
  for( j = 1;  j < KhePartGraphPartNodeCount(parts[0]);  j++ )
  {
    node = (KHE_RPR_NODE) KhePartGraphPartNode(parts[0], j);
    HnAssert(node->component != first_comp,
      "KheRprComponentsMerge internal error 3");
    if( !HaArrayContains(graph->components, node->component, &pos) )
      HnAbort("KheRprComponentsMerge internal error 4");
    if( DEBUG5 )
      fprintf(stderr, "  KheRprComponentsTryMerge merging component\n");
    HaArrayDeleteAndShift(graph->components, pos);
    /* KheRprComponentDelete(node->component); */
  }

  /* merge all nodes other than first_comp's into first_comp's */
  for( i = 0;  i < 2;  i++ )
  {
    for( j = 0;  j < KhePartGraphPartNodeCount(parts[i]);  j++ )
    {
      node = (KHE_RPR_NODE) KhePartGraphPartNode(parts[i], j);
      if( node != first_comp->parts[i] )
        KheRprNodeMerge(first_comp->parts[i], node);
    }
  }

  /* reset first_comp's assign fields based on its merged parts */
  KheRprComponentSetAssign(first_comp);

  /* if first_comp is rps-symmetrical, remove one of its two assign choices */
  if( non_rp_comp == NULL )
  {
    if( DEBUG5 )
      fprintf(stderr, "  KheRprComponentsTryMerge halving component\n");
    HnAssert(first_comp->assign_count == 2 && first_comp->assign_choice[0] &&
      first_comp->assign_choice[1], "KheRprComponentsMerge internal error 5");
    first_comp->assign_count = 1;
    first_comp->assign_choice[1] = false;
  }
  KheRprComponentConsistent(first_comp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRprGraphRemoveSymmetries(KHE_RPR_GRAPH graph)                    */
/*                                                                           */
/*  Remove symmetries in graph.                                              */
/*                                                                           */
/*****************************************************************************/
static void KheRprGraphDebug(KHE_RPR_GRAPH graph, int verbosity,
  int indent, FILE *fp);

static void KheRprGraphRemoveSymmetries(KHE_RPR_GRAPH graph, HA_ARENA a)
{
  KHE_PART_GRAPH part_graph;  KHE_RPR_COMPONENT comp;  int i;
  KHE_PART_GRAPH_COMPONENT pgc;
  if( DEBUG6 )
  {
    fprintf(stderr, "[ KheRprGraphRemoveSymmetries(graph), initial graph =\n");
    KheRprGraphDebug(graph, 2, 2, stderr);
  }

  /* build the part graph */
  part_graph = KhePartGraphMake(&KheRprSecondPartGraphRelFn, a);
  HaArrayForEach(graph->components, comp, i)
  {
    HnAssert(comp->parts[0] != NULL,
      "KheRprGraphRemoveSymmetries internal error 1");
    KhePartGraphAddNode(part_graph, (void *) comp->parts[0]);
    if( comp->parts[1] != NULL )
      KhePartGraphAddNode(part_graph, (void *) comp->parts[1]);
  }

  /* try to merge each component */
  KhePartGraphFindConnectedComponents(part_graph);
  for( i = 0;  i < KhePartGraphComponentCount(part_graph);  i++ )
  {
    pgc = KhePartGraphComponent(part_graph, i);
    KheRprComponentsTryMerge(pgc, graph);
  }
  if( DEBUG6 )
  {
    fprintf(stderr, "  final graph =\n");
    KheRprGraphDebug(graph, 2, 2, stderr);
    fprintf(stderr, "] KheRprGraphRemoveSymmetries returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRprGraphDebug(KHE_RPR_GRAPH graph, int verbosity, int indent,    */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of graph onto fp with the given verbosity and indent.        */
/*                                                                           */
/*****************************************************************************/

static void KheRprGraphDebug(KHE_RPR_GRAPH graph, int verbosity,
  int indent, FILE *fp)
{
  KHE_RPR_COMPONENT comp;  int i;  KHE_RESOURCE r1, r2;
  if( verbosity >= 1 && indent >= 0 )
  {
    r1 = graph->resources[0];
    r2 = graph->resources[1];
    fprintf(fp, "%*s[ RprGraph(%s, %s, fix_splits: %s)\n", indent, "",
      KheResourceId(r1) == NULL ? "-" : KheResourceId(r1),
      KheResourceId(r2) == NULL ? "-" : KheResourceId(r2),
      graph->fix_splits ? "true" : "false");
    HaArrayForEach(graph->components, comp, i)
      KheRprComponentDebug(comp, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "exported functions"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheRprGraphTreeSearch(KHE_RPR_GRAPH graph, int pos)                 */
/*                                                                           */
/*  Assign graph->components[pos...] in as many different ways as allowed by */
/*  the node limit, setting graph->best_cost and graph->best_t to the best.  */
/*                                                                           */
/*****************************************************************************/

static void KheRprGraphTreeSearch(KHE_RPR_GRAPH graph, int pos)
{
  KHE_RPR_COMPONENT comp;  int i;  KHE_MARK mark;
  if( pos >= HaArrayCount(graph->components) )
  {
    /* off the end, see whether we have a new best */
    /* ***
    if( KheSolnCost(graph->soln) < graph->best_cost &&
	(graph->init_defect_count == INT_MAX ||
	KheSolnMatchingDefectCount(graph->soln) <= graph->init_defect_count) )
    *** */
    if( KheSolnMatchingDefectCount(graph->soln) <= graph->init_defect_count )
    {
      /* ***
      if( DEBUG5 )
	fprintf(stderr, "  KheRprGraphTreeSearch new best (%.5f -> %.5f):\n",
	  KheCostShow(graph->best_cost), KheCostShow(KheSolnCost(graph->soln)));
      *** */
      KheMarkAddBestPath(graph->root_mark, 1);
      HnAssert(KheMarkPathCount(graph->root_mark) == 1,
	"KheRprGraphTreeSearch internal error 1");
      /* KheTransactionCopy(graph->curr_t, graph->best_t); */
      /* graph->best_cost = KheSolnCost(graph->soln); */
    }
  }
  else
  {
    /* assign comp in all possible ways, unless restricted by node limit */
    comp = HaArray(graph->components, pos);
    for( i = 0;  i < 2;  i++ )
      if( KheRprComponentAssignCheck(comp, i) &&
	  (graph->node_count < KHE_RPR_MAX_NODES || i == 0) )
      {
	graph->node_count++;
	if( DEBUG5 && graph->node_count == KHE_RPR_MAX_NODES )
	  fprintf(stderr, "  at node limit %d\n", KHE_RPR_MAX_NODES);
	mark = KheMarkBegin(graph->soln);
        KheRprComponentAssign(comp, i);
	KheRprGraphTreeSearch(graph, pos + 1);
	KheMarkEnd(mark, true);
	/* KheRprComponentUnAssign(comp); */
      }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourcePairDoReassign(KHE_SOLN soln, KHE_RESOURCE r1,           */
/*    KHE_RESOURCE r2, bool resource_invariant, bool fix_splits,             */
/*    bool *truncated)                                                       */
/*                                                                           */
/*  Redistribute the tasks assigned to r1 and r2 in soln, so as to reduce    */
/*  solution cost if possible.                                               */
/*                                                                           */
/*****************************************************************************/

static bool KheResourcePairDoReassign(KHE_SOLN soln, KHE_RESOURCE r1,
  KHE_RESOURCE r2, bool resource_invariant, bool fix_splits, bool *truncated,
  HA_ARENA a)
{
  KHE_RPR_GRAPH graph;  KHE_RPR_COMPONENT comp;  int i;  bool res;
  if( DEBUG1 )
    fprintf(stderr,
      "  [ KheResourcePairReassign(soln, %s, %s, %s, %s)\n",
      KheResourceId(r1), KheResourceId(r2),
      resource_invariant ? "true" : "false", fix_splits ? "true" : "false");

  /* build clash graph, remove symmetries, sort components by assignability */
  graph = KheRprGraphMake(soln, r1, r2, resource_invariant, fix_splits, a);
  KheRprGraphRemoveSymmetries(graph, a);
  HaArraySort(graph->components, &KheRprComponentAvailAsstsCmp);
  if( DEBUG4 )
    KheRprGraphDebug(graph, 2, 4, stderr);

  /* make current state the first path */
  graph->root_mark = KheMarkBegin(soln);
  KheMarkAddBestPath(graph->root_mark, 1);
  HnAssert(KheMarkPathCount(graph->root_mark) == 1,
    "KheResourcePairReassign internal error 1");

  /* remove the current assignments */
  HaArrayForEach(graph->components, comp, i)
    if( KheRprComponentIsAssignable(comp) )
      KheRprComponentUnAssign(comp);

  /* skip unassignable components, initialize for the tree search, and do it */
  HaArrayForEach(graph->components, comp, i)
    if( KheRprComponentIsAssignable(comp) )
      break;
  graph->node_count = 0;
  KheRprGraphTreeSearch(graph, i);
  *truncated = (graph->node_count >= KHE_RPR_MAX_NODES);

  /* redo the best path */
  HnAssert(KheMarkPathCount(graph->root_mark) == 1,
    "KheResourcePairReassign internal error 2");
  KheMarkUndo(graph->root_mark);
  KhePathRedo(KheMarkPath(graph->root_mark, 0));
  KheMarkEnd(graph->root_mark, false);
  HnAssert(KheSolnCost(soln) <= graph->init_cost,
    "KheResourcePairReassign internal error 3");

  /* work out res, delete graph, and return */
  res = (KheSolnCost(soln) < graph->init_cost);
  if( DEBUG1 )
  {
    if( res )
      fprintf(stderr,
	"  ] KheResourcePairReassign returning true (%.5f -> %.5f)\n",
	KheCostShow(graph->init_cost), KheCostShow(KheSolnCost(soln)));
    else
      fprintf(stderr,"  ] KheResourcePairReassign returning false\n");
  }
  /* KheRprGraphDelete(graph); */
  return res;
}


/* ***
static bool KheResourcePairDoReassign(KHE_SOLN soln, KHE_RESOURCE r1,
  KHE_RESOURCE r2, bool resource_invariant, bool fix_splits, bool *truncated)
{
  KHE_RPR_GRAPH graph;  KHE_RPR_COMPONENT comp;  int i;  KHE_MARK init_mark;
  ** KHE_TRANSACTION init_t; **
  if( DEBUG1 )
    fprintf(stderr,
      "  [ KheResourcePairReassign(soln, %s, %s, %s, %s)\n",
      KheResourceId(r1), KheResourceId(r2),
      resource_invariant ? "true" : "false", fix_splits ? "true" : "false");

  ** build clash graph, remove symmetries, sort components by assignability **
  graph = KheRprGraphMake(soln, r1, r2, resource_invariant, fix_splits);
  KheRprGraphRemoveSymmetries(graph);
  HaArraySort(graph->components, &KheRprComponentAvailAsstsCmp);
  if( DEBUG4 )
    KheRprGraphDebug(graph, 2, 4, stderr);

  ** remove all unfixed assignments in assignable components; save  **
  ** these changes in init_t in case they need to be restored later **
  ** ***
  init_t = KheTransactionMake(soln);
  KheTransactionBegin(init_t);
  KheTransactionEnd(init_t);
  KheTransactionBegin(graph->curr_t);
  KheTransactionEnd(graph->curr_t);
  *** **
  init_mark = KheMarkBegin(soln);
  HaArrayForEach(graph->components, comp, i)
    if( KheRprComponentIsAssignable(comp) )
      KheRprComponentUnAssign(comp);
  graph->root_mark = KheMarkBegin(soln);

  ** skip unassignable components, initialize for the tree search, and do it **
  HaArrayForEach(graph->components, comp, i)
    if( KheRprComponentIsAssignable(comp) )
      break;
  graph->node_count = 0;
  KheRprGraphTreeSearch(graph, i);
  *truncated = (graph->node_count >= KHE_RPR_MAX_NODES);

  if( graph->best_cost < graph->init_cost )
  {
    ** apply best_t **
    HnAssert(KheMarkPathCount(graph->root_mark) == 1,
      "KheResourcePairReassign internal error 1");
    KhePathRedo(KheMarkPath(graph->root_mark, 0));
    HnAssert(KheSolnCost(soln) == graph->best_cost,
      "KheResourcePairReassign internal error 2");
    if( DEBUG1 )
      fprintf(stderr,
	"  ] KheResourcePairReassign returning true (%.5f -> %.5f)\n",
	KheCostShow(graph->init_cost), KheCostShow(graph->best_cost));
    ** KheTransactionDelete(init_t); **
    KheMarkEnd(graph->root_mark, false);
    KheMarkEnd(init_mark, false);
    KheRprGraphDelete(graph);
    return true;
  }
  else
  {
    ** return to init_t **
    KheMarkEnd(graph->root_mark, true);
    KheMarkEnd(init_mark, true);
    ** ***
    if( DEBUG2 )
      KheTransactionDebug(init_t, 2, 2, stderr);
    KheTransactionUndo(init_t);
    *** **
    HnAssert(KheSolnCost(soln) == graph->init_cost,
      "KheResourcePairReassign internal error 3");
    if( DEBUG1 )
      fprintf(stderr,"  ] KheResourcePairReassign returning false\n");
    ** KheTransactionDelete(init_t); **
    KheRprGraphDelete(graph);
    return false;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourcePairReassign(KHE_SOLN soln, KHE_RESOURCE r1,             */
/*    KHE_RESOURCE r2, bool resource_invariant, bool fix_splits)             */
/*                                                                           */
/*  Redistribute the tasks assigned to r1 and r2 in soln, so as to reduce    */
/*  solution cost if possible.                                               */
/*                                                                           */
/*****************************************************************************/

bool KheResourcePairReassign(KHE_SOLN soln, KHE_RESOURCE r1,
  KHE_RESOURCE r2, bool resource_invariant, bool fix_splits)
{
  bool truncated, res;  HA_ARENA a;
  a = KheSolnArenaBegin(soln);
  res = KheResourcePairDoReassign(soln, r1, r2, resource_invariant,
    fix_splits, &truncated, a);
  KheSolnArenaEnd(soln, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheUpdateStats(KHE_OPTIONS options, int calls, int successes,       */
/*    int truncs)                                                            */
/*                                                                           */
/*  Update the statistics by adding these values to their current values.    */
/*                                                                           */
/*****************************************************************************/

void KheUpdateStats(KHE_OPTIONS options, int calls, int successes, int truncs)
{
  KheOptionsSetInt(options, "rs_pair_calls",
    KheOptionsGetInt(options, "rs_pair_calls", 0) + calls);
  KheOptionsSetInt(options, "rs_pair_successes",
    KheOptionsGetInt(options, "rs_pair_successes", 0) + successes);
  KheOptionsSetInt(options, "rs_pair_truncs",
    KheOptionsGetInt(options, "rs_pair_truncs", 0) + truncs);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIncStats(KHE_OPTIONS options, bool success, bool truncated)      */
/*                                                                           */
/*  Increment the statistics as required.                                    */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete
void KheIncStats(KHE_OPTIONS options, bool success, bool truncated)
{
  ** another call **
  KheOptionsSetResourcePairCalls(options,
    KheOptionsResourcePairCalls(options) + 1);

  ** if successful **
  if( success )
    KheOptionsSetResourcePairSuccesses(options,
      KheOptionsResourcePairSuccesses(options) + 1);

  ** if truncated **
  if( truncated )
    KheOptionsSetResourcePairTruncs(options,
      KheOptionsResourcePairTruncs(options) + 1);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourcePairRepairSplitAssignments(KHE_TASKING tasking,          */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  For each pair of resources involved in a split assignment in tasking,    */
/*  call KheResourcePairReassign.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceTypeResourcePairRepairSplits(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options, HA_ARENA a)
{
  int i, j, k, pos;  KHE_TASK task;  KHE_EVENT_RESOURCE er;
  ARRAY_KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR defects;  KHE_RESOURCE r1, r2;
  KHE_MONITOR m;  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam;
  bool res, success, truncated, resource_invariant;
  int calls, successes, truncs;

  resource_invariant = KheOptionsGetBool(options, "rs_invariant", false);
  if( DEBUG2 )
    fprintf(stderr, "[ KheResourceTypeResourcePairRepairSplits(tasking, %s)\n",
       resource_invariant ? "true" : "false");

  /* find the avoid split assignments defects */
  HaArrayInit(defects, a);
  for( i = 0;  i < KheSolnTaskCount(soln);  i++ )
  {
    task = KheSolnTask(soln, i);
    er = KheTaskEventResource(task);
    if( er != NULL && KheEventResourceResourceType(er) == rt )
      for( j = 0;  j < KheSolnEventResourceMonitorCount(soln, er);  j++ )
      {
	m = KheSolnEventResourceMonitor(soln, er, j);
	if( KheMonitorTag(m) == KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG &&
            KheMonitorCost(m) > 0 )
	{
	  asam = (KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m;
	  if( !HaArrayContains(defects, asam, &pos) )
	    HaArrayAddLast(defects, asam);
	}
      }
  }

  /* call KheResourcePairReassign on all pairs of resources within splits */
  res = false;
  calls = successes = truncs = 0;
  HaArrayForEach(defects, asam, i)
  {
    do
    {
      success = false;
      if( DEBUG2 )
      {
	fprintf(stderr, "  repairing ");
	KheMonitorDebug((KHE_MONITOR) asam, 1, 0, stderr);
      }
      for( j = 0;
	   !success && j < KheAvoidSplitAssignmentsMonitorResourceCount(asam);
	   j++ )
      {
	/* can't assign r1 here, sometimes it gives r1 == r2 owing to changes */
	for( k=j+1;
	     !success && k<KheAvoidSplitAssignmentsMonitorResourceCount(asam);
	     k++ )
	{
	  r1 = KheAvoidSplitAssignmentsMonitorResource(asam, j);
	  r2 = KheAvoidSplitAssignmentsMonitorResource(asam, k);
	  if( KheResourcePairDoReassign(soln, r1, r2, resource_invariant, true,
	      &truncated, a) )
	    res = success = true;
	  calls++;
	  if( success ) successes++;
	  if( truncated ) truncs++;
          /* KheIncStats(options, success, truncated); */
	}
      }
    } while( success );
  }
  KheUpdateStats(options, calls, successes, truncs);

  if( DEBUG2 )
    fprintf(stderr, "] KheResourceTypeResourcePairRepairSplits returning %s\n",
      res ? "true" : "false");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceGroupResourcePairRepairAll(KHE_RESOURCE_GROUP rg,        */
/*    KHE_SOLN soln, KHE_OPTIONS options)                                    */
/*                                                                           */
/*  Try a resource repair on each pair of resources in rg.                   */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceGroupResourcePairRepairAll(KHE_RESOURCE_GROUP rg,
  KHE_SOLN soln, KHE_OPTIONS options, HA_ARENA a)
{
  bool res, resource_invariant, success, truncated;  int i1, i2;
  KHE_RESOURCE r1, r2;  int calls, successes, truncs;
  res = false;
  resource_invariant = KheOptionsGetBool(options, "rs_invariant", false);
  calls = successes = truncs = 0;
  for( i1 = 0;  i1 < KheResourceGroupResourceCount(rg);  i1++ )
  {
    r1 = KheResourceGroupResource(rg, i1);
    for( i2 = i1 + 1;  i2 < KheResourceGroupResourceCount(rg);  i2++ )
    {
      r2 = KheResourceGroupResource(rg, i2);
      success = false;
      if( KheResourcePairDoReassign(soln, r1, r2, resource_invariant, false,
	  &truncated, a) )
	res = success = true;
      calls++;
      if( success ) successes++;
      if( truncated ) truncs++;
      /* KheIncStats(options, success, truncated); */
    }
  }
  KheUpdateStats(options, calls, successes, truncs);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTypeResourcePairRepairAll(KHE_RESOURCE_TYPE rt,          */
/*    KHE_TASKING tasking, KHE_OPTIONS options, bool partitions)             */
/*                                                                           */
/*  Carry out a resource pair repair on all pairs of resources of type       */
/*  rt (or all pairs that lie in the same partition, if there are any),      */
/*  not focussing on split assignments.                                      */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceTypeResourcePairRepairAll(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options, bool partitions, HA_ARENA a)
{
  bool res;  int i;  KHE_RESOURCE_GROUP rg;
  res = false;
  if( partitions && KheResourceTypeHasPartitions(rt) )
  {
    for( i = 0;  i < KheResourceTypePartitionCount(rt);  i++ )
    {
      rg = KheResourceTypePartition(rt, i);
      if( KheResourceGroupResourcePairRepairAll(rg, soln, options, a) )
	res = true;
    }
  }
  else
  {
    rg = KheResourceTypeFullResourceGroup(rt);
    if( KheResourceGroupResourcePairRepairAll(rg, soln, options, a) )
      res = true;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTypeResourcePairRepair(KHE_RESOURCE_TYPE rt,             */
/*    KHE_TASKING tasking, KHE_OPTIONS options)                              */
/*                                                                           */
/*  Carry out resource pair repairs on resources of type rt.                 */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceTypeResourcePairRepair(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options, HA_ARENA a)
{
  char *opt;
  opt = KheOptionsGet(options, "rs_pair_select", "splits");
  if( strcmp(opt, "none") == 0 )
    return false;
  else if( strcmp(opt, "splits") == 0 )
    return KheResourceTypeResourcePairRepairSplits(soln, rt, options, a);
  else if( strcmp(opt, "partitions") == 0 )
    return KheResourceTypeResourcePairRepairAll(soln, rt, options, true, a);
  else if( strcmp(opt, "all") == 0 )
    return KheResourceTypeResourcePairRepairAll(soln, rt, options, false, a);
  else
  {
    HnAbort("KheResourceTypeResourcePairRepair: illegal resource_pair");
    return false;  /* keep compiler happy */
  }
}


/* ***
static bool KheResourceTypeResourcePairRepair(KHE_RESOURCE_TYPE rt,
  KHE_TASKING tasking, KHE_OPTIONS options)
{
  bool res;
  switch( KheOptionsResourcePair(options) )
  {
    case KHE_OPTIONS_RESOURCE_PAIR_NONE:

      ** nothing to do here **
      res = false;
      break;

    case KHE_OPTIONS_RESOURCE_PAIR_SPLITS:

      res = KheResourceTypeResourcePairRepairSplits(rt, tasking, options);
      break;

    case KHE_OPTIONS_RESOURCE_PAIR_PARTITIONS:

      res = KheResourceTypeResourcePairRepairAll(rt, tasking, options, true);
      break;

    case KHE_OPTIONS_RESOURCE_PAIR_ALL:

      res = KheResourceTypeResourcePairRepairAll(rt, tasking, options, false);
      break;

    default:

      HnAbort("KheResourceTypeResourcePairRepair: illegal resource_pair");
      res = false;  ** keep compiler happy **
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourcePairRepair(KHE_TASKING tasking, KHE_OPTIONS options)     */
/*                                                                           */
/*  Try resource pair repairs in tasking.  Precisely which repairs depends   */
/*  on the rs_pair_off and rs_pair_select options.                           */
/*                                                                           */
/*****************************************************************************/

bool KheResourcePairRepair(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options)
{
  int i;  KHE_INSTANCE ins;  bool res, pair_off;  HA_ARENA a;

  /* quit immediately if resource pair repair is not wanted */
  pair_off = KheOptionsGetBool(options, "rs_pair_off", "false");
  if( pair_off )
    return false;

  /* boilerplate */
  a = KheSolnArenaBegin(soln);
  if( DEBUG3 )
    fprintf(stderr, "[ KheResourcePairRepair(%s, %s)\n",
      rt == NULL ? NULL : KheResourceTypeId(rt),
      KheOptionsGet(options, "rs_pair_select", "splits"));

  /* solve for tasking's resource types, or all resource types if none */
  res = false;
  if( rt != NULL )
  {
    if( KheResourceTypeResourcePairRepair(soln, rt, options, a) )
      res = true;
  }
  else
  {
    ins = KheSolnInstance(soln);
    for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
    {
      rt = KheInstanceResourceType(ins, i);
      if( KheResourceTypeResourcePairRepair(soln, rt, options, a) )
	res = true;
    }
  }
  if( DEBUG3 )
    fprintf(stderr, "] KheResourcePairRepair returning %s\n",
      res ? "true" : "false");
  KheSolnArenaEnd(soln, a);
  /* HaArenaD elete(a); */
  return res;
}

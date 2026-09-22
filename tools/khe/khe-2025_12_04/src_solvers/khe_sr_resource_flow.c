
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
/*  FILE:         khe_sr_resource_flow.c                                     */
/*  DESCRIPTION:  Resource flow                                              */
/*                                                                           */
/*****************************************************************************/
#include <limits.h>
#include "khe_solvers.h"
#include "khe_mmatch.h"
#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_FLOW_RESOURCE_NODE                                              */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_FLOW_RESOURCE_NODE) ARRAY_KHE_FLOW_RESOURCE_NODE;
typedef HA_ARRAY(KHE_FLOW_TASK_NODE) ARRAY_KHE_FLOW_TASK_NODE;

struct khe_flow_resource_node_rec {
  KHE_RESOURCE_SET		resources;
  KHE_RESOURCE_GROUP		optional_rg;
  int				capacity;
  ARRAY_KHE_FLOW_TASK_NODE	edges;
  KHE_MMATCH_NODE		match_node;
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_TASK_TYPE                                                       */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
typedef enum {
  KHE_TASK_NO,
  KHE_TASK_NO_AND_COUNT,
  KHE_TASK_YES
} KHE_TASK_TYPE;
*** */


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_FLOW_TASK_NODE                                                  */
/*                                                                           */
/*****************************************************************************/

struct khe_flow_task_node_rec {
  KHE_TASK_SET			tasks;
  KHE_RESOURCE_GROUP		domain;
  int				index;
  int				capacity;
  KHE_MMATCH_NODE		match_node;
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_FLOW                                                            */
/*                                                                           */
/*****************************************************************************/

struct khe_flow_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_RESOURCE_TYPE		resource_type;
  bool				preserve_assts;
  bool				include_soft;
  ARRAY_KHE_FLOW_RESOURCE_NODE	resource_nodes;
  ARRAY_KHE_FLOW_TASK_NODE	task_nodes;
  int				flow;
  ARRAY_KHE_FLOW_RESOURCE_NODE	resource_to_resource_node;
  ARRAY_KHE_FLOW_TASK_NODE	task_to_task_node;
  KHE_MMATCH			match;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_FLOW_RESOURCE_NODE"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_FLOW_RESOURCE_NODE KheFlowResourceNodeMake(KHE_RESOURCE r,           */
/*    KHE_FLOW flow)                                                         */
/*                                                                           */
/*  Make a new resource node containing just r.                              */
/*                                                                           */
/*****************************************************************************/

static KHE_FLOW_RESOURCE_NODE KheFlowResourceNodeMake(KHE_RESOURCE r,
  KHE_FLOW flow)
{
  KHE_FLOW_RESOURCE_NODE res;  int index;

  /* make the object and initialize the capacity to max busy times; */
  /* the capacity may be reduced as tasks assigned r are discovered */
  HaMake(res, flow->arena);
  res->resources = KheResourceSetMake(flow->resource_type, flow->arena);
  KheResourceSetAddResource(res->resources, r);
  res->optional_rg = NULL;
  if( !KheResourceMaxBusyTimes(flow->soln, r, &res->capacity) )
    res->capacity = KheInstanceTimeCount(KheSolnInstance(flow->soln));
  HaArrayInit(res->edges, flow->arena);
  res->match_node = NULL;

  /* record the addition in the resource_to_resource_node array */
  index = KheResourceResourceTypeIndex(r);
  HaArrayFill(flow->resource_to_resource_node, index + 1, NULL);
  HaArrayPut(flow->resource_to_resource_node, index, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowResourceNodeReduceCapacity(KHE_FLOW_RESOURCE_NODE rn,        */
/*    int amount)                                                            */
/*                                                                           */
/*  Reduce the capacity of rn by amount, although not to below zero.         */
/*                                                                           */
/*****************************************************************************/

static void KheFlowResourceNodeReduceCapacity(KHE_FLOW_RESOURCE_NODE rn,
  int amount)
{
  rn->capacity -= amount;
  if( rn->capacity < 0 )
    rn->capacity = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFlowResourceNodeMatchesTaskNode(KHE_FLOW_RESOURCE_NODE frn,      */
/*    KHE_FLOW_TASK_NODE ftn)                                                */
/*                                                                           */
/*  Return true if frn can match with ftn.                                   */
/*                                                                           */
/*****************************************************************************/

static bool KheFlowResourceNodeMatchesTaskNode(KHE_FLOW_RESOURCE_NODE frn,
  KHE_FLOW_TASK_NODE ftn)
{
  int i;  KHE_RESOURCE r;
  for( i = 0;  i < KheResourceSetResourceCount(frn->resources);  i++ )
  {
    r = KheResourceSetResource(frn->resources, i);
    if( !KheResourceGroupContains(ftn->domain, r) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowResourceNodeAddEdge(KHE_FLOW_RESOURCE_NODE frn,              */
/*    KHE_FLOW_TASK_NODE ftn)                                                */
/*                                                                           */
/*  Add an edge from frn to ftn.                                             */
/*                                                                           */
/*****************************************************************************/

static void KheFlowResourceNodeAddEdge(KHE_FLOW_RESOURCE_NODE frn,
  KHE_FLOW_TASK_NODE ftn)
{
  HaArrayAddLast(frn->edges, ftn);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheFlowResourceNodeTypedCmp(KHE_FLOW_RESOURCE_NODE frn1,             */
/*    KHE_FLOW_RESOURCE_NODE frn2)                                           */
/*                                                                           */
/*  Typed comparison function to bring resource nodes whose edges have the   */
/*  same destinations together.  We assume that these destinations are       */
/*  already sorted (they will be, because they are created that way).        */
/*                                                                           */
/*****************************************************************************/

static int KheFlowResourceNodeTypedCmp(KHE_FLOW_RESOURCE_NODE frn1,
  KHE_FLOW_RESOURCE_NODE frn2)
{
  int count1, count2, i;  KHE_FLOW_TASK_NODE ftn1, ftn2;

  /* nodes with fewer outgoing edges come before nodes with more */
  count1 = HaArrayCount(frn1->edges);
  count2 = HaArrayCount(frn2->edges);
  if( count1 != count2 )
    return count1 - count2;

  /* the first different outgoing edges determines the result */
  for( i = 0;  i < count1;  i++ )
  {
    ftn1 = HaArray(frn1->edges, i);
    ftn2 = HaArray(frn2->edges, i);
    if( ftn1->index != ftn2->index )
      return ftn1->index - ftn2->index;
  }

  /* same edges */
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheFlowResourceNodeCmp(const void *t1, const void *t2)               */
/*                                                                           */
/*  Untyped comparison function to bring resource nodes whose edges have     */
/*  the same destinations together.  We assume that these destinations are   */
/*  already sorted (they will be, because they are created that way).        */
/*                                                                           */
/*****************************************************************************/

static int KheFlowResourceNodeCmp(const void *t1, const void *t2)
{
  KHE_FLOW_RESOURCE_NODE frn1 = * (KHE_FLOW_RESOURCE_NODE *) t1;
  KHE_FLOW_RESOURCE_NODE frn2 = * (KHE_FLOW_RESOURCE_NODE *) t2;
  return KheFlowResourceNodeTypedCmp(frn1, frn2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheFlowResourceNodeIndexTypedCmp(KHE_FLOW_RESOURCE_NODE frn1,        */
/*    KHE_FLOW_RESOURCE_NODE frn2)                                           */
/*                                                                           */
/*  Typed comparison function to sort resource nodes by increasing order of  */
/*  the index of the first resource.                                         */
/*                                                                           */
/*****************************************************************************/

static int KheFlowResourceNodeIndexTypedCmp(KHE_FLOW_RESOURCE_NODE frn1,
  KHE_FLOW_RESOURCE_NODE frn2)
{
  KHE_RESOURCE r1, r2;
  r1 = KheResourceSetResource(frn1->resources, 0);
  r2 = KheResourceSetResource(frn2->resources, 0);
  return KheResourceResourceTypeIndex(r1) - KheResourceResourceTypeIndex(r2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheFlowResourceNodeIndexCmp(const void *t1, const void *t2)          */
/*                                                                           */
/*  Untyped comparison function to bring resource nodes whose edges have     */
/*  the same destinations together.  We assume that these destinations are   */
/*  already sorted (they will be, because they are created that way).        */
/*                                                                           */
/*****************************************************************************/

static int KheFlowResourceNodeIndexCmp(const void *t1, const void *t2)
{
  KHE_FLOW_RESOURCE_NODE frn1 = * (KHE_FLOW_RESOURCE_NODE *) t1;
  KHE_FLOW_RESOURCE_NODE frn2 = * (KHE_FLOW_RESOURCE_NODE *) t2;
  return KheFlowResourceNodeIndexTypedCmp(frn1, frn2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowResourceNodeMerge(KHE_FLOW_RESOURCE_NODE dst_frn,            */
/*    KHE_FLOW_RESOURCE_NODE src_frn, KHE_FLOW f)                            */
/*                                                                           */
/*  Merge src_frn into dst_frn.  This does not delete src_frn from the       */
/*  list of all resource nodes in the enclosing flow, but it does            */
/*  everything else.                                                         */
/*                                                                           */
/*****************************************************************************/

static void KheFlowResourceNodeMerge(KHE_FLOW_RESOURCE_NODE dst_frn,
  KHE_FLOW_RESOURCE_NODE src_frn, KHE_FLOW f)
{
  KHE_RESOURCE r;  int i, index;

  /* update the resource_to_resource_node array */
  for( i = 0;  i < KheResourceSetResourceCount(src_frn->resources);  i++ )
  {
    r = KheResourceSetResource(src_frn->resources, i);
    index = KheResourceResourceTypeIndex(r);
    HaArrayPut(f->resource_to_resource_node, index, dst_frn);
  }

  /* update dst_frn's resource set and capacity */
  KheResourceSetUnion(dst_frn->resources, src_frn->resources);
  dst_frn->capacity += src_frn->capacity;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowResourceNodeFindOptionalResourceGroup(                       */
/*    KHE_FLOW_RESOURCE_NODE frn)                                            */
/*                                                                           */
/*  Set frn->optional_rg to a resource group containing the resources of     */
/*  frn, if such a resource group can be found.                              */
/*                                                                           */
/*****************************************************************************/

static void KheFlowResourceNodeFindOptionalResourceGroup(
  KHE_FLOW_RESOURCE_NODE frn)
{
  KHE_RESOURCE r;  KHE_RESOURCE_GROUP rg;  int i;
  if( DEBUG4 )
  {
    fprintf(stderr, "[ KheFlowResourceNodeFindOptionalResourceGroup(frn)\n");
    KheFlowResourceNodeDebug(frn, 2, 2, stderr);
  }
  r = KheResourceSetResource(frn->resources, 0);
  for( i = 0;  i < KheResourceResourceGroupCount(r);  i++ )
  {
    rg = KheResourceResourceGroup(r, i);
    if( DEBUG4 )
      fprintf(stderr, "  trying %s (%d resources)\n",
	KheResourceGroupId(rg), KheResourceGroupResourceCount(rg));
    if( KheResourceSetEqualGroup(frn->resources, rg) )
    {
      frn->optional_rg = rg;
      break;
    }
  }
  if( DEBUG4 )
  {
    KheFlowResourceNodeDebug(frn, 2, 2, stderr);
    fprintf(stderr, "] KheFlowResourceNodeFindOptionalResourceGroup\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SET KheFlowResourceNodeResources(KHE_FLOW_RESOURCE_NODE frn)*/
/*                                                                           */
/*  Return a resource set containing the resources represented by flow       */
/*  resource node frn.                                                       */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_SET KheFlowResourceNodeResources(KHE_FLOW_RESOURCE_NODE frn)
{
  return frn->resources;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFlowResourceNodeResourceGroup(KHE_FLOW_RESOURCE_NODE frn,        */
/*    KHE_RESOURCE_GROUP *rg)                                                */
/*                                                                           */
/*  If frn->resources corresponds with an existing resource group, set *rg   */
/*  to that resource group and return true.  Othwerwise set *rg to NULL      */
/*  and return false.                                                        */
/*                                                                           */
/*****************************************************************************/

bool KheFlowResourceNodeResourceGroup(KHE_FLOW_RESOURCE_NODE frn,
  KHE_RESOURCE_GROUP *rg)
{
  *rg = frn->optional_rg;
  return *rg != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheFlowResourceNodeCapacity(KHE_FLOW_RESOURCE_NODE frn)              */
/*                                                                           */
/*  Return the capacity of flow resource node frn.                           */
/*                                                                           */
/*****************************************************************************/

int KheFlowResourceNodeCapacity(KHE_FLOW_RESOURCE_NODE frn)
{
  return frn->capacity;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFlowResourceNodeFlow(KHE_FLOW_RESOURCE_NODE frn,                 */
/*    KHE_FLOW_TASK_NODE *ftn, int *flow)                                    */
/*                                                                           */
/*  Return one edge of the max flow.                                         */
/*                                                                           */
/*****************************************************************************/

bool KheFlowResourceNodeFlow(KHE_FLOW_RESOURCE_NODE frn,
  KHE_FLOW_TASK_NODE *ftn, int *flow)
{
  KHE_MMATCH_NODE sn;  int c1, c2, c3;
  if( KheMMatchResultEdge(frn->match_node, &sn, flow, &c1, &c2, &c3) )
  {
    *ftn = (KHE_FLOW_TASK_NODE) KheMMatchSupplyNodeBack(sn);
    return true;
  }
  else
  {
    *ftn = NULL;
    *flow = 0;
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowResourceNodeHeaderDebug(KHE_FLOW_RESOURCE_NODE frn, FILE *fp)*/
/*                                                                           */
/*  Debug print of the header part of the debug print of frn onto fp.        */
/*                                                                           */
/*****************************************************************************/

static void KheFlowResourceNodeHeaderDebug(KHE_FLOW_RESOURCE_NODE frn, FILE *fp)
{
  fprintf(fp, "FlowResourceNode(%d %s, %d capacity)",
    KheResourceSetResourceCount(frn->resources),
    frn->optional_rg != NULL ? KheResourceGroupId(frn->optional_rg) :
    "resources", frn->capacity);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowResourceNodeDebug(KHE_FLOW_RESOURCE_NODE frn, int verbosity, */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of y onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheFlowResourceNodeDebug(KHE_FLOW_RESOURCE_NODE frn, int verbosity,
  int indent, FILE *fp)
{
  KHE_FLOW_TASK_NODE ftn;  int i;
  if( indent < 0 )
    KheFlowResourceNodeHeaderDebug(frn, fp);
  else if( verbosity == 1 )
  {
    fprintf(fp, "%*s", indent, "");
    KheFlowResourceNodeHeaderDebug(frn, fp);
    fprintf(fp, "\n");
  }
  else
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheFlowResourceNodeHeaderDebug(frn, fp);
    fprintf(fp, "\n");
    KheResourceSetDebug(frn->resources, verbosity, indent + 2, fp);
    HaArrayForEach(frn->edges, ftn, i)
    {
      fprintf(fp, "%*s", indent + 2, "");
      KheFlowTaskNodeDebug(ftn, 1, -1, fp);
      fprintf(fp, "\n");
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowResourceNodeDebugFn(void *frn, int verbosity,                */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Untyped version of KheFlowResourceNodeDebug, suited to passing to the    */
/*  mmatch.                                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheFlowResourceNodeDebugFn(void *frn, int verbosity,
  int indent, FILE *fp)
{
  KheFlowResourceNodeDebug((KHE_FLOW_RESOURCE_NODE) frn, verbosity,indent,fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TASK_TYPE"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *KheTaskTypeShow(KHE_TASK_TYPE tt)                                  */
/*                                                                           */
/*  Show tt.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static char *KheTaskTypeShow(KHE_TASK_TYPE tt)
{
  switch( tt )
  {
    case KHE_TASK_NO:		return "no";
    case KHE_TASK_NO_AND_COUNT:	return "no_and_count";
    case KHE_TASK_YES:		return "yes";
    default:			return "??";
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_FLOW_TASK_NODE"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_TYPE KheTaskType(KHE_TASK task, KHE_FLOW flow, KHE_RESOURCE *r) */
/*                                                                           */
/*  Return the type of task, determining what to do with it.                 */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static KHE_TASK_TYPE KheTaskType(KHE_TASK task, KHE_FLOW flow, KHE_RESOURCE *r)
{
  ** if task has the wrong type, omit it **
  if( KheTaskResourceType(task) != flow->resource_type )
  {
    if( DEBUG3 )
      fprintf(stderr, "wrong resource type (wanted %s, got %s), so:\n",
	KheResourceTypeId(flow->resource_type),
	KheResourceTypeId(KheTaskResourceType(task)));
    return *r = NULL, KHE_TASK_NO;
  }

  ** if task has no event resource, omit it **
  if( KheTaskEventResource(task) == NULL )
  {
    if( DEBUG3 )
      fprintf(stderr, "no event resource, so:\n");
    return *r = NULL, KHE_TASK_NO;
  }

  ** if task is not a proper root task, omit it **
  if( !KheTaskIsProperRoot(task) )
  {
    if( DEBUG3 )
      fprintf(stderr, "not proper root, so:\n");
    return *r = NULL, KHE_TASK_NO;
  }

  ** if task is fixed, or assigned and we are omitting those, omit it **
  *r = KheTaskAsstResource(task);
  if( KheTaskAssignIsFixed(task) || (flow->preserve_assts && *r != NULL) )
  {
    if( DEBUG3 )
      fprintf(stderr, "fixed %s, preserve_assts %s, *r %s NULL, so:\n",
	bool_show(KheTaskAssignIsFixed(task)),
	bool_show(flow->preserve_assts), (*r != NULL ? "!=" : "=="));
    return *r != NULL ? KHE_TASK_NO_AND_COUNT : KHE_TASK_NO;
  }

  ** otherwise we are including task **
  return KHE_TASK_YES;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_FLOW_TASK_NODE KheFlowTaskNodeMake(KHE_RESOURCE_GROUP domain,        */
/*    int index, KHE_FLOW flow)                                              */
/*                                                                           */
/*  Make a new, empty flow task node with these attributes.                  */
/*                                                                           */
/*****************************************************************************/

static KHE_FLOW_TASK_NODE KheFlowTaskNodeMake(KHE_RESOURCE_GROUP domain,
  int index, KHE_FLOW flow)
{
  KHE_FLOW_TASK_NODE res;
  HaMake(res, flow->arena);
  res->tasks = KheTaskSetMake(flow->soln);
  res->domain = domain;
  res->index = index;
  res->capacity = 0;
  res->match_node = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowTaskNodeAddTask(KHE_FLOW_TASK_NODE ftn, KHE_TASK task)       */
/*                                                                           */
/*  Add task to ftn.                                                         */
/*                                                                           */
/*****************************************************************************/

static void KheFlowTaskNodeAddTask(KHE_FLOW_TASK_NODE ftn, KHE_TASK task,
  KHE_FLOW flow)
{
  int index;

  /* add task to ftn */
  KheTaskSetAddTask(ftn->tasks, task);
  ftn->capacity += KheTaskTotalDuration(task);

  /* record the addition in the task_to_task_node array */
  index = KheTaskSolnIndex(task);
  HaArrayFill(flow->task_to_task_node, index + 1, NULL);
  HaArrayPut(flow->task_to_task_node, index, ftn);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_SET KheFlowTaskNodeTasks(KHE_FLOW_TASK_NODE ftn)                */
/*                                                                           */
/*  Return a task set containing the tasks represented by flow task node ftn.*/
/*                                                                           */
/*****************************************************************************/

KHE_TASK_SET KheFlowTaskNodeTasks(KHE_FLOW_TASK_NODE ftn)
{
  return ftn->tasks;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheFlowTaskNodeDomain(KHE_FLOW_TASK_NODE ftn)         */
/*                                                                           */
/*  Return the shared domain of the tasks of ftn.                            */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheFlowTaskNodeDomain(KHE_FLOW_TASK_NODE ftn)
{
  return ftn->domain;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheFlowTaskNodeCapacity(KHE_FLOW_TASK_NODE ftn)                      */
/*                                                                           */
/*  Return the capacity of flow task node ftn.                               */
/*                                                                           */
/*****************************************************************************/

int KheFlowTaskNodeCapacity(KHE_FLOW_TASK_NODE ftn)
{
  return ftn->capacity;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowTaskNodeHeaderDebug(KHE_FLOW_TASK_NODE ftn, FILE *fp)        */
/*                                                                           */
/*  Debug print of the header part of the debug print of ftn onto fp.        */
/*                                                                           */
/*****************************************************************************/

static void KheFlowTaskNodeHeaderDebug(KHE_FLOW_TASK_NODE ftn, FILE *fp)
{
  fprintf(fp, "FlowTaskNode %d(%d tasks, %d capacity)", ftn->index,
    KheTaskSetTaskCount(ftn->tasks), ftn->capacity);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowTaskNodeDebug(KHE_FLOW_TASK_NODE ftn, int verbosity,         */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of ftn onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

void KheFlowTaskNodeDebug(KHE_FLOW_TASK_NODE ftn, int verbosity,
  int indent, FILE *fp)
{
  if( indent < 0 )
    KheFlowTaskNodeHeaderDebug(ftn, fp);
  else if( verbosity == 1 )
  {
    fprintf(fp, "%*s", indent, "");
    KheFlowTaskNodeHeaderDebug(ftn, fp);
    fprintf(fp, "\n");
  }
  else
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheFlowTaskNodeHeaderDebug(ftn, fp);
    fprintf(fp, "\n");
    KheResourceGroupDebug(ftn->domain, 1, indent + 2, fp);
    KheTaskSetDebug(ftn->tasks, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowTaskNodeDebugFn(void *ftn, int verbosity,                    */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Untyped version of KheFlowTaskNodeDebug, suited to passing to the        */
/*  mmatch.                                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheFlowTaskNodeDebugFn(void *ftn, int verbosity,
  int indent, FILE *fp)
{
  KheFlowTaskNodeDebug((KHE_FLOW_TASK_NODE) ftn, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_FLOW"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheFlowBuildResourceNodes(KHE_FLOW f)                               */
/*                                                                           */
/*  Build resource nodes for f.                                              */
/*                                                                           */
/*****************************************************************************/

static void KheFlowBuildResourceNodes(KHE_FLOW f)
{
  KHE_RESOURCE_TYPE rt;  KHE_FLOW_RESOURCE_NODE frn;  int i;  KHE_RESOURCE r;
  rt = f->resource_type;
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    frn = KheFlowResourceNodeMake(r, f);
    HaArrayAddLast(f->resource_nodes, frn);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowAddTask(KHE_FLOW f, KHE_TASK task)                           */
/*                                                                           */
/*  Add task to flow.                                                        */
/*                                                                           */
/*****************************************************************************/

static void KheFlowAddTask(KHE_FLOW f, KHE_TASK task)
{
  KHE_EVENT_RESOURCE er;  KHE_RESOURCE_GROUP domain;
  KHE_FLOW_TASK_NODE ftn;  int i;

  /* get the task's domain */
  er = KheTaskEventResource(task);
  if( f->include_soft )
    domain = KheEventResourceHardAndSoftDomain(er);
  else
    domain = KheEventResourceHardDomain(er);

  /* add task to an existing task node, if a suitable one exists */
  HaArrayForEach(f->task_nodes, ftn, i)
    if( KheResourceGroupEqual(ftn->domain, domain) )
    {
      KheFlowTaskNodeAddTask(ftn, task, f);
      return;
    }

  /* otherwise make a new task node and add task to that */
  ftn = KheFlowTaskNodeMake(domain, HaArrayCount(f->task_nodes), f);
  KheFlowTaskNodeAddTask(ftn, task, f);
  HaArrayAddLast(f->task_nodes, ftn);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskIsAdmissible(KHE_TASK task, KHE_FLOW f)                      */
/*                                                                           */
/*  Return true if task is admissible, as defined in the documentation.      */
/*  Except we omit conditions (1) and (2); they are done separately.         */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskIsAdmissible(KHE_TASK task, KHE_FLOW f)
{
  KHE_RESOURCE r;  KHE_COST non_asst_cost, asst_cost;

  /* (3) It is derived from an event resource */
  if( KheTaskEventResource(task) == NULL )
    return false;

  /* (4) It is not preassigned */
  if( KheTaskIsPreassigned(task, &r) )
    return false;

  /* (5) Its assignment is not fixed */
  if( KheTaskAssignIsFixed(task) )
    return false;

  /* (6) It has positive non-assignment cost */
  KheTaskNonAsstAndAsstCost(task, &non_asst_cost, &asst_cost);
  if( non_asst_cost == 0 )
    return false;

  /* (7) Optionally, it is not assigned */
  if( f->preserve_assts )
  {
    if( KheTaskAsstResource(task) != NULL )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowBuildTaskNodes(KHE_FLOW f)                                   */
/*                                                                           */
/*  Build the task nodes of flow.                                            */
/*                                                                           */
/*****************************************************************************/

static void KheFlowBuildTaskNodes(KHE_FLOW f)
{
  int i, count;  KHE_TASK task;  KHE_RESOURCE r;  KHE_FLOW_RESOURCE_NODE rn;
  count = KheSolnTaskCount(f->soln);
  for( i = 0;  i < count;  i++ )
  {
    task = KheSolnTask(f->soln, i);
    if( DEBUG2 )
      fprintf(stderr, "  considering task %s:\n", KheTaskId(task));
    if( KheTaskResourceType(task) == f->resource_type &&      /* (1) */
        KheTaskIsProperRoot(task) )                          /* (2) */
    {
      if( KheTaskIsAdmissible(task, f) )
      {
	/* admissible task, add to task node */
	KheFlowAddTask(f, task);
      }
      else
      {
	/* inadmissible proper root task, reduce resource node capacity */
	r = KheTaskAsstResource(task);
	if( r != NULL )
	{
	  rn = KheFlowResourceToResourceNode(f, r);
	  HnAssert(rn != NULL, "KheFlowBuildTaskNodes internal error");
	  KheFlowResourceNodeReduceCapacity(rn, KheTaskTotalDuration(task));
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowCostDebug(int c1, int c2, int c3, FILE *fp)                  */
/*                                                                           */
/*  Debug print of the cost of an edge.  Actually we aren't using            */
/*  costs so all these numbers will be 0 and we just print a dash.           */
/*                                                                           */
/*****************************************************************************/

static void KheFlowCostDebug(int c1, int c2, int c3, FILE *fp)
{
  fprintf(fp, "-");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowBuildAndSolveFlowGraph(KHE_FLOW f)                           */
/*                                                                           */
/*  Build and solve the flow graph.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheFlowBuildAndSolveFlowGraph(KHE_FLOW f)
{
  KHE_FLOW_RESOURCE_NODE frn;  KHE_FLOW_TASK_NODE ftn;  int i, j, mult;

  /* clear out any old graph */
  KheMMatchClear(f->match);

  /* add the resource nodes as demand nodes */
  HaArrayForEach(f->resource_nodes, frn, i)
    frn->match_node = KheMMatchDemandNodeMake(f->match, frn->capacity,
      (void *) frn);

  /* add the task nodes as supply nodes */
  HaArrayForEach(f->task_nodes, ftn, j)
    ftn->match_node = KheMMatchSupplyNodeMake(f->match, ftn->capacity,
      (void *) ftn);

  /* add the edges */
  HaArrayForEach(f->resource_nodes, frn, i)
    HaArrayForEach(frn->edges, ftn, j)
      KheMMatchAddEdge(frn->match_node, ftn->match_node, INT_MAX/2, 0, 0, 0);

  /* solve */
  KheMMatchSolve(f->match);

  /* debug print */
  if( DEBUG5 )
  {
    fprintf(stderr, "[ Flow\n");
    HaArrayForEach(f->resource_nodes, frn, i)
    {
      KheFlowResourceNodeDebug(frn, 1, 2, stderr);
      while( KheFlowResourceNodeFlow(frn, &ftn, &mult) )
      {
	fprintf(stderr, "  --%d-> ", mult);
	KheFlowTaskNodeDebug(ftn, 1, 0, stderr);
      }
    }
    fprintf(stderr, "]\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_FLOW KheFlowMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,                */
/*    bool preserve_assts, bool include_soft)                                */
/*                                                                           */
/*  Make a resource flow object with these attributes in an arena taken      */
/*  from soln.                                                               */
/*                                                                           */
/*****************************************************************************/
static void KheFlowDebugFn(void *f, int verbosity, int indent, FILE *fp);

KHE_FLOW KheFlowMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  bool preserve_assts, bool include_soft)
{
  HA_ARENA a;  KHE_FLOW res;  KHE_FLOW_RESOURCE_NODE frn, prev_frn;
  KHE_FLOW_TASK_NODE ftn;  int i, j, prev_pos, deleted_count;

  if( DEBUG1 )
    fprintf(stderr, "[ KheFlowMake(%s, %s, %s, %s)\n",
      KheInstanceId(KheSolnInstance(soln)), KheResourceTypeId(rt),
      bool_show(preserve_assts), bool_show(include_soft));

  /* build the basic object */
  a = KheSolnArenaBegin(soln);
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->resource_type = rt;
  res->preserve_assts = preserve_assts;
  res->include_soft = include_soft;
  HaArrayInit(res->resource_nodes, a);
  HaArrayInit(res->task_nodes, a);
  res->flow = 0;
  HaArrayInit(res->resource_to_resource_node, a);
  HaArrayInit(res->task_to_task_node, a);
  res->match = KheMMatchMake((void *) res,
    &KheFlowDebugFn, &KheFlowResourceNodeDebugFn,
    &KheFlowTaskNodeDebugFn, &KheFlowCostDebug, a);

  /* add resource nodes and task nodes, and merge resource nodes */
  KheFlowBuildResourceNodes(res);
  KheFlowBuildTaskNodes(res);

  /* add edges */
  HaArrayForEach(res->resource_nodes, frn, i)
    HaArrayForEach(res->task_nodes, ftn, j)
      if( KheFlowResourceNodeMatchesTaskNode(frn, ftn) )
	KheFlowResourceNodeAddEdge(frn, ftn);

  /* sort resource nodes to bring equal sets of edges together */
  HaArraySort(res->resource_nodes, &KheFlowResourceNodeCmp);

  /* merge resource nodes with equal edges */
  prev_pos = -1;  prev_frn = NULL;  deleted_count = 0;
  HaArrayForEach(res->resource_nodes, frn, i)
  {
    /* does the i'th element match the pos'th element? */
    if( prev_frn != NULL && KheFlowResourceNodeTypedCmp(prev_frn, frn) == 0 )
    {
      KheFlowResourceNodeMerge(prev_frn, frn, res);
      deleted_count++;
    }
    else
    {
      prev_frn = frn;
      prev_pos++;
      HaArrayPut(res->resource_nodes, prev_pos, prev_frn);
    }
  }

  /* clear out the elements of res->resource_nodes beyond prev_pos */
  HaArrayDeleteLastSlice(res->resource_nodes, deleted_count);

  /* sort res->resource_nodes into a conventional order */
  HaArraySort(res->resource_nodes, &KheFlowResourceNodeIndexCmp);

  /* give names to the resource sets in the resource nodes, when available */
  HaArrayForEach(res->resource_nodes, frn, i)
    KheFlowResourceNodeFindOptionalResourceGroup(frn);

  /* find a maximum flow */
  KheFlowBuildAndSolveFlowGraph(res);

  /* all done, debug and exit */
  if( DEBUG1 )
  {
    KheFlowDebug(res, 2, 2, stderr);
    fprintf(stderr, "] KheFlowMake returning\n");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowDelete(KHE_FLOW f)                                           */
/*                                                                           */
/*  Delete f by returning its arena to soln.  Also delete the task sets.     */
/*                                                                           */
/*****************************************************************************/

void KheFlowDelete(KHE_FLOW f)
{
  KHE_FLOW_TASK_NODE ftn;  int i;

  /* delete the tasks sets (they don't come from rf->arena) */
  HaArrayForEach(f->task_nodes, ftn, i)
    KheTaskSetDelete(ftn->tasks);

  /* return the arena to f->soln */
  KheSolnArenaEnd(f->soln, f->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheFlowResourceNodeCount(KHE_FLOW f)                                 */
/*                                                                           */
/*  Return the number of flow resource nodes in f.                           */
/*                                                                           */
/*****************************************************************************/

int KheFlowResourceNodeCount(KHE_FLOW f)
{
  return HaArrayCount(f->resource_nodes);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_FLOW_RESOURCE_NODE KheFlowResourceNode(KHE_FLOW f, int i)            */
/*                                                                           */
/*  Return the i'th flow resource node of f.                                 */
/*                                                                           */
/*****************************************************************************/

KHE_FLOW_RESOURCE_NODE KheFlowResourceNode(KHE_FLOW f, int i)
{
  return HaArray(f->resource_nodes, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheFlowTaskNodeCount(KHE_FLOW f)                                     */
/*                                                                           */
/*  Return the number of flow task nodes in f.                               */
/*                                                                           */
/*****************************************************************************/

int KheFlowTaskNodeCount(KHE_FLOW f)
{
  return HaArrayCount(f->task_nodes);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_FLOW_TASK_NODE KheFlowTaskNode(KHE_FLOW f, int i)                    */
/*                                                                           */
/*  Return the i'th flow task node of f.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_FLOW_TASK_NODE KheFlowTaskNode(KHE_FLOW f, int i)
{
  return HaArray(f->task_nodes, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_FLOW_RESOURCE_NODE KheFlowResourceToResourceNode(KHE_FLOW f,         */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Return the flow resource node containing r, or NULL if none.             */
/*                                                                           */
/*****************************************************************************/

KHE_FLOW_RESOURCE_NODE KheFlowResourceToResourceNode(KHE_FLOW f,
  KHE_RESOURCE r)
{
  int index;
  index = KheResourceResourceTypeIndex(r);
  if( index >= HaArrayCount(f->resource_to_resource_node) )
    return NULL;
  else
    return HaArray(f->resource_to_resource_node, index);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_FLOW_TASK_NODE KheFlowTaskToTaskNode(KHE_FLOW f, KHE_TASK task)      */
/*                                                                           */
/*  Return the flow task node containing task, or NULL if none.              */
/*                                                                           */
/*****************************************************************************/

KHE_FLOW_TASK_NODE KheFlowTaskToTaskNode(KHE_FLOW f, KHE_TASK task)
{
  int index;
  index = KheTaskSolnIndex(task);
  if( index >= HaArrayCount(f->task_to_task_node) )
    return NULL;
  else
    return HaArray(f->task_to_task_node, index);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowHeaderDebug(KHE_FLOW f, FILE *fp)                            */
/*                                                                           */
/*  Print the header line of the debug print of f.                           */
/*                                                                           */
/*****************************************************************************/

static void KheFlowHeaderDebug(KHE_FLOW f, FILE *fp)
{
  fprintf(fp, "Flow(%s, %d resource nodes, %d task nodes, flow %d)",
    KheResourceTypeId(f->resource_type), HaArrayCount(f->resource_nodes),
    HaArrayCount(f->task_nodes), f->flow);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowDebug(KHE_FLOW f, int verbosity, int indent, FILE *fp)       */
/*                                                                           */
/*  Debug print of f onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheFlowDebug(KHE_FLOW f, int verbosity, int indent, FILE *fp)
{
  KHE_FLOW_RESOURCE_NODE frn;  KHE_FLOW_TASK_NODE ftn;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheFlowHeaderDebug(f, fp);
    fprintf(fp, "\n");
    HaArrayForEach(f->resource_nodes, frn, i)
      KheFlowResourceNodeDebug(frn, verbosity, indent + 2, fp);
    HaArrayForEach(f->task_nodes, ftn, i)
      KheFlowTaskNodeDebug(ftn, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheFlowHeaderDebug(f, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFlowDebugFn(void *f, int verbosity, int indent, FILE *fp)        */
/*                                                                           */
/*  Untyped version of KheFlowDebug, suited to passing to the mmatch.        */
/*                                                                           */
/*****************************************************************************/

static void KheFlowDebugFn(void *f, int verbosity, int indent, FILE *fp)
{
  KheFlowDebug((KHE_FLOW) f, verbosity, indent, fp);
}

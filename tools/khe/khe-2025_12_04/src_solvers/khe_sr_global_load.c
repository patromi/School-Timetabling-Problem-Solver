
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
/*  FILE:         khe_sr_global_load.c                                       */
/*  DESCRIPTION:  Global load balancing                                      */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>

#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1	0
#define DEBUG2	0
#define DEBUG3	0
#define DEBUG4	0
#define DEBUG5	0

/*****************************************************************************/
/*                                                                           */
/*  Forward declarations of types                                            */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_glb_task_set_rec *KHE_GLB_TASK_SET;
typedef HA_ARRAY(KHE_GLB_TASK_SET) ARRAY_KHE_GLB_TASK_SET;

typedef struct khe_glb_resource_rec *KHE_GLB_RESOURCE;
typedef HA_ARRAY(KHE_GLB_RESOURCE) ARRAY_KHE_GLB_RESOURCE;

typedef struct khe_glb_node_rec *KHE_GLB_NODE;
typedef HA_ARRAY(KHE_GLB_NODE) ARRAY_KHE_GLB_NODE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_GLB_TASK_SET - a task set with some extra stuff                 */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_glb_task_set_rec {
  KHE_TASK_SET			task_set;
  int				index;
  /* KHE_GLB_RESOURCE		init_asst;  unused */
  KHE_GROUP_MONITOR		gm;
} *KHE_GLB_TASK_SET;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_GLB_RESOURCE - a resource with some extra stuff                 */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_RESOURCE_OVERLOADED,
  KHE_RESOURCE_FULLY_LOADED,
  KHE_RESOURCE_UNDERLOADED,
  KHE_RESOURCE_UNKNOWN,
} KHE_RESOURCE_LOAD_TYPE;

typedef struct khe_glb_resource_rec {
  KHE_RESOURCE			resource;
  int				index;
  KHE_RESOURCE_LOAD_TYPE	load_type;
  ARRAY_KHE_GLB_TASK_SET	task_sets;
  KHE_GROUP_MONITOR		gm;
} *KHE_GLB_RESOURCE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_GLB_EDGE - one edge in the graph                                */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_glb_edge_rec {
  KHE_COST			delta_cost;   /* cost after minus cost before */
  KHE_GLB_NODE			node;
} KHE_GLB_EDGE;

typedef HA_ARRAY(KHE_GLB_EDGE) ARRAY_KHE_GLB_EDGE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_GLB_NODE - a node in the graph                                  */
/*                                                                           */
/*  We build the graph by a breadth-first search starting from the source    */
/*  nodes, so every node is on a path from a source node.                    */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_NODE_SOURCE,
  KHE_NODE_INTERMEDIATE,
  KHE_NODE_SINK
} KHE_NODE_TYPE;

struct khe_glb_node_rec {
  KHE_NODE_TYPE			node_type;
  KHE_GLB_RESOURCE		resource;
  KHE_GLB_TASK_SET		task_set;
  ARRAY_KHE_GLB_EDGE		outgoing_edges;
  KHE_GLB_NODE			pred_on_path_from_source;
  KHE_COST			cost_decrease_from_source;
  bool				on_path_to_sink;
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_GLB_NODE_ENTRY - an entry (for one resource) in the node table       */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_glb_node_entry_rec {
  /* KHE_GLB_NODE		source_node; */
  ARRAY_KHE_GLB_NODE		other_nodes;
} *KHE_GLB_NODE_ENTRY;

typedef HA_ARRAY(KHE_GLB_NODE_ENTRY) ARRAY_KHE_GLB_NODE_ENTRY;


/*****************************************************************************/
/*                                                                           */
/*  KHE_GLB_NODE_TABLE - a table for looking up non-source nodes             */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_glb_node_table_rec {
  ARRAY_KHE_GLB_NODE_ENTRY	resource_entries;	/* one per resource */
} *KHE_GLB_NODE_TABLE;

typedef HA_ARRAY(KHE_GLB_NODE_TABLE) ARRAY_KHE_GLB_NODE_TABLE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_GLB_GRAPH - a graph for traversing to find an improving path    */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_glb_graph_rec {
  KHE_GLB_NODE_TABLE		node_table;
  ARRAY_KHE_GLB_NODE		all_nodes_bfs;
  /* ARRAY_KHE_GLB_NODE		source_nodes; */
  /* ARRAY_KHE_GLB_NODE		bfs_queue; */
  ARRAY_KHE_GLB_NODE		sink_nodes;
} *KHE_GLB_GRAPH;

typedef HA_ARRAY(KHE_GLB_GRAPH) ARRAY_KHE_GLB_GRAPH;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_GLB_SOLVER - a solver for global load balancing                 */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_glb_solver_rec {

  /* free lists */
  ARRAY_KHE_GLB_TASK_SET	task_set_free_list;
  ARRAY_KHE_GLB_NODE		node_free_list;
  ARRAY_KHE_GLB_NODE_ENTRY	node_entry_free_list;
  ARRAY_KHE_GLB_NODE_TABLE	node_table_free_list;
  ARRAY_KHE_GLB_GRAPH		graph_free_list;

  /* regular fields */
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_RESOURCE_TYPE		resource_type;
  KHE_OPTIONS			options;
  KHE_FRAME			frame;
  /* int			min_len; */
  int				max_len;
  bool				in_seq;
  ARRAY_KHE_GLB_RESOURCE	resources;
  KHE_TASK_SET			scratch_ts;
  int				next_ts_index;
} *KHE_GLB_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_GLB_TASK_SET"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskAddMonitors(KHE_TASK task, KHE_GROUP_MONITOR gm)             */
/*                                                                           */
/*  Add to gm the event resource monitors of task and the tasks assigned to  */
/*  task, directly or indirectly.  Make sure to not add any monitor twice.   */
/*                                                                           */
/*****************************************************************************/

static void KheTaskAddMonitors(KHE_TASK task, KHE_GROUP_MONITOR gm)
{
  KHE_EVENT_RESOURCE er;  int i;  KHE_MONITOR m;  KHE_SOLN soln;
  KHE_TASK child_task;

  /* do it for task */
  soln = KheTaskSoln(task);
  er = KheTaskEventResource(task);
  if( er != NULL )
  {
    for( i = 0;  i < KheSolnEventResourceMonitorCount(soln, er);  i++ )
    {
      m = KheSolnEventResourceMonitor(soln, er, i);
      if( !KheGroupMonitorHasChildMonitor(gm, m) )
	KheGroupMonitorAddChildMonitor(gm, m);
    }
  }

  /* do it for the tasks assigned, directly or indirectly, to task */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    KheTaskAddMonitors(child_task, gm);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP_MONITOR KheTaskSetGroupMonitor(KHE_TASK_SET ts, KHE_SOLN soln) */
/*                                                                           */
/*  Make and return a group monitor holding the event resource monitors      */
/*  that monitor ts.                                                         */
/*                                                                           */
/*****************************************************************************/

static KHE_GROUP_MONITOR KheTaskSetGroupMonitor(KHE_TASK_SET ts, KHE_SOLN soln)
{
  KHE_GROUP_MONITOR res;  int i;  KHE_TASK task;
  res = KheGroupMonitorMake(soln, 8, "task set");
  for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
  {
    task = KheTaskSetTask(ts, i);
    KheTaskAddMonitors(task, res);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GLB_TASK_SET KheGlbTaskSetMake(KHE_TASK_SET ts,                      */
/*    KHE_GLB_RESOURCE init_asst, KHE_GLB_SOLVER gs)                         */
/*                                                                           */
/*  Make a glb task set with these attributes.                               */
/*                                                                           */
/*****************************************************************************/

static KHE_GLB_TASK_SET KheGlbTaskSetMake(KHE_TASK_SET ts,
  /* KHE_GLB_RESOURCE init_asst, */ KHE_GLB_SOLVER gs)
{
  KHE_GLB_TASK_SET res;
  if( HaArrayCount(gs->task_set_free_list) > 0 )
    res = HaArrayLastAndDelete(gs->task_set_free_list);
  else
    HaMake(res, gs->arena);
  res->task_set = ts;
  res->index = gs->next_ts_index++;
  /* res->init_asst = init_asst; */
  res->gm = KheTaskSetGroupMonitor(ts, gs->soln);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGlbTaskSetDelete(KHE_GLB_TASK_SET gts, KHE_GLB_SOLVER gs)        */
/*                                                                           */
/*  Delete gts.                                                              */
/*                                                                           */
/*****************************************************************************/

static void KheGlbTaskSetDelete(KHE_GLB_TASK_SET gts, KHE_GLB_SOLVER gs)
{
  KheGroupMonitorDelete(gts->gm);
  KheTaskSetDelete(gts->task_set);
  HaArrayAddLast(gs->task_set_free_list, gts);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGlbTaskSetDebug(KHE_GLB_TASK_SET gts, int verbosity,             */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of gts onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheGlbTaskSetDebug(KHE_GLB_TASK_SET gts, int verbosity,
  int indent, FILE *fp)
{
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "%d:", gts->index);
  KheTaskSetDebug(gts->task_set, 2, -1, fp);
  if( indent > 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_GLB_RESOURCE"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_LOAD_TYPE KheGlbResourceLoadType(KHE_RESOURCE r,            */
/*    KHE_SOLN soln)                                                         */
/*                                                                           */
/*  Work out whether r is overloaded, fully loaded, or underloaded.          */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_LOAD_TYPE KheGlbResourceLoadType(KHE_RESOURCE r,
  KHE_SOLN soln)
{
  bool has_avail_busy, has_avail_workload;
  int avail_busy;  float avail_workload;
  has_avail_busy = KheResourceAvailableBusyTimes(soln, r, &avail_busy);
  has_avail_workload = KheResourceAvailableWorkload(soln, r, &avail_workload);
  if( (has_avail_busy && avail_busy < 0) ||
      (has_avail_workload && avail_workload < 0) )
    return KHE_RESOURCE_OVERLOADED;
  else if( (has_avail_busy && avail_busy == 0) ||
      (has_avail_workload && avail_workload == 0.0) )
    return KHE_RESOURCE_FULLY_LOADED;
  else
    return KHE_RESOURCE_UNDERLOADED;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP_MONITOR KheResourceGroupMonitor(KHE_RESOURCE r, KHE_SOLN soln) */
/*                                                                           */
/*  Make and return a group monitor holding the resource monitors that       */
/*  monitor r.                                                               */
/*                                                                           */
/*****************************************************************************/

static KHE_GROUP_MONITOR KheResourceGroupMonitor(KHE_RESOURCE r, KHE_SOLN soln)
{
  KHE_GROUP_MONITOR res;  KHE_MONITOR m;  int i;
  res = KheGroupMonitorMake(soln, 9, KheResourceId(r));
  for( i = 0;  i < KheSolnResourceMonitorCount(soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(soln, r, i);
    KheGroupMonitorAddChildMonitor(res, m);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GLB_RESOURCE KheGlbResourceMake(KHE_RESOURCE r, KHE_GLB_SOLVER gs)   */
/*                                                                           */
/*  Make a new glb resource for r.  The load_type and task_sets fields       */
/*  have placeholder values; they are reset before each solve.               */
/*                                                                           */
/*****************************************************************************/

static KHE_GLB_RESOURCE KheGlbResourceMake(KHE_RESOURCE r, KHE_GLB_SOLVER gs)
{
  KHE_GLB_RESOURCE res;
  HaMake(res, gs->arena);
  res->resource = r;
  res->index = KheResourceResourceTypeIndex(r);
  res->load_type = KHE_RESOURCE_UNKNOWN;
  HaArrayInit(res->task_sets, gs->arena);
  res->gm = KheResourceGroupMonitor(r, gs->soln);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetNextRun(KHE_TASK_SET ts, KHE_FRAME frame,                 */
/*    int *first_index, int *last_index)                                     */
/*                                                                           */
/*  Change *first_index and *last_index (these are indexes into ts) so that  */
/*  they delimit the next run of tasks on consecutive days.  Or if there     */
/*  are no more runs, return false.                                          */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskSetNextRun(KHE_TASK_SET ts, KHE_FRAME frame,
  int *first_index, int *last_index)
{
  KHE_INTERVAL in, prev_in;  int i, new_first_index;  KHE_TASK task;

  /* if at end of task set, no more runs */
  if( *last_index == KheTaskSetTaskCount(ts) - 1 )
    return *first_index = -1, *last_index = -1, false;

  /* start of run is at next index after *last_index */
  new_first_index = *last_index + 1;
  task = KheTaskSetTask(ts, new_first_index);
  prev_in = KheTaskInterval(task, frame);
  for( i = new_first_index + 1;  i < KheTaskSetTaskCount(ts);  i++ )
  {
    task = KheTaskSetTask(ts, i);
    in = KheTaskInterval(task, frame);
    if( in.first > prev_in.last + 1 )
      break;
    prev_in = in;
  }
  return *first_index = new_first_index, *last_index = i - 1, true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetLenRun(KHE_TASK_SET ts, KHE_FRAME frame,                  */
/*    int first_index, int last_index, int len, int *res_index)              */
/*                                                                           */
/*  If there is a subsequence of the sequence of tasks in ts which           */
/*  begins at first_index, ends at or before last_index, and has             */
/*  length len (in the sense of number of days covered, not number of        */
/*  tasks) then return true with *res_index set to the index in ts of the    */
/*  last element of this subsequence.                                        */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskSetLenRun(KHE_TASK_SET ts, KHE_FRAME frame,
  int first_index, int last_index, int len, int *res_index)
{
  KHE_TASK task;  int i;  KHE_INTERVAL in;
  in = KheIntervalMake(1, 0);
  for( i = first_index;  i <= last_index;  i++ )
  {
    task = KheTaskSetTask(ts, i);
    in = KheIntervalUnion(in, KheTaskInterval(task, frame));
    if( KheIntervalLength(in) > len )
      return *res_index = -1, false;
    else if( KheIntervalLength(in) == len )
      return *res_index = i, true;
  }
  return *res_index = -1, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGlbResourceInitForSolve(KHE_GLB_RESOURCE gr, KHE_GLB_SOLVER gs)  */
/*                                                                           */
/*  Initialize gr for a run.  This includes updating gr's load type, and     */
/*  adding task sets to gr.                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheGlbResourceInitForSolve(KHE_GLB_RESOURCE gr, KHE_GLB_SOLVER gs)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_TASK_SET ts;
  int first_index, last_index, i, j, k, len;  KHE_GLB_TASK_SET gts;

  /* update gr's load type */
  gr->load_type = KheGlbResourceLoadType(gr->resource, gs->soln);

  /* delete any old task sets */
  while( HaArrayCount(gr->task_sets) > 0 )
  {
    gts = HaArrayLastAndDelete(gr->task_sets);
    KheGlbTaskSetDelete(gts, gs);
  }

  /* get the proper root tasks assigned gr into gs->scratch_ts */
  rtm = KheResourceTimetableMonitor(gs->soln, gr->resource);
  KheTaskSetClear(gs->scratch_ts);
  KheResourceTimetableMonitorAddProperRootTasksInTimeGroup(rtm, NULL, true,
    gs->scratch_ts);

  /* handle each interval length from 1 to gs->max_len */
  for( len = 1;  len <= gs->max_len;  len++ )
  {
    /* handle each run of consecutive tasks within gs->scratch_ts */
    last_index = -1;
    while( KheTaskSetNextRun(gs->scratch_ts,gs->frame,&first_index,&last_index))
    {
      /* here ts[first_index, last_index] is a run */
      for( i = first_index;  i <= last_index;  i++ )
	if( KheTaskSetLenRun(gs->scratch_ts, gs->frame, i, last_index, len, &j)
	    && (gs->in_seq || i == first_index || j == last_index) )
	{
	  /* build a new task set for tasks from i to j */
	  ts = KheTaskSetMake(gs->soln);
	  for( k = i;  k <= j;  k++ )
	    KheTaskSetAddTask(ts, KheTaskSetTask(gs->scratch_ts, k));
	  gts = KheGlbTaskSetMake(ts, /* gr, */ gs);
	  HaArrayAddLast(gr->task_sets, gts);
	}
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheResourceLoadTypeShow(KHE_RESOURCE_LOAD_TYPE load_type)          */
/*                                                                           */
/*  Return a string representing load_type;                                  */
/*                                                                           */
/*****************************************************************************/

static char *KheResourceLoadTypeShow(KHE_RESOURCE_LOAD_TYPE load_type)
{
  switch( load_type )
  {
    case KHE_RESOURCE_OVERLOADED:	return "overloaded";
    case KHE_RESOURCE_FULLY_LOADED:	return "fully_loaded";
    case KHE_RESOURCE_UNDERLOADED:	return "underloaded";
    case KHE_RESOURCE_UNKNOWN:		return "unknown";

    default:
      HnAbort("KheResourceLoadTypeShow internal error (%d)", load_type);
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGlbResourceDebugHeader(KHE_GLB_RESOURCE gr, FILE *fp)            */
/*                                                                           */
/*  Print the header of the debug print of gr onto fp.                       */
/*                                                                           */
/*****************************************************************************/

static void KheGlbResourceDebugHeader(KHE_GLB_RESOURCE gr, FILE *fp)
{
  fprintf(fp, "%s(%d, %s, %d task_sets)", KheResourceId(gr->resource),
    gr->index, KheResourceLoadTypeShow(gr->load_type),
    HaArrayCount(gr->task_sets));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGlbResourceDebug(KHE_GLB_RESOURCE gr, int verbosity,             */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of gr onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheGlbResourceDebug(KHE_GLB_RESOURCE gr, int verbosity,
  int indent, FILE *fp)
{
  KHE_GLB_TASK_SET gts;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheGlbResourceDebugHeader(gr, fp);
    fprintf(fp, "\n");
    if( verbosity >= 2 )
      HaArrayForEach(gr->task_sets, gts, i)
	KheGlbTaskSetDebug(gts, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheGlbResourceDebugHeader(gr, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_GLB_NODE"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_GLB_NODE KheGlbNodeMake(KHE_NODE_TYPE node_type,                     */
/*    KHE_GLB_RESOURCE gr, KHE_GLB_TASK_SET gts,                             */
/*    KHE_GLB_NODE pred_on_path_from_source,                                 */
/*    KHE_COST cost_decrease_from_source, KHE_GLB_SOLVER gs)                 */
/*                                                                           */
/*  Make a node with these attributes.                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_GLB_NODE KheGlbNodeMake(KHE_NODE_TYPE node_type,
  KHE_GLB_RESOURCE gr, KHE_GLB_TASK_SET gts,
  KHE_GLB_NODE pred_on_path_from_source,
  KHE_COST cost_decrease_from_source, KHE_GLB_SOLVER gs)
{
  KHE_GLB_NODE res;

  if( HaArrayCount(gs->node_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(gs->node_free_list);
    HaArrayClear(res->outgoing_edges);
  }
  else
  {
    HaMake(res, gs->arena);
    HaArrayInit(res->outgoing_edges, gs->arena);
  }
  res->node_type = node_type;
  res->resource = gr;
  res->task_set = gts;
  res->pred_on_path_from_source = pred_on_path_from_source;
  res->cost_decrease_from_source = cost_decrease_from_source;
  res->on_path_to_sink = false;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGlbNodeDelete(KHE_GLB_NODE gn, KHE_GLB_SOLVER gs)                */
/*                                                                           */
/*  Delete gn.                                                               */
/*                                                                           */
/*****************************************************************************/

static void KheGlbNodeDelete(KHE_GLB_NODE gn, KHE_GLB_SOLVER gs)
{
  HaArrayAddLast(gs->node_free_list, gn);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheGlbNodeTypedCmp(KHE_GLB_NODE gn1, KHE_GLB_NODE gn2)               */
/*                                                                           */
/*  Typed comparison function for sorting an array of nodes by increasing    */
/*  cost_decrease_from_source.                                               */
/*                                                                           */
/*****************************************************************************/

static int KheGlbNodeTypedCmp(KHE_GLB_NODE gn1, KHE_GLB_NODE gn2)
{
  return KheCostCmp(gn1->cost_decrease_from_source,
    gn2->cost_decrease_from_source);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheGlbNodeCmp(const void *t1, const void *t2)                        */
/*                                                                           */
/*  Untyped comparison function for sorting an array of nodes by increasing  */
/*  cost_decrease_from_source.                                               */
/*                                                                           */
/*****************************************************************************/

static int KheGlbNodeCmp(const void *t1, const void *t2)
{
  KHE_GLB_NODE gn1 = * (KHE_GLB_NODE *) t1;
  KHE_GLB_NODE gn2 = * (KHE_GLB_NODE *) t2;
  return KheGlbNodeTypedCmp(gn1, gn2);
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheGlbNodeTypeShow(KHE_NODE_TYPE gnt)                              */
/*                                                                           */
/*  Show node type gnt.                                                      */
/*                                                                           */
/*****************************************************************************/

static char *KheGlbNodeTypeShow(KHE_NODE_TYPE gnt)
{
  switch( gnt )
  {
    case KHE_NODE_SOURCE:		return "Source";
    case KHE_NODE_INTERMEDIATE:		return "Intermediate";
    case KHE_NODE_SINK:			return "Sink";
    default:				return "?";
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGlbNodeDebugHeader(KHE_GLB_NODE gn, FILE *fp)                    */
/*                                                                           */
/*  Debug print of the header part of gn onto fp.                            */
/*                                                                           */
/*****************************************************************************/

static void KheGlbNodeDebugHeader(KHE_GLB_NODE gn, FILE *fp)
{
  fprintf(fp, "%s", KheGlbNodeTypeShow(gn->node_type));
  if( gn->pred_on_path_from_source != NULL )
    fprintf(fp, " {-%.5f}", KheCostShow(- gn->cost_decrease_from_source));
  fprintf(fp, " %s", KheResourceId(gn->resource->resource));
  if( gn->task_set != NULL )
  {
    fprintf(fp, "+");
    KheTaskSetDebug(gn->task_set->task_set, 2, -1, fp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGlbNodeDebug(KHE_GLB_NODE gn, int verbosity,                     */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of gn onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheGlbNodeDebug(KHE_GLB_NODE gn, int verbosity,
  int indent, FILE *fp)
{
  KHE_GLB_EDGE edge;  int i, count;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheGlbNodeDebugHeader(gn, fp);
    if( HaArrayCount(gn->outgoing_edges) > 0 )
    {
      fprintf(fp, "\n");
      count = 0;
      HaArrayForEach(gn->outgoing_edges, edge, i)
      {
	fprintf(fp, "%*s  ", indent, "");
	if( edge.delta_cost == 0 )
	  fprintf(fp, "--> ");
	else
	  fprintf(fp, "-{-%.5f}-> ", KheCostShow(- edge.delta_cost));
	KheGlbNodeDebug(edge.node, 2, -1, fp);
	fprintf(fp, "\n");
	count++;
      }
      fprintf(fp, "%*s] (%d edges printed)\n", indent, "", count);
    }
    else if( gn->node_type == KHE_NODE_SINK )
      fprintf(fp, " ]\n");
    else
      fprintf(fp, " ] (0 edges)\n");
  }
  else
    KheGlbNodeDebugHeader(gn, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_GLB_NODE_ENTRY"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_GLB_NODE_ENTRY KheGlbNodeEntryMake(HA_ARENA a)                       */
/*                                                                           */
/*  Make a node table entry object.                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_GLB_NODE_ENTRY KheGlbNodeEntryMake(KHE_GLB_SOLVER gs)
{
  KHE_GLB_NODE_ENTRY res;
  if( HaArrayCount(gs->node_entry_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(gs->node_entry_free_list);
    HaArrayClear(res->other_nodes);
  }
  else
  {
    HaMake(res, gs->arena);
    HaArrayInit(res->other_nodes, gs->arena);
  }
  /* res->source_node = NULL; */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGlbNodeEntryDelete(KHE_GLB_NODE_ENTRY gne, KHE_GLB_SOLVER gs)    */
/*                                                                           */
/*  Delete gne, *not* including the nodes it contains.                       */
/*                                                                           */
/*****************************************************************************/

static void KheGlbNodeEntryDelete(KHE_GLB_NODE_ENTRY gne, KHE_GLB_SOLVER gs)
{
  /* KHE_GLB_NODE gn;  int i; */

  /* delete gne's nodes */
  /* *** we're not deleting nodes here
  if( gne->source_node != NULL )
    KheGlbNodeDelete(gne->source_node, gs);
  HaArrayForEach(gne->other_nodes, gn, i)
    if( gn != NULL )
      KheGlbNodeDelete(gn, gs);
  *** */

  /* delete gne itself */
  HaArrayAddLast(gs->node_entry_free_list, gne);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGlbNodeEntryInsertNode(KHE_GLB_NODE_ENTRY gne, KHE_GLB_NODE gn)  */
/*                                                                           */
/*  Insert gn into gne.                                                      */
/*                                                                           */
/*****************************************************************************/

static void KheGlbNodeEntryInsertNode(KHE_GLB_NODE_ENTRY gne, KHE_GLB_NODE gn)
{
  KHE_GLB_TASK_SET gts;
  gts = gn->task_set;
  HnAssert(gts != NULL, "KheGlbNodeEntryInsertNode internal error");
  HaArrayFill(gne->other_nodes, gts->index + 1, NULL);
  HnAssert(HaArray(gne->other_nodes, gts->index) == NULL,
    "KheGlbNodeEntryInsertNode internal error 2");
  HaArrayPut(gne->other_nodes, gts->index, gn);

  /* *** old version from when source nodes were stored in the table
  if( gts == NULL )
  {
    ** gn is a source node **
    HnAssert(gne->source_node == NULL,
      "KheGlbNodeEntryInsertNode internal error 1");
    gne->source_node = gn;
  }
  else
  {
    ** gn is an intermediate or sink node **
    HaArrayFill(gne->other_nodes, gts->index + 1, NULL);
    HnAssert(HaArray(gne->other_nodes, gts->index) == NULL,
      "KheGlbNodeEntryInsertNode internal error 2");
    HaArrayPut(gne->other_nodes, gts->index, gn);
  }
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGlbNodeEntryRetrieveNode(KHE_GLB_NODE_ENTRY gne,                 */
/*    KHE_GLB_TASK_SET gts, KHE_GLB_NODE *gn)                                */
/*                                                                           */
/*  Retrieve the node with task set gts from gne.                            */
/*                                                                           */
/*****************************************************************************/

static bool KheGlbNodeEntryRetrieveNode(KHE_GLB_NODE_ENTRY gne,
  KHE_GLB_TASK_SET gts, KHE_GLB_NODE *gn)
{
  HnAssert(gts != NULL, "KheGlbNodeEntryRetrieveNode internal error");
  HaArrayFill(gne->other_nodes, gts->index + 1, NULL);
  *gn = HaArray(gne->other_nodes, gts->index);
  return *gn != NULL;

  /* *** old version from when source nodes were stored in the table
  if( gts == NULL )
  {
    ** *gn is gne's source node **
    *gn = gne->source_node;
  }
  else
  {
    ** *gn is an intermediate or sink node **
    HaArrayFill(gne->other_nodes, gts->index + 1, NULL);
    *gn = HaArray(gne->other_nodes, gts->index);
  }
  return *gn != NULL;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_GLB_NODE_TABLE"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_GLB_NODE_TABLE KheGlbNodeTableMake(KHE_GLB_SOLVER gs)                */
/*                                                                           */
/*  Make a new node table object.                                            */
/*                                                                           */
/*****************************************************************************/

static KHE_GLB_NODE_TABLE KheGlbNodeTableMake(KHE_GLB_SOLVER gs)
{
  KHE_GLB_NODE_TABLE res;
  if( HaArrayCount(gs->node_table_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(gs->node_table_free_list);
    HaArrayClear(res->resource_entries);
  }
  else
  {
    HaMake(res, gs->arena);
    HaArrayInit(res->resource_entries, gs->arena);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGlbNodeTableInsertNode(KHE_GLB_NODE_TABLE gnt, KHE_GLB_NODE gn)  */
/*                                                                           */
/*  Add gn to gnt.                                                           */
/*                                                                           */
/*****************************************************************************/

static void KheGlbNodeTableInsertNode(KHE_GLB_NODE_TABLE gnt, KHE_GLB_NODE gn,
  KHE_GLB_SOLVER gs)
{
  KHE_GLB_RESOURCE gr;  KHE_GLB_NODE_ENTRY gne;

  /* find or make gne, an entry for gn's resource */
  gr = gn->resource;
  HaArrayFill(gnt->resource_entries, gr->index + 1, NULL);
  gne = HaArray(gnt->resource_entries, gr->index);
  if( gne == NULL )
  {
    gne = KheGlbNodeEntryMake(gs);
    HaArrayPut(gnt->resource_entries, gr->index, gne);
  }

  /* add gn to gne */
  KheGlbNodeEntryInsertNode(gne, gn);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGlbNodeTableRetrieveNode(KHE_GLB_NODE_TABLE gnt,                 */
/*    KHE_GLB_RESOURCE gr, KHE_GLB_TASK_SET gts, KHE_GLB_NODE *gn)           */
/*                                                                           */
/*  If gnt contains a node with resource gr and task set gts, return         */
/*  true and set *gn to that node.  Otherwise return false.                  */
/*                                                                           */
/*  Here gr must be non-NULL, but gts may be NULL.                           */
/*                                                                           */
/*****************************************************************************/

static bool KheGlbNodeTableRetrieveNode(KHE_GLB_NODE_TABLE gnt,
  KHE_GLB_RESOURCE gr, KHE_GLB_TASK_SET gts, KHE_GLB_NODE *gn)
{
  KHE_GLB_NODE_ENTRY gne;

  /* find gne, an entry for gr, or return false if not present */
  HaArrayFill(gnt->resource_entries, gr->index + 1, NULL);
  gne = HaArray(gnt->resource_entries, gr->index);
  if( gne == NULL )
    return *gn = NULL, false;

  /* find the node we want within gne */
  return KheGlbNodeEntryRetrieveNode(gne, gts, gn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGlbNodeTableDelete(KHE_GLB_NODE_TABLE gnt, KHE_GLB_SOLVER gs)    */
/*                                                                           */
/*  Delete gnt but not the nodes it contains.                                */
/*                                                                           */
/*****************************************************************************/

static void KheGlbNodeTableDelete(KHE_GLB_NODE_TABLE gnt, KHE_GLB_SOLVER gs)
{
  KHE_GLB_NODE_ENTRY gne;  int i;

  /* delete gnt's entries */
  HaArrayForEach(gnt->resource_entries, gne, i)
    if( gne != NULL )
      KheGlbNodeEntryDelete(gne, gs);

  /* delete gnt itself */
  HaArrayAddLast(gs->node_table_free_list, gnt);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_GLB_GRAPH"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_GLB_GRAPH KheGlbGraphMake(KHE_GLB_SOLVER gs)                         */
/*                                                                           */
/*  Make a new, empty graph.                                                 */
/*                                                                           */
/*****************************************************************************/

static KHE_GLB_GRAPH KheGlbGraphMake(KHE_GLB_SOLVER gs)
{
  KHE_GLB_GRAPH res;
  if( HaArrayCount(gs->graph_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(gs->graph_free_list);
    HaArrayClear(res->all_nodes_bfs);
    /* HaArrayClear(res->source_nodes); */
    /* HaArrayClear(res->bfs_queue); */
    HaArrayClear(res->sink_nodes);
  }
  else
  {
    HaMake(res, gs->arena);
    HaArrayInit(res->all_nodes_bfs, gs->arena);
    /* HaArrayInit(res->source_nodes, gs->arena); */
    /* HaArrayInit(res->bfs_queue, gs->arena); */
    HaArrayInit(res->sink_nodes, gs->arena);
  }
  res->node_table = KheGlbNodeTableMake(gs);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGlbGraphDelete(KHE_GLB_GRAPH gg, KHE_GLB_SOLVER gs)              */
/*                                                                           */
/*  Delete gg, recycling its objects through free lists in gs.               */
/*                                                                           */
/*****************************************************************************/

static void KheGlbGraphDelete(KHE_GLB_GRAPH gg, KHE_GLB_SOLVER gs)
{
  int i;  KHE_GLB_NODE gn;

  /* delete the node table */
  KheGlbNodeTableDelete(gg->node_table, gs);

  /* delete the nodes */
  HaArrayForEach(gg->all_nodes_bfs, gn, i)
    KheGlbNodeDelete(gn, gs);

  /* delete the graph object itself */
  HaArrayAddLast(gs->graph_free_list, gg);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheGlbGraphOneStepCost(KHE_GLB_NODE gn1, KHE_GLB_NODE gn2)      */
/*                                                                           */
/*  Return the cost of one step from gn1 to gn2.  This includes:             */
/*                                                                           */
/*    * The total cost of the resource monitors that monitor gn1->resource;  */
/*                                                                           */
/*    * The total cost of the event resource monitors that monitor the       */
/*      tasks of gn2->task-set;                                              */
/*                                                                           */
/*    * If gn2 is a sink node, the total cost of the resource monitors       */
/*      that monitor gn2->resource.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_COST KheGlbGraphOneStepCost(KHE_GLB_NODE gn1, KHE_GLB_NODE gn2)
{
  KHE_GLB_TASK_SET gts2;  KHE_GLB_RESOURCE gr1, gr2;
  gr1 = gn1->resource;
  gr2 = gn2->resource;
  gts2 = gn2->task_set;
  return KheMonitorCost((KHE_MONITOR) gr1->gm) +
    KheMonitorCost((KHE_MONITOR) gts2->gm) +
    (gn2->node_type==KHE_NODE_SINK ? KheMonitorCost((KHE_MONITOR) gr2->gm) : 0);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheGlbOneStep(KHE_GLB_NODE gn1, KHE_GLB_NODE gn2)                   */
/*                                                                           */
/*  Carry out the operation which takes the step from gn1 to gn2:            */
/*                                                                           */
/*    * Assign gn1->resource to gn1->task_set (if gn1->task_set != NULL);    */
/*                                                                           */
/*    * Move gn2->task_set from gn1->resource to gn2->resource.              */
/*                                                                           */
/*  Return true if the operation succeeds.                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheGlbOneStep(KHE_GLB_NODE gn1, KHE_GLB_NODE gn2)
{
  KHE_GLB_TASK_SET gts1, gts2;  KHE_GLB_RESOURCE gr1, gr2;
  gr1 = gn1->resource;
  gr2 = gn2->resource;
  gts1 = gn1->task_set;
  gts2 = gn2->task_set;
  return (gts1 == NULL || KheTaskSetMoveResource(gts1->task_set, gr1->resource))
    && KheTaskSetMoveResource(gts2->task_set, gr2->resource);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheGlbGraphAcceptsEdge(KHE_GLB_GRAPH gg, KHE_GLB_NODE gn1,          */
/*    KHE_GLB_NODE gn2, KHE_GLB_SOLVER gs)                                   */
/*                                                                           */
/*  Return true if an edge should be drawn from gn1 to gn2.                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheGlbGraphAcceptsEdge(KHE_GLB_GRAPH gg, KHE_GLB_NODE gn1,
  KHE_GLB_NODE gn2, KHE_GLB_SOLVER gs, KHE_COST *delta_cost)
{
  KHE_COST cost_before;  KHE_MARK mark;  bool res;

  ** resources must be different (also implies that the nodes are different) **
  if( gn1->resource == gn2->resource )
    return false;

  ** gn2 cannot be a source node, and gn1 cannot be a sink node **
  if( gn2->node_type == KHE_NODE_SOURCE || gn1->node_type == KHE_NODE_SINK )
    return false;

  ** step operation must succeed and not increase cost **
  mark = KheMarkBegin(gs->soln);
  cost_before = KheGlbGraphOneStepCost(gn1, gn2);
  if( KheGlbOneStep(gn1, gn2) )
  {
    *delta_cost = KheGlbGraphOneStepCost(gn1, gn2) - cost_before;
    if( gn1->node_type == KHE_NODE_SOURCE )
      res = (*delta_cost < 0);
    else
      res = (*delta_cost <= 0);
  }
  else
  {
    *delta_cost = 1;  ** keep compiler happy; actually undefined **
    res = false;
  }
  KheMarkEnd(mark, true);

  ** debug and return **
  ** ***
  if( DEBUG4 && res )
  {
    fprintf(stderr, "  edge ");
    KheGlbNodeDebug(gn1, 2, -1, stderr);
    if( *delta_cost == 0 )
      fprintf(stderr, " --(0)--> ");
    else
      fprintf(stderr, " --(-%.5f)--> ", KheCostShow(- *delta_cost));
    KheGlbNodeDebug(gn2, 2, -1, stderr);
    fprintf(stderr, "\n");
  }
  *** **
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheGlbGraphOneStepCost(KHE_GLB_NODE gn1,                        */
/*    KHE_GLB_RESOURCE gr2, KHE_GLB_TASK_SET gts2)                           */
/*                                                                           */
/*  Return the cost of one step from gn1 to gr2+gts2.  This includes:        */
/*                                                                           */
/*    * The total cost of the resource monitors that monitor gn1->resource;  */
/*                                                                           */
/*    * The total cost of the event resource monitors that monitor the       */
/*      tasks of gts2;                                                       */
/*                                                                           */
/*    * If gr2 is underloaded, making gr2+gts2 a sink node, the total cost   */
/*      of the resource monitors that monitor gr2.                           */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheGlbGraphOneStepCost(KHE_GLB_NODE gn1,
  KHE_GLB_RESOURCE gr2, KHE_GLB_TASK_SET gts2)
{
  KHE_GLB_RESOURCE gr1;
  gr1 = gn1->resource;
  return KheMonitorCost((KHE_MONITOR) gr1->gm) +
    KheMonitorCost((KHE_MONITOR) gts2->gm) +
    (gr2->load_type == KHE_RESOURCE_UNDERLOADED ?
      KheMonitorCost((KHE_MONITOR) gr2->gm) : 0);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGlbOneStep(KHE_GLB_NODE gn1, KHE_GLB_RESOURCE gr2,               */
/*    KHE_GLB_TASK_SET gts2)                                                 */
/*                                                                           */
/*  Carry out the operation which takes the step from gn1 to gr2+gts2:       */
/*                                                                           */
/*    * Assign gn1->resource to gn1->task_set (if gn1->task_set != NULL);    */
/*                                                                           */
/*    * Move gts2 from gn1->resource to gr2.                                 */
/*                                                                           */
/*  Return true if the operation succeeds.                                   */
/*                                                                           */
/*****************************************************************************/

static bool KheGlbOneStep(KHE_GLB_NODE gn1, KHE_GLB_RESOURCE gr2,
  KHE_GLB_TASK_SET gts2)
{
  KHE_GLB_TASK_SET gts1;  KHE_GLB_RESOURCE gr1;
  gr1 = gn1->resource;
  gts1 = gn1->task_set;
  return (gts1 == NULL || KheTaskSetMoveResource(gts1->task_set, gr1->resource))
    && KheTaskSetMoveResource(gts2->task_set, gr2->resource);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGlbGraphAcceptsEdge(KHE_GLB_GRAPH gg, KHE_GLB_NODE gn1,          */
/*    KHE_GLB_RESOURCE gr2, KHE_GLB_TASK_SET gts2, KHE_GLB_SOLVER gs,        */
/*    KHE_COST *delta_cost)                                                  */
/*                                                                           */
/*  If an edge can be drawn from gn1 to gr2+gts2, return true and set        */
/*  *delta_cost to its cost.   Otherwise return false.                       */
/*                                                                           */
/*****************************************************************************/

static bool KheGlbGraphAcceptsEdge(KHE_GLB_GRAPH gg, KHE_GLB_NODE gn1,
  KHE_GLB_RESOURCE gr2, KHE_GLB_TASK_SET gts2, KHE_GLB_SOLVER gs,
  KHE_COST *delta_cost)
{
  KHE_COST cost_before;  KHE_MARK mark;  bool res;

  /* resources must be different (also implies that the nodes are different) */
  HnAssert(gn1->resource != gr2, "KheGlbGraphAcceptsEdge internal error 1");

  /* gn1 cannot be a sink node */
  HnAssert(gn1->node_type != KHE_NODE_SINK,
    "KheGlbGraphAcceptsEdge internal error 2");

  /* gr2 cannot be overloaded (that would mean gn2 is a source node) */
  if( gr2->load_type == KHE_RESOURCE_OVERLOADED )
    return false;

  /* step operation must succeed and not increase cost */
  mark = KheMarkBegin(gs->soln);
  cost_before = KheGlbGraphOneStepCost(gn1, gr2, gts2);
  if( KheGlbOneStep(gn1, gr2, gts2) )
  {
    *delta_cost = KheGlbGraphOneStepCost(gn1, gr2, gts2) - cost_before;
    if( gn1->node_type == KHE_NODE_SOURCE )
      res = (*delta_cost < 0);
    else
      res = (*delta_cost <= 0);
  }
  else
  {
    *delta_cost = 1;  /* keep compiler happy; actually undefined */
    res = false;
  }
  KheMarkEnd(mark, true);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGlbGraphAddEdge(KHE_GLB_NODE gn1, KHE_GLB_NODE gn2)              */
/*                                                                           */
/*  Add an edge from gn1 to gn2.                                             */
/*                                                                           */
/*****************************************************************************/

static void KheGlbGraphAddEdge(KHE_GLB_NODE gn1, KHE_COST delta_cost,
  KHE_GLB_NODE gn2)
{
  KHE_GLB_EDGE edge;
  edge.delta_cost = delta_cost;
  edge.node = gn2;
  HaArrayAddLast(gn1->outgoing_edges, edge);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGlbGraphBreadthFirstSearch(KHE_GLB_GRAPH gg)                     */
/*                                                                           */
/*  Carry out a breadth-first search in gg, assuming that the                */
/*  parent_on_path fields are NULL in all nodes.                             */
/*                                                                           */
/*****************************************************************************/

/* *** merged into graph building now
static void KheGlbGraphBreadthFirstSearch(KHE_GLB_GRAPH gg)
{
  int i, j;  KHE_GLB_NODE gn, gn2;  KHE_GLB_EDGE edge;

  if( DEBUG4 )
    fprintf(stderr, "[ KheGlbGraphBreadthFirstSearch(gg)\n");

  ** copy the source nodes into the bfs queue **
  HaArrayClear(gg->bfs_queue);
  HaArrayAppend(gg->bfs_queue, gg->source_nodes, i);

  ** initially, no sink nodes are reachable **
  HaArrayClear(gg->sink_nodes);

  ** run the main algorithm **
  HaArrayForEach(gg->bfs_queue, gn, i)
  {
    if( DEBUG4 )
    {
      fprintf(stderr, "  reached ");
      KheGlbNodeDebug(gn, false, 2, -1, stderr);
      fprintf(stderr, "\n");
    }

    ** follow outgoing edges from gn **
    HaArrayForEach(gn->outgoing_edges, edge, j)
      if( edge.node->pred_on_path_from_source == NULL )
      {
	** first touch of this node **
        edge.node->pred_on_path_from_source = gn;
	edge.node->cost_decrease_from_source =
	  gn->cost_decrease_from_source + edge.delta_cost;
	HaArrayAddLast(gg->bfs_queue, edge.node);
      }

    ** if sink, remember it and mark all nodes back to source **
    if( gn->node_type == KHE_NODE_SINK )
    {
      HaArrayAddLast(gg->sink_nodes, gn);
      for( gn2 = gn;  gn2 != NULL;  gn2 = gn2->pred_on_path_from_source )
        gn2->on_path_to_sink = true;
    }
  }

  ** sort the reachable sink nodes by increasing cost decrease **
  HaArraySort(gg->sink_nodes, &KheGlbNodeCmp);
  if( DEBUG4 )
    fprintf(stderr, "] KheGlbGraphBreadthFirstSearch returning\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheGlbTrySinkNode(KHE_GLB_NODE gn, KHE_GLB_SOLVER gs)               */
/*                                                                           */
/*  Try to apply the operations of the path ending at sink node gn.  If      */
/*  they all succeeded and reduced the cost of the solution, leave the       */
/*  changes in place and return true.  Otherwise undo any changes and        */
/*  return false.                                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheGlbTrySinkNode(KHE_GLB_NODE gn, KHE_GLB_SOLVER gs)
{
  KHE_MARK mark;  KHE_COST init_cost;  bool res;
  KHE_RESOURCE r;  KHE_TASK_SET ts;

  /* set init_cost and start a mark */
  if( DEBUG5 )
  {
    fprintf(stderr, "[ KheGlbTrySinkNode(");
    KheGlbNodeDebug(gn, 2, -1, stderr);
    fprintf(stderr, ", gs)\n");
  }
  init_cost = KheSolnCost(gs->soln);
  mark = KheMarkBegin(gs->soln);

  /* apply the moves recorded by every edge on the path back to the source */
  for( ; gn->node_type != KHE_NODE_SOURCE;  gn = gn->pred_on_path_from_source )
  {
    r = gn->resource->resource;
    ts = gn->task_set->task_set;
    if( !KheTaskSetMoveResource(ts, r) )
    {
      if( DEBUG5 )
      {
	fprintf(stderr, "] KheGlbTrySinkNode returning false at ");
	KheGlbNodeDebug(gn, 2, -1, stderr);
	fprintf(stderr, "\n");
      }
      KheMarkEnd(mark, true);
      return false;
    }
  }

  /* success if solution cost has been reduced */
  res = (KheSolnCost(gs->soln) < init_cost);
  KheMarkEnd(mark, !res);
  if( DEBUG5 )
    fprintf(stderr, "] KheGlbTrySinkNode returning %s (%.5f %s %.5f)\n",
      bool_show(res), KheCostShow(KheSolnCost(gs->soln)),
      res ? "<" : ">=", KheCostShow(init_cost));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGlbGraphSolve(KHE_GLB_GRAPH gg, KHE_GLB_SOLVER gs)               */
/*                                                                           */
/*  Solve gg, returning true if that improved the solution.                  */
/*                                                                           */
/*****************************************************************************/

static bool KheGlbGraphSolve(KHE_GLB_GRAPH gg, KHE_GLB_SOLVER gs)
{
  KHE_GLB_NODE gn;  int i;
  HaArrayForEach(gg->sink_nodes, gn, i)
    if( KheGlbTrySinkNode(gn, gs) )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGlbGraphDebugHeader(KHE_GLB_GRAPH gg, FILE *fp)                  */
/*                                                                           */
/*  Print the header part of the debug print of gg onto fp.                  */
/*                                                                           */
/*****************************************************************************/

static void KheGlbGraphDebugHeader(KHE_GLB_GRAPH gg, FILE *fp)
{
  fprintf(fp, "GlbGraph(%d nodes)", HaArrayCount(gg->all_nodes_bfs));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGlbGraphDebug(KHE_GLB_GRAPH gg, int verbosity,                   */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of gg onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheGlbGraphDebug(KHE_GLB_GRAPH gg, int verbosity,
  int indent, FILE *fp)
{
  KHE_GLB_NODE gn;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheGlbGraphDebugHeader(gg, fp);
    fprintf(fp, "\n");
    HaArrayForEach(gg->all_nodes_bfs, gn, i)
      KheGlbNodeDebug(gn, verbosity, indent + 2, fp);
    if( HaArrayCount(gg->sink_nodes) > 0 )
    {
      fprintf(fp, "%*s  sink nodes:\n", indent, "");
      HaArrayForEach(gg->sink_nodes, gn, i)
	KheGlbNodeDebug(gn, verbosity, indent + 2, fp);
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheGlbGraphDebugHeader(gg, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_GLB_SOLVER"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_GLB_SOLVER KheGlbSolverMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,     */
/*    KHE_OPTIONS options, HA_ARENA a)                                       */
/*                                                                           */
/*  Make a new GLB solver object with these attributes.                      */
/*                                                                           */
/*****************************************************************************/

static KHE_GLB_SOLVER KheGlbSolverMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, /* int min_len, */ int max_len, bool in_seq, HA_ARENA a)
{
  KHE_GLB_SOLVER res;  KHE_RESOURCE r;  int i;  KHE_GLB_RESOURCE gr;

  /* free lists */
  HaMake(res, a);
  HaArrayInit(res->task_set_free_list, a);
  HaArrayInit(res->node_free_list, a);
  HaArrayInit(res->node_entry_free_list, a);
  HaArrayInit(res->node_table_free_list, a);
  HaArrayInit(res->graph_free_list, a);

  /* regular fields */
  res->arena = a;
  res->soln = soln;
  res->resource_type = rt;
  res->options = options;
  res->frame = KheOptionsFrame(options, "gs_common_frame", soln);
  /* res->min_len = min_len; */
  res->max_len = max_len;
  res->in_seq = in_seq;
  HaArrayInit(res->resources, a);
  res->scratch_ts = KheTaskSetMake(soln);
  res->next_ts_index = 0;

  /* add resources */
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    gr = KheGlbResourceMake(r, res);
    HaArrayAddLast(res->resources, gr);
    if( DEBUG2 )
      KheGlbResourceDebug(gr, 2, 2, stderr);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAddIntermediateOrSinkNodes(KHE_GLB_GRAPH gg,                     */
/*    KHE_GLB_RESOURCE gr, KHE_NODE_TYPE node_type, KHE_GLB_SOLVER gs)       */
/*                                                                           */
/*  Add nodes for fully loaded or underloaded resource gr.                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheAddIntermediateOrSinkNodes(KHE_GLB_GRAPH gg,
  KHE_GLB_RESOURCE gr, KHE_NODE_TYPE node_type, KHE_GLB_SOLVER gs)
{
  KHE_GLB_RESOURCE gr2;  KHE_GLB_TASK_SET gts;  int i, j;  KHE_GLB_NODE gn;
  HaArrayForEach(gs->resources, gr2, i)
    if( gr2 != gr )
      HaArrayForEach(gr2->task_sets, gts, j)
      {
	gn = KheGlbNod eMake(node_type, gr, gts, gs);
	KheGlbGraphAddNode(gg, gn, gs);
      }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_GLB_GRAPH KheGlbSolverBuildGraphOld(KHE_GLB_SOLVER gs, int len)      */
/*                                                                           */
/*  Build the graph for L = len, the old way.                                */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_GLB_GRAPH KheGlbSolverBuildGraphOld(KHE_GLB_SOLVER gs, int len)
{
  KHE_GLB_RESOURCE gr, gr1, gr2;  KHE_GLB_GRAPH res;  int i, j, k;
  KHE_GLB_NODE gn1, gn2;  KHE_GLB_TASK_SET gts;  KHE_COST delta_cost;

  ** initialize the resources for building a new graph **
  if( DEBUG3 )
    fprintf(stderr, "[ KheGlbSolverBuildGraph(gs, %d)\n", len);
  HaArrayForEach(gs->resources, gr, i)
  {
    KheGlbResourceInitForSolve(gr, len, gs, NULL);
    ** ***
    if( DEBUG3 )
      KheGlbResourceDebug(gr, 2, 2, stderr);
    *** **
  }

  ** make a new, empty graph **
  res = KheGlbGraphMake(gs);

  ** make and add the nodes **
  HaArrayForEach(gs->resources, gr, i)
    switch( gr->load_type )
    {
      case KHE_RESOURCE_OVERLOADED:

	gn1 = KheGlbNode Make(KHE_NODE_SOURCE, gr, NULL, gs);
	KheGlbGraphAddNode(res, gn1, gs);
	break;

      case KHE_RESOURCE_FULLY_LOADED:

        KheAddIntermediateOrSinkNodes(res, gr, KHE_NODE_INTERMEDIATE, gs);
	break;

      case KHE_RESOURCE_UNDERLOADED:

        KheAddIntermediateOrSinkNodes(res, gr, KHE_NODE_SINK, gs);
	break;

      case KHE_RESOURCE_UNKNOWN:
      default:

	HnAbort("KheGlbSolverBuildGraph internal error (%d)", gr->load_type);
	break;
    }

  ** debug print of all nodes **
  ** ***
  if( DEBUG4 )
    HaArrayForEach(res->all_nodes, gn1, i)
      KheGlbNodeDebug(gn1, 2, 2, stderr);
  *** **

  ** make and add the edges **
  HaArrayForEach(res->all_nodes, gn1, i)
  {
    gr1 = gn1->resource;
    HaArrayForEach(gr1->task_sets, gts, j)
      HaArrayForEach(gs->resources, gr2, k)
	if( KheGlbNodeTableRetrieveNode(res->node_table, gr2, gts, &gn2) &&
	    KheGlbGraphAcceptsEdge(res, gn1, gn2, gs, &delta_cost) )
	  KheGlbGraphAddEdge(gn1, delta_cost, gn2);
  }

  ** do the initial search of the graph **
  KheGlbGraphBreadthFirstSearch(res);
  ** delete useless edges and nodes **
  ** KheGlbGraphDeleteUselessEdgesAndNodes(res, gs); **

  ** all done **
  if( DEBUG3 )
  {
    KheGlbGraphDebug(res, true, 2, 2, stderr);
    fprintf(stderr, "] KheGlbSolverBuildGraph returning\n");
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheHandleEdge(KHE_GLB_GRAPH gg, KHE_GLB_NODE gn,                    */
/*    KHE_GLB_RESOURCE gr, KHE_GLB_TASK_SET gts, KHE_GLB_SOLVER gs)          */
/*                                                                           */
/*  Work out whether there is an edge from gn1 to gr2+gts2.  If there is,    */
/*  either retrieve or make a node for gr2+gts2 and link to back to gn1      */
/*  by an edge.                                                              */
/*                                                                           */
/*****************************************************************************/

static void KheHandleEdge(KHE_GLB_GRAPH gg, KHE_GLB_NODE gn1,
  KHE_GLB_RESOURCE gr2, KHE_GLB_TASK_SET gts2, KHE_GLB_SOLVER gs)
{
  KHE_COST delta_cost;  KHE_GLB_NODE gn2;  KHE_NODE_TYPE node_type;
  if( KheGlbGraphAcceptsEdge(gg, gn1, gr2, gts2, gs, &delta_cost) )
  {
    /* retrieve, or make and insert gn2, which is gr2+gts2 */
    if( !KheGlbNodeTableRetrieveNode(gg->node_table, gr2, gts2, &gn2) )
    {
      HnAssert(gr2->load_type != KHE_RESOURCE_OVERLOADED,
	"KheHandleEdge internal error 1");
      node_type = (gr2->load_type == KHE_RESOURCE_FULLY_LOADED ?
	KHE_NODE_INTERMEDIATE : KHE_NODE_SINK);
      gn2 = KheGlbNodeMake(node_type, gr2, gts2, gn1,
	gn1->cost_decrease_from_source + delta_cost, gs);
      KheGlbNodeTableInsertNode(gg->node_table, gn2, gs);
      HaArrayAddLast(gg->all_nodes_bfs, gn2);
    }

    /* add an edge from gn1 to gn2 */
    KheGlbGraphAddEdge(gn1, delta_cost, gn2);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GLB_GRAPH KheGlbSolverBuildGraph(KHE_GLB_SOLVER gs, int len)         */
/*                                                                           */
/*  Build the graph for L = len.                                             */
/*                                                                           */
/*****************************************************************************/

static KHE_GLB_GRAPH KheGlbSolverBuildGraph(KHE_GLB_SOLVER gs)
{
  KHE_GLB_RESOURCE gr1, gr2;  KHE_GLB_GRAPH res;  int i, j, k;
  KHE_GLB_NODE gn1, gn2;  KHE_GLB_TASK_SET gts;

  /* make a new, empty graph */
  res = KheGlbGraphMake(gs);
  if( DEBUG3 )
    fprintf(stderr, "[ KheGlbSolverBuildGraph(gs, %d)\n", gs->max_len);

  /* initialize each resource for building a new graph; */
  /* for each overloaded resource, add a source node to the queue */
  HaArrayForEach(gs->resources, gr1, i)
  {
    KheGlbResourceInitForSolve(gr1, gs);
    if( gr1->load_type == KHE_RESOURCE_OVERLOADED )
      HaArrayAddLast(res->all_nodes_bfs,
	KheGlbNodeMake(KHE_NODE_SOURCE, gr1, NULL, NULL, 0, gs));
  }

  /* initially, no sink nodes are reachable */
  HaArrayClear(res->sink_nodes);

  /* run the main algorithm */
  HaArrayForEach(res->all_nodes_bfs, gn1, i)
  {
    if( DEBUG4 )
    {
      fprintf(stderr, "  reached ");
      KheGlbNodeDebug(gn1, 2, -1, stderr);
      fprintf(stderr, "\n");
    }

    if( gn1->node_type == KHE_NODE_SINK )
    {
      /* if gn1 is a sink node, remember it and mark all nodes back to source */
      HaArrayAddLast(res->sink_nodes, gn1);
      for( gn2 = gn1;  gn2 != NULL;  gn2 = gn2->pred_on_path_from_source )
        gn2->on_path_to_sink = true;
    }
    else
    {
      /* find outgoing edges from non-sinke node gn1 */
      gr1 = gn1->resource;
      HaArrayForEach(gs->resources, gr2, j)
	if( gr2 != gr1 )
	  HaArrayForEach(gr1->task_sets, gts, k)
	    KheHandleEdge(res, gn1, gr2, gts, gs);
    }
  }

  /* sort the sink nodes by increasing cost decrease */
  HaArraySort(res->sink_nodes, &KheGlbNodeCmp);
  if( DEBUG4 )
  {
    KheGlbGraphDebug(res, 2, 2, stderr);
    fprintf(stderr, "] KheGlbSolverBuildGraph returning\n");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main function"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheGlobalLoadBalance(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,           */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Carry out global load balancing.                                         */
/*                                                                           */
/*****************************************************************************/

bool KheGlobalLoadBalance(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options)
{
  HA_ARENA a;  int max_len, i;  bool in_seq, res, progressing;
  KHE_GLB_SOLVER gs;  KHE_GLB_GRAPH gg;  KHE_GLB_RESOURCE gr;
  KHE_GLB_TASK_SET gts;

  if( DEBUG1 )
    fprintf(stderr, "[ KheGlobalLoadBalance(soln of %s, %s, options)\n",
      KheInstanceId(KheSolnInstance(soln)), KheResourceTypeId(rt));

  /* read options */
  max_len = KheOptionsGetInt(options, "rs_glr_max_len", 3);
  in_seq = KheOptionsGetBool(options, "rs_glr_in_seq", false);

  /* make a solver object */
  a = KheSolnArenaBegin(soln);
  gs = KheGlbSolverMake(soln, rt, options, /* min_len, */ max_len, in_seq, a);

  /* keep solving while progressing */
  res = false;
  progressing = true;
  while( progressing && !KheOptionsTimeLimitReached(options) )
  {
    progressing = false;
    gg = KheGlbSolverBuildGraph(gs);
    if( KheGlbGraphSolve(gg, gs) )
      progressing = res = true;
    KheGlbGraphDelete(gg, gs);
  }

  /* delete all group monitors - very important! */
  HaArrayForEach(gs->resources, gr, i)
  {
    KheGroupMonitorDelete(gr->gm);
    while( HaArrayCount(gr->task_sets) > 0 )
    {
      gts = HaArrayLastAndDelete(gr->task_sets);
      KheGlbTaskSetDelete(gts, gs);
    }
  }

  /* clear memory and return */
  KheSolnArenaEnd(soln, a);
  if( DEBUG1 )
    fprintf(stderr, "] KheGlobalLoadBalance returning %s\n", bool_show(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  THE KTS TIMETABLING SYSTEM                                               */
/*  COPYRIGHT (C) 2004, 2010 Jeffrey H. Kingston                             */
/*                                                                           */
/*  Jeffrey H. Kingston (jeff@it.usyd.edu.au)                                */
/*  School of Information Technologies                                       */
/*  The University of Sydney 2006                                            */
/*  AUSTRALIA                                                                */
/*                                                                           */
/*  FILE:         khe_matching.c                                             */
/*  MODULE:       Unweighted bipartite matching (implementation)             */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"
#include <limits.h>
#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG3_MARK_CALL 47566
#define DEBUG4 0
#define DEBUG4_MONITOR_INDEX INT_MAX
#define DEBUG5 0

#define USE_SNAPSHOT 0

#define DEBUG6(m) (USE_SNAPSHOT &&					\
  ((m)->debug_mark_calls == 60308 || (m)->debug_mark_calls == 61999 ||	\
   (m)->debug_mark_calls == 62001 || (m)->debug_mark_calls == 61995 ||	\
   (m)->debug_mark_calls == 66017 || (m)->debug_mark_calls == 66000 ||	\
   (m)->debug_mark_calls == 65976 || (m)->debug_mark_calls == 65982 ||	\
   (m)->debug_mark_calls == 60171) )


/*****************************************************************************/
/*                                                                           */
/*  Type declarations - snapshots                                            */
/*                                                                           */
/*****************************************************************************/

#if USE_SNAPSHOT
typedef struct khe_snapshot_rec *KHE_SNAPSHOT;
typedef HA_ARRAY(KHE_SNAPSHOT) ARRAY_KHE_SNAPSHOT;

typedef struct khe_snapshot_supply_node_rec *KHE_SNAPSHOT_SUPPLY_NODE;
typedef HA_ARRAY(KHE_SNAPSHOT_SUPPLY_NODE) ARRAY_KHE_SNAPSHOT_SUPPLY_NODE;

typedef struct khe_snapshot_supply_chunk_rec *KHE_SNAPSHOT_SUPPLY_CHUNK;
typedef HA_ARRAY(KHE_SNAPSHOT_SUPPLY_CHUNK) ARRAY_KHE_SNAPSHOT_SUPPLY_CHUNK;

typedef struct khe_snapshot_demand_node_rec *KHE_SNAPSHOT_DEMAND_NODE;
typedef HA_ARRAY(KHE_SNAPSHOT_DEMAND_NODE) ARRAY_KHE_SNAPSHOT_DEMAND_NODE;

typedef struct khe_snapshot_demand_chunk_rec *KHE_SNAPSHOT_DEMAND_CHUNK;
typedef HA_ARRAY(KHE_SNAPSHOT_DEMAND_CHUNK) ARRAY_KHE_SNAPSHOT_DEMAND_CHUNK;

struct khe_snapshot_supply_node_rec {
  KHE_SNAPSHOT_SUPPLY_CHUNK		supply_chunk;
  int					index;
  /* KHE_SNAPSHOT_DEMAND_NODE		supply_asst; */
};

struct khe_snapshot_supply_chunk_rec {
  KHE_SNAPSHOT				snapshot;
  int					index_in_snapshot;
  int					base;
  ARRAY_KHE_SNAPSHOT_SUPPLY_NODE	supply_nodes;
};

struct khe_snapshot_demand_node_rec {
  KHE_SNAPSHOT_DEMAND_CHUNK		demand_chunk;
  KHE_TASK				task;		/* ordinary demand */
  KHE_RESOURCE				resource;	/* workload demand */
  KHE_SET				domain;
  /* KHE_SNAPSHOT_SUPPLY_NODE		demand_asst; */
  /* int				demand_asst_index; */
  /* int				unmatched_pos; */
};

struct khe_snapshot_demand_chunk_rec {
  KHE_SNAPSHOT				snapshot;
  int					index_in_snapshot;
  int					base;
  int					increment;
  SSET					domain;
  ARRAY_KHE_SNAPSHOT_DEMAND_NODE	demand_nodes;
};

struct khe_snapshot_rec {
  HA_ARENA				arena;
  ARRAY_KHE_SNAPSHOT_SUPPLY_NODE	supply_node_free_list;
  ARRAY_KHE_SNAPSHOT_SUPPLY_CHUNK	supply_chunk_free_list;
  ARRAY_KHE_SNAPSHOT_DEMAND_NODE	demand_node_free_list;
  ARRAY_KHE_SNAPSHOT_DEMAND_CHUNK	demand_chunk_free_list;

  ARRAY_KHE_SNAPSHOT_SUPPLY_CHUNK	supply_chunks;
  ARRAY_KHE_SNAPSHOT_DEMAND_CHUNK	demand_chunks;
  ARRAY_KHE_SNAPSHOT_SUPPLY_NODE	all_supply_nodes;
  ARRAY_KHE_SNAPSHOT_DEMAND_NODE	all_demand_nodes;
  /* int				demand_node_count; */
  /* ARRAY_KHE_SNAPSHOT_DEMAND_NODE	unmatched_demand_nodes; */
  /* int				unmatched_lower_bound; */
};
#endif


/*****************************************************************************/
/*                                                                           */
/*  Type declarations                                                        */
/*                                                                           */
/*****************************************************************************/

struct khe_matching_supply_chunk_rec {
  KHE_MATCHING				matching;
  int					index_in_matching;
  void					*impl;
  int					base;
  ARRAY_KHE_MATCHING_SUPPLY_NODE	supply_nodes;
  KHE_MATCHING_SUPPLY_CHUNK		copy;
};

struct khe_matching_supply_node_rec {
  KHE_MATCHING_SUPPLY_CHUNK		supply_chunk;
  void					*impl;
  int					index;
  unsigned int				visit_num;
  KHE_MATCHING_DEMAND_NODE		supply_asst;
  KHE_MATCHING_HALL_SET			hall_set;
  /* short				demand_test_index; */
  KHE_MATCHING_SUPPLY_NODE		copy;
};

struct khe_matching_demand_chunk_rec {
  KHE_MATCHING				matching;
  int					index_in_matching;
  void					*impl;
  int					base;
  int					increment;
  SSET					domain;
  ARRAY_KHE_MATCHING_DEMAND_NODE	demand_nodes;
  KHE_MATCHING_DEMAND_CHUNK		copy;
};

struct khe_matching_demand_node_rec {  /* this is an abstract supertype */
  INHERIT_MATCHING_DEMAND_NODE
};

struct khe_matching_rec {
#if USE_SNAPSHOT
  ARRAY_KHE_SNAPSHOT			snapshot_free_list;
#endif
  KHE_SOLN				soln;
  ARRAY_KHE_MATCHING_SUPPLY_CHUNK	supply_chunks;
  ARRAY_KHE_MATCHING_DEMAND_CHUNK	demand_chunks;
  ARRAY_KHE_MATCHING_SUPPLY_NODE	supply_nodes;
  int					demand_node_count;
  unsigned int				visit_num;
  ARRAY_KHE_MATCHING_DEMAND_NODE	unmatched_demand_nodes;
  int					unmatched_lower_bound;
  bool					active;
  HA_ARRAY_INT				marks;
#if USE_SNAPSHOT
  ARRAY_KHE_SNAPSHOT			debug_snapshots_before;
#endif
  int					debug_mark_calls;
  ARRAY_KHE_MATCHING_HALL_SET		hall_sets;
  ARRAY_KHE_MATCHING_DEMAND_NODE	competitors;
  /* KHE_SNAPSHOT			debug_snapshot_before; */
  /* KHE_SNAPSHOT			debug_snapshot_after; */
  KHE_MATCHING				copy;
};

struct khe_matching_hall_set_rec {
  KHE_MATCHING_HALL_SET			parent_hall_set;
  ARRAY_KHE_MATCHING_SUPPLY_NODE	supply_nodes;
  ARRAY_KHE_MATCHING_DEMAND_NODE	demand_nodes;
  KHE_MATCHING_HALL_SET			copy;
};


#if USE_SNAPSHOT
/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SNAPSHOT_SUPPLY_NODE" (private)                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SNAPSHOT_SUPPLY_NODE KheSnapshotSupplyNodeMake(                      */
/*    KHE_SNAPSHOT_SUPPLY_CHUNK ssc, int index, HA_ARENA a)                  */
/*                                                                           */
/*  Make a snapshot supply node with these attributes.                       */
/*                                                                           */
/*****************************************************************************/

static KHE_SNAPSHOT_SUPPLY_NODE KheSnapshotSupplyNodeMake(
  KHE_SNAPSHOT_SUPPLY_CHUNK ssc, int index, KHE_SNAPSHOT s)
{
  KHE_SNAPSHOT_SUPPLY_NODE res;
  if( HaArrayCount(s->supply_node_free_list) > 0 )
    res = HaArrayLastAndDelete(s->supply_node_free_list);
  else
    HaMake(res, s->arena);
  res->supply_chunk = ssc;
  res->index = index;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSnapshotSupplyNodeCheckEqual(KHE_SNAPSHOT_SUPPLY_NODE ssn1,      */
/*    KHE_SNAPSHOT_SUPPLY_NODE ssn2)                                         */
/*                                                                           */
/*  Check that ssn1 and ssn2 are equal.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheSnapshotSupplyNodeCheckEqual(KHE_SNAPSHOT_SUPPLY_NODE ssn1,
  KHE_SNAPSHOT_SUPPLY_NODE ssn2)
{
  /* *** wrong
  HnAssert(ssn1->supply_chunk == ssn2->supply_chunk,
    "KheSnapshotSupplyNodeCheckEqual failing at supply_chunk");
  *** */
  HnAssert(ssn1->index == ssn2->index,
    "KheSnapshotSupplyNodeCheckEqual failing at index");
}
#endif


#if USE_SNAPSHOT
/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SNAPSHOT_SUPPLY_CHUNK" (private)                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SNAPSHOT_SUPPLY_CHUNK KheSnapshotSupplyChunkMake(KHE_SNAPSHOT s,     */
/*    int index_in_snapshot, int base, HA_ARENA a)                           */
/*                                                                           */
/*  Make a new, empty snapshot supply chunk with these attributes.           */
/*                                                                           */
/*****************************************************************************/

static KHE_SNAPSHOT_SUPPLY_CHUNK KheSnapshotSupplyChunkMake(
  int index_in_snapshot, int base, KHE_SNAPSHOT s)
{
  KHE_SNAPSHOT_SUPPLY_CHUNK res;
  if( HaArrayCount(s->supply_chunk_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(s->supply_chunk_free_list);
    HaArrayClear(res->supply_nodes);
  }
  else
  {
    HaMake(res, s->arena);
    HaArrayInit(res->supply_nodes, s->arena);
  }
  res->snapshot = s;
  res->index_in_snapshot = index_in_snapshot;
  res->base = base;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SNAPSHOT_SUPPLY_CHUNK KheSnapshotSupplyChunkBuild(KHE_SNAPSHOT s,    */
/*    KHE_MATCHING_SUPPLY_CHUNK msc, HA_ARENA a)                             */
/*                                                                           */
/*  Build a new snapshot supply chunk with these attributes.                 */
/*                                                                           */
/*****************************************************************************/

static KHE_SNAPSHOT_SUPPLY_CHUNK KheSnapshotSupplyChunkBuild(
  KHE_MATCHING_SUPPLY_CHUNK msc, KHE_SNAPSHOT s)
{
  KHE_SNAPSHOT_SUPPLY_CHUNK res;  /* KHE_MATCHING_SUPPLY_NODE msn; */  int i;
  KHE_SNAPSHOT_SUPPLY_NODE ssn;
  res = KheSnapshotSupplyChunkMake(msc->index_in_matching, msc->base, s);
  /* HaArrayForEach(msc->supply_nodes, msn, i) */
  for( i = 0;  i < HaArrayCount(msc->supply_nodes);  i++ )
  {
    ssn = KheSnapshotSupplyNodeMake(res, i, s);
    HaArrayAddLast(res->supply_nodes, ssn);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSnapshotSupplyChunkCheckEqual(KHE_SNAPSHOT_SUPPLY_CHUNK ssc1,    */
/*    KHE_SNAPSHOT_SUPPLY_CHUNK ssc2)                                        */
/*                                                                           */
/*  Check that ssc1 and ssc2 are equal.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheSnapshotSupplyChunkCheckEqual(KHE_SNAPSHOT_SUPPLY_CHUNK ssc1,
  KHE_SNAPSHOT_SUPPLY_CHUNK ssc2)
{
  KHE_SNAPSHOT_SUPPLY_NODE ssn1, ssn2;  int i;
  HnAssert(ssc1->index_in_snapshot == ssc2->index_in_snapshot,
    "KheSnapshotSupplyChunkCheckEqual failing at index_in_snapshot");
  HnAssert(ssc1->base == ssc2->base,
    "KheSnapshotSupplyChunkCheckEqual failing at base (%d != %d)",
    ssc1->base, ssc2->base);
  HnAssert(HaArrayCount(ssc1->supply_nodes) == HaArrayCount(ssc2->supply_nodes),
    "KheSnapshotSupplyChunkCheckEqual failing at supply nodes (%d != %d)",
    HaArrayCount(ssc1->supply_nodes), HaArrayCount(ssc2->supply_nodes));
  for( i = 0;  i < HaArrayCount(ssc1->supply_nodes);  i++ )
  {
    ssn1 = HaArray(ssc1->supply_nodes, i);
    ssn2 = HaArray(ssc2->supply_nodes, i);
    KheSnapshotSupplyNodeCheckEqual(ssn1, ssn2);
  }
}
#endif


#if USE_SNAPSHOT
/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SNAPSHOT_DEMAND_NODE" (private)                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SNAPSHOT_DEMAND_NODE KheSnapshotDemandNodeMake(                      */
/*    KHE_SNAPSHOT_DEMAND_CHUNK sdc, KHE_SET domain, HA_ARENA a)             */
/*                                                                           */
/*  Make a new snapshot demand node with these attributes.                   */
/*                                                                           */
/*****************************************************************************/

static KHE_SNAPSHOT_DEMAND_NODE KheSnapshotDemandNodeMake(
  KHE_SNAPSHOT_DEMAND_CHUNK sdc, KHE_TASK task, KHE_RESOURCE r,
  KHE_SET domain, KHE_SNAPSHOT s)
{
  KHE_SNAPSHOT_DEMAND_NODE res;
  if( HaArrayCount(s->demand_node_free_list) > 0 )
    res = HaArrayLastAndDelete(s->demand_node_free_list);
  else
    HaMake(res, s->arena);
  res->demand_chunk = sdc;
  res->task = task;
  res->resource = r;
  res->domain = domain;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSnapshotDemandNodeCheckEqual(KHE_SNAPSHOT_DEMAND_NODE sdn1,      */
/*    KHE_SNAPSHOT_DEMAND_NODE sdn2)                                         */
/*                                                                           */
/*  Check that sdn1 and sdn2 are equal.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheSnapshotDemandNodeCheckEqual(KHE_SNAPSHOT_DEMAND_NODE sdn1,
  KHE_SNAPSHOT_DEMAND_NODE sdn2)
{
  char buff1[200], buff2[200];
  /* *** wrong
  HnAssert(sdn1->demand_chunk == sdn2->demand_chunk,
    "KheSnapshotDemandNodeCheckEqual failing at demand_chunk");
  *** */
  HnAssert(sdn1->task == sdn2->task,
    "KheSnapshotDemandNodeCheckEqual failing at task (%s != %s)",
    sdn1->task == NULL ? "NULL" : KheTaskId(sdn1->task),
    sdn2->task == NULL ? "NULL" : KheTaskId(sdn2->task));
  HnAssert(sdn1->resource == sdn2->resource,
    "KheSnapshotDemandNodeCheckEqual failing at resource (%s != %s)",
    sdn1->resource == NULL ? "NULL" : KheResourceId(sdn1->resource),
    sdn2->resource == NULL ? "NULL" : KheResourceId(sdn2->resource));
  HnAssert(KheSetEqual(sdn1->domain, sdn2->domain),
    "KheSnapshotDemandNodeCheckEqual(%s, %s) failing at domain (%s != %s)\n",
    sdn1->task == NULL ? "NULL" : KheTaskId(sdn1->task),
    sdn1->resource == NULL ? "NULL" : KheResourceId(sdn1->resource),
    KheSetShow(sdn1->domain, buff1), KheSetShow(sdn2->domain, buff2));
}
#endif


#if USE_SNAPSHOT
/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SNAPSHOT_DEMAND_CHUNK" (private)                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SNAPSHOT_DEMAND_CHUNK KheSnapshotDemandChunkMake(KHE_SNAPSHOT s,     */
/*    int index_in_snapshot, int base, int increment,SSET domain,HA_ARENA a) */
/*                                                                           */
/*  Make a new, empty snapshot demand chunk with these attributes.           */
/*                                                                           */
/*****************************************************************************/

static KHE_SNAPSHOT_DEMAND_CHUNK KheSnapshotDemandChunkMake(
  int index_in_snapshot, int base, int increment, SSET domain, KHE_SNAPSHOT s)
{
  KHE_SNAPSHOT_DEMAND_CHUNK res;
  if( HaArrayCount(s->demand_chunk_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(s->demand_chunk_free_list);
    HaArrayClear(res->demand_nodes);
  }
  else
  {
    HaMake(res, s->arena);
    HaArrayInit(res->demand_nodes, s->arena);
  }
  res->snapshot = s;
  res->index_in_snapshot = index_in_snapshot;
  res->base = base;
  res->increment = increment;
  res->domain = domain;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SNAPSHOT_DEMAND_CHUNK KheSnapshotDemandChunkBuild(KHE_SNAPSHOT s,    */
/*    KHE_MATCHING_DEMAND_CHUNK mdc, HA_ARENA a)                             */
/*                                                                           */
/*  Build a snapshot demand chunk for s, corresponding to mdc.               */
/*                                                                           */
/*****************************************************************************/

static KHE_SNAPSHOT_DEMAND_CHUNK KheSnapshotDemandChunkBuild(
  KHE_MATCHING_DEMAND_CHUNK mdc, KHE_SNAPSHOT s)
{
  KHE_SNAPSHOT_DEMAND_CHUNK res;  KHE_MATCHING_DEMAND_NODE mdn;  int i;
  KHE_SNAPSHOT_DEMAND_NODE sdn;  KHE_TASK task;  KHE_RESOURCE resource;
  KHE_ORDINARY_DEMAND_MONITOR odm;  KHE_WORKLOAD_DEMAND_MONITOR wdm;
  res = KheSnapshotDemandChunkMake(mdc->index_in_matching, mdc->base,
    mdc->increment, mdc->domain, s);
  HaArrayForEach(mdc->demand_nodes, mdn, i)
  {
    switch( KheMonitorTag((KHE_MONITOR) mdn) )
    {
      case KHE_ORDINARY_DEMAND_MONITOR_TAG:

	odm = (KHE_ORDINARY_DEMAND_MONITOR) mdn;
	task = KheOrdinaryDemandMonitorTask(odm);
	resource = NULL;
	break;

      case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

	wdm = (KHE_WORKLOAD_DEMAND_MONITOR) mdn;
	task = NULL;
	resource = KheWorkloadDemandMonitorResource(wdm);
	break;

      default:

	task = NULL;
	resource = NULL;
	HnAbort("KheSnapshotDemandChunkBuild internal error (%d)",
          KheMonitorTag((KHE_MONITOR) mdn));
    }
    sdn = KheSnapshotDemandNodeMake(res, task, resource, mdn->domain, s);
    HaArrayAddLast(res->demand_nodes, sdn);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSnapshotDemandChunkCheckEqual(KHE_SNAPSHOT_DEMAND_CHUNK sdc1,    */
/*    KHE_SNAPSHOT_DEMAND_CHUNK sdc2)                                        */
/*                                                                           */
/*  Check that sdc1 and sdc2 are equal.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheSnapshotDemandChunkCheckEqual(KHE_SNAPSHOT_DEMAND_CHUNK sdc1,
  KHE_SNAPSHOT_DEMAND_CHUNK sdc2)
{
  int i;  KHE_SNAPSHOT_DEMAND_NODE sdn1, sdn2;
  HnAssert(sdc1->index_in_snapshot == sdc2->index_in_snapshot,
    "KheSnapshotDemandChunkCheckEqual failing at index_in_snapshot");
  HnAssert(sdc1->base == sdc2->base,
    "KheSnapshotDemandChunkCheckEqual failing at base (%d != %d)",
    sdc1->base, sdc2->base);
  HnAssert(sdc1->increment == sdc2->increment,
    "KheSnapshotDemandChunkCheckEqual failing at increment (%d != %d)",
    sdc1->increment, sdc2->increment);
  HnAssert(SSetEqual(sdc1->domain, sdc2->domain),
    "KheSnapshotDemandChunkCheckEqual failing at domain");
  HnAssert(HaArrayCount(sdc1->demand_nodes) == HaArrayCount(sdc2->demand_nodes),
    "KheSnapshotDemandChunkCheckEqual failing at demand_nodes (%d != %d)",
    HaArrayCount(sdc1->demand_nodes), HaArrayCount(sdc2->demand_nodes));
  for( i = 0;  i < HaArrayCount(sdc1->demand_nodes);  i++ )
  {
    sdn1 = HaArray(sdc1->demand_nodes, i);
    sdn2 = HaArray(sdc2->demand_nodes, i);
    KheSnapshotDemandNodeCheckEqual(sdn1, sdn2);
  }
}
#endif


#if USE_SNAPSHOT
/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SNAPSHOT" (private)                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SNAPSHOT KheSnapshotMake(HA_ARENA a)                                 */
/*                                                                           */
/*  Make a new, empty snapshot object.                                       */
/*                                                                           */
/*****************************************************************************/
static void KheSnapshotClear(KHE_SNAPSHOT s);

static KHE_SNAPSHOT KheSnapshotMake(KHE_MATCHING m)
{
  KHE_SNAPSHOT res;  HA_ARENA a;
  if( HaArrayCount(m->snapshot_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(m->snapshot_free_list);
    KheSnapshotClear(res);
  }
  else
  {
    a = KheSolnArena(m->soln);
    HaMake(res, a);
    res->arena = a;
    HaArrayInit(res->supply_node_free_list, a);
    HaArrayInit(res->supply_chunk_free_list, a);
    HaArrayInit(res->demand_node_free_list, a);
    HaArrayInit(res->demand_chunk_free_list, a);
    HaArrayInit(res->supply_chunks, a);
    HaArrayInit(res->demand_chunks, a);
    HaArrayInit(res->all_supply_nodes, a);
    HaArrayInit(res->all_demand_nodes, a);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSnapshotClear(KHE_SNAPSHOT s)                                    */
/*                                                                           */
/*  Clear s back to the empty snapshot.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheSnapshotClear(KHE_SNAPSHOT s)
{
  int i;
  HaArrayAppend(s->supply_node_free_list, s->all_supply_nodes, i);
  HaArrayClear(s->all_supply_nodes);
  HaArrayAppend(s->demand_node_free_list, s->all_demand_nodes, i);
  HaArrayClear(s->all_demand_nodes);
  HaArrayAppend(s->supply_chunk_free_list, s->supply_chunks, i);
  HaArrayClear(s->supply_chunks);
  HaArrayAppend(s->demand_chunk_free_list, s->demand_chunks, i);
  HaArrayClear(s->demand_chunks);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSnapshotBuild(KHE_SNAPSHOT s, KHE_MATCHING m)                    */
/*                                                                           */
/*  Clear s then build it into a snapshot of matching m.                     */
/*                                                                           */
/*****************************************************************************/

static void KheSnapshotBuild(KHE_SNAPSHOT s, KHE_MATCHING m)
{
  int i, j;
  KHE_MATCHING_SUPPLY_CHUNK msc;  KHE_MATCHING_DEMAND_CHUNK mdc;
  KHE_SNAPSHOT_SUPPLY_CHUNK ssc;  KHE_SNAPSHOT_DEMAND_CHUNK sdc;
  KheSnapshotClear(s);
  HaArrayForEach(m->supply_chunks, msc, i)
  {
    ssc = KheSnapshotSupplyChunkBuild(msc, s);
    HaArrayAddLast(s->supply_chunks, ssc);
    HaArrayAppend(s->all_supply_nodes, ssc->supply_nodes, j);
  }
  HaArrayForEach(m->demand_chunks, mdc, i)
  {
    sdc = KheSnapshotDemandChunkBuild(mdc, s);
    HaArrayAddLast(s->demand_chunks, sdc);
    HaArrayAppend(s->all_demand_nodes, sdc->demand_nodes, j);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSnapshotCheckEqual(KHE_SNAPSHOT s1, KHE_SNAPSHOT s2)             */
/*                                                                           */
/*  Check that s1 and s2 are equal, aborting with an error message if not.   */
/*                                                                           */
/*****************************************************************************/

static void KheSnapshotCheckEqual(KHE_SNAPSHOT s1, KHE_SNAPSHOT s2)
{
  KHE_SNAPSHOT_SUPPLY_CHUNK ssc1, ssc2;
  KHE_SNAPSHOT_DEMAND_CHUNK sdc1, sdc2;
  KHE_SNAPSHOT_SUPPLY_NODE ssn1, ssn2;  int i;

  /* supply chunks */
  HnAssert(HaArrayCount(s1->supply_chunks) == HaArrayCount(s2->supply_chunks),
    "KheSnapshotCheckEqual failing at supply chunks (%d != %d)",
    HaArrayCount(s1->supply_chunks), HaArrayCount(s2->supply_chunks));
  for( i = 0;  i < HaArrayCount(s1->supply_chunks);  i++ )
  {
    ssc1 = HaArray(s1->supply_chunks, i);
    ssc2 = HaArray(s2->supply_chunks, i);
    KheSnapshotSupplyChunkCheckEqual(ssc1, ssc2);
  }

  /* demand chunks */
  HnAssert(HaArrayCount(s1->demand_chunks) == HaArrayCount(s2->demand_chunks),
    "KheSnapshotCheckEqual failing at demand chunks (%d != %d)",
    HaArrayCount(s1->demand_chunks), HaArrayCount(s2->demand_chunks));
  for( i = 0;  i < HaArrayCount(s1->demand_chunks);  i++ )
  {
    sdc1 = HaArray(s1->demand_chunks, i);
    sdc2 = HaArray(s2->demand_chunks, i);
    KheSnapshotDemandChunkCheckEqual(sdc1, sdc2);
  }

  /* supply nodes */
  HnAssert(HaArrayCount(s1->all_supply_nodes) ==
    HaArrayCount(s2->all_supply_nodes),
    "KheSnapshotCheckEqual failing at supply nodes (%d != %d)",
    HaArrayCount(s1->all_supply_nodes), HaArrayCount(s2->all_supply_nodes));
  for( i = 0;  i < HaArrayCount(s1->all_supply_nodes);  i++ )
  {
    ssn1 = HaArray(s1->all_supply_nodes, i);
    ssn2 = HaArray(s2->all_supply_nodes, i);
    KheSnapshotSupplyNodeCheckEqual(ssn1, ssn2);
  }
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Solving" (private)                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheDemandNodeCheck(KHE_MATCHING_DEMAND_NODE dn)                     */
/*                                                                           */
/*  Check an invariant of dn.                                                */
/*                                                                           */
/*****************************************************************************/

static void KheDemandNodeCheck(KHE_MATCHING_DEMAND_NODE dn)
{
  HnAssert( (dn->demand_asst == NULL) == (dn->cost > 0),
    "KheDemandNodeCheck(weight %.5f, dn->demand_asst %s NULL, dn->cost %.5f)",
    KheCostShow(KheSolnMatchingWeight(dn->soln)),
    dn->demand_asst == NULL ? "==" : "!=", KheCostShow(dn->cost));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMatchingAssignBFS(KHE_MATCHING_DEMAND_NODE dn,                   */
/*    unsigned int visit_num)                                                */
/*                                                                           */
/*  Try to find and apply an augmenting path from unassigned demand node     */
/*  dn, returning true if successful.  Use breadth-first search.             */
/*                                                                           */
/*****************************************************************************/

static bool KheMatchingAssignBFS(KHE_MATCHING_DEMAND_NODE dn,
  unsigned int visit_num)
{
  int i, j, base;  KHE_MATCHING_SUPPLY_NODE sn, s;
  ARRAY_KHE_MATCHING_SUPPLY_NODE supply_nodes;
  KHE_MATCHING_DEMAND_NODE bfs_first, bfs_last, d;
  KHE_MATCHING_DEMAND_CHUNK dc;

  /* initialize the queue to contain just dn */
  bfs_first = bfs_last = dn;
  bfs_last->bfs_parent = bfs_last->bfs_next = NULL;

  /* each iteration processes one item, until success or queue is empty */
  supply_nodes = bfs_first->demand_chunk->matching->supply_nodes;
  do
  {
    /* add bfs_first's successors to the queue, unless one is unassigned */
    dc = bfs_first->demand_chunk;
    for( i = 0;  i < KheSetCount(dc->domain);  i++ )
    {
      base = dc->base + dc->increment * KheSetGet(dc->domain, i);
      for( j = 0;  j < SSetCount(bfs_first->domain);  j++ )
      {
	sn = HaArray(supply_nodes, base + KheSetGet(bfs_first->domain, j));
	if( sn->visit_num < visit_num )
	{
	  sn->visit_num = visit_num;
	  /* sn->demand_test_index = j; */
	  if( sn->supply_asst == NULL )
	  {
	    /* unassigned, unwind tree and make the augmenting assignments */
	    d = bfs_first;  s = sn;
	    do
	    {
	      sn = d->demand_asst;
	      s->supply_asst = d;
	      d->demand_asst = s;
	      d->demand_asst_index = j;  /* s->demand_test_index; */
	      d = d->bfs_parent;  s = sn;
	    } while( d != NULL );
	    return true;
	  }
	  else
	  {
	    /* assigned, add its demand node to the queue to be handled later */
	    bfs_last->bfs_next = sn->supply_asst;
	    bfs_last = bfs_last->bfs_next;
	    bfs_last->bfs_next = NULL;
	    bfs_last->bfs_parent = bfs_first;
	  }
	}
      }
    }
    bfs_first = bfs_first->bfs_next;
  } while( bfs_first != NULL );
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAddUnMatchedDemandNode(KHE_MATCHING m,                           */
/*    KHE_MATCHING_DEMAND_NODE dn)                                           */
/*                                                                           */
/*  Add dn to the set of unmatched demand nodes of m.                        */
/*                                                                           */
/*****************************************************************************/

static void KheAddUnMatchedDemandNode(KHE_MATCHING m,
  KHE_MATCHING_DEMAND_NODE dn)
{
  KHE_COST weight;
  HnAssert(dn->demand_chunk->matching == m, "KheAddUnMatchedDemandNode sanity");
  HnAssert(dn->unmatched_pos == -1,
    "KheAddUnMatchedDemandNode internal error (unmatched_pos)");
  HnAssert(dn->demand_asst == NULL,
    "KheAddUnMatchedDemandNode internal error (demand_asst)");
  dn->unmatched_pos = HaArrayCount(m->unmatched_demand_nodes);
  HaArrayAddLast(m->unmatched_demand_nodes, dn);
  HnAssert(dn->cost == 0, "KheAddUnMatchedDemandNode internal error");
  weight = KheSolnMatchingWeight(dn->soln);
  if( weight > 0 )
    KheMonitorChangeCost((KHE_MONITOR) dn, weight);
  KheDemandNodeCheck(dn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRemoveUnMatchedDemandNode(KHE_MATCHING m,                        */
/*    KHE_MATCHING_DEMAND_NODE dn)                                           */
/*                                                                           */
/*  Remove dn from the set of unmatched demand nodes of m.                   */
/*                                                                           */
/*  Implementation note.  When this function is called, dn->demand_asst      */
/*  may be either NULL or non-NULL, the latter case occurring because the    */
/*  node has just been assigned.                                             */
/*                                                                           */
/*****************************************************************************/

static void KheRemoveUnMatchedDemandNode(KHE_MATCHING m,
  KHE_MATCHING_DEMAND_NODE dn)
{
  KHE_MATCHING_DEMAND_NODE x;
  HnAssert(0 <= dn->unmatched_pos,
    "KheRemoveUnMatchedDemandNode internal error 1");
  HnAssert(dn->unmatched_pos < HaArrayCount(m->unmatched_demand_nodes),
    "KheRemoveUnMatchedDemandNode internal error 2");
  HnAssert(HaArray(m->unmatched_demand_nodes, dn->unmatched_pos) == dn,
    "KheRemoveUnMatchedDemandNode internal error 3");
  HaArrayDeleteAndPlug(m->unmatched_demand_nodes, dn->unmatched_pos);
  if( dn->unmatched_pos < HaArrayCount(m->unmatched_demand_nodes) )
  {
    x = HaArray(m->unmatched_demand_nodes, dn->unmatched_pos);
    x->unmatched_pos = dn->unmatched_pos;
  }
  dn->unmatched_pos = -1;
  if( dn->cost > 0 )
    KheMonitorChangeCost((KHE_MONITOR) dn, 0);
  /* this does not apply here! KheDemandNodeCheck(dn); */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingCheckInvt(KHE_MATCHING m)                                */
/*                                                                           */
/*  Check (part of) the invariant of m.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheMatchingCheckInvt(KHE_MATCHING m)
{
  int i;  KHE_MATCHING_DEMAND_NODE dn;
  HaArrayForEach(m->unmatched_demand_nodes, dn, i)
    HnAssert(dn->demand_asst == NULL, "KheMatchingCheckInvt internal error");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingCheckLowerBound(KHE_MATCHING m)                          */
/*                                                                           */
/*  Check that m->unmatched_lower_bound really is a lower bound on the       */
/*  number of unmatched demand nodes.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheMatchingCheckLowerBound(KHE_MATCHING m)
{
  HnAssert(0 <= m->unmatched_lower_bound,
    "KheMatchingCheckLowerBound internal error 1 (failed 0 <= %d)",
    m->unmatched_lower_bound);
  HnAssert(m->unmatched_lower_bound <= HaArrayCount(m->unmatched_demand_nodes),
    "KheMatchingCheckLowerBound internal error 2 (failed %d <= %d)",
    m->unmatched_lower_bound, HaArrayCount(m->unmatched_demand_nodes));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingMakeClean(KHE_MATCHING m)                                */
/*                                                                           */
/*  Ensure that m is up to date, by attempting to assign each unassigned     */
/*  demand node.  If the matching is not already up to date, also make sure  */
/*  that there are no Hall sets.                                             */
/*                                                                           */
/*  Implementation note.  This function optimizes by trying to assign the    */
/*  unmatched nodes in reverse order of insertion, trying a previously       */
/*  successful domain element before embarking on the general assignment     */
/*  algorithm, not incrementing the visit number when the immediately        */
/*  preceding assignment attempt failed, and stopping early if the lower     */
/*  bound is achieved.                                                       */
/*                                                                           */
/*****************************************************************************/
/* static void KheMatchingHallSetFree(KHE_MATCHING_HALL_SET hs); */
static void KheMatchingHallSetDelete(KHE_MATCHING_HALL_SET hs, KHE_SOLN soln);

static void KheMatchingMakeClean(KHE_MATCHING m)
{
  int i, j, base;  KHE_MATCHING_DEMAND_NODE dn;  bool visit_inc_needed_next;
  KHE_MATCHING_DEMAND_CHUNK dc;  KHE_MATCHING_SUPPLY_NODE sn;
  ARRAY_KHE_MATCHING_SUPPLY_NODE supply_nodes;  KHE_MATCHING_HALL_SET hs;
  KheMatchingCheckLowerBound(m);
  if( m->unmatched_lower_bound < HaArrayCount(m->unmatched_demand_nodes) )
  {
    /* lower bound not reached, so assigmments are required */
    if( DEBUG1 )
      KheMatchingCheckInvt(m);
    visit_inc_needed_next = true;
    HaArrayForEachReverse(m->unmatched_demand_nodes, dn, i)
    {
      /* see whether a previously successful elt of dn->domain will work now */
      HnAssert(dn->demand_asst==NULL, "KheMatchingMakeClean internal error 2");
      if( dn->demand_asst_index < SSetCount(dn->domain) )
      {
	dc = dn->demand_chunk;
	supply_nodes = dc->matching->supply_nodes;
	for( j = 0;  j < KheSetCount(dc->domain);  j++ )
	{
	  base = dc->base + dc->increment * KheSetGet(dc->domain, j);
	  sn = HaArray(supply_nodes,
	    base + KheSetGet(dn->domain, dn->demand_asst_index));
	  if( sn->supply_asst == NULL )
	  {
	    sn->supply_asst = dn;
	    dn->demand_asst = sn;
	    break;
	  }
	}
      }

      /* if still unassigned, call KheMatchingAssignBFS */
      if( dn->demand_asst == NULL )
      {
	if( visit_inc_needed_next )
	  m->visit_num++;
	visit_inc_needed_next = KheMatchingAssignBFS(dn, m->visit_num);
      }

      /* if dn is assigned, report it to m and quit if lower bound reached */
      if( dn->demand_asst != NULL )
      {
	KheRemoveUnMatchedDemandNode(m, dn);
	KheMatchingCheckLowerBound(m);
        if( m->unmatched_lower_bound == HaArrayCount(m->unmatched_demand_nodes))
	  break;
      }
      KheDemandNodeCheck(dn);
      if( DEBUG1 )
	KheMatchingCheckInvt(m);
    }

    /* bring lower bound up to date and make sure there are no Hall sets */
    m->unmatched_lower_bound = HaArrayCount(m->unmatched_demand_nodes);
    HaArrayForEach(m->hall_sets, hs, i)
      KheMatchingHallSetDelete(hs, m->soln);
    /* HaArrayAppend(m->free_hall_sets, m->hall_sets, i); */
    HaArrayClear(m->hall_sets);
    /* ***
    while( HaArrayCount(m->hall_sets) > 0 )
      KheMatchingHallSetFree(HaArrayLastAndDelete(m->hall_sets));
    *** */
    if( DEBUG1 )
      KheMatchingCheckInvt(m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MATCHING_SUPPLY_CHUNK"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_SUPPLY_CHUNK KheMatchingSupplyChunkMake(KHE_MATCHING m,     */
/*    void *impl)                                                            */
/*                                                                           */
/*  Make a supply chunk with these attributes.  Don't add any supply nodes.  */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_SUPPLY_CHUNK KheMatchingSupplyChunkMake(KHE_MATCHING m, void *impl)
{
  KHE_MATCHING_SUPPLY_CHUNK res;  HA_ARENA a;
  res = KheSolnGetMatchingSupplyChunkFromFreeList(m->soln);
  if( res != NULL )
  {
    HaArrayClear(res->supply_nodes);
  }
  else
  {
    a = KheSolnArena(m->soln);
    HaMake(res, a);
    HaArrayInit(res->supply_nodes, a);
  }
  res->matching = m;
  res->index_in_matching = HaArrayCount(m->supply_chunks);
  res->impl = impl;
  res->base = KheMatchingSupplyNodeCount(m);
  HaArrayAddLast(m->supply_chunks, res);
  res->copy = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingSupplyChunkDelete(KHE_MATCHING_SUPPLY_CHUNK sc)          */
/*                                                                           */
/*  Delete sc and its supply nodes.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheMatchingSupplyNodeDelete(KHE_MATCHING_SUPPLY_NODE sn);

void KheMatchingSupplyChunkDelete(KHE_MATCHING_SUPPLY_CHUNK sc)
{
  KHE_MATCHING_SUPPLY_CHUNK tmp;
  /* *** allowing (indeed expecting) supply nodes now
  HnAssert(HaArrayCount(sc->supply_nodes) == 0,
    "KheMatchingSupplyChunkDelete: chunk not empty");
  *** */
  HaArrayDeleteAndPlug(sc->matching->supply_chunks, sc->index_in_matching);
  if( sc->index_in_matching < HaArrayCount(sc->matching->supply_chunks) )
  {
    tmp = HaArray(sc->matching->supply_chunks, sc->index_in_matching);
    tmp->index_in_matching = sc->index_in_matching;
  }
  while( HaArrayCount(sc->supply_nodes) > 0 )
    KheMatchingSupplyNodeDelete(HaArrayLastAndDelete(sc->supply_nodes));
  KheSolnAddMatchingSupplyChunkToFreeList(sc->matching->soln, sc);
  /* ***
  MArrayFree(sc->supply_nodes);
  MFree(sc);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING KheMatchingSupplyChunkMatching(KHE_MATCHING_SUPPLY_CHUNK sc)*/
/*                                                                           */
/*  Return the matching attribute of sc.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING KheMatchingSupplyChunkMatching(KHE_MATCHING_SUPPLY_CHUNK sc)
{
  return sc->matching;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheMatchingSupplyChunkImpl(KHE_MATCHING_SUPPLY_CHUNK sc)           */
/*                                                                           */
/*  Return the impl attribute of sc.                                         */
/*                                                                           */
/*****************************************************************************/

void *KheMatchingSupplyChunkImpl(KHE_MATCHING_SUPPLY_CHUNK sc)
{
  return sc->impl;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingSupplyChunkSetImpl(KHE_MATCHING_SUPPLY_CHUNK sc,         */
/*    void *impl)                                                            */
/*                                                                           */
/*  Reset the impl pointer of sc.                                            */
/*                                                                           */
/*****************************************************************************/

void KheMatchingSupplyChunkSetImpl(KHE_MATCHING_SUPPLY_CHUNK sc, void *impl)
{
  sc->impl = impl;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMatchingSupplyChunkBase(KHE_MATCHING_SUPPLY_CHUNK sc)             */
/*                                                                           */
/*  Return the base of sc.  This is the number of supply nodes that          */
/*  existed in sc's matching at the moment sc was created.                   */
/*                                                                           */
/*****************************************************************************/

int KheMatchingSupplyChunkBase(KHE_MATCHING_SUPPLY_CHUNK sc)
{
  return sc->base;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_SUPPLY_CHUNK KheMatchingSupplyChunkCopyPhase1(              */
/*    KHE_MATCHING_SUPPLY_CHUNK sc, HA_ARENA a)                              */
/*                                                                           */
/*  Carry out Phase 1 of copying sc.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_SUPPLY_CHUNK KheMatchingSupplyChunkCopyPhase1(
  KHE_MATCHING_SUPPLY_CHUNK sc, HA_ARENA a)
{
  KHE_MATCHING_SUPPLY_CHUNK copy;  KHE_MATCHING_SUPPLY_NODE sn;  int i;
  if( sc->copy == NULL )
  {
    HaMake(copy, a);
    sc->copy = copy;
    copy->matching = KheMatchingCopyPhase1(sc->matching, a);
    copy->index_in_matching = sc->index_in_matching;
    copy->impl = sc->impl;
    copy->base = sc->base;
    HaArrayInit(copy->supply_nodes, a);
    HaArrayForEach(sc->supply_nodes, sn, i)
      HaArrayAddLast(copy->supply_nodes, KheMatchingSupplyNodeCopyPhase1(sn,a));
    copy->copy = NULL;
  }
  return sc->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingSupplyChunkCopyPhase2(KHE_MATCHING_SUPPLY_CHUNK sc)      */
/*                                                                           */
/*  Carry out Phase 2 of copying sc.                                         */
/*                                                                           */
/*****************************************************************************/

void KheMatchingSupplyChunkCopyPhase2(KHE_MATCHING_SUPPLY_CHUNK sc)
{
  KHE_MATCHING_SUPPLY_NODE sn;  int i;
  if( sc->copy != NULL )
  {
    sc->copy = NULL;
    KheMatchingCopyPhase2(sc->matching);
    HaArrayForEach(sc->supply_nodes, sn, i)
      KheMatchingSupplyNodeCopyPhase2(sn);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMatchingSupplyChunkSupplyNodeCount(KHE_MATCHING_SUPPLY_CHUNK sc)  */
/*                                                                           */
/*  Return the number of supply nodes in sc.                                 */
/*                                                                           */
/*****************************************************************************/

int KheMatchingSupplyChunkSupplyNodeCount(KHE_MATCHING_SUPPLY_CHUNK sc)
{
  return HaArrayCount(sc->supply_nodes);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_SUPPLY_NODE KheMatchingSupplyChunkSupplyNode(               */
/*    KHE_MATCHING_SUPPLY_CHUNK sc, int i)                                   */
/*                                                                           */
/*  Return the i'th supply node of sc.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_SUPPLY_NODE KheMatchingSupplyChunkSupplyNode(
  KHE_MATCHING_SUPPLY_CHUNK sc, int i)
{
  return HaArray(sc->supply_nodes, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MATCHING_SUPPLY_NODE"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_SUPPLY_NODE KheMatchingSupplyNodeMake(                      */
/*    KHE_MATCHING_SUPPLY_CHUNK sc, void *impl)                              */
/*                                                                           */
/*  Make a supply node with these attributes and add it to sc.               */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_SUPPLY_NODE KheMatchingSupplyNodeMake(
  KHE_MATCHING_SUPPLY_CHUNK sc, void *impl)
{
  KHE_MATCHING_SUPPLY_NODE res;  HA_ARENA a;
  res = KheSolnGetMatchingSupplyNodeFromFreeList(sc->matching->soln);
  if( res == NULL )
  {
    a = KheSolnArena(sc->matching->soln);
    HaMake(res, a);
  }
  res->supply_chunk = sc;
  res->impl = impl;
  res->index = HaArrayCount(sc->matching->supply_nodes);
  HaArrayAddLast(sc->matching->supply_nodes, res);
  HaArrayAddLast(sc->supply_nodes, res);
  res->visit_num = 0;
  res->supply_asst = NULL;
  res->hall_set = NULL;
  /* res->demand_test_index = 0; */
  res->copy = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingSupplyNodeDelete(KHE_MATCHING_SUPPLY_NODE sn)            */
/*                                                                           */
/*  Free sn.  We're assuming that it has already been deleted from its       */
/*  chunk.                                                                   */
/*                                                                           */
/*****************************************************************************/

static void KheMatchingSupplyNodeDelete(KHE_MATCHING_SUPPLY_NODE sn)
{
  KheSolnAddMatchingSupplyNodeToFreeList(sn->supply_chunk->matching->soln, sn);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_SUPPLY_CHUNK KheMatchingSupplyNodeChunk(                    */
/*    KHE_MATCHING_SUPPLY_NODE sn)                                           */
/*                                                                           */
/*  Return the supply_chunk attribute of sn.                                 */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_SUPPLY_CHUNK KheMatchingSupplyNodeChunk(
  KHE_MATCHING_SUPPLY_NODE sn)
{
  return sn->supply_chunk;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING KheMatchingSupplyNodeMatching(KHE_MATCHING_SUPPLY_NODE sn)  */
/*                                                                           */
/*  Return the matching containing sn.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING KheMatchingSupplyNodeMatching(KHE_MATCHING_SUPPLY_NODE sn)
{
  return sn->supply_chunk->matching;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheMatchingSupplyNodeImpl(KHE_MATCHING_SUPPLY_NODE sn)             */
/*                                                                           */
/*  Return the impl attribute of sn.                                         */
/*                                                                           */
/*****************************************************************************/

void *KheMatchingSupplyNodeImpl(KHE_MATCHING_SUPPLY_NODE sn)
{
  return sn->impl;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingSupplyNodeSetImpl(KHE_MATCHING_SUPPLY_NODE sn,           */
/*    void *impl)                                                            */
/*                                                                           */
/*  Reset the impl attribute of sn.                                          */
/*                                                                           */
/*****************************************************************************/

void KheMatchingSupplyNodeSetImpl(KHE_MATCHING_SUPPLY_NODE sn, void *impl)
{
  sn->impl = impl;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMatchingSupplyNodeIndex(KHE_MATCHING_SUPPLY_NODE sn)              */
/*                                                                           */
/*  Return the index number attribute of sn.  This is just the number        */
/*  of supply nodes present just before sn was created.                      */
/*                                                                           */
/*****************************************************************************/

int KheMatchingSupplyNodeIndex(KHE_MATCHING_SUPPLY_NODE sn)
{
  return sn->index;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_SUPPLY_NODE KheMatchingSupplyNodeCopyPhase1(                */
/*    KHE_MATCHING_SUPPLY_NODE sn, HA_ARENA a)                               */
/*                                                                           */
/*  Carry out Phase 1 of copying sn.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_SUPPLY_NODE KheMatchingSupplyNodeCopyPhase1(
  KHE_MATCHING_SUPPLY_NODE sn, HA_ARENA a)
{
  KHE_MATCHING_SUPPLY_NODE copy;
  if( sn->copy == NULL )
  {
    HaMake(copy, a);
    sn->copy = copy;
    copy->supply_chunk = KheMatchingSupplyChunkCopyPhase1(sn->supply_chunk, a);
    copy->impl = sn->impl;
    copy->index = sn->index;
    copy->visit_num = sn->visit_num;
    copy->supply_asst = (sn->supply_asst == NULL ? NULL :
      KheMatchingDemandNodeCopyPhase1(sn->supply_asst, a));
    copy->hall_set = sn->hall_set == NULL ? NULL :
      KheMatchingHallSetCopyPhase1(sn->hall_set, a);
    /* copy->demand_test_index = sn->demand_test_index; */
    copy->copy = NULL;
  }
  return sn->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingSupplyNodeCopyPhase2(KHE_MATCHING_SUPPLY_NODE sn)        */
/*                                                                           */
/*  Carry out Phase 2 of copying sn.                                         */
/*                                                                           */
/*****************************************************************************/

void KheMatchingSupplyNodeCopyPhase2(KHE_MATCHING_SUPPLY_NODE sn)
{
  if( sn->copy != NULL )
  {
    sn->copy = NULL;
    KheMatchingSupplyChunkCopyPhase2(sn->supply_chunk);
    if( sn->supply_asst != NULL )
      KheMatchingDemandNodeCopyPhase2(sn->supply_asst);
    if( sn->hall_set != NULL )
      KheMatchingHallSetCopyPhase2(sn->hall_set);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MATCHING_DEMAND_CHUNK"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_DEMAND_CHUNK KheMatchingDemandChunkMake(KHE_MATCHING m,     */
/*    void *impl, int base, int increment, SSET *domain)                     */
/*                                                                           */
/*  Make a demand chunk with these attributes.                               */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_DEMAND_CHUNK KheMatchingDemandChunkMake(KHE_MATCHING m,
  void *impl, int base, int increment, SSET *domain)
{
  KHE_MATCHING_DEMAND_CHUNK res;  HA_ARENA a;
  res = KheSolnGetMatchingDemandChunkFromFreeList(m->soln);
  if( res != NULL )
  {
    HaArrayClear(res->demand_nodes);
  }
  else
  {
    a = KheSolnArena(m->soln);
    HaMake(res, a);
    HaArrayInit(res->demand_nodes, a);
  }
  res->matching = m;
  res->index_in_matching = HaArrayCount(m->demand_chunks);
  res->impl = impl;
  res->base = base;
  res->increment = increment;
  res->domain = *domain;
  HaArrayAddLast(m->demand_chunks, res);
  res->copy = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING KheMatchingDemandChunkMatching(KHE_MATCHING_DEMAND_CHUNK dc)*/
/*                                                                           */
/*  Return the matching attribute of dc.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING KheMatchingDemandChunkMatching(KHE_MATCHING_DEMAND_CHUNK dc)
{
  return dc->matching;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheMatchingDemandChunkImpl(KHE_MATCHING_DEMAND_CHUNK dc)           */
/*                                                                           */
/*  Return the impl attribute of dc.                                         */
/*                                                                           */
/*****************************************************************************/

void *KheMatchingDemandChunkImpl(KHE_MATCHING_DEMAND_CHUNK dc)
{
  return dc->impl;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingDemandChunkSetImpl(KHE_MATCHING_DEMAND_CHUNK dc,         */
/*    void *impl)                                                            */
/*                                                                           */
/*  Reset the impl attribute of dc.                                          */
/*                                                                           */
/*****************************************************************************/

void KheMatchingDemandChunkSetImpl(KHE_MATCHING_DEMAND_CHUNK dc, void *impl)
{
  dc->impl = impl;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingDemandChunkDelete(KHE_MATCHING_DEMAND_CHUNK dc)          */
/*                                                                           */
/*  Delete dc and free its memory.  It must have no demand nodes.            */
/*                                                                           */
/*****************************************************************************/

void KheMatchingDemandChunkDelete(KHE_MATCHING_DEMAND_CHUNK dc)
{
  KHE_MATCHING_DEMAND_CHUNK tmp;
  HnAssert(HaArrayCount(dc->demand_nodes) == 0,
    "KheMatchingDemandChunkDelete: dc contains demand nodes");
  HaArrayDeleteAndPlug(dc->matching->demand_chunks, dc->index_in_matching);
  if( dc->index_in_matching < HaArrayCount(dc->matching->demand_chunks) )
  {
    tmp = HaArray(dc->matching->demand_chunks, dc->index_in_matching);
    tmp->index_in_matching = dc->index_in_matching;
  }
  KheSolnAddMatchingDemandChunkToFreeList(dc->matching->soln, dc);
  /* ***
  MArrayFree(dc->demand_nodes);
  MFree(dc);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_DEMAND_CHUNK KheMatchingDemandChunkCopyPhase1(              */
/*    KHE_MATCHING_DEMAND_CHUNK dc, HA_ARENA a)                              */
/*                                                                           */
/*  Carry out Phase 1 of copying dc.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_DEMAND_CHUNK KheMatchingDemandChunkCopyPhase1(
  KHE_MATCHING_DEMAND_CHUNK dc, HA_ARENA a)
{
  KHE_MATCHING_DEMAND_CHUNK copy;  KHE_MATCHING_DEMAND_NODE dn;  int i;
  if( dc->copy == NULL )
  {
    HaMake(copy, a);
    dc->copy = copy;
    copy->matching = KheMatchingCopyPhase1(dc->matching, a);
    copy->index_in_matching = dc->index_in_matching;
    copy->impl = dc->impl;
    copy->base = dc->base;
    copy->increment = dc->increment;
    SSetCopy(copy->domain, dc->domain, a);
    HaArrayInit(copy->demand_nodes, a);
    HaArrayForEach(dc->demand_nodes, dn, i)
      HaArrayAddLast(copy->demand_nodes, KheMatchingDemandNodeCopyPhase1(dn,a));
    copy->copy = NULL;
  }
  return dc->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingDemandChunkCopyPhase2(KHE_MATCHING_DEMAND_CHUNK dc)      */
/*                                                                           */
/*  Carry out Phase 2 of copying dc.                                         */
/*                                                                           */
/*****************************************************************************/

void KheMatchingDemandChunkCopyPhase2(KHE_MATCHING_DEMAND_CHUNK dc)
{
  KHE_MATCHING_DEMAND_NODE dn;  int i;
  if( dc->copy != NULL )
  {
    dc->copy = NULL;
    KheMatchingCopyPhase2(dc->matching);
    HaArrayForEach(dc->demand_nodes, dn, i)
      KheMatchingDemandNodeCopyPhase2(dn);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMatchingDemandChunkBase(KHE_MATCHING_DEMAND_CHUNK dc)             */
/*                                                                           */
/*  Return the base attribute of dc.                                         */
/*                                                                           */
/*****************************************************************************/

int KheMatchingDemandChunkBase(KHE_MATCHING_DEMAND_CHUNK dc)
{
  return dc->base;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMatchingDemandChunkIncrement(KHE_MATCHING_DEMAND_CHUNK dc)        */
/*                                                                           */
/*  Return the increment attribute of dc.                                    */
/*                                                                           */
/*****************************************************************************/

int KheMatchingDemandChunkIncrement(KHE_MATCHING_DEMAND_CHUNK dc)
{
  return dc->increment;
}


/*****************************************************************************/
/*                                                                           */
/*  SSET *KheMatchingDemandChunkDomain(KHE_MATCHING_DEMAND_CHUNK dc)         */
/*                                                                           */
/*  Return the domain attribute of dc.                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
SSET *KheMatchingDemandChunkDomain(KHE_MATCHING_DEMAND_CHUNK dc)
{
  return &dc->domain;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingDemandChunkSetBase(KHE_MATCHING_DEMAND_CHUNK dc,         */
/*    int base)                                                              */
/*                                                                           */
/*  Set the base attribute of dc.                                            */
/*                                                                           */
/*****************************************************************************/
static void KheMatchingDemandNodeDomainHasChanged(KHE_MATCHING_DEMAND_NODE dn,
  KHE_MATCHING_DOMAIN_CHANGE_TYPE domain_change_type);

void KheMatchingDemandChunkSetBase(KHE_MATCHING_DEMAND_CHUNK dc, int base)
{
  KHE_MATCHING_DEMAND_NODE dn;  int i;
  HnAssert(!dc->matching->active, "KheMatchingDemandChunkSetBase reentry in %p",
    (void *) dc->matching->soln);
  dc->matching->active = true;
  if( base != dc->base )
  {
    dc->base = base;
    HaArrayForEach(dc->demand_nodes, dn, i)
      KheMatchingDemandNodeDomainHasChanged(dn,
	KHE_MATCHING_DOMAIN_CHANGE_TO_OTHER);
  }
  dc->matching->active = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingDemandChunkSetIncrement(KHE_MATCHING_DEMAND_CHUNK dc,    */
/*    int increment)                                                         */
/*                                                                           */
/*  Set the increment attribute of dc.                                       */
/*                                                                           */
/*****************************************************************************/

void KheMatchingDemandChunkSetIncrement(KHE_MATCHING_DEMAND_CHUNK dc,
  int increment)
{
  KHE_MATCHING_DEMAND_NODE dn;  int i;
  HnAssert(!dc->matching->active, "KheMatchingDemandChunkSetIncrement "
    "reentry in %p", (void *) dc->matching->soln);
  dc->matching->active = true;
  if( increment != dc->increment )
  {
    dc->increment = increment;
    HaArrayForEach(dc->demand_nodes, dn, i)
      KheMatchingDemandNodeDomainHasChanged(dn,
	KHE_MATCHING_DOMAIN_CHANGE_TO_OTHER);
  }
  dc->matching->active = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingDemandChunkSetDomain(KHE_MATCHING_DEMAND_CHUNK dc,       */
/*    SSET *domain, KHE_MATCHING_DOMAIN_CHANGE_TYPE domain_change_type)      */
/*                                                                           */
/*  Set the domain attribute of dc.                                          */
/*                                                                           */
/*****************************************************************************/

void KheMatchingDemandChunkSetDomain(KHE_MATCHING_DEMAND_CHUNK dc,
  SSET *domain, KHE_MATCHING_DOMAIN_CHANGE_TYPE domain_change_type)
{
  KHE_MATCHING_DEMAND_NODE dn;  int i;
  HnAssert(!dc->matching->active, "KheMatchingDemandChunkSetBase reentry in %p",
    (void *) dc->matching->soln);
  dc->matching->active = true;
  dc->domain = *domain;
  HaArrayForEach(dc->demand_nodes, dn, i)
    KheMatchingDemandNodeDomainHasChanged(dn, domain_change_type);
  dc->matching->active = false;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMatchingDemandChunkNodeCount(KHE_MATCHING_DEMAND_CHUNK dc)        */
/*                                                                           */
/*  Return the number of demand nodes in dc.                                 */
/*                                                                           */
/*****************************************************************************/

int KheMatchingDemandChunkNodeCount(KHE_MATCHING_DEMAND_CHUNK dc)
{
  return HaArrayCount(dc->demand_nodes);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_DEMAND_NODE KheMatchingDemandChunkNode(                     */
/*    KHE_MATCHING_DEMAND_CHUNK dc, int i)                                   */
/*                                                                           */
/*  Return the i'th demand node of dc.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_DEMAND_NODE KheMatchingDemandChunkNode(
  KHE_MATCHING_DEMAND_CHUNK dc, int i)
{
  return HaArray(dc->demand_nodes, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MATCHING_DEMAND_NODE"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_DEMAND_CHUNK KheMatchingDemandNodeChunk(                    */
/*    KHE_MATCHING_DEMAND_NODE dn)                                           */
/*                                                                           */
/*  Return the demand chunk attribute of dn.                                 */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_DEMAND_CHUNK KheMatchingDemandNodeChunk(
  KHE_MATCHING_DEMAND_NODE dn)
{
  return dn->demand_chunk;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_DEMAND_NODE KheMatchingDemandNodeCopyPhase1(                */
/*    KHE_MATCHING_DEMAND_NODE dn, HA_ARENA a)                               */
/*                                                                           */
/*  Carry out Phase 1 of copying dn.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_DEMAND_NODE KheMatchingDemandNodeCopyPhase1(
  KHE_MATCHING_DEMAND_NODE dn, HA_ARENA a)
{
  if( dn->tag == KHE_ORDINARY_DEMAND_MONITOR_TAG )
    return (KHE_MATCHING_DEMAND_NODE) KheOrdinaryDemandMonitorCopyPhase1(
      (KHE_ORDINARY_DEMAND_MONITOR) dn, a);
  else if( dn->tag == KHE_WORKLOAD_DEMAND_MONITOR_TAG )
    return (KHE_MATCHING_DEMAND_NODE) KheWorkloadDemandMonitorCopyPhase1(
      (KHE_WORKLOAD_DEMAND_MONITOR) dn, a);
  else
  {
    HnAbort("KheMatchingDemandNodeCopyPhase1 internal error");
    return NULL;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingDemandNodeCopyPhase2(KHE_MATCHING_DEMAND_NODE dn)        */
/*                                                                           */
/*  Carry out Phase 2 of copying dn.                                         */
/*                                                                           */
/*****************************************************************************/

void KheMatchingDemandNodeCopyPhase2(KHE_MATCHING_DEMAND_NODE dn)
{
  if( dn->tag == KHE_ORDINARY_DEMAND_MONITOR_TAG )
    KheOrdinaryDemandMonitorCopyPhase2((KHE_ORDINARY_DEMAND_MONITOR) dn);
  else if( dn->tag == KHE_WORKLOAD_DEMAND_MONITOR_TAG )
    KheWorkloadDemandMonitorCopyPhase2((KHE_WORKLOAD_DEMAND_MONITOR) dn);
  else
    HnAbort("KheMatchingDemandNodeCopyPhase2 internal error");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingDemandChunkAddDemandNode(KHE_MATCHING_DEMAND_CHUNK dc,   */
/*    KHE_MATCHING_DEMAND_NODE dn)                                           */
/*                                                                           */
/*  Add dn to dc.                                                            */
/*                                                                           */
/*****************************************************************************/

void KheMatchingDemandNodeAdd(KHE_MATCHING_DEMAND_NODE dn)
{
  HnAssert(!dn->demand_chunk->matching->active, "KheMatchingDemandNodeAdd "
    "reentry in %p", (void *) dn->demand_chunk->matching->soln);
  dn->demand_chunk->matching->active = true;
  HaArrayAddLast(dn->demand_chunk->demand_nodes, dn);
  dn->demand_chunk->matching->demand_node_count++;
  KheAddUnMatchedDemandNode(dn->demand_chunk->matching, dn);
  dn->demand_chunk->matching->active = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingDemandNodeDelete(KHE_MATCHING_DEMAND_NODE dn)            */
/*                                                                           */
/*  Delete dn, but do not free it.                                           */
/*                                                                           */
/*****************************************************************************/

void KheMatchingDemandNodeDelete(KHE_MATCHING_DEMAND_NODE dn)
{
  int pos;  KHE_MATCHING m;

  /* decrement the total number of demand nodes */
  m = dn->demand_chunk->matching;
  HnAssert(!m->active, "KheMatchingDemandNodeDelete reentry in %p",
    (void *) m->soln);
  m->active = true;
  m->demand_node_count--;

  /* ensure dn is deassigned and not on the unmatched demand node list */
  if( dn->demand_asst != NULL )
  {
    dn->demand_asst->supply_asst = NULL;
    dn->demand_asst = NULL;
  }
  else
    KheRemoveUnMatchedDemandNode(m, dn);

  /* remove dn from its chunk */
  if( !HaArrayContains(dn->demand_chunk->demand_nodes, dn, &pos) )
    HnAbort("KheMatchingDemandNodeDelete internal error");
  HaArrayDeleteAndShift(dn->demand_chunk->demand_nodes, pos);

  /* reduce m's lower bound */
  if( m->unmatched_lower_bound > 0 )
    m->unmatched_lower_bound--;
  KheMatchingCheckLowerBound(m);
  m->active = false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SET *KheMatchingDemandNodeDomain(KHE_MATCHING_DEMAND_NODE dn)        */
/*                                                                           */
/*  Return the domain attribute of dn.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_SET *KheMatchingDemandNodeDomain(KHE_MATCHING_DEMAND_NODE dn)
{
  return &dn->domain;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingDemandNodeDomainHasChanged(KHE_MATCHING_DEMAND_NODE dn,  */
/*    KHE_MATCHING_DOMAIN_CHANGE_TYPE domain_change_type)                    */
/*                                                                           */
/*  Update lower bounds and assignments appropriately, given that the        */
/*  domain of dn has just changed according to domain_change_type.           */
/*                                                                           */
/*****************************************************************************/

static void KheMatchingDemandNodeDomainHasChanged(KHE_MATCHING_DEMAND_NODE dn,
  KHE_MATCHING_DOMAIN_CHANGE_TYPE domain_change_type)
{
  KHE_MATCHING m;

  /* decrease lower bound unless new domain is subset or already at 0 */
  m = dn->demand_chunk->matching;
  if( domain_change_type != KHE_MATCHING_DOMAIN_CHANGE_TO_SUBSET &&
      m->unmatched_lower_bound > 0 )
    m->unmatched_lower_bound--;
  KheMatchingCheckLowerBound(m);

  /* if assigned and not changing to a superset, deassign */
  if( dn->demand_asst != NULL &&
      domain_change_type != KHE_MATCHING_DOMAIN_CHANGE_TO_SUPERSET )
  {
    dn->demand_asst->supply_asst = NULL;
    dn->demand_asst = NULL;
    KheAddUnMatchedDemandNode(m, dn);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingDemandNodeSetDomain(KHE_MATCHING_DEMAND_NODE dn,         */
/*    KHE_SET *domain, KHE_MATCHING_DOMAIN_CHANGE_TYPE domain_change_type)   */
/*                                                                           */
/*  Set the domain attribute of dn.                                          */
/*                                                                           */
/*****************************************************************************/

void KheMatchingDemandNodeSetDomain(KHE_MATCHING_DEMAND_NODE dn,
  KHE_SET *domain, KHE_MATCHING_DOMAIN_CHANGE_TYPE domain_change_type)
{
  HnAssert(!dn->demand_chunk->matching->active,
    "KheMatchingDemandNodeSetDomain reentry in %p",
    (void *) dn->demand_chunk->matching->soln);
  dn->demand_chunk->matching->active = true;
  dn->domain = *domain;
  KheMatchingDemandNodeDomainHasChanged(dn, domain_change_type);
  dn->demand_chunk->matching->active = false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MATCHING"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING KheMatchingMake(KHE_SOLN soln)                              */
/*                                                                           */
/*  Make a new, empty bipartite matching graph with these attributes,        */
/*  in soln's arena.                                                         */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING KheMatchingMake(KHE_SOLN soln)
{
  KHE_MATCHING res;  HA_ARENA a;
  res = KheSolnGetMatchingFromFreeList(soln);
  if( res != NULL )
  {
#if USE_SNAPSHOT
    HaArrayClear(res->snapshot_free_list);
#endif
    HaArrayClear(res->supply_chunks);
    HaArrayClear(res->demand_chunks);
    HaArrayClear(res->supply_nodes);
    HaArrayClear(res->unmatched_demand_nodes);
    HaArrayClear(res->marks);
#if USE_SNAPSHOT
    HaArrayClear(res->debug_snapshots_before);
#endif
    HaArrayClear(res->hall_sets);
    HaArrayClear(res->competitors);
  }
  else
  {
    a = KheSolnArena(soln);
    HaMake(res, a);
#if USE_SNAPSHOT
    HaArrayInit(res->snapshot_free_list, a);
#endif
    HaArrayInit(res->supply_chunks, a);
    HaArrayInit(res->demand_chunks, a);
    HaArrayInit(res->supply_nodes, a);
    HaArrayInit(res->unmatched_demand_nodes, a);
    HaArrayInit(res->marks, a);
#if USE_SNAPSHOT
    HaArrayInit(res->debug_snapshots_before, a);
#endif
    HaArrayInit(res->hall_sets, a);
    HaArrayInit(res->competitors, a);
  }
  res->soln = soln;
  res->demand_node_count = 0;
  res->visit_num = 0;
  res->unmatched_lower_bound = 0;
  res->debug_mark_calls = 0;
  res->active = false;
  /* res->debug_snapshot_before = NULL; */
  /* res->debug_snapshot_after = NULL; */
  res->copy = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheMatchingImpl(KHE_MATCHING m)                                    */
/*                                                                           */
/*  Return the impl attribute of m.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
void *KheMatchingImpl(KHE_MATCHING m)
{
  return m->impl;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingSetImpl(KHE_MATCHING m, void *impl)                      */
/*                                                                           */
/*  Reset the impl pointer of m.                                             */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheMatchingSetImpl(KHE_MATCHING m, void *impl)
{
  m->impl = impl;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingDelete(KHE_MATCHING m)                                   */
/*                                                                           */
/*  Free m and all its chunks and nodes.                                     */
/*                                                                           */
/*  Implementation note.  Deletion requires a plan which guarantees that     */
/*  everything is deleted exactly once.  Here is the plan for matchings:     */
/*                                                                           */
/*    KHE_MATCHING_SUPPLY_CHUNK                                              */
/*    KHE_MATCHING_SUPPLY_NODE                                               */
/*                                                                           */
/*  As meets are deleted, they are moved onto the free list in the           */
/*  solution object, but they are not actually freed, and their supply       */
/*  chunks are not touched.  This function deletes all supply chunks, and    */
/*  each chunk deletes its supply nodes.                                     */
/*                                                                           */
/*    KHE_MATCHING_DEMAND_CHUNK                                              */
/*    KHE_MATCHING_DEMAND_NODE                                               */
/*                                                                           */
/*  As meets are deleted, these are deleted.  So there should be none left   */
/*  by the time KheMatchingDelete is called.                                 */
/*                                                                           */
/*    KHE_MATCHING                                                           */
/*    KHE_MATCHING_HALL_SET                                                  */
/*                                                                           */
/*  Deleted by KheMatchingDelete.                                            */
/*                                                                           */
/*****************************************************************************/
/* static void KheMatchingHallSetFree(KHE_MATCHING_HALL_SET hs); */

void KheMatchingDelete(KHE_MATCHING m)
{
  HnAssert(HaArrayCount(m->supply_chunks) == 0,
    "KheMatchingDelete internal error 1");
  /* ***
  while( HaArrayCount(m->supply_chunks) > 0 )
    KheMatchingSupplyChunkDelete(HaArrayLastAndDelete(m->supply_chunks));
  *** */
  HnAssert(HaArrayCount(m->demand_chunks) == 0,
    "KheMatchingDelete internal error 2");
  while( HaArrayCount(m->hall_sets) > 0 )
    KheMatchingHallSetDelete(HaArrayLastAndDelete(m->hall_sets), m->soln);
  KheSolnAddMatchingToFreeList(m->soln, m);
  /* remainder still to do */
  /* HaAre naDelete(m->arena); */
  /* ***
  HnAssert(HaArrayCount(m->unmatched_demand_nodes) == 0,
    "KheMatchingDelete internal error 2");
  MArrayFree(m->supply_nodes);
  MArrayFree(m->unmatched_demand_nodes);
  MArrayFree(m->marks);
  MFree(m);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING KheMatchingCopyPhase1(KHE_MATCHING m, HA_ARENA a)           */
/*                                                                           */
/*  Carry out Phase 1 of copying m (into a fresh arena).                     */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING KheMatchingCopyPhase1(KHE_MATCHING m, HA_ARENA a)
{
  KHE_MATCHING copy;  KHE_MATCHING_SUPPLY_CHUNK sc;  int i;
  KHE_MATCHING_DEMAND_CHUNK dc;  KHE_MATCHING_SUPPLY_NODE sn;  KHE_SOLN soln;
  KHE_MATCHING_DEMAND_NODE dn;  KHE_MATCHING_HALL_SET hs;  /* HA_ARENA a; */
  if( m->copy == NULL )
  {
    soln = KheSolnCopyPhase1(m->soln);
    /* a = KheSolnArena(soln); */
    HaMake(copy, a);
    m->copy = copy;
    copy->soln = soln;
    if( DEBUG2 )
      fprintf(stderr, "[ KheMatchingCopyPhase1(%p, soln %p) res %p, soln %p\n",
	(void *) m, (void *) m->soln, (void *) copy, (void *) copy->soln);
    HaArrayInit(copy->supply_chunks, a);
    HaArrayForEach(m->supply_chunks, sc, i)
      HaArrayAddLast(copy->supply_chunks,
	KheMatchingSupplyChunkCopyPhase1(sc, a));
    HaArrayInit(copy->demand_chunks, a);
    HaArrayForEach(m->demand_chunks, dc, i)
      HaArrayAddLast(copy->demand_chunks,
	KheMatchingDemandChunkCopyPhase1(dc, a));
    HaArrayInit(copy->supply_nodes, a);
    HaArrayForEach(m->supply_nodes, sn, i)
      HaArrayAddLast(copy->supply_nodes,
	KheMatchingSupplyNodeCopyPhase1(sn, a));
    copy->demand_node_count = m->demand_node_count;
    copy->visit_num = m->visit_num;
    HaArrayInit(copy->unmatched_demand_nodes, a);
    HaArrayForEach(m->unmatched_demand_nodes, dn, i)
      HaArrayAddLast(copy->unmatched_demand_nodes,
	KheMatchingDemandNodeCopyPhase1(dn, a));
    copy->unmatched_lower_bound = m->unmatched_lower_bound;
    HnAssert(!m->active, "KheMatchingCopyPhase1 internal error");
    copy->active = false;
    HaArrayInit(copy->marks, a);
    HaArrayAppend(copy->marks, m->marks, i);
    copy->debug_mark_calls = m->debug_mark_calls;
    HaArrayInit(copy->hall_sets, a);
    HaArrayForEach(m->hall_sets, hs, i)
      HaArrayAddLast(copy->hall_sets, KheMatchingHallSetCopyPhase1(hs, a));
    HaArrayInit(copy->competitors, a);
    HaArrayForEach(m->competitors, dn, i)
      HaArrayAddLast(copy->competitors, KheMatchingDemandNodeCopyPhase1(dn, a));
    /* copy->debug_snapshot_before = NULL; */
    /* copy->debug_snapshot_after = NULL; */
    copy->copy = NULL;
    if( DEBUG2 )
      fprintf(stderr, "] KheMatchingCopyPhase1 returning %p\n", (void *) copy);
  }
  return m->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingCopyPhase2(KHE_MATCHING m)                               */
/*                                                                           */
/*  Carry out Phase 2 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

void KheMatchingCopyPhase2(KHE_MATCHING m)
{
  KHE_MATCHING_SUPPLY_CHUNK sc;  int i;
  KHE_MATCHING_DEMAND_CHUNK dc;  KHE_MATCHING_SUPPLY_NODE sn;
  KHE_MATCHING_DEMAND_NODE dn;  KHE_MATCHING_HALL_SET hs;
  if( m->copy != NULL )
  {
    if( DEBUG2 )
      fprintf(stderr, "[ KheMatchingCopyPhase2(%p, soln %p)\n",
	(void *) m, (void *) m->soln);
    m->copy = NULL;
    KheSolnCopyPhase2(m->soln);
    HaArrayForEach(m->supply_chunks, sc, i)
      KheMatchingSupplyChunkCopyPhase2(sc);
    HaArrayForEach(m->demand_chunks, dc, i)
      KheMatchingDemandChunkCopyPhase2(dc);
    HaArrayForEach(m->supply_nodes, sn, i)
      KheMatchingSupplyNodeCopyPhase2(sn);
    HaArrayForEach(m->unmatched_demand_nodes, dn, i)
      KheMatchingDemandNodeCopyPhase2(dn);
    HaArrayForEach(m->hall_sets, hs, i)
      KheMatchingHallSetCopyPhase2(hs);
    HaArrayForEach(m->competitors, dn, i)
      KheMatchingDemandNodeCopyPhase2(dn);
    if( DEBUG2 )
      fprintf(stderr, "] KheMatchingCopyPhase2 returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMatchingSupplyNodeCount(KHE_MATCHING m)                           */
/*                                                                           */
/*  Return the number of supply nodes in m.                                  */
/*                                                                           */
/*****************************************************************************/

int KheMatchingSupplyNodeCount(KHE_MATCHING m)
{
  return HaArrayCount(m->supply_nodes);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMatchingDemandNodeCount(KHE_MATCHING m)                           */
/*                                                                           */
/*  Return the number of demand nodes in m.                                  */
/*                                                                           */
/*****************************************************************************/

int KheMatchingDemandNodeCount(KHE_MATCHING m)
{
  return m->demand_node_count;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMatchingSupplyChunkCount(KHE_MATCHING m)                          */
/*                                                                           */
/*  Return the number of supply chunks in m.                                 */
/*                                                                           */
/*****************************************************************************/

int KheMatchingSupplyChunkCount(KHE_MATCHING m)
{
  return HaArrayCount(m->supply_chunks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_SUPPLY_CHUNK KheMatchingSupplyChunk(KHE_MATCHING m, int i)  */
/*                                                                           */
/*  Return the i'th supply chunk of m.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_SUPPLY_CHUNK KheMatchingSupplyChunk(KHE_MATCHING m, int i)
{
  return HaArray(m->supply_chunks, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMatchingDemandChunkCount(KHE_MATCHING m)                          */
/*                                                                           */
/*  Return the number of demand chunks in m.                                 */
/*                                                                           */
/*****************************************************************************/

int KheMatchingDemandChunkCount(KHE_MATCHING m)
{
  return HaArrayCount(m->demand_chunks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_DEMAND_CHUNK KheMatchingDemandChunk(KHE_MATCHING m, int i)  */
/*                                                                           */
/*  Return the i'th demand chunk of m.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_DEMAND_CHUNK KheMatchingDemandChunk(KHE_MATCHING m, int i)
{
  return HaArray(m->demand_chunks, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMatchingIsActive(KHE_MATCHING m)                                 */
/*                                                                           */
/*  Return true if m is active.                                              */
/*                                                                           */
/*****************************************************************************/

bool KheMatchingIsActive(KHE_MATCHING m)
{
  return m->active;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMatchingUnmatchedDemandNodeCount(KHE_MATCHING m)                  */
/*                                                                           */
/*  Return the number of unmatched demand nodes in a maximum matching of m.  */
/*                                                                           */
/*****************************************************************************/

int KheMatchingUnmatchedDemandNodeCount(KHE_MATCHING m)
{
  HnAssert(!m->active, "KheMatchingUnmatchedDemandNodeCount reentry in %p",
    (void *) m->soln);
  m->active = true;
  KheMatchingMakeClean(m);
  m->active = false;
  return m->unmatched_lower_bound;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_DEMAND_NODE KheMatchingUnmatchedDemandNode(KHE_MATCHING m,  */
/*    int i)                                                                 */
/*                                                                           */
/*  Return the i'th unmatched demand node in a maximum matching of m.        */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_DEMAND_NODE KheMatchingUnmatchedDemandNode(KHE_MATCHING m, int i)
{
  HnAssert(!m->active, "KheMatchingUnmatchedDemandNode reentry in %p",
    (void *) m->soln);
  m->active = true;
  KheMatchingMakeClean(m);
  m->active = false;
  HnAssert(m->unmatched_lower_bound == HaArrayCount(m->unmatched_demand_nodes),
    "KheMatchingUnmatchedDemandNode internal error 1 (failed %d == %d)",
    m->unmatched_lower_bound, HaArrayCount(m->unmatched_demand_nodes));
  HnAssert(0 <= i && i < HaArrayCount(m->unmatched_demand_nodes),
    "KheMatchingUnmatchedDemandNode internal error 2 (failed 0 <= %d < %d)",
    i, HaArrayCount(m->unmatched_demand_nodes));
  return HaArray(m->unmatched_demand_nodes, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingLowerBoundsDebug(KHE_MATCHING m, char *message,          */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of lower bounds.                                             */
/*                                                                           */
/*****************************************************************************/

static void KheMatchingLowerBoundsDebug(KHE_MATCHING m, char *message,
  int verbosity, int indent, FILE *fp)
{
  int i, val;  KHE_MATCHING_DEMAND_NODE dn;
  KheMatchingUnmatchedDemandNodeCount(m);
  fprintf(stderr, "%*s%s %d with lower bounds ", indent, "", message,
    m->debug_mark_calls);
  HaArrayForEach(m->marks, val, i)
    fprintf(fp, "%s%d", i > 0 ? " " : "", val);
  fprintf(stderr, "\n");
  if( DEBUG3 && m->debug_mark_calls == DEBUG3_MARK_CALL )
  {
    /* print the unmatched demand nodes */
    fprintf(stderr, "%*s%d unmatched demand nodes:\n", indent + 2, "",
      HaArrayCount(m->unmatched_demand_nodes));
    HaArrayForEach(m->unmatched_demand_nodes, dn, i)
      KheMonitorDebug((KHE_MONITOR) dn, 2, indent + 2, stderr);
  }
  if( DEBUG4 && DEBUG4_MONITOR_INDEX < KheSolnMonitorCount(m->soln) )
  {
    KHE_MONITOR mon;  KHE_ORDINARY_DEMAND_MONITOR odm;  KHE_TASK task, t2;
    mon = KheSolnMonitor(m->soln, DEBUG4_MONITOR_INDEX);
    KheMonitorDebug(mon, 2, indent + 2, stderr);
    if( KheMonitorTag(mon) == KHE_ORDINARY_DEMAND_MONITOR_TAG )
    {
      odm = (KHE_ORDINARY_DEMAND_MONITOR) mon;
      task = KheOrdinaryDemandMonitorTask(odm);
      fprintf(stderr, "%*s    task %s", indent, "", KheTaskId(task));
      t2 = KheTaskAsst(task);
      while( t2 != NULL )
      {
	fprintf(stderr, " -> %s", KheTaskId(t2));
	t2 = KheTaskAsst(t2);
      }
      fprintf(stderr, "\n");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingMarkBegin(KHE_MATCHING m)                                */
/*                                                                           */
/*  Begin a bracketed sequence of operations on m.                           */
/*                                                                           */
/*****************************************************************************/

void KheMatchingMarkBegin(KHE_MATCHING m)
{
  HaArrayAddLast(m->marks, KheMatchingUnmatchedDemandNodeCount(m));
  m->debug_mark_calls++;
  if( DEBUG3 || DEBUG4 || DEBUG5 )
    KheMatchingLowerBoundsDebug(m, "[ KheMatchingMarkBegin", 1, 0, stderr);
#if USE_SNAPSHOT
  if( DEBUG6(m) )
  {
    KHE_SNAPSHOT snapshot_before;
    fprintf(stderr, "  KheMatchingMarkBegin taking snapshot before\n");
    snapshot_before = KheSnapshotMake(m);
    KheSnapshotBuild(snapshot_before, m);
    HaArrayAddLast(m->debug_snapshots_before, snapshot_before);
  }
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingMarkCheck(KHE_MATCHING m)                                */
/*                                                                           */
/*  Check the matching lower bound.                                          */
/*                                                                           */
/*****************************************************************************/

void KheMatchingMarkCheck(KHE_MATCHING m)
{
  int lower_bound;
  if( HaArrayCount(m->marks) > 0 )
  {
    lower_bound = HaArrayLast(m->marks);
    m->unmatched_lower_bound = lower_bound;
    KheMatchingMakeClean(m);
    HnAssert(m->unmatched_lower_bound == lower_bound,
      "KheMatchingMarkCheck: lower bound inconsistency (%d vs %d)",
      m->unmatched_lower_bound, lower_bound);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingMarkEnd(KHE_MATCHING m, bool undo)                       */
/*                                                                           */
/*  End a bracketed sequence of operations on m.  If undo is true, assume    */
/*  that this returns m to the same state it was in when the corresponding   */
/*  call to KheMatchingMarkBegin was called.                                 */
/*                                                                           */
/*****************************************************************************/

void KheMatchingMarkEnd(KHE_MATCHING m, bool undo)
{
  int lower_bound;
#if USE_SNAPSHOT
  KHE_SNAPSHOT snapshot_before, snapshot_after;
#endif
  if( DEBUG3 || DEBUG4 || DEBUG5 )
    KheMatchingLowerBoundsDebug(m, "] KheMatchingMarkEnd", 1, 0, stderr);
  lower_bound = HaArrayLastAndDelete(m->marks);
#if USE_SNAPSHOT
  if( DEBUG6(m) )
    snapshot_before = HaArrayLastAndDelete(m->debug_snapshots_before);
  else
    snapshot_before = NULL;
#endif
  if( undo )
  {
    HnAssert(!m->active, "KheMatchingMarkEnd reentry in %p", (void *) m->soln);
    m->active = true;
    m->unmatched_lower_bound = lower_bound;
    KheMatchingCheckLowerBound(m);
    KheMatchingMakeClean(m);
    HnAssert(m->unmatched_lower_bound == lower_bound,
      "KheMatchingMarkEnd: lower bound inconsistency (%d vs %d)",
      m->unmatched_lower_bound, lower_bound);
#if USE_SNAPSHOT
    if( DEBUG6(m) )
    {
      fprintf(stderr, "  KheMatchingMarkEnd taking snapshot after\n");
      snapshot_after = KheSnapshotMake(m);
      KheSnapshotBuild(snapshot_after, m);
      KheSnapshotCheckEqual(snapshot_before, snapshot_after);
      HaArrayAddLast(m->snapshot_free_list, snapshot_after);
    }
#endif
    m->active = false;
  }
#if USE_SNAPSHOT
  if( DEBUG6(m) )
    HaArrayAddLast(m->snapshot_free_list, snapshot_before);
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingEnsureHallSetsUpToDate(KHE_MATCHING m)                   */
/*                                                                           */
/*  Ensure that m's Hall sets are up to date.                                */
/*                                                                           */
/*  Implementation note.  This function is based on this global invariant:   */
/*                                                                           */
/*    (1) At all times, m->hall_sets is a well-defined array, but it may     */
/*        be not up to date (it may refer to non-existent nodes, etc.).      */
/*                                                                           */
/*    (2) If m is not up to date, then m->hall_sets is not up to date.       */
/*                                                                           */
/*    (3) If m is up to date, then                                           */
/*        (a) If m->hall_sets is empty, it may be not up to date;            */
/*        (b) If m->hall_sets is non-empty, then it is up to date.           */
/*                                                                           */
/*  First, it calls KheMatchingMakeClean to make sure that m is up to date,  */
/*  so (3) applies.  Then, if m->hall_sets is empty, it is up to date if     */
/*  and only if there are no unmatched demand nodes.  So BuildHallSets is    */
/*  called if m->hall_sets is empty and there are unmatched demand nodes.    */
/*                                                                           */
/*****************************************************************************/
static void KheMatchingBuildHallSets(KHE_MATCHING m);

static void KheMatchingEnsureHallSetsUpToDate(KHE_MATCHING m)
{
  HnAssert(!m->active, "KheMatchingMarkEnd reentry in %p", (void *) m->soln);
  m->active = true;
  KheMatchingMakeClean(m);
  if( HaArrayCount(m->hall_sets) == 0 &&
      HaArrayCount(m->unmatched_demand_nodes) > 0 )
    KheMatchingBuildHallSets(m);
  m->active = false;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMatchingHallSetCount(KHE_MATCHING m)                              */
/*                                                                           */
/*  Return the number of Hall sets in m, making sure they are up to date.    */
/*                                                                           */
/*****************************************************************************/

int KheMatchingHallSetCount(KHE_MATCHING m)
{
  KheMatchingEnsureHallSetsUpToDate(m);
  return HaArrayCount(m->hall_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_HALL_SET KheMatchingHallSet(KHE_MATCHING m, int i)          */
/*                                                                           */
/*  Return the i'th Hall set of m, making sure the Hall sets are up to date. */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_HALL_SET KheMatchingHallSet(KHE_MATCHING m, int i)
{
  KheMatchingEnsureHallSetsUpToDate(m);
  return HaArray(m->hall_sets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "competitors"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/* void KheMatchingSetCompetitors(KHE_MATCHING m,KHE_MATCHING_DEMAND_NODE dn)*/
/*                                                                           */
/*  Set the competitors.                                                     */
/*                                                                           */
/*****************************************************************************/

void KheMatchingSetCompetitors(KHE_MATCHING m, KHE_MATCHING_DEMAND_NODE dn)
{
  KHE_MATCHING_SUPPLY_NODE sn;  KHE_MATCHING_DEMAND_CHUNK dc;
  int i, j, k, base;  ARRAY_KHE_MATCHING_SUPPLY_NODE supply_nodes;
  KheMatchingUnmatchedDemandNodeCount(m);
  HnAssert(dn->demand_asst == NULL,
    "KheMatchingSetCompetitors: demand node is not unmatched");
  m->visit_num++;
  HaArrayClear(m->competitors);
  HaArrayAddLast(m->competitors, dn);
  HaArrayForEach(m->competitors, dn, i)
  {
    dc = dn->demand_chunk;
    supply_nodes = dc->matching->supply_nodes;
    for( j = 0;  j < KheSetCount(dc->domain);  j++ )
    {
      base = dc->base + dc->increment * KheSetGet(dc->domain, j);
      for( k = 0;  k < SSetCount(dn->domain);  k++ )
      {
	sn = HaArray(supply_nodes, base + KheSetGet(dn->domain, k));
	if( sn->visit_num < m->visit_num )
	{
	  sn->visit_num = m->visit_num;
	  HnAssert(sn->supply_asst != NULL,
	    "KheMatchingSetCompetitors internal error");
	  HaArrayAddLast(m->competitors, sn->supply_asst);
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMatchingCompetitorCount(KHE_MATCHING m)                           */
/*                                                                           */
/*  Return the number of competitors.                                        */
/*                                                                           */
/*****************************************************************************/

int KheMatchingCompetitorCount(KHE_MATCHING m)
{
  return HaArrayCount(m->competitors);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_DEMAND_NODE KheMatchingCompetitor(KHE_MATCHING m, int i)    */
/*                                                                           */
/*  Return the i'th competitor.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_DEMAND_NODE KheMatchingCompetitor(KHE_MATCHING m, int i)
{
  return HaArray(m->competitors, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingDebugBegin(KHE_MATCHING m,                               */
/*    KHE_MATCHING_SUPPLY_NODE_DEBUG_FN supply_node_debug_fn,                */
/*    KHE_MATCHING_DEMAND_NODE_DEBUG_FN demand_node_debug_fn,                */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Begin a matching debug.                                                  */
/*                                                                           */
/*****************************************************************************/

void KheMatchingDebugBegin(KHE_MATCHING m,
  KHE_MATCHING_SUPPLY_NODE_DEBUG_FN supply_node_debug_fn,
  KHE_MATCHING_DEMAND_NODE_DEBUG_FN demand_node_debug_fn,
  int verbosity, int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ Matching (%slower bound %d, unmatched %d)",
      verbosity == 2 ? "unmatched demand nodes only, " : "",
      m->unmatched_lower_bound, HaArrayCount(m->unmatched_demand_nodes));
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingDebugDemandChunk(KHE_MATCHING_DEMAND_CHUNK dc,           */
/*    KHE_MATCHING_SUPPLY_NODE_DEBUG_FN supply_node_debug_fn,                */
/*    KHE_MATCHING_DEMAND_NODE_DEBUG_FN demand_node_debug_fn,                */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Continue a matching debug by debugging one demand chunk.                 */
/*                                                                           */
/*****************************************************************************/

void KheMatchingDebugDemandChunk(KHE_MATCHING_DEMAND_CHUNK dc,
  KHE_MATCHING_SUPPLY_NODE_DEBUG_FN supply_node_debug_fn,
  KHE_MATCHING_DEMAND_NODE_DEBUG_FN demand_node_debug_fn,
  int verbosity, int indent, FILE *fp)
{
  KHE_MATCHING_DEMAND_NODE dn;  int j, k;  short d;
  if( verbosity >= 2 && indent >= 0 )
  {
    if( verbosity >= 4 )
    {
      fprintf(fp, "%*s  chunk: %d + %d*[", indent, "", dc->base, dc->increment);
      SSetForEach(dc->domain, &d, &j)
      {
	if( j > 0 )
	  fprintf(fp, ", ");
	fprintf(fp, "%d", d);
      }
      fprintf(fp, "]\n");
    }
    HaArrayForEach(dc->demand_nodes, dn, j)
      if( verbosity >= 3 || dn->demand_asst == NULL )
      {
	fprintf(fp, "%*s  ", indent, "");
	demand_node_debug_fn(dn, verbosity, -1, fp);
	if( dn->demand_asst != NULL )
	{
	  fprintf(fp, " -> ");
	  if( verbosity >= 4 )
	    fprintf(stderr, "[%d] ", dn->demand_asst->index);
	  supply_node_debug_fn(dn->demand_asst, verbosity, -1, fp);
	}
	fprintf(fp, "\n");
	if( verbosity >= 4 )
	{
	  fprintf(fp, "%*s    node: [", indent, "");
	  KheSetForEach(dn->domain, d, k)
	  {
	    if( k > 0 )
	      fprintf(fp, ", ");
	    fprintf(fp, "%d", d);
	  }
	  fprintf(fp, "]\n");
	}
      }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingDebugEnd(KHE_MATCHING m,                                 */
/*    KHE_MATCHING_SUPPLY_NODE_DEBUG_FN supply_node_debug_fn,                */
/*    KHE_MATCHING_DEMAND_NODE_DEBUG_FN demand_node_debug_fn,                */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  End a matching debug.                                                    */
/*                                                                           */
/*****************************************************************************/

void KheMatchingDebugEnd(KHE_MATCHING m,
  KHE_MATCHING_SUPPLY_NODE_DEBUG_FN supply_node_debug_fn,
  KHE_MATCHING_DEMAND_NODE_DEBUG_FN demand_node_debug_fn,
  int verbosity, int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    else
      fprintf(fp, " ");
    fprintf(fp, "]");
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingDebug(KHE_MATCHING m,                                    */
/*    KHE_MATCHING_SUPPLY_NODE_DEBUG_FN supply_node_debug_fn,                */
/*    KHE_MATCHING_DEMAND_NODE_DEBUG_FN demand_node_debug_fn,                */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheMatchingDebug(KHE_MATCHING m,
  KHE_MATCHING_SUPPLY_NODE_DEBUG_FN supply_node_debug_fn,
  KHE_MATCHING_DEMAND_NODE_DEBUG_FN demand_node_debug_fn,
  int verbosity, int indent, FILE *fp)
{
  KHE_MATCHING_DEMAND_CHUNK dc;  int i;
  KheMatchingDebugBegin(m, supply_node_debug_fn,
    demand_node_debug_fn, verbosity, indent, fp);
  HaArrayForEach(m->demand_chunks, dc, i)
    KheMatchingDebugDemandChunk(dc, supply_node_debug_fn,
      demand_node_debug_fn, verbosity, indent, fp);
  KheMatchingDebugEnd(m, supply_node_debug_fn,
    demand_node_debug_fn, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MATCHING_HALL_SET"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_HALL_SET KheMatchingHallSetMake(void)                       */
/*                                                                           */
/*  Return a new, empty Hall set, not yet loaded into m.                     */
/*                                                                           */
/*  When there is no parent set, the easiest arrangement is to let the       */
/*  parent_set field point to self rather than be NULL.                      */
/*                                                                           */
/*****************************************************************************/

static KHE_MATCHING_HALL_SET KheMatchingHallSetMake(KHE_MATCHING m)
{
  KHE_MATCHING_HALL_SET res;  HA_ARENA a;
  res = KheSolnGetMatchingHallSetFromFreeList(m->soln);
  if( res != NULL )
  {
    HaArrayClear(res->supply_nodes);
    HaArrayClear(res->demand_nodes);
  }
  else
  {
    a = KheSolnArena(m->soln);
    HaMake(res, a);
    HaArrayInit(res->supply_nodes, a);
    HaArrayInit(res->demand_nodes, a);
  }
  res->parent_hall_set = res;
  res->copy = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingHallSetDelete(KHE_MATCHING_HALL_SET hs, KHE_SOLN soln)   */
/*                                                                           */
/*  Add hs to soln's free list.                                              */
/*                                                                           */
/*****************************************************************************/

static void KheMatchingHallSetDelete(KHE_MATCHING_HALL_SET hs, KHE_SOLN soln)
{
  KheSolnAddMatchingHallSetToFreeList(soln, hs);
  /* ***
  MArrayFree(hs->supply_nodes);
  MArrayFree(hs->demand_nodes);
  MFree(hs);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_HALL_SET KheMatchingHallSetRoot(KHE_MATCHING_HALL_SET hs)   */
/*                                                                           */
/*  Return the root hall set of hs.                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_MATCHING_HALL_SET KheMatchingHallSetRoot(KHE_MATCHING_HALL_SET hs)
{
  HnAssert(hs != NULL, "HallSetRoot internal error");
  while( hs->parent_hall_set != hs )
    hs = hs->parent_hall_set;
  return hs;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingHallSetCompressPath(KHE_MATCHING_HALL_SET *hall_set)     */
/*                                                                           */
/*  Compress the path from *hall_set to its root Hall set, including         */
/*  changing *hall_set to its own root.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheMatchingHallSetCompressPath(KHE_MATCHING_HALL_SET *hall_set)
{
  KHE_MATCHING_HALL_SET hs, hs_parent, root;
  root = KheMatchingHallSetRoot(*hall_set);
  hs = *hall_set;
  *hall_set = root;
  while( hs->parent_hall_set != root )
  {
    hs_parent = hs->parent_hall_set;
    hs->parent_hall_set = root;
    hs = hs_parent;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingHallSetUnion(KHE_MATCHING_HALL_SET root,                 */
/*    KHE_MATCHING_HALL_SET *hs)                                             */
/*                                                                           */
/*  Union *hs into root.                                                     */
/*                                                                           */
/*****************************************************************************/

static void KheMatchingHallSetUnion(KHE_MATCHING_HALL_SET root,
  KHE_MATCHING_HALL_SET *hs)
{
  KHE_MATCHING_HALL_SET other_root;
  other_root = KheMatchingHallSetRoot(*hs);
  other_root->parent_hall_set = root;
  KheMatchingHallSetCompressPath(hs);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingHallSetAssign(KHE_MATCHING m,                            */
/*    KHE_MATCHING_DEMAND_NODE dn, KHE_MATCHING_HALL_SET root)               */
/*                                                                           */
/*  Explore the matching graph from demand_node, as an assignment would      */
/*  do, and add all the nodes encountered to root, either directly or by     */
/*  changing the roots of their own Hall sets to point to this one.          */
/*                                                                           */
/*  Since this traversal starts at an unmatched demand node at a time when   */
/*  the matching is maximal, it can never encounter an unassigned supply     */
/*  node.  This is checked.                                                  */
/*                                                                           */
/*  Instead of visit_num, this code uses the presence of a non-NULL Hall     */
/*  set in a demand or supply node to determine whether the node has been    */
/*  visited.  If there is a Hall set, the search can terminate, although     */
/*  the Hall set does need to be unioned with root.                          */
/*                                                                           */
/*****************************************************************************/

static void KheMatchingHallSetAssign(KHE_MATCHING m,
  KHE_MATCHING_DEMAND_NODE dn, KHE_MATCHING_HALL_SET root)
{
  KHE_MATCHING_SUPPLY_NODE sn;  int i, j, base, item_i, item_j;
  KHE_MATCHING_DEMAND_CHUNK dc;  ARRAY_KHE_MATCHING_SUPPLY_NODE supply_nodes;
  if( dn->hall_set != NULL )
  {
    /* dn is already visited, so just make sure the Hall sets are unioned */
    if( dn->hall_set != root )
      KheMatchingHallSetUnion(root, &dn->hall_set);
  }
  else
  {
    /* add dn to root and recurse to all supply nodes */
    dn->hall_set = root;
    supply_nodes = dn->demand_chunk->matching->supply_nodes;
    dc = dn->demand_chunk;
    SSetForEach(dc->domain, &item_i, &i)
    {
      base = dc->base + dc->increment * item_i;
      KheSetForEach(dn->domain, item_j, j)
      {
	sn = HaArray(supply_nodes, base + item_j);
	HnAssert(sn->supply_asst != NULL, "HallSetAssign internal error");
	if( sn->hall_set != NULL )
	{
	  /* sn already visited; make sure Hall sets are unioned */
	  if( sn->hall_set != root )
	    KheMatchingHallSetUnion(root, &sn->hall_set);
	}
	else
	{
	  /* add sn to root and recurse to assigned demand node */
	  sn->hall_set = root;
	  KheMatchingHallSetAssign(m, sn->supply_asst, root);
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingBuildHallSets(KHE_MATCHING m)                            */
/*                                                                           */
/*  Build the Hall sets for m, first bringing m up to date.                  */
/*                                                                           */
/*****************************************************************************/

static void KheMatchingBuildHallSets(KHE_MATCHING m)
{
  KHE_MATCHING_DEMAND_NODE dn;  KHE_MATCHING_SUPPLY_NODE sn;
  KHE_MATCHING_HALL_SET hs;  KHE_MATCHING_DEMAND_CHUNK dc;  int i, j;

  /* bring m up to date and remove any traces of previous Hall sets */
  KheMatchingMakeClean(m);
  HaArrayClear(m->hall_sets);
  HaArrayForEach(m->supply_nodes, sn, i)
    sn->hall_set = NULL;
  HaArrayForEach(m->demand_chunks, dc, i)
    HaArrayForEach(dc->demand_nodes, dn, j)
      dn->hall_set = NULL;

  /* assign nodes to Hall sets (including merging overlapping sets) */
  HaArrayForEach(m->unmatched_demand_nodes, dn, i)
    if( dn->hall_set == NULL )
    {
      HnAssert(dn->demand_asst == NULL, "BuildHallSets internal error 1");
      hs = KheMatchingHallSetMake(m);
      HaArrayAddLast(m->hall_sets, hs);
      KheMatchingHallSetAssign(m, dn, hs);
    }

  /* compress the path at each node, and add each node to its root set */
  HaArrayForEach(m->supply_nodes, sn, i)
    if( sn->hall_set != NULL )
    {
      KheMatchingHallSetCompressPath(&sn->hall_set);
      HaArrayAddLast(sn->hall_set->supply_nodes, sn);
    }
  HaArrayForEach(m->demand_chunks, dc, i)
    HaArrayForEach(dc->demand_nodes, dn, j)
    if( dn->hall_set != NULL )
    {
      KheMatchingHallSetCompressPath(&dn->hall_set);
      HaArrayAddLast(dn->hall_set->demand_nodes, dn);
    }

  /* remove and free the defunct Hall sets */
  HaArrayForEach(m->hall_sets, hs, i)
    if( hs->parent_hall_set == hs )
    {
      HnAssert(HaArrayCount(hs->demand_nodes) > HaArrayCount(hs->supply_nodes),
	"KheMatchingBuildHallSets internal error 2");
    }
    else
    {
      HnAssert(HaArrayCount(hs->demand_nodes) == 0,
	"KheMatchingBuildHallSets internal error 3");
      HnAssert(HaArrayCount(hs->supply_nodes) == 0,
	"KheMatchingBuildHallSets internal error 4");
      HaArrayDeleteAndPlug(m->hall_sets, i);
      KheMatchingHallSetDelete(hs, m->soln);
      /* HaArrayAddLast(m->free_hall_sets, hs); */
      /* KheMatchingHallSetFree(hs); */
      i--;
    }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_HALL_SET KheMatchingHallSetCopyPhase1(                      */
/*    KHE_MATCHING_HALL_SET hs, HA_ARENA a)                                  */
/*                                                                           */
/*  Carry out Phase 1 of copying hs.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_HALL_SET KheMatchingHallSetCopyPhase1(KHE_MATCHING_HALL_SET hs,
  HA_ARENA a)
{
  KHE_MATCHING_HALL_SET copy;  KHE_MATCHING_SUPPLY_NODE sn;
  KHE_MATCHING_DEMAND_NODE dn;  int i;
  if( hs->copy == NULL )
  {
    HaMake(copy, a);
    hs->copy = copy;
    copy->parent_hall_set = (hs->parent_hall_set == NULL ? NULL :
      KheMatchingHallSetCopyPhase1(hs->parent_hall_set, a));
    HaArrayInit(copy->supply_nodes, a);
    HaArrayForEach(hs->supply_nodes, sn, i)
      HaArrayAddLast(copy->supply_nodes,
	KheMatchingSupplyNodeCopyPhase1(sn, a));
    HaArrayInit(copy->demand_nodes, a);
    HaArrayForEach(hs->demand_nodes, dn, i)
      HaArrayAddLast(copy->demand_nodes,
	KheMatchingDemandNodeCopyPhase1(dn, a));
    copy->copy = NULL;
  }
  return hs->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingHallSetCopyPhase2(KHE_MATCHING_HALL_SET hs)              */
/*                                                                           */
/*  Carry out Phase 2 of copying hs.                                         */
/*                                                                           */
/*****************************************************************************/

void KheMatchingHallSetCopyPhase2(KHE_MATCHING_HALL_SET hs)
{
  KHE_MATCHING_SUPPLY_NODE sn;  KHE_MATCHING_DEMAND_NODE dn;  int i;
  if( hs->copy != NULL )
  {
    hs->copy = NULL;
    if( hs->parent_hall_set != NULL )
      KheMatchingHallSetCopyPhase2(hs->parent_hall_set);
    HaArrayForEach(hs->supply_nodes, sn, i)
      KheMatchingSupplyNodeCopyPhase2(sn);
    HaArrayForEach(hs->demand_nodes, dn, i)
      KheMatchingDemandNodeCopyPhase2(dn);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMatchingHallSetSupplyNodeCount(KHE_MATCHING_HALL_SET hs)          */
/*                                                                           */
/*  Return the number of supply nodes in hs.                                 */
/*                                                                           */
/*****************************************************************************/

int KheMatchingHallSetSupplyNodeCount(KHE_MATCHING_HALL_SET hs)
{
  return HaArrayCount(hs->supply_nodes);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_SUPPLY_NODE KheMatchingHallSetSupplyNode(                   */
/*    KHE_MATCHING_HALL_SET hs, int i)                                       */
/*                                                                           */
/*  Return the i'th supply node of hs.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_SUPPLY_NODE KheMatchingHallSetSupplyNode(
  KHE_MATCHING_HALL_SET hs, int i)
{
  return HaArray(hs->supply_nodes, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMatchingHallSetDemandNodeCount(KHE_MATCHING_HALL_SET hs)          */
/*                                                                           */
/*  Return the number of demand nodes in hs.                                 */
/*                                                                           */
/*****************************************************************************/

int KheMatchingHallSetDemandNodeCount(KHE_MATCHING_HALL_SET hs)
{
  return HaArrayCount(hs->demand_nodes);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_DEMAND_NODE KheMatchingHallSetDemandNode(                   */
/*    KHE_MATCHING_HALL_SET hs, int i)                                       */
/*                                                                           */
/*  Return the i'th demand node of hs.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_DEMAND_NODE KheMatchingHallSetDemandNode(
  KHE_MATCHING_HALL_SET hs, int i)
{
  return HaArray(hs->demand_nodes, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingHallSetDebug(KHE_MATCHING_HALL_SET hs,                   */
/*    KHE_MATCHING_SUPPLY_NODE_DEBUG_FN supply_node_debug_fn,                */
/*    KHE_MATCHING_DEMAND_NODE_DEBUG_FN demand_node_debug_fn,                */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of hs onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

void KheMatchingHallSetDebug(KHE_MATCHING_HALL_SET hs,
  KHE_MATCHING_SUPPLY_NODE_DEBUG_FN supply_node_debug_fn,
  KHE_MATCHING_DEMAND_NODE_DEBUG_FN demand_node_debug_fn,
  int verbosity, int indent, FILE *fp)
{
  KHE_MATCHING_DEMAND_NODE dn;  KHE_MATCHING_SUPPLY_NODE sn;  int i;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ Hall set (demand %d, supply %d)\n", indent, "",
      HaArrayCount(hs->demand_nodes), HaArrayCount(hs->supply_nodes));
    HnAssert(HaArrayCount(hs->demand_nodes) > HaArrayCount(hs->supply_nodes),
      "KheMatchingHallSetDebug internal error");
    HaArrayForEach(hs->demand_nodes, dn, i)
    {
      fprintf(fp, "%*s  ", indent, "");
      demand_node_debug_fn(dn, verbosity, -1, fp);
      if( i < HaArrayCount(hs->supply_nodes) )
      {
	fprintf(fp, "  |  ");
	sn = HaArray(hs->supply_nodes, i);
	supply_node_debug_fn(sn, verbosity, -1, fp);
      }
      fprintf(fp, "\n");
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}

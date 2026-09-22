
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
/*  FILE:         khe_soln.c                                                 */
/*  DESCRIPTION:  A solution                                                 */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0
#define DEBUG9 0
#define DEBUG10 0
#define DEBUG11 0
#define DEBUG12 0
#define DEBUG13 0
#define DEBUG14 0
#define DEBUG15 0
#define DEBUG16 0
#define DEBUG17 0
#define DEBUG18 0
#define DEBUG19 0

#define KHE_PLACEHOLDER -1

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN - a solution                                                    */
/*                                                                           */
/*  A solution is a placeholder solution when inner_arena == NULL.  It is    */
/*  an invalid solution when invalid_error != NULL.                          */
/*                                                                           */
/*****************************************************************************/

struct khe_soln_rec {

  /* group monitor attributes */
  INHERIT_GROUP_MONITOR

  /* free lists */
  ARRAY_KHE_MEET			free_meets;
  ARRAY_KHE_MEET_BOUND			free_meet_bounds;
  ARRAY_KHE_TASK			free_tasks;
  ARRAY_KHE_TASK_BOUND			free_task_bounds;
  ARRAY_KHE_MARK			free_marks;
  ARRAY_KHE_PATH			free_paths;
  ARRAY_KHE_NODE			free_nodes;
  ARRAY_KHE_LAYER			free_layers;
  ARRAY_KHE_ZONE			free_zones;
  /* ARRAY_KHE_TASKING			free_taskings; */
  ARRAY_KHE_TASK_SET			free_task_sets;
  ARRAY_KHE_MEET_SET			free_meet_sets;
  /* ARRAY_KHE_FRAME_MAKE		free_frame_makes; */
  ARRAY_KHE_MONITOR_LINK		free_monitor_links;
  ARRAY_KHE_GROUP_MONITOR		free_group_monitors;
  ARRAY_KHE_TRACE			free_traces;
  /* ARRAY_KHE_WORKLOAD_REQUIREMENT	free_workload_requirements; */
  ARRAY_KHE_MATCHING			free_matchings;
  ARRAY_KHE_MATCHING_SUPPLY_CHUNK	free_matching_supply_chunks;
  ARRAY_KHE_MATCHING_SUPPLY_NODE	free_matching_supply_nodes;
  ARRAY_KHE_MATCHING_DEMAND_CHUNK	free_matching_demand_chunks;
  ARRAY_KHE_ORDINARY_DEMAND_MONITOR	free_ordinary_demand_monitors;
  ARRAY_KHE_WORKLOAD_DEMAND_MONITOR	free_workload_demand_monitors;
  ARRAY_KHE_MATCHING_HALL_SET		free_matching_hall_sets;

  /* top-level attributes */
  HA_ARENA			outer_arena;		/* holds this obj    */
  HA_ARENA			inner_arena;		/* holds descendants */
  HA_ARENA_SET			arena_set;		/* optional arenas   */
  ARRAY_KHE_SOLN_GROUP		soln_groups;		/* soln groups       */
  KHE_INSTANCE			instance;		/* instance solved   */
  char				*description;		/* description       */
  float				running_time;		/* running_time      */
  int				diversifier;		/* diversifier       */
  int				global_visit_num;	/* global visit num  */

  /* solution type and related fields */
  KHE_SOLN_TYPE			type;
  KHE_SOLN_WRITE_ONLY		write_only_soln;
  KML_ERROR			invalid_error;		/* when invalid      */

  /* soln time groups */
  bool				time_sset_building;	/* when building tl  */
  SSET				time_sset;		/* time lset         */
  SSET_TABLE			time_sset_table;	/* trie of lsets     */

  /* soln resource groups */
  KHE_RESOURCE_TYPE		resource_set_rt;	/* when building rl  */
  KHE_SET			resource_set;		/* resource lset     */
  KHE_SET_TABLE			resource_set_table;	/* table of lsets    */

  /* soln event groups */
  KHE_EVENT_GROUP		curr_event_group;	/* temp variable     */

  /* inner attributes */
  ARRAY_KHE_EVENT_IN_SOLN	events_in_soln;		/* for each event    */
  ARRAY_KHE_RESOURCE_IN_SOLN	resources_in_soln;	/* for each resource */
  KHE_AVAIL_SOLVER		avail_solver;		/* avail solver      */
  ARRAY_KHE_MEET		meets;			/* all meets         */
  ARRAY_KHE_TASK		tasks;			/* all tasks         */
  KHE_PATH			main_path;		/* the main path     */
  ARRAY_KHE_MARK		marks;			/* all marks         */
  ARRAY_KHE_NODE		nodes;			/* all nodes         */
  /* ARRAY_KHE_TASKING		taskings; */		/* all taskings      */
  ARRAY_KHE_MONITOR		monitors;		/* all monitors      */

  /* cycle meets and tasks */
  ARRAY_KHE_MEET		time_to_cycle_meet;	/* maps time to c.m. */
  HA_ARRAY_INT			time_to_cycle_offset;	/* maps time to c.o. */
  ARRAY_KHE_TIME_GROUP		packing_time_groups;	/* packing time grps */

  /* matchings and evenness */
  KHE_MATCHING_TYPE		matching_type;		/* matching type     */
  KHE_COST			matching_weight;	/* weight of matching*/
  KHE_MATCHING			matching;		/* the matching      */
  KHE_SET			matching_zero_domain;	/* domain { 0 }      */
  SSET				matching_zero_time_domain;/* domain { 0 }    */
  KHE_EVENNESS_HANDLER		evenness_handler;	/* evenness handler  */
  KHE_REDUNDANCY_HANDLER	redundancy_handler;	/* redundancy handler*/

  /* copy */
  KHE_SOLN			copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "top-level operations - creation, deletion, and copy"          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAddInitialCycleMeet(KHE_SOLN soln)                           */
/*                                                                           */
/*  Add the initial cycle meet to soln, that is, if there is at least one    */
/*  time in the instance.                                                    */
/*                                                                           */
/*****************************************************************************/

static void KheSolnAddInitialCycleMeet(KHE_SOLN soln)
{
  KHE_INSTANCE ins;  KHE_TIME t;  KHE_MEET meet;  int i;
  ins = KheSolnInstance(soln);
  if( KheInstanceTimeCount(ins) > 0 )
  {
    /* add the meet, setting various fields appropriately for a cycle meet */
    meet = KheMeetMake(soln, KheInstanceTimeCount(ins), NULL);
    t = KheInstanceTime(ins, 0);
    KheMeetSetAssignedTimeIndexAndDomain(meet, t);
    KheMeetAssignFix(meet);

    /* initialize time_to_cycle_meet and time_to_cycle_offset */
    for( i = 0;  i < KheMeetDuration(meet);  i++ )
    {
      HaArrayAddLast(soln->time_to_cycle_meet, meet);
      HaArrayAddLast(soln->time_to_cycle_offset, i);
    }
    KheMeetCheck(meet);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAddCycleTasks(KHE_SOLN soln)                                 */
/*                                                                           */
/*  Add the cycle tasks to the soln.                                         */
/*                                                                           */
/*****************************************************************************/

static void KheSolnAddCycleTasks(KHE_SOLN soln)
{
  int i;  KHE_RESOURCE r;
  for( i = 0;  i < KheInstanceResourceCount(soln->instance);  i++ )
  {
    r = KheInstanceResource(soln->instance, i);
    KheCycleTaskMake(soln, r);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachConstraintMonitors(KHE_SOLN soln)               */
/*                                                                           */
/*  Make and attach the constraint monitors of soln.                         */
/*                                                                           */
/*****************************************************************************/

static void KheSolnMakeAndAttachConstraintMonitors(KHE_SOLN soln)
{
  int i;  KHE_INSTANCE ins;  KHE_CONSTRAINT c;
  ins = KheSolnInstance(soln);
  if( DEBUG12 )
    fprintf(stderr, "[ KheSolnMakeAndAttachConstraintMonitors (%d cons.)\n",
      KheInstanceConstraintCount(ins));
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( DEBUG12 )
      KheConstraintDebug(c, 2, 2, stderr);
    KheConstraintMakeAndAttachMonitors(c, soln);
  }
  if( DEBUG12 )
    fprintf(stderr, "] KheSolnMakeAndAttachConstraintMonitors returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheSolnMake(KHE_INSTANCE ins)                                   */
/*                                                                           */
/*  Make and return a new soln of instance, with no meets, no nodes, and     */
/*  no layers.                                                               */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheSolnMake(KHE_INSTANCE ins, HA_ARENA_SET as)
{
  HA_ARENA outer_a, inner_a;  KHE_SOLN res;  int i;  KHE_RESOURCE r;
  KHE_EVENT_IN_SOLN es;  KHE_RESOURCE_IN_SOLN rs;  KHE_EVENT e;

  /* boilerplate */
  if( DEBUG1 || DEBUG7 )
    fprintf(stderr, "[ KheSolnMake(%s)\n",
      KheInstanceId(ins) != NULL ? KheInstanceId(ins) : "-");
  HnAssert(ins != NULL, "KheSolnMake:  ins is NULL");
  HnAssert(as != NULL, "KheSolnMake:  as is NULL");
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_ALL,
    "KheSolnMake called before KheInstanceMakeEnd");
  outer_a = HaArenaMake(as);
  inner_a = HaArenaMake(as);
  HaMake(res, outer_a);
  if( DEBUG15 )
    fprintf(stderr, "  %p = KheSolnMake(%s, as %p) using arenas %p and %p\n",
      (void *) res, KheInstanceId(ins) != NULL ? KheInstanceId(ins) : "-",
      (void *) as, (void *) outer_a, (void *) inner_a);
  res->outer_arena = outer_a;			/* must be done early */
  res->inner_arena = inner_a;			/* must be done early */
  res->arena_set = as;
  HaArrayInit(res->monitors, inner_a);		/* must be done early */

  /* free lists (do them first) */
  HaArrayInit(res->free_meets, inner_a);
  if( DEBUG16 )
    fprintf(stderr, "  (a) %p->free_meets: items %p, count %d, length %d\n",
      (void *) res, (void *) res->free_meets.items,
      res->free_meets.count, res->free_meets.length);
  HaArrayInit(res->free_meet_bounds, inner_a);
  HaArrayInit(res->free_tasks, inner_a);
  HaArrayInit(res->free_task_bounds, inner_a);
  HaArrayInit(res->free_marks, inner_a);
  HaArrayInit(res->free_paths, inner_a);
  HaArrayInit(res->free_nodes, inner_a);
  HaArrayInit(res->free_layers, inner_a);
  HaArrayInit(res->free_zones, inner_a);
  /* HaArrayInit(res->free_taskings, inner_a); */
  HaArrayInit(res->free_task_sets, inner_a);
  HaArrayInit(res->free_meet_sets, inner_a);
  /* HaArrayInit(res->free_frame_makes, inner_a); */
  /* HaArrayInit(res->free_frame_workloads, inner_a); */
  HaArrayInit(res->free_monitor_links, inner_a);
  HaArrayInit(res->free_group_monitors, inner_a);
  HaArrayInit(res->free_traces, inner_a);
  /* HaArrayInit(res->free_workload_requirements, inner_a); */
  HaArrayInit(res->free_matchings, inner_a);
  HaArrayInit(res->free_matching_supply_chunks, inner_a);
  HaArrayInit(res->free_matching_supply_nodes, inner_a);
  HaArrayInit(res->free_matching_demand_chunks, inner_a);
  HaArrayInit(res->free_ordinary_demand_monitors, inner_a);
  HaArrayInit(res->free_workload_demand_monitors, inner_a);
  HaArrayInit(res->free_matching_hall_sets, inner_a);

  /* group monitor attributes */
  KheMonitorInitCommonFields((KHE_MONITOR) res, res, KHE_GROUP_MONITOR_TAG,
    KHE_LINEAR_COST_FUNCTION, 0);
  HaArrayInit(res->parent_links, inner_a);
  HaArrayInit(res->child_links, inner_a);
  HaArrayInit(res->defect_links, inner_a);
  HaArrayInit(res->traces, inner_a);
  res->sub_tag = -1;
  res->sub_tag_label = "Soln";

  /* outer attributes */
  HaArrayInit(res->soln_groups, outer_a);  /* must survive in placeholder */
  res->description = NULL;
  res->running_time = -1.0; /* negative means absent */
  res->instance = ins;
  res->diversifier = 0;
  res->global_visit_num = 0;
  if( DEBUG14 )
    fprintf(stderr, "  global_visit_num := 0\n");
  res->type = KHE_SOLN_ORDINARY;
  res->write_only_soln = NULL;
  res->invalid_error = NULL;

  /* soln time groups */
  res->time_sset_building = false;
  SSetInit(res->time_sset, inner_a);
  res->time_sset_table = SSetTableMake(inner_a);

  /* soln resource groups */
  res->resource_set_rt = NULL;
  KheSetInit(res->resource_set, inner_a);
  res->resource_set_table = KheSetTableMake(inner_a);
  /* HaArrayInit(res->resource_groups, inner_a); */

  /* soln event groups */
  res->curr_event_group = NULL;
  /* HaArrayInit(res->event_groups, inner_a); */

  /* inner attributes */
  HaArrayInit(res->meets, inner_a);
  /* HaArrayInit(res->meet_bounds, inner_a); */
  HaArrayInit(res->tasks, inner_a);
  /* HaArrayInit(res->task_bounds, inner_a); */
  HaArrayInit(res->nodes, inner_a);
  /* HaArrayInit(res->taskings, inner_a); */
  res->main_path = KhePathMake(res);
  HaArrayInit(res->marks, inner_a);

  /* inner attributes - event in soln objects, one per instance event */
  if( DEBUG1 )
    fprintf(stderr, "  KheSolnMake creating %d event in soln objects:\n",
      KheInstanceEventCount(ins));
  HaArrayInit(res->events_in_soln, inner_a);
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    es = KheEventInSolnMake(res, e);
    HaArrayAddLast(res->events_in_soln, es);
    KheEventInSolnAddTimetableMonitor(es);
  }

  /* inner attributes - resource in soln objects, one per resource */
  if( DEBUG1 )
    fprintf(stderr, "  KheSolnMake creating %d resource in soln objects:\n",
      KheInstanceResourceCount(ins));
  HaArrayInit(res->resources_in_soln, inner_a);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    r = KheInstanceResource(ins, i);
    rs = KheResourceInSolnMake(res, r);
    HaArrayAddLast(res->resources_in_soln, rs);
  }
  res->avail_solver = NULL;

  /* matchings and evenness */
  res->matching_type = KHE_MATCHING_TYPE_SOLVE;
  res->matching_weight = 0;
  res->matching = NULL;  /* no matching initially */
  if( DEBUG17 )
    fprintf(stderr, "make %p->matching = %p\n", (void *) res,
      (void *) res->matching);
  KheSetInit(res->matching_zero_domain, inner_a);
  KheSetInsert(res->matching_zero_domain, 0);
  SSetInit(res->matching_zero_time_domain, inner_a);
  SSetInsert(res->matching_zero_time_domain, 0);
  SSetFinalize(res->matching_zero_time_domain);
  res->evenness_handler = NULL;
  res->redundancy_handler = NULL;

  /* cycle meets and tasks */
  if( DEBUG1 )
    fprintf(stderr, "  KheSolnMake adding cycle meets and tasks:\n");
  HaArrayInit(res->time_to_cycle_meet, inner_a);
  HaArrayInit(res->time_to_cycle_offset, inner_a);
  HaArrayInit(res->packing_time_groups, inner_a);
  KheSolnAddInitialCycleMeet(res);  /* after HaArrayInit(res->free_meets) */
  KheSolnAddCycleTasks(res);

  /* copy */
  res->copy = NULL;

  /* make and attach constraint monitors */
  if( DEBUG1 )
    fprintf(stderr, "  KheSolnMake adding monitors:\n");
  KheSolnMakeAndAttachConstraintMonitors(res);
  if( DEBUG1 || DEBUG7 )
    fprintf(stderr, "] KheSolnMake(%s, %p) returning %p\n",
      KheInstanceId(ins), (void *) as, (void *) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnDelete(KHE_SOLN soln)                                        */
/*                                                                           */
/*  Delete soln.  This done by deleting the solution from its solution       */
/*  groups and then deleting (or recycling) its two arenas.                  */
/*                                                                           */
/*****************************************************************************/

void KheSolnDelete(KHE_SOLN soln)
{
  if( DEBUG4 )
  {
    fprintf(stderr, "[ KheSolnDelete(");
    KheSolnDebug(soln, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }
  while( HaArrayCount(soln->soln_groups) > 0 )
    KheSolnGroupDeleteSoln(HaArrayLast(soln->soln_groups), soln);
  if( soln->inner_arena != NULL )
  {
    if( DEBUG15 )
      fprintf(stderr, "  KheSolnDelete(%s %p) relinquishing inner arena %p\n",
	KheInstanceId(KheSolnInstance(soln)), (void *) soln,
	(void *) soln->inner_arena);
    KheSolnArenaEnd(soln, soln->inner_arena);
  }
  if( DEBUG15 )
    fprintf(stderr, "  KheSolnDelete(%s %p) relinquishing outer arena %p\n",
      KheInstanceId(KheSolnInstance(soln)), (void *) soln,
      (void *) soln->outer_arena);
  KheSolnArenaEnd(soln, soln->outer_arena);
  /* ***
  HaAr enaDelete(soln->inner_arena);
  HaAr enaDelete(soln->outer_arena);
  *** */
  if( DEBUG4 )
    fprintf(stderr, "] KheSolnDelete returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheSolnCopyDoPhase1(KHE_SOLN soln, HA_ARENA_SET as)             */
/*                                                                           */
/*  Carry out Phase 1 of the copying of soln, assuming soln->copy == NULL.   */
/*                                                                           */
/*****************************************************************************/

static KHE_SOLN KheSolnCopyDoPhase1(KHE_SOLN soln, HA_ARENA_SET as)
{
  KHE_EVENT_IN_SOLN es;  KHE_RESOURCE_IN_SOLN rs;  HA_ARENA inner_a, outer_a;
  int i;  KHE_MEET meet;  KHE_NODE node;  KHE_TIME_GROUP tg;  KHE_MONITOR m;
  KHE_MONITOR_LINK link;  KHE_SOLN copy;  KHE_TASK task;
  /* KHE_TASKING tasking; */

  if( soln->type == KHE_SOLN_ORDINARY )
  {
    /* soln is ordinary, full copy using outer and inner arenas */
    outer_a = HaArenaMake(as);
    inner_a = HaArenaMake(as);
    HaMake(copy, outer_a);
    if( DEBUG15 )
      fprintf(stderr,"  %p = KheSolnCopy(%s %p, as %p) with inner arena %p\n",
	(void *) copy, KheInstanceId(KheSolnInstance(soln)), (void *) soln,
	(void *) as, (void *) inner_a);
    soln->copy = copy;
    if( DEBUG7 )
      fprintf(stderr, "[ KheSolnCopyDoPhase1 ordinary(%p) making copy %p\n",
	(void *) soln, (void *) soln->copy);

    /* monitor attributes */
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) soln,
      inner_a);

    /* group monitor attributes */
    copy->sub_tag = soln->sub_tag;
    HaArrayInit(copy->child_links, inner_a);
    HaArrayForEach(soln->child_links, link, i)
      HaArrayAddLast(copy->child_links, KheMonitorLinkCopyPhase1(link,inner_a));
    HaArrayInit(copy->defect_links, inner_a);
    HaArrayForEach(soln->defect_links, link, i)
      HaArrayAddLast(copy->defect_links,KheMonitorLinkCopyPhase1(link,inner_a));
    HaArrayInit(copy->traces, inner_a);
    /* *** not checking this any more, because of the out of memory case
    HnAssert(HaArrayCount(soln->traces) == 0,
      "KheSolnCopyPhase1 internal error (traces)");
    *** */
    copy->sub_tag_label = HnStringCopy(soln->sub_tag_label, outer_a);

    /* free lists */
    HaArrayInit(copy->free_meets, inner_a);
    if( DEBUG16 )
      fprintf(stderr, "  (b) %p->free_meets: items %p, count %d, length %d\n",
	(void *) copy, (void *) copy->free_meets.items,
	copy->free_meets.count, copy->free_meets.length);
    HaArrayInit(copy->free_meet_bounds, inner_a);
    HaArrayInit(copy->free_tasks, inner_a);
    HaArrayInit(copy->free_task_bounds, inner_a);
    HaArrayInit(copy->free_marks, inner_a);
    HaArrayInit(copy->free_paths, inner_a);
    HaArrayInit(copy->free_nodes, inner_a);
    HaArrayInit(copy->free_layers, inner_a);
    HaArrayInit(copy->free_zones, inner_a);
    /* HaArrayInit(copy->free_taskings, inner_a); */
    HaArrayInit(copy->free_task_sets, inner_a);
    HaArrayInit(copy->free_meet_sets, inner_a);
    /* HaArrayInit(copy->free_frame_makes, inner_a); */
    HaArrayInit(copy->free_monitor_links, inner_a);
    HaArrayInit(copy->free_group_monitors, inner_a);
    HaArrayInit(copy->free_traces, inner_a);
    /* HaArrayInit(copy->free_workload_requirements, inner_a); */
    HaArrayInit(copy->free_matchings, inner_a);
    HaArrayInit(copy->free_matching_supply_chunks, inner_a);
    HaArrayInit(copy->free_matching_supply_nodes, inner_a);
    HaArrayInit(copy->free_matching_demand_chunks, inner_a);
    HaArrayInit(copy->free_ordinary_demand_monitors, inner_a);
    HaArrayInit(copy->free_workload_demand_monitors, inner_a);
    HaArrayInit(copy->free_matching_hall_sets, inner_a);

    /* top-level attributes */
    copy->outer_arena = outer_a;
    copy->inner_arena = inner_a;
    copy->arena_set = as;
    HaArrayInit(copy->soln_groups, outer_a);
    copy->instance = soln->instance;
    copy->description = HnStringCopy(soln->description, outer_a);
    copy->running_time = soln->running_time;
    copy->diversifier = soln->diversifier;
    copy->global_visit_num = soln->global_visit_num;

    /* solution type and related fields */
    copy->type = soln->type;
    copy->write_only_soln = (soln->write_only_soln == NULL ? NULL :
      KheSolnWriteOnlyCopyPhase1(soln->write_only_soln, outer_a));
    copy->invalid_error = (soln->invalid_error == NULL ? NULL :
      KmlErrorCopy(soln->invalid_error, outer_a));

    /* soln time groups */
    copy->time_sset_building = false;
    SSetInit(copy->time_sset, inner_a);
    copy->time_sset_table = SSetTableMake(inner_a);

    /* soln resource groups */
    copy->resource_set_rt = NULL;
    KheSetInit(copy->resource_set, inner_a);
    copy->resource_set_table = KheSetTableMake(inner_a);

    /* soln event groups */
    copy->curr_event_group = NULL;

    /* inner attributes */
    HaArrayInit(copy->events_in_soln, inner_a);
    HaArrayForEach(soln->events_in_soln, es, i)
      HaArrayAddLast(copy->events_in_soln,
	KheEventInSolnCopyPhase1(es, inner_a));

    HaArrayInit(copy->resources_in_soln, inner_a);
    HaArrayForEach(soln->resources_in_soln, rs, i)
      HaArrayAddLast(copy->resources_in_soln,
	KheResourceInSolnCopyPhase1(rs, inner_a));

    copy->avail_solver = NULL;

    HaArrayInit(copy->meets, inner_a);
    HaArrayForEach(soln->meets, meet, i)
      HaArrayAddLast(copy->meets, KheMeetCopyPhase1(meet, inner_a));

    HaArrayInit(copy->tasks, inner_a);
    HaArrayForEach(soln->tasks, task, i)
      HaArrayAddLast(copy->tasks, KheTaskCopyPhase1(task, inner_a));

    copy->main_path = KhePathMake(copy);
    HaArrayInit(copy->marks, inner_a);

    HaArrayInit(copy->nodes, inner_a);
    HaArrayForEach(soln->nodes, node, i)
      HaArrayAddLast(copy->nodes, KheNodeCopyPhase1(node, inner_a));

    /* ***
    HaArrayInit(copy->taskings, inner_a);
    HaArrayForEach(soln->taskings, tasking, i)
      HaArrayAddLast(copy->taskings, KheTaskingCopyPhase1(tasking, inner_a));
    *** */

    HaArrayInit(copy->monitors, inner_a);
    HaArrayForEach(soln->monitors, m, i)
      HaArrayAddLast(copy->monitors, KheMonitorCopyPhase1(m, inner_a));

    /* cycle meets and tasks */
    HaArrayInit(copy->time_to_cycle_meet, inner_a);
    HaArrayForEach(soln->time_to_cycle_meet, meet, i)
      HaArrayAddLast(copy->time_to_cycle_meet, KheMeetCopyPhase1(meet,inner_a));
    HaArrayInit(copy->time_to_cycle_offset, inner_a);
    HaArrayAppend(copy->time_to_cycle_offset, soln->time_to_cycle_offset, i);
    HaArrayInit(copy->packing_time_groups, inner_a);
    HaArrayForEach(soln->packing_time_groups, tg, i)
      HaArrayAddLast(copy->packing_time_groups, KheTimeGroupCopy(tg, inner_a));
    /* ***
    HaArrayAppend(copy->packing_time_groups, soln->packing_time_groups, i);
    *** */

    /* matchings and evenness */
    copy->matching_type = soln->matching_type;
    copy->matching_weight = soln->matching_weight;
    copy->matching = (soln->matching == NULL ? NULL :
      KheMatchingCopyPhase1(soln->matching, inner_a));
    if( DEBUG17 )
      fprintf(stderr, "copy %p->matching = %p\n", (void *) copy,
	(void *) copy->matching);
    KheSetInit(copy->matching_zero_domain, inner_a);
    KheSetInsert(copy->matching_zero_domain, 0);
    SSetInit(copy->matching_zero_time_domain, inner_a);
    SSetInsert(copy->matching_zero_time_domain, 0);
    SSetFinalize(copy->matching_zero_time_domain);
    copy->evenness_handler = (soln->evenness_handler == NULL ? NULL :
      KheEvennessHandlerCopyPhase1(soln->evenness_handler, inner_a));
    copy->redundancy_handler = (soln->redundancy_handler == NULL ? NULL :
      KheRedundancyHandlerCopyPhase1(soln->redundancy_handler, inner_a));
  }
  else
  {
    /* soln is a placeholder, minimal copy using outer arena only */
    outer_a = HaArenaMake(as);
    HaMake(copy, outer_a);
    soln->copy = copy;
    if( DEBUG7 )
      fprintf(stderr, "[ KheSolnCopyDoPhase1 placeholder(%p) making copy %p\n",
	(void *) soln, (void *) soln->copy);

    /* group monitor attributes */
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) soln,
      outer_a);
    /* no copying of parent links, apparently (should be empty anyway) */
    HaArrayInit(copy->child_links, outer_a);
    HaArrayInit(copy->defect_links, outer_a);
    HaArrayInit(copy->traces, outer_a);
    copy->sub_tag = soln->sub_tag;
    copy->sub_tag_label = HnStringCopy(soln->sub_tag_label, outer_a);

    /* free lists */
    HaArrayInit(copy->free_meets, outer_a);
    if( DEBUG16 )
      fprintf(stderr, "  (c) %p->free_meets: items %p, count %d, length %d\n",
	(void *) copy, (void *) copy->free_meets.items,
	copy->free_meets.count, copy->free_meets.length);
    HaArrayInit(copy->free_meet_bounds, outer_a);
    HaArrayInit(copy->free_tasks, outer_a);
    HaArrayInit(copy->free_task_bounds, outer_a);
    HaArrayInit(copy->free_marks, outer_a);
    HaArrayInit(copy->free_paths, outer_a);
    HaArrayInit(copy->free_nodes, outer_a);
    HaArrayInit(copy->free_layers, outer_a);
    HaArrayInit(copy->free_zones, outer_a);
    /* HaArrayInit(copy->free_taskings, outer_a); */
    HaArrayInit(copy->free_task_sets, outer_a);
    HaArrayInit(copy->free_meet_sets, outer_a);
    /* HaArrayInit(copy->free_frame_makes, outer_a); */
    HaArrayInit(copy->free_monitor_links, outer_a);
    HaArrayInit(copy->free_group_monitors, outer_a);
    HaArrayInit(copy->free_traces, outer_a);
    /* HaArrayInit(copy->free_workload_requirements, outer_a); */
    HaArrayInit(copy->free_matchings, outer_a);
    HaArrayInit(copy->free_matching_supply_chunks, outer_a);
    HaArrayInit(copy->free_matching_supply_nodes, outer_a);
    HaArrayInit(copy->free_matching_demand_chunks, outer_a);
    HaArrayInit(copy->free_ordinary_demand_monitors, outer_a);
    HaArrayInit(copy->free_workload_demand_monitors, outer_a);
    HaArrayInit(copy->free_matching_hall_sets, outer_a);

    /* top-level attributes */
    copy->outer_arena = outer_a;
    copy->inner_arena = NULL;
    copy->arena_set = as;
    HaArrayInit(copy->soln_groups, outer_a);
    copy->instance = soln->instance;
    copy->description = HnStringCopy(soln->description, outer_a);
    copy->running_time = soln->running_time;
    copy->diversifier = soln->diversifier;
    copy->global_visit_num = soln->global_visit_num;

    /* solution type and related fields */
    copy->type = soln->type;
    copy->write_only_soln = (soln->write_only_soln == NULL ? NULL :
      KheSolnWriteOnlyCopyPhase1(soln->write_only_soln, outer_a));
    copy->invalid_error = (soln->invalid_error == NULL ? NULL :
      KmlErrorCopy(soln->invalid_error, outer_a));

    /* soln time groups */
    copy->time_sset_building = false;
    SSetInit(copy->time_sset, outer_a);
    copy->time_sset_table = SSetTableMake(outer_a);

    /* soln resource groups */
    copy->resource_set_rt = NULL;
    KheSetInit(copy->resource_set, outer_a);
    copy->resource_set_table = KheSetTableMake(outer_a);

    /* soln event groups */
    copy->curr_event_group = NULL;

    /* inner attributes (but using the outer arena for this placeholder) */
    HaArrayInit(copy->events_in_soln, outer_a);
    HaArrayInit(copy->resources_in_soln, outer_a);
    copy->avail_solver = NULL;
    HaArrayInit(copy->meets, outer_a);
    HaArrayInit(copy->tasks, outer_a);
    copy->main_path = NULL;
    HaArrayInit(copy->marks, outer_a);
    HaArrayInit(copy->nodes, outer_a);
    /* HaArrayInit(copy->taskings, outer_a); */
    HaArrayInit(copy->monitors, outer_a);

    /* cycle meets and tasks */
    HaArrayInit(copy->time_to_cycle_meet, outer_a);
    HaArrayInit(copy->time_to_cycle_offset, outer_a);
    HaArrayInit(copy->packing_time_groups, outer_a);

    /* matchings and evenness */
    copy->matching_type = soln->matching_type;
    copy->matching_weight = soln->matching_weight;
    copy->matching = NULL;
    if( DEBUG17 )
      fprintf(stderr, "copy %p->matching = %p\n", (void *) copy,
	(void *) copy->matching);
    KheSetInit(copy->matching_zero_domain, outer_a);
    KheSetInsert(copy->matching_zero_domain, 0);
    SSetInit(copy->matching_zero_time_domain, outer_a);
    SSetInsert(copy->matching_zero_time_domain, 0);
    SSetFinalize(copy->matching_zero_time_domain);
    copy->evenness_handler = NULL;
    copy->redundancy_handler = NULL;
  }

  /* copy */
  copy->copy = NULL;
  if( DEBUG7 )
    fprintf(stderr, "] KheSolnCopyDoPhase1(%p) returning copy %p\n",
      (void *) soln, (void *) soln->copy);
  return soln->copy;
}


/* *** old version
static KHE_SOLN KheSolnCopyDoPhase1(KHE_SOLN soln, HA_ARENA_SET as)
{
  int i;  KHE_SOLN copy;  KHE_EVENT_IN_SOLN es;  KHE_RESOURCE_IN_SOLN rs;
  KHE_MEET meet;  KHE_NODE node;  KHE_MONITOR m;  KHE_MONITOR_LINK link;
  KHE_TASK task;  KHE_TASKING tasking;  HA_ARENA inner_a, outer_a;
  if( soln->copy == NULL )
  {
    ** boilerplate **
    if( soln->type == KHE_SOLN_ORDINARY )
    {
      ** soln is ordinary, full copy using outer and inner arenas **
      outer_a = HaArenaMake(as);
      inner_a = HaArenaMake(as);
      HaMake(copy, outer_a);
      if( DEBUG15 )
	fprintf(stderr,"  %p = KheSolnCopy(%s %p, as %p) with inner arena %p\n",
	  (void *) copy, KheInstanceId(KheSolnInstance(soln)), (void *) soln,
	  (void *) as, (void *) inner_a);
      if( DEBUG7 )
	fprintf(stderr, "[ %p = KheSolnCopyPhase1(%p)\n", (void *) copy,
	  (void *) soln);
      soln->copy = copy;

      ** group monitor attributes **
      KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) soln,
	inner_a);
      ** no copying of parent links, apparently (should be empty anyway) **
      HaArrayInit(copy->child_links, inner_a);
      HaArrayForEach(soln->child_links, link, i)
	HaArrayAddLast(copy->child_links, KheMonitorLinkCopyPhase1(link,inner_a));
      HaArrayInit(copy->defect_links, inner_a);
      HaArrayForEach(soln->defect_links, link, i)
	HaArrayAddLast(copy->defect_links,KheMonitorLinkCopyPhase1(link,inner_a));
      HaArrayInit(copy->traces, inner_a);
      copy->sub_tag = soln->sub_tag;
      copy->sub_tag_label = HnStringCopy(soln->sub_tag_label, outer_a);

      ** outer attributes **
      copy->outer_arena = outer_a;
      copy->inner_arena = inner_a;
      copy->arena_set = as;
      HaArrayInit(copy->soln_groups, outer_a);
      copy->description = HnStringCopy(soln->description, outer_a);
      copy->running_time = soln->running_time;
      copy->instance = soln->instance;
      copy->diversifier = soln->diversifier;
      copy->global_visit_num = soln->global_visit_num;
      copy->type = soln->type;
      copy->write_only_soln = (soln->write_only_soln == NULL ? NULL :
	KheSolnWriteOnlyCopyPhase1(soln->write_only_soln, outer_a));
      copy->invalid_error = (soln->invalid_error == NULL ? NULL :
	KmlErrorMake(soln->outer_arena, KmlErrorLineNum(soln->invalid_error),
	  KmlErrorColNum(soln->invalid_error), "%s",
	  KmlErrorString(soln->invalid_error)));

      ** soln time groups **
      copy->time_sset_building = false;
      SSetInit(copy->time_sset, inner_a);
      copy->time_sset_table = SSetTableMake(inner_a);

      ** soln resource groups **
      copy->resource_set_rt = NULL;
      KheSetInit(copy->resource_set, inner_a);
      copy->resource_set_table = KheSetTableMake(inner_a);

      ** soln event groups **
      copy->curr_event_group = NULL;

      ** inner attributes **
      HaArrayInit(copy->monitors, inner_a);
      HaArrayForEach(soln->monitors, m, i)
	HaArrayAddLast(copy->monitors, KheMonitorCopyPhase1(m, inner_a));
      HaArrayInit(copy->meets, inner_a);
      HaArrayForEach(soln->meets, meet, i)
	HaArrayAddLast(copy->meets, KheMeetCopyPhase1(meet, inner_a));
      HaArrayInit(copy->tasks, inner_a);
      HaArrayForEach(soln->tasks, task, i)
	HaArrayAddLast(copy->tasks, KheTaskCopyPhase1(task, inner_a));
      HaArrayInit(copy->nodes, inner_a);
      HaArrayForEach(soln->nodes, node, i)
	HaArrayAddLast(copy->nodes, KheNodeCopyPhase1(node, inner_a));
      HaArrayInit(copy->taskings, inner_a);
      HaArrayForEach(soln->taskings, tasking, i)
	HaArrayAddLast(copy->taskings, KheTaskingCopyPhase1(tasking, inner_a));
      copy->main_path = KhePathMake(copy);
      HaArrayInit(copy->marks, inner_a);

      ** inner attributes - event in soln objects, one per instance event **
      HaArrayInit(copy->events_in_soln, inner_a);
      HaArrayForEach(soln->events_in_soln, es, i)
	HaArrayAddLast(copy->events_in_soln,
	  KheEventInSolnCopyPhase1(es, inner_a));

      ** inner attributes - resource in soln objects, one per resource **
      HaArrayInit(copy->resources_in_soln, inner_a);
      HaArrayForEach(soln->resources_in_soln, rs, i)
	HaArrayAddLast(copy->resources_in_soln,
	  KheResourceInSolnCopyPhase1(rs, inner_a));
      copy->avail_solver = NULL;

      ** cycle meets and tasks **
      HaArrayInit(copy->time_to_cycle_meet, inner_a);
      HaArrayForEach(soln->time_to_cycle_meet, meet, i)
	HaArrayAddLast(copy->time_to_cycle_meet, KheMeetCopyPhase1(meet,inner_a));
      HaArrayInit(copy->time_to_cycle_offset, inner_a);
      HaArrayAppend(copy->time_to_cycle_offset, soln->time_to_cycle_offset, i);
      HaArrayInit(copy->packing_time_groups, inner_a);
      HaArrayAppend(copy->packing_time_groups, soln->packing_time_groups, i);

      ** matchings and evenness **
      copy->matching_type = soln->matching_type;
      copy->matching_weight = soln->matching_weight;
      copy->matching = soln->matching == NULL ? NULL :
	KheMatchingCopyPhase1(soln->matching);
      KheSetInit(copy->matching_zero_domain, inner_a);
      KheSetInsert(copy->matching_zero_domain, 0);
      SSetInit(copy->matching_zero_time_domain, inner_a);
      SSetInsert(copy->matching_zero_time_domain, 0);
      SSetFinalize(copy->matching_zero_time_domain);
      copy->evenness_handler = (soln->evenness_handler == NULL ? NULL :
	KheEvennessHandlerCopyPhase1(soln->evenness_handler, inner_a));

      ** free lists **
      HaArrayInit(copy->free_meets, inner_a);
      HaArrayInit(copy->free_meet_bounds, inner_a);
      HaArrayInit(copy->free_tasks, inner_a);
      HaArrayInit(copy->free_task_bounds, inner_a);
      HaArrayInit(copy->free_marks, inner_a);
      HaArrayInit(copy->free_paths, inner_a);
      HaArrayInit(copy->free_nodes, inner_a);
      HaArrayInit(copy->free_layers, inner_a);
      HaArrayInit(copy->free_zones, inner_a);
      HaArrayInit(copy->free_taskings, inner_a);
      HaArrayInit(copy->free_task_sets, inner_a);
      HaArrayInit(copy->free_meet_sets, inner_a);
      HaArrayInit(copy->free_frame_makes, inner_a);
      HaArrayInit(copy->free_monitor_links, inner_a);
      HaArrayInit(copy->free_group_monitors, inner_a);
      HaArrayInit(copy->free_traces, inner_a);
      HaArrayInit(copy->free_workload_requirements, inner_a);
      HaArrayInit(copy->free_matchings, inner_a);
      HaArrayInit(copy->free_matching_supply_chunks, inner_a);
      HaArrayInit(copy->free_matching_supply_nodes, inner_a);
      HaArrayInit(copy->free_matching_demand_chunks, inner_a);
      HaArrayInit(copy->free_ordinary_demand_monitors, inner_a);
      HaArrayInit(copy->free_workload_demand_monitors, inner_a);
      HaArrayInit(copy->free_matching_hall_sets, inner_a);
    }
    else
    {
      ** soln is a placeholder, minimal copy using outer arena only **
      outer_a = HaArenaMake(as);
      HaMake(copy, outer_a);
      if( DEBUG7 )
	fprintf(stderr, "[ %p = KheSolnCopyPhase1(%p)\n", (void *) copy,
	  (void *) soln);
      soln->copy = copy;

      ** group monitor attributes **
      KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) soln,
	outer_a);
      ** no copying of parent links, apparently (should be empty anyway) **
      HaArrayInit(copy->child_links, outer_a);
      HaArrayInit(copy->defect_links, outer_a);
      HaArrayInit(copy->traces, outer_a);
      copy->sub_tag = soln->sub_tag;
      copy->sub_tag_label = HnStringCopy(soln->sub_tag_label, outer_a);

      ** outer attributes **
      copy->outer_arena = outer_a;
      copy->inner_arena = NULL;
      copy->arena_set = as;
      HaArrayInit(copy->soln_groups, outer_a);
      copy->description = HnStringCopy(soln->description, outer_a);
      copy->running_time = soln->running_time;
      copy->instance = soln->instance;
      copy->diversifier = soln->diversifier;
      copy->global_visit_num = soln->global_visit_num;
      copy->type = soln->type;
      copy->write_only_soln = (soln->write_only_soln == NULL ? NULL :
	KheSolnWriteOnlyCopyPhase1(soln->write_only_soln, outer_a));
      copy->invalid_error = (soln->invalid_error == NULL ? NULL :
	KmlErrorCopy(soln->invalid_error, outer_a));

      ** soln time groups **
      copy->time_sset_building = false;
      SSetInit(copy->time_sset, outer_a);
      copy->time_sset_table = SSetTableMake(outer_a);

      ** soln resource groups **
      copy->resource_set_rt = NULL;
      KheSetInit(copy->resource_set, outer_a);
      copy->resource_set_table = KheSetTableMake(outer_a);

      ** soln event groups **
      copy->curr_event_group = NULL;

      ** inner attributes (but using the outer arena for this placeholder) **
      HaArrayInit(copy->monitors, outer_a);
      HaArrayInit(copy->meets, outer_a);
      HaArrayInit(copy->tasks, outer_a);
      HaArrayInit(copy->nodes, outer_a);
      HaArrayInit(copy->taskings, outer_a);
      copy->main_path = NULL;
      HaArrayInit(copy->marks, outer_a);

      ** inner attributes - event in soln objects, one per instance event **
      HaArrayInit(copy->events_in_soln, outer_a);

      ** inner attributes - resource in soln objects, one per resource **
      HaArrayInit(copy->resources_in_soln, outer_a);
      copy->avail_solver = NULL;

      ** cycle meets and tasks **
      HaArrayInit(copy->time_to_cycle_meet, outer_a);
      HaArrayInit(copy->time_to_cycle_offset, outer_a);
      HaArrayInit(copy->packing_time_groups, outer_a);

      ** matchings and evenness **
      copy->matching_type = soln->matching_type;
      copy->matching_weight = soln->matching_weight;
      copy->matching = NULL;
      KheSetInit(copy->matching_zero_domain, outer_a);
      KheSetInsert(copy->matching_zero_domain, 0);
      SSetInit(copy->matching_zero_time_domain, outer_a);
      SSetInsert(copy->matching_zero_time_domain, 0);
      SSetFinalize(copy->matching_zero_time_domain);
      copy->evenness_handler = NULL;

      ** free lists **
      HaArrayInit(copy->free_meets, outer_a);
      HaArrayInit(copy->free_meet_bounds, outer_a);
      HaArrayInit(copy->free_tasks, outer_a);
      HaArrayInit(copy->free_task_bounds, outer_a);
      HaArrayInit(copy->free_marks, outer_a);
      HaArrayInit(copy->free_paths, outer_a);
      HaArrayInit(copy->free_nodes, outer_a);
      HaArrayInit(copy->free_layers, outer_a);
      HaArrayInit(copy->free_zones, outer_a);
      HaArrayInit(copy->free_taskings, outer_a);
      HaArrayInit(copy->free_task_sets, outer_a);
      HaArrayInit(copy->free_meet_sets, outer_a);
      HaArrayInit(copy->free_frame_makes, outer_a);
      HaArrayInit(copy->free_monitor_links, outer_a);
      HaArrayInit(copy->free_group_monitors, outer_a);
      HaArrayInit(copy->free_traces, outer_a);
      HaArrayInit(copy->free_workload_requirements, outer_a);
      HaArrayInit(copy->free_matchings, outer_a);
      HaArrayInit(copy->free_matching_supply_chunks, outer_a);
      HaArrayInit(copy->free_matching_supply_nodes, outer_a);
      HaArrayInit(copy->free_matching_demand_chunks, outer_a);
      HaArrayInit(copy->free_ordinary_demand_monitors, outer_a);
      HaArrayInit(copy->free_workload_demand_monitors, outer_a);
      HaArrayInit(copy->free_matching_hall_sets, outer_a);
    }

    ** copy **
    copy->copy = NULL;
    if( DEBUG7 )
      fprintf(stderr, "] KheSolnCopyPhase1 returning\n");
  }
  return soln->copy;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheSolnCopyPhase1(KHE_SOLN soln)                                */
/*                                                                           */
/*  Carry out Phase 1 of the copying of soln.                                */
/*                                                                           */
/*  Implementation note.  This is only called from elsewhere in the          */
/*  program, after KheSolnCopyDoPhase1 has set soln->copy.                   */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheSolnCopyPhase1(KHE_SOLN soln)
{
  HnAssert(soln->copy != NULL, "KheSolnCopyPhase1 internal error");
  return soln->copy;
  /* return KheSolnCopyDoPhase1(soln, NULL); */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnCopyPhase2(KHE_SOLN soln)                                    */
/*                                                                           */
/*  Carry out Phase 2 of the copying of soln.                                */
/*                                                                           */
/*****************************************************************************/

void KheSolnCopyPhase2(KHE_SOLN soln)
{
  int i;  KHE_EVENT_IN_SOLN es;  KHE_RESOURCE_IN_SOLN rs;  KHE_MEET meet;
  KHE_NODE node;  KHE_MONITOR m;  KHE_TASK task;
  /* KHE_TASKING tasking; */
  KHE_MONITOR_LINK link;
  if( soln->copy != NULL )
  {
    /* boilerplate */
    if( DEBUG7 )
      fprintf(stderr, "[ KheSolnCopyPhase2(%p); soln->copy = %p\n",
	(void *) soln, (void *) soln->copy);
    soln->copy = NULL;

    /* group monitor attributes */
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) soln);
    HaArrayForEach(soln->child_links, link, i)
      KheMonitorLinkCopyPhase2(link);
    HaArrayForEach(soln->defect_links, link, i)
      KheMonitorLinkCopyPhase2(link);

    /* outer attributes */
    if( soln->write_only_soln != NULL )
      KheSolnWriteOnlyCopyPhase2(soln->write_only_soln);

    /* soln time groups */

    /* soln resource groups */

    /* soln event groups */

    /* inner attributes */
    HaArrayForEach(soln->monitors, m, i)
      KheMonitorCopyPhase2(m);
    HaArrayForEach(soln->meets, meet, i)
      KheMeetCopyPhase2(meet);
    /* ***
    HaArrayForEach(soln->meet_bounds, mb, i)
      KheMeetBoundCopyPhase2(mb);
    *** */
    HaArrayForEach(soln->tasks, task, i)
      KheTaskCopyPhase2(task);
    HaArrayForEach(soln->nodes, node, i)
      KheNodeCopyPhase2(node);
    /* ***
    HaArrayForEach(soln->taskings, tasking, i)
      KheTaskingCopyPhase2(tasking);
    *** */
    HaArrayForEach(soln->events_in_soln, es, i)
      KheEventInSolnCopyPhase2(es);
    HaArrayForEach(soln->resources_in_soln, rs, i)
      KheResourceInSolnCopyPhase2(rs);

    /* cycle meets */

    /* matchings and evenness */
    if( soln->matching != NULL )
      KheMatchingCopyPhase2(soln->matching);
    if( soln->evenness_handler != NULL )
      KheEvennessHandlerCopyPhase2(soln->evenness_handler);
    if( soln->redundancy_handler != NULL )
      KheRedundancyHandlerCopyPhase2(soln->redundancy_handler);

    /* free lists */
    if( DEBUG7 )
      fprintf(stderr, "] KheSolnCopyPhase2(%p) returning\n", (void *) soln);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheSolnCopy(KHE_SOLN soln)                                      */
/*                                                                           */
/*  Make a deep copy of soln.                                                */
/*                                                                           */
/*  Formerly this code checked that various things were not partially        */
/*  completed.  However, if memory runs out those things could indeed        */
/*  be partially completed.  So those checks have been removed.              */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheSolnCopy(KHE_SOLN soln, HA_ARENA_SET as)
{
  KHE_SOLN copy;

  /* probabilistic check for re-entrant call */
  /* ***
  if( DEBUG7 )
    fprintf(stderr, "[ KheSolnCopy(soln = %p, as = %p)\n",
      (void *) soln, (void *) as);
  *** */
  HnAssert(soln->copy == NULL, "re-entrant call on KheSolnCopy");

  /* check temporary group construction variables */
  /* *** removed as above
  HnAssert(!soln->time_sset_building,
    "KheSolnCopy called while time group under construction");
  HnAssert(soln->resource_set_rt == NULL,
    "KheSolnCopy called while resource group under construction");
  HnAssert(soln->curr_event_group == NULL,
    "KheSolnCopy called while event group under construction");
  HnAssert(HaArrayCount(soln->marks) == 0,
    "KheSolnCopy called after unmatched KheMarkBegin");
  *** */

  /* check not currently tracing */
  /* *** removed as above
  HnAssert(HaArrayCount(soln->traces) == 0,
    "KheSolnCopy called while being traced");
  *** */

  /* update the matching */
  KheSolnMatchingUpdate(soln);

  /* carry out phases 1 and 2 of the copy */
  copy = KheSolnCopyDoPhase1(soln, as);
  /* ***
  if( DEBUG7 )
    fprintf(stderr, "  KheSolnCopyDoPhase1(soln = %p, as = %p) returned %p\n",
      (void *) soln, (void *) as, (void *) copy);
  *** */
  KheSolnCopyPhase2(soln);
  /* ***
  if( DEBUG7 )
    fprintf(stderr, "] KheSolnCopy(soln = %p, as = %p) returning %p\n",
      (void *) soln, (void *) as, (void *) copy);
  *** */
  return copy;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "top-level operations - simple attributes - arenas"            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA KheSolnArenaBegin(KHE_SOLN soln)                                */
/*                                                                           */
/*  Obtain a fresh arena from soln.                                          */
/*                                                                           */
/*****************************************************************************/

HA_ARENA KheSolnArenaBegin(KHE_SOLN soln)
{
  if( DEBUG18 )
    fprintf(stderr, "  calling KheSolnArenaBegin(#%d)\n",
      KheSolnDiversifier(soln));
  return HaArenaMake(soln->arena_set);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnArenaEnd(KHE_SOLN soln, HA_ARENA a)                          */
/*                                                                           */
/*  Return a to soln.                                                        */
/*                                                                           */
/*****************************************************************************/

void KheSolnArenaEnd(KHE_SOLN soln, HA_ARENA a)
{
  if( DEBUG18 )
    fprintf(stderr, "  calling KheSolnArenaEnd(#%d)\n",
      KheSolnDiversifier(soln));
  HaArenaDelete(a);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnSetArenaSet(KHE_SOLN soln, HA_ARENA_SET as)                  */
/*                                                                           */
/*  Set soln's arena set to as.                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnSetArenaSet(KHE_SOLN soln, HA_ARENA_SET as)
{
  soln->arena_set = as;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA_SET KheSolnArenaSet(KHE_SOLN soln)                              */
/*                                                                           */
/*  Return soln's arena set; it may be NULL.                                 */
/*                                                                           */
/*****************************************************************************/

HA_ARENA_SET KheSolnArenaSet(KHE_SOLN soln)
{
  return soln->arena_set;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnJmpEnvBegin(KHE_SOLN soln, jmp_buf *env)                     */
/*                                                                           */
/*  Call HaArenaSetJmpEnvBegin on soln's arena set.                          */
/*                                                                           */
/*****************************************************************************/

void KheSolnJmpEnvBegin(KHE_SOLN soln, jmp_buf *env)
{
  HaArenaSetJmpEnvBegin(soln->arena_set, env);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnJmpEnvEnd(KHE_SOLN soln)                                     */
/*                                                                           */
/*  Call HaArenaSetJmpEnvEnd on soln's arena set.                            */
/*                                                                           */
/*****************************************************************************/

void KheSolnJmpEnvEnd(KHE_SOLN soln)
{
  HaArenaSetJmpEnvEnd(soln->arena_set);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "top-level operations - simple attributes - solution groups"   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAddSolnGroup(KHE_SOLN soln, KHE_SOLN_GROUP soln_group)       */
/*                                                                           */
/*  Add soln_group to soln; soln's instance is in soln_group's archive.      */
/*                                                                           */
/*****************************************************************************/

void KheSolnAddSolnGroup(KHE_SOLN soln, KHE_SOLN_GROUP soln_group)
{
  HaArrayAddLast(soln->soln_groups, soln_group);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnDeleteSolnGroup(KHE_SOLN soln, KHE_SOLN_GROUP soln_group)    */
/*                                                                           */
/*  Delete soln_group from soln's list of soln_groups.  It must be present.  */
/*                                                                           */
/*****************************************************************************/

void KheSolnDeleteSolnGroup(KHE_SOLN soln, KHE_SOLN_GROUP soln_group)
{
  int pos;
  if( !HaArrayContains(soln->soln_groups, soln_group, &pos) )
    HnAbort("KheSolnDeleteSolnGroup internal error");
  HaArrayDeleteAndShift(soln->soln_groups, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnSolnGroupCount(KHE_SOLN soln)                                 */
/*                                                                           */
/*  Return the number of solution groups containing soln.                    */
/*                                                                           */
/*****************************************************************************/

int KheSolnSolnGroupCount(KHE_SOLN soln)
{
  return HaArrayCount(soln->soln_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_GROUP KheSolnSolnGroup(KHE_SOLN soln, int i)                    */
/*                                                                           */
/*  Return the i'th solution group containing soln.                          */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN_GROUP KheSolnSolnGroup(KHE_SOLN soln, int i)
{
  return HaArray(soln->soln_groups, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "top-level operations - simple attributes - other"             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA KheSolnArena(KHE_SOLN soln)                                     */
/*                                                                           */
/*  Return soln's (inner) arena.                                             */
/*                                                                           */
/*****************************************************************************/

HA_ARENA KheSolnArena(KHE_SOLN soln)
{
  return soln->inner_arena;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INSTANCE KheSolnInstance(KHE_SOLN soln)                              */
/*                                                                           */
/*  Return the instance of soln.                                             */
/*                                                                           */
/*****************************************************************************/

KHE_INSTANCE KheSolnInstance(KHE_SOLN soln)
{
  return soln->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnSetDescription(KHE_SOLN soln, char *description)             */
/*                                                                           */
/*  Set or reset the Description attribute of soln.                          */
/*                                                                           */
/*****************************************************************************/

void KheSolnSetDescription(KHE_SOLN soln, char *description)
{
  soln->description = HnStringCopy(description, soln->outer_arena);
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheSolnDescription(KHE_SOLN soln)                                  */
/*                                                                           */
/*  Return the Description attribute of soln.                                */
/*                                                                           */
/*****************************************************************************/

char *KheSolnDescription(KHE_SOLN soln)
{
  return soln->description;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnSetRunningTime(KHE_SOLN soln, float running_time)            */
/*                                                                           */
/*  Set or reset the RunningTime attribute of soln.                          */
/*                                                                           */
/*****************************************************************************/

void KheSolnSetRunningTime(KHE_SOLN soln, float running_time)
{
  soln->running_time = running_time;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnHasRunningTime(KHE_SOLN soln, float *running_time)           */
/*                                                                           */
/*  Return the RunningTime attribute of soln.                                */
/*                                                                           */
/*****************************************************************************/

bool KheSolnHasRunningTime(KHE_SOLN soln, float *running_time)
{
  *running_time = soln->running_time;
  return soln->running_time >= 0.0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnIncreasingRunningTimeTypedCmp(KHE_SOLN soln1, KHE_SOLN soln2) */
/*                                                                           */
/*  Comparison function for sorting into increasing running time, with       */
/*  no running time taken to be a very large value.                          */
/*                                                                           */
/*****************************************************************************/

int KheSolnIncreasingRunningTimeTypedCmp(KHE_SOLN soln1, KHE_SOLN soln2)
{
  if( soln1->running_time < 0.0 )
  {
    if( soln2->running_time < 0.0 )
    {
      /* both absent */
      return 0;
    }
    else
    {
      /* soln1 absent, soln2 present */
      return 1;
    }
  }
  else
  {
    if( soln2->running_time < 0.0 )
    {
      /* soln1 present, soln2 absent */
      return -1;
    }
    else
    {
      /* both present */
      if( soln1->running_time < soln2->running_time )
	return -1;
      else if( soln1->running_time > soln2->running_time )
	return 1;
      else
	return 0;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnIncreasingRunningTimeCmp(const void *t1, const void *t2)      */
/*                                                                           */
/*  Untyped version of KheSolnIncreasingRunningTimeTypedCmp.                 */
/*                                                                           */
/*****************************************************************************/

int KheSolnIncreasingRunningTimeCmp(const void *t1, const void *t2)
{
  KHE_SOLN soln1 = * (KHE_SOLN *) t1;
  KHE_SOLN soln2 = * (KHE_SOLN *) t2;
  return KheSolnIncreasingRunningTimeTypedCmp(soln1, soln2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnSetBack(KHE_SOLN soln, void *back)                           */
/*                                                                           */
/*  Set the back pointer of soln.                                            */
/*                                                                           */
/*****************************************************************************/

void KheSolnSetBack(KHE_SOLN soln, void *back)
{
  soln->back = back;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheSolnBack(KHE_SOLN soln)                                         */
/*                                                                           */
/*  Return the back pointer of soln.                                         */
/*                                                                           */
/*****************************************************************************/

void *KheSolnBack(KHE_SOLN soln)
{
  return soln->back;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "top-level operations - diversification"                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnSetDiversifier(KHE_SOLN soln, int val)                       */
/*                                                                           */
/*  Set the diversifier of soln to val.                                      */
/*                                                                           */
/*****************************************************************************/

void KheSolnSetDiversifier(KHE_SOLN soln, int val)
{
  soln->diversifier = val;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnDiversifier(KHE_SOLN soln)                                    */
/*                                                                           */
/*  Return the diversifier of soln.                                          */
/*                                                                           */
/*****************************************************************************/

int KheSolnDiversifier(KHE_SOLN soln)
{
  return soln->diversifier;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnDiversifierChoose(KHE_SOLN soln, int c)                       */
/*                                                                           */
/*  Choose an integer i in the range 0 <= i < c using soln's diversifier.    */
/*                                                                           */
/*****************************************************************************/

int KheSolnDiversifierChoose(KHE_SOLN soln, int c)
{
  int i, c1f;
  c1f = 1;
  for( i = 1;  i < c && c1f <= soln->diversifier;  i++ )
    c1f *= i;
  return ((soln->diversifier / c1f) + (soln->diversifier % c1f)) % c;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "top-level operations - visit numbers"                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnSetGlobalVisitNum(KHE_SOLN soln, int num)                    */
/*                                                                           */
/*  Set soln's visit number to num.                                          */
/*                                                                           */
/*****************************************************************************/

void KheSolnSetGlobalVisitNum(KHE_SOLN soln, int num)
{
  soln->global_visit_num = num;
  if( DEBUG14 )
    fprintf(stderr, "  global_visit_num := %d\n", num);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnGlobalVisitNum(KHE_SOLN soln)                                 */
/*                                                                           */
/*  Return soln's visit number.                                              */
/*                                                                           */
/*****************************************************************************/

int KheSolnGlobalVisitNum(KHE_SOLN soln)
{
  return soln->global_visit_num;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnNewGlobalVisit(KHE_SOLN soln)                                */
/*                                                                           */
/*  Increment soln's visit number.                                           */
/*                                                                           */
/*****************************************************************************/

void KheSolnNewGlobalVisit(KHE_SOLN soln)
{
  soln->global_visit_num++;
  if( DEBUG14 )
    fprintf(stderr, "  global_visit_num := %d\n", soln->global_visit_num);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "top-level operations - placeholder and invalid solutions"     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_TYPE KheSolnType(KHE_SOLN soln)                                 */
/*                                                                           */
/*  Return the type of soln.                                                 */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN_TYPE KheSolnType(KHE_SOLN soln)
{
  return soln->type;
}


/*****************************************************************************/
/*                                                                           */
/*  KML_ERROR KheSolnInvalidError(KHE_SOLN soln)                             */
/*                                                                           */
/*  Return the error that rendered soln invalid, or NULL if not invalid.     */
/*                                                                           */
/*****************************************************************************/

KML_ERROR KheSolnInvalidError(KHE_SOLN soln)
{
  return soln->invalid_error;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnReduceFromOrdinaryToPlaceholder(KHE_SOLN soln)               */
/*                                                                           */
/*  Reduce soln from an ordinary solution to some kind of placeholder        */
/*  solution.  This includes deleting soln's inner arena.                    */
/*                                                                           */
/*****************************************************************************/

static void KheSolnReduceFromOrdinaryToPlaceholder(KHE_SOLN soln)
{
  if( DEBUG15 )
    fprintf(stderr,
      "  KheSolnReduceFromOrdinaryToPlaceholder(%s %p) relinquishing "
      "inner arena %p\n", KheInstanceId(KheSolnInstance(soln)),
      (void *) soln, (void *) soln->inner_arena);

  /* recycle inner arena */
  KheSolnArenaEnd(soln, soln->inner_arena);
  soln->inner_arena = NULL;

  /* monitor and group monitor attributes */
  HaArrayInit(soln->parent_links, soln->outer_arena);
  HaArrayInit(soln->child_links, soln->outer_arena);
  HaArrayInit(soln->defect_links, soln->outer_arena);
  HaArrayInit(soln->traces, soln->outer_arena);

  /* free lists */
  HaArrayInit(soln->free_meets, soln->outer_arena);
  if( DEBUG16 )
    fprintf(stderr, "  (d) %p->free_meets: items %p, count %d, length %d\n",
      (void *) soln, (void *) soln->free_meets.items,
      soln->free_meets.count, soln->free_meets.length);
  HaArrayInit(soln->free_meet_bounds, soln->outer_arena);
  HaArrayInit(soln->free_tasks, soln->outer_arena);
  HaArrayInit(soln->free_task_bounds, soln->outer_arena);
  HaArrayInit(soln->free_marks, soln->outer_arena);
  HaArrayInit(soln->free_paths, soln->outer_arena);
  HaArrayInit(soln->free_nodes, soln->outer_arena);
  HaArrayInit(soln->free_layers, soln->outer_arena);
  HaArrayInit(soln->free_zones, soln->outer_arena);
  /* HaArrayInit(soln->free_taskings, soln->outer_arena); */
  HaArrayInit(soln->free_task_sets, soln->outer_arena);
  HaArrayInit(soln->free_meet_sets, soln->outer_arena);
  /* HaArrayInit(soln->free_frame_makes, soln->outer_arena); */
  HaArrayInit(soln->free_monitor_links, soln->outer_arena);
  HaArrayInit(soln->free_group_monitors, soln->outer_arena);
  HaArrayInit(soln->free_traces, soln->outer_arena);
  /* HaArrayInit(soln->free_workload_requirements, soln->outer_arena); */
  HaArrayInit(soln->free_matchings, soln->outer_arena);
  HaArrayInit(soln->free_matching_supply_chunks, soln->outer_arena);
  HaArrayInit(soln->free_matching_supply_nodes, soln->outer_arena);
  HaArrayInit(soln->free_matching_demand_chunks, soln->outer_arena);
  HaArrayInit(soln->free_ordinary_demand_monitors, soln->outer_arena);
  HaArrayInit(soln->free_workload_demand_monitors, soln->outer_arena);
  HaArrayInit(soln->free_matching_hall_sets, soln->outer_arena);

  /* top-level attributes */
  /* no changes needed here, other than the one to soln->inner_arena above */

  /* solution type and related fields */
  /* the caller will make whatever changes are needed here */

  /* soln time groups */
  soln->time_sset_building = false;
  SSetInit(soln->time_sset, soln->outer_arena);
  soln->time_sset_table = SSetTableMake(soln->outer_arena);

  /* soln resource groups */
  soln->resource_set_rt = NULL;
  KheSetInit(soln->resource_set, soln->outer_arena);
  soln->resource_set_table = KheSetTableMake(soln->outer_arena);

  /* soln event groups */
  soln->curr_event_group = NULL;

  /* inner attributes */
  HaArrayInit(soln->events_in_soln, soln->outer_arena);
  HaArrayInit(soln->resources_in_soln, soln->outer_arena);
  soln->avail_solver = NULL;
  HaArrayInit(soln->meets, soln->outer_arena);
  HaArrayInit(soln->tasks, soln->outer_arena);
  soln->main_path = NULL;
  HaArrayInit(soln->marks, soln->outer_arena);
  HaArrayInit(soln->nodes, soln->outer_arena);
  /* HaArrayInit(soln->taskings, soln->outer_arena); */
  HaArrayInit(soln->monitors, soln->outer_arena);

  /* cycle meets and tasks */
  HaArrayInit(soln->time_to_cycle_meet, soln->outer_arena);
  HaArrayInit(soln->time_to_cycle_offset, soln->outer_arena);
  HaArrayInit(soln->packing_time_groups, soln->outer_arena);

  /* matchings and evenness */
  soln->matching = NULL;
  if( DEBUG17 )
    fprintf(stderr, "recycle %p->matching = %p\n", (void *) soln,
      (void *) soln->matching);
  KheSetInit(soln->matching_zero_domain, soln->outer_arena);
  KheSetInsert(soln->matching_zero_domain, 0);
  SSetInit(soln->matching_zero_time_domain, soln->outer_arena);
  SSetInsert(soln->matching_zero_time_domain, 0);
  SSetFinalize(soln->matching_zero_time_domain);
  soln->evenness_handler = NULL;
  soln->redundancy_handler = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheSolnTypeShow(KHE_SOLN_TYPE soln_type)                           */
/*                                                                           */
/*  Return a string representation of the vale of soln_type.                 */
/*                                                                           */
/*****************************************************************************/

static char *KheSolnTypeShow(KHE_SOLN_TYPE soln_type)
{
  switch( soln_type )
  {
    case KHE_SOLN_INVALID_PLACEHOLDER:	return "invalid placeholder";
    case KHE_SOLN_BASIC_PLACEHOLDER:	return "basic placeholder";
    case KHE_SOLN_WRITABLE_PLACEHOLDER:	return "writable placeholder";
    case KHE_SOLN_ORDINARY:		return "ordinary";
    default:				return "??";
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void ReduceAbort(KHE_SOLN_TYPE from_type, KHE_SOLN_TYPE to_type)         */
/*                                                                           */
/*  Abort with an error message, saying either that to_type is invalid,      */
/*  or that we cannot reduce from from_type to to_type.                      */
/*                                                                           */
/*****************************************************************************/

static void ReduceAbort(KHE_SOLN_TYPE from_type, KHE_SOLN_TYPE to_type)
{
  if( to_type < KHE_SOLN_INVALID_PLACEHOLDER || to_type > KHE_SOLN_ORDINARY )
    HnAbort("KheSolnReduceToPlaceholder: unknown soln_type (value %d)",to_type);
  else
    HnAbort("KheSolnReduceToPlaceholder: cannot reduce from %s to %s",
      KheSolnTypeShow(from_type), KheSolnTypeShow(to_type));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnTypeReduce(KHE_SOLN soln, KHE_SOLN_TYPE soln_type,           */
/*    KML_ERROR ke)                                                          */
/*                                                                           */
/*  Reduce the type of soln to soln_type.                                    */
/*                                                                           */
/*  If soln_type is KHE_SOLN_INVALID_PLACEHOLDER, set soln->invalid_error    */
/*  to a copy of ke, which in that case must be non-NULL.                    */
/*                                                                           */
/*****************************************************************************/

void KheSolnTypeReduce(KHE_SOLN soln, KHE_SOLN_TYPE soln_type, KML_ERROR ke)
{
  switch( soln->type )
  {
    case KHE_SOLN_INVALID_PLACEHOLDER:

      switch( soln_type )
      {
	case KHE_SOLN_INVALID_PLACEHOLDER:

	  /* reduce from invalid placeholder to invalid placeholder */
	  HnAssert(ke != NULL, "KheSolnReduceToPlaceholder: ke is NULL");
	  soln->invalid_error = KmlErrorCopy(ke, soln->outer_arena);
	  break;

	case KHE_SOLN_BASIC_PLACEHOLDER:
	case KHE_SOLN_WRITABLE_PLACEHOLDER:
	case KHE_SOLN_ORDINARY:
	default:

	  /* cannot reduce from invalid placeholder to these types */
          ReduceAbort(soln->type, soln_type);
	  break;
      }
      break;

    case KHE_SOLN_BASIC_PLACEHOLDER:

      switch( soln_type )
      {
	case KHE_SOLN_INVALID_PLACEHOLDER:

	  /* reduce from basic placeholder to invalid placeholder */
	  soln->type = KHE_SOLN_INVALID_PLACEHOLDER;
	  HnAssert(ke != NULL, "KheSolnReduceToPlaceholder: ke is NULL");
	  soln->invalid_error = KmlErrorCopy(ke, soln->outer_arena);
	  break;

	case KHE_SOLN_BASIC_PLACEHOLDER:

	  /* reduce from basic placeholder to basic placeholder */
	  /* nothing to do here */
	  break;

	case KHE_SOLN_WRITABLE_PLACEHOLDER:
	case KHE_SOLN_ORDINARY:
	default:

	  /* cannot reduce from basic placeholder to these types */
          ReduceAbort(soln->type, soln_type);
	  break;
      }
      break;

    case KHE_SOLN_WRITABLE_PLACEHOLDER:

      switch( soln_type )
      {
	case KHE_SOLN_INVALID_PLACEHOLDER:

	  /* reduce from writable placeholder to invalid placeholder */
	  soln->type = KHE_SOLN_INVALID_PLACEHOLDER;
	  HnAssert(ke != NULL, "KheSolnReduceToPlaceholder: ke is NULL");
	  soln->invalid_error = KmlErrorCopy(ke, soln->outer_arena);
	  break;

	case KHE_SOLN_BASIC_PLACEHOLDER:

	  /* reduce from writable placeholder to basic placeholder */
	  soln->type = KHE_SOLN_BASIC_PLACEHOLDER;
	  break;

	case KHE_SOLN_WRITABLE_PLACEHOLDER:

	  /* reduce from writable placeholder to writable placeholder */
	  /* nothing to do here */
	  break;

	case KHE_SOLN_ORDINARY:
	default:

	  /* cannot reduce from writable placeholder to these types */
          ReduceAbort(soln->type, soln_type);
	  break;
      }
      break;

    case KHE_SOLN_ORDINARY:

      switch( soln_type )
      {
	case KHE_SOLN_INVALID_PLACEHOLDER:

	  /* reduce from ordinary to invalid placeholder */
	  soln->type = KHE_SOLN_INVALID_PLACEHOLDER;
          KheSolnReduceFromOrdinaryToPlaceholder(soln);
	  HnAssert(ke != NULL, "KheSolnReduceToPlaceholder: ke is NULL");
	  soln->invalid_error = KmlErrorCopy(ke, soln->outer_arena);
	  break;

	case KHE_SOLN_BASIC_PLACEHOLDER:

	  /* reduce from ordinary to basic placeholder */
	  soln->type = KHE_SOLN_BASIC_PLACEHOLDER;
          KheSolnReduceFromOrdinaryToPlaceholder(soln);
	  break;

	case KHE_SOLN_WRITABLE_PLACEHOLDER:

	  /* reduce from ordinary to writable placeholder */
	  soln->write_only_soln =
	    KheSolnWriteOnlyFromSoln(soln, soln->outer_arena);
	  soln->type = KHE_SOLN_WRITABLE_PLACEHOLDER;
          KheSolnReduceFromOrdinaryToPlaceholder(soln);
	  break;

	case KHE_SOLN_ORDINARY:

	  /* reduce from ordinary to ordinary  */
	  /* nothing to do here */
	  break;

	default:

	  /* cannot reduce from ordinary to anything else */
          ReduceAbort(soln->type, soln_type);
	  break;
      }
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnIsPlaceholder(KHE_SOLN soln)                                 */
/*                                                                           */
/*  Return true if soln is a placeholder solution.                           */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheSolnIsPlaceholder(KHE_SOLN soln)
{
  return soln->inner_arena == NULL;
  ** return soln->placeholder; **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnIsInvalid(KHE_SOLN soln)                                     */
/*                                                                           */
/*  Return true if soln is invalid.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheSolnIsInvalid(KHE_SOLN soln)
{
  return soln->invalid_error != NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnReduceToInvalid(KHE_SOLN soln, KML_ERROR ke)                 */
/*                                                                           */
/*  Make soln invalid.  It may already be a placeholder, but it may not      */
/*  already be invalid.                                                      */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnReduceToInvalid(KHE_SOLN soln, KML_ERROR ke)
{
  HnAssert(soln->type != KHE_SOLN_INVALID_PLACEHOLDER,
    "KheSolnReduceToInvalid:  soln is already invalid");
  HnAssert(ke != NULL, "KheSolnReduceToInvalid:  ke == NULL");
  if( soln->type == KHE_SOLN_ORDINARY )
    KheSolnReduceToPlaceholder(soln, false);
  soln->type = KHE_SOLN_INVALID_PLACEHOLDER;
  soln->invalid_error = KmlErrorMake(soln->outer_arena,
    KmlErrorLineNum(ke), KmlErrorColNum(ke), "%s", KmlErrorString(ke));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnReduceToType(KHE_SOLN soln, KHE_SOLN_TYPE soln_type)         */
/*                                                                           */
/*  Reduce soln to soln_type.                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnReduceToType(KHE_SOLN soln, KHE_SOLN_TYPE soln_type)
{
  HnAssert(soln->type == KHE_SOLN_ORDINARY,
    "KheSolnReduceToType: soln does not have ordinary type initially");
  switch( soln_type )
  {
    case KHE_SOLN_ORDINARY:

      ** nothing to do here **
      break;

    case KHE_SOLN_BASIC_PLACEHOLDER:

      KheSolnReduceToPlaceholder(soln, false);
      break;

    case KHE_SOLN_WRITABLE_PLACEHOLDER:

      KheSolnReduceToPlaceholder(soln, true);
      break;

    case KHE_SOLN_INVALID_PLACEHOLDER:

      HnAbort("KheSolnReduceToType: soln_type is KHE_SOLN_INVALID_PLACEHOLDER");
      break;

    default:

      HnAbort("KheSolnReduceToType: unexpected soln_type (%d)", soln_type);
      break;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "top-level operations - traversing solutions - meets"          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAddMeet(KHE_SOLN soln, KHE_MEET meet)                        */
/*                                                                           */
/*  Add meet to soln, and set its soln and soln_index fields.                */
/*                                                                           */
/*****************************************************************************/

void KheSolnAddMeet(KHE_SOLN soln, KHE_MEET meet)
{
  KheMeetCheck(meet);
  KheMeetSetSoln(meet, soln);
  KheMeetSetSolnIndex(meet, HaArrayCount(soln->meets));
  HaArrayAddLast(soln->meets, meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnDeleteMeet(KHE_SOLN soln, KHE_MEET meet)                     */
/*                                                                           */
/*  Delete meet from soln.                                                   */
/*                                                                           */
/*****************************************************************************/

void KheSolnDeleteMeet(KHE_SOLN soln, KHE_MEET meet)
{
  int index;
  index = KheMeetSolnIndex(meet);
  HaArrayDeleteAndPlug(soln->meets, index);
  if( index < HaArrayCount(soln->meets) )
    KheMeetSetSolnIndex(HaArray(soln->meets, index), index);
  /* tmp = MArrayRemoveAndPlug(soln->meets, KheMeetSolnIndex(meet)); */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnMeetCount(KHE_SOLN soln)                                      */
/*                                                                           */
/*  Return the number of meets in soln.                                      */
/*                                                                           */
/*****************************************************************************/

int KheSolnMeetCount(KHE_SOLN soln)
{
  return HaArrayCount(soln->meets);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheSolnMeet(KHE_SOLN soln, int i)                               */
/*                                                                           */
/*  Return the i'th meet of soln.                                            */
/*                                                                           */
/*****************************************************************************/

KHE_MEET KheSolnMeet(KHE_SOLN soln, int i)
{
  HnAssert(0 <= i && i < HaArrayCount(soln->meets),
    "KheSolnMeet:  i (%d) out of range (0 .. %d)", i,
    HaArrayCount(soln->meets) - 1);
  KheMeetCheck(HaArray(soln->meets, i));
  return HaArray(soln->meets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_IN_SOLN KheSolnEventInSoln(KHE_SOLN soln, KHE_EVENT e)         */
/*                                                                           */
/*  Return the event in soln object of soln corresponding to e.              */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_IN_SOLN KheSolnEventInSoln(KHE_SOLN soln, KHE_EVENT e)
{
  return HaArray(soln->events_in_soln, KheEventIndex(e));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventMeetCount(KHE_SOLN soln, KHE_EVENT e)                        */
/*                                                                           */
/*  Return the number of meets for e in soln.                                */
/*                                                                           */
/*****************************************************************************/

int KheEventMeetCount(KHE_SOLN soln, KHE_EVENT e)
{
  KHE_EVENT_IN_SOLN es;
  es = HaArray(soln->events_in_soln, KheEventIndex(e));
  return KheEventInSolnMeetCount(es);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheEventMeet(KHE_SOLN soln, KHE_EVENT e, int i)                 */
/*                                                                           */
/*  Return the i'th meet of event e in soln.                                 */
/*                                                                           */
/*****************************************************************************/

KHE_MEET KheEventMeet(KHE_SOLN soln, KHE_EVENT e, int i)
{
  KHE_EVENT_IN_SOLN es;
  es = HaArray(soln->events_in_soln, KheEventIndex(e));
  return KheEventInSolnMeet(es, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "cycle meets"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheSolnTimeCycleMeet(KHE_SOLN soln, KHE_TIME t)                 */
/*                                                                           */
/*  Return the cycle meet covering t.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_MEET KheSolnTimeCycleMeet(KHE_SOLN soln, KHE_TIME t)
{
  return HaArray(soln->time_to_cycle_meet, KheTimeIndex(t));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnTimeCycleMeetOffset(KHE_SOLN soln, KHE_TIME t)                */
/*                                                                           */
/*  Return the offset of t in its cycle meet.                                */
/*                                                                           */
/*****************************************************************************/

int KheSolnTimeCycleMeetOffset(KHE_SOLN soln, KHE_TIME t)
{
  return HaArray(soln->time_to_cycle_offset, KheTimeIndex(t));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnTimeMayBeginBlock(KHE_SOLN soln, KHE_TIME t, int durn)       */
/*                                                                           */
/*  Return true if a meet of this durn may start at time t.                  */
/*                                                                           */
/*****************************************************************************/

static bool KheSolnTimeMayBeginBlock(KHE_SOLN soln, KHE_TIME t, int durn)
{
  KHE_MEET cycle_meet;  int cycle_offset;
  cycle_meet = KheSolnTimeCycleMeet(soln, t);
  cycle_offset = KheSolnTimeCycleMeetOffset(soln, t);
  return cycle_offset + durn <= KheMeetDuration(cycle_meet);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KhePackingTimeGroupMake(KHE_SOLN soln, int duration)      */
/*                                                                           */
/*  Make the packing time group for meets of soln of this duration.          */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP KhePackingTimeGroupMake(KHE_SOLN soln, int duration)
{
  KHE_TIME t;  int i;
  KheSolnTimeGroupBegin(soln);
  for( i = 0;  i < KheInstanceTimeCount(soln->instance);  i++ )
  {
    t = KheInstanceTime(soln->instance, i);
    if( KheSolnTimeMayBeginBlock(soln, t, duration) )
      KheSolnTimeGroupAddTime(soln, t);
  }
  return KheSolnTimeGroupEnd(soln);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheSolnPackingTimeGroup(KHE_SOLN soln, int duration)      */
/*                                                                           */
/*  Return the time group suitable for use as the domain of meets of         */
/*  the given duration.                                                      */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheSolnPackingTimeGroup(KHE_SOLN soln, int duration)
{
  HaArrayFill(soln->packing_time_groups, duration, NULL);
  if( HaArray(soln->packing_time_groups, duration - 1) == NULL )
    HaArrayPut(soln->packing_time_groups, duration - 1,
      KhePackingTimeGroupMake(soln, duration));
  return HaArray(soln->packing_time_groups, duration - 1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnCycleMeetSplit(KHE_SOLN soln, KHE_MEET meet1, KHE_MEET meet2)*/
/*                                                                           */
/*  Record the fact that a cycle meet has just split into meet1 and meet2.   */
/*                                                                           */
/*****************************************************************************/

void KheSolnCycleMeetSplit(KHE_SOLN soln, KHE_MEET meet1, KHE_MEET meet2)
{
  int i, ti;

  /* packing time groups are now out of date */
  HaArrayClear(soln->packing_time_groups);

  /* reset time_to_cycle_meet and time_to_cycle_offset */
  for( i = 0;  i < KheMeetDuration(meet2);  i++ )
  {
    ti = i + KheMeetAssignedTimeIndex(meet2);
    HaArrayPut(soln->time_to_cycle_meet, ti, meet2);
    HaArrayPut(soln->time_to_cycle_offset, ti, i);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnCycleMeetMerge(KHE_SOLN soln, KHE_MEET meet1, KHE_MEET meet2)*/
/*                                                                           */
/*  Record the fact that these cycle meets are merging.                      */
/*                                                                           */
/*****************************************************************************/

void KheSolnCycleMeetMerge(KHE_SOLN soln, KHE_MEET meet1, KHE_MEET meet2)
{
  int i, ti;

  /* packing time groups are now out of date */
  HaArrayClear(soln->packing_time_groups);

  /* reset time_to_cycle_meet and time_to_cycle_offset */
  for( i = 0;  i < KheMeetDuration(meet2);  i++ )
  {
    ti = i + KheMeetAssignedTimeIndex(meet2);
    HaArrayPut(soln->time_to_cycle_meet, ti, meet1);
    HaArrayPut(soln->time_to_cycle_offset, ti, i + KheMeetDuration(meet1));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "top-level operations - traversing solutions - tasks"          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAddTask(KHE_SOLN soln, KHE_TASK task)                        */
/*                                                                           */
/*  Add task to soln, including setting its soln_index.                      */
/*                                                                           */
/*****************************************************************************/

void KheSolnAddTask(KHE_SOLN soln, KHE_TASK task)
{
  KheTaskSetSolnIndex(task, HaArrayCount(soln->tasks));
  HaArrayAddLast(soln->tasks, task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnDeleteTask(KHE_SOLN soln, KHE_TASK task)                     */
/*                                                                           */
/*  Delete task from soln.  Yes, this code works even when task is at the    */
/*  end of the array, and even when it is the last element of the array.     */
/*                                                                           */
/*****************************************************************************/

void KheSolnDeleteTask(KHE_SOLN soln, KHE_TASK task)
{
  int index;
  index = KheTaskSolnIndex(task);
  HaArrayDeleteAndPlug(soln->tasks, index);
  /* tmp = MArrayRemoveAndPlug(soln->tasks, KheTaskSolnIndex(task)); */
  if( index < HaArrayCount(soln->tasks) )
    KheTaskSetSolnIndex(HaArray(soln->tasks, index), index);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnTaskCount(KHE_SOLN soln)                                      */
/*                                                                           */
/*  Return the number of tasks in soln.                                      */
/*                                                                           */
/*****************************************************************************/

int KheSolnTaskCount(KHE_SOLN soln)
{
  return HaArrayCount(soln->tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheSolnTask(KHE_SOLN soln, int i)                               */
/*                                                                           */
/*  Return the i'th task of soln.                                            */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheSolnTask(KHE_SOLN soln, int i)
{
  return HaArray(soln->tasks, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE_IN_SOLN KheSolnEventResourceInSoln(KHE_SOLN soln,     */
/*    KHE_EVENT_RESOURCE er)                                                 */
/*                                                                           */
/*  Return the event resource in soln object correponding to er.             */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_RESOURCE_IN_SOLN KheSolnEventResourceInSoln(KHE_SOLN soln,
  KHE_EVENT_RESOURCE er)
{
  KHE_EVENT_IN_SOLN es;
  es = KheSolnEventInSoln(soln, KheEventResourceEvent(er));
  return KheEventInSolnEventResourceInSoln(es, KheEventResourceEventIndex(er));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventResourceTaskCount(KHE_SOLN soln, KHE_EVENT_RESOURCE er)      */
/*                                                                           */
/*  Return the number of soln resources in soln corresponding to er.         */
/*                                                                           */
/*****************************************************************************/

int KheEventResourceTaskCount(KHE_SOLN soln, KHE_EVENT_RESOURCE er)
{
  KHE_EVENT_RESOURCE_IN_SOLN ers;
  ers = KheSolnEventResourceInSoln(soln, er);
  return KheEventResourceInSolnTaskCount(ers);
}


/*****************************************************************************/
/*                                                                           */
/* KHE_TASK KheEventResourceTask(KHE_SOLN soln, KHE_EVENT_RESOURCE er, int i)*/
/*                                                                           */
/*  Return the i'th solution resource corresponding to er in soln.           */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheEventResourceTask(KHE_SOLN soln, KHE_EVENT_RESOURCE er, int i)
{
  KHE_EVENT_RESOURCE_IN_SOLN ers;
  ers = KheSolnEventResourceInSoln(soln, er);
  return KheEventResourceInSolnTask(ers, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "top-level operations - traversing solutions - nodes"          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAddNode(KHE_SOLN soln, KHE_NODE node)                        */
/*                                                                           */
/*  Add node to soln, and set its soln and soln_index fields.                */
/*                                                                           */
/*****************************************************************************/

void KheSolnAddNode(KHE_SOLN soln, KHE_NODE node)
{
  KheNodeSetSoln(node, soln);
  KheNodeSetSolnIndex(node, HaArrayCount(soln->nodes));
  HaArrayAddLast(soln->nodes, node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnDeleteNode(KHE_SOLN soln, KHE_NODE node)                     */
/*                                                                           */
/*  Delete node from soln.                                                   */
/*                                                                           */
/*****************************************************************************/

void KheSolnDeleteNode(KHE_SOLN soln, KHE_NODE node)
{
  int index;
  /* tmp = MArrayRemoveAndPlug(soln->nodes, KheNodeSolnIndex(node)); */
  index = KheNodeSolnIndex(node);
  HaArrayDeleteAndPlug(soln->nodes, index);
  if( index < HaArrayCount(soln->nodes) )
    KheNodeSetSolnIndex(HaArray(soln->nodes, index), index);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnNodeCount(KHE_SOLN soln)                                      */
/*                                                                           */
/*  Return the number of nodes in soln.                                      */
/*                                                                           */
/*****************************************************************************/

int KheSolnNodeCount(KHE_SOLN soln)
{
  return HaArrayCount(soln->nodes);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE KheSolnNode(KHE_SOLN soln, int i)                               */
/*                                                                           */
/*  Return the i'th solution node of soln.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_NODE KheSolnNode(KHE_SOLN soln, int i)
{
  return HaArray(soln->nodes, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "top-level operations - traversing solutions - taskings"       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAddTasking(KHE_SOLN soln, KHE_TASKING tasking,               */
/*    int *index_in_soln)                                                    */
/*                                                                           */
/*  Add tasking to soln, returning its index.                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnAddTasking(KHE_SOLN soln, KHE_TASKING tasking)
{
  KheTaskingSetSolnIndex(tasking, HaArrayCount(soln->taskings));
  HaArrayAddLast(soln->taskings, tasking);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnDeleteTasking(KHE_SOLN soln, KHE_TASKING tasking)            */
/*                                                                           */
/*  Delete tasking from soln.                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnDeleteTasking(KHE_SOLN soln, KHE_TASKING tasking)
{
  int index;
  index = KheTaskingSolnIndex(tasking);
  HaArrayDeleteAndPlug(soln->taskings, index);
  if( index < HaArrayCount(soln->taskings) )
    KheTaskingSetSolnIndex(HaArray(soln->taskings, index), index);
  ** ***
  KHE_TASKING tmp;
  tmp = MArrayRemoveAndPlug(soln->taskings, KheTaskingSolnIndex(tasking));
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnTaskingCount(KHE_SOLN soln)                                   */
/*                                                                           */
/*  Return the number of taskings in soln.                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheSolnTaskingCount(KHE_SOLN soln)
{
  return HaArrayCount(soln->taskings);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASKING KheSolnTasking(KHE_SOLN soln, int i)                         */
/*                                                                           */
/*  Return the i'th tasking of soln.                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_TASKING KheSolnTasking(KHE_SOLN soln, int i)
{
  return HaArray(soln->taskings, i);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "complete representation and preassignment conversion"         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnMakeCompleteRepresentation(KHE_SOLN soln,                    */
/*    KHE_EVENT *problem_event)                                              */
/*                                                                           */
/*  Ensure that soln is a complete representation of its instance, by        */
/*  adding one meet to each meet set whose total duration is less than       */
/*  the duration of its event, and adding tasks to all meets as required.    */
/*                                                                           */
/*  Return true if successful, or, if prevented by the presence of an        */
/*  event whose meets' durations are already too great, set *problem_event   */
/*  to the first such event and return false.                                */
/*                                                                           */
/*****************************************************************************/

bool KheSolnMakeCompleteRepresentation(KHE_SOLN soln,
  KHE_EVENT *problem_event)
{
  int i;  KHE_EVENT_IN_SOLN es;
  HaArrayForEach(soln->events_in_soln, es, i)
    if( !KheEventInSolnMakeCompleteRepresentation(es, problem_event) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAssignPreassignedTimes(KHE_SOLN soln)                        */
/*                                                                           */
/*  Assign preassigned times to all the meets of soln that have them and     */
/*  are not already assigned.                                                */
/*                                                                           */
/*****************************************************************************/

void KheSolnAssignPreassignedTimes(KHE_SOLN soln)
{
  KHE_MEET meet;  int i;  KHE_TIME time;
  if( DEBUG9 )
    fprintf(stderr, "[ KheSolnAssignPreassignedTimes(soln)\n");
  HaArrayForEach(soln->meets, meet, i)
    if( KheMeetAsst(meet) == NULL && KheMeetIsPreassigned(meet, &time) )
    {
      if( !KheMeetAssignTime(meet, time) )
	HnAbort("KheSolnAssignPreassignedTimes failed to assign");
    }
  if( DEBUG9 )
    fprintf(stderr, "] KheSolnAssignPreassignedTimes returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAssignPreassignedResources(KHE_SOLN soln,                    */
/*    KHE_RESOURCE_TYPE rt)                                                  */
/*                                                                           */
/*  Assign preassigned resources to those tasks that are not already         */
/*  assigned and have them.  If rt != NULL, this applies only to tasks       */
/*  of resource type rt.                                                     */
/*                                                                           */
/*  If as_in_event_resource is true, a task is considered to have a          */
/*  preassigned resource if it is derived from an event resource with        */
/*  a preassigned resource.  If as_in_event_resource is false, a task        */
/*  is considered to have a preassigned resource if its domain contains      */
/*  exactly one element.                                                     */
/*                                                                           */
/*****************************************************************************/

void KheSolnAssignPreassignedResources(KHE_SOLN soln, KHE_RESOURCE_TYPE rt)
{
  KHE_MEET meet;  int i;
  HaArrayForEach(soln->meets, meet, i)
    KheMeetAssignPreassignedResources(meet, rt);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "solution time groups"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnTimeGroupBegin(KHE_SOLN soln)                                */
/*                                                                           */
/*  Begin the construction of a soln time group.                             */
/*                                                                           */
/*****************************************************************************/

void KheSolnTimeGroupBegin(KHE_SOLN soln)
{
  HnAssert(!soln->time_sset_building,
    "KheSolnTimeGroupBegin: time group already under construction");
  SSetInit(soln->time_sset, soln->inner_arena);
  soln->time_sset_building = true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnTimeGroupAddTime(KHE_SOLN soln, KHE_TIME time)               */
/*                                                                           */
/*  Add a time to the time group currently being constructed.                */
/*                                                                           */
/*****************************************************************************/

void KheSolnTimeGroupAddTime(KHE_SOLN soln, KHE_TIME time)
{
  HnAssert(soln->time_sset_building,
    "KheSolnTimeGroupAddTime: time group not under construction");
  SSetInsert(soln->time_sset, KheTimeIndex(time));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnTimeGroupSubTime(KHE_SOLN soln, KHE_TIME time)               */
/*                                                                           */
/*  Take away time from the time group currently being constructed.          */
/*                                                                           */
/*****************************************************************************/

void KheSolnTimeGroupSubTime(KHE_SOLN soln, KHE_TIME time)
{
  HnAssert(soln->time_sset_building,
    "KheSolnTimeGroupSubTime: time group not under construction");
  SSetDelete(soln->time_sset, KheTimeIndex(time));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnTimeGroupUnion(KHE_SOLN soln, KHE_TIME_GROUP tg2)            */
/*                                                                           */
/*  Union a time group to the time group currently being constructed.        */
/*                                                                           */
/*****************************************************************************/

void KheSolnTimeGroupUnion(KHE_SOLN soln, KHE_TIME_GROUP tg2)
{
  HnAssert(soln->time_sset_building,
    "KheSolnTimeGroupUnionTimeGroup: time group not under construction");
  SSetUnion(soln->time_sset, *KheTimeGroupTimeSet(tg2));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnTimeGroupIntersect(KHE_SOLN soln,KHE_TIME_GROUP tg2)         */
/*                                                                           */
/*  Intersect a time group to the time group currently being constructed.    */
/*                                                                           */
/*****************************************************************************/

void KheSolnTimeGroupIntersect(KHE_SOLN soln, KHE_TIME_GROUP tg2)
{
  HnAssert(soln->time_sset_building,
    "KheSolnTimeGroupIntersect: time group not under construction");
  SSetIntersect(soln->time_sset, *KheTimeGroupTimeSet(tg2));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnTimeGroupDifference(KHE_SOLN soln, KHE_TIME_GROUP tg2)       */
/*                                                                           */
/*  Take away tg2 from the time group currently being constructed.           */
/*                                                                           */
/*****************************************************************************/

void KheSolnTimeGroupDifference(KHE_SOLN soln, KHE_TIME_GROUP tg2)
{
  HnAssert(soln->time_sset_building,
    "KheSolnTimeGroupDifference: time group not under construction");
  SSetDifference(soln->time_sset, *KheTimeGroupTimeSet(tg2));
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheSolnTimeGroupEnd(KHE_SOLN soln)                        */
/*                                                                           */
/*  End the construction of a time group and return it.                      */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheSolnTimeGroupEnd(KHE_SOLN soln)
{
  KHE_TIME_GROUP res;
  HnAssert(soln->time_sset_building,
    "KheSolnTimeGroupEnd: time group not under construction");
  soln->time_sset_building = false;
  if( SSetTableRetrieve(soln->time_sset_table, soln->time_sset, (void **)&res) )
    return res;
  res = KheTimeGroupMakeAndFinalize(soln->instance, KHE_TIME_GROUP_KIND_SOLN,
    NULL, NULL, &soln->time_sset, /* soln, */ true, KheSolnArena(soln));
  SSetImplTableInsert(soln->time_sset_table, KheTimeGroupTimeSet(res),
    (void *) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "solution resource groups"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnResourceGroupBegin(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,      */
/*    void *impl)                                                            */
/*                                                                           */
/*  Begin the construction of a soln resource group.                         */
/*                                                                           */
/*****************************************************************************/

void KheSolnResourceGroupBegin(KHE_SOLN soln, KHE_RESOURCE_TYPE rt)
{
  if( DEBUG10 )
    fprintf(stderr, "[ KheSolnResourceGroupBegin(soln, rt)\n");
  HnAssert(soln->resource_set_rt == NULL,
    "KheSolnResourceGroupBegin: resource group already under construction");
  HnAssert(rt != NULL, "KheSolnResourceGroupBegin: rt is NULL");
  KheSetInit(soln->resource_set, soln->inner_arena);
  soln->resource_set_rt = rt;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnResourceGroupAddResource(KHE_SOLN soln, KHE_RESOURCE r)      */
/*                                                                           */
/*  Add a r to the resource group currently being constructed.               */
/*                                                                           */
/*****************************************************************************/

void KheSolnResourceGroupAddResource(KHE_SOLN soln, KHE_RESOURCE r)
{
  HnAssert(soln->resource_set_rt != NULL,
    "KheSolnResourceGroupAddResource: resource group not under construction");
  HnAssert(KheResourceResourceType(r) == soln->resource_set_rt,
    "KheSolnResourceGroupAddResource: resource has wrong resource type");
  KheSetInsert(soln->resource_set, KheResourceInstanceIndex(r));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnResourceGroupSubResource(KHE_SOLN soln, KHE_RESOURCE r)      */
/*                                                                           */
/*  Take away r from the resource group currently being constructed.         */
/*                                                                           */
/*****************************************************************************/

void KheSolnResourceGroupSubResource(KHE_SOLN soln, KHE_RESOURCE r)
{
  HnAssert(soln->resource_set_rt != NULL,
    "KheSolnResourceGroupSubResource: resource group not under construction");
  HnAssert(KheResourceResourceType(r) == soln->resource_set_rt,
    "KheSolnResourceGroupSubResource: resource has wrong resource type");
  KheSetDelete(soln->resource_set, KheResourceInstanceIndex(r));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnResourceGroupUnion(KHE_SOLN soln, KHE_RESOURCE_GROUP rg2)    */
/*                                                                           */
/*  Add a resource group to the resource group currently being constructed.  */
/*                                                                           */
/*****************************************************************************/

void KheSolnResourceGroupUnion(KHE_SOLN soln, KHE_RESOURCE_GROUP rg2)
{
  HnAssert(soln->resource_set_rt != NULL,
    "KheSolnResourceGroupUnionResourceGroup: "
    "resource group not under construction");
  HnAssert(KheResourceGroupResourceType(rg2) == soln->resource_set_rt,
    "KheSolnResourceGroupUnion: resource group has wrong resource type");
  KheSetUnion(soln->resource_set, *KheResourceGroupKheSet(rg2));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnResourceGroupIntersect(KHE_SOLN soln, KHE_RESOURCE_GROUP rg2)*/
/*                                                                           */
/*  Intersect a resource group to the resource group currently being         */
/*  constructed.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheSolnResourceGroupIntersect(KHE_SOLN soln, KHE_RESOURCE_GROUP rg2)
{
  HnAssert(soln->resource_set_rt != NULL,
    "KheSolnResourceGroupIntersect: resource group not under construction");
  HnAssert(KheResourceGroupResourceType(rg2) == soln->resource_set_rt,
    "KheSolnResourceGroupIntersect: resource group has wrong resource type");
  KheSetIntersect(soln->resource_set, *KheResourceGroupKheSet(rg2));
}


/*****************************************************************************/
/*                                                                           */
/* void KheSolnResourceGroupDifference(KHE_SOLN soln, KHE_RESOURCE_GROUP rg2)*/
/*                                                                           */
/*  Take away rg2 from the resource group currently being constructed.       */
/*                                                                           */
/*****************************************************************************/

void KheSolnResourceGroupDifference(KHE_SOLN soln, KHE_RESOURCE_GROUP rg2)
{
  HnAssert(soln->resource_set_rt != NULL,
    "KheSolnResourceGroupDifference: resource group not under construction");
  HnAssert(KheResourceGroupResourceType(rg2) == soln->resource_set_rt,
    "KheSolnResourceGroupDifference: resource group has wrong resource type");
  KheSetDifference(soln->resource_set, *KheResourceGroupKheSet(rg2));
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheSolnResourceGroupEnd(KHE_SOLN soln)                */
/*                                                                           */
/*  End the construction of a resource group and return it.                  */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheSolnResourceGroupEnd(KHE_SOLN soln)
{
  KHE_RESOURCE_GROUP res;  KHE_RESOURCE_TYPE rt;  int i, index;
  HnAssert(soln->resource_set_rt != NULL,
    "KheSolnResourceGroupEnd: resource group not under construction");
  rt = soln->resource_set_rt;
  soln->resource_set_rt = NULL;
  if( KheSetTableRetrieve(soln->resource_set_table, soln->resource_set,
	(void **) &res) )
    return res;
  res = KheResourceGroupMakeInternal(rt, KHE_RESOURCE_GROUP_TYPE_SOLN,
    soln, NULL, NULL /* , final_lset */);
  for( i = 0;  i < KheSetCount(soln->resource_set);  i++ )
  {
    index = KheSetGet(soln->resource_set, i);
    KheResourceGroupAddResourceInternal(res,
      KheInstanceResource(soln->instance, index));
  }
  KheResourceGroupFinalize(res);
  KheSetImplTableInsert(soln->resource_set_table,
    KheResourceGroupKheSet(res), (void *) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "solution event groups"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnEventGroupBegin(KHE_SOLN soln)                               */
/*                                                                           */
/*  Begin the construction of a soln event group.                            */
/*                                                                           */
/*****************************************************************************/

void KheSolnEventGroupBegin(KHE_SOLN soln)
{
  HnAssert(soln->curr_event_group == NULL,
    "KheSolnEventGroupBegin: event group already under construction");
  soln->curr_event_group = KheEventGroupMakeInternal(soln->instance,
    KHE_EVENT_GROUP_TYPE_SOLN, soln, KHE_EVENT_GROUP_KIND_ORDINARY, NULL, NULL);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnEventGroupAddEvent(KHE_SOLN soln, KHE_EVENT e)               */
/*                                                                           */
/*  Add a r to the event group currently being constructed.                  */
/*                                                                           */
/*****************************************************************************/

void KheSolnEventGroupAddEvent(KHE_SOLN soln, KHE_EVENT e)
{
  HnAssert(soln->curr_event_group != NULL,
    "KheSolnEventGroupAddEvent: no event group under construction");
  KheEventGroupUnionInternal(soln->curr_event_group,
    KheEventSingletonEventGroup(e));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnEventGroupSubEvent(KHE_SOLN soln, KHE_EVENT e)               */
/*                                                                           */
/*  Take away r from the event group currently being constructed.            */
/*                                                                           */
/*****************************************************************************/

void KheSolnEventGroupSubEvent(KHE_SOLN soln, KHE_EVENT e)
{
  HnAssert(soln->curr_event_group != NULL,
    "KheSolnEventGroupSubEvent: no event group under construction");
  KheEventGroupDifferenceInternal(soln->curr_event_group,
    KheEventSingletonEventGroup(e));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnEventGroupUnion(KHE_SOLN soln, KHE_EVENT_GROUP eg2)          */
/*                                                                           */
/*  Add a event group to the event group currently being constructed.        */
/*                                                                           */
/*****************************************************************************/

void KheSolnEventGroupUnion(KHE_SOLN soln, KHE_EVENT_GROUP eg2)
{
  HnAssert(soln->curr_event_group != NULL,
    "KheSolnEventGroupUnion: no event group under construction");
  KheEventGroupUnionInternal(soln->curr_event_group, eg2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnEventGroupIntersect(KHE_SOLN soln, KHE_EVENT_GROUP eg2)      */
/*                                                                           */
/*  Intersect a event group to the event group currently being constructed.  */
/*                                                                           */
/*****************************************************************************/

void KheSolnEventGroupIntersect(KHE_SOLN soln, KHE_EVENT_GROUP eg2)
{
  HnAssert(soln->curr_event_group != NULL,
    "KheSolnEventGroupIntersect: no event group under construction");
  KheEventGroupIntersectInternal(soln->curr_event_group, eg2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnEventGroupDifference(KHE_SOLN soln, KHE_EVENT_GROUP eg2)     */
/*                                                                           */
/*  Take away eg2 from the event group currently being constructed.          */
/*                                                                           */
/*****************************************************************************/

void KheSolnEventGroupDifference(KHE_SOLN soln, KHE_EVENT_GROUP eg2)
{
  HnAssert(soln->curr_event_group != NULL,
   "KheSolnEventGroupDifference: no event group under construction");
  KheEventGroupDifferenceInternal(soln->curr_event_group, eg2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAddEventGroup(KHE_SOLN soln, KHE_EVENT_GROUP eg)             */
/*                                                                           */
/*  Add eg to soln.                                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnAddEventGroup(KHE_SOLN soln, KHE_EVENT_GROUP eg)
{
  HaArrayAddLast(soln->event_groups, eg);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_GROUP KheSolnEventGroupEnd(KHE_SOLN soln)                      */
/*                                                                           */
/*  End the construction of a event group and return it.                     */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_GROUP KheSolnEventGroupEnd(KHE_SOLN soln)
{
  KHE_EVENT_GROUP res;
  HnAssert(soln->curr_event_group != NULL,
    "KheSolnEventGroupSubEvent: no event group under construction");
  res = soln->curr_event_group;
  KheEventGroupFinalize(res);
  soln->curr_event_group = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "marks"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMarkBegin(KHE_SOLN soln, KHE_MARK mark,                      */
/*    int *index, int *start_pos)                                            */
/*                                                                           */
/*  This function informs soln that mark is beginning.  It adds mark to      */
/*  the end of its mark stack, and sets *index and *start_pos to the         */
/*  correct values for the new mark.                                         */
/*                                                                           */
/*****************************************************************************/

void KheSolnMarkBegin(KHE_SOLN soln, KHE_MARK mark,
  int *index, int *start_pos)
{
  /* set *index and *start_pos */
  *index = HaArrayCount(soln->marks);
  *start_pos = KhePathCount(soln->main_path);

  /* add mark to the mark stack */
  HaArrayAddLast(soln->marks, mark);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMarkEnd(KHE_SOLN soln, KHE_MARK mark)                        */
/*                                                                           */
/*  This function informs soln that a mark is ending.  It checks that this   */
/*  mark is on top of the stack and removes it from the mark stack.          */
/*                                                                           */
/*****************************************************************************/

void KheSolnMarkEnd(KHE_SOLN soln, KHE_MARK mark)
{
  KHE_MARK mark2;  int pos;
  HnAssert(HaArrayCount(soln->marks) > 0, "KheMarkEnd: mark already ended");
  mark2 = HaArrayLastAndDelete(soln->marks);
  HnAssert(mark2 == mark, HaArrayContains(soln->marks, mark, &pos) ?
    "KheMarkEnd: wrong mark has ended" : "KheMarkEnd: mark already ended");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnMarkOnTop(KHE_SOLN soln, KHE_MARK mark)                      */
/*                                                                           */
/*  Return true if mark is on top of soln stack.                             */
/*                                                                           */
/*****************************************************************************/

bool KheSolnMarkOnTop(KHE_SOLN soln, KHE_MARK mark)
{
  return HaArrayCount(soln->marks) > 0 && HaArrayLast(soln->marks) == mark;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "paths"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_PATH KheSolnMainPath(KHE_SOLN soln)                                  */
/*                                                                           */
/*  Return the main path.                                                    */
/*                                                                           */
/*****************************************************************************/

KHE_PATH KheSolnMainPath(KHE_SOLN soln)
{
  return soln->main_path;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "path operation loading - meets"                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpMeetSetBack(KHE_SOLN soln, KHE_MEET meet,                  */
/*    void *old_back, void *new_back)                                        */
/*                                                                           */
/*  Inform soln that a call to KheMeetSetBack has occurred.                  */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpMeetSetBack(KHE_SOLN soln, KHE_MEET meet,
  void *old_back, void *new_back)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpMeetSetBack(soln->main_path, meet, old_back, new_back);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpMeetAdd(KHE_SOLN soln, KHE_MEET meet, int duration,        */
/*    KHE_EVENT e)                                                           */
/*                                                                           */
/*  Inform soln that a call to KheMeetKernelAdd has occurred.                */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpMeetAdd(KHE_SOLN soln, KHE_MEET meet, int duration,
  KHE_EVENT e)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpMeetAdd(soln->main_path, meet, duration, e);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpMeetDelete(KHE_SOLN soln, KHE_MEET meet, int duration,     */
/*    KHE_EVENT e)                                                           */
/*                                                                           */
/*  Inform soln that a call to KheMeetKernelDelete has occurred.             */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpMeetDelete(KHE_SOLN soln, KHE_MEET meet, int duration,
  KHE_EVENT e)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpMeetDelete(soln->main_path, meet, duration, e);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpMeetSplit(KHE_SOLN soln, KHE_MEET meet1, KHE_MEET meet2,   */
/*    int durn1)                                                             */
/*                                                                           */
/*  Inform soln that a call to KheMeetSplit has occurred.                    */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpMeetSplit(KHE_SOLN soln, KHE_MEET meet1, KHE_MEET meet2,
  int durn1)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpMeetSplit(soln->main_path, meet1, meet2, durn1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpMeetMerge(KHE_SOLN soln, KHE_MEET meet1, KHE_MEET meet2,   */
/*    int durn1)                                                             */
/*                                                                           */
/*  Inform soln that a call to KheMeetMerge has occurred.                    */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpMeetMerge(KHE_SOLN soln, KHE_MEET meet1, KHE_MEET meet2,
  int durn1)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpMeetMerge(soln->main_path, meet1, meet2, durn1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpMeetMove(KHE_SOLN soln, KHE_MEET meet,                     */
/*    KHE_MEET old_target_meet, int old_target_offset,                       */
/*    KHE_MEET new_target_meet, int new_target_offset)                       */
/*                                                                           */
/*  Inform soln that a call to KheMeetMove has occurred.                     */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpMeetMove(KHE_SOLN soln, KHE_MEET meet,
  KHE_MEET old_target_meet, int old_target_offset,
  KHE_MEET new_target_meet, int new_target_offset)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpMeetMove(soln->main_path, meet, old_target_meet,
      old_target_offset, new_target_meet, new_target_offset);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpMeetSetAutoDomain(KHE_SOLN soln, KHE_MEET meet,            */
/*    bool automatic)                                                        */
/*                                                                           */
/*  Inform soln that a call to KheMeetSetAutoDomain has occurred.            */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpMeetSetAutoDomain(KHE_SOLN soln, KHE_MEET meet, bool automatic)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpMeetSetAutoDomain(soln->main_path, meet, automatic);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpMeetAssignFix(KHE_SOLN soln, KHE_MEET meet)                */
/*                                                                           */
/*  Inform soln that a call to KheMeetAssignFix has occurred.                */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpMeetAssignFix(KHE_SOLN soln, KHE_MEET meet)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpMeetAssignFix(soln->main_path, meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpMeetAssignUnFix(KHE_SOLN soln, KHE_MEET meet)              */
/*                                                                           */
/*  Inform soln that a call to KheMeetAssignUnFix has occurred.              */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpMeetAssignUnFix(KHE_SOLN soln, KHE_MEET meet)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpMeetAssignUnFix(soln->main_path, meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpMeetAddMeetBound(KHE_SOLN soln,                            */
/*    KHE_MEET meet, KHE_MEET_BOUND mb)                                      */
/*                                                                           */
/*  Inform soln that a call to KheMeetAddMeetBound has occurred.             */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpMeetAddMeetBound(KHE_SOLN soln,
  KHE_MEET meet, KHE_MEET_BOUND mb)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpMeetAddMeetBound(soln->main_path, meet, mb);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpMeetDeleteMeetBound(KHE_SOLN soln,                         */
/*    KHE_MEET meet, KHE_MEET_BOUND mb)                                      */
/*                                                                           */
/*  Inform soln that a call to KheMeetDeleteMeetBound has occurred.          */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpMeetDeleteMeetBound(KHE_SOLN soln,
  KHE_MEET meet, KHE_MEET_BOUND mb)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpMeetDeleteMeetBound(soln->main_path, meet, mb);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "path operation loading - meet bounds"                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpMeetBoundAdd(KHE_SOLN soln, KHE_MEET_BOUND mb,             */
/*    bool occupancy, KHE_TIME_GROUP dft_tg)                                 */
/*                                                                           */
/*  Inform soln that a call to KheMeetBoundMake has occurred.                */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpMeetBoundAdd(KHE_SOLN soln, KHE_MEET_BOUND mb,
  bool occupancy, KHE_TIME_GROUP dft_tg)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpMeetBoundAdd(soln->main_path, mb, occupancy, dft_tg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpMeetBoundDelete(KHE_SOLN soln, KHE_MEET_BOUND mb,          */
/*    bool occupancy, KHE_TIME_GROUP dft_tg)                                 */
/*                                                                           */
/*  Inform soln that a call to KheMeetBoundDelete has occurred.              */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpMeetBoundDelete(KHE_SOLN soln, KHE_MEET_BOUND mb,
  bool occupancy, KHE_TIME_GROUP dft_tg)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpMeetBoundDelete(soln->main_path, mb, occupancy, dft_tg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpMeetBoundAddTimeGroup(KHE_SOLN soln, KHE_MEET_BOUND mb,    */
/*    int duration, KHE_TIME_GROUP tg)                                       */
/*                                                                           */
/*  Inform soln that a call to KheMeetBoundAddTimeGroup has occurred.        */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpMeetBoundAddTimeGroup(KHE_SOLN soln, KHE_MEET_BOUND mb,
  int duration, KHE_TIME_GROUP tg)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpMeetBoundAddTimeGroup(soln->main_path, mb, duration, tg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpMeetBoundDeleteTimeGroup(KHE_SOLN soln, KHE_MEET_BOUND mb, */
/*    int duration, KHE_TIME_GROUP tg)                                       */
/*                                                                           */
/*  Inform soln that a call to KheMeetBoundDeleteTimeGroup has occurred.     */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpMeetBoundDeleteTimeGroup(KHE_SOLN soln, KHE_MEET_BOUND mb,
  int duration, KHE_TIME_GROUP tg)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpMeetBoundDeleteTimeGroup(soln->main_path, mb, duration, tg);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "path operation loading - tasks"                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpTaskSetBack(KHE_SOLN soln, KHE_TASK task,                  */
/*    void *old_back, void *new_back)                                        */
/*                                                                           */
/*  Inform soln that a call to KheTaskSetBack has occurred.                  */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpTaskSetBack(KHE_SOLN soln, KHE_TASK task,
  void *old_back, void *new_back)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpTaskSetBack(soln->main_path, task, old_back, new_back);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpTaskAdd(KHE_SOLN soln, KHE_TASK task,                      */
/*    KHE_RESOURCE_TYPE rt, KHE_MEET meet, KHE_EVENT_RESOURCE er)            */
/*                                                                           */
/*  Inform soln that a call to KheTaskAdd has occurred.                      */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpTaskAdd(KHE_SOLN soln, KHE_TASK task,
  KHE_RESOURCE_TYPE rt, KHE_MEET meet, KHE_EVENT_RESOURCE er)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpTaskAdd(soln->main_path, task, rt, meet, er);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpTaskDelete(KHE_SOLN soln, KHE_TASK task,                   */
/*    KHE_RESOURCE_TYPE rt, KHE_MEET meet, KHE_EVENT_RESOURCE er)            */
/*                                                                           */
/*  Inform soln that a call to KheTaskDelete has occurred.                   */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpTaskDelete(KHE_SOLN soln, KHE_TASK task,
  KHE_RESOURCE_TYPE rt, KHE_MEET meet, KHE_EVENT_RESOURCE er)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpTaskDelete(soln->main_path, task, rt, meet, er);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpTaskSplit(KHE_SOLN soln, KHE_TASK task1,                   */
/*    KHE_TASK task2, int durn1, KHE_MEET meet2)                             */
/*                                                                           */
/*  Inform soln that a task split with these attributes is occurring.        */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpTaskSplit(KHE_SOLN soln, KHE_TASK task1,
  KHE_TASK task2, int durn1, KHE_MEET meet2)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpTaskSplit(soln->main_path, task1, task2, durn1, meet2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpTaskMerge(KHE_SOLN soln, KHE_TASK task1,                   */
/*    KHE_TASK task2, int durn1, KHE_MEET meet2)                             */
/*                                                                           */
/*  Inform soln that a task merge with these attributes is occurring.        */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpTaskMerge(KHE_SOLN soln, KHE_TASK task1,
  KHE_TASK task2, int durn1, KHE_MEET meet2)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpTaskMerge(soln->main_path, task1, task2, durn1, meet2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpTaskMove(KHE_SOLN soln, KHE_TASK task,                     */
/*    KHE_TASK old_target_task, KHE_TASK new_target_task)                    */
/*                                                                           */
/*  Inform soln that a call to KheTaskMove has occurred.                     */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpTaskMove(KHE_SOLN soln, KHE_TASK task,
  KHE_TASK old_target_task, KHE_TASK new_target_task)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpTaskMove(soln->main_path, task, old_target_task, new_target_task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpTaskAssignFix(KHE_SOLN soln, KHE_TASK task)                */
/*                                                                           */
/*  Inform soln that a call to KheTaskAssignFix has occurred.                */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpTaskAssignFix(KHE_SOLN soln, KHE_TASK task)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpTaskAssignFix(soln->main_path, task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpTaskAssignUnFix(KHE_SOLN soln, KHE_TASK task)              */
/*                                                                           */
/*  Inform soln that a call to KheTaskAssignUnFix has occurred.              */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpTaskAssignUnFix(KHE_SOLN soln, KHE_TASK task)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpTaskAssignUnFix(soln->main_path, task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpTaskAddTaskBound(KHE_SOLN soln,                            */
/*    KHE_TASK task, KHE_TASK_BOUND tb)                                      */
/*                                                                           */
/*  Inform soln that a call to KheTaskAddTaskBound has occurred.             */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpTaskAddTaskBound(KHE_SOLN soln,
  KHE_TASK task, KHE_TASK_BOUND tb)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpTaskAddTaskBound(soln->main_path, task, tb);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpTaskDeleteTaskBound(KHE_SOLN soln,                         */
/*    KHE_TASK task, KHE_TASK_BOUND tb)                                      */
/*                                                                           */
/*  Inform soln that a call to KheTaskDeleteTaskBound has occurred.          */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpTaskDeleteTaskBound(KHE_SOLN soln,
  KHE_TASK task, KHE_TASK_BOUND tb)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpTaskDeleteTaskBound(soln->main_path, task, tb);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "path operation loading - task bounds"                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpTaskBoundAdd(KHE_SOLN soln, KHE_TASK_BOUND tb,             */
/*    KHE_TASK_BOUND_GROUP tbg, KHE_TASK task, KHE_RESOURCE_GROUP rg)        */
/*                                                                           */
/*  Inform soln that a call to KheTaskBoundAdd has occurred.                 */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpTaskBoundAdd(KHE_SOLN soln, KHE_TASK_BOUND tb,
  KHE_RESOURCE_GROUP rg)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpTaskBoundAdd(soln->main_path, tb, rg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpTaskBoundDelete(KHE_SOLN soln, KHE_TASK_BOUND tb,          */
/*    KHE_TASK_BOUND_GROUP tbg, KHE_TASK task, KHE_RESOURCE_GROUP rg)        */
/*                                                                           */
/*  Inform soln that a call to KheTaskBoundDelete has occurred.              */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpTaskBoundDelete(KHE_SOLN soln, KHE_TASK_BOUND tb,
  KHE_RESOURCE_GROUP rg)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpTaskBoundDelete(soln->main_path, tb, rg);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "path operation loading - nodes"                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpNodeSetBack(KHE_SOLN soln, KHE_NODE node,                  */
/*    void *old_back, void *new_back)                                        */
/*                                                                           */
/*  Inform soln that a call to KheNodeSetBack has occurred.                  */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpNodeSetBack(KHE_SOLN soln, KHE_NODE node,
  void *old_back, void *new_back)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpNodeSetBack(soln->main_path, node, old_back, new_back);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpNodeAdd(KHE_SOLN soln, KHE_NODE node)                      */
/*                                                                           */
/*  Inform soln that a call to KheNodeAdd has occurred.                      */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpNodeAdd(KHE_SOLN soln, KHE_NODE node)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpNodeAdd(soln->main_path, node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpNodeDelete(KHE_SOLN soln, KHE_NODE node)                   */
/*                                                                           */
/*  Inform soln that a call to KheNodeDelete has occurred.                   */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpNodeDelete(KHE_SOLN soln, KHE_NODE node)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpNodeDelete(soln->main_path, node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpNodeAddParent(KHE_SOLN soln,                               */
/*    KHE_NODE child_node, KHE_NODE parent_node)                             */
/*                                                                           */
/*  Inform soln that a call to KheNodeAddParent has occurred.                */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpNodeAddParent(KHE_SOLN soln,
  KHE_NODE child_node, KHE_NODE parent_node)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpNodeAddParent(soln->main_path, child_node, parent_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpNodeDeleteParent(KHE_SOLN soln,                            */
/*    KHE_NODE child_node, KHE_NODE parent_node)                             */
/*                                                                           */
/*  Inform soln that a call to KheNodeDeleteParent has occurred.             */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpNodeDeleteParent(KHE_SOLN soln,
  KHE_NODE child_node, KHE_NODE parent_node)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpNodeDeleteParent(soln->main_path, child_node, parent_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpNodeSwapChildNodesAndLayers(KHE_SOLN soln,                 */
/*    KHE_NODE node1, KHE_NODE node2)                                        */
/*                                                                           */
/*  Inform soln that a call to KheNodeSwapChildNodesAndLayers has occurred.  */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpNodeSwapChildNodesAndLayers(KHE_SOLN soln,
  KHE_NODE node1, KHE_NODE node2)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpNodeSwapChildNodesAndLayers(soln->main_path, node1, node2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpNodeAddMeet(KHE_SOLN soln, KHE_NODE node, KHE_MEET meet)   */
/*                                                                           */
/*  Inform soln that a call to KheNodeAddMeet has occurred.                  */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpNodeAddMeet(KHE_SOLN soln, KHE_NODE node, KHE_MEET meet)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpNodeAddMeet(soln->main_path, node, meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpNodeDeleteMeet(KHE_SOLN soln, KHE_NODE node, KHE_MEET meet)*/
/*                                                                           */
/*  Inform soln that a call to KheNodeDeleteMeet has occurred.               */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpNodeDeleteMeet(KHE_SOLN soln, KHE_NODE node, KHE_MEET meet)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpNodeDeleteMeet(soln->main_path, node, meet);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "path operation loading - layers"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpLayerSetBack(KHE_SOLN soln, KHE_LAYER layer,               */
/*    void *old_back, void *new_back)                                        */
/*                                                                           */
/*  Inform soln that a call to KheLayerSetBack has occurred.                 */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpLayerSetBack(KHE_SOLN soln, KHE_LAYER layer,
  void *old_back, void *new_back)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpLayerSetBack(soln->main_path, layer, old_back, new_back);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpLayerAdd(KHE_SOLN soln, KHE_LAYER layer,                   */
/*    KHE_NODE parent_node)                                                  */
/*                                                                           */
/*  Inform soln that a call to KheLayerMake has occurred.                    */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpLayerAdd(KHE_SOLN soln, KHE_LAYER layer, KHE_NODE parent_node)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpLayerAdd(soln->main_path, layer, parent_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpLayerDelete(KHE_SOLN soln, KHE_LAYER layer,                */
/*    KHE_NODE parent_node)                                                  */
/*                                                                           */
/*  Inform soln that a call to KheLayerDelete has occurred.                  */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpLayerDelete(KHE_SOLN soln, KHE_LAYER layer, KHE_NODE parent_node)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpLayerDelete(soln->main_path, layer, parent_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpLayerAddChildNode(KHE_SOLN soln, KHE_LAYER layer,          */
/*    KHE_NODE child_node)                                                   */
/*                                                                           */
/*  Inform soln that a call to KheLayerAddChildNode has occurred.            */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpLayerAddChildNode(KHE_SOLN soln, KHE_LAYER layer,
  KHE_NODE child_node)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpLayerAddChildNode(soln->main_path, layer, child_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpLayerDeleteChildNode(KHE_SOLN soln, KHE_LAYER layer,       */
/*    KHE_NODE child_node)                                                   */
/*                                                                           */
/*  Inform soln that a call to KheLayerDeleteChildNode has occurred.         */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpLayerDeleteChildNode(KHE_SOLN soln, KHE_LAYER layer,
  KHE_NODE child_node)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpLayerDeleteChildNode(soln->main_path, layer, child_node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpLayerAddResource(KHE_SOLN soln, KHE_LAYER layer,           */
/*    KHE_RESOURCE resource)                                                 */
/*                                                                           */
/*  Inform soln that a call to KheLayerAddResource has occurred.             */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpLayerAddResource(KHE_SOLN soln, KHE_LAYER layer,
  KHE_RESOURCE resource)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpLayerAddResource(soln->main_path, layer, resource);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpLayerDeleteResource(KHE_SOLN soln, KHE_LAYER layer,        */
/*    KHE_RESOURCE resource)                                                 */
/*                                                                           */
/*  Inform soln that a call to KheLayerDeleteResource has occurred.          */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpLayerDeleteResource(KHE_SOLN soln, KHE_LAYER layer,
  KHE_RESOURCE resource)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpLayerDeleteResource(soln->main_path, layer, resource);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "path operation loading - zones"                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpZoneSetBack(KHE_SOLN soln, KHE_ZONE zone,                  */
/*    void *old_back, void *new_back)                                        */
/*                                                                           */
/*  Inform soln that a call to KheZoneSetBack has occurred.                  */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpZoneSetBack(KHE_SOLN soln, KHE_ZONE zone,
  void *old_back, void *new_back)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpZoneSetBack(soln->main_path, zone, old_back, new_back);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpZoneAdd(KHE_SOLN soln, KHE_ZONE zone, KHE_NODE node)       */
/*                                                                           */
/*  Inform soln that a call to KheZoneMake has occurred.                     */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpZoneAdd(KHE_SOLN soln, KHE_ZONE zone, KHE_NODE node)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpZoneAdd(soln->main_path, zone, node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpZoneDelete(KHE_SOLN soln, KHE_ZONE zone, KHE_NODE node)    */
/*                                                                           */
/*  Inform soln that a call to KheZoneDelete has occurred.                   */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpZoneDelete(KHE_SOLN soln, KHE_ZONE zone, KHE_NODE node)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpZoneDelete(soln->main_path, zone, node);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpZoneAddMeetOffset(KHE_SOLN soln,                           */
/*    KHE_ZONE zone, KHE_MEET meet, int offset)                              */
/*                                                                           */
/*  Inform soln that a call to KheZoneAddMeetOffset has occurred.            */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpZoneAddMeetOffset(KHE_SOLN soln,
  KHE_ZONE zone, KHE_MEET meet, int offset)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpZoneAddMeetOffset(soln->main_path, zone, meet, offset);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnOpZoneDeleteMeetOffset(KHE_SOLN soln,                        */
/*    KHE_ZONE zone, KHE_MEET meet, int offset)                              */
/*                                                                           */
/*  Inform soln that a call to KheZoneDeleteMeetOffset has occurred.         */
/*                                                                           */
/*****************************************************************************/

void KheSolnOpZoneDeleteMeetOffset(KHE_SOLN soln,
  KHE_ZONE zone, KHE_MEET meet, int offset)
{
  if( HaArrayCount(soln->marks) > 0 )
    KhePathOpZoneDeleteMeetOffset(soln->main_path, zone, meet, offset);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "evenness handling"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENNESS_HANDLER KheSolnEvennessHandler(KHE_SOLN soln)               */
/*                                                                           */
/*  Return soln's evenness handler, or NULL if none.                         */
/*                                                                           */
/*****************************************************************************/

KHE_EVENNESS_HANDLER KheSolnEvennessHandler(KHE_SOLN soln)
{
  return soln->evenness_handler;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnEvennessBegin(KHE_SOLN soln)                                 */
/*                                                                           */
/*  Begin evenness handling.                                                 */
/*                                                                           */
/*****************************************************************************/

void KheSolnEvennessBegin(KHE_SOLN soln)
{
  HnAssert(soln->evenness_handler == NULL,
    "KheSolnEvennessBegin: evenness monitoring already on");
  soln->evenness_handler = KheEvennessHandlerMake(soln);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnEvennessEnd(KHE_SOLN soln)                                   */
/*                                                                           */
/*  End evenness handling.                                                   */
/*                                                                           */
/*****************************************************************************/

void KheSolnEvennessEnd(KHE_SOLN soln)
{
  HnAssert(soln->evenness_handler != NULL,
    "KheSolnEvennessEnd: evenness monitoring not on");
  /* *** omitting this is fine
  KheEvennessHandlerDelete(soln->evenness_handler);
  *** */
  soln->evenness_handler = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnHasEvenness(KHE_SOLN soln)                                   */
/*                                                                           */
/*  Return true when evenness handling is on in soln.                        */
/*                                                                           */
/*****************************************************************************/

bool KheSolnHasEvenness(KHE_SOLN soln)
{
  return soln->evenness_handler != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAttachAllEvennessMonitors(KHE_SOLN soln)                     */
/*                                                                           */
/*  Ensure that all evenness monitors are attached.                          */
/*                                                                           */
/*****************************************************************************/

void KheSolnAttachAllEvennessMonitors(KHE_SOLN soln)
{
  HnAssert(soln->evenness_handler != NULL,
    "KheSolnAttachAllEvennessMonitors: evenness monitoring not on");
  KheEvennessHandlerAttachAllEvennessMonitors(soln->evenness_handler);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnDetachAllEvennessMonitors(KHE_SOLN soln)                     */
/*                                                                           */
/*  Ensure that all evenness monitors are detached.                          */
/*                                                                           */
/*****************************************************************************/

void KheSolnDetachAllEvennessMonitors(KHE_SOLN soln)
{
  HnAssert(soln->evenness_handler != NULL,
    "KheSolnDetachAllEvennessMonitors: evenness monitoring not on");
  KheEvennessHandlerDetachAllEvennessMonitors(soln->evenness_handler);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnSetAllEvennessMonitorWeights(KHE_SOLN soln, KHE_COST weight) */
/*                                                                           */
/*  Set the weight of all evenness monitors.                                 */
/*                                                                           */
/*****************************************************************************/

void KheSolnSetAllEvennessMonitorWeights(KHE_SOLN soln, KHE_COST weight)
{
  HnAssert(soln->evenness_handler != NULL,
    "KheSolnSetAllEvennessMonitorWeights: evenness monitoring not on");
  KheEvennessHandlerSetAllEvennessMonitorWeights(soln->evenness_handler,
    weight);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "redundancy handling"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENNESS_HANDLER KheSolnEvennessHandler(KHE_SOLN soln)               */
/*                                                                           */
/*  Return soln's redundancy handler, or NULL if none.                       */
/*                                                                           */
/*****************************************************************************/

KHE_REDUNDANCY_HANDLER KheSolnRedundancyHandler(KHE_SOLN soln)
{
  return soln->redundancy_handler;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnRedundancyBegin(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,         */
/*    KHE_COST_FUNCTION cf, KHE_COST combined_weight)                        */
/*                                                                           */
/*  Begin redundancy handling.                                               */
/*                                                                           */
/*****************************************************************************/

void KheSolnRedundancyBegin(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_COST_FUNCTION cf, KHE_COST combined_weight)
{
  if( DEBUG19 )
    fprintf(stderr, "[ KheSolnRedundancyBegin(soln of %s, %s, cf, %.5f)\n",
      KheInstanceId(KheSolnInstance(soln)), KheResourceTypeId(rt),
      KheCostShow(combined_weight));
  HnAssert(soln->redundancy_handler == NULL,
    "KheSolnRedundancyBegin: redundancy monitoring already on");
  soln->redundancy_handler = KheRedundancyHandlerMake(soln, rt, cf,
    combined_weight);
  if( DEBUG19 )
    fprintf(stderr, "] KheSolnRedundancyBegin returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnRedundancyEnd(KHE_SOLN soln)                                 */
/*                                                                           */
/*  End redundancy handling.                                                 */
/*                                                                           */
/*****************************************************************************/

void KheSolnRedundancyEnd(KHE_SOLN soln)
{
  HnAssert(soln->redundancy_handler != NULL,
    "KheSolnRedundancyEnd: redundancy monitoring not on");
  KheRedundancyHandlerDelete(soln->redundancy_handler);
  soln->redundancy_handler = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnHasRedundancy(KHE_SOLN soln)                                 */
/*                                                                           */
/*  Return true when redundancy handling is on in soln.                      */
/*                                                                           */
/*****************************************************************************/

bool KheSolnHasRedundancy(KHE_SOLN soln)
{
  return soln->redundancy_handler != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAttachAllRedundancyMonitors(KHE_SOLN soln)                   */
/*                                                                           */
/*  Ensure that all redundancy monitors are attached.                        */
/*                                                                           */
/*****************************************************************************/

void KheSolnAttachAllRedundancyMonitors(KHE_SOLN soln)
{
  HnAssert(soln->redundancy_handler != NULL,
    "KheSolnAttachAllRedundancyMonitors: redundancy monitoring not on");
  KheRedundancyHandlerAttachAllRedundancyMonitors(soln->redundancy_handler);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnDetachAllRedundancyMonitors(KHE_SOLN soln)                   */
/*                                                                           */
/*  Ensure that all redundancy monitors are detached.                        */
/*                                                                           */
/*****************************************************************************/

void KheSolnDetachAllRedundancyMonitors(KHE_SOLN soln)
{
  HnAssert(soln->redundancy_handler != NULL,
    "KheSolnDetachAllRedundancyMonitors: redundancy monitoring not on");
  KheRedundancyHandlerDetachAllRedundancyMonitors(soln->redundancy_handler);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "cost and monitors"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheCost(int hard_cost, int soft_cost)                           */
/*                                                                           */
/*  Combine hard_cost and soft_cost into a single cost.                      */
/*                                                                           */
/*****************************************************************************/
#define KHE_HARD_COST_WEIGHT ((KHE_COST) 1 << (KHE_COST) 32)

KHE_COST KheCost(int hard_cost, int soft_cost)
{
  return (KHE_COST) hard_cost * KHE_HARD_COST_WEIGHT + (KHE_COST) soft_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheHardCost(KHE_COST combined_cost)                                  */
/*                                                                           */
/*  Return the hard cost component of combined_cost.                         */
/*                                                                           */
/*****************************************************************************/

int KheHardCost(KHE_COST combined_cost)
{
  return (int) (combined_cost / KHE_HARD_COST_WEIGHT);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSoftCost(KHE_COST combined_cost)                                  */
/*                                                                           */
/*  Return the soft cost component of combined_cost.                         */
/*                                                                           */
/*****************************************************************************/

int KheSoftCost(KHE_COST combined_cost)
{
  return (int) (combined_cost % KHE_HARD_COST_WEIGHT);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCostCmp(KHE_COST cost1, KHE_COST cost2)                           */
/*                                                                           */
/*  Return an int which is less than, equal to, or greater than zero if      */
/*  the first argument is respectively less than, equal to, or greater       */
/*  than the second.                                                         */
/*                                                                           */
/*  Implementation note.  We can't just return cost1 - cost2 here, because   */
/*  of the difference in width of int and KHE_COST.  But this code does the  */
/*  equivalent of that, even when the costs are negative.                    */
/*                                                                           */
/*****************************************************************************/

int KheCostCmp(KHE_COST cost1, KHE_COST cost2)
{
  if( cost1 < cost2 )
    return -1;
  else if( cost1 > cost2 )
    return 1;
  else
    return 0;
  /* ***
  if( KheHardCost(cost1) != KheHardCost(cost2) )
    return KheHardCost(cost1) - KheHardCost(cost2);
  else
    return KheSoftCost(cost1) - KheSoftCost(cost2);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  double KheCostShow(KHE_COST combined_cost)                               */
/*                                                                           */
/*  Return a floating point value suitable for displaying combined_cost.     */
/*                                                                           */
/*****************************************************************************/
#define KHE_COST_SHOW_DIGITS 99999

double KheCostShow(KHE_COST combined_cost)
{
  int soft_cost;
  soft_cost = KheSoftCost(combined_cost);
  if( soft_cost > KHE_COST_SHOW_DIGITS )
    soft_cost = KHE_COST_SHOW_DIGITS;
  return (double) KheHardCost(combined_cost) +
    (double) soft_cost / (KHE_COST_SHOW_DIGITS + 1);
}


/*****************************************************************************/
/*                                                                           */
/*  double KheCostShowWide(KHE_COST combined_cost)                           */
/*                                                                           */
/*  Return a floating point value suitable for displaying combined_cost.     */
/*                                                                           */
/*****************************************************************************/
#define KHE_COST_SHOW_WIDE_DIGITS 99999999

double KheCostShowWide(KHE_COST combined_cost)
{
  int soft_cost;
  soft_cost = KheSoftCost(combined_cost);
  if( soft_cost > KHE_COST_SHOW_WIDE_DIGITS )
    soft_cost = KHE_COST_SHOW_WIDE_DIGITS;
  return (double) KheHardCost(combined_cost) +
    (double) soft_cost / (KHE_COST_SHOW_WIDE_DIGITS + 1);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheSolnCost(KHE_SOLN soln)                                      */
/*                                                                           */
/*  Return the total cost of soln.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheSolnCost(KHE_SOLN soln)
{
  if( soln->matching != NULL )
    KheMatchingUnmatchedDemandNodeCount(soln->matching);
  return soln->cost;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnChildMonitorCount(KHE_SOLN soln)                              */
/*                                                                           */
/*  Return the number of child monitors of soln.                             */
/*                                                                           */
/*****************************************************************************/

int KheSolnChildMonitorCount(KHE_SOLN soln)
{
  return KheGroupMonitorChildMonitorCount((KHE_GROUP_MONITOR) soln);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheSolnChildMonitor(KHE_SOLN soln, int i)                    */
/*                                                                           */
/*  Return the i'th child monitor of soln.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR KheSolnChildMonitor(KHE_SOLN soln, int i)
{
  return KheGroupMonitorChildMonitor((KHE_GROUP_MONITOR) soln, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnDefectCount(KHE_SOLN soln)                                    */
/*                                                                           */
/*  Return the number of defects (child monitors of non-zero cost) of soln.  */
/*                                                                           */
/*****************************************************************************/

int KheSolnDefectCount(KHE_SOLN soln)
{
  return KheGroupMonitorDefectCount((KHE_GROUP_MONITOR) soln);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheSolnDefect(KHE_SOLN soln, int i)                          */
/*                                                                           */
/*  Return the i'th defect (child monitor of non-zero cost) of soln.         */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR KheSolnDefect(KHE_SOLN soln, int i)
{
  return KheGroupMonitorDefect((KHE_GROUP_MONITOR) soln, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "event monitors and cost"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheSolnEventMonitorCount(KHE_SOLN soln, KHE_EVENT e)                 */
/*                                                                           */
/*  Return the number of event monitors of e in soln.                        */
/*                                                                           */
/*****************************************************************************/

int KheSolnEventMonitorCount(KHE_SOLN soln, KHE_EVENT e)
{
  KHE_EVENT_IN_SOLN es;
  es = HaArray(soln->events_in_soln, KheEventIndex(e));
  return KheEventInSolnMonitorCount(es);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheSolnEventMonitor(KHE_SOLN soln, KHE_EVENT e, int i)       */
/*                                                                           */
/*  Return the i'th event monitor of e in soln.                              */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR KheSolnEventMonitor(KHE_SOLN soln, KHE_EVENT e, int i)
{
  KHE_EVENT_IN_SOLN es;
  es = HaArray(soln->events_in_soln, KheEventIndex(e));
  return KheEventInSolnMonitor(es, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheSolnEventCost(KHE_SOLN soln, KHE_EVENT e)                    */
/*                                                                           */
/*  Return the total cost of monitors applicable to e.                       */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheSolnEventCost(KHE_SOLN soln, KHE_EVENT e)
{
  KHE_EVENT_IN_SOLN es;
  es = HaArray(soln->events_in_soln, KheEventIndex(e));
  return KheEventInSolnCost(es);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheSolnEventMonitorCost(KHE_SOLN soln, KHE_EVENT e,             */
/*    KHE_MONITOR_TAG tag)                                                   */
/*                                                                           */
/*  Return the total cost of monitors of type tag applicable to e.           */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheSolnEventMonitorCost(KHE_SOLN soln, KHE_EVENT e,
  KHE_MONITOR_TAG tag)
{
  KHE_EVENT_IN_SOLN es;
  es = HaArray(soln->events_in_soln, KheEventIndex(e));
  return KheEventInSolnMonitorCost(es, tag);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_TIMETABLE_MONITOR KheEventTimetableMonitor(KHE_SOLN soln,      */
/*    KHE_EVENT e)                                                           */
/*                                                                           */
/*  Return the timetable of e in soln.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_TIMETABLE_MONITOR KheEventTimetableMonitor(KHE_SOLN soln, KHE_EVENT e)
{
  KHE_EVENT_IN_SOLN es;
  es = HaArray(soln->events_in_soln, KheEventIndex(e));
  return KheEventInSolnTimetableMonitor(es);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "event resource monitors and cost"                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheSolnEventResourceMonitorCount(KHE_SOLN soln,                      */
/*    KHE_EVENT_RESOURCE er)                                                 */
/*                                                                           */
/*  Return the number of event resource monitors of er in soln.              */
/*                                                                           */
/*****************************************************************************/

int KheSolnEventResourceMonitorCount(KHE_SOLN soln, KHE_EVENT_RESOURCE er)
{
  KHE_EVENT_RESOURCE_IN_SOLN ers;
  ers = KheSolnEventResourceInSoln(soln, er);
  return KheEventResourceInSolnMonitorCount(ers);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheSolnEventResourceMonitor(KHE_SOLN soln,                   */
/*    KHE_EVENT_RESOURCE er, int i)                                          */
/*                                                                           */
/*  Return the i'th event resource monitor of er in soln.                    */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR KheSolnEventResourceMonitor(KHE_SOLN soln,
  KHE_EVENT_RESOURCE er, int i)
{
  KHE_EVENT_RESOURCE_IN_SOLN ers;
  ers = KheSolnEventResourceInSoln(soln, er);
  return KheEventResourceInSolnMonitor(ers, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheSolnEventResourceCost(KHE_SOLN soln, KHE_EVENT_RESOURCE er)  */
/*                                                                           */
/*  Return the total cost of monitors applicable to er.                      */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheSolnEventResourceCost(KHE_SOLN soln, KHE_EVENT_RESOURCE er)
{
  KHE_EVENT_RESOURCE_IN_SOLN ers;
  ers = KheSolnEventResourceInSoln(soln, er);
  return KheEventResourceInSolnCost(ers);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheSolnEventResourceMonitorCost(KHE_SOLN soln,                  */
/*    KHE_EVENT_RESOURCE er, KHE_MONITOR_TAG tag)                            */
/*                                                                           */
/*  Return the total cost of monitors of type tag applicable to er.          */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheSolnEventResourceMonitorCost(KHE_SOLN soln, KHE_EVENT_RESOURCE er,
  KHE_MONITOR_TAG tag)
{
  KHE_EVENT_RESOURCE_IN_SOLN ers;
  ers = KheSolnEventResourceInSoln(soln, er);
  return KheEventResourceInSolnMonitorCost(ers, tag);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "tasks assigned to resources"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheResourceAssignedTaskCount(KHE_SOLN soln, KHE_RESOURCE r)          */
/*                                                                           */
/*  Return the number of tasks assigned r in soln.                           */
/*                                                                           */
/*****************************************************************************/

int KheResourceAssignedTaskCount(KHE_SOLN soln, KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = HaArray(soln->resources_in_soln, KheResourceInstanceIndex(r));
  return KheResourceInSolnAssignedTaskCount(rs);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheResourceAssignedTask(KHE_SOLN soln, KHE_RESOURCE r, int i)   */
/*                                                                           */
/*  Return the i'th task assigned to r in soln.                              */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheResourceAssignedTask(KHE_SOLN soln, KHE_RESOURCE r, int i)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = HaArray(soln->resources_in_soln, KheResourceInstanceIndex(r));
  return KheResourceInSolnAssignedTask(rs, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource availability"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_AVAIL_SOLVER KheSolnAvailSolver(KHE_SOLN soln)                       */
/*                                                                           */
/*  Return soln's avail solver.  Make it first, if necessary.                */
/*                                                                           */
/*****************************************************************************/

KHE_AVAIL_SOLVER KheSolnAvailSolver(KHE_SOLN soln)
{
  if( soln->avail_solver == NULL )
    soln->avail_solver = KheAvailSolverMake(soln, soln->inner_arena);
  return soln->avail_solver;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceMaxBusyTimes(KHE_SOLN soln, KHE_RESOURCE r, int *res)    */
/*                                                                           */
/*  If there is a non-trivial maximum limit on the total number of busy      */
/*  times of r in soln's instance, return true and set *res to that number,  */
/*  otherwise return false and set *res to INT_MAX.                          */
/*                                                                           */
/*****************************************************************************/

bool KheResourceMaxBusyTimes(KHE_SOLN soln, KHE_RESOURCE r, int *res)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = HaArray(soln->resources_in_soln, KheResourceInstanceIndex(r));
  return KheResourceInSolnMaxBusyTimes(rs, KheSolnAvailSolver(soln), res);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceMaxWorkload(KHE_SOLN soln, KHE_RESOURCE r, float *res)   */
/*                                                                           */
/*  If there is a non-trivial maximum limit on the total workload of r       */
/*  in soln's instance, return true and set *res to that number, otherwise   */
/*  return false and set *res to FLT_MAX.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheResourceMaxWorkload(KHE_SOLN soln, KHE_RESOURCE r, float *res)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = HaArray(soln->resources_in_soln, KheResourceInstanceIndex(r));
  return KheResourceInSolnMaxWorkload(rs, KheSolnAvailSolver(soln), res);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceBusyTimes(KHE_SOLN soln, KHE_RESOURCE r)                  */
/*                                                                           */
/*  Return the total busy times of r in soln.                                */
/*                                                                           */
/*****************************************************************************/

int KheResourceBusyTimes(KHE_SOLN soln, KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = HaArray(soln->resources_in_soln, KheResourceInstanceIndex(r));
  return KheResourceInSolnBusyTimes(rs);
}


/*****************************************************************************/
/*                                                                           */
/*  float KheResourceWorkload(KHE_SOLN soln, KHE_RESOURCE r)                 */
/*                                                                           */
/*  Return the total workload of r in soln.                                  */
/*                                                                           */
/*****************************************************************************/

float KheResourceWorkload(KHE_SOLN soln, KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = HaArray(soln->resources_in_soln, KheResourceInstanceIndex(r));
  return KheResourceInSolnWorkload(rs);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceAvailableBusyTimes(KHE_SOLN soln, KHE_RESOURCE r,        */
/*    int *res)                                                              */
/*                                                                           */
/*  Return the available busy times of r in soln.                            */
/*                                                                           */
/*****************************************************************************/

bool KheResourceAvailableBusyTimes(KHE_SOLN soln, KHE_RESOURCE r, int *res)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = HaArray(soln->resources_in_soln, KheResourceInstanceIndex(r));
  return KheResourceInSolnAvailableBusyTimes(rs, KheSolnAvailSolver(soln), res);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceAvailableWorkload(KHE_SOLN soln, KHE_RESOURCE r,         */
/*    float *res)                                                            */
/*                                                                           */
/*  Return the available workload of r in soln.                              */
/*                                                                           */
/*****************************************************************************/

bool KheResourceAvailableWorkload(KHE_SOLN soln, KHE_RESOURCE r, float *res)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = HaArray(soln->resources_in_soln, KheResourceInstanceIndex(r));
  return KheResourceInSolnAvailableWorkload(rs, KheSolnAvailSolver(soln), res);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource monitors and cost"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheSolnResourceMonitorCount(KHE_SOLN soln, KHE_RESOURCE r)           */
/*                                                                           */
/*  Return the number of resource monitors of r in soln.                     */
/*                                                                           */
/*****************************************************************************/

int KheSolnResourceMonitorCount(KHE_SOLN soln, KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = HaArray(soln->resources_in_soln, KheResourceInstanceIndex(r));
  return KheResourceInSolnMonitorCount(rs);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheSolnResourceMonitor(KHE_SOLN soln, KHE_RESOURCE r, int i) */
/*                                                                           */
/*  Return the i'th resource monitor of r in soln.                           */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR KheSolnResourceMonitor(KHE_SOLN soln, KHE_RESOURCE r, int i)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = HaArray(soln->resources_in_soln, KheResourceInstanceIndex(r));
  return KheResourceInSolnMonitor(rs, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnResourceMonitorSort(KHE_SOLN soln, KHE_RESOURCE r,           */
/*    int(*compar)(const void *, const void *))                              */
/*                                                                           */
/*  Sort the resource monitors of r using compar.                            */
/*                                                                           */
/*****************************************************************************/

void KheSolnResourceMonitorSort(KHE_SOLN soln, KHE_RESOURCE r,
  int(*compar)(const void *, const void *))
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = HaArray(soln->resources_in_soln, KheResourceInstanceIndex(r));
  KheResourceInSolnMonitorSort(rs, compar);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheSolnResourceCost(KHE_SOLN soln, KHE_RESOURCE r)              */
/*                                                                           */
/*  Return the total cost of monitors applicable to r.                       */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheSolnResourceCost(KHE_SOLN soln, KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = HaArray(soln->resources_in_soln, KheResourceInstanceIndex(r));
  return KheResourceInSolnCost(rs);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheSolnResourceMonitorCost(KHE_SOLN soln, KHE_RESOURCE r,       */
/*    KHE_MONITOR_TAG tag)                                                   */
/*                                                                           */
/*  Return the total cost of constraints of type tag applicable to r.        */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheSolnResourceMonitorCost(KHE_SOLN soln, KHE_RESOURCE r,
  KHE_MONITOR_TAG tag)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = HaArray(soln->resources_in_soln, KheResourceInstanceIndex(r));
  return KheResourceInSolnMonitorCost(rs, tag);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIMETABLE_MONITOR KheResourceTimetableMonitor(KHE_SOLN soln,         */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Return the timetable of r in soln.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TIMETABLE_MONITOR KheResourceTimetableMonitor(KHE_SOLN soln,
  KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = HaArray(soln->resources_in_soln, KheResourceInstanceIndex(r));
  return KheResourceInSolnTimetableMonitor(rs);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_IN_SOLN KheSolnResourceInSoln(KHE_SOLN soln, KHE_RESOURCE r)*/
/*                                                                           */
/*  Return the resource monitor corresponding to r in soln.                  */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_IN_SOLN KheSolnResourceInSoln(KHE_SOLN soln, KHE_RESOURCE r)
{
  return HaArray(soln->resources_in_soln, KheResourceInstanceIndex(r));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitors"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAddMonitor(KHE_SOLN soln, KHE_MONITOR m, int *index_in_soln) */
/*                                                                           */
/*  Add m to soln, including setting m's soln_index.                         */
/*                                                                           */
/*****************************************************************************/

void KheSolnAddMonitor(KHE_SOLN soln, KHE_MONITOR m)
{
  KheMonitorSetSolnIndex(m, HaArrayCount(soln->monitors));
  HaArrayAddLast(soln->monitors, m);
  if( DEBUG3 )
    fprintf(stderr, "KheSolnAddMonitor(soln, %p at %d)\n", (void *) m,
      KheMonitorSolnIndex(m));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnDeleteMonitor(KHE_SOLN soln, KHE_MONITOR m)                  */
/*                                                                           */
/*  Delete m from soln.                                                      */
/*                                                                           */
/*****************************************************************************/

void KheSolnDeleteMonitor(KHE_SOLN soln, KHE_MONITOR m)
{
  int index;
  index = KheMonitorSolnIndex(m);
  HaArrayDeleteAndPlug(soln->monitors, index);
  if( index < HaArrayCount(soln->monitors) )
    KheMonitorSetSolnIndex(HaArray(soln->monitors, index), index);
  /* tmp = MArrayRemoveAndPlug(soln->monitors, KheMonitorSolnIndex(m)); */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnMonitorCount(KHE_SOLN soln)                                   */
/*                                                                           */
/*  Return the number of monitors of soln.                                   */
/*                                                                           */
/*****************************************************************************/

int KheSolnMonitorCount(KHE_SOLN soln)
{
  return HaArrayCount(soln->monitors);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheSolnMonitor(KHE_SOLN soln, int i)                         */
/*                                                                           */
/*  Return the i'th monitor of soln.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR KheSolnMonitor(KHE_SOLN soln, int i)
{
  return HaArray(soln->monitors, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT GetConstraint(KHE_SOLN soln, char *str)                   */
/*                                                                           */
/*  Get the first field of str, supposed to be a constraint.                 */
/*                                                                           */
/*****************************************************************************/

static KHE_CONSTRAINT GetConstraint(KHE_SOLN soln, char *str, char **next)
{
  char *p;  KHE_CONSTRAINT res;

  /* find the field */
  p = strstr(str, "/");
  HnAssert(p != NULL, "GetResource: monitor id \"%s\" has no '/'", str);
  *p = '\0';

  /* get the constraint from the field */
  if( !KheInstanceRetrieveConstraint(soln->instance, str, &res) )
    HnAbort("KheSolnRetrieveMonitor field %s is not a constraint Id", str);
  if( p != NULL )
    *p = '/';
  *next = p;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT GetEvent(KHE_SOLN soln, char *str)                             */
/*                                                                           */
/*  Get the first field of str, supposed to be an event.                     */
/*                                                                           */
/*****************************************************************************/

static KHE_EVENT GetEvent(KHE_SOLN soln, char *str)
{
  char *p;  KHE_EVENT res;

  /* find the field */
  p = strstr(str, "/");
  if( p != NULL )
    *p = '\0';

  /* get the event from the field */
  if( !KheInstanceRetrieveEvent(soln->instance, str, &res) )
    HnAbort("KheSolnRetrieveMonitor field %s is not an event Id", str);
  if( p != NULL )
    *p = '/';
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE GetResource(KHE_SOLN soln, char *str)                       */
/*                                                                           */
/*  Get the first field of str, supposed to be a resource.                   */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE GetResource(KHE_SOLN soln, char *str)
{
  char *p;  KHE_RESOURCE res;

  /* find the field */
  p = strstr(str, "/");
  if( p != NULL )
    *p = '\0';

  /* get the resource from the field */
  if( !KheInstanceRetrieveResource(soln->instance, str, &res) )
    HnAbort("KheSolnRetrieveMonitor field %s is not a resource Id", str);
  if( p != NULL )
    *p = '/';
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnRetrieveMonitor(KHE_SOLN soln, char *id, KHE_MONITOR *m)     */
/*                                                                           */
/*  Retrieve the monitor with the given id from soln.                        */
/*                                                                           */
/*****************************************************************************/

bool KheSolnRetrieveMonitor(KHE_SOLN soln, char *id, KHE_MONITOR *m)
{
  KHE_CONSTRAINT c;  KHE_RESOURCE r;  int i, j;  KHE_MONITOR m2;
  KHE_EVENT e;  KHE_EVENT_RESOURCE er;  char *p;  char buff[500];

  HnAssert(strlen(id) < 500, "KheSolnRetrieveMonitor: id too long");
  strcpy(buff, id);
  c = GetConstraint(soln, buff, &p);
  switch( KheConstraintTag(c) )
  {
    /* event constraints that monitor one or two events */
    case KHE_ASSIGN_TIME_CONSTRAINT_TAG:
    case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:
    case KHE_PREFER_TIMES_CONSTRAINT_TAG:
    case KHE_ORDER_EVENTS_CONSTRAINT_TAG:

      e = GetEvent(soln, p + 1);
      for( i = 0;  i < KheSolnEventMonitorCount(soln, e);  i++ )
      {
	m2 = KheSolnEventMonitor(soln, e, i);
	if( strcmp(KheMonitorId(m2), buff) == 0 )
	  return *m = m2, true;
      }
      break;

    /* event constraints that monitor an event group */
    case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:
    case KHE_LINK_EVENTS_CONSTRAINT_TAG:

      /* not implemented */
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:

      /* not implemented */
      break;

    case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:
    case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:
    case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:

      /* implemented for monitors whose Id's contain event Id's */
      e = GetEvent(soln, p + 1);
      for( i = 0;  i < KheEventResourceCount(e);  i++ )
      {
	er = KheEventResource(e, i);
	for( j = 0;  j < KheSolnEventResourceMonitorCount(soln, er);  j++ )
	{
	  m2 = KheSolnEventResourceMonitor(soln, er, j);
	  if( strcmp(KheMonitorId(m2), buff) == 0 )
	    return *m = m2, true;
	}
      }
      break;

    /* resource constraints */
    case KHE_AVOID_CLASHES_CONSTRAINT_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:
    case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:
    case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:
    case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:
    case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:

      r = GetResource(soln, p + 1);
      for( i = 0;  i < KheSolnResourceMonitorCount(soln, r);  i++ )
      {
	m2 = KheSolnResourceMonitor(soln, r, i);
	if( strcmp(KheMonitorId(m2), buff) == 0 )
	  return *m = m2, true;
      }
      break;

    default:

      HnAbort("KheSolnRetrieveMonitor: constraint tag %d", KheConstraintTag(c));
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnBypassAndDeleteAllGroupMonitors(KHE_SOLN soln)               */
/*                                                                           */
/*  Bypass and delete every group monitor of soln.                           */
/*                                                                           */
/*****************************************************************************/

void KheSolnBypassAndDeleteAllGroupMonitors(KHE_SOLN soln)
{
  KHE_MONITOR m;  int i;
  HaArrayForEach(soln->monitors, m, i)
    if( KheMonitorTag(m) == KHE_GROUP_MONITOR_TAG )
    {
      KheGroupMonitorBypassAndDelete((KHE_GROUP_MONITOR) m);
      i--;
    }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool Error(KHE_SOLN soln, bool allow_invalid_solns, KML_ERROR *ke)       */
/*                                                                           */
/*  Helper function for KheSolnMakeFromKml below.  An error has been         */
/*  found and assigned to *ke, so it is time to return.  But, depending on   */
/*  allow_invalid_solns, we either return false immediately, or make         */
/*  soln invalid and return true.                                            */
/*                                                                           */
/*****************************************************************************/

static bool Error(KHE_SOLN soln, bool allow_invalid_solns, KML_ERROR *ke)
{
  if( allow_invalid_solns )
  {
    KheSolnTypeReduce(soln, KHE_SOLN_INVALID_PLACEHOLDER, *ke);
    *ke = NULL;
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnMakeFromKml(KML_ELT soln_elt, KHE_ARCHIVE archive,           */
/*    bool allow_invalid_solns, KHE_SOLN_TYPE soln_type,                     */
/*    KHE_SOLN *soln, KML_ERROR *ke, HA_ARENA a)                             */
/*                                                                           */
/*  Make a solution based on soln_elt.  Its instance must come from          */
/*  archive.  Reduce it to a placeholder, or not, depending on soln_type.    */
/*                                                                           */
/*  Parameter a is used to hold *ke, if it needs to be created.              */
/*                                                                           */
/*****************************************************************************/

bool KheSolnMakeFromKml(KML_ELT soln_elt, KHE_ARCHIVE archive,
  bool allow_invalid_solns, KHE_SOLN_TYPE soln_type, HA_ARENA_SET as,
  KHE_SOLN *soln, KML_ERROR *ke, HA_ARENA a)
{
  KML_ELT events_elt, event_elt, descr_elt, rt_elt;  char *ref, *text;
  KHE_EVENT e;  KHE_INSTANCE ins;  int i, junk;  float running_time;

  /* check soln_elt, find ins, and make res, the result object */
  if( DEBUG2 )
    fprintf(stderr, "[ KheSolnMakeFromKml(soln_elt)\n");
  if( !KmlCheck(soln_elt,
	"Reference : +$Description +%RunningTime +Events +Report", ke) )
    return false;
  ref = KmlAttributeValue(soln_elt, 0);
  if( !KheArchiveRetrieveInstance(archive, ref, &ins, &junk) )
    return KmlError(ke, a, KmlLineNum(soln_elt), KmlColNum(soln_elt),
      "<Solution> Reference \"%s\" unknown", ref);
  /* ***
  if( DEBUG2 )
    fprintf(stderr, "  KheSolnMakeFromKml making soln (with_monitors = %s)\n",
      with_monitors ? "true" : "false");
  *** */
  *soln = KheSolnMake(ins, as);

  /* Description */
  if( KmlContainsChild(soln_elt, "Description", &descr_elt) )
    KheSolnSetDescription(*soln, KmlText(descr_elt));

  /* RunningTime */
  if( KmlContainsChild(soln_elt, "RunningTime", &rt_elt) )
  {
    text = KmlText(rt_elt);
    sscanf(text, "%f", &running_time);
    KheSolnSetRunningTime(*soln, running_time);
  }

  /* Events */
  if( KmlContainsChild(soln_elt, "Events", &events_elt) )
  {
    if( !KmlCheck(events_elt, ": *Event", ke) )
      return Error(*soln, allow_invalid_solns, ke);
    if( DEBUG2 )
      fprintf(stderr, "  KheSolnMakeFromKml making %d meets\n",
	KmlChildCount(events_elt));
    for( i = 0;  i < KmlChildCount(events_elt);  i++ )
    {
      event_elt = KmlChild(events_elt, i);
      if( !KheMeetMakeFromKml(event_elt, *soln, ke) )
	return Error(*soln, allow_invalid_solns, ke);
    }
  }

  /* make sure event durations are correct, then add any missing ones */
  if( DEBUG2 )
    fprintf(stderr, "  KheSolnMakeFromKml making complete representation\n");
  if( !KheSolnMakeCompleteRepresentation(*soln, &e) )
  {
    KmlError(ke, a, KmlLineNum(soln_elt), KmlColNum(soln_elt),
      "<Solution> invalid total duration of meets of event \"%s\"",
      KheEventId(e));
    return Error(*soln, allow_invalid_solns, ke);
  }

  /* convert preassignments into assignments */
  if( DEBUG2 )
    fprintf(stderr, "  KheSolnMakeFromKml making preassignments\n");
  KheSolnAssignPreassignedTimes(*soln);
  KheSolnAssignPreassignedResources(*soln, NULL);

  /* reduce to placeholder, if requested (cannot be invalid here) */
  HnAssert(soln_type != KHE_SOLN_INVALID_PLACEHOLDER,
    "KheSolnMakeFromKml:  soln_type is KHE_SOLN_INVALID_PLACEHOLDER");
  KheSolnTypeReduce(*soln, soln_type, NULL);

  if( DEBUG2 )
  {
    KheSolnDebug(*soln, 2, 2, stderr);
    fprintf(stderr, "] KheSolnMakeFromKml returning\n");
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnResourcesReportWrite(KHE_SOLN soln, KML_FILE kf)             */
/*                                                                           */
/*  Write the Resources part of the report on soln.                          */
/*                                                                           */
/*****************************************************************************/

static void KheSolnResourcesReportWrite(KHE_SOLN soln, KML_FILE kf)
{
  bool section_started, resource_started;  KHE_CONSTRAINT c;
  KHE_INSTANCE ins;  KHE_RESOURCE r;  int i, j, cost;  KHE_MONITOR m;
  section_started = false;
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    r = KheInstanceResource(ins, i);
    resource_started = false;
    for( j = 0;  j < KheSolnResourceMonitorCount(soln, r);  j++ )
    {
      m = KheSolnResourceMonitor(soln, r, j);
      c = KheMonitorConstraint(m);
      if( c != NULL && KheMonitorCost(m) > 0 )
      {
	if( !section_started )
	{
	  KmlBegin(kf, "Resources");
	  section_started = true;
	}
	if( !resource_started )
	{
	  KmlBegin(kf, "Resource");
	  KmlAttribute(kf, "Reference", KheResourceId(r));
	  resource_started = true;
	}
        KmlBegin(kf, "Constraint");
	KmlAttribute(kf, "Reference", KheConstraintId(c));
	cost = KheConstraintRequired(c) ? KheHardCost(KheMonitorCost(m)) :
	  KheSoftCost(KheMonitorCost(m));
	KmlEltFmtText(kf, "Cost", "%d", cost);
	/* no Description at present */
        KmlEnd(kf, "Constraint");
      }
    }
    if( resource_started )
      KmlEnd(kf, "Resource");
  }
  if( section_started )
    KmlEnd(kf, "Resources");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorReportWithEvent(KHE_MONITOR_TAG tag)                      */
/*                                                                           */
/*  Return true if tag tags a monitor that is reported in the Events         */
/*  section of a report.                                                     */
/*                                                                           */
/*****************************************************************************/

static bool KheMonitorReportWithEvent(KHE_MONITOR_TAG tag)
{
  return tag == KHE_ASSIGN_RESOURCE_MONITOR_TAG ||
    tag == KHE_ASSIGN_TIME_MONITOR_TAG ||
    tag == KHE_SPLIT_EVENTS_MONITOR_TAG ||
    tag == KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG ||
    tag == KHE_PREFER_RESOURCES_MONITOR_TAG ||
    tag == KHE_PREFER_TIMES_MONITOR_TAG;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheHandleEventMonitor(KHE_SOLN soln, KML_FILE kf, KHE_MONITOR m,    */
/*    KHE_EVENT e, bool *section_started, bool *event_started)               */
/*                                                                           */
/*  Handle the reporting of monitor m, if relevant with non-zero cost.       */
/*                                                                           */
/*****************************************************************************/

static void KheHandleEventMonitor(KHE_SOLN soln, KML_FILE kf, KHE_MONITOR m,
  KHE_EVENT e, bool *section_started, bool *event_started)
{
  KHE_CONSTRAINT c;  int cost;
  if( KheMonitorReportWithEvent(KheMonitorTag(m)) && KheMonitorCost(m) > 0 )
  {
    c = KheMonitorConstraint(m);
    if( c != NULL )
    {
      if( !*section_started )
      {
	KmlBegin(kf, "Events");
	*section_started = true;
      }
      if( !*event_started )
      {
	KmlBegin(kf, "Event");
	KmlAttribute(kf, "Reference", KheEventId(e));
	*event_started = true;
      }
      KmlBegin(kf, "Constraint");
      KmlAttribute(kf, "Reference", KheConstraintId(c));
      cost = KheConstraintRequired(c) ? KheHardCost(KheMonitorCost(m)) :
	KheSoftCost(KheMonitorCost(m));
      KmlEltFmtText(kf, "Cost", "%d", cost);
      /* no Description at present */
      KmlEnd(kf, "Constraint");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventsReportWrite(KHE_SOLN soln, KML_FILE kf)                    */
/*                                                                           */
/*  Write the Events part of the report on soln.                             */
/*                                                                           */
/*****************************************************************************/

static void KheEventsReportWrite(KHE_SOLN soln, KML_FILE kf)
{
  bool section_started, event_started;  KHE_EVENT_RESOURCE er;
  KHE_INSTANCE ins;  KHE_EVENT e;  int i, j, k;  KHE_MONITOR m;
  section_started = false;
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    event_started = false;
    for( j = 0;  j < KheSolnEventMonitorCount(soln, e);  j++ )
    {
      m = KheSolnEventMonitor(soln, e, j);
      KheHandleEventMonitor(soln, kf, m, e, &section_started, &event_started);
    }
    for( j = 0;  j < KheEventResourceCount(e);  j++ )
    {
      er = KheEventResource(e, j);
      for( k = 0;  k < KheSolnEventResourceMonitorCount(soln, er);  k++ )
      {
	m = KheSolnEventResourceMonitor(soln, er, k);
	KheHandleEventMonitor(soln, kf, m, e, &section_started, &event_started);
      }
    }
    if( event_started )
      KmlEnd(kf, "Event");
  }
  if( section_started )
    KmlEnd(kf, "Events");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorReportWithEventGroup(KHE_MONITOR m, KHE_EVENT_GROUP eg)   */
/*                                                                           */
/*  Return true if m is to be reported in eg's section of the report.        */
/*                                                                           */
/*****************************************************************************/

static bool KheMonitorReportWithEventGroup(KHE_MONITOR m, KHE_EVENT_GROUP eg)
{
  KHE_SPREAD_EVENTS_MONITOR sem;  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam;
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c;  int i;
  switch( KheMonitorTag(m) )
  {
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      asam = (KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m;
      c = KheAvoidSplitAssignmentsMonitorConstraint(asam);
      i = KheAvoidSplitAssignmentsMonitorEventGroupIndex(asam);
      return KheAvoidSplitAssignmentsConstraintEventGroup(c, i) == eg;

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      sem = (KHE_SPREAD_EVENTS_MONITOR) m;
      return KheSpreadEventsMonitorEventGroup(sem) == eg;

    case KHE_LINK_EVENTS_MONITOR_TAG:

      return KheLinkEventsMonitorEventGroup((KHE_LINK_EVENTS_MONITOR) m) == eg;

    default:

      return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventGroupsReportWrite(KHE_SOLN soln, KML_FILE kf)               */
/*                                                                           */
/*  Write the EventGroups part of the report on soln to ktf.                 */
/*                                                                           */
/*****************************************************************************/

static void KheEventGroupsReportWrite(KHE_SOLN soln, KML_FILE kf)
{
  bool section_started;  KHE_EVENT_GROUP eg;  KHE_CONSTRAINT c;
  KHE_INSTANCE ins;  KHE_EVENT e;  int i, j, k, n, pos, cost;
  ARRAY_KHE_MONITOR eg_monitors;  KHE_MONITOR m;  KHE_EVENT_RESOURCE er;
  section_started = false;
  ins = KheSolnInstance(soln);
  HaArrayInit(eg_monitors, soln->inner_arena);
  for( i = 0;  i < KheInstanceEventGroupCount(ins);  i++ )
  {
    eg = KheInstanceEventGroup(ins, i);

    /* find the monitors of event group eg */
    HaArrayClear(eg_monitors);
    for( j = 0;  j < KheEventGroupEventCount(eg);  j++ )
    {
      e = KheEventGroupEvent(eg, j);
      for( k = 0;  k < KheSolnEventMonitorCount(soln, e);  k++ )
      {
	m = KheSolnEventMonitor(soln, e, k);
	if( KheMonitorReportWithEventGroup(m, eg) &&
	    KheMonitorCost(m) > 0 && !HaArrayContains(eg_monitors, m, &pos) )
	  HaArrayAddLast(eg_monitors, m);
      }
      for( k = 0;  k < KheEventResourceCount(e);  k++ )
      {
	er = KheEventResource(e, k);
	for( n = 0;  n < KheSolnEventResourceMonitorCount(soln, er);  n++ )
	{
	  m = KheSolnEventResourceMonitor(soln, er, n);
	  if( KheMonitorReportWithEventGroup(m, eg) &&
	      KheMonitorCost(m) > 0 && !HaArrayContains(eg_monitors, m, &pos) )
	    HaArrayAddLast(eg_monitors, m);
	}
      }
    }

    /* print the report for event group eg, if any relevant monitors */
    if( HaArrayCount(eg_monitors) > 0 )
    {
      if( !section_started )
      {
	KmlBegin(kf, "EventGroups");
	section_started = true;
      }
      KmlBegin(kf, "EventGroup");
      KmlAttribute(kf, "Reference", KheEventGroupId(eg));
      HaArrayForEach(eg_monitors, m, k)
      {
	c = KheMonitorConstraint(m);
	if( c != NULL )
	{
	  KmlBegin(kf, "Constraint");
	  KmlAttribute(kf, "Reference", KheConstraintId(c));
	  cost = KheConstraintRequired(c) ? KheHardCost(KheMonitorCost(m)) :
	    KheSoftCost(KheMonitorCost(m));
	  KmlEltFmtText(kf, "Cost", "%d", cost);
	  /* no Description at present */
	  KmlEnd(kf, "Constraint");
	}
      }
      KmlEnd(kf, "EventGroup");
    }
  }
  if( section_started )
    KmlEnd(kf, "EventGroups");
  HaArrayFree(eg_monitors);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnWrite(KHE_SOLN soln, bool with_reports, KML_FILE kf)         */
/*                                                                           */
/*  Write soln to kf, with a report if with_reports is true.                 */
/*                                                                           */
/*****************************************************************************/

void KheSolnWrite(KHE_SOLN soln, bool with_reports, KML_FILE kf)
{
  KHE_EVENT_IN_SOLN es;  int i;  bool event_written;  KHE_COST cost;
  switch( soln->type )
  {
    case KHE_SOLN_INVALID_PLACEHOLDER:

      HnAbort("KheSolnWrite: cannot write invalid placeholder soln");
      break;

    case KHE_SOLN_BASIC_PLACEHOLDER:

      HnAbort("KheSolnWrite: cannot write basic placeholder soln");
      break;

    case KHE_SOLN_WRITABLE_PLACEHOLDER:

      HnAssert(soln->write_only_soln != NULL, "KheSolnWrite internal error 1");
      HnAssert(!with_reports,
	"KheSolnWrite: cannot write report for writable placeholder soln");
      KheSolnWriteOnlyWrite(soln->write_only_soln, soln->instance,
	soln->description, soln->running_time, kf);
      break;

    case KHE_SOLN_ORDINARY:

      KmlBegin(kf, "Solution");
      KmlAttribute(kf, "Reference", KheInstanceId(soln->instance));
      if( soln->description != NULL )
	KmlEltPlainText(kf, "Description", soln->description);
      if( soln->running_time >= 0.0 )
	KmlEltFmtText(kf, "RunningTime", "%.1f", soln->running_time);
      event_written = false;
      HaArrayForEach(soln->events_in_soln, es, i)
	KheEventInSolnWrite(es, kf, &event_written);
      if( event_written )
	KmlEnd(kf, "Events");
      if( with_reports )
      {
	KmlBegin(kf, "Report");
	cost = KheSolnCost(soln);
	KmlEltFmtText(kf, "InfeasibilityValue", "%d", KheHardCost(cost));
	KmlEltFmtText(kf, "ObjectiveValue", "%d", KheSoftCost(cost));
	KheSolnResourcesReportWrite(soln, kf);
	KheEventsReportWrite(soln, kf);
	KheEventGroupsReportWrite(soln, kf);
	KmlEnd(kf, "Report");
      }
      KmlEnd(kf, "Solution");
      break;

    default:

      HnAbort("KheSolnWrite internal error 2 (type %d)", soln->type);
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "matchings - zero domain"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SET *KheSolnMatchingZeroDomain(KHE_SOLN soln)                        */
/*                                                                           */
/*  Return a domain containing just 0.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_SET *KheSolnMatchingZeroDomain(KHE_SOLN soln)
{
  return &soln->matching_zero_domain;
}


/*****************************************************************************/
/*                                                                           */
/*  SSET *KheSolnMatchingZeroTimeDomain(KHE_SOLN soln)                       */
/*                                                                           */
/*  Like KheSolnMatchingZeroDomain except it returns an sset.                */
/*                                                                           */
/*****************************************************************************/

SSET *KheSolnMatchingZeroTimeDomain(KHE_SOLN soln)
{
  return &soln->matching_zero_time_domain;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "matchings - setting up"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING KheSolnMatching(KHE_SOLN soln)                              */
/*                                                                           */
/*  Return the matching held by soln, or NULL if no matching.                */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING KheSolnMatching(KHE_SOLN soln)
{
  return soln->matching;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingUpdate(KHE_SOLN soln)                                */
/*                                                                           */
/*  Bring soln's matching up to date, or do nothing if there is no matching. */
/*                                                                           */
/*****************************************************************************/

void KheSolnMatchingUpdate(KHE_SOLN soln)
{
  if( soln->matching != NULL )
    KheMatchingUnmatchedDemandNodeCount(soln->matching);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnMatchingIsActive(KHE_SOLN soln)                              */
/*                                                                           */
/*  Return true if soln's matching is present and active.                    */
/*                                                                           */
/*****************************************************************************/

bool KheSolnMatchingIsActive(KHE_SOLN soln)
{
  return soln->matching != NULL && KheMatchingIsActive(soln->matching);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingBegin(KHE_SOLN soln)                                 */
/*                                                                           */
/*  Begin matching.                                                          */
/*                                                                           */
/*****************************************************************************/

void KheSolnMatchingBegin(KHE_SOLN soln)
{
  KHE_MEET meet;  /* KHE_TASK task; */  int i;
  HnAssert(soln->matching == NULL,
    "KheSolnMatchingBegin: soln already has a matching");
  soln->matching = KheMatchingMake(soln);
  if( DEBUG17 )
    fprintf(stderr, "begin %p->matching = %p\n", (void *) soln,
      (void *) soln->matching);
  HaArrayForEach(soln->meets, meet, i)
    KheMeetMatchingBegin(meet);
  /* *** creates demand nodes, so obsolete
  HaArrayForEach(soln->tasks, task, i)
    KheTaskMatchingBegin(task);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingCheck(KHE_SOLN soln)                                 */
/*                                                                           */
/*  Check soln.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheSolnMatchingMarkCheck(KHE_SOLN soln)
{
  if( soln->matching != NULL )
    KheMatchingMarkCheck(soln->matching);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingEnd(KHE_SOLN soln)                                   */
/*                                                                           */
/*  End matching.                                                            */
/*                                                                           */
/*****************************************************************************/

void KheSolnMatchingEnd(KHE_SOLN soln)
{
  KHE_MEET meet;  int i;  KHE_RESOURCE_IN_SOLN rs;
  HnAssert(soln->matching != NULL, "KheSolnMatchingEnd: no matching");
  HaArrayForEach(soln->meets, meet, i)
    KheMeetMatchingEnd(meet);
  HaArrayForEach(soln->resources_in_soln, rs, i)
    KheResourceInSolnDeleteDemandChunks(rs);
  KheMatchingDelete(soln->matching);
  soln->matching = NULL;
  /* *** old version
  KHE_MEET meet;  KHE_TASK task;  int i;  KHE_RESOURCE_IN_SOLN rs;
  HaArrayForEach(soln->resources_in_soln, rs, i)
    KheResourceInSolnDeleteWorkloadRequirements(rs);
  HaArrayForEach(soln->meets, meet, i)
    KheMeetMatchingEnd(meet);
  HaArrayForEach(soln->tasks, task, i)
    KheTaskMatchingEnd(task);
  KheMatchingDelete(soln->matching);
  soln->matching = NULL;
  if( DEBUG17 )
    fprintf(stderr, "end %p->matching = %p\n", (void *) soln,
      (void *) soln->matching);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnHasMatching(KHE_SOLN soln)                                   */
/*                                                                           */
/*  Return true if soln has a matching.                                      */
/*                                                                           */
/*****************************************************************************/

bool KheSolnHasMatching(KHE_SOLN soln)
{
  return soln->matching != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingSetWeight(KHE_SOLN soln, KHE_COST matching_weight)   */
/*                                                                           */
/*  Set the weight that the matching is to have in the total cost.           */
/*                                                                           */
/*****************************************************************************/

void KheSolnMatchingSetWeight(KHE_SOLN soln, KHE_COST matching_weight)
{
  KHE_RESOURCE_IN_SOLN rs;  KHE_MEET meet;  int i;
  HnAssert(soln->matching != NULL, "KheSolnMatchingSetWeight: no matching");
  if( matching_weight != soln->matching_weight )
  {
    HaArrayForEach(soln->resources_in_soln, rs, i)
      KheResourceInSolnMatchingSetWeight(rs, matching_weight);
    HaArrayForEach(soln->meets, meet, i)
      KheMeetMatchingSetWeight(meet, matching_weight);
    soln->matching_weight = matching_weight;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheSolnMatchingWeight(KHE_SOLN soln)                            */
/*                                                                           */
/*  Return the weight of matching in soln.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheSolnMatchingWeight(KHE_SOLN soln)
{
  HnAssert(soln->matching != NULL, "KheSolnMatchingWeight: no matching");
  return soln->matching_weight;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheSolnMinMatchingWeight(KHE_SOLN soln)                         */
/*                                                                           */
/*  Return a suitable value for the matching_weight field of soln, being     */
/*  the minimum of all the following quantities:                             */
/*                                                                           */
/*    * for each resource r, the sum of the combined weights of the avoid    */
/*      clashes constraints applicable to r;                                 */
/*                                                                           */
/*    * for each event resource er, the sum of the combined weights of the   */
/*      assign resource constraints applicable to er.                        */
/*                                                                           */
/*  If there are no such quantities, the value returned is 0.                */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheSolnMinMatchingWeight(KHE_SOLN soln)
{
  int i, j, k;  KHE_COST weight, res;  KHE_EVENT e;  KHE_EVENT_RESOURCE er;
  KHE_RESOURCE r;  KHE_INSTANCE ins;  KHE_CONSTRAINT c;

  /* avoid clashes constraints */
  ins = KheSolnInstance(soln);
  res = KheCostMax;
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    r = KheInstanceResource(ins, i);
    weight = 0;
    for( j = 0;  j < KheResourceConstraintCount(r);  j++ )
    {
      c = KheResourceConstraint(r, j);
      if( KheConstraintTag(c) == KHE_AVOID_CLASHES_CONSTRAINT_TAG )
	weight += KheConstraintCombinedWeight(c);
    }
    if( weight < res )
      res = weight;
  }

  /* assign resource constraints */
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    for( j = 0;  j < KheEventResourceCount(e);  j++ )
    {
      er = KheEventResource(e, j);
      weight = 0;
      for( k = 0;  k < KheEventResourceConstraintCount(er);  k++ )
      {
	c = KheEventResourceConstraint(er, k);
	if( KheConstraintTag(c) == KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG )
	  weight += KheConstraintCombinedWeight(c);
      }
      if( weight < res )
	res = weight;
    }
  }

  /* final value is res, or 0 if no cases */
  return res == KheCostMax ? 0 : res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_TYPE KheSolnMatchingType(KHE_SOLN soln)                     */
/*                                                                           */
/*  Return the type of soln's matching.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_TYPE KheSolnMatchingType(KHE_SOLN soln)
{
  HnAssert(soln->matching != NULL, "KheSolnMatchingType: no matching");
  return soln->matching_type;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingSetType(KHE_SOLN soln, KHE_MATCHING_TYPE mt)         */
/*                                                                           */
/*  Set the type of soln's matching.                                         */
/*                                                                           */
/*****************************************************************************/

void KheSolnMatchingSetType(KHE_SOLN soln, KHE_MATCHING_TYPE mt)
{
  /* KHE_RESOURCE_IN_SOLN rs; */  KHE_MEET meet;  int i;  KHE_TASK task;
  HnAssert(soln->matching != NULL, "KheSolnMatchingSetType: no matching");
  if( mt != soln->matching_type )
  {
    /* *** workload demand monitors are unaffected by type
    HaArrayForEach(soln->resources_in_soln, rs, i)
      KheResourceInSolnMatchingSetType(rs, mt);
    *** */
    soln->matching_type = mt;  /* must come first */
    HaArrayForEach(soln->meets, meet, i)
      KheMeetMatchingReset(meet);
    HaArrayForEach(soln->tasks, task, i)
      KheTaskMatchingReset(task);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingMarkBegin(KHE_SOLN soln)                             */
/*                                                                           */
/*  Begin a bracketed section of code that might return the matching to      */
/*  its initial state.                                                       */
/*                                                                           */
/*****************************************************************************/

void KheSolnMatchingMarkBegin(KHE_SOLN soln)
{
  if( soln->matching != NULL )
    KheMatchingMarkBegin(soln->matching);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingMarkEnd(KHE_SOLN soln, bool undo)                    */
/*                                                                           */
/*  End a bracketed section of code that might return the matching to        */
/*  its state at the start of the bracketing.  Parameter undo should be      */
/*  true if indeed it did return the matching to that state.                 */
/*                                                                           */
/*****************************************************************************/

void KheSolnMatchingMarkEnd(KHE_SOLN soln, bool undo)
{
  if( soln->matching != NULL )
    KheMatchingMarkEnd(soln->matching, undo);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnSupplyNodeIsOrdinary(KHE_MATCHING_SUPPLY_NODE sn,            */
/*    KHE_MEET *meet, int *meet_offset, KHE_RESOURCE *r)                     */
/*                                                                           */
/*  If sn is an ordinary supply node (which at present it always is), set    */
/*  *meet to the meet it lies in, *meet_offset to its offset in that meet,   */
/*  and *r to the resource it represents, and return true.                   */
/*                                                                           */
/*****************************************************************************/

static bool KheSolnSupplyNodeIsOrdinary(KHE_MATCHING_SUPPLY_NODE sn,
  KHE_MEET *meet, int *meet_offset, KHE_RESOURCE *r)
{
  *meet = (KHE_MEET)
    KheMatchingSupplyChunkImpl(KheMatchingSupplyNodeChunk(sn));
  *meet_offset = KheMeetSupplyNodeOffset(*meet, sn);
  *r = (KHE_RESOURCE) KheMatchingSupplyNodeImpl(sn);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSupplyNodeDebug(KHE_MATCHING_SUPPLY_NODE sn, int verbosity,      */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Show supply node sn onto fp.                                             */
/*                                                                           */
/*****************************************************************************/

void KheSupplyNodeDebug(KHE_MATCHING_SUPPLY_NODE sn, int verbosity,
  int indent, FILE *fp)
{
  KHE_MEET meet;  int meet_offset;  KHE_RESOURCE r;  KHE_TIME t;
  KHE_INSTANCE ins;  KHE_EVENT e;
  if( !KheSolnSupplyNodeIsOrdinary(sn, &meet, &meet_offset, &r) )
    HnAbort("KheSupplyNodeDebug internal error");
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ ");
    ins = KheSolnInstance(KheMeetSoln(meet));
    if( KheMeetIsCycleMeet(meet) )
    {
      t = KheInstanceTime(ins, KheMeetAssignedTimeIndex(meet) + meet_offset);
      fprintf(fp, "%s:%s", KheResourceId(r) != NULL ? KheResourceId(r) : "-",
	KheTimeId(t) != NULL ? KheTimeId(t) : "-");
    }
    else
    {
      e = KheMeetEvent(meet);
      fprintf(fp, "%s:%s+%d", KheResourceId(r) != NULL ? KheResourceId(r) : "-",
	e==NULL ? "~" : KheEventId(e)==NULL ? "-" : KheEventId(e), meet_offset);
    }
    fprintf(fp, " ]");
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingDebug(KHE_SOLN soln, int verbosity,                  */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of soln's matching onto fp with the given verbosity and      */
/*  indent.                                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheDemandNodeDebug(KHE_MATCHING_DEMAND_NODE dn, int verbosity,
  int indent, FILE *fp)
{
  KheMonitorDebug((KHE_MONITOR) dn, verbosity, indent, fp);
}

void KheSolnMatchingDebug(KHE_SOLN soln, int verbosity, int indent, FILE *fp)
{
  if( soln->matching != NULL )
  {
    KheSolnMatchingUpdate(soln);
    KheMatchingDebug(soln->matching, &KheSupplyNodeDebug, &KheDemandNodeDebug,
      verbosity, indent, fp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "matchings - ordinary supply and demand nodes"                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingAttachAllOrdinaryDemandMonitors(KHE_SOLN soln)       */
/*                                                                           */
/*  Make sure all the ordinary demand monitors of soln are attached.         */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnMatchingAttachAllOrdinaryDemandMonitors(KHE_SOLN soln)
{
  KHE_MEET meet;  int i;
  HaArrayForEach(soln->meets, meet, i)
    KheMeetMatchingAttachAllOrdinaryDemandMonitors(meet);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingDetachAllOrdinaryDemandMonitors(KHE_SOLN soln)       */
/*                                                                           */
/*  Make sure all the ordinary demand monitors of soln are detached.         */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnMatchingDetachAllOrdinaryDemandMonitors(KHE_SOLN soln)
{
  KHE_MEET meet;  int i;
  HaArrayForEach(soln->meets, meet, i)
    KheMeetMatchingDetachAllOrdinaryDemandMonitors(meet);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "matchings - workload demand nodes"                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingAddAllWorkloadRequirements(KHE_SOLN soln)            */
/*                                                                           */
/*  Add all workload requirements for soln.                                  */
/*                                                                           */
/*****************************************************************************/

/* see khe_workload.c for the implementation of this function */


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnMatchingWorkloadRequirementCount(KHE_SOLN soln,               */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Return the number of workload requirements associated with r in soln.    */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheSolnMatchingWorkloadRequirementCount(KHE_SOLN soln, KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = KheSolnResourceInSoln(soln, r);
  return KheResourceInSolnWorkloadRequirementCount(rs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingWorkloadRequirement(KHE_SOLN soln,                   */
/*    KHE_RESOURCE r, int i, int *num, KHE_TIME_GROUP *tg)                   */
/*                                                                           */
/*  Return the i'th workload requirement associated with r in soln.          */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnMatchingWorkloadRequirement(KHE_SOLN soln,
  KHE_RESOURCE r, int i, int *num, KHE_TIME_GROUP *tg, KHE_MONITOR *m)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = KheSolnResourceInSoln(soln, r);
  KheResourceInSolnWorkloadRequirement(rs, i, num, tg, m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingBeginWorkloadRequirements(KHE_SOLN soln,             */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Begin a new set of workload requirements for r in soln.                  */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnMatchingBeginWorkloadRequirements(KHE_SOLN soln, KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = KheSolnResourceInSoln(soln, r);
  KheResourceInSolnBeginWorkloadRequirements(rs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingAddWorkloadRequirement(KHE_SOLN soln,                */
/*    KHE_RESOURCE r, int num, KHE_TIME_GROUP tg, KHE_MONITOR m)             */
/*                                                                           */
/*  Add one workload requirement for r in soln.                              */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnMatchingAddWorkloadRequirement(KHE_SOLN soln,
  KHE_RESOURCE r, int num, KHE_TIME_GROUP tg, KHE_MONITOR m)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = KheSolnResourceInSoln(soln, r);
  KheResourceInSolnAddWorkloadRequirement(rs, num, tg, m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingEndWorkloadRequirements(KHE_SOLN soln,               */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  End a new set of workload requirements for r in soln.                    */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnMatchingEndWorkloadRequirements(KHE_SOLN soln, KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = KheSolnResourceInSoln(soln, r);
  KheResourceInSolnEndWorkloadRequirements(rs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingDeleteWorkloadRequirements(KHE_SOLN soln,            */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Delete r's workload requirements and workload demand monitors.           */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnMatchingDeleteWorkloadRequirements(KHE_SOLN soln,
  KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;
  rs = KheSolnResourceInSoln(soln, r);
  KheResourceInSolnDeleteWorkloadRequirements(rs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "matchings - diagnosing failure to match"                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheSolnMatchingDefectCount(KHE_SOLN soln)                            */
/*                                                                           */
/*  Return the number of unmatched demand nodes of soln.                     */
/*                                                                           */
/*****************************************************************************/

int KheSolnMatchingDefectCount(KHE_SOLN soln)
{
  /* ***  I thought this was not here
  HnAssert(soln->matching != NULL, "KheSolnMatchingDefectCount: no matching");
  *** */
  return soln->matching == NULL ? 0 :
    KheMatchingUnmatchedDemandNodeCount(soln->matching);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheSolnMatchingDefect(KHE_SOLN soln, int i)                  */
/*                                                                           */
/*  Return the i'th unmatched demand node of soln.                           */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR KheSolnMatchingDefect(KHE_SOLN soln, int i)
{
  HnAssert(soln->matching != NULL, "KheSolnMatchingDefect: no matching");
  return (KHE_MONITOR) KheMatchingUnmatchedDemandNode(soln->matching, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnMatchingHallSetCount(KHE_SOLN soln)                           */
/*                                                                           */
/*  Return the number of Hall sets.                                          */
/*                                                                           */
/*****************************************************************************/

int KheSolnMatchingHallSetCount(KHE_SOLN soln)
{
  HnAssert(soln->matching != NULL, "KheSolnMatchingHallSetCount: no matching");
  return KheMatchingHallSetCount(soln->matching);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnMatchingHallSetSupplyNodeCount(KHE_SOLN soln, int i)          */
/*                                                                           */
/*  Return the number of supply nodes in the i'th Hall set.                  */
/*                                                                           */
/*****************************************************************************/

int KheSolnMatchingHallSetSupplyNodeCount(KHE_SOLN soln, int i)
{
  KHE_MATCHING_HALL_SET hs;
  hs = KheMatchingHallSet(soln->matching, i);
  return KheMatchingHallSetSupplyNodeCount(hs);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnMatchingHallSetDemandNodeCount(KHE_SOLN soln, int i)          */
/*                                                                           */
/*  Return the number of demand nodes in the i'th Hall set.                  */
/*                                                                           */
/*****************************************************************************/

int KheSolnMatchingHallSetDemandNodeCount(KHE_SOLN soln, int i)
{
  KHE_MATCHING_HALL_SET hs;
  hs = KheMatchingHallSet(soln->matching, i);
  return KheMatchingHallSetDemandNodeCount(hs);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnMatchingHallSetSupplyNodeIsOrdinary(KHE_SOLN soln,           */
/*    int i, int j, KHE_MEET *meet, int *meet_offset, KHE_RESOURCE *r)       */
/*                                                                           */
/*  Extract information about the j'th supply node of the i'th Hall set.     */
/*                                                                           */
/*****************************************************************************/

bool KheSolnMatchingHallSetSupplyNodeIsOrdinary(KHE_SOLN soln,
  int i, int j, KHE_MEET *meet, int *meet_offset, KHE_RESOURCE *r)
{
  KHE_MATCHING_HALL_SET hs;  KHE_MATCHING_SUPPLY_NODE sn;
  hs = KheMatchingHallSet(soln->matching, i);
  sn = KheMatchingHallSetSupplyNode(hs, j);
  return KheSolnSupplyNodeIsOrdinary(sn, meet, meet_offset, r);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingHallSetsDebug(KHE_SOLN soln, int verbosity,          */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of the Hall sets of soln onto fp with the given verbosity    */
/*  and indent.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheSolnMatchingHallSetsDebug(KHE_SOLN soln, int verbosity,
  int indent, FILE *fp)
{
  KHE_MATCHING_HALL_SET hs;  int i;
  HnAssert(soln->matching != NULL, "KheSolnMatchingHallSetsDebug: no matching");
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ %d Hall Sets:\n", indent, "",
      KheSolnMatchingHallSetCount(soln));
    for( i = 0;  i < KheSolnMatchingHallSetCount(soln);  i++ )
    {
      hs = KheMatchingHallSet(soln->matching, i);
      KheMatchingHallSetDebug(hs, &KheSupplyNodeDebug, &KheDemandNodeDebug,
	verbosity, indent + 2, fp);
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingSetCompetitors(KHE_SOLN soln, KHE_MONITOR m)         */
/*                                                                           */
/*  Set the competitors of m.                                                */
/*                                                                           */
/*****************************************************************************/

void KheSolnMatchingSetCompetitors(KHE_SOLN soln, KHE_MONITOR m)
{
  HnAssert(soln->matching != NULL,"KheSolnMatchingSetCompetitors: no matching");
  HnAssert(KheMonitorTag(m) == KHE_ORDINARY_DEMAND_MONITOR_TAG ||
    KheMonitorTag(m) == KHE_WORKLOAD_DEMAND_MONITOR_TAG,
    "KheSolnMatchingSetCompetitors: m is not a demand monitor");
  HnAssert(KheMonitorAttachedToSoln(m),
    "KheSolnMatchingSetCompetitors: m is not attached");
  KheMatchingSetCompetitors(soln->matching, (KHE_MATCHING_DEMAND_NODE) m);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnMatchingCompetitorCount(KHE_SOLN soln)                        */
/*                                                                           */
/*  Return the number of competitors in the current set.                     */
/*                                                                           */
/*****************************************************************************/

int KheSolnMatchingCompetitorCount(KHE_SOLN soln)
{
  HnAssert(soln->matching!=NULL, "KheSolnMatchingCompetitorCount: no matching");
  return KheMatchingCompetitorCount(soln->matching);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheSolnMatchingCompetitor(KHE_SOLN soln, int i)              */
/*                                                                           */
/*  Return the i'th competitor in the current set.                           */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR KheSolnMatchingCompetitor(KHE_SOLN soln, int i)
{
  HnAssert(soln->matching != NULL, "KheSolnMatchingCompetitor: no matching");
  return (KHE_MONITOR) KheMatchingCompetitor(soln->matching, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheSolnCostByType(KHE_SOLN soln, KHE_MONITOR_TAG tag,           */
/*    int *defect_count)                                                     */
/*                                                                           */
/*  Return the cost and number of defects of monitors below soln that        */
/*  have the given tag.                                                      */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheSolnCostByType(KHE_SOLN soln, KHE_MONITOR_TAG tag,
  int *defect_count)
{
  return KheGroupMonitorCostByType((KHE_GROUP_MONITOR) soln, tag,
    defect_count);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnCostByTypeDebug(KHE_SOLN soln, int verbosity,                */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of the cost of soln onto fp with the given indent.           */
/*                                                                           */
/*****************************************************************************/

void KheSolnCostByTypeDebug(KHE_SOLN soln, int verbosity, int indent, FILE *fp)
{
  KheGroupMonitorCostByTypeDebug((KHE_GROUP_MONITOR) soln,
    verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnDoDebug(KHE_SOLN soln, FILE *fp)                             */
/*                                                                           */
/*  Print the header part of a soln debug onto fp.                           */
/*                                                                           */
/*****************************************************************************/

static void KheSolnDoDebug(KHE_SOLN soln, FILE *fp)
{
  fprintf(fp, "Soln(instance \"%s\", type \"%s\", diversifier %d, cost %.5f)",
    KheInstanceId(soln->instance)!=NULL ? KheInstanceId(soln->instance) : "-",
    KheSolnTypeShow(KheSolnType(soln)), soln->diversifier,
    KheCostShow(KheSolnCost(soln)));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnDebug(KHE_SOLN soln, int verbosity, int indent, FILE *fp)    */
/*                                                                           */
/*  Debug print of soln onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

void KheSolnDebug(KHE_SOLN soln, int verbosity, int indent, FILE *fp)
{
  int i;  KHE_EVENT_IN_SOLN es;  KHE_MEET meet;  KHE_RESOURCE_IN_SOLN rs;
  if( verbosity >= 1 )
  {
    if( indent == 0 )
      KheSolnDoDebug(soln, fp);
    else
    {
      fprintf(fp, "%*s[ ", indent, "");
      KheSolnDoDebug(soln, fp);
      if( KheSolnType(soln) != KHE_SOLN_ORDINARY )
	fprintf(fp, " ]\n");
      else
      {
	fprintf(fp, "\n");
	if( verbosity >= 2 )
	{
	  if( verbosity >= 4 )
	  {
	    HaArrayForEach(soln->meets, meet, i)
	      KheMeetDebug(meet, verbosity, indent + 2, fp);
	    HaArrayForEach(soln->events_in_soln, es, i)
	      KheEventInSolnDebug(es, verbosity, indent + 2, fp);
	    HaArrayForEach(soln->resources_in_soln, rs, i)
	      KheResourceInSolnDebug(rs, verbosity, indent + 2, fp);
	  }
	  if( verbosity >= 2 )
	  {
	    fprintf(fp, "%*s  defects:\n", indent, "");
	    KheGroupMonitorDefectDebug((KHE_GROUP_MONITOR) soln, 2,
	      indent + 2, fp);
	  }
	  KheSolnCostByTypeDebug(soln, 2, indent + 2, fp);
	}
	fprintf(fp, "\n%*s]\n", indent, "");
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "free lists"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheSolnGetMeetFromFreeList(KHE_SOLN soln)                       */
/*  void KheSolnAddMeetToFreeList(KHE_SOLN soln, KHE_MEET meet)              */
/*                                                                           */
/*  Return a new meet from soln's free list, or NULL if none; and add one.   */
/*                                                                           */
/*****************************************************************************/

KHE_MEET KheSolnGetMeetFromFreeList(KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_meets) > 0 )
  {
    if( DEBUG16 )
      fprintf(stderr, "  (e) %p->free_meets: items %p, count %d, length %d\n",
	(void *) soln, (void *) soln->free_meets.items,
	soln->free_meets.count, soln->free_meets.length);
    return HaArrayLastAndDelete(soln->free_meets);
  }
  else
    return NULL;
}

void KheSolnAddMeetToFreeList(KHE_SOLN soln, KHE_MEET meet)
{
  HaArrayAddLast(soln->free_meets, meet);
  if( DEBUG16 )
    fprintf(stderr, "  (f) %p->free_meets: items %p, count %d, length %d\n",
      (void *) soln, (void *) soln->free_meets.items,
      soln->free_meets.count, soln->free_meets.length);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET_BOUND KheSolnGetMeetBoundFromFreeList(KHE_SOLN soln)            */
/*  void KheSolnAddMeetBoundToFreeList(KHE_SOLN soln, KHE_MEET_BOUND mb)     */
/*                                                                           */
/*  Return a meet bound object from soln's free list, or NULL if none.       */
/*                                                                           */
/*****************************************************************************/

KHE_MEET_BOUND KheSolnGetMeetBoundFromFreeList(KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_meet_bounds) > 0 )
    return HaArrayLastAndDelete(soln->free_meet_bounds);
  else
    return NULL;
}

void KheSolnAddMeetBoundToFreeList(KHE_SOLN soln, KHE_MEET_BOUND mb)
{
  HaArrayAddLast(soln->free_meet_bounds, mb);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheSolnGetTaskFromFreeList(KHE_SOLN soln)                       */
/*  void KheSolnAddTaskToFreeList(KHE_SOLN soln, KHE_TASK task)              */
/*                                                                           */
/*  Return a new task from soln's free list, or NULL if none.                */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheSolnGetTaskFromFreeList(KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_tasks) > 0 )
    return HaArrayLastAndDelete(soln->free_tasks);
  else
    return NULL;
}

void KheSolnAddTaskToFreeList(KHE_SOLN soln, KHE_TASK task)
{
  HaArrayAddLast(soln->free_tasks, task);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_BOUND KheSolnGetTaskBoundFromFreeList(KHE_SOLN soln)            */
/*  void KheSolnAddTaskBoundToFreeList(KHE_SOLN soln, KHE_TASK_BOUND tb)     */
/*                                                                           */
/*  Return a task bound object from soln's free list, or NULL if none.       */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_BOUND KheSolnGetTaskBoundFromFreeList(KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_task_bounds) > 0 )
    return HaArrayLastAndDelete(soln->free_task_bounds);
  else
    return NULL;
}

void KheSolnAddTaskBoundToFreeList(KHE_SOLN soln, KHE_TASK_BOUND tb)
{
  HaArrayAddLast(soln->free_task_bounds, tb);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MARK KheSolnTakeMarkFromFreeList(KHE_SOLN soln)                      */
/*  void KheSolnAddMarkToFreeList(KHE_SOLN soln, KHE_MARK mark)              */
/*                                                                           */
/*  Add mark to the free list.                                               */
/*                                                                           */
/*****************************************************************************/

KHE_MARK KheSolnTakeMarkFromFreeList(KHE_SOLN soln)
{
  return HaArrayCount(soln->free_marks) == 0 ? NULL :
    HaArrayLastAndDelete(soln->free_marks);
}

void KheSolnAddMarkToFreeList(KHE_SOLN soln, KHE_MARK mark)
{
  HaArrayAddLast(soln->free_marks, mark);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PATH KheSolnTakePathFromFreeList(KHE_SOLN soln)                      */
/*  void KheSolnAddPathToFreeList(KHE_SOLN soln, KHE_PATH path)              */
/*                                                                           */
/*  Add path to soln's free list.                                            */
/*                                                                           */
/*****************************************************************************/

KHE_PATH KheSolnTakePathFromFreeList(KHE_SOLN soln)
{
  return HaArrayCount(soln->free_paths) == 0 ? NULL :
    HaArrayLastAndDelete(soln->free_paths);
}

void KheSolnAddPathToFreeList(KHE_SOLN soln, KHE_PATH path)
{
  HaArrayAddLast(soln->free_paths, path);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE KheSolnGetNodeFromFreeList(KHE_SOLN soln)                       */
/*  void KheSolnAddNodeToFreeList(KHE_SOLN soln, KHE_NODE node)              */
/*                                                                           */
/*  Return a new node from soln's free list, or NULL if none.                */
/*                                                                           */
/*****************************************************************************/

KHE_NODE KheSolnGetNodeFromFreeList(KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_nodes) > 0 )
    return HaArrayLastAndDelete(soln->free_nodes);
  else
    return NULL;
}

void KheSolnAddNodeToFreeList(KHE_SOLN soln, KHE_NODE node)
{
  HaArrayAddLast(soln->free_nodes, node);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LAYER KheSolnGetLayerFromFreeList(KHE_SOLN soln)                     */
/*  void KheSolnAddLayerToFreeList(KHE_SOLN soln, KHE_LAYER layer)           */
/*                                                                           */
/*  Return a new layer from soln's free list, or NULL if none.               */
/*                                                                           */
/*****************************************************************************/

KHE_LAYER KheSolnGetLayerFromFreeList(KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_layers) > 0 )
    return HaArrayLastAndDelete(soln->free_layers);
  else
    return NULL;
}

void KheSolnAddLayerToFreeList(KHE_SOLN soln, KHE_LAYER layer)
{
  HaArrayAddLast(soln->free_layers, layer);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ZONE KheSolnGetZoneFromFreeList(KHE_SOLN soln)                       */
/*  void KheSolnAddZoneToFreeList(KHE_SOLN soln, KHE_ZONE zone)              */
/*                                                                           */
/*  Return a new zone from soln's free list, or NULL if none.                */
/*                                                                           */
/*****************************************************************************/

KHE_ZONE KheSolnGetZoneFromFreeList(KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_zones) > 0 )
    return HaArrayLastAndDelete(soln->free_zones);
  else
    return NULL;
}

void KheSolnAddZoneToFreeList(KHE_SOLN soln, KHE_ZONE zone)
{
  HaArrayAddLast(soln->free_zones, zone);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASKING KheSolnGetTaskingFromFreeList(KHE_SOLN soln)                 */
/*  void KheSolnAddTaskingToFreeList(KHE_SOLN soln, KHE_TASKING tasking)     */
/*                                                                           */
/*  Return a new tasking from soln's free list, or NULL if none.             */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_TASKING KheSolnGetTaskingFromFreeList(KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_taskings) > 0 )
    return HaArrayLastAndDelete(soln->free_taskings);
  else
    return NULL;
}

void KheSolnAddTaskingToFreeList(KHE_SOLN soln, KHE_TASKING tasking)
{
  HaArrayAddLast(soln->free_taskings, tasking);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_SET KheSolnGetTaskSetFromFreeList(KHE_SOLN soln)                */
/*  void KheSolnAddTaskSetToFreeList(KHE_SOLN soln, KHE_TASK_SET ts)         */
/*                                                                           */
/*  Return a new task set from soln's free list, or NULL if none.            */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_SET KheSolnGetTaskSetFromFreeList(KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_task_sets) > 0 )
    return HaArrayLastAndDelete(soln->free_task_sets);
  else
    return NULL;
}

void KheSolnAddTaskSetToFreeList(KHE_SOLN soln, KHE_TASK_SET ts)
{
  HaArrayAddLast(soln->free_task_sets, ts);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET_SET KheSolnGetMeetSetFromFreeList(KHE_SOLN soln)                */
/*  void KheSolnAddMeetSetToFreeList(KHE_SOLN soln, KHE_MEET_SET ts)         */
/*                                                                           */
/*  Return a new meet set from soln's free list, or NULL if none.            */
/*                                                                           */
/*****************************************************************************/

KHE_MEET_SET KheSolnGetMeetSetFromFreeList(KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_meet_sets) > 0 )
    return HaArrayLastAndDelete(soln->free_meet_sets);
  else
    return NULL;
}

void KheSolnAddMeetSetToFreeList(KHE_SOLN soln, KHE_MEET_SET ts)
{
  HaArrayAddLast(soln->free_meet_sets, ts);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME_MAKE KheSolnGetFrameMakeFromFreeList(KHE_SOLN soln)            */
/*  void KheSolnAddFrameMakeToFreeList(KHE_SOLN soln, KHE_FRAME_MAKE fm)     */
/*                                                                           */
/*  Return a new frame make from soln's free list, or NULL if none.          */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_FRAME_MAKE KheSolnGetFrameMakeFromFreeList(KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_frame_makes) > 0 )
    return HaArrayLastAndDelete(soln->free_frame_makes);
  else
    return NULL;
}

void KheSolnAddFrameMakeToFreeList(KHE_SOLN soln, KHE_FRAME_MAKE fm)
{
  HaArrayAddLast(soln->free_frame_makes, fm);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME_WORKLOAD KheSolnGetFrameWorkloadFromFreeList(KHE_SOLN soln)    */
/*  void KheSolnAddFrameWorkloadToFreeList(KHE_SOLN soln,                    */
/*    KHE_FRAME_WORKLOAD fw)                                                 */
/*                                                                           */
/*  Return a new frame workload from soln's free list, or NULL if none.      */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_FRAME_WORKLOAD KheSolnGetFrameWorkloadFromFreeList(KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_frame_workloads) > 0 )
    return HaArrayLastAndDelete(soln->free_frame_workloads);
  else
    return NULL;
}

void KheSolnAddFrameWorkloadToFreeList(KHE_SOLN soln, KHE_FRAME_WORKLOAD fw)
{
  HaArrayAddLast(soln->free_frame_workloads, fw);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_LINK KheSolnGetMonitorLinkFromFreeList(KHE_SOLN soln)        */
/*  void KheSolnAddMonitorLinkToFreeList(KHE_SOLN soln, KHE_MONITOR_LINK ml) */
/*                                                                           */
/*  Return a new monitor link object from soln's free list, or NULL if none. */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR_LINK KheSolnGetMonitorLinkFromFreeList(KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_monitor_links) > 0 )
    return HaArrayLastAndDelete(soln->free_monitor_links);
  else
    return NULL;
}

void KheSolnAddMonitorLinkToFreeList(KHE_SOLN soln, KHE_MONITOR_LINK ml)
{
  HaArrayAddLast(soln->free_monitor_links, ml);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP_MONITOR KheSolnGetGroupMonitorFromFreeList(KHE_SOLN soln)      */
/*  void KheSolnAddGroupMonitorToFreeList(KHE_SOLN soln,KHE_GROUP_MONITOR gm)*/
/*                                                                           */
/*  Return a new group monitor object from soln's free list, or NULL if none.*/
/*                                                                           */
/*****************************************************************************/

KHE_GROUP_MONITOR KheSolnGetGroupMonitorFromFreeList(KHE_SOLN soln)
{
  KHE_GROUP_MONITOR res;
  if( DEBUG13 )
    fprintf(stderr, "[ KheSolnGetGroupMonitorFromFreeList(%p), len = %d\n",
      (void *) soln, HaArrayCount(soln->free_group_monitors));
  if( HaArrayCount(soln->free_group_monitors) > 0 )
    res = HaArrayLastAndDelete(soln->free_group_monitors);
  else
    res = NULL;
  if( DEBUG13 )
    fprintf(stderr, "] KheSolnGetGroupMonitorFromFreeList ret %p, len = %d\n",
      (void *) res, HaArrayCount(soln->free_group_monitors));
  return res;
}

void KheSolnAddGroupMonitorToFreeList(KHE_SOLN soln, KHE_GROUP_MONITOR gm)
{
  if( DEBUG13 )
    fprintf(stderr, "[ KheSolnAddGroupMonitorToFreeList(%p, %p), len = %d\n",
      (void *) soln, (void *) gm, HaArrayCount(soln->free_group_monitors));
  if( HaArrayCount(soln->free_group_monitors) > 0 )
    HnAssert(HaArrayLast(soln->free_group_monitors) != gm,
      "KheSolnAddGroupMonitorToFreeList internal error (gm deleted twice)");
  HaArrayAddLast(soln->free_group_monitors, gm);
  if( DEBUG13 )
    fprintf(stderr, "] KheSolnAddGroupMonitorToFreeList ret, len = %d\n",
      HaArrayCount(soln->free_group_monitors));
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TRACE KheSolnGetTraceFromFreeList(KHE_SOLN soln)                     */
/*  void KheSolnAddTraceToFreeList(KHE_SOLN soln, KHE_TRACE t)               */
/*                                                                           */
/*  Return a new trace from soln's free list, or NULL if none.               */
/*                                                                           */
/*****************************************************************************/

KHE_TRACE KheSolnGetTraceFromFreeList(KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_traces) > 0 )
    return HaArrayLastAndDelete(soln->free_traces);
  else
    return NULL;
}

void KheSolnAddTraceToFreeList(KHE_SOLN soln, KHE_TRACE t)
{
  HaArrayAddLast(soln->free_traces, t);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_WORKLOAD_REQUIREMENT KheSolnGetWorkloadRequirementFromFreeList(      */
/*    KHE_SOLN soln)                                                         */
/*  void KheSolnAddWorkloadRequirementToFreeList(KHE_SOLN soln,              */
/*    KHE_WORKLOAD_REQUIREMENT wr)                                           */
/*                                                                           */
/*  Return a new workload requirement from soln's free list, NULL if none.   */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_WORKLOAD_REQUIREMENT KheSolnGetWorkloadRequirementFromFreeList(
  KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_workload_requirements) > 0 )
    return HaArrayLastAndDelete(soln->free_workload_requirements);
  else
    return NULL;
}
*** */

/* ***
void KheSolnAddWorkloadRequirementToFreeList(KHE_SOLN soln,
  KHE_WORKLOAD_REQUIREMENT wr)
{
  HaArrayAddLast(soln->free_workload_requirements, wr);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING KheSolnGetMatchingFromFreeList(KHE_SOLN soln)               */
/*  void KheSolnAddMatchingToFreeList(KHE_SOLN soln, KHE_MATCHING m)         */
/*                                                                           */
/*  Get a matching from the free list, or return NULL if none.               */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING KheSolnGetMatchingFromFreeList(KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_matchings) > 0 )
    return HaArrayLastAndDelete(soln->free_matchings);
  else
    return NULL;
}

void KheSolnAddMatchingToFreeList(KHE_SOLN soln, KHE_MATCHING m)
{
  HaArrayAddLast(soln->free_matchings, m);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_SUPPLY_CHUNK KheSolnGetMatchingSupplyChunkFromFreeList(     */
/*    KHE_SOLN soln)                                                         */
/*  void KheSolnAddMatchingSupplyChunkToFreeList(KHE_SOLN soln,              */
/*    KHE_MATCHING_SUPPLY_CHUNK msc)                                         */
/*                                                                           */
/*  Get a matching supply chunk from the free list, or return NULL if none.  */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_SUPPLY_CHUNK KheSolnGetMatchingSupplyChunkFromFreeList(
  KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_matching_supply_chunks) > 0 )
    return HaArrayLastAndDelete(soln->free_matching_supply_chunks);
  else
    return NULL;
}

void KheSolnAddMatchingSupplyChunkToFreeList(KHE_SOLN soln,
  KHE_MATCHING_SUPPLY_CHUNK msc)
{
  HaArrayAddLast(soln->free_matching_supply_chunks, msc);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_SUPPLY_CHUNK KheSolnMatchingMakeOrdinarySupplyChunk(        */
/*    KHE_SOLN soln, KHE_MEET meet)                                          */
/*                                                                           */
/*  Return a fresh ordinary supply chunk, with one node for each resource    */
/*  of the instance, either taken from a free list or made from scratch.     */
/*  Set the impl field of the supply chunk to meet.                          */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_MATCHING_SUPPLY_CHUNK KheSolnMatchingMakeOrdinarySupplyChunk(
  KHE_SOLN soln, KHE_MEET meet)
{
  KHE_MATCHING_SUPPLY_CHUNK res;  KHE_INSTANCE ins;  int i;
  if( HaArrayCount(soln->free_matching_supply_chunks) > 0 )
    res = HaArrayLastAndDelete(soln->free_matching_supply_chunks);
  else
  {
    ins = KheSolnInstance(soln);
    res = KheMatchingSupply ChunkMake(soln->matching, NULL);
    for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
      KheMatchingSup plyNodeMake(res, (void *) KheInstanceResource(ins, i));
  }
  KheMatchingSupplyChunkSetImpl(res, meet);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMatchingAddOrdinarySupplyChunkToFreeList(KHE_SOLN soln,      */
/*    KHE_MATCHING_SUPPLY_CHUNK sc)                                          */
/*                                                                           */
/*  Save sc for reuse later.                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnMatchingAddOrdinarySupplyChunkToFreeList(KHE_SOLN soln,
  KHE_MATCHING_SUPPLY_CHUNK sc)
{
  HaArrayAddLast(soln->free_matching_supply_chunks, sc);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_SUPPLY_NODE KheSolnGetMatchingSupplyNodeFromFreeList(       */
/*    KHE_SOLN soln)                                                         */
/*  void KheSolnAddMatchingSupplyNodeToFreeList(KHE_SOLN soln,               */
/*    KHE_MATCHING_SUPPLY_NODE msn)                                          */
/*                                                                           */
/*  Get a matching supply node from the free list, or return NULL if none.   */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_SUPPLY_NODE KheSolnGetMatchingSupplyNodeFromFreeList(
  KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_matching_supply_nodes) > 0 )
    return HaArrayLastAndDelete(soln->free_matching_supply_nodes);
  else
    return NULL;
}

void KheSolnAddMatchingSupplyNodeToFreeList(KHE_SOLN soln,
  KHE_MATCHING_SUPPLY_NODE msn)
{
  HaArrayAddLast(soln->free_matching_supply_nodes, msn);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_DEMAND_CHUNK KheSolnGetMatchingDemandChunkFromFreeList(     */
/*    KHE_SOLN soln)                                                         */
/*  void KheSolnAddMatchingDemandChunkToFreeList(KHE_SOLN soln,              */
/*    KHE_MATCHING_DEMAND_CHUNK mdc)                                         */
/*                                                                           */
/*  Get a matching demand chunk from the free list, or return NULL if none.  */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_DEMAND_CHUNK KheSolnGetMatchingDemandChunkFromFreeList(
  KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_matching_demand_chunks) > 0 )
    return HaArrayLastAndDelete(soln->free_matching_demand_chunks);
  else
    return NULL;
}

void KheSolnAddMatchingDemandChunkToFreeList(KHE_SOLN soln,
  KHE_MATCHING_DEMAND_CHUNK mdc)
{
  HaArrayAddLast(soln->free_matching_demand_chunks, mdc);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ORDINARY_DEMAND_MONITOR KheSolnGetOrdinaryDemandMonitorFromFreeList( */
/*    KHE_SOLN soln)                                                         */
/*  void KheSolnAddOrdinaryDemandMonitorToFreeList(KHE_SOLN soln,            */
/*    KHE_ORDINARY_DEMAND_MONITOR odm)                                       */
/*                                                                           */
/*  Get an ordinary demand monitor from the free list, or NULL if none.      */
/*                                                                           */
/*****************************************************************************/

KHE_ORDINARY_DEMAND_MONITOR KheSolnGetOrdinaryDemandMonitorFromFreeList(
  KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_ordinary_demand_monitors) > 0 )
    return HaArrayLastAndDelete(soln->free_ordinary_demand_monitors);
  else
    return NULL;
}


void KheSolnAddOrdinaryDemandMonitorToFreeList(KHE_SOLN soln,
  KHE_ORDINARY_DEMAND_MONITOR odm)
{
  HaArrayAddLast(soln->free_ordinary_demand_monitors, odm);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_WORKLOAD_DEMAND_MONITOR KheSolnGetWorkloadDemandMonitorFromFreeList( */
/*    KHE_SOLN soln)                                                         */
/*  void KheSolnAddWorkloadDemandMonitorToFreeList(KHE_SOLN soln,            */
/*    KHE_WORKLOAD_DEMAND_MONITOR wdm)                                       */
/*                                                                           */
/*  Get a workload demand monitor from the free list, or NULL if none.       */
/*                                                                           */
/*****************************************************************************/

KHE_WORKLOAD_DEMAND_MONITOR KheSolnGetWorkloadDemandMonitorFromFreeList(
  KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_workload_demand_monitors) > 0 )
    return HaArrayLastAndDelete(soln->free_workload_demand_monitors);
  else
    return NULL;
}

void KheSolnAddWorkloadDemandMonitorToFreeList(KHE_SOLN soln,
  KHE_WORKLOAD_DEMAND_MONITOR wdm)
{
  HaArrayAddLast(soln->free_workload_demand_monitors, wdm);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_HALL_SET KheSolnGetMatchingHallSetFromFreeList(             */
/*    KHE_SOLN soln)                                                         */
/*  void KheSolnAddMatchingHallSetToFreeList(KHE_SOLN soln,                  */
/*    KHE_MATCHING_HALL_SET mhs)                                             */
/*                                                                           */
/*  Get a matching Hall set from the free list, or NULL if none.             */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_HALL_SET KheSolnGetMatchingHallSetFromFreeList(KHE_SOLN soln)
{
  if( HaArrayCount(soln->free_matching_hall_sets) > 0 )
    return HaArrayLastAndDelete(soln->free_matching_hall_sets);
  else
    return NULL;
}

void KheSolnAddMatchingHallSetToFreeList(KHE_SOLN soln,
  KHE_MATCHING_HALL_SET mhs)
{
  HaArrayAddLast(soln->free_matching_hall_sets, mhs);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "obsolete below here"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachAssignResourceConstraintMonitors(               */
/*    KHE_SOLN soln, KHE_ASSIGN_RESOURCE_CONSTRAINT c)                       */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khs_assign_resource_constraint.c
static void KheSolnMakeAndAttachAssignResourceConstraintMonitors(
  KHE_SOLN soln, KHE_ASSIGN_RESOURCE_CONSTRAINT c)
{
  int i;  KHE_EVENT_RESOURCE er;
  KHE_EVENT_RESOURCE_IN_SOLN ers;  KHE_ASSIGN_RESOURCE_MONITOR m;
  for( i = 0;  i < KheAssignResourceConstraintEventResourceCount(c);  i++ )
  {
    er = KheAssignResourceConstraintEventResource(c, i);
    ers = KheSolnEventResourceInSoln(soln, er);
    m = KheAssignResourceMonitorMake(ers, c);
    if( DEBUG5 )
      fprintf(stderr, "new KheAssignResourceMonitor %p (attached %d)\n",
	(void *) m, (int) KheMonitorAttachedToSoln((KHE_MONITOR) m));
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachAssignTimeConstraintMonitors(                   */
/*    KHE_SOLN soln, KHE_ASSIGN_TIME_CONSTRAINT c)                           */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khs_assign_time_constraint.c
static void KheSolnMakeAndAttachAssignTimeConstraintMonitors(
  KHE_SOLN soln, KHE_ASSIGN_TIME_CONSTRAINT c)
{
  int i, j;  KHE_EVENT_GROUP eg;  KHE_EVENT e;
  KHE_EVENT_IN_SOLN es;  KHE_ASSIGN_TIME_MONITOR m;
  for( i = 0;  i < KheAssignTimeConstraintEventCount(c);  i++ )
  {
    e = KheAssignTimeConstraintEvent(c, i);
    es = HaArray(soln->events_in_soln, KheEventIndex(e));
    m = KheAssignTimeMonitorMake(es, c);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
  for( i = 0;  i < KheAssignTimeConstraintEventGroupCount(c);  i++ )
  {
    eg = KheAssignTimeConstraintEventGroup(c, i);
    for( j = 0;  j < KheEventGroupEventCount(eg);  j++ )
    {
      e = KheEventGroupEvent(eg, j);
      es = HaArray(soln->events_in_soln, KheEventIndex(e));
      m = KheAssignTimeMonitorMake(es, c);
      KheMonitorAttachToSoln((KHE_MONITOR) m);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachSplitEventsConstraintMonitors(                  */
/*    KHE_SOLN soln, KHE_SPLIT_EVENTS_CONSTRAINT c)                          */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_split_events_constraint.c
static void KheSolnMakeAndAttachSplitEventsConstraintMonitors(
  KHE_SOLN soln, KHE_SPLIT_EVENTS_CONSTRAINT c)
{
  int i, j;  KHE_EVENT_GROUP eg;  KHE_EVENT e;
  KHE_EVENT_IN_SOLN es;  KHE_SPLIT_EVENTS_MONITOR m;
  if( DEBUG6 )
    fprintf(stderr, "[ KheSolnMakeAndAttachSplitEventsConstraintMonitors()\n");
  for( i = 0;  i < KheSplitEventsConstraintEventCount(c);  i++ )
  {
    e = KheSplitEventsConstraintEvent(c, i);
    if( DEBUG6 )
    {
      fprintf(stderr, "  event %d ", KheEventIndex(e));
      KheEventDebug(e, 1, 0, stderr);
    }
    es = HaArray(soln->events_in_soln, KheEventIndex(e));
    m = KheSplitEventsMonitorMake(es, c);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
  for( i = 0;  i < KheSplitEventsConstraintEventGroupCount(c);  i++ )
  {
    eg = KheSplitEventsConstraintEventGroup(c, i);
    for( j = 0;  j < KheEventGroupEventCount(eg);  j++ )
    {
      e = KheEventGroupEvent(eg, j);
      if( DEBUG6 )
      {
	fprintf(stderr, "    event group event %d ", KheEventIndex(e));
	KheEventDebug(e, 1, 0, stderr);
      }
      es = HaArray(soln->events_in_soln, KheEventIndex(e));
      m = KheSplitEventsMonitorMake(es, c);
      KheMonitorAttachToSoln((KHE_MONITOR) m);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
    }
  }
  if( DEBUG6 )
    fprintf(stderr, "] KheSolnMakeAndAttachSplitEventsConstraintMonitors()\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachDistributeSplitEventsConstraintMonitors(        */
/*    KHE_SOLN soln, KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)               */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_distribute_split_events_constraint.c
static void KheSolnMakeAndAttachDistributeSplitEventsConstraintMonitors(
  KHE_SOLN soln, KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)
{
  int i, j;  KHE_EVENT_GROUP eg;  KHE_EVENT e;
  KHE_EVENT_IN_SOLN es;  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m;
  for( i = 0;  i < KheDistributeSplitEventsConstraintEventCount(c);  i++ )
  {
    e = KheDistributeSplitEventsConstraintEvent(c, i);
    es = HaArray(soln->events_in_soln, KheEventIndex(e));
    m = KheDistributeSplitEventsMonitorMake(es, c);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
  for( i = 0;  i < KheDistributeSplitEventsConstraintEventGroupCount(c);  i++ )
  {
    eg = KheDistributeSplitEventsConstraintEventGroup(c, i);
    for( j = 0;  j < KheEventGroupEventCount(eg);  j++ )
    {
      e = KheEventGroupEvent(eg, j);
      es = HaArray(soln->events_in_soln, KheEventIndex(e));
      m = KheDistributeSplitEventsMonitorMake(es, c);
      KheMonitorAttachToSoln((KHE_MONITOR) m);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachPreferResourcesConstraintMonitors(              */
/*    KHE_SOLN soln, KHE_PREFER_RESOURCES_CONSTRAINT c)                      */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSolnMakeAndAttachPreferResourcesConstraintMonitors(
  KHE_SOLN soln, KHE_PREFER_RESOURCES_CONSTRAINT c)
{
  int i;  KHE_EVENT_RESOURCE er;  KHE_EVENT_RESOURCE_IN_SOLN ers;
  KHE_PREFER_RESOURCES_MONITOR m;
  for( i = 0;  i < KhePreferResourcesConstraintEventResourceCount(c);  i++ )
  {
    er = KhePreferResourcesConstraintEventResource(c, i);
    ers = KheSolnEventResourceInSoln(soln, er);
    m = KhePreferR esourcesMonitorMake(ers, c);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachPreferTimesConstraintMonitors(                  */
/*    KHE_SOLN soln, KHE_PREFER_TIMES_CONSTRAINT c)                          */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_prefer_times_constraint.c
static void KheSolnMakeAndAttachPreferTimesConstraintMonitors(
  KHE_SOLN soln, KHE_PREFER_TIMES_CONSTRAINT c)
{
  int i, j;  KHE_EVENT_GROUP eg;  KHE_EVENT e;
  KHE_EVENT_IN_SOLN es;  KHE_PREFER_TIMES_MONITOR m;
  for( i = 0;  i < KhePreferTimesConstraintEventCount(c);  i++ )
  {
    e = KhePreferTimesConstraintEvent(c, i);
    es = HaArray(soln->events_in_soln, KheEventIndex(e));
    m = KhePreferTimesMonitorMake(es, c);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
  for( i = 0;  i < KhePreferTimesConstraintEventGroupCount(c);  i++ )
  {
    eg = KhePreferTimesConstraintEventGroup(c, i);
    for( j = 0;  j < KheEventGroupEventCount(eg);  j++ )
    {
      e = KheEventGroupEvent(eg, j);
      es = HaArray(soln->events_in_soln, KheEventIndex(e));
      m = KhePreferTimesMonitorMake(es, c);
      KheMonitorAttachToSoln((KHE_MONITOR) m);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachAvoidSplitAssignmentsConstraintMonitors(        */
/*    KHE_SOLN soln, KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c)               */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_avoid_split_assignments_constraint.c
static void KheSolnMakeAndAttachAvoidSplitAssignmentsConstraintMonitors(
  KHE_SOLN soln, KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c)
{
  int i;  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m;
  for( i = 0;  i < KheAvoidSplitAssignmentsConstraintEventGroupCount(c);  i++ )
  {
    m = KheAvoidSplitAssignmentsMonitorMake(soln, c, i);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachSpreadEventsConstraintMonitors(                 */
/*    KHE_SOLN soln, KHE_SPREAD_EVENTS_CONSTRAINT c)                         */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_spread_events_constraint.c
static void KheSolnMakeAndAttachSpreadEventsConstraintMonitors(
  KHE_SOLN soln, KHE_SPREAD_EVENTS_CONSTRAINT c)
{
  int i;  KHE_EVENT_GROUP eg;  KHE_SPREAD_EVENTS_MONITOR m;
  for( i = 0;  i < KheSpreadEventsConstraintEventGroupCount(c);  i++ )
  {
    eg = KheSpreadEventsConstraintEventGroup(c, i);
    m = KheSpreadEventsMonitorMake(soln, c, eg);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachLinkEventsConstraintMonitors(                   */
/*    KHE_SOLN soln, KHE_LINK_EVENTS_CONSTRAINT c)                           */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_link_events_constraint.c
static void KheSolnMakeAndAttachLinkEventsConstraintMonitors(
  KHE_SOLN soln, KHE_LINK_EVENTS_CONSTRAINT c)
{
  int i;  KHE_EVENT_GROUP eg;  KHE_LINK_EVENTS_MONITOR m;
  for( i = 0;  i < KheLinkEventsConstraintEventGroupCount(c);  i++ )
  {
    eg = KheLinkEventsConstraintEventGroup(c, i);
    m = KheLinkEventsMonitorMake(soln, c, eg);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachOrderEventsConstraintMonitors(                  */
/*    KHE_SOLN soln, KHE_ORDER_EVENTS_CONSTRAINT c)                          */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_order_events_constraint.c
static void KheSolnMakeAndAttachOrderEventsConstraintMonitors(
  KHE_SOLN soln, KHE_ORDER_EVENTS_CONSTRAINT c)
{
  int i, min_sep, max_sep;  KHE_EVENT e1, e2;  KHE_ORDER_EVENTS_MONITOR m;
  for( i = 0;  i < KheOrderEventsConstraintEventPairCount(c);  i++ )
  {
    e1 = KheOrderEventsConstraintFirstEvent(c, i);
    e2 = KheOrderEventsConstraintSecondEvent(c, i);
    min_sep = KheOrderEventsConstraintMinSeparation(c, i);
    max_sep = KheOrderEventsConstraintMaxSeparation(c, i);
    m = KheOrderEventsMonitorMake(soln, c, e1, e2, min_sep, max_sep);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachAvoidClashesConstraintMonitors(                 */
/*    KHE_SOLN soln, KHE_AVOID_CLASHES_CONSTRAINT c)                         */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_avoid_clashes_constraint.c
static void KheSolnMakeAndAttachAvoidClashesConstraintMonitors(
  KHE_SOLN soln, KHE_AVOID_CLASHES_CONSTRAINT c)
{
  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;  KHE_RESOURCE_IN_SOLN rs;
  KHE_AVOID_CLASHES_MONITOR m;
  for( i = 0;  i < KheAvoidClashesConstraintResourceGroupCount(c);  i++ )
  {
    rg = KheAvoidClashesConstraintResourceGroup(c, i);
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
    {
      r = KheResourceGroupResource(rg, j);
      rs = KheSolnResourceInSoln(soln, r);
      m = KheAvoidClashesMonitorMake(rs, c);
      KheMonitorAttachToSoln((KHE_MONITOR) m);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
    }
  }
  for( i = 0;  i < KheAvoidClashesConstraintResourceCount(c);  i++ )
  {
    r = KheAvoidClashesConstraintResource(c, i);
    rs = KheSolnResourceInSoln(soln, r);
    m = KheAvoidClashesMonitorMake(rs, c);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachAvoidUnavailableTimesConstraintMonitors(        */
/*    KHE_SOLN soln, KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)               */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_avoid_unavailable_times_constraint.c
static void KheSolnMakeAndAttachAvoidUnavailableTimesConstraintMonitors(
  KHE_SOLN soln, KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)
{
  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;  KHE_RESOURCE_IN_SOLN rs;
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m;
  for( i=0; i < KheAvoidUnavailableTimesConstraintResourceGroupCount(c); i++ )
  {
    rg = KheAvoidUnavailableTimesConstraintResourceGroup(c, i);
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
    {
      r = KheResourceGroupResource(rg, j);
      rs = KheSolnResourceInSoln(soln, r);
      m = KheAvoidUnavailableTimesMonitorMake(rs, c);
      KheMonitorAttachToSoln((KHE_MONITOR) m);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
    }
  }
  for( i = 0;  i < KheAvoidUnavailableTimesConstraintResourceCount(c);  i++ )
  {
    r = KheAvoidUnavailableTimesConstraintResource(c, i);
    rs = KheSolnResourceInSoln(soln, r);
    m = KheAvoidUnavailableTimesMonitorMake(rs, c);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachLimitIdleTimesConstraintMonitors(               */
/*    KHE_SOLN soln, KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)                      */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_limit_idle_times_constraint.c
static void KheSolnMakeAndAttachLimitIdleTimesConstraintMonitors(
  KHE_SOLN soln, KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)
{
  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;  KHE_RESOURCE_IN_SOLN rs;
  KHE_LIMIT_IDLE_TIMES_MONITOR m;
  for( i = 0;  i < KheLimitIdleTimesConstraintResourceGroupCount(c);  i++ )
  {
    rg = KheLimitIdleTimesConstraintResourceGroup(c, i);
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
    {
      r = KheResourceGroupResource(rg, j);
      rs = KheSolnResourceInSoln(soln, r);
      m = KheLimitIdleTimesMonitorMake(rs, c);
      KheMonitorAttachToSoln((KHE_MONITOR) m);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
    }
  }
  for( i = 0;  i < KheLimitIdleTimesConstraintResourceCount(c);  i++ )
  {
    r = KheLimitIdleTimesConstraintResource(c, i);
    rs = KheSolnResourceInSoln(soln, r);
    m = KheLimitIdleTimesMonitorMake(rs, c);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void DoClusterBusyTimesConstraintMonitorsForResourceAndOffset(           */
/*    KHE_SOLN soln, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c,                    */
/*    KHE_RESOURCE_IN_SOLN rs, int offset)                                   */
/*                                                                           */
/*  Make and attach the monitor for c that monitors r at offset.             */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_cluster_busy_times_constraint.c
static void DoClusterBusyTimesConstraintMonitorsForResourceAndOffset(
  KHE_SOLN soln, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c,
  KHE_RESOURCE_IN_SOLN rs, int offset)
{
  KHE_CLUSTER_BUSY_TIMES_MONITOR m;
  m = KheCluster BusyTimesMonitorMake(rs, offset, c);
  KheMonitorAttachToSoln((KHE_MONITOR) m);
  KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void DoClusterBusyTimesConstraintMonitorsForResource(                    */
/*    KHE_SOLN soln, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r)    */
/*                                                                           */
/*  Make and attach the monitors for this constraint that monitor r.         */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_cluster_busy_times_constraint.c
static void DoClusterBusyTimesConstraintMonitorsForResource(
  KHE_SOLN soln, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;  int i, count;
  rs = KheSolnResourceInSoln(soln, r);
  count = KheClusterBusyTimesConstraintAppliesToOffsetCount(c);
  for( i = 0;  i < count;  i++ )
    DoClusterBusyTimesConstraintMonitorsForResourceAndOffset(soln, c, rs,
      KheClusterBusyTimesConstraintAppliesToOffset(c, i));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachClusterBusyTimesConstraintMonitors(             */
/*    KHE_SOLN soln, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                    */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_cluster_busy_times_constraint.c
static void KheSolnMakeAndAttachClusterBusyTimesConstraintMonitors(
  KHE_SOLN soln, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  for( i = 0;  i < KheClusterBusyTimesConstraintResourceGroupCount(c);  i++ )
  {
    rg = KheClusterBusyTimesConstraintResourceGroup(c, i);
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
    {
      r = KheResourceGroupResource(rg, j);
      DoClusterBusyTimesConstraintMonitorsForResource(soln, c, r);
      ** ***
      rs = KheSolnResourceInSoln(soln, r);
      m = KheCluste rBusyTimesMonitorMake(rs, 0, c);
      KheMonitorAttachToSoln((KHE_MONITOR) m);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
      *** **
    }
  }
  for( i = 0;  i < KheClusterBusyTimesConstraintResourceCount(c);  i++ )
  {
    r = KheClusterBusyTimesConstraintResource(c, i);
    DoClusterBusyTimesConstraintMonitorsForResource(soln, c, r);
    ** ***
    rs = KheSolnResourceInSoln(soln, r);
    m = KheCluster BusyTimesMonitorMake(rs, 0, c);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
    *** **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void DoLimitBusyTimesConstraintMonitorsForResourceAndOffset(             */
/*    KHE_SOLN soln, KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r,      */
/*    int offset)                                                            */
/*                                                                           */
/*  Make and attach the monitor for c that monitors r at offset.             */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_limit_busy_times_constraint.c
static void DoLimitBusyTimesConstraintMonitorsForResourceAndOffset(
  KHE_SOLN soln, KHE_LIMIT_BUSY_TIMES_CONSTRAINT c,
  KHE_RESOURCE_IN_SOLN rs, int offset)
{
  KHE_LIMIT_BUSY_TIMES_MONITOR m;
  m = KheLimitBusyTimes MonitorMake(rs, offset, c);
  KheMonitorAttachToSoln((KHE_MONITOR) m);
  KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void DoLimitBusyTimesConstraintMonitorsForResource(                      */
/*    KHE_SOLN soln, KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r)      */
/*                                                                           */
/*  Make and attach the monitors for this constraint that monitor r.         */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_limit_busy_times_constraint.c
static void DoLimitBusyTimesConstraintMonitorsForResource(
  KHE_SOLN soln, KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;  int i, count;
  rs = KheSolnResourceInSoln(soln, r);
  count = KheLimitBusyTimesConstraintAppliesToOffsetCount(c);
  for( i = 0;  i < count;  i++ )
    DoLimitBusyTimesConstraintMonitorsForResourceAndOffset(soln, c, rs,
      KheLimitBusyTimesConstraintAppliesToOffset(c, i));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachLimitBusyTimesConstraintMonitors(               */
/*    KHE_SOLN soln, KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)                      */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_limit_busy_times_constraint.c
static void KheSolnMakeAndAttachLimitBusyTimesConstraintMonitors(
  KHE_SOLN soln, KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
{
  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  for( i = 0;  i < KheLimitBusyTimesConstraintResourceGroupCount(c);  i++ )
  {
    rg = KheLimitBusyTimesConstraintResourceGroup(c, i);
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
    {
      r = KheResourceGroupResource(rg, j);
      DoLimitBusyTimesConstraintMonitorsForResource(soln, c, r);
    }
  }
  for( i = 0;  i < KheLimitBusyTimesConstraintResourceCount(c);  i++ )
  {
    r = KheLimitBusyTimesConstraintResource(c, i);
    DoLimitBusyTimesConstraintMonitorsForResource(soln, c, r);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void DoLimitWorkloadConstraintMonitorsForResourceAndOffset(              */
/*    KHE_SOLN soln, KHE_LIMIT_WORKLOAD_CONSTRAINT c, KHE_RESOURCE r,        */
/*    int offset)                                                            */
/*                                                                           */
/*  Make and attach the monitor for c that monitors r at offset.             */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_limit_workload_constraint.c
static void DoLimitWorkloadConstraintMonitorsForResourceAndOffset(
  KHE_SOLN soln, KHE_LIMIT_WORKLOAD_CONSTRAINT c,
  KHE_RESOURCE_IN_SOLN rs, int offset)
{
  KHE_LIMIT_WORKLOAD_MONITOR m;
  m = KheLimitWorkloadMonitorMake(rs, offset, c);
  KheMonitorAttachToSoln((KHE_MONITOR) m);
  KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void DoLimitWorkloadConstraintMonitorsForResource(                       */
/*    KHE_SOLN soln, KHE_LIMIT_WORKLOAD_CONSTRAINT c, KHE_RESOURCE r)        */
/*                                                                           */
/*  Make and attach the monitors for this constraint that monitor r.         */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_limit_workload_constraint.c
static void DoLimitWorkloadConstraintMonitorsForResource(
  KHE_SOLN soln, KHE_LIMIT_WORKLOAD_CONSTRAINT c, KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;  int i, count;
  rs = KheSolnResourceInSoln(soln, r);
  count = KheLimitWorkloadConstraintAppliesToOffsetCount(c);
  for( i = 0;  i < count;  i++ )
    DoLimitWorkloadConstraintMonitorsForResourceAndOffset(soln, c, rs,
      KheLimitWorkloadConstraintAppliesToOffset(c, i));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachLimitWorkloadConstraintMonitors(                */
/*    KHE_SOLN soln, KHE_LIMIT_WORKLOAD_CONSTRAINT c)                        */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_limit_workload_constraint.c
static void KheSolnMakeAndAttachLimitWorkloadConstraintMonitors(
  KHE_SOLN soln, KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  for( i = 0;  i < KheLimitWorkloadConstraintResourceGroupCount(c);  i++ )
  {
    rg = KheLimitWorkloadConstraintResourceGroup(c, i);
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
    {
      r = KheResourceGroupResource(rg, j);
      DoLimitWorkloadConstraintMonitorsForResource(soln, c, r);
    }
  }
  for( i = 0;  i < KheLimitWorkloadConstraintResourceCount(c);  i++ )
  {
    r = KheLimitWorkloadConstraintResource(c, i);
    DoLimitWorkloadConstraintMonitorsForResource(soln, c, r);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachLimitWorkloadConstraintMonitors(                */
/*    KHE_SOLN soln, KHE_LIMIT_WORKLOAD_CONSTRAINT c)                        */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete
static void KheSolnMakeAndAttachLimitWorkloadConstraintMonitors(
  KHE_SOLN soln, KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;  KHE_RESOURCE_IN_SOLN rs;
  KHE_LIMIT_WORKLOAD_MONITOR m;
  for( i = 0;  i < KheLimitWorkloadConstraintResourceGroupCount(c);  i++ )
  {
    rg = KheLimitWorkloadConstraintResourceGroup(c, i);
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
    {
      r = KheResourceGroupResource(rg, j);
      rs = KheSolnResourceInSoln(soln, r);
      m = KheLimitWorkloadMonitorMake(rs, c);
      KheMonitorAttachToSoln((KHE_MONITOR) m);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
    }
  }
  for( i = 0;  i < KheLimitWorkloadConstraintResourceCount(c);  i++ )
  {
    r = KheLimitWorkloadConstraintResource(c, i);
    rs = KheSolnResourceInSoln(soln, r);
    m = KheLimitWorkloadMonitorMake(rs, c);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void DoLimitActiveIntervalsConstraintMonitorsForResourceAndOffset(       */
/*    KHE_SOLN soln, KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c,                */
/*    KHE_RESOURCE_IN_SOLN rs, int offset)                                   */
/*                                                                           */
/*  Make and attach the monitor for c that monitors r at offset.             */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_limit_active_intervals_constraint.c
static void DoLimitActiveIntervalsConstraintMonitorsForResourceAndOffset(
  KHE_SOLN soln, KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c,
  KHE_RESOURCE_IN_SOLN rs, int offset)
{
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m;
  m = KheLimitActiveIntervalsMonitorMake(rs, offset, c);
  KheMonitorAttachToSoln((KHE_MONITOR) m);
  KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void DoLimitActiveIntervalsConstraintMonitorsForResource(                */
/*    KHE_SOLN soln, KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE r)*/
/*                                                                           */
/*  Make and attach the monitors for this constraint that monitor r.         */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_limit_active_intervals_constraint.c
static void DoLimitActiveIntervalsConstraintMonitorsForResource(
  KHE_SOLN soln, KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;  int i, count;
  rs = KheSolnResourceInSoln(soln, r);
  count = KheLimitActiveIntervalsConstraintAppliesToOffsetCount(c);
  for( i = 0;  i < count;  i++ )
    DoLimitActiveIntervalsConstraintMonitorsForResourceAndOffset(soln, c, rs,
      KheLimitActiveIntervalsConstraintAppliesToOffset(c, i));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachLimitActiveIntervalsConstraintMonitors(         */
/*    KHE_SOLN soln, KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_limit_active_intervals_constraint.c
static void KheSolnMakeAndAttachLimitActiveIntervalsConstraintMonitors(
  KHE_SOLN soln, KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  for( i = 0; i < KheLimitActiveIntervalsConstraintResourceGroupCount(c); i++ )
  {
    rg = KheLimitActiveIntervalsConstraintResourceGroup(c, i);
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
    {
      r = KheResourceGroupResource(rg, j);
      DoLimitActiveIntervalsConstraintMonitorsForResource(soln, c, r);
      ** ***
      rs = KheSolnResourceInSoln(soln, r);
      m = KheLimitActiveIntervalsMonitorMake(rs, 0, c);
      KheMonitorAttachToSoln((KHE_MONITOR) m);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
      *** **
    }
  }
  for( i = 0;  i < KheLimitActiveIntervalsConstraintResourceCount(c);  i++ )
  {
    r = KheLimitActiveIntervalsConstraintResource(c, i);
    DoLimitActiveIntervalsConstraintMonitorsForResource(soln, c, r);
    ** ***
    rs = KheSolnResourceInSoln(soln, r);
    m = KheLimitActiveIntervalsMonitorMake(rs, 0, c);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
    *** **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnMakeAndAttachLimitResourcesConstraintMonitors(               */
/*    KHE_SOLN soln, KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c)               */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_limit_resources_constraint.c
static void KheSolnMakeAndAttachLimitResourcesConstraintMonitors(
  KHE_SOLN soln, KHE_LIMIT_RESOURCES_CONSTRAINT c)
{
  int i;  KHE_LIMIT_RESOURCES_MONITOR m;
  for( i = 0;  i < KheLimitResourcesConstraintEventGroupCount(c);  i++ )
  {
    m = KheLimitResourcesMonitorMake(soln, c, i);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
}
*** */


/* *** obsolete version (switch moved to khe_constraint.c)
static void KheSolnMakeAndAttachConstraintMonitors(KHE_SOLN soln)
{
  int i;  KHE_CONSTRAINT c;  KHE_INSTANCE ins;
  ins = KheSolnInstance(soln);
  if( DEBUG12 )
    fprintf(stderr,
      "[ KheSolnMakeAndAttachConstraintMonitors (%d constraints)\n",
      KheInstanceConstraintCount(ins));
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( DEBUG12 )
      fprintf(stderr, "  adding monitors for %s %s\n",
        KheConstrain tTagShow(KheConstraintTag(c)), KheConstraintId(c));
    switch( KheConstraintTag(c) )
    {
      case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:

	KheSolnMakeAndAttachAssignResourceConstraintMonitors(soln,
	  (KHE_ASSIGN_RESOURCE_CONSTRAINT) c);
	break;

      case KHE_ASSIGN_TIME_CONSTRAINT_TAG:

	KheSolnMakeAndAttachAssignTimeConstraintMonitors(soln,
	  (KHE_ASSIGN_TIME_CONSTRAINT) c);
	break;

      case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:

	KheSolnMakeAndAttachSplitEventsConstraintMonitors(soln,
	  (KHE_SPLIT_EVENTS_CONSTRAINT) c);
	break;

      case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:

	KheSolnMakeAndAttachDistributeSplitEventsConstraintMonitors(soln,
	  (KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT) c);
	break;

      case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:

	KheSolnMakeAndAttachPreferResourcesConstraintMonitors(soln,
	  (KHE_PREFER_RESOURCES_CONSTRAINT) c);
	break;

      case KHE_PREFER_TIMES_CONSTRAINT_TAG:

	KheSolnMakeAndAttachPreferTimesConstraintMonitors(soln,
	  (KHE_PREFER_TIMES_CONSTRAINT) c);
	break;

      case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:

	KheSolnMakeAndAttachAvoidSplitAssignmentsConstraintMonitors(soln,
	  (KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT) c);
	break;

      case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:

	KheSolnMakeAndAttachSpreadEventsConstraintMonitors(soln,
	  (KHE_SPREAD_EVENTS_CONSTRAINT) c);
	break;

      case KHE_LINK_EVENTS_CONSTRAINT_TAG:

	KheSolnMakeAndAttachLinkEventsConstraintMonitors(soln,
	  (KHE_LINK_EVENTS_CONSTRAINT) c);
	break;

      case KHE_ORDER_EVENTS_CONSTRAINT_TAG:

	KheSolnMakeAndAttachOrderEventsConstraintMonitors(soln,
	  (KHE_ORDER_EVENTS_CONSTRAINT) c);
	break;

      case KHE_AVOID_CLASHES_CONSTRAINT_TAG:

	KheSolnMakeAndAttachAvoidClashesConstraintMonitors(soln,
	  (KHE_AVOID_CLASHES_CONSTRAINT) c);
	break;

      case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:

	KheSolnMakeAndAttachAvoidUnavailableTimesConstraintMonitors(soln,
	  (KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT) c);
	break;

      case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:

	KheSolnMakeAndAttachLimitIdleTimesConstraintMonitors(soln,
	  (KHE_LIMIT_IDLE_TIMES_CONSTRAINT) c);
	break;

      case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:

	KheSolnMakeAndAttachClusterBusyTimesConstraintMonitors(soln,
	  (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c);
	break;

      case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:

	KheSolnMakeAndAttachLimitBusyTimesConstraintMonitors(soln,
	  (KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c);
	break;

      case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:

	KheSolnMakeAndAttachLimitWorkloadConstraintMonitors(soln,
	  (KHE_LIMIT_WORKLOAD_CONSTRAINT) c);
	break;

      case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:

	KheSolnMakeAndAttachLimitActiveIntervalsConstraintMonitors(soln,
	  (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c);
	break;

      case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:

        KheSolnMakeAndAttachLimitResourcesConstraintMonitors(soln,
          (KHE_LIMIT_RESOURCES_CONSTRAINT) c);
	break;

      default:

	HnAbort(
	  "KheSolnMakeAndAttachConstraintMonitors illegal constraint tag");
	break;
    }
  }
  if( DEBUG12 )
    fprintf(stderr, "] KheSolnMakeAndAttachConstraintMonitors returning\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnDoDelete(KHE_SOLN soln)                                      */
/*                                                                           */
/*  Carry out that part of the deletion of soln which is common to both      */
/*  KheSolnDelete and KheSolnReduceToPlaceholder.  This is everything        */
/*  except deletion from the solution group and freeing the object.          */
/*                                                                           */
/*  Implementation note.  Deletion requires a plan.  We must ensure that     */
/*  every object to be deleted is deleted exactly once, and we must do it    */
/*  in a way that re-uses user-accessible operations for deleting parts of   */
/*  a solution.  This plan shows which deletion functions delete what.       */
/*                                                                           */
/*    KheSolnDoDelete (public)                                               */
/*      KheLayerDelete (public, deletes only the layer)                      */
/*      KheMeetDelete (public)                                               */
/*        KheSolnResourceDelete (public)                                     */
/*          KheOrdinaryDemandMonitorDelete (private)                         */
/*        KheMatchingDemandChunkDelete (private, only when no nodes)         */
/*      KheNodeDelete (public, deletes only the node)                        */
/*      KheTimeDomainCacheDelete (private)                                   */
/*      KheEventInSolnDelete (private, does not free any monitors)           */
/*        KheEventResourceInSolnDelete (private, doesn't free mons/chunks)   */
/*      KheResourceInSolnDelete (private, doesn't free monitors or chunks)   */
/*        KheResourceInSolnWorkloadRequirementDelete (private)               */
/*      KheTimeDomainFree (private, doesn't free any elements)               */
/*      KheEventGroupDelete (private, doesn't free any elements)             */
/*      KheResourceGroupDelete (private, doesn't free any elements)          */
/*      KheTimeGroupDelete (private, doesn't free any elements)              */
/*      KheMonitorDelete (private) + its many redefs in child classes        */
/*      KheOrdinaryDemandMonitorDelete (private)                             */
/*      KheWorkloadDemandMonitorDelete (private)                             */
/*      KheTransactionFree (deletes free transactions, user deletes others)  */
/*      KheGroupMonitorDelete (public, deletes just the one monitor)         */
/*                                                                           */
/*  NB the following functions are not deletions in the present sense,       */
/*  because they move their objects to free lists rather than freeing them.  */
/*                                                                           */
/*      KheTransactionDelete (public)                                        */
/*      KheSolnMatchingAddOrdinarySupplyChunkToFreeList                      */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete; we delete now by deleting arenas
void KheSolnDoDelete(KHE_SOLN soln)
{
  KHE_MONITOR m;  KHE_TASKING tasking;  KHE_TASK task;  KHE_MEET meet;
  KHE_NODE node;  int i;

  *************************************************************************
  **                                                                     **
  **  First phase, in which the normal solution invariant is maintained  **
  **                                                                     **
  *************************************************************************

  ** detach attached monitors **
  HaArrayForEach(soln->monitors, m, i)
    if( KheMonitorAttachedToSoln(m) )
      KheMonitorDetachFromSoln(m);

  ** delete taskings and tasks **
  while( HaArrayCount(soln->taskings) > 0 )
  {
    tasking = HaArrayLast(soln->taskings);
    HnAssert(KheTaskingSoln(tasking) == soln, "KheSolnDelete internal error 1");
    KheTaskingDelete(tasking);
  }
  while( HaArrayCount(soln->tasks) > 0 )
  {
    task = HaArrayLast(soln->tasks);
    HnAssert(KheTaskSoln(task) == soln, "KheSolnDelete internal error 2");
    KheTaskDelete(task);
  }

  ** delete meets **
  while( HaArrayCount(soln->meets) > 0 )
  {
    meet = HaArrayLast(soln->meets);
    HnAssert(KheMeetSoln(meet) == soln, "KheSolnDelete internal error 3");
    KheMeetDelete(meet);
  }

  ** delete task bounds **
  while( HaArrayCount(soln->task_bounds) > 0 )
    KheTaskBoundDelete(HaArrayLast(soln->task_bounds));

  ** delete meet bounds **
  while( HaArrayCount(soln->meet_bounds) > 0 )
    KheMeetBoundDelete(HaArrayLast(soln->meet_bounds));

  ** delete nodes (NB meets must be deleted before nodes, else nodes fail) **
  while( HaArrayCount(soln->nodes) > 0 )
  {
    node = HaArrayLast(soln->nodes);
    HnAssert(KheNodeSoln(node) == soln, "KheSolnDelete internal error 4");
    if( !KheNodeDelete(node) )
      HnAbort("KheSolnDelete internal error 5");
  }

  ** delete monitors **
  ** ***
  while( HaArrayCount(soln->monitors) > 1 )  ** first element is soln itself **
  {
    m = HaArrayLast(soln->monitors);
    HnAssert(KheMonitorSoln(m) == soln, "KheSolnDelete internal error 6");
    HnAssert(m != (KHE_MONITOR) soln, "KheSolnDelete internal error 7");
    KheMonitorDelete(m);
  }
  *** **


  *************************************************************************
  **                                                                     **
  **  Second phase, in which the solution invariant is not maintained    **
  **  (working up the object's list of attributes)                       **
  **                                                                     **
  *************************************************************************

  ** free meets, tasks, taskings, and task sets **
  ** ***
  while( HaArrayCount(soln->free_task_sets) > 0 )
    KheTaskSetFree(HaArrayLastAndDelete(soln->free_task_sets));
  *** **
  HaArrayFree(soln->free_task_sets);
  HaArrayFree(soln->free_meet_sets);
  HaArrayFree(soln->free_frame_makes);
  HaArrayFree(soln->free_frame_workloads);
  HaArrayFree(soln->free_workload_requirements);
  ** ***
  while( HaArrayCount(soln->free_taskings) > 0 )
    KheTaskingFree(HaArrayLastAndDelete(soln->free_taskings));
  *** **
  HaArrayFree(soln->free_taskings);
  HaArrayFree(soln->taskings);
  ** ***
  while( HaArrayCount(soln->free_meets) > 0 )
    KheMeetUnMake(HaArrayLastAndDelete(soln->free_meets));
  *** **
  HaArrayFree(soln->free_meets);
  ** ***
  while( HaArrayCount(soln->free_nodes) > 0 )
    KheNodeUnMake(HaArrayLastAndDelete(soln->free_nodes));
  *** **
  HaArrayFree(soln->free_nodes);
  ** ***
  while( HaArrayCount(soln->free_layers) > 0 )
    KheLayerUnMake(HaArrayLastAndDelete(soln->free_layers));
  *** **
  HaArrayFree(soln->free_layers);
  ** ***
  while( HaArrayCount(soln->free_zones) > 0 )
    KheZoneUnMake(HaArrayLastAndDelete(soln->free_zones));
  *** **
  HaArrayFree(soln->free_zones);
  ** ***
  while( HaArrayCount(soln->free_task_bounds) > 0 )
    KheTaskBoundUnMake(HaArrayLastAndDelete(soln->free_task_bounds));
  *** **
  ** ***
  while( HaArrayCount(soln->free_meet_bounds) > 0 )
    KheMeetBoundUnMake(HaArrayLastAndDelete(soln->free_meet_bounds));
  while( HaArrayCount(soln->free_meet_bounds) > 0 )
    KheMeetBoundUnMake(HaArrayLastAndDelete(soln->free_meet_bounds));
  *** **
  HaArrayFree(soln->free_meet_bounds);
  ** ***
  while( HaArrayCount(soln->free_tasks) > 0 )
    KheTaskUnMake(HaArrayLastAndDelete(soln->free_tasks));
  *** **
  HaArrayFree(soln->free_tasks);
  HaArrayFree(soln->tasks);

  ** free various arrays whose elements are freed separately **
  HaArrayFree(soln->nodes);
  HaArrayFree(soln->meets);
  HaArrayFree(soln->time_to_cycle_meet);
  HaArrayFree(soln->time_to_cycle_offset);
  HaArrayFree(soln->packing_time_groups);
  HaArrayFree(soln->monitors);
  HaArrayFree(soln->meet_bounds);
  HaArrayFree(soln->task_bounds);

  ** delete event in soln objects **
  ** ***
  while( HaArrayCount(soln->events_in_soln) > 0 )
    KheEventInSolnDelete(HaArrayLastAndDelete(soln->events_in_soln));
  *** **
  HaArrayFree(soln->events_in_soln);

  ** delete resource in soln objects **
  ** ***
  while( HaArrayCount(soln->resources_in_soln) > 0 )
    KheResourceInSolnDelete(HaArrayLastAndDelete(soln->resources_in_soln));
    *** **
  HaArrayFree(soln->resources_in_soln);

  ** delete event groups **
  ** ***
  while( HaArrayCount(soln->event_groups) > 0 )
    KheEventGroupDelete(HaArrayLastAndDelete(soln->event_groups));
  *** **
  HaArrayFree(soln->event_groups);

  ** delete resource groups **
  ** ***
  while( HaArrayCount(soln->resource_groups) > 0 )
    KheResourceGroupDelete(HaArrayLastAndDelete(soln->resource_groups));
  *** **
  HaArrayFree(soln->resource_groups);

  ** delete time groups **
  ** ***
  while( HaArrayCount(soln->time_groups) > 0 )
    KheTimeGroupDelete(HaArrayLastAndDelete(soln->time_groups));
  *** **
  HaArrayFree(soln->time_groups);

  ** delete time nhoods **
  ** ***
  while( HaArrayCount(soln->time_nhoods) > 0 )
    KheTimeGroupNHoodDelete(HaArrayLastAndDelete(soln->time_nhoods));
  *** **
  HaArrayFree(soln->time_nhoods);

  ** delete free monitor links **
  ** ***
  while( HaArrayCount(soln->free_monitor_links) > 0 )
    MFree(HaArrayLastAndDelete(soln->free_monitor_links));
  HaArrayFree(soln->free_monitor_links);
  *** **

  ** delete free traces (user must delete others) **
  ** ***
  while( HaArrayCount(soln->free_traces) > 0 )
    KheTraceFree(HaArrayLastAndDelete(soln->free_traces));
  *** **
  HaArrayFree(soln->free_traces);

  ** delete all marks and paths **
  ** KhePathFree(soln->main_path); **
  ** ***
  while( HaArrayCount(soln->marks) > 0 )
    KheMarkFree(HaArrayLastAndDelete(soln->marks));
  while( HaArrayCount(soln->free_marks) > 0 )
    KheMarkFree(HaArrayLastAndDelete(soln->free_marks));
  while( HaArrayCount(soln->free_paths) > 0 )
    KhePathFree(HaArrayLastAndDelete(soln->free_paths));
  *** **

  ** delete monitors **
  if( DEBUG8 && HaArrayCount(soln->child_links) > 0 )
  {
    int i;  KHE_MONITOR_LINK link;
    HaArrayForEach(soln->child_links, link, i)
      KheMonitorDebug(link->child, 1, 2, stderr);
  }
  HnAssert(HaArrayCount(soln->child_links) == 0,
    "KheSolnDoDelete internal error 8 (%d child monitors)",
    HaArrayCount(soln->child_links));
  HaArrayFree(soln->child_links);
  HnAssert(HaArrayCount(soln->defect_links) == 0,
    "KheSolnDoDelete internal error 9 (%d defects)",
    HaArrayCount(soln->defect_links));
  HaArrayFree(soln->defect_links);

  ** check traces **
  HnAssert(HaArrayCount(soln->traces) == 0,
    "KheSolnDoDelete:  soln is currently being traced");
  HaArrayFree(soln->traces);

  ** delete the matching (should be no demand nodes or chunks left by now) **
  if( soln->matching != NULL )
  {
    KheMatchin gDelete(soln->matching);
    soln->matching = NULL;
  }
  HaArrayFree(soln->free_matchings);
  HaArrayFree(soln->free_matching_supply_chunks);
  HaArrayFree(soln->free_matching_supply_nodes);
  HaArrayFree(soln->free_matching_demand_chunks);
  HaArrayFree(soln->free_ordinary_demand_monitors);
  HaArrayFree(soln->free_workload_demand_monitors);
  HaArrayFree(soln->free_matching_hall_sets);

  ** delete evenness handler **
  ** ***
  if( soln->evenness_handler != NULL )
    KheEvennessHandlerDelete(soln->evenness_handler);
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "meet bounds"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAddMeetBound(KHE_SOLN soln, KHE_MEET_BOUND mb)               */
/*                                                                           */
/*  Add mb to soln.                                                          */
/*                                                                           */
/*****************************************************************************/

/* *** not needed now we are using arenas
void KheSolnAddMeetBound(KHE_SOLN soln, KHE_MEET_BOUND mb)
{
  KheMeetBoundSetSoln(mb, soln);
  KheMeetBoundSetSolnIndex(mb, HaArrayCount(soln->meet_bounds));
  HaArrayAddLast(soln->meet_bounds, mb);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnDeleteMeetBound(KHE_SOLN soln, KHE_MEET_BOUND mb)            */
/*                                                                           */
/*  Delete mb from soln.                                                     */
/*                                                                           */
/*****************************************************************************/

/* *** not needed now we are using arenas
void KheSolnDeleteMeetBound(KHE_SOLN soln, KHE_MEET_BOUND mb)
{
  KHE_MEET_BOUND tmp;  int soln_index;
  tmp = HaArrayLastAndDelete(soln->meet_bounds);
  if( tmp != mb )
  {
    soln_index = KheMeetBoundSolnIndex(mb);
    KheMeetBoundSetSolnIndex(tmp, soln_index);
    HaArrayPut(soln->meet_bounds, soln_index, tmp);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "task bounds"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAddTaskBound(KHE_SOLN soln, KHE_TASK_BOUND tb)               */
/*                                                                           */
/*  Add tb to soln.                                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnAddTaskBound(KHE_SOLN soln, KHE_TASK_BOUND tb)
{
  KheTaskBoundSetSoln(tb, soln);
  KheTaskBoundSetSolnIndex(tb, HaArrayCount(soln->task_bounds));
  HaArrayAddLast(soln->task_bounds, tb);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnDeleteTaskBound(KHE_SOLN soln, KHE_TASK_BOUND tb)            */
/*                                                                           */
/*  Delete tb from soln.                                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnDeleteTaskBound(KHE_SOLN soln, KHE_TASK_BOUND tb)
{
  KHE_TASK_BOUND tmp;  int soln_index;
  tmp = HaArrayLastAndDelete(soln->task_bounds);
  if( tmp != tb )
  {
    soln_index = KheTaskBoundSolnIndex(tb);
    KheTaskBoundSetSolnIndex(tmp, soln_index);
    HaArrayPut(soln->task_bounds, soln_index, tmp);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "soft time limits"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  float KheSolnTimeNow(KHE_SOLN soln)                                      */
/*                                                                           */
/*  Return the wall clock time since soln was created.                       */
/*                                                                           */
/*****************************************************************************/

/* ***
float KheSolnTimeNow(KHE_SOLN soln)
{
  return KheStatsTimerNow(soln->timer);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnSetTimeLimit(KHE_SOLN soln, float limit_in_secs)             */
/*                                                                           */
/*  Set the time limit to limit_in_secs.                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnSetTimeLimit(KHE_SOLN soln, float limit_in_secs)
{
  soln->time_limit = limit_in_secs;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  float KheSolnTimeLimit(KHE_SOLN soln)                                    */
/*                                                                           */
/*  Return the time limit, or -1.0 if none.                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
float KheSolnTimeLimit(KHE_SOLN soln)
{
  return soln->time_limit;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnTimeLimitReached(KHE_SOLN soln)                              */
/*                                                                           */
/*  Return true if there is a time limit and it has been reached.            */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheSolnTimeLimitReached(KHE_SOLN soln)
{
  float now;
  if( soln->time_limit == -1 )
  {
    if( DEBUG11 )
      fprintf(stderr, "KheSolnTimeLimitReached = false (no time limit)\n");
    return false;
  }
  now = KheSolnTimeNow(soln);
  if( now == -1 )
  {
    if( DEBUG11 )
      fprintf(stderr, "KheSolnTimeLimitReached = false (time not known)\n");
    return false;
  }
  if( DEBUG11 )
    fprintf(stderr,
      "KheSolnTimeLimitReached = %s (now %.2f mins, limit %.2f mins)\n",
      now >= soln->time_limit ? "true" : "false", now / 60.0,
      soln->time_limit / 60.0);
  return now >= soln->time_limit;
}
*** */

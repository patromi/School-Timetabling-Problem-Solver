
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
/*  FILE:         khe_solvers.h                                              */
/*  DESCRIPTION:  Include this file when you use KHE's platform and solvers  */
/*                                                                           */
/*  This file has two parts:  type declarations and function declarations.   */
/*  Each part is organized to parallel the User's Guide exactly:             */
/*                                                                           */
/*                    Part B:  Solvers                                       */
/*                                                                           */
/*    Chapter 8.   Introducing Solvers                                       */
/*    Chapter 9.   Time-Structural Solvers                                   */
/*    Chapter 10.  Time Solvers                                              */
/*    Chapter 11.  Resource-Structural Solvers                               */
/*    Chapter 12.  Resource Solvers                                          */
/*    Chapter 13.  Ejection Chains                                           */
/*                                                                           */
/*  This simplifies verifying that KHE offers what the User's Guide says.    */
/*                                                                           */
/*****************************************************************************/
#ifndef KHE_SOLVERS_HEADER_FILE
#define KHE_SOLVERS_HEADER_FILE

#define KHE_USE_PTHREAD 1
#define KHE_USE_TIMING 1
#define KHE_USE_SYSINFO 1
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "khe_platform.h"
#include "howard_a.h"
#include "howard_n.h"

#define KHE_ANY_DURATION 0
#define KHE_COST_SHOW_DIGITS 99999


/*****************************************************************************/
/*                                                                           */
/*                        TYPE DECLARATIONS                                  */
/*                                                                           */
/*****************************************************************************/


/*****************************************************************************/
/*                                                                           */
/*                    Part B:  Solvers                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*    Chapter 8.  Introducing Solvers                                        */
/*                                                                           */
/*****************************************************************************/

#define KHE_NO_TIME -1.0
typedef struct khe_timer_rec *KHE_TIMER;
typedef struct khe_timer_set_rec *KHE_TIMER_SET;
typedef struct khe_options_rec *KHE_OPTIONS;

typedef KHE_SOLN (*KHE_GENERAL_SOLVER)(KHE_SOLN soln, KHE_OPTIONS options);

typedef struct khe_soln_adjuster_rec *KHE_SOLN_ADJUSTER;
typedef struct khe_cluster_minimum_solver_rec *KHE_CLUSTER_MINIMUM_SOLVER;
typedef struct khe_cluster_minimum_group_rec *KHE_CLUSTER_MINIMUM_GROUP;

typedef enum {
  KHE_SUBTAG_SPLIT_EVENTS,	      /* "SplitEventsGroupMonitor"           */
  KHE_SUBTAG_DISTRIBUTE_SPLIT_EVENTS, /* "DistributeSplitEventsGroupMonitor" */
  KHE_SUBTAG_ASSIGN_TIME,	      /* "AssignTimeGroupMonitor"            */
  KHE_SUBTAG_PREFER_TIMES,	      /* "PreferTimesGroupMonitor"           */
  KHE_SUBTAG_SPREAD_EVENTS,	      /* "SpreadEventsGroupMonitor"          */
  KHE_SUBTAG_LINK_EVENTS,	      /* "LinkEventsGroupMonitor"            */
  KHE_SUBTAG_ORDER_EVENTS,	      /* "OrderEventsGroupMonitor"           */
  KHE_SUBTAG_ASSIGN_RESOURCE,	      /* "AssignResourceGroupMonitor"        */
  KHE_SUBTAG_PREFER_RESOURCES,	      /* "PreferResourcesGroupMonitor"       */
  KHE_SUBTAG_AVOID_SPLIT_ASSIGNMENTS, /* "AvoidSplitAssignmentsGroupMonitor" */
  KHE_SUBTAG_AVOID_CLASHES,	      /* "AvoidClashesGroupMonitor"          */
  KHE_SUBTAG_AVOID_UNAVAILABLE_TIMES, /* "AvoidUnavailableTimesGroupMonitor" */
  KHE_SUBTAG_LIMIT_IDLE_TIMES,	      /* "LimitIdleTimesGroupMonitor"        */
  KHE_SUBTAG_CLUSTER_BUSY_TIMES,      /* "ClusterBusyTimesGroupMonitor"      */
  KHE_SUBTAG_LIMIT_BUSY_TIMES,	      /* "LimitBusyTimesGroupMonitor"        */
  KHE_SUBTAG_LIMIT_WORKLOAD,	      /* "LimitWorkloadGroupMonitor"         */
  KHE_SUBTAG_LIMIT_ACTIVE_INTERVALS,  /* "LimitActiveIntervalsGroupMonitor"  */
  KHE_SUBTAG_LIMIT_RESOURCES,         /* "LimitResourcesGroupMonitor"        */
  KHE_SUBTAG_ORDINARY_DEMAND,	      /* "OrdinaryDemandGroupMonitor"        */
  KHE_SUBTAG_WORKLOAD_DEMAND,	      /* "WorkloadDemandGroupMonitor"        */
  KHE_SUBTAG_KEMPE_DEMAND,	      /* "KempeDemandGroupMonitor"           */
  KHE_SUBTAG_NODE_TIME_REPAIR,	      /* "NodeTimeRepairGroupMonitor"        */
  KHE_SUBTAG_LAYER_TIME_REPAIR,	      /* "LayerTimeRepairGroupMonitor"       */
  /* KHE_SUBTAG_TASKING, */	      /* "TaskingGroupMonitor"               */
  KHE_SUBTAG_RESOURCE_REPAIR,	      /* "ResourceRepairGroupMonitor"        */
  KHE_SUBTAG_ALL_DEMAND		      /* "AllDemandGroupMonitor"             */
} KHE_SUBTAG_STANDARD_TYPE;

typedef struct khe_file_rec *KHE_FILE;
typedef struct khe_table_rec *KHE_TABLE;
typedef struct khe_row_rec *KHE_ROW;
typedef struct khe_graph_rec *KHE_GRAPH;
typedef struct khe_dataset_rec *KHE_DATASET;

typedef enum {
  KHE_FILE_PLAIN,
  KHE_FILE_LOUT,
  KHE_FILE_LOUT_STANDALONE,
  KHE_FILE_LATEX
} KHE_FILE_FORMAT;

typedef enum {
  KHE_DATASET_POINTS_NONE,
  KHE_DATASET_POINTS_CROSS,
  KHE_DATASET_POINTS_SQUARE,
  KHE_DATASET_POINTS_DIAMOND,
  KHE_DATASET_POINTS_CIRCLE,
  KHE_DATASET_POINTS_TRIANGLE,
  KHE_DATASET_POINTS_PLUS,
  KHE_DATASET_POINTS_FILLED_SQUARE,
  KHE_DATASET_POINTS_FILLED_DIAMOND,
  KHE_DATASET_POINTS_FILLED_CIRCLE,
  KHE_DATASET_POINTS_FILLED_TRIANGLE
} KHE_DATASET_POINTS_TYPE;

typedef enum {
  KHE_DATASET_PAIRS_NONE,
  KHE_DATASET_PAIRS_SOLID,
  KHE_DATASET_PAIRS_DASHED,
  KHE_DATASET_PAIRS_DOTTED,
  KHE_DATASET_PAIRS_DOT_DASHED,
  KHE_DATASET_PAIRS_DOT_DOT_DASHED,
  KHE_DATASET_PAIRS_DOT_DOT_DOT_DASHED,
  KHE_DATASET_PAIRS_YHISTO,
  KHE_DATASET_PAIRS_SURFACE_YHISTO,
  KHE_DATASET_PAIRS_FILLED_YHISTO,
  KHE_DATASET_PAIRS_XHISTO,
  KHE_DATASET_PAIRS_SURFACE_XHISTO,
  KHE_DATASET_PAIRS_FILLED_XHISTO
} KHE_DATASET_PAIRS_TYPE;

/* ***
typedef enum {
  KHE_STATS_TABLE_PLAIN,
  KHE_STATS_TABLE_LOUT,
  KHE_STATS_TABLE_LATEX
} KHE_STATS_TABLE_TYPE;

typedef enum {
  KHE_STATS_DATASET_DASHED,
  KHE_STATS_DATASET_HISTO
} KHE_STATS_DATASET_TYPE;
*** */

/* typedef struct khe_stats_timer_rec *KHE_STATS_TIMER; */

typedef enum {
  KHE_BACKOFF_NONE,
  KHE_BACKOFF_EXPONENTIAL
} KHE_BACKOFF_TYPE;

typedef struct khe_backoff_rec *KHE_BACKOFF;

typedef uint64_t KHE_RANDOM_GENERATOR;

typedef struct khe_interval_rec {
  int first;
  int last;
} KHE_INTERVAL;

#define KheIntervalFirst(in) ((in).first)
#define KheIntervalLast(in) ((in).last)

typedef struct khe_interval_cost_table_rec *KHE_INTERVAL_COST_TABLE;

typedef struct khe_multiset_generator_rec *KHE_MULTISET_GENERATOR;


/*****************************************************************************/
/*                                                                           */
/*    Chapter 9.   Time-Structural Solvers                                   */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_time_equiv_rec *KHE_TIME_EQUIV;

typedef struct khe_split_analyser_rec *KHE_SPLIT_ANALYSER;


/*****************************************************************************/
/*                                                                           */
/*    Chapter 10.   Time Solvers                                             */
/*                                                                           */
/*****************************************************************************/

typedef bool (*KHE_NODE_TIME_SOLVER)(KHE_NODE parent_node, KHE_OPTIONS options);
typedef bool (*KHE_LAYER_TIME_SOLVER)(KHE_LAYER layer, KHE_OPTIONS options);

typedef struct khe_kempe_stats_rec *KHE_KEMPE_STATS;
typedef struct khe_meet_bound_group_rec *KHE_MEET_BOUND_GROUP;

typedef struct khe_layer_asst_rec *KHE_LAYER_ASST;

typedef struct khe_meet_set_solver_rec *KHE_MEET_SET_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*    Chapter 11.  Resource-Structural Solvers                               */
/*                                                                           */
/*****************************************************************************/

/* typedef struct khe_task_bound_group_rec *KHE_TASK_BOUND_GROUP; */

typedef struct khe_consec_solver_rec *KHE_CONSEC_SOLVER;
/* typedef struct khe_balance_solver_rec *KHE_BALANCE_SOLVER; */

typedef struct khe_flow_rec *KHE_FLOW;
typedef struct khe_flow_resource_node_rec *KHE_FLOW_RESOURCE_NODE;
typedef struct khe_flow_task_node_rec *KHE_FLOW_TASK_NODE;

/* ***
typedef struct khe_tasker_rec *KHE_TASKER;
typedef struct khe_tasker_time_rec *KHE_TASKER_TIME;
typedef struct khe_tasker_class_rec *KHE_TASKER_CLASS;
typedef struct khe_profile_time_group_rec *KHE_PROFILE_TIME_GROUP;
typedef struct khe_comb_solver_rec *KHE_COMB_SOLVER;

typedef enum {
  KHE_COMB_SOLVER_COVER_YES,
  KHE_COMB_SOLVER_COVER_NO,
  KHE_COMB_SOLVER_COVER_FREE,
  KHE_COMB_SOLVER_COVER_PREV
} KHE_COMB_SOLVER_COVER_TYPE;

typedef enum {
  KHE_COMB_SOLVER_COST_MIN,
  KHE_COMB_SOLVER_COST_ZERO,
  KHE_COMB_SOLVER_COST_SOLE_ZERO
} KHE_COMB_SOLVER_COST_TYPE;
*** */

/* ***
typedef enum {
  KHE_GROUP_SOLVER_COVER_YES,
  KHE_GROUP_SOLVER_COVER_NO,
  KHE_GROUP_SOLVER_COVER_PREV
} KHE_GROUP_SOLVER_COVER_TYPE;

typedef enum {
  KHE_GROUP_SOLVER_COST_MIN,
  KHE_GROUP_SOLVER_COST_ZERO,
  KHE_GROUP_SOLVER_COST_SOLE_ZERO
} KHE_GROUP_SOLVER_COST_TYPE;

typedef struct khe_group_solver_rec *KHE_GROUP_SOLVER;
*** */
typedef struct khe_constraint_class_finder_rec *KHE_CONSTRAINT_CLASS_FINDER;
typedef struct khe_constraint_class_rec *KHE_CONSTRAINT_CLASS;

typedef struct khe_task_finder_rec *KHE_TASK_FINDER;
/* typedef struct khe_widened_task_set_rec *KHE_WIDENED_TASK_SET; */
/* typedef struct khe_widened_mtask_set_rec *KHE_WIDENED_MTASK_SET; */
typedef struct khe_daily_schedule_rec *KHE_DAILY_SCHEDULE;
/* ***
typedef HA_ARRAY(KHE_TIME) ARRAY_KHE_TIME;
typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK;
*** */

/* ***
typedef struct khe_task_group_rec *KHE_TASK_GROUP;
typedef struct khe_task_groups_rec *KHE_TASK_GROUPS;
*** */

/* ***
typedef struct khe_task_class_solver_rec *KHE_TASK_CLASS_SOLVER;
typedef struct khe_task_class_rec *KHE_TASK_CLASS;
*** */

typedef struct khe_mtask_finder_rec *KHE_MTASK_FINDER;
typedef struct khe_mtask_rec *KHE_MTASK;
typedef struct khe_mtask_set_rec *KHE_MTASK_SET;

/* ***
typedef struct khe_comb_grouper_rec *KHE_COMB_GROUPER;

typedef bool (*KHE_COMB_MTASK_FN)(KHE_MTASK mt, void *impl);
typedef bool (*KHE_COMB_MTASK_SET_FN)(KHE_MTASK_SET mts, void *impl);

typedef enum {
  KHE_COMB_COVER_YES,
  KHE_COMB_COVER_NO,
  KHE_COMB_COVER_PREV,
  KHE_COMB_COVER_FREE
} KHE_COMB_COVER_TYPE;

typedef enum {
  KHE_COMB_VARIANT_MIN,
  KHE_COMB_VARIANT_ZERO,
  KHE_COMB_VARIANT_SOLE_ZERO,
  KHE_COMB_VARIANT_SINGLES
} KHE_COMB_VARIANT_TYPE;
*** */

/* *** withdrawn
typedef struct khe_monitor_framer_rec *KHE_MONITOR_FRAMER;
*** */

typedef struct khe_task_group_domain_finder_rec *KHE_TASK_GROUP_DOMAIN_FINDER;
typedef struct khe_task_group_domain_rec *KHE_TASK_GROUP_DOMAIN;

/* *** no longer needed
typedef enum {
  KHE_TASK_GROUP_DOMAIN_RG_ONLY,
  KHE_TASK_GROUP_DOMAIN_RG_SUBSET_PREV,
  KHE_TASK_GROUP_DOMAIN_PREV_SUBSET_RG,
  KHE_TASK_GROUP_DOMAIN_PREV_INTERSECT_RG
} KHE_TASK_GROUP_DOMAIN_TYPE;
*** */

typedef struct khe_task_grouper_rec *KHE_TASK_GROUPER;
/* typedef struct khe_task_grouper_entry_rec *KHE_TASK_GROUPER_ENTRY; */

/* ***
typedef enum {
  KHE_TASK_GROUPER_ENTRY_ORDINARY,
  KHE_TASK_GROUPER_ENTRY_HISTORY
  ** KHE_TASK_GROUPER_ENTRY_DUMMY **
} KHE_TASK_GROUPER_ENTRY_TYPE;
*** */

/* ***
#define INHERIT_KHE_TASK_GROUPER_ENTRY					\
  KHE_TASK				task;				\
  ** KHE_TASK				leader; **			\
  KHE_TASK_GROUP_DOMAIN			domain;				\
  KHE_RESOURCE				assigned_resource;		\
  struct khe_task_grouper_entry_rec	*prev;				\
  KHE_INTERVAL				interval;			\
  KHE_TASK_GROUPER_ENTRY_TYPE		type : 8;			\
  bool					has_fixed_unassigned;		\

typedef struct khe_task_grouper_entry_rec {
  INHERIT_KHE_TASK_GROUPER_ENTRY
} *KHE_TASK_GROUPER_ENTRY;
*** */

typedef struct khe_mtask_grouper_rec *KHE_MTASK_GROUPER;
/* typedef struct khe_mtask_grouper_entry_rec *KHE_MTASK_GROUPER_ENTRY; */

/* ***
#define INHERIT_KHE_MTASK_GROUPER_ENTRY					\
  KHE_MTASK				mtask;				\
  ** KHE_MTASK				leader; **			\
  KHE_TASK_GROUP_DOMAIN			domain;				\
  struct khe_mtask_grouper_entry_rec	*prev;				\
  KHE_INTERVAL				interval;			\
  bool					dummy_entry;

typedef struct khe_mtask_grouper_entry_rec {
  INHERIT_KHE_MTASK_GROUPER_ENTRY
} *KHE_MTASK_GROUPER_ENTRY;
*** */

/* ***
typedef struct khe_task_resource_grouper_rec *KHE_TASK_RESOURCE_GROUPER;
*** */

typedef struct khe_simple_grouper_rec *KHE_SIMPLE_GROUPER;

typedef enum {
  KHE_SIMPLE_GROUPER_GROUP_SAME_RESOURCE,
  KHE_SIMPLE_GROUPER_GROUP_SAME_RESOURCE_CONSECUTIVE
} KHE_SIMPLE_GROUPER_GROUP_TYPE;

typedef struct khe_grouped_tasks_display_rec *KHE_GROUPED_TASKS_DISPLAY;


/*****************************************************************************/
/*                                                                           */
/*    Chapter 12.  Resource Solvers                                          */
/*                                                                           */
/*****************************************************************************/

typedef bool (KHE_RESOURCE_SOLVER)(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options);
/* ***
typedef bool (*KHE_TASK ING_SOLVER)(KHE_TA SKING tasking, KHE_OPTIONS options);
*** */

/* ***
typedef struct khe_frame_iterator_rec {
  KHE_FRAME		frame;		** the frame being iterated over     **
  int			count;		** 1 + KheFrameTimeGroupCount(frame) **
  int			extra;		** start point along frame           **
  int			curr;		** current position of iterator      **
  KHE_RESOURCE		resource;	** resource whose activity is wanted **
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;	** timetable monitor of resource     **
} *KHE_FRAME_ITERATOR;
*** */

typedef enum {
  KHE_SRS_DOM_WEAK,
  KHE_SRS_DOM_MEDIUM,
  KHE_SRS_DOM_STRONG,
  KHE_SRS_DOM_TRIE
} KHE_SRS_DOM_KIND;

typedef struct khe_single_resource_solver_rec *KHE_SINGLE_RESOURCE_SOLVER;

typedef enum {
  KHE_DRS_DOM_LIST_NONE,
  KHE_DRS_DOM_LIST_SEPARATE,
  KHE_DRS_DOM_LIST_TRADEOFF,
  KHE_DRS_DOM_LIST_TABULATED,
  KHE_DRS_DOM_HASH_EQUALITY,
  KHE_DRS_DOM_HASH_MEDIUM,
  /* KHE_DRS_DOM_TRIE_SEPARATE, */
  /* KHE_DRS_DOM_TRIE_TRADEOFF, */
  KHE_DRS_DOM_INDEXED_TRADEOFF,
  KHE_DRS_DOM_INDEXED_TABULATED
} KHE_DRS_DOM_KIND;

/* *** old version
typedef enum {
  KHE_DRS_DOM_NONE,
  KHE_DRS_DOM_WEAK,
  KHE_DRS_DOM_MEDIUM,
  KHE_DRS_DOM_STRONG,
  KHE_DRS_DOM_TRIE,
  KHE_DRS_DOM_STRONG_WITH_TRADEOFF,
  KHE_DRS_DOM_TRIE_WITH_TRADEOFF,
  KHE_DRS_DOM_INDEXED_WITH_TRADEOFF,
  KHE_DRS_DOM_UNIFORM,
  KHE_DRS_DOM_INDEXED_UNIFORM
} KHE_DRS_DOM_KIND;
*** */

typedef struct khe_dynamic_resource_solver_rec *KHE_DYNAMIC_RESOURCE_SOLVER;

typedef struct khe_resource_matching_solver_rec *KHE_RESOURCE_MATCHING_SOLVER;
typedef struct khe_resource_matching_demand_set_rec
  *KHE_RESOURCE_MATCHING_DEMAND_SET;
typedef HA_ARRAY(KHE_RESOURCE_MATCHING_DEMAND_SET)
  ARRAY_KHE_RESOURCE_MATCHING_DEMAND_SET;

typedef enum {
  KHE_MOVE_UNCHECKED,			/* just a basic unchecked move       */
  KHE_MOVE_CHECKED,			/* do nothing if would cause clashes */
  KHE_MOVE_EJECTING,			/* eject clashes                     */
  KHE_MOVE_KEMPE			/* fix clashes using Kempe chains    */
} KHE_MOVE_TYPE;

typedef struct khe_resource_run_solver_rec *KHE_RESOURCE_RUN_SOLVER;

typedef struct khe_reassign_solver_rec *KHE_REASSIGN_SOLVER;

typedef enum {
  KHE_REASSIGN_MINIMAL,
  KHE_REASSIGN_RUNS,
  KHE_REASSIGN_MAXIMAL
} KHE_REASSIGN_GROUPING;

typedef enum {
  KHE_REASSIGN_EXHAUSTIVE,
  KHE_REASSIGN_MATCHING
} KHE_REASSIGN_METHOD;


/*****************************************************************************/
/*                                                                           */
/*    Chapter 13.  Ejection Chains                                           */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_ejector_rec *KHE_EJECTOR;
typedef struct khe_augment_options_rec *KHE_AUGMENT_OPTIONS;
typedef bool (*KHE_EJECTOR_AUGMENT_FN)(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d);


/*****************************************************************************/
/*                                                                           */
/*                     FUNCTION DECLARATIONS                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*                    Part B:  Solvers                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*    Chapter 8.  Introducing Solvers                                        */
/*                                                                           */
/*****************************************************************************/

/* 8.1 Keeping track of running time */
extern char *KheDateToday(void);
extern float KheTimeFromString(char *str);
extern bool KheStringContainsTime(char *str, float *time);
extern char *KheTimeShow(float secs, char buff[20]);

extern KHE_TIMER KheTimerMake(char *tag, float limit_in_seconds, HA_ARENA a);
extern KHE_TIMER KheTimerCopy(KHE_TIMER timer, HA_ARENA a);
extern char *KheTimerTag(KHE_TIMER timer);
extern float KheTimerTimeLimit(KHE_TIMER timer);
extern void KheTimerResetStartTime(KHE_TIMER timer);
extern void KheTimerResetTimeLimit(KHE_TIMER timer, float limit_in_seconds);
extern float KheTimerElapsedTime(KHE_TIMER timer);
extern float KheTimerRemainingTime(KHE_TIMER timer);
extern bool KheTimerTimeLimitReached(KHE_TIMER timer);
extern void KheTimerDebug(KHE_TIMER timer, int verbosity, int indent, FILE *fp);

extern KHE_TIMER_SET KheTimerSetMake(HA_ARENA a);
extern KHE_TIMER_SET KheTimerSetCopy(KHE_TIMER_SET timer_set, HA_ARENA a);
extern void KheTimerSetAddTimer(KHE_TIMER_SET timer_set, KHE_TIMER timer);
extern void KheTimerSetDeleteTimer(KHE_TIMER_SET timer_set, KHE_TIMER timer);
extern bool KheTimerSetContainsTimer(KHE_TIMER_SET timer_set, char *tag,
  KHE_TIMER *timer);
extern float KheTimerSetRemainingTime(KHE_TIMER_SET timer_set);
extern bool KheTimerSetTimeLimitReached(KHE_TIMER_SET timer_set);
/* extern int KheTimeSetTimeLimitReachedQueryCount(KHE_TIMER_SET timer_set); */
/* extern void KheTimerSetSaveLog(KHE_TIMER_SET ts, KHE_SOLN soln); */
extern void KheTimerSetTimeLimitConsistencyBegin(KHE_TIMER_SET timer_set,
  KHE_SOLN soln);
extern void KheTimerSetTimeLimitConsistencyEnd(KHE_TIMER_SET timer_set,
  KHE_SOLN soln);
extern void KheTimerSetDebug(KHE_TIMER_SET timer_set, int verbosity,
  int indent, FILE *fp);

/* 8.2 Options, running time, and time limits */
extern KHE_OPTIONS KheOptionsMake(HA_ARENA a);
extern HA_ARENA KheOptionsArena(KHE_OPTIONS options);
/* extern void KheOptionsDelete(KHE_OPTIONS options); */
extern KHE_OPTIONS KheOptionsCopy(KHE_OPTIONS options, HA_ARENA a);

extern void KheOptionsSet(KHE_OPTIONS options, char *key, char *value);
extern char *KheOptionsGet(KHE_OPTIONS options, char *key, char *dft);

extern void KheOptionsSetBool(KHE_OPTIONS options, char *key, bool value);
extern bool KheOptionsGetBool(KHE_OPTIONS options, char *key, bool dft);

extern void KheOptionsSetInt(KHE_OPTIONS options, char *key, int value);
extern int KheOptionsGetInt(KHE_OPTIONS options, char *key, int dft);

extern void KheOptionsSetFloat(KHE_OPTIONS options, char *key, float value);
extern float KheOptionsGetFloat(KHE_OPTIONS options, char *key, float dft);

extern void KheOptionsSetObject(KHE_OPTIONS options, char *key, void *value);
extern void *KheOptionsGetObject(KHE_OPTIONS options, char *key, void *dft);

extern KHE_TIMER KheOptionsAddTimer(KHE_OPTIONS options, char *tag,
  float limit_in_seconds);
extern void KheOptionsDeleteTimer(KHE_OPTIONS options, KHE_TIMER timer);
extern bool KheOptionsContainsTimer(KHE_OPTIONS options, char *tag,
  KHE_TIMER *timer);
extern float KheOptionsRemainingTime(KHE_OPTIONS options);
extern bool KheOptionsTimeLimitReached(KHE_OPTIONS options);
extern void KheOptionsTimeLimitConsistencyBegin(KHE_OPTIONS options,
  KHE_SOLN soln);
extern void KheOptionsTimeLimitConsistencyEnd(KHE_OPTIONS options,
  KHE_SOLN soln);
/* extern int KheOptionsTimeLimitReachedQueryCount(KHE_OPTIONS options); */
/* ***
extern void KheOptionsTimerSetSaveLog(KHE_OPTIONS options, KHE_SOLN soln);
*** */
extern void KheOptionsTimerSetDebug(KHE_OPTIONS options, int verbosity,
  int indent, FILE *fp);


/* ***
extern void KheOptionsTimeLimitBegin(KHE_OPTIONS options, char *tag,
  float limit_in_secs);
extern float KheOptionsTimeLimitEnd(KHE_OPTIONS options, char *tag);
extern bool KheOptionsTimeLimitNow(KHE_OPTIONS options, char *tag,
  float *elapsed_time_in_secs);
extern bool KheOptionsTimeLimitReached(KHE_OPTIONS options);
extern void KheOptionsTimeLimitReset(KHE_OPTIONS options, char *tag,
  float limit_in_secs);
*** */

/* ***
extern float KheOptionsTimeNow(KHE_OPTIONS options);
extern void KheOptionsSetTimeLimit(KHE_OPTIONS options, float limit_in_secs);
extern float KheOptionsTimeLimit(KHE_OPTIONS options);
extern bool KheOptionsTimeLimitReached(KHE_OPTIONS options);
*** */

extern KHE_FRAME KheOptionsFrame(KHE_OPTIONS options, char *key, KHE_SOLN soln);

/* 8.3 Do-it-yourself solving */
extern bool KheDoItYourselfSolverParseAndRun(KHE_SOLN soln, KHE_NODE cycle_node,
  KHE_OPTIONS options, char *solver_option_name, char *solver_option_val);

/* 8.4 General solving */
extern KHE_SOLN KheGeneralSolve2024(KHE_SOLN soln, KHE_OPTIONS options);
/* extern float KheConvertTimeLimit(char *str); */
extern void KheSolveDebug(KHE_SOLN soln, KHE_OPTIONS options, char *fmt, ...);

/* 8.5 Parallel solving */
extern void KheArchiveParallelSolve(KHE_ARCHIVE archive,
  KHE_GENERAL_SOLVER solver, KHE_OPTIONS options,
  KHE_SOLN_TYPE soln_type, /* char *out_file_name, */ HA_ARENA_SET as);
extern KHE_SOLN KheInstanceParallelSolve(KHE_INSTANCE ins,
  KHE_GENERAL_SOLVER solver, KHE_OPTIONS options,
  KHE_SOLN_TYPE soln_type, HA_ARENA_SET as);

/* 8.6.1 The monitor adjuster */
extern KHE_SOLN_ADJUSTER KheSolnAdjusterMake(KHE_SOLN soln, HA_ARENA a);
/* extern void KheSolnAdjusterDelete(KHE_SOLN_ADJUSTER sa); */
extern void KheSolnAdjusterClear(KHE_SOLN_ADJUSTER sa);
extern KHE_SOLN KheSolnAdjusterSoln(KHE_SOLN_ADJUSTER sa);

extern void KheSolnAdjusterUndo(KHE_SOLN_ADJUSTER sa);
extern void KheSolnAdjusterRedo(KHE_SOLN_ADJUSTER sa);

extern void KheSolnAdjusterMonitorAttachToSoln(KHE_SOLN_ADJUSTER sa,
  KHE_MONITOR m);
extern void KheSolnAdjusterMonitorDetachFromSoln(KHE_SOLN_ADJUSTER sa,
  KHE_MONITOR m);
extern void KheSolnAdjusterMonitorSetCombinedWeight(KHE_SOLN_ADJUSTER sa,
  KHE_MONITOR m, KHE_COST combined_weight);
extern void KheSolnAdjusterMonitorSetTilt(KHE_SOLN_ADJUSTER sa,
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim);

extern bool KheSolnAdjusterTaskMove(KHE_SOLN_ADJUSTER sa,
  KHE_TASK task, KHE_TASK target_task);
extern bool KheSolnAdjusterTaskAssignResource(KHE_SOLN_ADJUSTER sa,
  KHE_TASK task, KHE_RESOURCE r);
extern void KheSolnAdjusterTaskAssignFix(KHE_SOLN_ADJUSTER sa, KHE_TASK task);
extern void KheSolnAdjusterTaskAssignUnFix(KHE_SOLN_ADJUSTER sa,
  KHE_TASK task);
extern bool KheSolnAdjusterTaskAddTaskBound(KHE_SOLN_ADJUSTER sa,
  KHE_TASK task, KHE_TASK_BOUND tb);
extern bool KheSolnAdjusterTaskDeleteTaskBound(KHE_SOLN_ADJUSTER sa,
  KHE_TASK task, KHE_TASK_BOUND tb);

extern bool KheSolnAdjusterTaskGroup(KHE_SOLN_ADJUSTER sa,
  KHE_TASK task, KHE_TASK leader_task);

extern KHE_TASK_BOUND KheSolnAdjusterTaskBoundMake(KHE_SOLN_ADJUSTER sa,
  KHE_SOLN soln, KHE_RESOURCE_GROUP rg);

/* 8.6.2 Detaching low-cost monitors */
extern void KheDetachLowCostMonitors(KHE_SOLN soln,
  KHE_COST min_weight, KHE_SOLN_ADJUSTER sa);

/* 8.7 Monitor grouping */
extern char *KheSubTagLabel(KHE_SUBTAG_STANDARD_TYPE sub_tag);
extern KHE_SUBTAG_STANDARD_TYPE KheSubTagFromTag(KHE_MONITOR_TAG tag);
extern bool KheMonitorHasParent(KHE_MONITOR m, int sub_tag,
  KHE_GROUP_MONITOR *res_gm);
extern void KheMonitorAddSelfOrParent(KHE_MONITOR m, int sub_tag,
  KHE_GROUP_MONITOR gm);
extern void KheMonitorDeleteAllParentsRecursive(KHE_MONITOR m);

extern void KheGroupCorrelatedMonitors(KHE_SOLN soln);
extern void KheUnGroupCorrelatedMonitors(KHE_SOLN soln);

/* 8.8 Matchings */
extern void KheMatchingBegin(KHE_SOLN soln, bool integrated);
extern void KheMatchingEnd(KHE_SOLN soln);

/* Benchmarking - obsolete */
/* ***
extern bool KheBenchmarkTryInstance(KHE_INSTANCE ins);
extern void KheBenchmark(KHE_ARCHIVE archive, KHE_GENERAL_SOLVER solver,
  char *solver_name, char *author_name, char test_label,
  KHE_STATS_TABLE_TYPE table_type);
*** */

/* ***
** 8.6 Gathering statistics **
extern KHE_STATS_TIMER KheStatsTimerMake(HA_ARENA a);
extern void KheStatsTimerReset(KHE_STATS_TIMER st);
extern float KheStatsTimerNow(KHE_STATS_TIMER st);
** extern void KheStatsTimerDelete(KHE_STATS_TIMER st); **
extern KHE_STATS_TIMER KheStatsTimerCopy(KHE_STATS_TIMER st, HA_ARENA a);

extern char *KheStatsDateToday(void);

extern void KheStatsFileBegin(char *file_name);
extern void KheStatsFileEnd(char *file_name);

extern void KheStatsTableBegin(char *file_name, KHE_STATS_TABLE_TYPE table_type,
  int col_width, char *corner, bool with_average_row, bool with_total_row,
  bool highlight_cost_minima, bool highlight_time_minima,
  bool highlight_int_minima);
extern void KheStatsTableEnd(char *file_name);
extern void KheStatsCaptionAdd(char *file_name, char *fmt, ...);
extern void KheStatsRowAdd(char *file_name, char *row_label, bool rule_below);
extern void KheStatsColAdd(char *file_name, char *col_label, bool rule_after);

extern void KheStatsEntryAddString(char *file_name, char *row_label,
  char *col_label, char *str);
extern void KheStatsEntryAddCost(char *file_name, char *row_label,
  char *col_label, KHE_COST cost);
extern void KheStatsEntryAddTime(char *file_name, char *row_label,
  char *col_label, float time);
extern void KheStatsEntryAddInt(char *file_name, char *row_label,
  char *col_label, int val);

extern void KheStatsGraphBegin(char *file_name);
extern void KheStatsGraphEnd(char *file_name);
extern void KheStatsGraphSetWidth(char *file_name, float width);
extern void KheStatsGraphSetHeight(char *file_name, float height);
extern void KheStatsGraphSetXMax(char *file_name, float xmax);
extern void KheStatsGraphSetYMax(char *file_name, float ymax);
extern void KheStatsGraphSetAboveCaption(char *file_name, char *val);
extern void KheStatsGraphSetBelowCaption(char *file_name, char *val);
extern void KheStatsGraphSetLeftCaption(char *file_name, char *val);
extern void KheStatsGraphSetRightCaption(char *file_name, char *val);
extern void KheStatsDataSetAdd(char *file_name, char *dataset_label,
  KHE_STATS_DATASET_TYPE dataset_type);
extern void KheStatsPointAdd(char *file_name, char *dataset_label,
  float x, float y);
*** */

/* 8.9 Generating files of tables and graphs */
extern KHE_FILE KheFileBegin(char *file_name, KHE_FILE_FORMAT fmt);
extern void KheFileEnd(KHE_FILE kf);

extern KHE_TABLE KheTableBegin(KHE_FILE kf, int col_width, char *corner,
  bool with_average_row, bool with_total_row, bool highlight_cost_minima,
  bool highlight_time_minima, bool highlight_int_minima);
extern void KheTableEnd(KHE_TABLE kt);
extern void KheTableCaptionAdd(KHE_TABLE kt, char *fmt, ...);
extern void KheTableRowAdd(KHE_TABLE kt, char *row_label, bool rule_below);
extern void KheTableColAdd(KHE_TABLE kt, char *col_label, bool rule_after);

extern void KheTableEntryAddString(KHE_TABLE kt, char *row_label,
  char *col_label, char *str);
extern void KheTableEntryAddCost(KHE_TABLE kt, char *row_label,
  char *col_label, KHE_COST cost);
extern void KheTableEntryAddTime(KHE_TABLE kt, char *row_label,
  char *col_label, float time);
extern void KheTableEntryAddInt(KHE_TABLE kt, char *row_label,
  char *col_label, int val);

extern KHE_GRAPH KheGraphBegin(KHE_FILE kf);
extern void KheGraphEnd(KHE_GRAPH kg);
extern void KheGraphSetWidth(KHE_GRAPH kg, float width);
extern void KheGraphSetHeight(KHE_GRAPH kg, float height);
extern void KheGraphSetXMax(KHE_GRAPH kg, float xmax);
extern void KheGraphSetYMax(KHE_GRAPH kg, float ymax);
extern void KheGraphSetAboveCaption(KHE_GRAPH kg, char *val);
extern void KheGraphSetBelowCaption(KHE_GRAPH kg, char *val);
extern void KheGraphSetLeftCaptionAndGap(KHE_GRAPH kg, char *val, char *gap);
extern void KheGraphSetRightCaptionAndGap(KHE_GRAPH kg, char *val, char *gap);
extern void KheGraphSetKeyLabel(KHE_GRAPH kg, char *val);
extern KHE_DATASET KheDataSetAdd(KHE_GRAPH kg,
  KHE_DATASET_POINTS_TYPE points_type,
  KHE_DATASET_PAIRS_TYPE pairs_type, char *label);
extern void KhePointAdd(KHE_DATASET kd, float x, float y);

/* 8.10 Exponential backoff */
extern KHE_BACKOFF KheBackoffBegin(KHE_BACKOFF_TYPE backoff_type, HA_ARENA a);
extern bool KheBackoffAcceptOpportunity(KHE_BACKOFF bk);
extern void KheBackoffResult(KHE_BACKOFF bk, bool success);
/* extern void KheBackoffEnd(KHE_BACKOFF bk); */

extern char *KheBackoffShowNextDecision(KHE_BACKOFF bk);
extern void KheBackoffDebug(KHE_BACKOFF bk, int verbosity, int indent,FILE *fp);
extern void KheBackoffTest(FILE *fp);

/* 8.11 Thread-safe random numbers */
extern void KheRandomGeneratorInit(KHE_RANDOM_GENERATOR *rgen, uint32_t seed);
extern uint32_t KheRandomGeneratorNext(KHE_RANDOM_GENERATOR *rgen);
extern int KheRandomGeneratorNextRange(KHE_RANDOM_GENERATOR *rgen,
  int first, int last);
extern bool KheRandomGeneratorNextBool(KHE_RANDOM_GENERATOR *rgen);
extern void KheRandomGeneratorTest(uint32_t seed, int count,
  int first, int last, int indent, FILE *fp);

/* 8.12 Intervals */
extern KHE_INTERVAL KheIntervalMake(int first, int last);
extern int KheIntervalLength(KHE_INTERVAL in);
extern bool KheIntervalEmpty(KHE_INTERVAL in);
extern bool KheIntervalContains(KHE_INTERVAL in, int index);
extern bool KheIntervalEqual(KHE_INTERVAL in1, KHE_INTERVAL in2);
extern bool KheIntervalSubset(KHE_INTERVAL in1, KHE_INTERVAL in2);
extern bool KheIntervalDisjoint(KHE_INTERVAL in1, KHE_INTERVAL in2);
extern KHE_INTERVAL KheIntervalUnion(KHE_INTERVAL in1, KHE_INTERVAL in2);
extern KHE_INTERVAL KheIntervalIntersection(KHE_INTERVAL in1, KHE_INTERVAL in2);
extern int KheIntervalTypedCmp(KHE_INTERVAL i1, KHE_INTERVAL i2);
extern int KheIntervalCmp(const void *t1, const void *t2);
extern char *KheIntervalShow(KHE_INTERVAL in, KHE_FRAME frame);

extern KHE_INTERVAL KheTimeInterval(KHE_TIME t, KHE_FRAME frame);
extern KHE_INTERVAL KheTimeGroupInterval(KHE_TIME_GROUP tg, KHE_FRAME frame);
extern KHE_INTERVAL KheTaskInterval(KHE_TASK task, KHE_FRAME frame);
extern KHE_INTERVAL KheTaskSetInterval(KHE_TASK_SET ts, KHE_FRAME frame);

extern KHE_INTERVAL_COST_TABLE KheIntervalCostTableMake(HA_ARENA a);
extern void KheIntervalCostTableClear(KHE_INTERVAL_COST_TABLE ict);
extern void KheIntervalCostTableInsert(KHE_INTERVAL_COST_TABLE ict,
  KHE_INTERVAL in, KHE_COST cost);
extern bool KheIntervalCostTableRetrieve(KHE_INTERVAL_COST_TABLE ict,
  KHE_INTERVAL in, KHE_COST *cost);
extern void KheIntervalCostTableDebug(KHE_INTERVAL_COST_TABLE ict,
  KHE_FRAME frame, int verbosity, int indent, FILE *fp);

/* 8.13 Multiset generation */
extern KHE_MULTISET_GENERATOR KheMultisetGeneratorMake(HA_ARENA a);
extern void KheMultisetGeneratorSetupBegin(KHE_MULTISET_GENERATOR mg,
  int element_sum);
extern void KheMultisetGeneratorSetupAdd(KHE_MULTISET_GENERATOR mg,
  int min_value, int max_value);
extern void KheMultisetGeneratorSetupEnd(KHE_MULTISET_GENERATOR mg);
extern bool KheMultisetGeneratorNext(KHE_MULTISET_GENERATOR mg);
extern int KheMultisetGeneratorNextValue(KHE_MULTISET_GENERATOR mg, int j);
extern int KheMultisetGeneratorSetupAddCount(KHE_MULTISET_GENERATOR mg);
extern void KheMulisetGeneratorTest(HA_ARENA a);


/*****************************************************************************/
/*                                                                           */
/*    Chapter 9.   Time-Structural Solvers                                   */
/*                                                                           */
/*****************************************************************************/

/* 9.1 Layer tree construction */
extern KHE_NODE KheLayerTreeMake(KHE_SOLN soln);

/* 9.2 Time-equivalence */
extern KHE_TIME_EQUIV KheTimeEquivMake(KHE_SOLN soln);
extern void KheTimeEquivDelete(KHE_TIME_EQUIV te);
extern void KheTimeEquivSolve(KHE_TIME_EQUIV te);

extern KHE_TIME_EQUIV KheTimeEquivOption(KHE_OPTIONS options,
  char *key, KHE_SOLN soln);

extern int KheTimeEquivEventGroupCount(KHE_TIME_EQUIV te);
extern KHE_EVENT_GROUP KheTimeEquivEventGroup(KHE_TIME_EQUIV te, int i);
extern KHE_EVENT_GROUP KheTimeEquivEventEventGroup(KHE_TIME_EQUIV te,
  KHE_EVENT e);
extern int KheTimeEquivEventEventGroupIndex(KHE_TIME_EQUIV te, KHE_EVENT e);

extern int KheTimeEquivResourceGroupCount(KHE_TIME_EQUIV te);
extern KHE_RESOURCE_GROUP KheTimeEquivResourceGroup(KHE_TIME_EQUIV te, int i);
extern KHE_RESOURCE_GROUP KheTimeEquivResourceResourceGroup(KHE_TIME_EQUIV te,
  KHE_RESOURCE r);
extern int KheTimeEquivResourceResourceGroupIndex(KHE_TIME_EQUIV te,
  KHE_RESOURCE r);

/* 9.3.1 Layer construction */
extern KHE_LAYER KheLayerMakeFromResource(KHE_NODE parent_node,
  KHE_RESOURCE r);
extern void KheNodeChildLayersMake(KHE_NODE parent_node);
extern void KheNodeChildLayersReduce(KHE_NODE parent_node);

/* 9.3.2 Layer coordination */
extern void KheCoordinateLayers(KHE_NODE parent_node, bool with_domination);

/* 9.4 Runarounds */
extern bool KheMinimumRunaroundDuration(KHE_NODE parent_node,
  KHE_NODE_TIME_SOLVER time_solver, KHE_OPTIONS options, int *duration);
extern void KheBuildRunarounds(KHE_NODE parent_node,
  KHE_NODE_TIME_SOLVER mrd_solver, KHE_OPTIONS mrd_options,
  KHE_NODE_TIME_SOLVER runaround_solver, KHE_OPTIONS runaround_options);

/* 9.5.1 Node merging */
extern bool KheNodeMergeCheck(KHE_NODE node1, KHE_NODE node2);
extern bool KheNodeMerge(KHE_NODE node1, KHE_NODE node2, KHE_NODE *res);

/* 9.5.2 Node meet splitting and merging */
extern void KheNodeMeetSplit(KHE_NODE node, bool recursive);
extern void KheNodeMeetMerge(KHE_NODE node, bool recursive);

/* 9.5.3 Node moving */
extern bool KheNodeMoveCheck(KHE_NODE child_node, KHE_NODE parent_node);
extern bool KheNodeMove(KHE_NODE child_node, KHE_NODE parent_node);

/* 9.5.4 Vizier nodes */
extern KHE_NODE KheNodeVizierMake(KHE_NODE parent_node);
extern void KheNodeVizierDelete(KHE_NODE parent_node);

/* 9.5.5 Flattening */
extern void KheNodeBypass(KHE_NODE node);
extern void KheNodeFlatten(KHE_NODE parent_node);

/* 9.6 Adding zones */
extern void KheLayerInstallZonesInParent(KHE_LAYER layer);
extern void KheNodeExtendZones(KHE_NODE node);

/* 9.7.1 Analysing split defects */
extern KHE_SPLIT_ANALYSER KheSplitAnalyserMake(KHE_SOLN soln);
extern void KheSplitAnalyserDelete(KHE_SPLIT_ANALYSER sa);
extern KHE_SPLIT_ANALYSER KheSplitAnalyserOption(KHE_OPTIONS options,
  char *key, KHE_SOLN soln);

extern void KheSplitAnalyserAnalyse(KHE_SPLIT_ANALYSER sa, KHE_EVENT e);
extern int KheSplitAnalyserSplitSuggestionCount(KHE_SPLIT_ANALYSER sa);
extern void KheSplitAnalyserSplitSuggestion(KHE_SPLIT_ANALYSER sa, int i,
  int *merged_durn, int *split1_durn);
extern int KheSplitAnalyserMergeSuggestionCount(KHE_SPLIT_ANALYSER sa);
extern void KheSplitAnalyserMergeSuggestion(KHE_SPLIT_ANALYSER sa, int i,
  int *split1_durn, int *split2_durn);
extern void KheSplitAnalyserDebug(KHE_SPLIT_ANALYSER sa, int verbosity,
  int indent, FILE *fp);

/* 9.7.2 Merging adjacent meets */
extern void KheMergeMeets(KHE_SOLN soln);


/*****************************************************************************/
/*                                                                           */
/*    Chapter 10.   Time Solvers                                             */
/*                                                                           */
/*****************************************************************************/

/* 10.2.1 Node assignment functions */
extern bool KheNodeMeetSwapCheck(KHE_NODE node1, KHE_NODE node2);
extern bool KheNodeMeetSwap(KHE_NODE node1, KHE_NODE node2);
extern bool KheNodeMeetRegularAssignCheck(KHE_NODE node, KHE_NODE sibling_node);
extern bool KheNodeMeetRegularAssign(KHE_NODE node, KHE_NODE sibling_node);
extern void KheNodeMeetUnAssign(KHE_NODE node);

/* 10.2.2 Checking for unassigned preassigned meets */
extern void KheCheckForUnassignedPreassignedMeets(KHE_SOLN soln, char *msg);

/* 10.2.3 Kempe and ejecting meet moves */
extern KHE_GROUP_MONITOR KheKempeDemandGroupMonitorMake(KHE_SOLN soln);
extern bool KheKempeMeetMove(KHE_MEET meet, KHE_MEET target_meet,
  int offset, bool preserve_regularity, int *demand, bool *basic,
  KHE_KEMPE_STATS kempe_stats);
extern bool KheKempeMeetMoveTime(KHE_MEET meet, KHE_TIME t,
  bool preserve_regularity, int *demand, bool *basic,
  KHE_KEMPE_STATS kempe_stats);

extern KHE_KEMPE_STATS KheKempeStatsMake(HA_ARENA a);
extern void KheKempeStatsDelete(KHE_KEMPE_STATS kempe_stats);
extern KHE_KEMPE_STATS KheKempeStatsOption(KHE_OPTIONS options, char *key);

extern int KheKempeStatsStepHistoMax(KHE_KEMPE_STATS kempe_stats);
extern int KheKempeStatsStepHistoFrequency(KHE_KEMPE_STATS kempe_stats,
  int step_count);
extern int KheKempeStatsStepHistoTotal(KHE_KEMPE_STATS kempe_stats);
extern float KheKempeStatsStepHistoAverage(KHE_KEMPE_STATS kempe_stats);

extern int KheKempeStatsPhaseHistoMax(KHE_KEMPE_STATS kempe_stats);
extern int KheKempeStatsPhaseHistoFrequency(KHE_KEMPE_STATS kempe_stats,
  int phase_count);
extern int KheKempeStatsPhaseHistoTotal(KHE_KEMPE_STATS kempe_stats);
extern float KheKempeStatsPhaseHistoAverage(KHE_KEMPE_STATS kempe_stats);

extern bool KheEjectingMeetMove(KHE_MEET meet, KHE_MEET target_meet, int offset,
  bool allow_eject, bool preserve_regularity, int *demand, bool *basic);
extern bool KheEjectingMeetMoveTime(KHE_MEET meet, KHE_TIME t,
  bool allow_eject, bool preserve_regularity, int *demand, bool *basic);
extern bool KheBasicMeetMove(KHE_MEET meet, KHE_MEET target_meet,
  int offset, bool preserve_regularity, int *demand);
extern bool KheBasicMeetMoveTime(KHE_MEET meet, KHE_TIME t,
  bool preserve_regularity, int *demand);

extern bool KheTypedMeetMove(KHE_MEET meet, KHE_MEET target_meet, int offset,
  KHE_MOVE_TYPE mt, bool preserve_regularity, int *demand, bool *basic,
  KHE_KEMPE_STATS kempe_stats);
extern bool KheTypedMeetMoveTime(KHE_MEET meet, KHE_TIME t,
  KHE_MOVE_TYPE mt, bool preserve_regularity, int *demand, bool *basic,
  KHE_KEMPE_STATS kempe_stats);

/* 10.3.1 Meet bound groups */
extern KHE_MEET_BOUND_GROUP KheMeetBoundGroupMake(KHE_SOLN soln);
extern void KheMeetBoundGroupAddMeetBound(KHE_MEET_BOUND_GROUP mbg,
  KHE_MEET_BOUND mb);
extern int KheMeetBoundGroupMeetBoundCount(KHE_MEET_BOUND_GROUP mbg);
extern KHE_MEET_BOUND KheMeetBoundGroupMeetBound(KHE_MEET_BOUND_GROUP mbg,
  int i);
extern bool KheMeetBoundGroupDelete(KHE_MEET_BOUND_GROUP mbg);

/* 10.3.2 Exposing resource unavailability */
extern void KheMeetAddUnavailableBound(KHE_MEET meet, KHE_COST min_weight,
  KHE_MEET_BOUND_GROUP mbg);
extern void KheSolnAddUnavailableBounds(KHE_SOLN soln, KHE_COST min_weight,
  KHE_MEET_BOUND_GROUP mbg);

/* 10.3.3 Preventing cluster busy times and limit idle times defects */
extern void KheSolnClusterAndLimitMeetDomains(KHE_SOLN soln,
  KHE_COST min_cluster_weight, KHE_COST min_idle_weight,
  float slack, KHE_MEET_BOUND_GROUP mbg, KHE_OPTIONS options);

/* 10.4 Some basic time solvers */
extern bool KheNodeSimpleAssignTimes(KHE_NODE parent_node, KHE_OPTIONS options);
extern bool KheLayerSimpleAssignTimes(KHE_LAYER layer, KHE_OPTIONS options);

extern bool KheNodeRecursiveAssignTimes(KHE_NODE parent_node,
  KHE_NODE_TIME_SOLVER solver, KHE_OPTIONS options);

extern bool KheNodeUnAssignTimes(KHE_NODE parent_node, KHE_OPTIONS options);
extern bool KheLayerUnAssignTimes(KHE_LAYER layer, KHE_OPTIONS options);

extern bool KheNodeAllChildMeetsAssigned(KHE_NODE parent_node);
extern bool KheLayerAllChildMeetsAssigned(KHE_LAYER layer);

extern bool KheNodePreassignedAssignTimes(KHE_NODE root_node,
  KHE_OPTIONS options);
extern bool KheLayerPreassignedAssignTimes(KHE_LAYER layer,
  KHE_OPTIONS options);

extern bool KheSolnTryMeetUnAssignments(KHE_SOLN soln);

/* 10.5 A time solver for runarounds */
extern bool KheRunaroundNodeAssignTimes(KHE_NODE parent_node,
  KHE_OPTIONS options);

/* 10.6 Extended layer matching with Elm */
extern bool KheElmLayerAssign(KHE_LAYER layer,
  KHE_SPREAD_EVENTS_CONSTRAINT sec, KHE_OPTIONS options);

/* 10.7.1 Node-regular time repair using layer node matching */
extern bool KheLayerNodeMatchingNodeRepairTimes(KHE_NODE parent_node,
  KHE_OPTIONS options);
extern bool KheLayerNodeMatchingLayerRepairTimes(KHE_LAYER layer,
  KHE_OPTIONS options);

/* 10.7.2 Ejection chain time repair */
/* the functions presented in this section are declared elsewhere */

/* 10.7.3 Tree search layer time repair */
extern bool KheTreeSearchLayerRepairTimes(KHE_SOLN soln, KHE_RESOURCE r);
extern bool KheTreeSearchRepairTimes(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  bool with_defects);

/* 10.7.4 Meet set time repair */
extern KHE_MEET_SET_SOLVER KheMeetSetSolveBegin(KHE_SOLN soln, int max_meets);
extern void KheMeetSetSolveAddMeet(KHE_MEET_SET_SOLVER mss, KHE_MEET meet);
extern bool KheMeetSetSolveEnd(KHE_MEET_SET_SOLVER mss);

extern bool KheFuzzyMeetMove(KHE_MEET meet, KHE_MEET target_meet, int offset,
  int width, int depth, int max_meets);

/* 10.8.1 Layer assignments */
extern KHE_LAYER_ASST KheLayerAsstMake(KHE_SOLN soln);
extern void KheLayerAsstDelete(KHE_LAYER_ASST layer_asst);
extern void KheLayerAsstBegin(KHE_LAYER_ASST layer_asst, KHE_LAYER layer);
extern void KheLayerAsstEnd(KHE_LAYER_ASST layer_asst);
extern void KheLayerAsstUndo(KHE_LAYER_ASST layer_asst);
extern void KheLayerAsstRedo(KHE_LAYER_ASST layer_asst);
extern void KheLayerAsstDebug(KHE_LAYER_ASST layer_asst, int verbosity,
  int indent, FILE *fp);

/* 10.8.2 A solver for layered time assignment */
extern bool KheNodeLayeredAssignTimes(KHE_NODE parent_node,KHE_OPTIONS options);
extern int KheNodeLayeredLayerCmp(const void *t1, const void *t2);
extern bool KheLayerParallelAssignTimes(KHE_LAYER layer, KHE_OPTIONS options);

/* 10.9 Putting it all together */
/* ***
extern bool KheCycleNodeAssignTimes(KHE_NODE cycle_node, KHE_OPTIONS options);
*** */
extern bool KheCombinedTimeAssign(KHE_NODE cycle_node, KHE_OPTIONS options);


/*****************************************************************************/
/*                                                                           */
/*    Chapter 11.  Resource-Structural Solvers                               */
/*                                                                           */
/*****************************************************************************/

/* 11.1 Task bound groups */
/* *** replaced by solution adjuster
extern KHE_TASK_BOUND_GROUP KheTaskBoundGroupMake(KHE_SOLN soln);
extern void KheTaskBoundGroupAddTaskBound(KHE_TASK_BOUND_GROUP tbg,
  KHE_TASK_BOUND tb);
extern int KheTaskBoundGroupTaskBoundCount(KHE_TASK_BOUND_GROUP tbg);
extern KHE_TASK_BOUND KheTaskBoundGroupTaskBound(KHE_TASK_BOUND_GROUP tbg,
  int i);
extern bool KheTaskBoundGroupDelete(KHE_TASK_BOUND_GROUP tbg);
*** */

/* 11.1.2 Task tree construction */
extern bool KheTaskTreeMake(KHE_SOLN soln, KHE_SOLN_ADJUSTER sa,
  KHE_OPTIONS options);
/* ***
extern KHE_TA SKING KheTaskingMakeFromResourceType(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt);
extern bool KheTaskingMakeTaskTree(KHE_TASK ING tasking, KHE_SOLN_ADJUSTER sa,
  ** KHE_TASK_BOUND_GROUP tbg, ** KHE_OPTIONS options);
*** */

/* 11.2.1 Classifying resources by available workload */
extern bool KheClassifyResourcesByWorkload(KHE_SOLN soln, KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE_GROUP *rg1, KHE_RESOURCE_GROUP *rg2);

/* 11.2.2 Limits on consecutive days */
extern KHE_CONSEC_SOLVER KheConsecSolverMake(KHE_SOLN soln, KHE_FRAME frame);
extern KHE_SOLN KheConsecSolverSoln(KHE_CONSEC_SOLVER cs);
extern KHE_FRAME KheConsecSolverFrame(KHE_CONSEC_SOLVER cs);
extern void KheConsecSolverDelete(KHE_CONSEC_SOLVER cs);

extern void KheConsecSolverFreeDaysLimits(KHE_CONSEC_SOLVER cs, KHE_RESOURCE r,
  int *history, int *min_limit, int *max_limit);
extern void KheConsecSolverBusyDaysLimits(KHE_CONSEC_SOLVER cs, KHE_RESOURCE r,
  int *history, int *min_limit, int *max_limit);
extern void KheConsecSolverBusyTimesLimits(KHE_CONSEC_SOLVER cs, KHE_RESOURCE r,
  int offset, int *history, int *min_limit, int *max_limit);

extern int KheConsecSolverMaxOffset(KHE_CONSEC_SOLVER cs);

extern int KheConsecSolverNonRigidity(KHE_CONSEC_SOLVER cs, KHE_RESOURCE r);

extern void KheConsecSolverDebug(KHE_CONSEC_SOLVER cs, int verbosity,
  int indent, FILE *fp);

/* 11.2.3 Tighten to partition */
extern bool KheTightenToPartition(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_SOLN_ADJUSTER sa, KHE_OPTIONS options);

/* 11.2.4 Balancing supply and demand */
extern bool KheResourceDemandExceedsSupply(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  int *demand, int *supply, KHE_COST *task_cost, KHE_COST *resource_cost);
/* ***
extern KHE_BALANCE_SOLVER KheBalanceSolverMake(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_FRAME days_frame, HA_ARENA a);
extern int KheBalanceSolverTotalSupply(KHE_BALANCE_SOLVER bs);
extern int KheBalanceSolverTotalDemand(KHE_BALANCE_SOLVER bs);
extern int KheBalanceSolverDemandGroupCount(KHE_BALANCE_SOLVER bs);
extern void KheBalanceSolverDemandGroup(KHE_BALANCE_SOLVER bs, int i,
  KHE_COST *cost_reduction, int *total_durn);
extern KHE_COST KheBalanceSolverMarginalCost(KHE_BALANCE_SOLVER bs);
extern void KheBalanceSolverDebug(KHE_BALANCE_SOLVER bs, int verbosity,
  int indent, FILE *fp);
*** */

/* 11.2.5 Resource flow */
extern KHE_RESOURCE_SET KheFlowResourceNodeResources(
  KHE_FLOW_RESOURCE_NODE frn);
extern bool KheFlowResourceNodeResourceGroup(KHE_FLOW_RESOURCE_NODE frn,
  KHE_RESOURCE_GROUP *rg);
extern int KheFlowResourceNodeCapacity(KHE_FLOW_RESOURCE_NODE frn);
extern bool KheFlowResourceNodeFlow(KHE_FLOW_RESOURCE_NODE frn,
  KHE_FLOW_TASK_NODE *ftn, int *flow);
extern void KheFlowResourceNodeDebug(KHE_FLOW_RESOURCE_NODE frn, int verbosity,
  int indent, FILE *fp);

extern KHE_TASK_SET KheFlowTaskNodeTasks(KHE_FLOW_TASK_NODE ftn);
extern KHE_RESOURCE_GROUP KheFlowTaskNodeDomain(KHE_FLOW_TASK_NODE ftn);
extern int KheFlowTaskNodeCapacity(KHE_FLOW_TASK_NODE ftn);
extern void KheFlowTaskNodeDebug(KHE_FLOW_TASK_NODE ftn, int verbosity,
  int indent, FILE *fp);

extern KHE_FLOW KheFlowMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  bool preserve_assts, bool include_soft);
extern void KheFlowDelete(KHE_FLOW f);
extern int KheFlowResourceNodeCount(KHE_FLOW f);
extern KHE_FLOW_RESOURCE_NODE KheFlowResourceNode(KHE_FLOW f, int i);
extern int KheFlowTaskNodeCount(KHE_FLOW f);
extern KHE_FLOW_TASK_NODE KheFlowTaskNode(KHE_FLOW f, int i);
extern KHE_FLOW_RESOURCE_NODE KheFlowResourceToResourceNode(KHE_FLOW f,
  KHE_RESOURCE r);
extern KHE_FLOW_TASK_NODE KheFlowTaskToTaskNode(KHE_FLOW f, KHE_TASK task);
extern void KheFlowDebug(KHE_FLOW f, int verbosity, int indent, FILE *fp);

/* 11.2.6 Workload packing */
extern bool KheWorkloadPack(KHE_SOLN soln, KHE_OPTIONS options,
  KHE_RESOURCE_TYPE rt, KHE_SOLN_ADJUSTER sa);
  /* KHE_TASK_BOUND_GROUP *tbg); */

/* 11.3.1 Changing the multipliers of cluster busy times monitors */
extern void KheSetClusterMonitorMultipliers(KHE_SOLN soln,
  KHE_SOLN_ADJUSTER sa, char *str, int val);

/* 11.3.2 Tilting the plateau */
extern void KheTiltPlateau(KHE_SOLN soln, KHE_SOLN_ADJUSTER sa);

/* 11.3.3 Propagating unavailable times to resource monitors */
extern bool KhePropagateUnavailableTimes(KHE_SOLN soln, KHE_RESOURCE_TYPE rt);

/* 11.3.4 Changing the minimum limits of cluster busy times monitors */
extern KHE_CLUSTER_MINIMUM_SOLVER KheClusterMinimumSolverMake(HA_ARENA a);
extern void KheClusterMinimumSolverSolve(KHE_CLUSTER_MINIMUM_SOLVER cms,
  KHE_SOLN soln, KHE_OPTIONS options, KHE_RESOURCE_TYPE rt);
extern KHE_SOLN KheClusterMinimumSolverSoln(KHE_CLUSTER_MINIMUM_SOLVER cms);
extern KHE_OPTIONS KheClusterMinimumSolverOptions(
  KHE_CLUSTER_MINIMUM_SOLVER cms);
extern KHE_RESOURCE_TYPE KheClusterMinimumSolverResourceType(
  KHE_CLUSTER_MINIMUM_SOLVER cms);

extern int KheClusterMinimumSolverGroupCount(KHE_CLUSTER_MINIMUM_SOLVER cms);
extern KHE_CLUSTER_MINIMUM_GROUP KheClusterMinimumSolverGroup(
  KHE_CLUSTER_MINIMUM_SOLVER cms, int i);
extern int KheClusterMinimumGroupConstraintCount(KHE_CLUSTER_MINIMUM_GROUP cmg);
extern KHE_CLUSTER_BUSY_TIMES_CONSTRAINT KheClusterMinimumGroupConstraint(
  KHE_CLUSTER_MINIMUM_GROUP cmg, int i);
extern int KheClusterMinimumGroupConstraintOffset(KHE_CLUSTER_MINIMUM_GROUP cmg);
extern int KheClusterMinimumGroupSupply(KHE_CLUSTER_MINIMUM_GROUP cmg);
extern int KheClusterMinimumGroupDemand(KHE_CLUSTER_MINIMUM_GROUP cmg);
extern void KheClusterMinimumGroupDebug(KHE_CLUSTER_MINIMUM_GROUP cmg,
  int verbosity, int indent, FILE *fp);
extern bool KheClusterMinimumSolverMonitorGroup(KHE_CLUSTER_MINIMUM_SOLVER cms,
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm, KHE_CLUSTER_MINIMUM_GROUP *cmg);
extern void KheClusterMinimumSolverSetBegin(KHE_CLUSTER_MINIMUM_SOLVER cms);
extern void KheClusterMinimumSolverSet(KHE_CLUSTER_MINIMUM_SOLVER cms,
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int val);
extern void KheClusterMinimumSolverSetEnd(KHE_CLUSTER_MINIMUM_SOLVER cms,
  bool undo);
extern void KheClusterMinimumSolverSetMulti(KHE_CLUSTER_MINIMUM_SOLVER cms,
  KHE_RESOURCE_GROUP rg);
extern void KheClusterMinimumSolverDebug(KHE_CLUSTER_MINIMUM_SOLVER cms,
  int verbosity, int indent, FILE *fp);

/* 11.3.5 Allowing split assignments */
extern void KheAllowSplitAssignments(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  bool unassigned_only);

/* 11.3.6 Enlarging task domains */
extern void KheEnlargeDomains(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  bool unassigned_only);

/* *** withdrawn
** 11.6.1 Tasker objects **
extern int KheTaskerClassTaskCount(KHE_TASKER_CLASS c);
extern KHE_TASK KheTaskerClassTask(KHE_TASKER_CLASS c, int i);

extern KHE_RESOURCE_GROUP KheTaskerClassDomain(KHE_TASKER_CLASS c);
extern KHE_RESOURCE KheTaskerClassAsstResource(KHE_TASKER_CLASS c);
extern KHE_TIME_SET KheTaskerClassTimeSet(KHE_TASKER_CLASS c);

extern void KheTaskerClassDebug(KHE_TASKER_CLASS c, int verbosity,
  int indent, FILE *fp);

extern KHE_TIME KheTaskerTimeTime(KHE_TASKER_TIME t);

extern int KheTaskerTimeClassCount(KHE_TASKER_TIME t);
extern KHE_TASKER_CLASS KheTaskerTimeClass(KHE_TASKER_TIME t, int i);

extern void KheTaskerTimeDebug(KHE_TASKER_TIME t, int verbosity,
  int indent, FILE *fp);

extern KHE_TASKER KheTaskerMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  ** bool include_assigned_tasks, ** KHE_TASK_SET task_set, HA_ARENA a);

extern KHE_SOLN KheTaskerSoln(KHE_TASKER tr);
extern KHE_RESOURCE_TYPE KheTaskerResourceType(KHE_TASKER tr);
extern HA_ARENA KheTaskerArena(KHE_TASKER tr);

extern int KheTaskerTimeCount(KHE_TASKER tr);
extern KHE_TASKER_TIME KheTaskerTime(KHE_TASKER tr, int i);

extern void KheTaskerDebug(KHE_TASKER tr, int verbosity, int indent, FILE *fp);

** 11.6.2 Tasker support for grouping **
extern void KheTaskerGroupingClear(KHE_TASKER tr);
extern bool KheTaskerGroupingAddClass(KHE_TASKER tr, KHE_TASKER_CLASS c);
extern bool KheTaskerGroupingDeleteClass(KHE_TASKER tr, KHE_TASKER_CLASS c);
extern int KheTaskerGroupingBuild(KHE_TASKER tr, int max_num, char *debug_str);

extern int KheTaskerGroupingClassCount(KHE_TASKER tr);
extern KHE_TASKER_CLASS KheTaskerGroupingClass(KHE_TASKER tr, int i);

extern void KheTaskerAddOverlapFrame(KHE_TASKER tr, KHE_FRAME frame);
extern void KheTaskerDeleteOverlapFrame(KHE_TASKER tr);
** ***
extern void KheTaskerAddOverlapTimeGroup(KHE_TASKER tr, KHE_TIME_GROUP tg);
extern void KheTaskerClearOverlapTimeGroups(KHE_TASKER tr);
*** **

extern bool KheTaskerTimeOverlapsGrouping(KHE_TASKER_TIME t);
extern bool KheTaskerClassOverlapsGrouping(KHE_TASKER_CLASS c);

** ***
extern bool KheTaskerGroupingTestAsstBegin(KHE_TASKER tr, KHE_RESOURCE *r);
extern void KheTaskerGroupingTestAsstEnd(KHE_TASKER tr);
*** **

** 11.6.3 Tasker support for profile grouping **
extern void KheTaskerSetProfileMaxLen(KHE_TASKER tr, int profile_max_len);
extern int KheTaskerProfileMaxLen(KHE_TASKER tr);

extern int KheTaskerProfileTimeGroupCount(KHE_TASKER tr);
extern KHE_PROFILE_TIME_GROUP KheTaskerProfileTimeGroup(KHE_TASKER tr, int i);

extern KHE_PROFILE_TIME_GROUP KheProfileTimeGroupMake(KHE_TASKER tr,
  KHE_TIME_GROUP tg);
extern void KheProfileTimeGroupDelete(KHE_PROFILE_TIME_GROUP ptg);
extern void KheTaskerDeleteProfileTimeGroups(KHE_TASKER tr);

extern KHE_TASKER KheProfileTimeGroupTasker(KHE_PROFILE_TIME_GROUP ptg);
extern KHE_TIME_GROUP KheProfileTimeGroupTimeGroup(KHE_PROFILE_TIME_GROUP ptg);
extern int KheProfileTimeGroupCover(KHE_PROFILE_TIME_GROUP ptg);
extern int KheProfileTimeGroupDomainCount(KHE_PROFILE_TIME_GROUP ptg);
extern KHE_RESOURCE_GROUP KheProfileTimeGroupDomain(KHE_PROFILE_TIME_GROUP ptg,
  int i, int *cover);
extern bool KheProfileTimeGroupContainsDomain(KHE_PROFILE_TIME_GROUP ptg,
  KHE_RESOURCE_GROUP domain, int *cover);
extern bool KheTaskerClassCoversProfileTimeGroup(KHE_TASKER_CLASS c,
  KHE_PROFILE_TIME_GROUP ptg);
extern int KheTaskerClassProfileTimeCount(KHE_TASKER_CLASS c);
extern KHE_TASKER_TIME KheTaskerClassProfileTime(KHE_TASKER_CLASS c, int i);

extern void KheTaskerProfileDebug(KHE_TASKER tr, int verbosity, int indent,
  FILE *fp);

** 11.6.4 Combinatorial grouping **
extern KHE_COMB_SOLVER KheCombSolverMake(KHE_TASKER tr, KHE_FRAME days_frame);
extern KHE_TASKER KheCombSolverTasker(KHE_COMB_SOLVER cs);
extern KHE_FRAME KheCombSolverFrame(KHE_COMB_SOLVER cs);

extern void KheCombSolverClearRequirements(KHE_COMB_SOLVER cs);
extern void KheCombSolverAddTimeGroupRequirement(KHE_COMB_SOLVER cs,
  KHE_TIME_GROUP tg, KHE_COMB_SOLVER_COVER_TYPE cover);
extern void KheCombSolverDeleteTimeGroupRequirement(KHE_COMB_SOLVER cs,
  KHE_TIME_GROUP tg);
extern void KheCombSolverAddClassRequirement(KHE_COMB_SOLVER cs,
  KHE_TASKER_CLASS c, KHE_COMB_SOLVER_COVER_TYPE cover);
extern void KheCombSolverDeleteClassRequirement(KHE_COMB_SOLVER cs,
  KHE_TASKER_CLASS c);

extern void KheCombSolverAddProfileGroupRequirement(KHE_COMB_SOLVER cs,
  KHE_PROFILE_TIME_GROUP ptg, KHE_RESOURCE_GROUP domain);
extern void KheCombSolverDeleteProfileGroupRequirement(KHE_COMB_SOLVER cs,
  KHE_PROFILE_TIME_GROUP ptg);
extern void KheCombSolverAddProfileMaxLenRequirement(KHE_COMB_SOLVER cs);
extern void KheCombSolverDeleteProfileMaxLenRequirement(KHE_COMB_SOLVER cs);
extern void KheCombSolverAddNoSinglesRequirement(KHE_COMB_SOLVER cs);
extern void KheCombSolverDeleteNoSinglesRequirement(KHE_COMB_SOLVER cs);

extern int KheCombSolverSolve(KHE_COMB_SOLVER cs, int max_num,
  KHE_COMB_SOLVER_COST_TYPE ct, char *debug_str);
extern int KheCombSolverSingleTasks(KHE_COMB_SOLVER cs);

extern void KheCombSolverDebug(KHE_COMB_SOLVER cs, int verbosity,
  int indent, FILE *fp);

** 11.6.5 Applying combinatorial grouping **
extern int KheCombGrouping(KHE_COMB_SOLVER cs, KHE_OPTIONS options);

** 11.6.6 Profile grouping **
extern int KheProfileGrouping(KHE_COMB_SOLVER cs, bool extended);

** 11.6.7 Grouping by history **
** *** replaced by KheAssignByHistory
extern int KheGroupByHistory(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_TASK_SET ts);
*** **

** group solvers **
** ***
extern KHE_GROUP_SOLVER KheGroupSolverMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  HA_ARENA a);
extern void KheGroupSolverClearTimeGroups(KHE_GROUP_SOLVER gs);
extern void KheGroupSolverAddTimeGroup(KHE_GROUP_SOLVER gs, KHE_TIME_GROUP tg,
  KHE_GROUP_SOLVER_COVER_TYPE cover);

extern bool KheGroupSolverTestSolve(KHE_GROUP_SOLVER gs,
  KHE_GROUP_SOLVER_COST_TYPE ct, KHE_RESOURCE_GROUP domain,
  bool allow_single, KHE_TASK *leader_task);
extern int KheGroupSolverSolve(KHE_GROUP_SOLVER gs, int max_num,
  KHE_GROUP_SOLVER_COST_TYPE ct, KHE_RESOURCE_GROUP domain,
  bool allow_single, KHE_TASK_SET ts, char *debug_str);
*** **
*** */

/* 11.4 Constraint classes */
extern KHE_CONSTRAINT_CLASS_FINDER KheConstraintClassFinderMake(
  KHE_RESOURCE_TYPE rt, KHE_FRAME days_frame, HA_ARENA a);
extern void KheConstraintClassFinderAddConstraint(
  KHE_CONSTRAINT_CLASS_FINDER ccf, KHE_CONSTRAINT c, int offset);
extern void KheConstraintClassFinderAddCompleteWeekendsConstraints(
  KHE_CONSTRAINT_CLASS_FINDER ccf, bool exact_days);

extern bool KheConstraintTimeGroupsAllSingletons(KHE_CONSTRAINT c);
extern bool KheConstraintTimeGroupsEqualFrame(KHE_CONSTRAINT c, int offset,
  KHE_FRAME days_frame);
extern bool KheConstraintTimeGroupsSubsetFrame(KHE_CONSTRAINT c, int offset,
  KHE_FRAME days_frame);

extern int KheConstraintClassFinderClassCount(KHE_CONSTRAINT_CLASS_FINDER ccf);
extern KHE_CONSTRAINT_CLASS KheConstraintClassFinderClass(
  KHE_CONSTRAINT_CLASS_FINDER ccf, int i);

extern void KheConstraintClassFinderDebug(KHE_CONSTRAINT_CLASS_FINDER ccf,
  int verbosity, int indent, FILE *fp);

extern int KheConstraintClassConstraintCount(KHE_CONSTRAINT_CLASS cc);
extern KHE_CONSTRAINT KheConstraintClassConstraint(KHE_CONSTRAINT_CLASS cc,
  int i, int *offset);

extern bool KheConstraintClassCoversResourceType(KHE_CONSTRAINT_CLASS cc);
extern KHE_CONSTRAINT_TAG KheConstraintClassTag(KHE_CONSTRAINT_CLASS cc);
extern char *KheConstraintClassId(KHE_CONSTRAINT_CLASS cc);

extern int KheConstraintClassTimeGroupCount(KHE_CONSTRAINT_CLASS cc);
extern KHE_TIME_GROUP KheConstraintClassTimeGroup(KHE_CONSTRAINT_CLASS cc,
  int i, KHE_POLARITY *po);

extern bool KheConstraintClassAllPositive(KHE_CONSTRAINT_CLASS cc);
extern bool KheConstraintClassAllNegative(KHE_CONSTRAINT_CLASS cc);
extern bool KheConstraintClassTimeGroupsDisjoint(KHE_CONSTRAINT_CLASS cc);
extern bool KheConstraintClassTimeGroupsCoverWholeCycle(
  KHE_CONSTRAINT_CLASS cc);
extern bool KheConstraintClassHasUniformLimits(KHE_CONSTRAINT_CLASS cc);

extern bool KheConstraintClassAllowZero(KHE_CONSTRAINT_CLASS cc);
extern int KheConstraintClassMinimum(KHE_CONSTRAINT_CLASS cc);
extern int KheConstraintClassMaximum(KHE_CONSTRAINT_CLASS cc);
extern KHE_COST KheConstraintClassCombinedWeight(KHE_CONSTRAINT_CLASS cc);
/* ***
extern KHE_COST KheConstraintClassDevToCost(KHE_CONSTRAINT_CLASS cc, int dev);
*** */
extern KHE_COST KheConstraintClassDeterminantToCost(KHE_CONSTRAINT_CLASS cc,
  int determinant, bool at_end);

extern bool KheConstraintClassResourceAllowZero(KHE_CONSTRAINT_CLASS cc,
  KHE_RESOURCE r);
extern int KheConstraintClassResourceMinimum(KHE_CONSTRAINT_CLASS cc,
  KHE_RESOURCE r);
extern int KheConstraintClassResourceMaximum(KHE_CONSTRAINT_CLASS cc,
  KHE_RESOURCE r);
extern KHE_COST KheConstraintClassResourceCombinedWeight(
  KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r);
/* ***
extern KHE_COST KheConstraintClassResourceDevToCost(KHE_CONSTRAINT_CLASS cc,
  KHE_RESOURCE r, int dev);
*** */
extern KHE_COST KheConstraintClassResourceDeterminantToCost(
  KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r, int determinant, bool at_end);

extern int KheConstraintClassResourceHistory(KHE_CONSTRAINT_CLASS cc,
  KHE_RESOURCE r);
extern int KheConstraintClassResourceMaximumMinusHistory(
  KHE_CONSTRAINT_CLASS cc, KHE_RESOURCE r);

extern void KheConstraintClassDebug(KHE_CONSTRAINT_CLASS cc,
  int verbosity, int indent, FILE *fp);

/* 11.5.1 Task finder objects */
extern KHE_TASK_FINDER KheTaskFinderMake(KHE_SOLN soln, KHE_OPTIONS options,
  HA_ARENA a);
extern int KheTaskFinderLastIndex(KHE_TASK_FINDER tf);
extern KHE_FRAME KheTaskFinderFrame(KHE_TASK_FINDER tf);

extern KHE_INTERVAL KheTaskFinderTaskInterval(KHE_TASK_FINDER tf,
  KHE_TASK task);
extern KHE_INTERVAL KheTaskFinderTaskSetInterval(KHE_TASK_FINDER tf,
  KHE_TASK_SET ts);
extern KHE_INTERVAL KheTaskFinderTimeGroupInterval(KHE_TASK_FINDER tf,
  KHE_TIME_GROUP tg);

extern void KheFindTasksInInterval(KHE_TASK_FINDER tf, KHE_INTERVAL in,
  KHE_RESOURCE_TYPE rt, KHE_RESOURCE from_r, bool allow_preassigned,
  bool allow_partial, KHE_TASK_SET res_ts, KHE_INTERVAL *res_in);
extern bool KheFindFirstRunInInterval(KHE_TASK_FINDER tf, KHE_INTERVAL in,
  KHE_RESOURCE_TYPE rt, KHE_RESOURCE from_r, bool allow_preassigned,
  bool allow_partial, bool sep_need_asst, KHE_TASK_SET res_ts,
  KHE_INTERVAL *res_in);
extern bool KheFindLastRunInInterval(KHE_TASK_FINDER tf, KHE_INTERVAL in,
  KHE_RESOURCE_TYPE rt, KHE_RESOURCE from_r, bool allow_preassigned,
  bool allow_partial, bool sep_need_asst, KHE_TASK_SET res_ts,
  KHE_INTERVAL *res_in);

extern KHE_DAILY_SCHEDULE KheTaskFinderTaskDailySchedule(
  KHE_TASK_FINDER tf, KHE_TASK task);
extern KHE_DAILY_SCHEDULE KheTaskFinderTaskSetDailySchedule(
  KHE_TASK_FINDER tf, KHE_TASK_SET ts);
extern KHE_DAILY_SCHEDULE KheTaskFinderTimeGroupDailySchedule(
  KHE_TASK_FINDER tf, KHE_TIME_GROUP tg);
extern KHE_DAILY_SCHEDULE KheTaskFinderNullDailySchedule(
  KHE_TASK_FINDER tf, KHE_INTERVAL in);

extern void KheDailyScheduleDelete(KHE_DAILY_SCHEDULE ds);

extern KHE_TASK_FINDER KheDailyScheduleTaskFinder(KHE_DAILY_SCHEDULE ds);
extern bool KheDailyScheduleNoOverlap(KHE_DAILY_SCHEDULE ds);
extern KHE_INTERVAL KheDailyScheduleInterval(KHE_DAILY_SCHEDULE ds);
/* ***
extern int KheDailyScheduleFirstDayIndex(KHE_DAILY_SCHEDULE ds);
extern int KheDailyScheduleLastDayIndex(KHE_DAILY_SCHEDULE ds);
*** */
extern KHE_TASK KheDailyScheduleTask(KHE_DAILY_SCHEDULE ds, int day_index);
extern KHE_TIME KheDailyScheduleTime(KHE_DAILY_SCHEDULE ds, int day_index);

/* ***
extern bool KheTaskFinderTaskIntervalAndTimes(KHE_TASK_FINDER tf,
  KHE_TASK task, int *first_index, int *last_index,
  ARRAY_KHE_TIME *times_by_day, ARRAY_KHE_TASK *tasks_by_day);
extern bool KheTaskFinderTaskSetIntervalAndTimes(KHE_TASK_FINDER tf,
  KHE_TASK_SET ts, int *first_index, int *last_index,
  ARRAY_KHE_TIME *times_by_day);
extern bool KheTaskFinderTimeGroupIntervalAndTimes(KHE_TASK_FINDER tf,
  KHE_TIME_GROUP tg, int *first_index, int *last_index,
  ARRAY_KHE_TIME *times_by_day);
*** */

/* ***
extern void KheTaskFinderTaskIntervalAndTimes(KHE_TASK_FINDER tf,
  KHE_TASK task, int *first_index, int *last_index,
  KHE_TIME *first_time, KHE_TIME *last_time);
extern void KheTaskFinderTaskSetIntervalAndTimes(KHE_TASK_FINDER tf,
  KHE_TASK_SET ts, int *first_index, int *last_index,
  KHE_TIME *first_time, KHE_TIME *last_time);
extern void KheTaskFinderTimeGroupIntervalAndTimes(KHE_TASK_FINDER tf,
  KHE_TIME_GROUP tg, int *first_index, int *last_index,
  KHE_TIME *first_time, KHE_TIME *last_time);
*** */

/* 11.6.3 Widened task sets */
/* ***
extern bool KheWidenedTaskSetMake(KHE_TASK_FINDER tf, KHE_RESOURCE from_r,
  KHE_TASK_SET from_r_ts, int max_left_wing_count,
  int max_right_wing_count, KHE_WIDENED_TASK_SET *wts);
extern bool KheWidenedTaskSetMakeFlexible(KHE_TASK_FINDER tf,
  KHE_RESOURCE from_r, KHE_TASK_SET from_r_ts,
  int max_wing_count, KHE_WIDENED_TASK_SET *wts);
extern void KheWidenedTaskSetDelete(KHE_WIDENED_TASK_SET wts);

extern void KheWidenedTaskSetInterval(KHE_WIDENED_TASK_SET wts,
  int left_count, int right_count, int *first_index, int *last_index);
extern void KheWidenedTaskSetFullInterval(KHE_WIDENED_TASK_SET wts,
  int *first_index, int *last_index);

extern bool KheFindMovableWidenedTaskSetRight(KHE_TASK_FINDER tf,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, int days_first_index,
  KHE_WIDENED_TASK_SET *res_wts);
extern bool KheFindMovableWidenedTaskSetLeft(KHE_TASK_FINDER tf,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, int days_last_index,
  KHE_WIDENED_TASK_SET *res_wts);

extern void KheWidenedTaskSetDebug(KHE_WIDENED_TASK_SET wts,
  int left_count, int right_count, int verbosity, int indent, FILE *fp);
*** */

/* 11.6.4 Widened task set moves */
/* ***
extern bool KheWidenedTaskSetMoveCheck(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, bool force, int *max_left_count, int *max_right_count);
extern bool KheWidenedTaskSetMove(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, int left_count, int right_count,
  int *from_r_durn_change, int *to_r_durn_change);
extern void KheWidenedTaskSetMoveDebug(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, int left_count, int right_count,
  int verbosity, int indent, FILE *fp);

extern bool KheWidenedTaskSetMovePartial(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, int first_index, int last_index);
extern void KheWidenedTaskSetMovePartialDebug(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, int first_index, int last_index,
  int verbosity, int indent, FILE *fp);

extern bool KheWidenedTaskSetFindInitial(KHE_WIDENED_TASK_SET wts,
  int wanted_durn, int *first_index, int *last_index);
extern bool KheWidenedTaskSetFindFinal(KHE_WIDENED_TASK_SET wts,
  int wanted_durn, int *first_index, int *last_index);
*** */

/* 11.6.5 Widened task set swaps */
/* ***
extern bool KheWidenedTaskSetSwapCheck(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, bool exact, KHE_TIME_GROUP blocking_tg,
  KHE_MONITOR blocking_m, int *max_left_count, int *max_right_count);
extern bool KheWidenedTaskSetSwap(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, int left_count, int right_count,
  int *from_r_durn_change, int *to_r_durn_change);
extern void KheWidenedTaskSetSwapDebug(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, int left_count, int right_count,
  int verbosity, int indent, FILE *fp);
*** */

/* 11.6.6 Widened task set optimal moves */
/* ***
extern bool KheWidenedTaskSetOptimalMoveCheck(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, KHE_TIME_GROUP blocking_tg, KHE_MONITOR blocking_m);
extern bool KheWidenedTaskSetOptimalMove(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, int *from_r_durn_change, int *to_r_durn_change);
extern void KheWidenedTaskSetOptimalMoveDebug(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, int verbosity, int indent, FILE *fp);
*** */

/* ***
extern bool KheFindTaskRunInitial(KHE_TASK_FINDER tf, KHE_TASK_SET ts,
  int wanted_durn, KHE_TASK_SET res_ts);
extern bool KheFindTaskRunFinal(KHE_TASK_FINDER tf, KHE_TASK_SET ts,
  int wanted_durn, KHE_TASK_SET res_ts);
*** */

/* 11.6.1 Multi-tasks */
extern KHE_MTASK_FINDER KheMTaskFinder(KHE_MTASK mt);
extern char *KheMTaskId(KHE_MTASK mt);

extern KHE_RESOURCE_TYPE KheMTaskResourceType(KHE_MTASK mt);
extern bool KheMTaskIsPreassigned(KHE_MTASK mt, KHE_RESOURCE *r);
extern bool KheMTaskAssignIsFixed(KHE_MTASK mt);
extern KHE_RESOURCE_GROUP KheMTaskDomain(KHE_MTASK mt);
extern int KheMTaskTotalDuration(KHE_MTASK mt);
extern float KheMTaskTotalWorkload(KHE_MTASK mt);

extern bool KheMTaskHasSoleMeet(KHE_MTASK mt, KHE_MEET *meet);
extern bool KheMTaskHasFixedTimes(KHE_MTASK mt);

extern KHE_INTERVAL KheMTaskInterval(KHE_MTASK mt);
extern KHE_TIME KheMTaskDayTime(KHE_MTASK mt, int day_index,
  float *workload_per_time);
extern KHE_TIME_SET KheMTaskTimeSet(KHE_MTASK mt);
/* extern void KheTaskAddTimesToTimeSet(KHE_TASK task, KHE_TIME_SET ts); */

extern bool KheMTaskNoOverlap(KHE_MTASK mt);
extern bool KheMTaskNoGaps(KHE_MTASK mt);

extern int KheMTaskTaskCount(KHE_MTASK mt);
extern KHE_TASK KheMTaskTask(KHE_MTASK mt, int i, KHE_COST *non_asst_cost,
  KHE_COST *asst_cost);

extern void KheTaskNonAsstAndAsstCost(KHE_TASK task, KHE_COST *non_asst_cost,
  KHE_COST *asst_cost);

extern bool KheMTaskMoveResourceCheck(KHE_MTASK mt, KHE_RESOURCE from_r,
  KHE_RESOURCE to_r, bool disallow_preassigned);
extern bool KheMTaskMoveResource(KHE_MTASK mt, KHE_RESOURCE from_r,
  KHE_RESOURCE to_r, bool disallow_preassigned);

extern bool KheMTaskResourceAssignSuggestion(KHE_MTASK mt, KHE_RESOURCE *to_r);

extern int KheMTaskAsstResourceCount(KHE_MTASK mt);
extern KHE_RESOURCE KheMTaskAsstResource(KHE_MTASK mt, int i);

extern int KheMTaskAssignedTaskCount(KHE_MTASK mt);
extern int KheMTaskUnassignedTaskCount(KHE_MTASK mt);
extern bool KheMTaskNeedsAssignment(KHE_MTASK mt);
extern int KheMTaskNeedsAssignmentTaskCount(KHE_MTASK mt);
extern KHE_TASK KheMTaskNeedsAssignmentTask(KHE_MTASK mt, int i,
  KHE_COST *non_asst_cost, KHE_COST *asst_cost);
extern bool KheMTaskContainsNeedlessAssignment(KHE_MTASK mt);

extern bool KheMTaskAddTaskBoundCheck(KHE_MTASK mt, KHE_TASK_BOUND tb);
extern bool KheMTaskAddTaskBound(KHE_MTASK mt, KHE_TASK_BOUND tb);
extern bool KheMTaskDeleteTaskBoundCheck(KHE_MTASK mt, KHE_TASK_BOUND tb);
extern bool KheMTaskDeleteTaskBound(KHE_MTASK mt, KHE_TASK_BOUND tb);

extern int KheMTaskTaskBoundCount(KHE_MTASK mt);
extern KHE_TASK_BOUND KheMTaskTaskBound(KHE_MTASK mt, int i);

extern void KheMTaskDebug(KHE_MTASK mt, int verbosity, int indent, FILE *fp);

/* 11.6.2 Multi-task sets */
extern KHE_MTASK_SET KheMTaskSetMake(KHE_MTASK_FINDER mtf);
extern void KheMTaskSetDelete(KHE_MTASK_SET mts, KHE_MTASK_FINDER mtf);

extern void KheMTaskSetClear(KHE_MTASK_SET mts);
extern void KheMTaskSetClearFromEnd(KHE_MTASK_SET mts, int count);
extern void KheMTaskSetDropFromEnd(KHE_MTASK_SET mts, int n);
extern void KheMTaskSetAddMTask(KHE_MTASK_SET mts, KHE_MTASK mt);
extern void KheMTaskSetAddMTaskSet(KHE_MTASK_SET dst_mts,
  KHE_MTASK_SET src_mts);
extern void KheMTaskSetDeleteMTask(KHE_MTASK_SET mts, KHE_MTASK mt);
extern KHE_MTASK KheMTaskSetLastAndDelete(KHE_MTASK_SET mts);

extern bool KheMTaskSetContainsMTask(KHE_MTASK_SET mts, KHE_MTASK mt, int *pos);
extern int KheMTaskSetMTaskCount(KHE_MTASK_SET mts);
extern KHE_MTASK KheMTaskSetMTask(KHE_MTASK_SET mts, int i);
extern KHE_MTASK KheMTaskSetFirst(KHE_MTASK_SET mts);
extern KHE_MTASK KheMTaskSetLast(KHE_MTASK_SET mts);

extern void KheMTaskSetSort(KHE_MTASK_SET mts,
  int(*compar)(const void *, const void *));
extern void KheMTaskSetUniqueify(KHE_MTASK_SET mts);
extern KHE_INTERVAL KheMTaskSetInterval(KHE_MTASK_SET mts);

extern bool KheMTaskSetMoveResourceCheck(KHE_MTASK_SET mts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, bool disallow_preassigned,
  bool unassign_extreme_unneeded);
extern bool KheMTaskSetMoveResource(KHE_MTASK_SET mts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, bool disallow_preassigned,
  bool unassign_extreme_unneeded);

extern bool KheMTaskSetMoveResourcePartialCheck(KHE_MTASK_SET mts,
  int first_index, int last_index, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  bool disallow_preassigned, bool unassign_extreme_unneeded);
extern bool KheMTaskSetMoveResourcePartial(KHE_MTASK_SET mts,
  int first_index, int last_index, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  bool disallow_preassigned, bool unassign_extreme_unneeded);

extern void KheMTaskSetDebug(KHE_MTASK_SET mts, int verbosity, int indent,
  FILE *fp);
extern void KheMTaskSetDebugPartial(KHE_MTASK_SET mts, int first_index,
  int last_index, int verbosity, int indent, FILE *fp);

/* 11.6.3 Multi-task finders */
extern KHE_MTASK_FINDER KheMTaskFinderMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_FRAME days_frame, bool fixed_times, HA_ARENA a);

extern KHE_SOLN KheMTaskFinderSoln(KHE_MTASK_FINDER mtf);
/* extern KHE_RESOURCE_TYPE KheMTaskFinderResourceType(KHE_MTASK_FINDER mtf); */
extern KHE_FRAME KheMTaskFinderDaysFrame(KHE_MTASK_FINDER mtf);
extern KHE_EVENT_TIMETABLE_MONITOR KheMTaskFinderEventTimetableMonitor(
  KHE_MTASK_FINDER mtf);
extern bool KheMTaskFinderFixedTimes(KHE_MTASK_FINDER mtf);
extern HA_ARENA KheMTaskFinderArena(KHE_MTASK_FINDER mtf);

extern int KheMTaskFinderResourceTypeCount(KHE_MTASK_FINDER mtf);
extern KHE_RESOURCE_TYPE KheMTaskFinderResourceType(KHE_MTASK_FINDER mtf,
  int i);
extern bool KheMTaskFinderHandlesResourceType(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt);

extern int KheMTaskFinderLastIndex(KHE_MTASK_FINDER mtf);
extern int KheMTaskFinderMTaskCount(KHE_MTASK_FINDER mtf);
extern KHE_MTASK KheMTaskFinderMTask(KHE_MTASK_FINDER mtf, int i);
extern void KheMTaskFinderMTaskSort(KHE_MTASK_FINDER mtf,
  int (*compar)(const void *, const void *));
extern int KheMTaskDecreasingDurationCmp(const void *, const void *);

/* ***
extern int KheMTaskFinderMTaskAtTimeCount(KHE_MTASK_FINDER mtf, KHE_TIME time);
extern KHE_MTASK KheMTaskFinderMTaskAtTime(KHE_MTASK_FINDER mtf,
  KHE_TIME time, int i);
*** */

extern KHE_MTASK KheMTaskFinderTaskToMTask(KHE_MTASK_FINDER mtf, KHE_TASK t);

extern KHE_MTASK_SET KheMTaskFinderMTasksInTimeGroup(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg);
extern KHE_MTASK_SET KheMTaskFinderMTasksInInterval(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt, KHE_INTERVAL in);

extern void KheMTaskFinderAddResourceMTasksInInterval(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE r, KHE_INTERVAL in, KHE_MTASK_SET mts);
extern void KheAddResourceProperRootTasksInInterval(KHE_RESOURCE r,
  KHE_INTERVAL in, KHE_SOLN soln, KHE_FRAME days_frame, KHE_TASK_SET ts);

extern bool KheMTaskFinderTaskMove(KHE_MTASK_FINDER mtf, KHE_TASK task,
  KHE_TASK target_task);
extern bool KheMTaskFinderTaskAssign(KHE_MTASK_FINDER mtf, KHE_TASK task,
  KHE_TASK target_task);
extern bool KheMTaskFinderTaskUnAssign(KHE_MTASK_FINDER mtf, KHE_TASK task);
extern bool KheMTaskFinderTaskSwap(KHE_MTASK_FINDER mtf, KHE_TASK task1,
  KHE_TASK task2);
extern void KheMTaskFinderTaskAssignFix(KHE_MTASK_FINDER mtf, KHE_TASK task);
extern void KheMTaskFinderTaskAssignUnFix(KHE_MTASK_FINDER mtf, KHE_TASK task);

/* ***
extern void KheMTaskFinderGroupBegin(KHE_MTASK_FINDER mtf,
  KHE_TASK leader_task);
extern bool KheMTaskFinderGroupAddTask(KHE_MTASK_FINDER mtf, KHE_TASK task);
extern void KheMTaskFinderGroupEnd(KHE_MTASK_FINDER mtf, KHE_SOLN_ADJUSTER sa);
*** */

/* ***
extern void KheMTaskFinderTaskGrouperClear(KHE_MTASK_FINDER mtf);
extern bool KheMTaskFinderTaskGrouperAddTask(KHE_MTASK_FINDER mtf,
  KHE_TASK task);
extern void KheMTaskFinderTaskGrouperDeleteTask(KHE_MTASK_FINDER mtf,
  KHE_TASK task);
extern KHE_COST KheMTaskFinderTaskGrouperCost(KHE_MTASK_FINDER mtf);
*** */
extern KHE_TASK KheMTaskFinderTaskGrouperMakeGroup(KHE_MTASK_FINDER mtf,
  KHE_TASK_GROUPER tg, KHE_SOLN_ADJUSTER sa);
/* ***
extern KHE_TASK KheMTaskFinderTaskGrouperEntryMakeGroup(KHE_MTASK_FINDER mtf,
  KHE_TASK_GROUPER_ENTRY tge, KHE_SOLN_ADJUSTER sa);
*** */


/* *** decided against this
extern void KheMTaskFinderClearCachedMTaskSets(KHE_MTASK_FINDER mtf);
*** */

extern void KheMTaskFinderDebug(KHE_MTASK_FINDER mtf, int verbosity,
  int indent, FILE *fp);

/* 11.7.1 Finding task group domains */
extern KHE_TASK_GROUP_DOMAIN_FINDER KheTaskGroupDomainFinderMake(KHE_SOLN soln,
  HA_ARENA a);
extern KHE_TASK_GROUP_DOMAIN KheTaskGroupDomainMake(
  KHE_TASK_GROUP_DOMAIN_FINDER tgdf, KHE_TASK_GROUP_DOMAIN prev,
  KHE_RESOURCE_GROUP rg);
extern KHE_RESOURCE_GROUP KheTaskGroupDomainValue(KHE_TASK_GROUP_DOMAIN tgd);
  /* KHE_TASK_GROUP_DOMAIN_TYPE *type); */
extern KHE_TASK_BOUND KheTaskGroupDomainTaskBound(KHE_TASK_GROUP_DOMAIN tgd);
extern void KheTaskGroupDomainDebug(KHE_TASK_GROUP_DOMAIN tgd,
  int verbosity, int indent, FILE *fp);

/* dominance testing */
extern void KheTaskGroupDomainFinderDominanceClear(
  KHE_TASK_GROUP_DOMAIN_FINDER tgdf);
extern void KheTaskGroupDomainFinderDominanceAddTaskDomain(
  KHE_TASK_GROUP_DOMAIN_FINDER tgdf, KHE_RESOURCE_GROUP rg);
extern bool KheTaskGroupDomainDominanceTest(KHE_TASK_GROUP_DOMAIN tgd1,
  KHE_TASK_GROUP_DOMAIN tgd2, bool *proper);

/* 11.7.2 The task grouper */
extern KHE_TASK_GROUPER KheTaskGrouperMake(KHE_SOLN soln,
  KHE_FRAME days_frame, KHE_TASK_GROUP_DOMAIN_FINDER tgdf, HA_ARENA a);

extern void KheTaskGrouperClear(KHE_TASK_GROUPER tg);
extern bool KheTaskGrouperAddTaskCheck(KHE_TASK_GROUPER tg, KHE_TASK task);
extern bool KheTaskGrouperAddTask(KHE_TASK_GROUPER tg, KHE_TASK task);
extern void KheTaskGrouperDeleteTask(KHE_TASK_GROUPER tg, KHE_TASK task);
extern void KheTaskGrouperAddHistory(KHE_TASK_GROUPER tg, KHE_RESOURCE r,
  int durn);
extern void KheTaskGrouperDeleteHistory(KHE_TASK_GROUPER tg, KHE_RESOURCE r,
  int durn);

extern int KheTaskGrouperTaskCount(KHE_TASK_GROUPER tg);
extern KHE_TASK KheTaskGrouperTask(KHE_TASK_GROUPER tg, int i);
/* ***
extern void KheTaskGrouperAddDummyEntry(KHE_TASK_GROUPER tg);
extern bool KheTaskGrouperContainsTask(KHE_TASK_GROUPER tg, KHE_TASK task);
extern void KheTaskGrouperCopy(KHE_TASK_GROUPER dst_tg,
  KHE_TASK_GROUPER src_tg);
extern KHE_INTERVAL KheTaskGrouperInterval(KHE_TASK_GROUPER tg);
*** */

extern KHE_COST KheTaskGrouperCost(KHE_TASK_GROUPER tg);
extern KHE_TASK KheTaskGrouperMakeGroup(KHE_TASK_GROUPER tg,
  KHE_SOLN_ADJUSTER sa);
extern void KheTaskGrouperDebug(KHE_TASK_GROUPER tg,
  int verbosity, int indent, FILE *fp);

/* ***
extern bool KheTaskGrouperEntryMakeTask(KHE_TASK_GROUPER tg,
  KHE_TASK_GROUPER_ENTRY prev, KHE_TASK task, bool unchecked,
  KHE_TASK_GROUPER_ENTRY *res);
extern KHE_TASK_GROUPER_ENTRY KheTaskGrouperEntryMakeHistory(
  KHE_TASK_GROUPER tg, KHE_RESOURCE r, int durn);
extern void KheTaskGrouperEntryFree(KHE_TASK_GROUPER_ENTRY tge,
  KHE_TASK_GROUPER tg);
*** */
/* ***
extern bool KheTaskGrouperEntryMakeUnchecked(KHE_TASK_GROUPER_ENTRY prev,
  KHE_TASK task, KHE_FRAME days_frame,
  KHE_TASK_GROUP_DOMAIN_FINDER tgdf, HA_ARENA a,
  KHE_TASK_GROUPER_ENTRY *res);
*** */
/* ***
extern void KheTaskGrouperEntryCopy(KHE_TASK_GROUPER_ENTRY dst_last,
  KHE_TASK_GROUPER_ENTRY src_last);
*** */
/* ***
extern KHE_TASK_GROUPER_ENTRY KheTaskGrouperLastEntry(KHE_TASK_GROUPER tg);
extern KHE_COST KheTaskGrouperEntryCost(KHE_TASK_GROUPER_ENTRY tge,
  KHE_TASK_GROUPER tg);
extern KHE_TASK KheTaskGrouperEntryMakeGroup(KHE_TASK_GROUPER_ENTRY tge,
  KHE_SOLN_ADJUSTER sa);
*** */

/* ***
extern void KheTaskGrouperEntryAddDummy(KHE_TASK_GROUPER_ENTRY prev,
  KHE_TASK_GROUPER_ENTRY next);
*** */

/* ***
extern KHE_TASK_GROUPER_ENTRY_TYPE KheTaskGrouperEntryType(
  KHE_TASK_GROUPER_ENTRY tge);
*** */
/* ***
extern KHE_TASK_GROUPER_ENTRY KheTaskGrouperEntryPrev(
  KHE_TASK_GROUPER_ENTRY tge);
extern KHE_TASK KheTaskGrouperEntryTask(KHE_TASK_GROUPER_ENTRY tge);
extern KHE_INTERVAL KheTaskGrouperEntryInterval(KHE_TASK_GROUPER_ENTRY tge);
extern KHE_TASK_GROUP_DOMAIN KheTaskGrouperEntryDomain(
  KHE_TASK_GROUPER_ENTRY tge);
extern KHE_RESOURCE KheTaskGrouperEntryAssignedResource(
  KHE_TASK_GROUPER_ENTRY tge);

extern void KheTaskGrouperEntryDebug(KHE_TASK_GROUPER_ENTRY tge,
  int verbosity, int indent, FILE *fp);
*** */

extern bool KheTaskGrouperSeparateAddTask(KHE_TASK_GROUPER tg,
  KHE_INTERVAL prev_interval, KHE_TASK_GROUP_DOMAIN prev_domain,
  KHE_RESOURCE prev_assigned_resource, KHE_TASK task,
  KHE_INTERVAL *new_interval, KHE_TASK_GROUP_DOMAIN *new_domain,
  KHE_RESOURCE *new_assigned_resource);
extern bool KheTaskGrouperSeparateAddInitialTask(KHE_TASK_GROUPER tg,
  KHE_TASK task, KHE_INTERVAL *new_interval,
  KHE_TASK_GROUP_DOMAIN *new_domain, KHE_RESOURCE *new_assigned_resource);
extern void KheTaskGrouperSeparateAddHistory(KHE_TASK_GROUPER tg,
  KHE_RESOURCE r, int durn, KHE_INTERVAL *new_interval,
  KHE_TASK_GROUP_DOMAIN *new_domain,
  KHE_RESOURCE *new_assigned_resource);
/* ***
extern bool KheTaskGrouperEntryHasFixedUnassigned(KHE_TASK_GROUPER_ENTRY tge);
*** */

/* *** old version
extern KHE_TASK_GROUPER KheTaskGrouperMake(KHE_FRAME days_frame, HA_ARENA a);
extern void KheTaskGrouperClear(KHE_TASK_GROUPER tg);
extern bool KheTaskGrouperAddTask(KHE_TASK_GROUPER tg, KHE_TASK task);
extern void KheTaskGrouperDeleteTask(KHE_TASK_GROUPER tg, KHE_TASK task);
extern int KheTaskGrouperTaskCount(KHE_TASK_GROUPER tg);
extern KHE_TASK KheTaskGrouperTask(KHE_TASK_GROUPER tg, int i);
extern bool KheTaskGrouperContainsTask(KHE_TASK_GROUPER tg, KHE_TASK task);
extern void KheTaskGrouperCopy(KHE_TASK_GROUPER dst_tg,
  KHE_TASK_GROUPER src_tg);
extern KHE_INTERVAL KheTaskGrouperInterval(KHE_TASK_GROUPER tg);
extern KHE_COST KheTaskGrouperCost(KHE_TASK_GROUPER tg);
extern KHE_TASK KheTaskGrouperMakeGroup(KHE_TASK_GROUPER tg,
  KHE_SOLN_ADJUSTER sa);
extern void KheTaskGrouperDebug(KHE_TASK_GROUPER tg,
  int verbosity, int indent, FILE *fp);

extern bool KheTaskGrouperEntryAddTask(KHE_TASK_GROUPER_ENTRY prev,
  KHE_TASK task, KHE_FRAME days_frame, KHE_TASK_GROUPER_ENTRY next);
extern void KheTaskGrouperEntryCopy(KHE_TASK_GROUPER_ENTRY dst_last,
  KHE_TASK_GROUPER_ENTRY src_last);
extern KHE_INTERVAL KheTaskGrouperEntryInterval(KHE_TASK_GROUPER_ENTRY last);
extern KHE_COST KheTaskGrouperEntryCost(KHE_TASK_GROUPER_ENTRY last,
  KHE_FRAME days_frame);
extern KHE_TASK KheTaskGrouperEntryMakeGroup(KHE_TASK_GROUPER_ENTRY last,
  KHE_SOLN_ADJUSTER sa);

extern void KheTaskGrouperEntryAddDummy(KHE_TASK_GROUPER_ENTRY prev,
  KHE_TASK_GROUPER_ENTRY next);
extern bool KheTaskGrouperEntryIsDummy(KHE_TASK_GROUPER_ENTRY entry);
extern KHE_TASK_GROUPER_ENTRY KheTaskGrouperEntryPrev(
  KHE_TASK_GROUPER_ENTRY entry);
extern KHE_TASK KheTaskGrouperEntryTask(KHE_TASK_GROUPER_ENTRY entry);
*** */

/* 11.7.3 The mtask grouper */
extern KHE_MTASK_GROUPER KheMTaskGrouperMake(KHE_SOLN soln,
  KHE_FRAME days_frame, KHE_TASK_GROUP_DOMAIN_FINDER tgdf, HA_ARENA a);

extern void KheMTaskGrouperClear(KHE_MTASK_GROUPER mtg);
extern bool KheMTaskGrouperAddMTask(KHE_MTASK_GROUPER mtg, KHE_MTASK mt);
extern void KheMTaskGrouperDeleteMTask(KHE_MTASK_GROUPER mtg, KHE_MTASK mt);

extern int KheMTaskGrouperMTaskCount(KHE_MTASK_GROUPER mtg);
extern KHE_MTASK KheMTaskGrouperMTask(KHE_MTASK_GROUPER mtg, int i);
extern bool KheMTaskGrouperContainsMTask(KHE_MTASK_GROUPER mtg, KHE_MTASK mt);
extern void KheMTaskGrouperCopy(KHE_MTASK_GROUPER dst_mtg,
  KHE_MTASK_GROUPER src_mtg);

extern KHE_COST KheMTaskGrouperCost(KHE_MTASK_GROUPER mtg);
extern int KheMTaskGrouperMakeGroups(KHE_MTASK_GROUPER mtg,
  int max_num, KHE_SOLN_ADJUSTER sa);
extern void KheMTaskGrouperDebug(KHE_MTASK_GROUPER mtg,
  int verbosity, int indent, FILE *fp);

/* ***
extern bool KheMTaskGrouperEntryMakeMTask(KHE_MTASK_GROUPER mtg,
  KHE_MTASK_GROUPER_ENTRY prev, KHE_MTASK mt, bool unchecked,
  KHE_MTASK_GROUPER_ENTRY *res);
*** */
/* ***
extern void KheMTaskGrouperEntryAddDummy(KHE_MTASK_GROUPER_ENTRY prev,
  KHE_MTASK_GROUPER_ENTRY next);
void KheMTaskGrouperEntryCopy(KHE_MTASK_GROUPER_ENTRY dst_last,
  KHE_MTASK_GROUPER_ENTRY src_last);
*** */
/* ***
extern void KheMTaskGrouperEntryFree(KHE_MTASK_GROUPER_ENTRY mtge,
  KHE_MTASK_GROUPER mtg);
*** */

/* ***
extern KHE_MTASK_GROUPER_ENTRY KheMTaskGrouperLastEntry(KHE_MTASK_GROUPER mtg);
extern KHE_COST KheMTaskGrouperEntryCost(KHE_MTASK_GROUPER_ENTRY mtge,
  KHE_MTASK_GROUPER mtg);
extern int KheMTaskGrouperEntryMakeGroups(KHE_MTASK_GROUPER_ENTRY mtge,
  int max_num, KHE_MTASK_GROUPER mtg, KHE_SOLN_ADJUSTER sa);

extern KHE_MTASK_GROUPER_ENTRY KheMTaskGrouperEntryPrev(
  KHE_MTASK_GROUPER_ENTRY mtge);
extern KHE_MTASK KheMTaskGrouperEntryMTask(KHE_MTASK_GROUPER_ENTRY mtge);
extern KHE_INTERVAL KheMTaskGrouperEntryInterval(KHE_MTASK_GROUPER_ENTRY mtge);
extern KHE_TASK_GROUP_DOMAIN KheMTaskGrouperEntryDomain(
  KHE_MTASK_GROUPER_ENTRY mtge);
*** */

/* 11.7.4 Simple grouping */
extern KHE_SIMPLE_GROUPER KheSimpleGrouperMake(KHE_SOLN soln,
  KHE_FRAME days_frame, KHE_TASK_GROUP_DOMAIN_FINDER tgdf, HA_ARENA a);
extern void KheSimpleGrouperClear(KHE_SIMPLE_GROUPER sg);
extern bool KheSimpleGrouperAddTask(KHE_SIMPLE_GROUPER sg, KHE_TASK task);
extern void KheSimpleGrouperAddResourceTypeTasks(KHE_SIMPLE_GROUPER sg,
  KHE_RESOURCE_TYPE rt);
extern void KheSimpleGrouperAddAssignedResourceTypeTasks(KHE_SIMPLE_GROUPER sg,
  KHE_RESOURCE_TYPE rt);
extern void KheSimpleGrouperMakeGroups(KHE_SIMPLE_GROUPER sg,
  KHE_SIMPLE_GROUPER_GROUP_TYPE group_type, KHE_SOLN_ADJUSTER sa);
extern int KheSimpleGrouperGroupCount(KHE_SIMPLE_GROUPER sg);
extern KHE_TASK KheSimpleGrouperGroup(KHE_SIMPLE_GROUPER sg, int i);

/* 11.7.3 Grouping by resource */
/* *** replaced by task multi-grouper
extern bool KheGroupByResource(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa);
extern bool KheTaskingGroupByResource(KHE_TASK ING tasking, KHE_OPTIONS options,
  KHE_TASK_SET ts);
*** */

/* 11.7.5 Combinatorial grouping */
extern int KheCombinatorialGrouping(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_TASK_GROUP_DOMAIN_FINDER tgdf,
  KHE_SOLN_ADJUSTER sa);

/* 11.7.6 Weekend grouping */
extern void KheWeekendGrouping(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_TASK_GROUP_DOMAIN_FINDER tgdf,
  KHE_SOLN_ADJUSTER sa);

/* 11.7.7 Interval grouping */
extern int KheIntervalGrouping(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_TASK_GROUP_DOMAIN_FINDER tgdf,
  KHE_SOLN_ADJUSTER sa);

/* 11.7.8 Displaying grouped tasks */
extern KHE_GROUPED_TASKS_DISPLAY KheGroupedTasksDisplayMake(KHE_SOLN soln,
  char *id, int min_limit, int max_limit, KHE_COST cost,
  KHE_FRAME days_frame, HA_ARENA a);
extern void KheGroupedTasksDisplayGroupBegin(KHE_GROUPED_TASKS_DISPLAY gtd,
  bool optional, int primary_durn, int index_in_soln);
extern void KheGroupedTasksDisplayGroupAddTask(KHE_GROUPED_TASKS_DISPLAY gtd,
  KHE_TASK task);
extern void KheGroupedTasksDisplayGroupAddHistory(KHE_GROUPED_TASKS_DISPLAY gtd,
  KHE_RESOURCE r, int durn);
extern void KheGroupedTasksDisplayGroupEnd(KHE_GROUPED_TASKS_DISPLAY gtd);
extern void KheGroupedTasksDisplayPrint(KHE_GROUPED_TASKS_DISPLAY gtd,
  bool show_asst, int indent, FILE *fp);
/* ***
extern bool KheGroupedTasksDisplayCompatible(KHE_GROUPED_TASKS_DISPLAY gtd1,
  KHE_GROUPED_TASKS_DISPLAY gtd2, int frame_index,
  KHE_TASK_GROUP_DOMAIN_FINDER domain_finder, HA_ARENA a);
*** */

/* 11.6.4 The task resource grouper */
/* ***
extern KHE_TASK_RESOURCE_GROUPER KheTaskResourceGrouperMake(
  KHE_RESOURCE_TYPE rt, HA_ARENA a);
extern void KheTaskResourceGrouperClear(KHE_TASK_RESOURCE_GROUPER trg);
extern bool KheTaskResourceGrouperAddTask(KHE_TASK_RESOURCE_GROUPER trg,
  KHE_TASK t);
extern KHE_TASK KheTaskGrouperLeaderTask(KHE_TASK_RESOURCE_GROUPER trg,
  KHE_RESOURCE r);
extern void KheTaskResourceGrouperGroup(KHE_TASK_RESOURCE_GROUPER trg);
extern void KheTaskResourceGrouperUnGroup(KHE_TASK_RESOURCE_GROUPER trg);
*** */

/* 11.7 Grouping by resource constraints */
/* ***
extern bool KheGroupByResourceConstraints(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa);
*** */

/* 11.7.1 Combinatorial grouping */
/* *** moved to khe_sr_tgrc.h
extern KHE_COMB_GROUPER KheC ombGrouperMake(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt, HA_ARENA a);
extern KHE_MTASK_FINDER KheCombGrouperMTaskFinder(KHE_COMB_GROUPER cg);
extern KHE_RESOURCE_TYPE KheCombGrouperResourceType(KHE_COMB_GROUPER cg);

extern void KheCombGrouperClearRequirements(KHE_COMB_GROUPER cg);
extern void KheCombGrouperAddTimeGroupRequirement(KHE_COMB_GROUPER cg,
  KHE_TIME_GROUP tg, KHE_COMB_COVER_TYPE cover);
extern void KheCombGrouperDeleteTimeGroupRequirement(KHE_COMB_GROUPER cg,
  KHE_TIME_GROUP tg);
extern void KheCombGrouperAddMTaskRequirement(KHE_COMB_GROUPER cg,
  KHE_MTASK mt, KHE_COMB_COVER_TYPE cover);
extern void KheCombGrouperDeleteMTaskRequirement(KHE_COMB_GROUPER cg,
  KHE_MTASK mt);
*** */

/* *** may not be needed
extern void KheCombGrouperAddMTaskFnRequirement(KHE_COMB_GROUPER cg,
  KHE_COMB_MTASK_FN mtask_fn, void *impl);
extern void KheCombGrouperDeleteMTaskFnRequirement(KHE_COMB_GROUPER cg);
extern void KheCombSolverAddMTaskSetFnRequirement(KHE_COMB_GROUPER cg,
  KHE_COMB_MTASK_SET_FN mtask_set_fn, void *impl);
extern void KheCombSolverDeleteMTaskSetFnRequirement(KHE_COMB_GROUPER cg);
*** */

/* ***
extern void KheCombGrouperAddProfileGroupRequirement(KHE_COMB_GROUPER cg,
  KHE_PROFILE_TIME_GROUP ptg, KHE_RESOURCE_GROUP domain);
extern void KheCombGrouperDeleteProfileGroupRequirement(KHE_COMB_GROUPER cg,
  KHE_PROFILE_TIME_GROUP ptg);
extern void KheCombGrouperAddProfileMaxLenRequirement(KHE_COMB_GROUPER cg);
extern void KheCombGrouperDeleteProfileMaxLenRequirement(KHE_COMB_GROUPER cg);
*** */

/* *** moved to khe_sr_tgrc.h
extern void KheCombGrouperAddNoSinglesRequirement(KHE_COMB_GROUPER cg);
extern void KheCombGrouperDeleteNoSinglesRequirement(KHE_COMB_GROUPER cg);

extern void KheCombGrouperAddPreferredDomainRequirement(KHE_COMB_GROUPER cg,
  KHE_RESOURCE_GROUP rg);
extern void KheCombGrouperDeletePreferredDomainRequirement(KHE_COMB_GROUPER cg);

extern int KheCombGrouperSolve(KHE_COMB_GROUPER cg, int max_num,
  KHE_COMB_VARIANT_TYPE cg_variant, KHE_SOLN_ADJUSTER sa,
  char *debug_str);
** extern int KheCombGrouperSingleTasks(KHE_COMB_GROUPER cg); **

extern void KheCombGrouperDebug(KHE_COMB_GROUPER cg, int verbosity,
  int indent, FILE *fp);
*** */

/* 11.7.2 Using combinatorial grouping with combination reduction */
/* *** moved to khe_sr_tgrc.h
extern int KheCombGrouping(KHE_COMB_GROUPER cg, KHE_OPTIONS options,
  KHE_SOLN_ADJUSTER sa);
*** */

/* 11.7.3 Profile grouping */
/* *** moved to khe_sr_tgrc.h
extern int KheProfileGrouping(KHE_COMB_GROUPER cg, bool extended,
  KHE_SOLN_ADJUSTER sa);
*** */


/*****************************************************************************/
/*                                                                           */
/*    Chapter 12.  Resource Solvers                                          */
/*                                                                           */
/*****************************************************************************/

/* 12.2 The resource assignment invariant */
extern void KheAtomicOperationBegin(KHE_SOLN soln, KHE_MARK *mark,
  int *init_count, bool resource_invariant);
extern bool KheAtomicOperationEnd(KHE_SOLN soln, KHE_MARK *mark,
  int *init_count, bool resource_invariant, bool success);
extern void KheDisconnectAllDemandMonitors(KHE_SOLN soln, KHE_RESOURCE_TYPE rt);

/* 12.3 Ordinary, ejecting, and Kempe task and task set moves */
/* extern bool KheTaskSetMove(KHE_TASK_SET ts, KHE_RESOURCE r); */
extern bool KheEjectingTaskMove(KHE_TASK task, KHE_RESOURCE r,
  bool allow_eject);
extern bool KheEjectingTaskMoveFrame(KHE_TASK task, KHE_RESOURCE r,
  bool allow_eject, KHE_FRAME frame);
extern bool KheEjectingTaskSetMove(KHE_TASK_SET ts, KHE_RESOURCE r,
  bool allow_eject);
extern bool KheEjectingTaskSetMoveFrame(KHE_TASK_SET ts, KHE_RESOURCE r,
  bool allow_eject, KHE_FRAME frame);

extern bool KheKempeTaskMove(KHE_TASK task, KHE_RESOURCE r);
extern bool KheKempeTaskMoveFrame(KHE_TASK task, KHE_RESOURCE r,
  KHE_FRAME frame);
extern bool KheKempeTaskSetMove(KHE_TASK_SET ts, KHE_RESOURCE r);
extern bool KheKempeTaskSetMoveFrame(KHE_TASK_SET ts, KHE_RESOURCE r,
  KHE_FRAME frame);

extern bool KheTypedTaskMove(KHE_TASK task, KHE_RESOURCE r, KHE_MOVE_TYPE mt);
extern bool KheTypedTaskMoveFrame(KHE_TASK task, KHE_RESOURCE r,
  KHE_MOVE_TYPE mt, KHE_FRAME frame);
extern bool KheTypedTaskSetMove(KHE_TASK_SET ts, KHE_RESOURCE r,
  KHE_MOVE_TYPE mt);
extern bool KheTypedTaskSetMoveFrame(KHE_TASK_SET ts, KHE_RESOURCE r,
  KHE_MOVE_TYPE mt, KHE_FRAME frame);
extern char *KheMoveTypeShow(KHE_MOVE_TYPE mt);

/* 12.4 Frame operations for resource solvers - obsolete */
/* ***
extern bool KheFrameTaskSet(KHE_FRAME frame, KHE_RESOURCE r, KHE_TASK_SET *ts);

extern void KheFrameSetVisitNum(KHE_FRAME frame, KHE_RESOURCE r, int num);
extern int KheFrameVisitNum(KHE_FRAME frame, KHE_RESOURCE r);
extern bool KheFrameVisited(KHE_FRAME frame, KHE_RESOURCE r, int slack);
extern void KheFrameVisit(KHE_FRAME frame, KHE_RESOURCE r);
extern void KheFrameUnVisit(KHE_FRAME frame, KHE_RESOURCE r);
*** */

/* ***
extern bool KheFrameIsActive(KHE_FRAME frame, KHE_RESOURCE r);
extern bool KheFrameIsInactive(KHE_FRAME frame, KHE_RESOURCE r);
*** */

/* ***
extern KHE_FRAME KheFrameActiveAtLeft(KHE_FRAME frame, KHE_RESOURCE r);
extern KHE_FRAME KheFrameInactiveAtLeft(KHE_FRAME frame, KHE_RESOURCE r);
extern KHE_FRAME KheFrameActiveAtRight(KHE_FRAME frame, KHE_RESOURCE r);
extern KHE_FRAME KheFrameInactiveAtRight(KHE_FRAME frame, KHE_RESOURCE r);
*** */

/* ***
extern void KheFrameIteratorInit(KHE_FRAME_ITERATOR fi, KHE_FRAME frame,
  KHE_RESOURCE r, int extra);
extern bool KheFrameIteratorNext(KHE_FRAME_ITERATOR fi, KHE_FRAME *res);
extern bool KheFrameIteratorNextPair(KHE_FRAME_ITERATOR fi, KHE_FRAME *res);
*** */

/* 12.4.1 Satisfying requested task assignments */
extern bool KheSolnAssignRequestedResources(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options);
extern bool KheMonitorRequestsSpecificBusyTimes(KHE_MONITOR m);

/* 12.4.2 Assignment by history */
extern int KheAssignByHistory(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa);

/* 12.4.3 Most-constrained-first assignment */
extern bool KheMostConstrainedFirstAssignResources(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options);

/* 12.4.4 Resource packing */
extern bool KheResourcePackAssignResources(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options);

/* 12.4.4 Consecutive packing */
/* ***
extern bool KheResourcePackConsecutive(KHE_T ASKING tasking,
  KHE_OPTIONS options);
*** */

/* 12.4.5 Split assignments */
extern bool KheFindSplitResourceAssignments(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options);

/* 12.5 Single resource assignment using dynamic programming */
extern KHE_SINGLE_RESOURCE_SOLVER KheSingleResourceSolverMake(KHE_SOLN soln,
  KHE_OPTIONS options);
extern void KheSingleResourceSolverDelete(KHE_SINGLE_RESOURCE_SOLVER srs);
extern void KheSingleResourceSolverSolve(KHE_SINGLE_RESOURCE_SOLVER srs,
  KHE_RESOURCE r, KHE_SRS_DOM_KIND dom_kind, int min_assts, int max_assts,
  KHE_COST cost_limit);
/* ***
extern void KheSingleResourceSolverAddCostCutoff(KHE_SINGLE_RESOURCE_SOLVER srs,
  KHE_COST cost_cutoff);
*** */
extern int KheSingleResourceSolverTimetableCount(
  KHE_SINGLE_RESOURCE_SOLVER srs);
extern void KheSingleResourceSolverTimetable(KHE_SINGLE_RESOURCE_SOLVER srs,
  int i, int *asst_count, KHE_COST *r_cost /* , KHE_COST *soln_cost */);
extern void KheSingleResourceSolverAdopt(KHE_SINGLE_RESOURCE_SOLVER srs,
  int i);
extern int KheSingleResourceSolverBest(KHE_SINGLE_RESOURCE_SOLVER srs,
  KHE_COST cost_reduction);
/* ***
extern KHE_COST KheSingleResourceSolverMinCostReduction(
  KHE_SINGLE_RESOURCE_SOLVER srs);
*** */
extern void KheSingleResourceSolverDebug(KHE_SINGLE_RESOURCE_SOLVER srs,
  int verbosity, int indent, FILE *fp);
extern void KheSingleResourceSolverTest(KHE_SOLN soln, KHE_OPTIONS options,
  KHE_RESOURCE r);

/* 12.6 Optimal resource reassignment using dynamic programming */
extern KHE_DYNAMIC_RESOURCE_SOLVER KheDynamicResourceSolverMake(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options);
extern void KheDynamicResourceSolverDelete(KHE_DYNAMIC_RESOURCE_SOLVER drs);
extern void KheDynamicResourceSolverAddDayRange(KHE_DYNAMIC_RESOURCE_SOLVER drs,
  int first_day_index, int last_day_index);
extern void KheDynamicResourceSolverAddResource(KHE_DYNAMIC_RESOURCE_SOLVER drs,
  KHE_RESOURCE r);
extern bool KheDynamicResourceSolverSolve(KHE_DYNAMIC_RESOURCE_SOLVER drs,
  bool priqueue, bool extra_selection, bool expand_by_shifts,
  bool shift_pairs, bool correlated_exprs, int daily_expand_limit,
  int daily_prune_trigger, int resource_expand_limit, int dom_approx,
  KHE_DRS_DOM_KIND main_dom_kind, bool cache, KHE_DRS_DOM_KIND cache_dom_kind);
extern void KheDynamicResourceSolverTest(KHE_DYNAMIC_RESOURCE_SOLVER drs,
  bool priqueue, bool extra_selection, bool expand_by_shifts,
  bool shift_pairs, bool correlated_exprs, int daily_expand_limit,
  int daily_prune_trigger, int resource_expand_limit, int dom_approx,
  KHE_DRS_DOM_KIND main_dom_kind, bool cache,
  KHE_DRS_DOM_KIND cache_dom_kind, KHE_COST *cost);
extern void KheDynamicResourceSolverDebug(KHE_DYNAMIC_RESOURCE_SOLVER drs,
  int verbosity, int indent, FILE *fp);

/* ***
extern void KheDynamicResourceSolverTest(KHE_DYNAMIC_RESOURCE_SOLVER drs,
  int soln_limit);
*** */
extern int KheDynamicResourceSolverSolveStatsCount(
  KHE_DYNAMIC_RESOURCE_SOLVER drs);
extern void KheDynamicResourceSolverSolveStats(KHE_DYNAMIC_RESOURCE_SOLVER drs,
  int i, int *solns_made, int *table_size, float *running_time);

extern bool KheDynamicResourceVLSNSolve(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options);
extern void KheDynamicResourceVLSNTest(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options);

extern bool KheDynamicResourceSequentialSolve(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options);

extern bool KheDynamicResourceBalanceWorkloads(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options);

/* ***
extern void KheDynamicResourceSolverAdopt(KHE_DYNAMIC_RESOURCE_SOLVER drs);
extern void KheDynamicResourceSolverTest(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options, KHE_RESOURCE r1,
  KHE_RESOURCE r2, KHE_RESOURCE r3, KHE_RESOURCE r4,
  int first_day_index1, int last_day_index1,
  int first_day_index2, int last_day_index2,
  int first_day_index3, int last_day_index3,
  int first_day_index4, int last_day_index4);
*** */

/* 12.7.1 A solver for resource matching */
extern KHE_RESOURCE_MATCHING_SOLVER KheResourceMatchingSolverMake(KHE_SOLN soln,
  KHE_RESOURCE_GROUP rg, KHE_OPTIONS options, HA_ARENA a);
/* ***
extern void KheResourceMatchingSolverDelete(KHE_RESOURCE_MATCHING_SOLVER rms);
*** */

extern bool KheResourceMatchingSolverSolve(KHE_RESOURCE_MATCHING_SOLVER rms,
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds, bool edge_adjust1_off,
  bool edge_adjust2_off, bool edge_adjust3_off, bool edge_adjust4_off,
  bool ejection_off, /* bool nocost_off, */ KHE_OPTIONS options);

extern KHE_RESOURCE_MATCHING_DEMAND_SET KheResourceMatchingDemandSetMake(
  KHE_RESOURCE_MATCHING_SOLVER rms /* , bool preserve_existing */);
extern void KheResourceMatchingDemandSetAddTime(
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds, KHE_TIME t);
extern void KheResourceMatchingDemandSetAddTimeGroup(
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds, KHE_TIME_GROUP tg);
extern void KheResourceMatchingDemandSetAddFrame(
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds, KHE_FRAME frame);
extern void KheResourceMatchingDemandSetDelete(
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds);
extern void KheResourceMatchingDemandSetClear(
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds);

/* 12.7.3 Time sweep resource assignment */
extern bool KheTimeSweepAssignResources(KHE_SOLN soln, KHE_RESOURCE_GROUP rg,
  KHE_OPTIONS options);

/* 12.7.4 Time sweep with lookahead */
extern bool KheResourceMatchingSolverSolveWithLookahead(
  KHE_RESOURCE_MATCHING_SOLVER rms,
  ARRAY_KHE_RESOURCE_MATCHING_DEMAND_SET *rmds_array,
  int first_index, int last_index, bool edge_adjust1_off,
  bool edge_adjust2_off, bool edge_adjust3_off, bool edge_adjust4_off,
  bool ejection_off, /* bool nocost_off, */ KHE_OPTIONS options);

/* 12.7.5 Resource rematching repair */
extern bool KheResourceRematch(KHE_SOLN soln, KHE_RESOURCE_GROUP rg,
  KHE_OPTIONS options, int variant);

/* 12.8 Run homogenization */
extern void KheRunHomogenize(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options);

/* 12.9 Moving unnecessary assignments */
extern void KheMoveUnnecessaryAssignments(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options);

/* 12.10 Ejection chain repair */
/* the functions presented in this section are declared elsewhere */

/* 12.11 Global load balancing */
extern bool KheGlobalLoadBalance(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options);

/* 12.12 Resource pair repair */
extern bool KheResourcePairReassign(KHE_SOLN soln, KHE_RESOURCE r1,
  KHE_RESOURCE r2, bool resource_invariant, bool fix_splits);
extern bool KheResourcePairRepair(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options);

/* *** replaced by resource reassign repair
extern bool KheResourcePairSimpleReassign(KHE_SOLN soln,
  KHE_RESOURCE r1, KHE_RESOURCE r2, KHE_FRAME frame, int fi, int li,
  bool resource_invariant, int max_assignments);
extern bool KheResourcePairSimpleRepair(KHE_SOLN soln, KHE_OPTIONS options);
extern bool KheResourcePairSimpleBusyRepair(KHE_SOLN soln, KHE_OPTIONS options);

extern bool KheResourceTripleSimpleReassign(KHE_SOLN soln,
  KHE_RESOURCE r1, KHE_RESOURCE r2, KHE_RESOURCE r3, KHE_FRAME frame,
  int fi, int li, bool resource_invariant, int max_assignments);
extern bool KheResourceTripleSimpleRepair(KHE_SOLN soln, KHE_OPTIONS options);

extern bool KheResourcePairRunReassign(KHE_SOLN soln, KHE_OPTIONS options,
  KHE_RESOURCE r1, KHE_RESOURCE r2, int fi, int li,
  bool resource_invariant, int max_assignments);
extern bool KheResourcePairRunRepair(KHE_SOLN soln, KHE_OPTIONS options);

extern bool KheResourcePairSwap(KHE_SOLN soln, KHE_RESOURCE r1,
  KHE_RESOURCE r2, KHE_FRAME frame, int fi, int li, bool resource_invariant);
extern bool KheResourcePairSwapRepair(KHE_SOLN soln, KHE_OPTIONS options);

extern void KheResourcePairAllSwapRepair(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options);
*** */


/* 12.8 Resource run repair */
/* *** replaced by resource reassign repair
extern KHE_RESOURCE_RUN_SOLVER KheResourceRunSolverMake(KHE_SOLN soln,
  KHE_OPTIONS options, bool resource_invariant, int max_assignments);
extern void KheResourceRunSolverDelete(KHE_RESOURCE_RUN_SOLVER rrs);
extern bool KheResourceRunSolverAddResource(KHE_RESOURCE_RUN_SOLVER rrs,
  KHE_RESOURCE r);
extern bool KheResourceRunSolverDeleteResource(KHE_RESOURCE_RUN_SOLVER rrs,
  KHE_RESOURCE r);
extern void KheResourceRunSolverClearResources(KHE_RESOURCE_RUN_SOLVER rrs);
extern bool KheResourceRunSolverSolve(KHE_RESOURCE_RUN_SOLVER rrs,
  int fi, int li);
extern bool KheResourceRunRepair(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options);
*** */

/* 12.13 Resource reassignment */
extern KHE_REASSIGN_SOLVER KheReassignSolverMake(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options);
extern void KheReassignSolverDelete(KHE_REASSIGN_SOLVER rs);
extern bool KheReassignSolverAddResource(KHE_REASSIGN_SOLVER rs,
  KHE_RESOURCE r);
extern bool KheReassignSolverDeleteResource(KHE_REASSIGN_SOLVER rs,
  KHE_RESOURCE r);
extern void KheReassignSolverClearResources(KHE_REASSIGN_SOLVER rs);
extern bool KheReassignSolverSolve(KHE_REASSIGN_SOLVER rs, int first_index,
  int last_index, KHE_REASSIGN_GROUPING grouping, bool allow_partial,
  KHE_REASSIGN_METHOD method, int max_assignments);
extern void KheReassignSolverDebug(KHE_REASSIGN_SOLVER rs,
  int verbosity, int indent, FILE *fp);
extern bool KheReassign1Repair(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options);
extern bool KheReassign2Repair(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options);
extern bool KheReassign3Repair(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options);

/* 12.14 Trying unassignments */
extern bool KheSolnTryTaskUnAssignments(KHE_SOLN soln, bool strict,
  KHE_OPTIONS options);

/* Putting it all together (obsolete) */
/* ***
extern bool KheTaskingAssignResourcesStage1(KHE_TA SKING tasking,
  KHE_OPTIONS options);
extern bool KheTaskingAssignResourcesStage2(KHE_TAS KING tasking,
  KHE_OPTIONS options);
extern bool KheTaskingAssignResourcesStage3(KHE_TAS KING tasking,
  KHE_OPTIONS options);
*** */
/* extern void KheAssignResourcesGlobalCheck(KHE_SOLN soln, char *pos); */

/* 12.15 Putting it all together */
extern bool KheCombinedResourceAssign(KHE_SOLN soln, KHE_OPTIONS options);


/*****************************************************************************/
/*                                                                           */
/*    Chapter 13.  Ejection Chains                                           */
/*                                                                           */
/*****************************************************************************/

/* 13.2 Ejector construction */
extern KHE_EJECTOR KheEjectorMakeBegin( /* char *schedule, */ HA_ARENA a);
extern void KheEjectorMakeEnd(KHE_EJECTOR ej);
/* extern char *KheEjectorSchedules(KHE_EJECTOR ej); */
extern HA_ARENA KheEjectorArena(KHE_EJECTOR ej);

extern void KheEjectorAddAugment(KHE_EJECTOR ej, KHE_MONITOR_TAG tag,
  KHE_EJECTOR_AUGMENT_FN augment_fn, int augment_type);
extern void KheEjectorAddGroupAugment(KHE_EJECTOR ej, int sub_tag,
  KHE_EJECTOR_AUGMENT_FN augment_fn, int augment_type);

/* 13.2 Ejector construction - major schedule */
/* ***
extern int KheEjectorMajorScheduleCount(KHE_EJECTOR ej);
extern KHE_EJECTOR_MAJOR_SCHEDULE KheEjectorMajorSchedule(KHE_EJECTOR ej,
  int i);
*** */

/* ***
extern void KheEjectorMajorScheduleBegin(KHE_EJECTOR ej);
extern void KheEjectorMajorScheduleEnd(KHE_EJECTOR ej);
extern int KheEjectorMajorScheduleMinorScheduleCount(
  KHE_EJECTOR_MAJOR_SCHEDULE ejm);
extern KHE_EJECTOR_MINOR_SCHEDULE KheEjectorMajorScheduleMinorSchedule(
  KHE_EJECTOR_MAJOR_SCHEDULE ejm, int i);
*** */

/* 13.2 Ejector construction - minor schedule */
/* ***
extern void KheEjectorMinorScheduleAdd(KHE_EJECTOR ej,
  int max_depth, bool may_revisit);
extern int KheEjectorMinorScheduleMaxLength(KHE_EJECTOR_MINOR_SCHEDULE ejms);
extern bool KheEjectorMinorScheduleMayRevisit(KHE_EJECTOR_MINOR_SCHEDULE ejms);
*** */

/* 13.2 Ejector construction - other schedule */
/* ***
extern void KheEjectorAddDefaultSchedules(KHE_EJECTOR ej);
extern void KheEjectorSetSchedulesFromString(KHE_EJECTOR ej,
  char *ejector_schedule_string);
*** */

/* 13.3 Ejector solving */
extern bool KheEjectorSolve(KHE_EJECTOR ej, KHE_GROUP_MONITOR start_gm,
  KHE_GROUP_MONITOR continue_gm, char *schedule, KHE_AUGMENT_OPTIONS ao,
  KHE_OPTIONS options);
extern void KheEjectorSolveBegin(KHE_EJECTOR ej, KHE_GROUP_MONITOR start_gm,
  KHE_GROUP_MONITOR continue_gm, char *schedule, KHE_AUGMENT_OPTIONS ao,
  KHE_OPTIONS options);
extern bool KheEjectorSolveEnd(KHE_EJECTOR ej);

extern void KheEjectorAddMonitorCostLimit(KHE_EJECTOR ej,
  KHE_MONITOR m, KHE_COST cost_limit);
extern void KheEjectorAddMonitorCostLimitReducing(KHE_EJECTOR ej,
  KHE_MONITOR m);
extern int KheEjectorMonitorCostLimitCount(KHE_EJECTOR ej);
extern void KheEjectorMonitorCostLimit(KHE_EJECTOR ej, int i,
  KHE_MONITOR *m, KHE_COST *cost_limit, bool *reducing);

extern KHE_GROUP_MONITOR KheEjectorStartGroupMonitor(KHE_EJECTOR ej);
extern KHE_GROUP_MONITOR KheEjectorContinueGroupMonitor(KHE_EJECTOR ej);
extern KHE_OPTIONS KheEjectorOptions(KHE_EJECTOR ej);
extern KHE_SOLN KheEjectorSoln(KHE_EJECTOR ej);
extern KHE_COST KheEjectorTargetCost(KHE_EJECTOR ej);
extern bool KheEjectorCurrMayRevisit(KHE_EJECTOR ej);
extern int KheEjectorCurrLength(KHE_EJECTOR ej);
extern int KheEjectorCurrAugmentCount(KHE_EJECTOR ej);
extern bool KheEjectorCurrDebug(KHE_EJECTOR ej);
extern int KheEjectorCurrDebugIndent(KHE_EJECTOR ej);

extern void KheEjectorRepairBegin(KHE_EJECTOR ej);
extern bool KheEjectorRepairEnd(KHE_EJECTOR ej, int repair_type, bool success);

/* 13.5.3 Statistics describing a single solve */
extern int KheEjectorImprovementCount(KHE_EJECTOR ej);
extern int KheEjectorImprovementNumberOfRepairs(KHE_EJECTOR ej, int i);
extern float KheEjectorImprovementTime(KHE_EJECTOR ej, int i);
extern KHE_COST KheEjectorImprovementCost(KHE_EJECTOR ej, int i);
extern int KheEjectorImprovementDefects(KHE_EJECTOR ej, int i);

extern KHE_COST KheEjectorInitCost(KHE_EJECTOR ej);
extern int KheEjectorInitDefects(KHE_EJECTOR ej);

/* 13.5.4 Statistics describing multiple solves */
extern int KheEjectorImprovementRepairHistoMax(KHE_EJECTOR ej);
extern int KheEjectorImprovementRepairHistoFrequency(KHE_EJECTOR ej,
  int repair_count);
extern int KheEjectorImprovementRepairHistoTotal(KHE_EJECTOR ej);
extern float KheEjectorImprovementRepairHistoAverage(KHE_EJECTOR ej);

extern int KheEjectorImprovementAugmentHistoMax(KHE_EJECTOR ej);
extern int KheEjectorImprovementAugmentHistoFrequency(KHE_EJECTOR ej,
  int augment_count);
extern int KheEjectorImprovementAugmentHistoTotal(KHE_EJECTOR ej);
extern float KheEjectorImprovementAugmentHistoAverage(KHE_EJECTOR ej);

extern int KheEjectorTotalRepairs(KHE_EJECTOR ej, int augment_type);
extern int KheEjectorTotalRepairsByType(KHE_EJECTOR ej, int augment_type,
  int repair_type);
extern int KheEjectorSuccessfulRepairs(KHE_EJECTOR ej, int augment_type);
extern int KheEjectorSuccessfulRepairsByType(KHE_EJECTOR ej, int augment_type,
  int repair_type);

extern void KheEjectorAddAugmentType(KHE_EJECTOR ej, int augment_type,
  char *augment_label);
extern int KheEjectorAugmentTypeCount(KHE_EJECTOR ej);
extern int KheEjectorAugmentType(KHE_EJECTOR ej, int i);
extern char *KheEjectorAugmentTypeLabel(KHE_EJECTOR ej, int augment_type);

extern void KheEjectorAddRepairType(KHE_EJECTOR ej, int repair_type,
  char *repair_label);
extern int KheEjectorRepairTypeCount(KHE_EJECTOR ej);
extern int KheEjectorRepairType(KHE_EJECTOR ej, int i);
extern char *KheEjectorRepairTypeLabel(KHE_EJECTOR ej, int repair_type);

/* 13.6.1 Top-level ejection chains functions */
extern bool KheEjectionChainNodeRepairTimes(KHE_NODE parent_node,
  KHE_OPTIONS options, char *schedule);
extern bool KheEjectionChainLayerRepairTimes(KHE_LAYER layer,
  KHE_OPTIONS options, char *schedule);
extern bool KheEjectionChainRepairResources(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options, char *schedule);
extern bool KheEjectionChainRepairInitialResourceAssignment(
  KHE_GROUP_MONITOR limit_resources_gm, KHE_OPTIONS options, char *schedule);

extern KHE_EJECTOR KheEjectionChainEjectorMake(KHE_OPTIONS options,
  HA_ARENA a);
extern KHE_EJECTOR KheEjectionChainEjectorOption(KHE_OPTIONS options,
  char *key);

/* 13.6.2 Focus groupings for ejection chains */
extern KHE_GROUP_MONITOR KheNodeTimeRepairStartGroupMonitorMake(KHE_NODE node);
extern KHE_GROUP_MONITOR KheLayerTimeRepairStartGroupMonitorMake(
  KHE_LAYER layer);
extern KHE_GROUP_MONITOR KheLayerTimeRepairLongStartGroupMonitorMake(
  KHE_LAYER layer);
extern KHE_GROUP_MONITOR KheStartGroupMonitorMake(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt);
extern KHE_GROUP_MONITOR KheGroupEventMonitors(KHE_SOLN soln,
  KHE_MONITOR_TAG tag, KHE_SUBTAG_STANDARD_TYPE sub_tag);
extern KHE_GROUP_MONITOR KheAllDemandGroupMonitorMake(KHE_SOLN soln);

#endif

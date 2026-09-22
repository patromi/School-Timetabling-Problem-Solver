
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
/*  FILE:         khe_platform.h                                             */
/*  DESCRIPTION:  Include this file whenever you use KHE                     */
/*                                                                           */
/*  This file has two parts:  type declarations and function declarations.   */
/*  Each part is organized to parallel the User's Guide exactly:             */
/*                                                                           */
/*                    Part A:  The Platform                                  */
/*                                                                           */
/*    Chapter 1.   Introduction                                              */
/*    Chapter 2.   Archives and Solution Groups                              */
/*    Chapter 3.   Instances                                                 */
/*    Chapter 4.   Solutions                                                 */
/*    Chapter 5.   Extra Types for Solving                                   */
/*    Chapter 6.   Solution Monitoring                                       */
/*    Chapter 7.   Matchings and Evenness                                    */
/*                                                                           */
/*    Appendix A.  Modules Packaged with KHE                                 */
/*    Appendix B.  Implementation Notes                                      */
/*                                                                           */
/*  This simplifies verifying that KHE offers what the User's Guide says.    */
/*                                                                           */
/*****************************************************************************/
#ifndef KHE_HEADER_FILE
#define KHE_HEADER_FILE

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "kml.h"

#define KHE_ANY_DURATION 0


/*****************************************************************************/
/*                                                                           */
/*                        TYPE DECLARATIONS                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*                    Part A: The Platform                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*    Chapter 1.   Introduction                                              */
/*                                                                           */
/*****************************************************************************/


/*****************************************************************************/
/*                                                                           */
/*    Chapter 2.   Archives and Solution Groups                              */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_MODEL_HIGH_SCHOOL_TIMETABLE,
  KHE_MODEL_EMPLOYEE_SCHEDULE
} KHE_MODEL;

typedef struct khe_archive_rec *KHE_ARCHIVE;
/* typedef struct khe_archive_metadata_rec *KHE_ARCHIVE_METADATA; */
typedef struct khe_soln_group_rec *KHE_SOLN_GROUP;
/* typedef struct khe_soln_group_metadata_rec *KHE_SOLN_GROUP_METADATA; */
typedef struct khe_soln_set_rec *KHE_SOLN_SET;
typedef struct khe_soln_rec *KHE_SOLN;
typedef void (*KHE_ARCHIVE_FN)(KHE_ARCHIVE archive, void *impl);
typedef void (*KHE_SOLN_GROUP_FN)(KHE_SOLN_GROUP soln_group, void *impl);
typedef void (*KHE_SOLN_FN)(KHE_SOLN soln, void *impl);


/*****************************************************************************/
/*                                                                           */
/*    Chapter 3.   Instances                                                 */
/*                                                                           */
/*****************************************************************************/

/* 3.1 Creating instances */
typedef struct khe_instance_rec *KHE_INSTANCE;
/* typedef struct khe_instance_metadata_rec *KHE_INSTANCE_METADATA; */

/* 3.4 Times */
typedef enum {
  KHE_TIME_GROUP_KIND_ORDINARY,
  KHE_TIME_GROUP_KIND_WEEK,
  KHE_TIME_GROUP_KIND_DAY,
  KHE_TIME_GROUP_KIND_SOLN,
  KHE_TIME_GROUP_KIND_AUTO
} KHE_TIME_GROUP_KIND;

typedef struct khe_time_group_rec *KHE_TIME_GROUP;
typedef struct khe_time_rec *KHE_TIME;

/* 3.5 Resources */
typedef struct khe_resource_type_rec *KHE_RESOURCE_TYPE;
typedef struct khe_resource_group_rec *KHE_RESOURCE_GROUP;
typedef struct khe_resource_rec *KHE_RESOURCE;

/* 3.6 Events */
typedef enum {
  KHE_EVENT_GROUP_KIND_COURSE,
  KHE_EVENT_GROUP_KIND_ORDINARY
} KHE_EVENT_GROUP_KIND;

typedef struct khe_event_group_rec *KHE_EVENT_GROUP;
typedef struct khe_event_rec *KHE_EVENT;
typedef struct khe_event_resource_rec *KHE_EVENT_RESOURCE;
typedef struct khe_event_resource_group_rec *KHE_EVENT_RESOURCE_GROUP;

typedef enum {
  KHE_NO,
  KHE_MAYBE,
  KHE_YES
} KHE_MAYBE_TYPE;

/* 3.7 Constraints */
typedef struct khe_constraint_rec *KHE_CONSTRAINT;

typedef enum {
  KHE_STEP_COST_FUNCTION,
  KHE_LINEAR_COST_FUNCTION,
  KHE_QUADRATIC_COST_FUNCTION
} KHE_COST_FUNCTION;

typedef enum {
  KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG,
  KHE_ASSIGN_TIME_CONSTRAINT_TAG,
  KHE_SPLIT_EVENTS_CONSTRAINT_TAG,
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG,
  KHE_PREFER_RESOURCES_CONSTRAINT_TAG,
  KHE_PREFER_TIMES_CONSTRAINT_TAG,
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG,
  KHE_SPREAD_EVENTS_CONSTRAINT_TAG,
  KHE_LINK_EVENTS_CONSTRAINT_TAG,
  KHE_ORDER_EVENTS_CONSTRAINT_TAG,
  KHE_AVOID_CLASHES_CONSTRAINT_TAG,
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG,
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG,
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG,
  /* KHE_LIMIT_EFFORT_CONSTRAINT_TAG, */
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG,
  KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG,
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG,
  KHE_LIMIT_RESOURCES_CONSTRAINT_TAG,
  KHE_CONSTRAINT_TAG_COUNT
} KHE_CONSTRAINT_TAG;

typedef struct khe_limited_time_group_rec *KHE_LIMITED_TIME_GROUP;
typedef struct khe_time_spread_rec *KHE_TIME_SPREAD;

typedef enum {
  KHE_NEGATIVE,
  KHE_POSITIVE
} KHE_POLARITY;

typedef struct khe_assign_resource_constraint_rec
  *KHE_ASSIGN_RESOURCE_CONSTRAINT;
typedef struct khe_assign_time_constraint_rec
  *KHE_ASSIGN_TIME_CONSTRAINT;
typedef struct khe_split_events_constraint_rec
  *KHE_SPLIT_EVENTS_CONSTRAINT;
typedef struct khe_distribute_split_events_constraint_rec
  *KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT;
typedef struct khe_prefer_resources_constraint_rec
  *KHE_PREFER_RESOURCES_CONSTRAINT;
typedef struct khe_prefer_times_constraint_rec
  *KHE_PREFER_TIMES_CONSTRAINT;
typedef struct khe_avoid_split_assignments_constraint_rec
  *KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT;
typedef struct khe_spread_events_constraint_rec
  *KHE_SPREAD_EVENTS_CONSTRAINT;
typedef struct khe_link_events_constraint_rec
  *KHE_LINK_EVENTS_CONSTRAINT;
typedef struct khe_order_events_constraint_rec
  *KHE_ORDER_EVENTS_CONSTRAINT;
typedef struct khe_avoid_clashes_constraint_rec
  *KHE_AVOID_CLASHES_CONSTRAINT;
typedef struct khe_avoid_unavailable_times_constraint_rec
  *KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT;
typedef struct khe_limit_idle_times_constraint_rec
  *KHE_LIMIT_IDLE_TIMES_CONSTRAINT;
typedef struct khe_cluster_busy_times_constraint_rec
  *KHE_CLUSTER_BUSY_TIMES_CONSTRAINT;
/* ***
typedef struct khe_limit_effort_constraint_rec
  *KHE_LIMIT_EFFORT_CONSTRAINT;
*** */
typedef struct khe_limit_busy_times_constraint_rec
  *KHE_LIMIT_BUSY_TIMES_CONSTRAINT;
typedef struct khe_limit_workload_constraint_rec
  *KHE_LIMIT_WORKLOAD_CONSTRAINT;
typedef struct khe_limit_active_intervals_constraint_rec
  *KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT;
typedef struct khe_limit_resources_constraint_rec
  *KHE_LIMIT_RESOURCES_CONSTRAINT;


/*****************************************************************************/
/*                                                                           */
/*    Chapter 4.   Solutions                                                 */
/*                                                                           */
/*****************************************************************************/

typedef int64_t KHE_COST;
#define KheCostMax INT64_MAX
typedef struct khe_meet_rec *KHE_MEET;
/* typedef struct khe_meet_bound_group_rec *KHE_MEET_BOUND_GROUP; */
typedef struct khe_meet_bound_rec *KHE_MEET_BOUND;
typedef struct khe_task_rec *KHE_TASK;
typedef struct khe_task_bound_rec *KHE_TASK_BOUND;
typedef struct khe_mark_rec *KHE_MARK;
typedef struct khe_path_rec *KHE_PATH;

typedef enum {
  KHE_SOLN_INVALID_PLACEHOLDER,
  KHE_SOLN_BASIC_PLACEHOLDER,
  KHE_SOLN_WRITABLE_PLACEHOLDER,
  KHE_SOLN_ORDINARY
} KHE_SOLN_TYPE;

typedef enum {
  KHE_AVAIL_NODE_UNASSIGNABLE_TIME,
  KHE_AVAIL_NODE_UNAVAILABLE_TIME,
  KHE_AVAIL_NODE_LIMIT_BUSY_ZERO,
  KHE_AVAIL_NODE_CLUSTER_BUSY_ZERO,
  KHE_AVAIL_NODE_LIMIT_BUSY,
  KHE_AVAIL_NODE_CLUSTER_BUSY,
  KHE_AVAIL_NODE_CLUSTER_BUSY_MIN,
  KHE_AVAIL_NODE_WORKLOAD,
  KHE_AVAIL_NODE_LEFTOVER
} KHE_AVAIL_NODE_TYPE;

typedef struct khe_avail_solver_rec *KHE_AVAIL_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*    Chapter 5.   Extra Types for Solving                                   */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_node_rec *KHE_NODE;
typedef struct khe_layer_rec *KHE_LAYER;
typedef struct khe_zone_rec *KHE_ZONE;
/* typedef struct khe_tasking_rec *KHE_TASKING; */
typedef struct khe_task_set_rec *KHE_TASK_SET;
typedef struct khe_meet_set_rec *KHE_MEET_SET;
typedef struct khe_time_set_rec *KHE_TIME_SET;
typedef struct khe_resource_set_rec *KHE_RESOURCE_SET;

/* typedef struct khe_frame_make_rec *KHE_FRAME_MAKE; */
typedef struct khe_frame_rec *KHE_FRAME;
/* typedef struct khe_frame_workload_rec *KHE_FRAME_WORKLOAD; */

/* ***
typedef struct khe_frame_rec {
  KHE_FRAME_MAKE		priv;
  int				start_index;
  int				stop_index;
} KHE_FRAME;
*** */


/*****************************************************************************/
/*                                                                           */
/*    Chapter 6.   Solution Monitoring                                       */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_ASSIGN_RESOURCE_MONITOR_TAG,
  KHE_ASSIGN_TIME_MONITOR_TAG,
  KHE_SPLIT_EVENTS_MONITOR_TAG,
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG,
  KHE_PREFER_RESOURCES_MONITOR_TAG,
  KHE_PREFER_TIMES_MONITOR_TAG,
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG,
  KHE_SPREAD_EVENTS_MONITOR_TAG,
  KHE_LINK_EVENTS_MONITOR_TAG,
  KHE_ORDER_EVENTS_MONITOR_TAG,
  KHE_AVOID_CLASHES_MONITOR_TAG,
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG,
  KHE_LIMIT_IDLE_TIMES_MONITOR_TAG,
  KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG,
  /* KHE_LIMIT_EFFORT_MONITOR_TAG, */
  KHE_LIMIT_BUSY_TIMES_MONITOR_TAG,
  KHE_LIMIT_WORKLOAD_MONITOR_TAG,
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG,
  KHE_LIMIT_RESOURCES_MONITOR_TAG,
  KHE_EVENT_TIMETABLE_MONITOR_TAG,
  KHE_RESOURCE_TIMETABLE_MONITOR_TAG,
  KHE_ORDINARY_DEMAND_MONITOR_TAG,
  KHE_WORKLOAD_DEMAND_MONITOR_TAG,
  KHE_EVENNESS_MONITOR_TAG,
  KHE_GROUP_MONITOR_TAG,
  KHE_MONITOR_TAG_COUNT
} KHE_MONITOR_TAG;

typedef struct khe_monitor_rec *KHE_MONITOR;

typedef struct khe_assign_resource_monitor_rec
  *KHE_ASSIGN_RESOURCE_MONITOR;
typedef struct khe_assign_time_monitor_rec
  *KHE_ASSIGN_TIME_MONITOR;
typedef struct khe_split_events_monitor_rec
  *KHE_SPLIT_EVENTS_MONITOR;
typedef struct khe_distribute_split_events_monitor_rec
  *KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR;
typedef struct khe_prefer_resources_monitor_rec
  *KHE_PREFER_RESOURCES_MONITOR;
typedef struct khe_prefer_times_monitor_rec
  *KHE_PREFER_TIMES_MONITOR;
typedef struct khe_avoid_split_assignments_monitor_rec
  *KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR;
typedef struct khe_spread_events_monitor_rec
  *KHE_SPREAD_EVENTS_MONITOR;
typedef struct khe_link_events_monitor_rec
  *KHE_LINK_EVENTS_MONITOR;
typedef struct khe_order_events_monitor_rec
  *KHE_ORDER_EVENTS_MONITOR;
typedef struct khe_avoid_clashes_monitor_rec
  *KHE_AVOID_CLASHES_MONITOR;
typedef struct khe_avoid_unavailable_times_monitor_rec
  *KHE_AVOID_UNAVAILABLE_TIMES_MONITOR;
typedef struct khe_limit_idle_times_monitor_rec
  *KHE_LIMIT_IDLE_TIMES_MONITOR;
typedef struct khe_cluster_busy_times_monitor_rec
  *KHE_CLUSTER_BUSY_TIMES_MONITOR;
/* ***
typedef struct khe_limit_effort_monitor_rec
  *KHE_LIMIT_EFFORT_MONITOR;
*** */
typedef struct khe_limit_busy_times_monitor_rec
  *KHE_LIMIT_BUSY_TIMES_MONITOR;
typedef struct khe_limit_workload_monitor_rec
  *KHE_LIMIT_WORKLOAD_MONITOR;
typedef struct khe_limit_active_intervals_monitor_rec
  *KHE_LIMIT_ACTIVE_INTERVALS_MONITOR;
typedef struct khe_limit_resources_monitor_rec
  *KHE_LIMIT_RESOURCES_MONITOR;
typedef struct khe_event_timetable_monitor_rec
  *KHE_EVENT_TIMETABLE_MONITOR;
typedef struct khe_resource_timetable_monitor_rec
  *KHE_RESOURCE_TIMETABLE_MONITOR;
typedef struct khe_group_monitor_rec
  *KHE_GROUP_MONITOR;

typedef struct khe_trace_rec *KHE_TRACE;


/*****************************************************************************/
/*                                                                           */
/*    Chapter 7.   Matchings and Evenness                                    */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_ordinary_demand_monitor_rec
  *KHE_ORDINARY_DEMAND_MONITOR;

typedef struct khe_workload_demand_monitor_rec
  *KHE_WORKLOAD_DEMAND_MONITOR;

typedef enum {
  KHE_MATCHING_TYPE_EVAL_INITIAL,
  KHE_MATCHING_TYPE_EVAL_TIMES,
  KHE_MATCHING_TYPE_EVAL_RESOURCES,
  KHE_MATCHING_TYPE_SOLVE
} KHE_MATCHING_TYPE;

typedef struct khe_evenness_monitor_rec *KHE_EVENNESS_MONITOR;


/*****************************************************************************/
/*                                                                           */
/*                     FUNCTION DECLARATIONS                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*                    Part A: The Platform                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*    Chapter 1.   Introduction                                              */
/*                                                                           */
/*****************************************************************************/

extern char *KheVersionNumber(void);
extern char *KheVersionBanner(void);


/*****************************************************************************/
/*                                                                           */
/*    Chapter 2.   Archives and Solution Groups                              */
/*                                                                           */
/*****************************************************************************/

/* 2.1 Archives */
extern KHE_ARCHIVE KheArchiveMake(char *id, KHE_MODEL model, HA_ARENA_SET as);
extern char *KheArchiveId(KHE_ARCHIVE archive);
extern KHE_MODEL KheArchiveModel(KHE_ARCHIVE archive);

extern void KheArchiveDelete(KHE_ARCHIVE archive);
extern void KheArchiveClear(KHE_ARCHIVE archive);

extern void KheArchiveMeld(KHE_ARCHIVE dst_archive, KHE_ARCHIVE src_archive);

extern void KheArchiveSetBack(KHE_ARCHIVE archive, void *back);
extern void *KheArchiveBack(KHE_ARCHIVE archive);

extern void KheArchiveSetMetaData(KHE_ARCHIVE archive, char *name,
  char *contributor, char *date, char *description, char *remarks);
extern void KheArchiveMetaData(KHE_ARCHIVE archive, char **name,
  char **contributor, char **date, char **description, char **remarks);
extern char *KheArchiveMetaDataText(KHE_ARCHIVE archive);

/* ***
extern KHE_ARCHIVE_METADATA KheArchiveMetaData(KHE_ARCHIVE archive);
extern KHE_ARCHIVE_METADATA KheArchiveMetaDataMake(char *name,
  char *contributor, char *date, char *description, char *remarks);
extern char *KheArchiveMetaDataName(KHE_ARCHIVE_METADATA md);
extern char *KheArchiveMetaDataContributor(KHE_ARCHIVE_METADATA md);
extern char *KheArchiveMetaDataDate(KHE_ARCHIVE_METADATA md);
extern char *KheArchiveMetaDataDescription(KHE_ARCHIVE_METADATA md);
extern char *KheArchiveMetaDataRemarks(KHE_ARCHIVE_METADATA md);
*** */

extern bool KheArchiveAddInstance(KHE_ARCHIVE archive, KHE_INSTANCE ins);
extern void KheArchiveDeleteInstance(KHE_ARCHIVE archive, KHE_INSTANCE ins);

extern int KheArchiveInstanceCount(KHE_ARCHIVE archive);
extern KHE_INSTANCE KheArchiveInstance(KHE_ARCHIVE archive, int i);
extern bool KheArchiveRetrieveInstance(KHE_ARCHIVE archive, char *id,
  KHE_INSTANCE *ins, int *index);
extern bool KheArchiveContainsInstance(KHE_ARCHIVE archive,
  KHE_INSTANCE ins, int *index);

extern int KheArchiveSolnGroupCount(KHE_ARCHIVE archive);
extern KHE_SOLN_GROUP KheArchiveSolnGroup(KHE_ARCHIVE archive, int i);
extern bool KheArchiveRetrieveSolnGroup(KHE_ARCHIVE archive, char *id,
  KHE_SOLN_GROUP *soln_group);

/* 2.2 Solution groups */
extern bool KheSolnGroupMake(KHE_ARCHIVE archive, char *id,
  KHE_SOLN_GROUP *soln_group);
extern void KheSolnGroupDelete(KHE_SOLN_GROUP soln_group, bool delete_solns);
extern void KheSolnGroupSetBack(KHE_SOLN_GROUP soln_group, void *back);
extern void *KheSolnGroupBack(KHE_SOLN_GROUP soln_group);

extern KHE_ARCHIVE KheSolnGroupArchive(KHE_SOLN_GROUP soln_group);
extern char *KheSolnGroupId(KHE_SOLN_GROUP soln_group);

extern void KheSolnGroupSetMetaData(KHE_SOLN_GROUP soln_group,
  char *contributor, char *date, char *description, char *publication,
  char *remarks);
extern void KheSolnGroupMetaData(KHE_SOLN_GROUP soln_group,
  char **contributor, char **date, char **description, char **publication,
  char **remarks);
extern char *KheSolnGroupMetaDataText(KHE_SOLN_GROUP soln_group);
/* ***
extern KHE_SOLN_GROUP_METADATA KheSolnGroupMetaData(KHE_SOLN_GROUP soln_group);
extern KHE_SOLN_GROUP_METADATA KheSolnGroupMetaDataMake(char *contributor,
  char *date, char *description, char *publication, char *remarks);
extern char *KheSolnGroupMetaDataContributor(KHE_SOLN_GROUP_METADATA md);
extern char *KheSolnGroupMetaDataDate(KHE_SOLN_GROUP_METADATA md);
extern char *KheSolnGroupMetaDataDescription(KHE_SOLN_GROUP_METADATA md);
extern char *KheSolnGroupMetaDataPublication(KHE_SOLN_GROUP_METADATA md);
extern char *KheSolnGroupMetaDataRemarks(KHE_SOLN_GROUP_METADATA md);
*** */

extern void KheSolnGroupAddSoln(KHE_SOLN_GROUP soln_group, KHE_SOLN soln);
extern void KheSolnGroupDeleteSoln(KHE_SOLN_GROUP soln_group, KHE_SOLN soln);

extern int KheSolnGroupSolnCount(KHE_SOLN_GROUP soln_group);
extern KHE_SOLN KheSolnGroupSoln(KHE_SOLN_GROUP soln_group, int i);

extern KHE_SOLN_SET KheSolnGroupInstanceSolnSet(KHE_SOLN_GROUP soln_group,
  KHE_INSTANCE ins);
extern KHE_SOLN_SET KheSolnGroupInstanceSolnSetByIndex(
  KHE_SOLN_GROUP soln_group, int index);

/* 2.3 Solution sets */
extern KHE_SOLN_SET KheSolnSetMake(HA_ARENA a);
/* extern void KheSolnSetDelete(KHE_SOLN_SET ss); */
extern void KheSolnSetClear(KHE_SOLN_SET ss);
extern void KheSolnSetAddSoln(KHE_SOLN_SET ss, KHE_SOLN soln);
extern void KheSolnSetDeleteSoln(KHE_SOLN_SET ss, KHE_SOLN soln);
extern bool KheSolnSetContainsSoln(KHE_SOLN_SET ss, KHE_SOLN soln, int *pos);
extern int KheSolnSetSolnCount(KHE_SOLN_SET ss);
extern KHE_SOLN KheSolnSetSoln(KHE_SOLN_SET ss, int i);
extern void KheSolnSetSort(KHE_SOLN_SET ss,
  int(*compar)(const void *, const void *));
extern void KheSolnSetSortUnique(KHE_SOLN_SET ss,
  int(*compar)(const void *, const void *));
extern int KheIncreasingCostTypedCmp(KHE_SOLN soln1, KHE_SOLN soln2);
extern int KheIncreasingCostCmp(const void *t1, const void *t2);
extern void KheSolnSetDebug(KHE_SOLN_SET ss, int verbosity,
  int indent, FILE *fp);

/* 2.4 Write-only solutions */
/* ***
extern void KheSolnGroupAddSolnWriteOnly(KHE_SOLN_GROUP soln_group,
  KHE_SOLN soln);
*** */

/* 2.4 Reading archives */
extern bool KheArchiveRead(FILE *fp, HA_ARENA_SET as, KHE_ARCHIVE *archive,
  KML_ERROR *ke, bool audit_and_fix, bool resource_type_partitions,
  bool infer_resource_partitions, bool allow_invalid_solns,
  KHE_SOLN_TYPE soln_type, FILE *echo_fp);
extern bool KheArchiveLoad(KHE_ARCHIVE archive, FILE *fp,
  KML_ERROR *ke, bool audit_and_fix, bool resource_type_partitions,
  bool infer_resource_partitions, bool allow_invalid_solns,
  KHE_SOLN_TYPE soln_type, FILE *echo_fp);

/* 2.5 Reading archives incrementally */
extern bool KheArchiveReadIncremental(FILE *fp, HA_ARENA_SET as,
  KHE_ARCHIVE *archive, KML_ERROR *ke, bool audit_and_fix,
  bool resource_type_partitions, bool infer_resource_partitions,
  bool allow_invalid_solns, KHE_SOLN_TYPE soln_type, FILE *echo_fp,
  KHE_ARCHIVE_FN archive_begin_fn, KHE_ARCHIVE_FN archive_end_fn,
  KHE_SOLN_GROUP_FN soln_group_begin_fn,
  KHE_SOLN_GROUP_FN soln_group_end_fn, KHE_SOLN_FN soln_fn, void *impl);

/* 2.6  Reading archives from the command line */
extern KHE_ARCHIVE KheArchiveReadFromCommandLine(int argc, char *argv[],
  int *pos, HA_ARENA_SET as, bool audit_and_fix,
  bool resource_type_partitions, bool infer_resource_partitions,
  bool allow_invalid_solns, KHE_SOLN_TYPE soln_type, FILE *echo_fp);
/* *** KheArchiveReadFromCommandLineIncremental still to do
extern KHE_ARCHIVE KheArchiveReadFromCommandLineIncremental(int argc,
  char *argv[], int *pos, HA_ARENA_SET as, bool audit_and_fix,
  bool infer_resource_partitions,
  bool allow_invalid_solns, FILE *echo_fp,
  KHE_ARCHIVE_FN archive_begin_fn, KHE_ARCHIVE_FN archive_end_fn,
  KHE_SOLN_GROUP_FN soln_group_begin_fn,
  KHE_SOLN_GROUP_FN soln_group_end_fn, KHE_SOLN_FN soln_fn, void *impl);
*** */

/* 2.7 Writing archives */
extern void KheArchiveWrite(KHE_ARCHIVE archive, bool with_reports, FILE *fp);
extern void KheArchiveWriteSolnGroup(KHE_ARCHIVE archive,
  KHE_SOLN_GROUP soln_group, bool with_reports, FILE *fp);
extern void KheArchiveWriteWithoutSolnGroups(KHE_ARCHIVE archive, FILE *fp);


/*****************************************************************************/
/*                                                                           */
/*    Chapter 3.   Instances                                                 */
/*                                                                           */
/*****************************************************************************/

/* 3.1 Creating instances */
extern KHE_INSTANCE KheInstanceMakeBegin(char *id, KHE_MODEL model,
  HA_ARENA_SET as);
extern void KheInstanceDelete(KHE_INSTANCE ins);

extern char *KheInstanceId(KHE_INSTANCE ins);
extern char *KheInstanceName(KHE_INSTANCE ins);
extern KHE_MODEL KheInstanceModel(KHE_INSTANCE ins);

extern int KheInstanceArchiveCount(KHE_INSTANCE ins);
extern KHE_ARCHIVE KheInstanceArchive(KHE_INSTANCE ins, int i);

extern void KheInstanceSetBack(KHE_INSTANCE ins, void *back);
extern void *KheInstanceBack(KHE_INSTANCE ins);

extern bool KheInstanceMakeEnd(KHE_INSTANCE ins, bool audit_and_fix,
  bool resource_type_partitions, bool infer_resource_partitions,
  char **error_message);

extern void KheInstanceSetMetaData(KHE_INSTANCE ins, char *name,
  char *contributor, char *date, char *country, char *description,
  char *remarks);
extern void KheInstanceMetaData(KHE_INSTANCE ins, char **name,
  char **contributor, char **date, char **country, char **description,
  char **remarks);
extern char *KheInstanceMetaDataText(KHE_INSTANCE ins);

/* ***
extern KHE_INSTANCE_METADATA KheInstanceMetaData(KHE_INSTANCE ins);
extern KHE_INSTANCE_METADATA KheInstanceMetaDataMake(char *name,
  char *contributor, char *date, char *country, char *description,
  char *remarks);
extern char *KheInstanceMetaDataName(KHE_INSTANCE_METADATA md);
extern char *KheInstanceMetaDataContributor(KHE_INSTANCE_METADATA md);
extern char *KheInstanceMetaDataDate(KHE_INSTANCE_METADATA md);
extern char *KheInstanceMetaDataCountry(KHE_INSTANCE_METADATA md);
extern char *KheInstanceMetaDataDescription(KHE_INSTANCE_METADATA md);
extern char *KheInstanceMetaDataRemarks(KHE_INSTANCE_METADATA md);
*** */

/* 3.2 Visiting and retrieving the components of instances */
extern int KheInstanceTimeGroupCount(KHE_INSTANCE ins);
extern KHE_TIME_GROUP KheInstanceTimeGroup(KHE_INSTANCE ins, int i);
extern bool KheInstanceRetrieveTimeGroup(KHE_INSTANCE ins, char *id,
  KHE_TIME_GROUP *tg);

extern KHE_TIME_GROUP KheInstanceFullTimeGroup(KHE_INSTANCE ins);
extern KHE_TIME_GROUP KheInstanceEmptyTimeGroup(KHE_INSTANCE ins);

extern int KheInstanceTimeCount(KHE_INSTANCE ins);
extern KHE_TIME KheInstanceTime(KHE_INSTANCE ins, int i);
extern bool KheInstanceRetrieveTime(KHE_INSTANCE ins, char *id,
  KHE_TIME *t);

extern int KheInstanceResourceTypeCount(KHE_INSTANCE ins);
extern KHE_RESOURCE_TYPE KheInstanceResourceType(KHE_INSTANCE ins, int i);
extern bool KheInstanceRetrieveResourceType(KHE_INSTANCE ins, char *id,
  KHE_RESOURCE_TYPE *rt);

extern bool KheInstanceRetrieveResourceGroup(KHE_INSTANCE ins, char *id,
  KHE_RESOURCE_GROUP *rg);
extern bool KheInstanceRetrieveResource(KHE_INSTANCE ins, char *id,
  KHE_RESOURCE *r);
extern int KheInstanceResourceCount(KHE_INSTANCE ins);
extern KHE_RESOURCE KheInstanceResource(KHE_INSTANCE ins, int i);

extern int KheInstanceEventGroupCount(KHE_INSTANCE ins);
extern KHE_EVENT_GROUP KheInstanceEventGroup(KHE_INSTANCE ins, int i);
extern bool KheInstanceRetrieveEventGroup(KHE_INSTANCE ins, char *id,
  KHE_EVENT_GROUP *eg);
extern KHE_EVENT_GROUP KheInstanceFullEventGroup(KHE_INSTANCE ins);
extern KHE_EVENT_GROUP KheInstanceEmptyEventGroup(KHE_INSTANCE ins);

extern int KheInstanceEventCount(KHE_INSTANCE ins);
extern KHE_EVENT KheInstanceEvent(KHE_INSTANCE ins, int i);
extern bool KheInstanceRetrieveEvent(KHE_INSTANCE ins, char *id,
  KHE_EVENT *e);
extern bool KheInstanceAllEventsHavePreassignedTimes(KHE_INSTANCE ins);
extern int KheInstanceMaximumEventDuration(KHE_INSTANCE ins);

extern int KheInstanceEventResourceCount(KHE_INSTANCE ins);
extern KHE_EVENT_RESOURCE KheInstanceEventResource(KHE_INSTANCE ins, int i);

extern int KheInstanceConstraintCount(KHE_INSTANCE ins);
extern KHE_CONSTRAINT KheInstanceConstraint(KHE_INSTANCE ins, int i);
extern bool KheInstanceRetrieveConstraint(KHE_INSTANCE ins, char *id,
  KHE_CONSTRAINT *c);

extern int KheInstanceConstraintOfTypeCount(KHE_INSTANCE ins,
  KHE_CONSTRAINT_TAG constraint_tag);

/* 3.3 Constraint density */
extern int KheInstanceConstraintDensityCount(KHE_INSTANCE ins,
  KHE_CONSTRAINT_TAG constraint_tag);
extern int KheInstanceConstraintDensityTotal(KHE_INSTANCE ins,
  KHE_CONSTRAINT_TAG constraint_tag);

/* 3.4.1 Time groups */
extern bool KheTimeGroupMake(KHE_INSTANCE ins, KHE_TIME_GROUP_KIND kind,
  char *id, char *name, KHE_TIME_GROUP *tg);
extern void KheTimeGroupSetBack(KHE_TIME_GROUP tg, void *back);
extern void *KheTimeGroupBack(KHE_TIME_GROUP tg);

extern KHE_INSTANCE KheTimeGroupInstance(KHE_TIME_GROUP tg);
extern KHE_TIME_GROUP_KIND KheTimeGroupKind(KHE_TIME_GROUP tg);
extern char *KheTimeGroupId(KHE_TIME_GROUP tg);
extern char *KheTimeGroupName(KHE_TIME_GROUP tg);

extern void KheTimeGroupAddTime(KHE_TIME_GROUP tg, KHE_TIME t);
extern void KheTimeGroupSubTime(KHE_TIME_GROUP tg, KHE_TIME t);
extern void KheTimeGroupUnion(KHE_TIME_GROUP tg, KHE_TIME_GROUP tg2);
extern void KheTimeGroupIntersect(KHE_TIME_GROUP tg, KHE_TIME_GROUP tg2);
extern void KheTimeGroupDifference(KHE_TIME_GROUP tg, KHE_TIME_GROUP tg2);

extern int KheTimeGroupTimeCount(KHE_TIME_GROUP tg);
extern KHE_TIME KheTimeGroupTime(KHE_TIME_GROUP tg, int i);

extern bool KheTimeGroupContains(KHE_TIME_GROUP tg, KHE_TIME t, int *pos);
extern bool KheTimeGroupEqual(KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2);
extern bool KheTimeGroupSubset(KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2);
extern bool KheTimeGroupDisjoint(KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2);

extern int KheTimeGroupTypedCmp(KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2);
extern int KheTimeGroupCmp(const void *t1, const void *t2);

extern bool KheTimeGroupIsCompact(KHE_TIME_GROUP tg);
extern int KheTimeGroupOverlap(KHE_TIME_GROUP tg, KHE_TIME time, int durn);

extern KHE_TIME_GROUP KheTimeGroupNeighbour(KHE_TIME_GROUP tg, int delta);

extern void KheTimeGroupDebug(KHE_TIME_GROUP tg, int verbosity,
  int indent, FILE *fp);

/* 3.4.2 Times */
extern bool KheTimeMake(KHE_INSTANCE ins, char *id, char *name,
  bool break_after, KHE_TIME *t);
extern void KheTimeSetBack(KHE_TIME t, void *back);
extern void *KheTimeBack(KHE_TIME t);

extern KHE_INSTANCE KheTimeInstance(KHE_TIME t);
extern char *KheTimeId(KHE_TIME t);
extern char *KheTimeName(KHE_TIME t);
extern bool KheTimeBreakAfter(KHE_TIME t);
extern int KheTimeIndex(KHE_TIME t);

extern int KheTimeTypedCmp(KHE_TIME t1, KHE_TIME t2);
extern int KheTimeCmp(const void *t1, const void *t2);

extern bool KheTimeHasNeighbour(KHE_TIME t, int delta);
extern KHE_TIME KheTimeNeighbour(KHE_TIME t, int delta);

extern bool KheTimeLE(KHE_TIME time1, int delta1, KHE_TIME time2, int delta2);
extern bool KheTimeLT(KHE_TIME time1, int delta1, KHE_TIME time2, int delta2);
extern bool KheTimeGT(KHE_TIME time1, int delta1, KHE_TIME time2, int delta2);
extern bool KheTimeGE(KHE_TIME time1, int delta1, KHE_TIME time2, int delta2);
extern bool KheTimeEQ(KHE_TIME time1, int delta1, KHE_TIME time2, int delta2);
extern bool KheTimeNE(KHE_TIME time1, int delta1, KHE_TIME time2, int delta2);

extern int KheTimeIntervalsOverlap(KHE_TIME time1, int durn1,
  KHE_TIME time2, int durn2);
extern bool KheTimeIntervalsOverlapInterval(KHE_TIME time1, int durn1,
  KHE_TIME time2, int durn2, KHE_TIME *overlap_time, int *overlap_durn);

extern KHE_TIME_GROUP KheTimeSingletonTimeGroup(KHE_TIME t);

extern int KheTimePreassignedEventCount(KHE_TIME t);
extern KHE_EVENT KheTimePreassignedEvent(KHE_TIME t, int i);

/* 3.5.1 Resource types */
extern bool KheResourceTypeMake(KHE_INSTANCE ins, char *id, char *name,
  bool has_partitions, KHE_RESOURCE_TYPE *rt);
extern void KheResourceTypeSetBack(KHE_RESOURCE_TYPE rt, void *back);
extern void *KheResourceTypeBack(KHE_RESOURCE_TYPE rt);

extern KHE_INSTANCE KheResourceTypeInstance(KHE_RESOURCE_TYPE rt);
extern int KheResourceTypeIndex(KHE_RESOURCE_TYPE rt);
extern char *KheResourceTypeId(KHE_RESOURCE_TYPE rt);
extern char *KheResourceTypeName(KHE_RESOURCE_TYPE rt);
extern bool KheResourceTypeHasPartitions(KHE_RESOURCE_TYPE rt);

extern int KheResourceTypeResourceGroupCount(KHE_RESOURCE_TYPE rt);
extern KHE_RESOURCE_GROUP KheResourceTypeResourceGroup(KHE_RESOURCE_TYPE rt,
  int i);
extern bool KheResourceTypeRetrieveResourceGroup(KHE_RESOURCE_TYPE rt,
  char *id, KHE_RESOURCE_GROUP *rg);

extern int KheResourceTypePartitionCount(KHE_RESOURCE_TYPE rt);
extern KHE_RESOURCE_GROUP KheResourceTypePartition(KHE_RESOURCE_TYPE rt, int i);

extern KHE_RESOURCE_GROUP KheResourceTypeFullResourceGroup(
  KHE_RESOURCE_TYPE rt);
extern KHE_RESOURCE_GROUP KheResourceTypeEmptyResourceGroup(
  KHE_RESOURCE_TYPE rt);

extern int KheResourceTypeResourceCount(KHE_RESOURCE_TYPE rt);
extern KHE_RESOURCE KheResourceTypeResource(KHE_RESOURCE_TYPE rt, int i);
extern bool KheResourceTypeRetrieveResource(KHE_RESOURCE_TYPE rt,
  char *id, KHE_RESOURCE *r);

extern bool KheResourceTypeDemandIsAllPreassigned(KHE_RESOURCE_TYPE rt);
extern int KheResourceTypeAvoidSplitAssignmentsCount(KHE_RESOURCE_TYPE rt);
extern int KheResourceTypeLimitResourcesCount(KHE_RESOURCE_TYPE rt);
extern float KheResourceTypeMaxWorkloadPerTime(KHE_RESOURCE_TYPE rt);

/* 3.5.2 Resource groups */
extern bool KheResourceGroupMake(KHE_RESOURCE_TYPE rt, char *id, char *name,
  bool is_partition, KHE_RESOURCE_GROUP *rg);
extern void KheResourceGroupSetBack(KHE_RESOURCE_GROUP rg, void *back);
extern void *KheResourceGroupBack(KHE_RESOURCE_GROUP rg);

extern KHE_RESOURCE_TYPE KheResourceGroupResourceType(KHE_RESOURCE_GROUP rg);
extern KHE_INSTANCE KheResourceGroupInstance(KHE_RESOURCE_GROUP rg);
extern char *KheResourceGroupId(KHE_RESOURCE_GROUP rg);
extern char *KheResourceGroupName(KHE_RESOURCE_GROUP rg);
extern bool KheResourceGroupIsPartition(KHE_RESOURCE_GROUP rg);

extern void KheResourceGroupAddResource(KHE_RESOURCE_GROUP rg, KHE_RESOURCE r);
extern void KheResourceGroupSubResource(KHE_RESOURCE_GROUP rg, KHE_RESOURCE r);
extern void KheResourceGroupUnion(KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE_GROUP rg2);
extern void KheResourceGroupIntersect(KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE_GROUP rg2);
extern void KheResourceGroupDifference(KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE_GROUP rg2);

extern int KheResourceGroupUnionCount(KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE_GROUP rg2);
extern int KheResourceGroupIntersectCount(KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE_GROUP rg2);
extern int KheResourceGroupDifferenceCount(KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE_GROUP rg2);
extern int KheResourceGroupSymmetricDifferenceCount(KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE_GROUP rg2);

extern int KheResourceGroupResourceCount(KHE_RESOURCE_GROUP rg);
extern KHE_RESOURCE KheResourceGroupResource(KHE_RESOURCE_GROUP rg, int i);

extern bool KheResourceGroupContains(KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE r);
extern bool KheResourceGroupEqual(KHE_RESOURCE_GROUP rg1,
  KHE_RESOURCE_GROUP rg2);
extern bool KheResourceGroupSubset(KHE_RESOURCE_GROUP rg1,
  KHE_RESOURCE_GROUP rg2);
extern bool KheResourceGroupDisjoint(KHE_RESOURCE_GROUP rg1,
  KHE_RESOURCE_GROUP rg2);

extern int KheResourceGroupTypedCmp(KHE_RESOURCE_GROUP rg1,
  KHE_RESOURCE_GROUP rg2);
extern int KheResourceGroupCmp(const void *t1, const void *t2);

extern KHE_RESOURCE_GROUP KheResourceGroupPartition(KHE_RESOURCE_GROUP rg);

extern void KheResourceGroupDebug(KHE_RESOURCE_GROUP rg, int verbosity,
  int indent, FILE *fp);

/* 3.5.3 Resources */
extern bool KheResourceMake(KHE_RESOURCE_TYPE rt, char *id, char *name,
  KHE_RESOURCE_GROUP partition, KHE_RESOURCE *r);
extern void KheResourceSetBack(KHE_RESOURCE r, void *back);
extern void *KheResourceBack(KHE_RESOURCE r);

extern KHE_INSTANCE KheResourceInstance(KHE_RESOURCE r);
extern int KheResourceInstanceIndex(KHE_RESOURCE r);
extern KHE_RESOURCE_TYPE KheResourceResourceType(KHE_RESOURCE r);
extern int KheResourceResourceTypeIndex(KHE_RESOURCE r);
extern char *KheResourceId(KHE_RESOURCE r);
extern char *KheResourceName(KHE_RESOURCE r);
extern KHE_RESOURCE_GROUP KheResourcePartition(KHE_RESOURCE r);

extern KHE_RESOURCE_GROUP KheResourceSingletonResourceGroup(KHE_RESOURCE r);
extern int KheResourceResourceGroupCount(KHE_RESOURCE r);
extern KHE_RESOURCE_GROUP KheResourceResourceGroup(KHE_RESOURCE r, int i);

extern int KheResourcePreassignedEventResourceCount(KHE_RESOURCE r);
extern KHE_EVENT_RESOURCE KheResourcePreassignedEventResource(KHE_RESOURCE r,
  int i);

extern int KheResourceConstraintCount(KHE_RESOURCE r);
extern KHE_CONSTRAINT KheResourceConstraint(KHE_RESOURCE r, int i);

extern KHE_TIME_GROUP KheResourceHardUnavailableTimeGroup(KHE_RESOURCE r);
extern KHE_TIME_GROUP KheResourceHardAndSoftUnavailableTimeGroup(
  KHE_RESOURCE r);

extern bool KheResourceHasAvoidClashesConstraint(KHE_RESOURCE r, KHE_COST cost);
extern int KheResourcePreassignedEventsDuration(KHE_RESOURCE r, KHE_COST cost);

extern int KheResourceTypedCmp(KHE_RESOURCE r1, KHE_RESOURCE r2);
extern int KheResourceCmp(const void *t1, const void *t2);

extern void KheResourceDebug(KHE_RESOURCE r, int verbosity,
  int indent, FILE *fp);

/* 3.5.4 Resource layers */
extern int KheResourceLayerEventCount(KHE_RESOURCE r);
extern KHE_EVENT KheResourceLayerEvent(KHE_RESOURCE r, int i);
extern int KheResourceLayerDuration(KHE_RESOURCE r);

/* 3.5.5 Resource similarity and inferring resource partitions */
extern bool KheResourceSimilar(KHE_RESOURCE r1, KHE_RESOURCE r2);

/* 3.6.1 Event groups */
extern bool KheEventGroupMake(KHE_INSTANCE ins, KHE_EVENT_GROUP_KIND kind,
  char *id, char *name, KHE_EVENT_GROUP *eg);
extern void KheEventGroupSetBack(KHE_EVENT_GROUP eg, void *back);
extern void *KheEventGroupBack(KHE_EVENT_GROUP eg);

extern KHE_INSTANCE KheEventGroupInstance(KHE_EVENT_GROUP eg);
extern KHE_EVENT_GROUP_KIND KheEventGroupKind(KHE_EVENT_GROUP eg);
extern char *KheEventGroupId(KHE_EVENT_GROUP eg);
extern char *KheEventGroupName(KHE_EVENT_GROUP eg);

extern void KheEventGroupAddEvent(KHE_EVENT_GROUP eg, KHE_EVENT e);
extern void KheEventGroupSubEvent(KHE_EVENT_GROUP eg, KHE_EVENT e);
extern void KheEventGroupUnion(KHE_EVENT_GROUP eg, KHE_EVENT_GROUP eg2);
extern void KheEventGroupIntersect(KHE_EVENT_GROUP eg, KHE_EVENT_GROUP eg2);
extern void KheEventGroupDifference(KHE_EVENT_GROUP eg, KHE_EVENT_GROUP eg2);

extern int KheEventGroupEventCount(KHE_EVENT_GROUP eg);
extern KHE_EVENT KheEventGroupEvent(KHE_EVENT_GROUP eg, int i);

extern bool KheEventGroupContains(KHE_EVENT_GROUP eg, KHE_EVENT e);
extern bool KheEventGroupEqual(KHE_EVENT_GROUP eg1, KHE_EVENT_GROUP eg2);
extern bool KheEventGroupSubset(KHE_EVENT_GROUP eg1, KHE_EVENT_GROUP eg2);
extern bool KheEventGroupDisjoint(KHE_EVENT_GROUP eg1, KHE_EVENT_GROUP eg2);

extern int KheEventGroupConstraintCount(KHE_EVENT_GROUP eg);
extern KHE_CONSTRAINT KheEventGroupConstraint(KHE_EVENT_GROUP eg, int i);

extern void KheEventGroupDebug(KHE_EVENT_GROUP eg, int verbosity,
  int indent, FILE *fp);

/* 3.6.2 Events */
extern bool KheEventMake(KHE_INSTANCE ins, char *id, char *name, char *color,
  int duration, int workload, KHE_TIME preassigned_time, KHE_EVENT *e);
extern void KheEventSetBack(KHE_EVENT e, void *back);
extern void *KheEventBack(KHE_EVENT e);

extern KHE_INSTANCE KheEventInstance(KHE_EVENT e);
extern char *KheEventId(KHE_EVENT e);
extern char *KheEventName(KHE_EVENT e);
extern char *KheEventColor(KHE_EVENT e);
extern int KheEventDuration(KHE_EVENT e);
extern int KheEventWorkload(KHE_EVENT e);
extern KHE_TIME KheEventPreassignedTime(KHE_EVENT e);

extern int KheEventIndex(KHE_EVENT e);
extern int KheEventDemand(KHE_EVENT e);

extern int KheEventResourceCount(KHE_EVENT e);
extern KHE_EVENT_RESOURCE KheEventResource(KHE_EVENT e, int i);
extern bool KheEventRetrieveEventResource(KHE_EVENT e, char *role,
  KHE_EVENT_RESOURCE *er);

extern int KheEventResourceGroupCount(KHE_EVENT e);
extern KHE_EVENT_RESOURCE_GROUP KheEventResourceGroup(KHE_EVENT e, int i);

extern KHE_EVENT_GROUP KheEventSingletonEventGroup(KHE_EVENT e);

extern int KheEventConstraintCount(KHE_EVENT e);
extern KHE_CONSTRAINT KheEventConstraint(KHE_EVENT e, int i);

extern bool KheEventSimilar(KHE_EVENT e1, KHE_EVENT e2);
extern bool KheEventMergeable(KHE_EVENT e1, KHE_EVENT e2, int slack);
extern bool KheEventSharePreassignedResource(KHE_EVENT e1, KHE_EVENT e2,
  KHE_RESOURCE *r);

extern void KheEventDebug(KHE_EVENT e, int verbosity, int indent, FILE *fp);

/* 3.6.3 Event resources */
extern bool KheEventResourceMake(KHE_EVENT event, KHE_RESOURCE_TYPE rt,
  KHE_RESOURCE preassigned_resource, char *role, int workload,
  KHE_EVENT_RESOURCE *er);
extern void KheEventResourceSetBack(KHE_EVENT_RESOURCE er, void *back);
extern void *KheEventResourceBack(KHE_EVENT_RESOURCE er);

extern KHE_INSTANCE KheEventResourceInstance(KHE_EVENT_RESOURCE er);
extern int KheEventResourceInstanceIndex(KHE_EVENT_RESOURCE er);
extern KHE_EVENT KheEventResourceEvent(KHE_EVENT_RESOURCE er);
extern int KheEventResourceEventIndex(KHE_EVENT_RESOURCE er);
extern KHE_RESOURCE_TYPE KheEventResourceResourceType(KHE_EVENT_RESOURCE er);
extern KHE_RESOURCE KheEventResourcePreassignedResource(KHE_EVENT_RESOURCE er);
extern char *KheEventResourceRole(KHE_EVENT_RESOURCE er);
extern int KheEventResourceWorkload(KHE_EVENT_RESOURCE er);
extern float KheEventResourceWorkloadPerTime(KHE_EVENT_RESOURCE er);

extern int KheEventResourceConstraintCount(KHE_EVENT_RESOURCE er);
extern KHE_CONSTRAINT KheEventResourceConstraint(KHE_EVENT_RESOURCE er, int i);
extern int KheEventResourceConstraintEventGroupIndex(KHE_EVENT_RESOURCE er,
  int i);

extern KHE_MAYBE_TYPE KheEventResourceNeedsAssignment(KHE_EVENT_RESOURCE er);

extern KHE_RESOURCE_GROUP KheEventResourceHardDomain(KHE_EVENT_RESOURCE er);
extern KHE_RESOURCE_GROUP KheEventResourceHardAndSoftDomain(
  KHE_EVENT_RESOURCE er);

/* *** withdrawn; use multi-tasks
extern bool KheEventResourceEquivalent(KHE_EVENT_RESOURCE er1,
  KHE_EVENT_RESOURCE er2);
*** */

extern void KheEventResourceDebug(KHE_EVENT_RESOURCE er, int verbosity,
  int indent, FILE *fp);

/* 3.6.4 Event resource groups */
extern KHE_EVENT_RESOURCE_GROUP KheEventResourceGroupMake(KHE_EVENT event,
  KHE_RESOURCE_GROUP rg);

extern KHE_EVENT KheEventResourceGroupEvent(KHE_EVENT_RESOURCE_GROUP erg);
extern KHE_RESOURCE_GROUP KheEventResourceGroupResourceGroup(
  KHE_EVENT_RESOURCE_GROUP erg);

extern KHE_EVENT_RESOURCE_GROUP KheEventResourceEventResourceGroup(
  KHE_EVENT_RESOURCE er);

extern void KheEventResourceGroupDebug(KHE_EVENT_RESOURCE_GROUP erg,
  int verbosity, int indent, FILE *fp);

/* 3.7 Constraints */
extern void KheConstraintSetBack(KHE_CONSTRAINT c, void *back);
extern void *KheConstraintBack(KHE_CONSTRAINT c);

extern KHE_INSTANCE KheConstraintInstance(KHE_CONSTRAINT c);
extern char *KheConstraintId(KHE_CONSTRAINT c);
extern char *KheConstraintName(KHE_CONSTRAINT c);
extern bool KheConstraintRequired(KHE_CONSTRAINT c);
extern int KheConstraintWeight(KHE_CONSTRAINT c);
extern KHE_COST KheConstraintCombinedWeight(KHE_CONSTRAINT c);
extern KHE_COST_FUNCTION KheConstraintCostFunction(KHE_CONSTRAINT c);
extern int KheConstraintIndex(KHE_CONSTRAINT c);
extern KHE_CONSTRAINT_TAG KheConstraintTag(KHE_CONSTRAINT c);

extern KHE_COST KheConstraintDevToCost(KHE_CONSTRAINT c, int dev);

extern int KheConstraintAppliesToCount(KHE_CONSTRAINT c);

extern char *KheConstraintTagShow(KHE_CONSTRAINT_TAG tag);
extern char *KheConstraintTagShowSpaced(KHE_CONSTRAINT_TAG tag);
extern KHE_CONSTRAINT_TAG KheStringToConstraintTag(char *str);
extern char *KheCostFunctionShow(KHE_COST_FUNCTION cf);

extern void KheConstraintDebug(KHE_CONSTRAINT c, int verbosity,
  int indent, FILE *fp);

extern KHE_CONSTRAINT KheFromAssignResourceConstraint(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c);
extern KHE_CONSTRAINT KheFromAssignTimeConstraint(
  KHE_ASSIGN_TIME_CONSTRAINT c);
extern KHE_CONSTRAINT KheFromSplitEventsConstraint(
  KHE_SPLIT_EVENTS_CONSTRAINT c);
extern KHE_CONSTRAINT KheFromDistributeSplitEventsConstraint(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c);
extern KHE_CONSTRAINT KheFromPreferResourcesConstraint(
  KHE_PREFER_RESOURCES_CONSTRAINT c);
extern KHE_CONSTRAINT KheFromPreferTimesConstraint(
  KHE_PREFER_TIMES_CONSTRAINT c);
extern KHE_CONSTRAINT KheFromAvoidSplitAssignmentsConstraint(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c);
extern KHE_CONSTRAINT KheFromSpreadEventsConstraint(
  KHE_SPREAD_EVENTS_CONSTRAINT c);
extern KHE_CONSTRAINT KheFromLinkEventsConstraint(
  KHE_LINK_EVENTS_CONSTRAINT c);
extern KHE_CONSTRAINT KheFromOrderEventsConstraint(
  KHE_ORDER_EVENTS_CONSTRAINT c);
extern KHE_CONSTRAINT KheFromAvoidClashesConstraint(
  KHE_AVOID_CLASHES_CONSTRAINT c);
extern KHE_CONSTRAINT KheFromAvoidUnavailableTimesConstraint(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c);
extern KHE_CONSTRAINT KheFromLimitIdleTimesConstraint(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c);
extern KHE_CONSTRAINT KheFromClusterBusyTimesConstraint(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c);
/* ***
extern KHE_CONSTRAINT KheFromLimitEffortConstraint(
  KHE_LIMIT_EFFORT_CONSTRAINT c);
*** */
extern KHE_CONSTRAINT KheFromLimitBusyTimesConstraint(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c);
extern KHE_CONSTRAINT KheFromLimitWorkloadConstraint(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c);
extern KHE_CONSTRAINT KheFromLimitActiveIntervalsConstraint(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c);
extern KHE_CONSTRAINT KheFromLimitResourcesConstraint(
  KHE_LIMIT_RESOURCES_CONSTRAINT c);

extern KHE_ASSIGN_RESOURCE_CONSTRAINT
  KheToAssignResourceConstraint(KHE_CONSTRAINT c);
extern KHE_ASSIGN_TIME_CONSTRAINT
  KheToAssignTimeConstraint(KHE_CONSTRAINT c);
extern KHE_SPLIT_EVENTS_CONSTRAINT
  KheToSplitEventsConstraint(KHE_CONSTRAINT c);
extern KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT
  KheToDistributeSplitEventsConstraint(KHE_CONSTRAINT c);
extern KHE_PREFER_RESOURCES_CONSTRAINT
  KheToPreferResourcesConstraint(KHE_CONSTRAINT c);
extern KHE_PREFER_TIMES_CONSTRAINT
  KheToPreferTimesConstraint(KHE_CONSTRAINT c);
extern KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT
  KheToAvoidSplitAssignmentsConstraint(KHE_CONSTRAINT c);
extern KHE_SPREAD_EVENTS_CONSTRAINT
  KheToSpreadEventsConstraint(KHE_CONSTRAINT c);
extern KHE_LINK_EVENTS_CONSTRAINT
  KheToLinkEventsConstraint(KHE_CONSTRAINT c);
extern KHE_ORDER_EVENTS_CONSTRAINT
  KheToOrderEventsConstraint(KHE_CONSTRAINT c);
extern KHE_AVOID_CLASHES_CONSTRAINT
  KheToAvoidClashesConstraint(KHE_CONSTRAINT c);
extern KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT
  KheToAvoidUnavailableTimesConstraint(KHE_CONSTRAINT c);
extern KHE_LIMIT_IDLE_TIMES_CONSTRAINT
  KheToLimitIdleTimesConstraint(KHE_CONSTRAINT c);
extern KHE_CLUSTER_BUSY_TIMES_CONSTRAINT
  KheToClusterBusyTimesConstraint(KHE_CONSTRAINT c);
/* ***
extern KHE_LIMIT_EFFORT_CONSTRAINT
  KheToLimitEffortConstraint(KHE_CONSTRAINT c);
*** */
extern KHE_LIMIT_BUSY_TIMES_CONSTRAINT
  KheToLimitBusyTimesConstraint(KHE_CONSTRAINT c);
extern KHE_LIMIT_WORKLOAD_CONSTRAINT
  KheToLimitWorkloadConstraint(KHE_CONSTRAINT c);
extern KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT
  KheToLimitActiveIntervalsConstraint(KHE_CONSTRAINT c);
extern KHE_LIMIT_RESOURCES_CONSTRAINT
  KheToLimitResourcesConstraint(KHE_CONSTRAINT c);

/* 3.7.1 Assign resource constraints */
extern bool KheAssignResourceConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  char *role, KHE_ASSIGN_RESOURCE_CONSTRAINT *c);
extern char *KheAssignResourceConstraintRole(KHE_ASSIGN_RESOURCE_CONSTRAINT c);

extern void KheAssignResourceConstraintAddEventResource(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c, KHE_EVENT_RESOURCE er);
extern int KheAssignResourceConstraintEventResourceCount(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c);
extern KHE_EVENT_RESOURCE KheAssignResourceConstraintEventResource(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c, int i);

extern void KheAssignResourceConstraintAddEvent(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c, KHE_EVENT e);
extern int KheAssignResourceConstraintEventCount(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c);
extern KHE_EVENT KheAssignResourceConstraintEvent(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c, int i);

extern void KheAssignResourceConstraintAddEventGroup(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c, KHE_EVENT_GROUP eg);
extern int KheAssignResourceConstraintEventGroupCount(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c);
extern KHE_EVENT_GROUP KheAssignResourceConstraintEventGroup(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c, int i);

extern void KheAssignResourceConstraintDebug(KHE_ASSIGN_RESOURCE_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);

/* 3.7.2 Assign time constraints */
extern bool KheAssignTimeConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_ASSIGN_TIME_CONSTRAINT *c);

extern void KheAssignTimeConstraintAddEvent(KHE_ASSIGN_TIME_CONSTRAINT c,
  KHE_EVENT e);
extern int KheAssignTimeConstraintEventCount(KHE_ASSIGN_TIME_CONSTRAINT c);
extern KHE_EVENT KheAssignTimeConstraintEvent(KHE_ASSIGN_TIME_CONSTRAINT c,
  int i);

extern void KheAssignTimeConstraintAddEventGroup(KHE_ASSIGN_TIME_CONSTRAINT c,
  KHE_EVENT_GROUP eg);
extern int KheAssignTimeConstraintEventGroupCount(KHE_ASSIGN_TIME_CONSTRAINT c);
extern KHE_EVENT_GROUP KheAssignTimeConstraintEventGroup(
  KHE_ASSIGN_TIME_CONSTRAINT c, int i);

extern void KheAssignTimeConstraintDebug(KHE_ASSIGN_TIME_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);

/* 3.7.3 Split events constraints */
extern bool KheSplitEventsConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  int min_duration, int max_duration, int min_amount, int max_amount,
  KHE_SPLIT_EVENTS_CONSTRAINT *c);

extern int KheSplitEventsConstraintMinDuration(KHE_SPLIT_EVENTS_CONSTRAINT c);
extern int KheSplitEventsConstraintMaxDuration(KHE_SPLIT_EVENTS_CONSTRAINT c);
extern int KheSplitEventsConstraintMinAmount(KHE_SPLIT_EVENTS_CONSTRAINT c);
extern int KheSplitEventsConstraintMaxAmount(KHE_SPLIT_EVENTS_CONSTRAINT c);

extern void KheSplitEventsConstraintAddEvent(KHE_SPLIT_EVENTS_CONSTRAINT c,
  KHE_EVENT e);
extern int KheSplitEventsConstraintEventCount(KHE_SPLIT_EVENTS_CONSTRAINT c);
extern KHE_EVENT KheSplitEventsConstraintEvent(KHE_SPLIT_EVENTS_CONSTRAINT c,
  int i);

extern void KheSplitEventsConstraintAddEventGroup(
  KHE_SPLIT_EVENTS_CONSTRAINT c, KHE_EVENT_GROUP eg);
extern int KheSplitEventsConstraintEventGroupCount(
  KHE_SPLIT_EVENTS_CONSTRAINT c);
extern KHE_EVENT_GROUP KheSplitEventsConstraintEventGroup(
  KHE_SPLIT_EVENTS_CONSTRAINT c, int i);

extern void KheSplitEventsConstraintDebug(KHE_SPLIT_EVENTS_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);

/* 3.7.4 Distribute split events constraints */
extern bool KheDistributeSplitEventsConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  int duration, int minimum, int maximum,
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT *c);

extern int KheDistributeSplitEventsConstraintDuration(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c);
extern int KheDistributeSplitEventsConstraintMinimum(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c);
extern int KheDistributeSplitEventsConstraintMaximum(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c);

extern void KheDistributeSplitEventsConstraintAddEvent(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, KHE_EVENT e);
extern int KheDistributeSplitEventsConstraintEventCount(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c);
extern KHE_EVENT KheDistributeSplitEventsConstraintEvent(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, int i);

extern void KheDistributeSplitEventsConstraintAddEventGroup(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, KHE_EVENT_GROUP eg);
extern int KheDistributeSplitEventsConstraintEventGroupCount(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c);
extern KHE_EVENT_GROUP KheDistributeSplitEventsConstraintEventGroup(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, int i);

extern void KheDistributeSplitEventsConstraintDebug(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);

/* 3.7.5 Prefer resources constraints */
extern bool KhePreferResourcesConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  char *role, KHE_PREFER_RESOURCES_CONSTRAINT *c);
extern char *KhePreferResourcesConstraintRole(
  KHE_PREFER_RESOURCES_CONSTRAINT c);

extern bool KhePreferResourcesConstraintAddResourceGroup(
  KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_RESOURCE_GROUP rg);
extern int KhePreferResourcesConstraintResourceGroupCount(
  KHE_PREFER_RESOURCES_CONSTRAINT c);
extern KHE_RESOURCE_GROUP KhePreferResourcesConstraintResourceGroup(
  KHE_PREFER_RESOURCES_CONSTRAINT c, int i);

extern bool KhePreferResourcesConstraintAddResource(
  KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_RESOURCE r);
extern int KhePreferResourcesConstraintResourceCount(
  KHE_PREFER_RESOURCES_CONSTRAINT c);
extern KHE_RESOURCE KhePreferResourcesConstraintResource(
  KHE_PREFER_RESOURCES_CONSTRAINT c, int i);

extern KHE_RESOURCE_GROUP KhePreferResourcesConstraintDomain(
  KHE_PREFER_RESOURCES_CONSTRAINT c);
extern KHE_RESOURCE_GROUP KhePreferResourcesConstraintDomainComplement(
  KHE_PREFER_RESOURCES_CONSTRAINT c);

extern bool KhePreferResourcesConstraintAddEventResource(
  KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_EVENT_RESOURCE er);
extern int KhePreferResourcesConstraintEventResourceCount(
  KHE_PREFER_RESOURCES_CONSTRAINT c);
extern KHE_EVENT_RESOURCE KhePreferResourcesConstraintEventResource(
  KHE_PREFER_RESOURCES_CONSTRAINT c, int i);

extern bool KhePreferResourcesConstraintAddEvent(
  KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_EVENT e);
extern int KhePreferResourcesConstraintEventCount(
  KHE_PREFER_RESOURCES_CONSTRAINT c);
extern KHE_EVENT KhePreferResourcesConstraintEvent(
  KHE_PREFER_RESOURCES_CONSTRAINT c, int i);

extern bool KhePreferResourcesConstraintAddEventGroup(
  KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_EVENT_GROUP eg,
  KHE_EVENT *problem_event);
extern int KhePreferResourcesConstraintEventGroupCount(
  KHE_PREFER_RESOURCES_CONSTRAINT c);
extern KHE_EVENT_GROUP KhePreferResourcesConstraintEventGroup(
  KHE_PREFER_RESOURCES_CONSTRAINT c, int i);

extern void KhePreferResourcesConstraintDebug(KHE_PREFER_RESOURCES_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);

/* 3.7.6 Prefer times constraints */
extern bool KhePreferTimesConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  int duration, KHE_PREFER_TIMES_CONSTRAINT *c);
extern int KhePreferTimesConstraintDuration(KHE_PREFER_TIMES_CONSTRAINT c);

extern void KhePreferTimesConstraintAddTimeGroup(
  KHE_PREFER_TIMES_CONSTRAINT c, KHE_TIME_GROUP tg);
extern int KhePreferTimesConstraintTimeGroupCount(
  KHE_PREFER_TIMES_CONSTRAINT c);
extern KHE_TIME_GROUP KhePreferTimesConstraintTimeGroup(
  KHE_PREFER_TIMES_CONSTRAINT c, int i);

extern void KhePreferTimesConstraintAddTime(
  KHE_PREFER_TIMES_CONSTRAINT c, KHE_TIME t);
extern int KhePreferTimesConstraintTimeCount(
  KHE_PREFER_TIMES_CONSTRAINT c);
extern KHE_TIME KhePreferTimesConstraintTime(
  KHE_PREFER_TIMES_CONSTRAINT c, int i);

extern KHE_TIME_GROUP KhePreferTimesConstraintDomain(
  KHE_PREFER_TIMES_CONSTRAINT c);

extern void KhePreferTimesConstraintAddEvent(
  KHE_PREFER_TIMES_CONSTRAINT c, KHE_EVENT e);
extern int KhePreferTimesConstraintEventCount(
  KHE_PREFER_TIMES_CONSTRAINT c);
extern KHE_EVENT KhePreferTimesConstraintEvent(
  KHE_PREFER_TIMES_CONSTRAINT c, int i);

extern void KhePreferTimesConstraintAddEventGroup(
  KHE_PREFER_TIMES_CONSTRAINT c, KHE_EVENT_GROUP eg);
extern int KhePreferTimesConstraintEventGroupCount(
  KHE_PREFER_TIMES_CONSTRAINT c);
extern KHE_EVENT_GROUP KhePreferTimesConstraintEventGroup(
  KHE_PREFER_TIMES_CONSTRAINT c, int i);

extern void KhePreferTimesConstraintDebug(KHE_PREFER_TIMES_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);

/* 3.7.7 Avoid split assignments constraints */
extern bool KheAvoidSplitAssignmentsConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  char *role, KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT *c);
extern char *KheAvoidSplitAssignmentsConstraintRole(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c);

extern bool KheAvoidSplitAssignmentsConstraintAddEventGroup(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, KHE_EVENT_GROUP eg,
  KHE_EVENT *problem_event);
extern int KheAvoidSplitAssignmentsConstraintEventGroupCount(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c);
extern KHE_EVENT_GROUP KheAvoidSplitAssignmentsConstraintEventGroup(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, int i);

extern void KheAvoidSplitAssignmentsConstraintAddEventResource(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, int eg_index,
  KHE_EVENT_RESOURCE er);
extern int KheAvoidSplitAssignmentsConstraintEventResourceCount(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, int eg_index);
extern KHE_EVENT_RESOURCE KheAvoidSplitAssignmentsConstraintEventResource(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, int eg_index, int er_index);

extern void KheAvoidSplitAssignmentsConstraintDebug(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);

/* 3.7.8 Spread events constraints */
extern bool KheSpreadEventsConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_TIME_SPREAD ts, KHE_SPREAD_EVENTS_CONSTRAINT *c);
extern KHE_TIME_SPREAD KheSpreadEventsConstraintTimeSpread(
  KHE_SPREAD_EVENTS_CONSTRAINT c);

extern KHE_TIME_SPREAD KheTimeSpreadMake(KHE_INSTANCE ins);
extern void KheTimeSpreadAddLimitedTimeGroup(KHE_TIME_SPREAD ts,
  KHE_LIMITED_TIME_GROUP ltg);
extern int KheTimeSpreadLimitedTimeGroupCount(KHE_TIME_SPREAD ts);
extern KHE_LIMITED_TIME_GROUP KheTimeSpreadLimitedTimeGroup(KHE_TIME_SPREAD ts,
  int i);

extern KHE_LIMITED_TIME_GROUP KheLimitedTimeGroupMake(KHE_TIME_GROUP tg,
  int minimum, int maximum);
extern KHE_TIME_GROUP KheLimitedTimeGroupTimeGroup(KHE_LIMITED_TIME_GROUP ltg);
extern int KheLimitedTimeGroupMinimum(KHE_LIMITED_TIME_GROUP ltg);
extern int KheLimitedTimeGroupMaximum(KHE_LIMITED_TIME_GROUP ltg);

extern bool KheTimeSpreadTimeGroupsDisjoint(KHE_TIME_SPREAD ts);
extern bool KheTimeSpreadCoversWholeCycle(KHE_TIME_SPREAD ts);

extern void KheSpreadEventsConstraintAddEventGroup(
  KHE_SPREAD_EVENTS_CONSTRAINT c, KHE_EVENT_GROUP eg);
extern int KheSpreadEventsConstraintEventGroupCount(
  KHE_SPREAD_EVENTS_CONSTRAINT c);
extern KHE_EVENT_GROUP KheSpreadEventsConstraintEventGroup(
  KHE_SPREAD_EVENTS_CONSTRAINT c, int i);

extern void KheSpreadEventsConstraintDebug(KHE_SPREAD_EVENTS_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);

/* 3.7.9 Link events constraints */
extern bool KheLinkEventsConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_LINK_EVENTS_CONSTRAINT *c);

extern void KheLinkEventsConstraintAddEventGroup(KHE_LINK_EVENTS_CONSTRAINT c,
  KHE_EVENT_GROUP eg);
extern int KheLinkEventsConstraintEventGroupCount(KHE_LINK_EVENTS_CONSTRAINT c);
extern KHE_EVENT_GROUP KheLinkEventsConstraintEventGroup(
  KHE_LINK_EVENTS_CONSTRAINT c, int i);

extern void KheLinkEventsConstraintDebug(KHE_LINK_EVENTS_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);

/* 3.7.10 Order events constraints */
extern bool KheOrderEventsConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_ORDER_EVENTS_CONSTRAINT *c);

extern void KheOrderEventsConstraintAddEventPair(KHE_ORDER_EVENTS_CONSTRAINT c,
  KHE_EVENT first_event, KHE_EVENT second_event, int min_separation,
  int max_separation);

extern int KheOrderEventsConstraintEventPairCount(
  KHE_ORDER_EVENTS_CONSTRAINT c);
extern KHE_EVENT KheOrderEventsConstraintFirstEvent(
  KHE_ORDER_EVENTS_CONSTRAINT c, int i);
extern KHE_EVENT KheOrderEventsConstraintSecondEvent(
  KHE_ORDER_EVENTS_CONSTRAINT c, int i);
extern int KheOrderEventsConstraintMinSeparation(
  KHE_ORDER_EVENTS_CONSTRAINT c, int i);
extern int KheOrderEventsConstraintMaxSeparation(
  KHE_ORDER_EVENTS_CONSTRAINT c, int i);

extern void KheOrderEventsConstraintDebug(KHE_ORDER_EVENTS_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);

/* 3.7.11 Avoid clashes constraints */
extern bool KheAvoidClashesConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_AVOID_CLASHES_CONSTRAINT *c);

extern void KheAvoidClashesConstraintAddResourceGroup(
  KHE_AVOID_CLASHES_CONSTRAINT c, KHE_RESOURCE_GROUP rg);
extern int KheAvoidClashesConstraintResourceGroupCount(
  KHE_AVOID_CLASHES_CONSTRAINT c);
extern KHE_RESOURCE_GROUP KheAvoidClashesConstraintResourceGroup(
  KHE_AVOID_CLASHES_CONSTRAINT c, int i);

extern void KheAvoidClashesConstraintAddResource(
  KHE_AVOID_CLASHES_CONSTRAINT c, KHE_RESOURCE r);
extern int KheAvoidClashesConstraintResourceCount(
  KHE_AVOID_CLASHES_CONSTRAINT c);
extern KHE_RESOURCE KheAvoidClashesConstraintResource(
  KHE_AVOID_CLASHES_CONSTRAINT c, int i);

extern int KheAvoidClashesConstraintResourceOfTypeCount(
  KHE_AVOID_CLASHES_CONSTRAINT c, KHE_RESOURCE_TYPE rt);

extern void KheAvoidClashesConstraintDebug(KHE_AVOID_CLASHES_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);

/* 3.7.12 Avoid unavailable times constraints */
extern bool KheAvoidUnavailableTimesConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT *c);

extern void KheAvoidUnavailableTimesConstraintAddResourceGroup(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, KHE_RESOURCE_GROUP rg);
extern int KheAvoidUnavailableTimesConstraintResourceGroupCount(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c);
extern KHE_RESOURCE_GROUP KheAvoidUnavailableTimesConstraintResourceGroup(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, int i);

extern void KheAvoidUnavailableTimesConstraintAddResource(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, KHE_RESOURCE r);
extern int KheAvoidUnavailableTimesConstraintResourceCount(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c);
extern KHE_RESOURCE KheAvoidUnavailableTimesConstraintResource(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, int i);

extern void KheAvoidUnavailableTimesConstraintAddTimeGroup(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, KHE_TIME_GROUP tg);
extern int KheAvoidUnavailableTimesConstraintTimeGroupCount(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c);
extern KHE_TIME_GROUP KheAvoidUnavailableTimesConstraintTimeGroup(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, int i);

extern void KheAvoidUnavailableTimesConstraintAddTime(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, KHE_TIME t);
extern int KheAvoidUnavailableTimesConstraintTimeCount(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c);
extern KHE_TIME KheAvoidUnavailableTimesConstraintTime(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, int i);

extern KHE_TIME_GROUP KheAvoidUnavailableTimesConstraintUnavailableTimes(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c);
extern KHE_TIME_GROUP KheAvoidUnavailableTimesConstraintAvailableTimes(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c);

extern void KheAvoidUnavailableTimesConstraintDebug(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);

/* 3.7.13 Limit idle times constraints */
extern bool KheLimitIdleTimesConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  int minimum, int maximum, KHE_LIMIT_IDLE_TIMES_CONSTRAINT *c);
extern int KheLimitIdleTimesConstraintMinimum(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c);
extern int KheLimitIdleTimesConstraintMaximum(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c);

extern void KheLimitIdleTimesConstraintAddTimeGroup(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, KHE_TIME_GROUP tg);
extern int KheLimitIdleTimesConstraintTimeGroupCount(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c);
extern KHE_TIME_GROUP KheLimitIdleTimesConstraintTimeGroup(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, int i);

extern bool KheLimitIdleTimesConstraintTimeGroupsDisjoint(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c);
extern bool KheLimitIdleTimesConstraintTimeGroupsCoverWholeCycle(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c);

extern void KheLimitIdleTimesConstraintAddResourceGroup(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, KHE_RESOURCE_GROUP rg);
extern int KheLimitIdleTimesConstraintResourceGroupCount(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c);
extern KHE_RESOURCE_GROUP KheLimitIdleTimesConstraintResourceGroup(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, int i);

extern void KheLimitIdleTimesConstraintAddResource(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, KHE_RESOURCE r);
extern int KheLimitIdleTimesConstraintResourceCount(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c);
extern KHE_RESOURCE KheLimitIdleTimesConstraintResource(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, int i);

extern void KheLimitIdleTimesConstraintDebug(KHE_LIMIT_IDLE_TIMES_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);

/* 3.7.14 Cluster busy times constraints */
extern bool KheClusterBusyTimesConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_TIME_GROUP applies_to_tg, int minimum, int maximum,
  bool allow_zero, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT *c);
extern KHE_TIME_GROUP KheClusterBusyTimesConstraintAppliesToTimeGroup(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c);
extern int KheClusterBusyTimesConstraintMinimum(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c);
extern int KheClusterBusyTimesConstraintMaximum(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c);
extern bool KheClusterBusyTimesConstraintAllowZero(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c);
extern bool KheClusterBusyTimesConstraintLimitBusyRecode(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c);

extern int KheClusterBusyTimesConstraintAppliesToOffsetCount(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c);
extern int KheClusterBusyTimesConstraintAppliesToOffset(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int i);

extern void KheClusterBusyTimesConstraintAddTimeGroup(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_TIME_GROUP tg, KHE_POLARITY po);
extern int KheClusterBusyTimesConstraintTimeGroupCount(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c);
extern KHE_TIME_GROUP KheClusterBusyTimesConstraintTimeGroup(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int i, int offset, KHE_POLARITY *po);

extern bool KheClusterBusyTimesConstraintAllPositive(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c);
extern bool KheClusterBusyTimesConstraintAllNegative(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c);
extern bool KheClusterBusyTimesConstraintTimeGroupsDisjoint(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c);
extern bool KheClusterBusyTimesConstraintTimeGroupsCoverWholeCycle(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c);
extern bool KheClusterBusyTimesConstraintRange(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int offset,
  KHE_TIME *first_time, KHE_TIME *last_time);

extern void KheClusterBusyTimesConstraintAddResourceGroup(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE_GROUP rg);
extern int KheClusterBusyTimesConstraintResourceGroupCount(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c);
extern KHE_RESOURCE_GROUP KheClusterBusyTimesConstraintResourceGroup(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int i);

extern void KheClusterBusyTimesConstraintAddResource(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r);
extern int KheClusterBusyTimesConstraintResourceCount(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c);
extern KHE_RESOURCE KheClusterBusyTimesConstraintResource(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int i);

extern int KheClusterBusyTimesConstraintResourceOfTypeCount(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE_TYPE rt);

extern void KheClusterBusyTimesConstraintAddHistoryBefore(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int val);
extern int KheClusterBusyTimesConstraintHistoryBefore(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c);

extern void KheClusterBusyTimesConstraintAddHistoryAfter(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int val);
extern int KheClusterBusyTimesConstraintHistoryAfter(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c);

extern void KheClusterBusyTimesConstraintAddHistory(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r, int val);
extern int KheClusterBusyTimesConstraintHistory(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r);

extern void KheClusterBusyTimesConstraintDebug(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);

/* 3.7.15 Limit effort constraints */
/* *** postponed, possibly cancelled
extern bool KheLimitEffortConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  bool is_workload, KHE_TIME_GROUP applies_to_tg, int minimum,
  int maximum, bool allow_zero, KHE_LIMIT_EFFORT_CONSTRAINT *c);
extern bool KheLimitEffortConstraintIsWorkload(KHE_LIMIT_EFFORT_CONSTRAINT c);
extern KHE_TIME_GROUP KheLimitEffortConstraintAppliesToTimeGroup(
  KHE_LIMIT_EFFORT_CONSTRAINT c);
extern int KheLimitEffortConstraintMinimum(
  KHE_LIMIT_EFFORT_CONSTRAINT c);
extern int KheLimitEffortConstraintMaximum(
  KHE_LIMIT_EFFORT_CONSTRAINT c);
extern bool KheLimitEffortConstraintAllowZero(
  KHE_LIMIT_EFFORT_CONSTRAINT c);

extern int KheLimitEffortConstraintAppliesToOffsetCount(
  KHE_LIMIT_EFFORT_CONSTRAINT c);
extern int KheLimitEffortConstraintAppliesToOffset(
  KHE_LIMIT_EFFORT_CONSTRAINT c, int i);

extern void KheLimitEffortConstraintAddTimeGroup(
  KHE_LIMIT_EFFORT_CONSTRAINT c, KHE_TIME_GROUP tg);
extern int KheLimitEffortConstraintTimeGroupCount(
  KHE_LIMIT_EFFORT_CONSTRAINT c);
extern KHE_TIME_GROUP KheLimitEffortConstraintTimeGroup(
  KHE_LIMIT_EFFORT_CONSTRAINT c, int i, int offset);

extern KHE_TIME_GROUP KheLimitEffortConstraintDomain(
  KHE_LIMIT_EFFORT_CONSTRAINT c);
extern bool KheLimitEffortConstraintLimitsWholeCycle(
  KHE_LIMIT_EFFORT_CONSTRAINT c);

extern void KheLimitEffortConstraintAddResourceGroup(
  KHE_LIMIT_EFFORT_CONSTRAINT c, KHE_RESOURCE_GROUP rg);
extern int KheLimitEffortConstraintResourceGroupCount(
  KHE_LIMIT_EFFORT_CONSTRAINT c);
extern KHE_RESOURCE_GROUP KheLimitEffortConstraintResourceGroup(
  KHE_LIMIT_EFFORT_CONSTRAINT c, int i);

extern void KheLimitEffortConstraintAddResource(
  KHE_LIMIT_EFFORT_CONSTRAINT c, KHE_RESOURCE r);
extern int KheLimitEffortConstraintResourceCount(
  KHE_LIMIT_EFFORT_CONSTRAINT c);
extern KHE_RESOURCE KheLimitEffortConstraintResource(
  KHE_LIMIT_EFFORT_CONSTRAINT c, int i);

extern int KheLimitEffortConstraintResourceOfTypeCount(
  KHE_LIMIT_EFFORT_CONSTRAINT c, KHE_RESOURCE_TYPE rt);

extern void KheLimitEffortConstraintDebug(KHE_LIMIT_EFFORT_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);
*** */

/* 3.7.15 Limit busy times constraints */
extern bool KheLimitBusyTimesConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_TIME_GROUP applies_to_tg, int minimum, int maximum, bool allow_zero,
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT *c);
extern KHE_TIME_GROUP KheLimitBusyTimesConstraintAppliesToTimeGroup(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c);
extern int KheLimitBusyTimesConstraintMinimum(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c);
extern int KheLimitBusyTimesConstraintMaximum(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c);
extern bool KheLimitBusyTimesConstraintAllowZero(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c);

extern int KheLimitBusyTimesConstraintAppliesToOffsetCount(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c);
extern int KheLimitBusyTimesConstraintAppliesToOffset(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, int i);

extern void KheLimitBusyTimesConstraintAddTimeGroup(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_TIME_GROUP tg);
extern int KheLimitBusyTimesConstraintTimeGroupCount(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c);
extern KHE_TIME_GROUP KheLimitBusyTimesConstraintTimeGroup(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, int i, int offset);

extern KHE_TIME_GROUP KheLimitBusyTimesConstraintDomain(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c);
extern bool KheLimitBusyTimesConstraintLimitsWholeCycle(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c);

extern void KheLimitBusyTimesConstraintAddResourceGroup(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE_GROUP rg);
extern int KheLimitBusyTimesConstraintResourceGroupCount(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c);
extern KHE_RESOURCE_GROUP KheLimitBusyTimesConstraintResourceGroup(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, int i);

extern void KheLimitBusyTimesConstraintAddResource(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r);
extern int KheLimitBusyTimesConstraintResourceCount(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c);
extern KHE_RESOURCE KheLimitBusyTimesConstraintResource(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, int i);

extern int KheLimitBusyTimesConstraintResourceOfTypeCount(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE_TYPE rt);

extern void KheLimitBusyTimesConstraintDebug(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);

/* 3.7.16 Limit workload constraints */
extern bool KheLimitWorkloadConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_TIME_GROUP applies_to_tg, int minimum, int maximum,
  bool allow_zero, KHE_LIMIT_WORKLOAD_CONSTRAINT *c);
extern KHE_TIME_GROUP KheLimitWorkloadConstraintAppliesToTimeGroup(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c);
extern int KheLimitWorkloadConstraintMinimum(KHE_LIMIT_WORKLOAD_CONSTRAINT c);
extern int KheLimitWorkloadConstraintMaximum(KHE_LIMIT_WORKLOAD_CONSTRAINT c);
extern bool KheLimitWorkloadConstraintAllowZero(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c);

extern int KheLimitWorkloadConstraintAppliesToOffsetCount(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c);
extern int KheLimitWorkloadConstraintAppliesToOffset(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c, int i);

extern void KheLimitWorkloadConstraintAddTimeGroup(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c, KHE_TIME_GROUP tg);
extern int KheLimitWorkloadConstraintTimeGroupCount(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c);
extern KHE_TIME_GROUP KheLimitWorkloadConstraintTimeGroup(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c, int i, int offset);

extern KHE_TIME_GROUP KheLimitWorkloadConstraintDomain(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c);
extern bool KheLimitWorkloadConstraintLimitsWholeCycle(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c);

extern void KheLimitWorkloadConstraintAddResourceGroup(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c, KHE_RESOURCE_GROUP rg);
extern int KheLimitWorkloadConstraintResourceGroupCount(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c);
extern KHE_RESOURCE_GROUP KheLimitWorkloadConstraintResourceGroup(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c, int i);

extern void KheLimitWorkloadConstraintAddResource(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c, KHE_RESOURCE r);
extern int KheLimitWorkloadConstraintResourceCount(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c);
extern KHE_RESOURCE KheLimitWorkloadConstraintResource(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c, int i);

extern void KheLimitWorkloadConstraintDebug(KHE_LIMIT_WORKLOAD_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);

/* 3.7.17 Limit active intervals constraints */
extern bool KheLimitActiveIntervalsConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_TIME_GROUP applies_to_tg, int minimum, int maximum,
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT *c);
extern KHE_TIME_GROUP KheLimitActiveIntervalsConstraintAppliesToTimeGroup(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c);
extern int KheLimitActiveIntervalsConstraintMinimum(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c);
extern int KheLimitActiveIntervalsConstraintMaximum(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c);

extern int KheLimitActiveIntervalsConstraintAppliesToOffsetCount(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c);
extern int KheLimitActiveIntervalsConstraintAppliesToOffset(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, int i);

extern void KheLimitActiveIntervalsConstraintAddTimeGroup(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_TIME_GROUP tg, KHE_POLARITY po);
extern int KheLimitActiveIntervalsConstraintTimeGroupCount(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c);
extern KHE_TIME_GROUP KheLimitActiveIntervalsConstraintTimeGroup(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, int i, int offset, KHE_POLARITY *po);

extern bool KheLimitActiveIntervalsConstraintAllPositive(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c);
extern bool KheLimitActiveIntervalsConstraintAllNegative(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c);

extern void KheLimitActiveIntervalsConstraintAddResourceGroup(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE_GROUP rg);
extern int KheLimitActiveIntervalsConstraintResourceGroupCount(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c);
extern KHE_RESOURCE_GROUP KheLimitActiveIntervalsConstraintResourceGroup(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, int i);

extern void KheLimitActiveIntervalsConstraintAddResource(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE r);
extern int KheLimitActiveIntervalsConstraintResourceCount(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c);
extern KHE_RESOURCE KheLimitActiveIntervalsConstraintResource(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, int i);

extern int KheLimitActiveIntervalsConstraintResourceOfTypeCount(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE_TYPE rt);

extern void KheLimitActiveIntervalsConstraintAddHistoryBefore(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, int val);
extern int KheLimitActiveIntervalsConstraintHistoryBefore(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c);

extern void KheLimitActiveIntervalsConstraintAddHistoryAfter(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, int val);
extern int KheLimitActiveIntervalsConstraintHistoryAfter(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c);

extern void KheLimitActiveIntervalsConstraintAddHistory(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE r, int val);
extern int KheLimitActiveIntervalsConstraintHistory(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE r);

extern void KheLimitActiveIntervalsConstraintDebug(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);

/* 3.7.18 Limit resources constraints */
extern bool KheLimitResourcesConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  int minimum, int maximum, KHE_LIMIT_RESOURCES_CONSTRAINT *c);
extern int KheLimitResourcesConstraintMinimum(KHE_LIMIT_RESOURCES_CONSTRAINT c);
extern int KheLimitResourcesConstraintMaximum(KHE_LIMIT_RESOURCES_CONSTRAINT c);

extern bool KheLimitResourcesConstraintAddResourceGroup(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, KHE_RESOURCE_GROUP rg);
extern int KheLimitResourcesConstraintResourceGroupCount(
  KHE_LIMIT_RESOURCES_CONSTRAINT c);
extern KHE_RESOURCE_GROUP KheLimitResourcesConstraintResourceGroup(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, int i);

extern bool KheLimitResourcesConstraintAddResource(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, KHE_RESOURCE r);
extern int KheLimitResourcesConstraintResourceCount(
  KHE_LIMIT_RESOURCES_CONSTRAINT c);
extern KHE_RESOURCE KheLimitResourcesConstraintResource(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, int i);

extern KHE_RESOURCE_GROUP KheLimitResourcesConstraintDomain(
  KHE_LIMIT_RESOURCES_CONSTRAINT c);
extern KHE_RESOURCE_GROUP KheLimitResourcesConstraintDomainComplement(
  KHE_LIMIT_RESOURCES_CONSTRAINT c);

extern void KheLimitResourcesConstraintAddEventGroup(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, KHE_EVENT_GROUP eg);
extern int KheLimitResourcesConstraintEventGroupCount(
  KHE_LIMIT_RESOURCES_CONSTRAINT c);
extern KHE_EVENT_GROUP KheLimitResourcesConstraintEventGroup(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, int i);

extern void KheLimitResourcesConstraintAddRole(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, char *role);
extern int KheLimitResourcesConstraintRoleCount(
  KHE_LIMIT_RESOURCES_CONSTRAINT c);
extern char *KheLimitResourcesConstraintRole(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, int i);

extern void KheLimitResourcesConstraintAddEventResource(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, int eg_index, KHE_EVENT_RESOURCE er);
extern int KheLimitResourcesConstraintEventResourceCount(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, int eg_index);
extern KHE_EVENT_RESOURCE KheLimitResourcesConstraintEventResource(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, int eg_index, int er_index);

extern void KheLimitResourcesConstraintDebug(KHE_LIMIT_RESOURCES_CONSTRAINT c,
  int verbosity, int indent, FILE *fp);


/*****************************************************************************/
/*                                                                           */
/*    Chapter 4.   Solutions                                                 */
/*                                                                           */
/*****************************************************************************/

/* 4.2.1 Creation, deletion, and copy */
extern KHE_SOLN KheSolnMake(KHE_INSTANCE ins, HA_ARENA_SET as);
extern void KheSolnDelete(KHE_SOLN soln);
extern KHE_SOLN KheSolnCopy(KHE_SOLN soln, HA_ARENA_SET as);
extern void KheSolnDebug(KHE_SOLN soln, int verbosity, int indent, FILE *fp);

/* 4.2.2 Solutions and arenas */
extern HA_ARENA KheSolnArenaBegin(KHE_SOLN soln);
extern void KheSolnArenaEnd(KHE_SOLN soln, HA_ARENA a);
/* extern void KheSolnSetArenaSet(KHE_SOLN soln, HA_ARENA_SET as); */
extern HA_ARENA_SET KheSolnArenaSet(KHE_SOLN soln);

extern void KheSolnJmpEnvBegin(KHE_SOLN soln, jmp_buf *env);
extern void KheSolnJmpEnvEnd(KHE_SOLN soln);

/* 4.2.3 Simple attributes */
extern int KheSolnSolnGroupCount(KHE_SOLN soln);
extern KHE_SOLN_GROUP KheSolnSolnGroup(KHE_SOLN soln, int i);

extern KHE_INSTANCE KheSolnInstance(KHE_SOLN soln);

extern void KheSolnSetDescription(KHE_SOLN soln, char *description);
extern char *KheSolnDescription(KHE_SOLN soln);

extern void KheSolnSetRunningTime(KHE_SOLN soln, float running_time);
extern bool KheSolnHasRunningTime(KHE_SOLN soln, float *running_time);
extern int KheSolnIncreasingRunningTimeTypedCmp(KHE_SOLN soln1, KHE_SOLN soln2);
extern int KheSolnIncreasingRunningTimeCmp(const void *t1, const void *t2);

extern void KheSolnSetBack(KHE_SOLN soln, void *back);
extern void *KheSolnBack(KHE_SOLN soln);

/* 4.2.4 Diversification */
extern void KheSolnSetDiversifier(KHE_SOLN soln, int val);
extern int KheSolnDiversifier(KHE_SOLN soln);
extern int KheSolnDiversifierChoose(KHE_SOLN soln, int c);

/* 4.2.5 Visit numbers */
extern void KheSolnSetGlobalVisitNum(KHE_SOLN soln, int num);
extern int KheSolnGlobalVisitNum(KHE_SOLN soln);
extern void KheSolnNewGlobalVisit(KHE_SOLN soln);

/* 4.2.6 Placeholder and invalid solutions */
extern KHE_SOLN_TYPE KheSolnType(KHE_SOLN soln);
extern KML_ERROR KheSolnInvalidError(KHE_SOLN soln);
extern void KheSolnTypeReduce(KHE_SOLN soln, KHE_SOLN_TYPE soln_type,
  KML_ERROR ke);

/* ***
extern void KheSolnReduceToPlaceholder(KHE_SOLN soln, bool writable);
extern bool KheSolnIsPlaceholder(KHE_SOLN soln);
extern bool KheSolnIsInvalid(KHE_SOLN soln);
extern KML_ERROR KheSolnInvalidError(KHE_SOLN soln);
extern void KheSolnReduceToInvalid(KHE_SOLN soln, KML_ERROR ke);
extern KHE_SOLN_TYPE KheSolnType(KHE_SOLN soln);
extern void KheSolnReduceToType(KHE_SOLN soln, KHE_SOLN_TYPE soln_type);
*** */

/* 4.2.7 Traversing the parts of solutions */
extern int KheSolnMeetCount(KHE_SOLN soln);
extern KHE_MEET KheSolnMeet(KHE_SOLN soln, int i);
extern int KheEventMeetCount(KHE_SOLN soln, KHE_EVENT e);
extern KHE_MEET KheEventMeet(KHE_SOLN soln, KHE_EVENT e, int i);

extern int KheSolnTaskCount(KHE_SOLN soln);
extern KHE_TASK KheSolnTask(KHE_SOLN soln, int i);
extern int KheEventResourceTaskCount(KHE_SOLN soln, KHE_EVENT_RESOURCE er);
extern KHE_TASK KheEventResourceTask(KHE_SOLN soln, KHE_EVENT_RESOURCE er,
  int i);

extern int KheSolnNodeCount(KHE_SOLN soln);
extern KHE_NODE KheSolnNode(KHE_SOLN soln, int i);

/* ***
extern int KheSolnTaskingCount(KHE_SOLN soln);
extern KHE_TASKING KheSolnTasking(KHE_SOLN soln, int i);
*** */

/* 4.3 Complete representation and preassignment conversion */
extern bool KheSolnMakeCompleteRepresentation(KHE_SOLN soln,
  KHE_EVENT *problem_event);
extern void KheSolnAssignPreassignedTimes(KHE_SOLN soln);
extern void KheSolnAssignPreassignedResources(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt);

/* 4.4 Solution time, resource, and event groups */
extern void KheSolnTimeGroupBegin(KHE_SOLN soln);
extern void KheSolnTimeGroupAddTime(KHE_SOLN soln, KHE_TIME t);
extern void KheSolnTimeGroupSubTime(KHE_SOLN soln, KHE_TIME t);
extern void KheSolnTimeGroupUnion(KHE_SOLN soln, KHE_TIME_GROUP tg2);
extern void KheSolnTimeGroupIntersect(KHE_SOLN soln, KHE_TIME_GROUP tg2);
extern void KheSolnTimeGroupDifference(KHE_SOLN soln, KHE_TIME_GROUP tg2);
extern KHE_TIME_GROUP KheSolnTimeGroupEnd(KHE_SOLN soln);

extern void KheSolnResourceGroupBegin(KHE_SOLN soln, KHE_RESOURCE_TYPE rt);
extern void KheSolnResourceGroupAddResource(KHE_SOLN soln, KHE_RESOURCE r);
extern void KheSolnResourceGroupSubResource(KHE_SOLN soln, KHE_RESOURCE r);
extern void KheSolnResourceGroupUnion(KHE_SOLN soln, KHE_RESOURCE_GROUP rg2);
extern void KheSolnResourceGroupIntersect(KHE_SOLN soln,
  KHE_RESOURCE_GROUP rg2);
extern void KheSolnResourceGroupDifference(KHE_SOLN soln,
  KHE_RESOURCE_GROUP rg2);
extern KHE_RESOURCE_GROUP KheSolnResourceGroupEnd(KHE_SOLN soln);

extern void KheSolnEventGroupBegin(KHE_SOLN soln);
extern void KheSolnEventGroupAddEvent(KHE_SOLN soln, KHE_EVENT e);
extern void KheSolnEventGroupSubEvent(KHE_SOLN soln, KHE_EVENT e);
extern void KheSolnEventGroupUnion(KHE_SOLN soln, KHE_EVENT_GROUP eg2);
extern void KheSolnEventGroupIntersect(KHE_SOLN soln, KHE_EVENT_GROUP eg2);
extern void KheSolnEventGroupDifference(KHE_SOLN soln, KHE_EVENT_GROUP eg2);
extern KHE_EVENT_GROUP KheSolnEventGroupEnd(KHE_SOLN soln);

/* 4.7 Running times and time limits */
/* extern float KheSolnTimeNow(KHE_SOLN soln); */
/* extern void KheSolnSetTimeLimit(KHE_SOLN soln, float limit_in_secs); */
/* extern float KheSolnTimeLimit(KHE_SOLN soln); */
/* extern bool KheSolnTimeLimitReached(KHE_SOLN soln); */

/* 4.5 Meets */
extern void KheMeetCheck(KHE_MEET meet);
extern KHE_MEET KheMeetMake(KHE_SOLN soln, int duration, KHE_EVENT e);
extern void KheMeetDelete(KHE_MEET meet);

extern char *KheMeetId(KHE_MEET meet);

extern void KheMeetSetBack(KHE_MEET meet, void *back);
extern void *KheMeetBack(KHE_MEET meet);

extern void KheMeetSetVisitNum(KHE_MEET meet, int num);
extern int KheMeetVisitNum(KHE_MEET meet);
extern bool KheMeetVisited(KHE_MEET meet, int slack);
extern void KheMeetVisit(KHE_MEET meet);
extern void KheMeetUnVisit(KHE_MEET meet);

extern KHE_SOLN KheMeetSoln(KHE_MEET meet);
extern int KheMeetSolnIndex(KHE_MEET meet);
extern int KheMeetDuration(KHE_MEET meet);
extern KHE_EVENT KheMeetEvent(KHE_MEET meet);
extern bool KheMeetIsPreassigned(KHE_MEET meet, KHE_TIME *time);
extern bool KheMeetIsAssignedPreassigned(KHE_MEET meet, KHE_TIME *time);

extern int KheMeetAssignedDuration(KHE_MEET meet);
extern int KheMeetDemand(KHE_MEET meet);

extern int KheMeetTaskCount(KHE_MEET meet);
extern KHE_TASK KheMeetTask(KHE_MEET meet, int i);
extern bool KheMeetRetrieveTask(KHE_MEET meet, char *role, KHE_TASK *task);
extern bool KheMeetFindTask(KHE_MEET meet, KHE_EVENT_RESOURCE er,
  KHE_TASK *task);

extern bool KheMeetContainsResourcePreassignment(KHE_MEET meet,
  KHE_RESOURCE r, KHE_TASK *task);
extern bool KheMeetContainsResourceAssignment(KHE_MEET meet,
  KHE_RESOURCE r, KHE_TASK *task);

extern KHE_NODE KheMeetNode(KHE_MEET meet);
extern int KheMeetNodeIndex(KHE_MEET meet);

extern void KheMeetDebug(KHE_MEET meet, int verbosity, int indent, FILE *fp);

/* 4.5.1 Splitting and merging */
extern bool KheMeetSplitCheck(KHE_MEET meet, int duration1, bool recursive);
extern bool KheMeetSplit(KHE_MEET meet, int duration1, bool recursive,
  KHE_MEET *meet1, KHE_MEET *meet2);
extern bool KheMeetMergeCheck(KHE_MEET meet1, KHE_MEET meet2);
extern bool KheMeetMerge(KHE_MEET meet1, KHE_MEET meet2, bool recursive,
  KHE_MEET *meet);

/* 4.5.2 Assignment */
extern bool KheMeetMoveCheck(KHE_MEET meet, KHE_MEET target_meet, int offset);
extern bool KheMeetMove(KHE_MEET meet, KHE_MEET target_meet, int offset);

extern bool KheMeetAssignCheck(KHE_MEET meet, KHE_MEET target_meet, int offset);
extern bool KheMeetAssign(KHE_MEET meet, KHE_MEET target_meet, int offset);

extern bool KheMeetUnAssignCheck(KHE_MEET meet);
extern bool KheMeetUnAssign(KHE_MEET meet);

extern bool KheMeetSwapCheck(KHE_MEET meet1, KHE_MEET meet2);
extern bool KheMeetSwap(KHE_MEET meet1, KHE_MEET meet2);

extern bool KheMeetBlockSwapCheck(KHE_MEET meet1, KHE_MEET meet2);
extern bool KheMeetBlockSwap(KHE_MEET meet1, KHE_MEET meet2);

extern KHE_MEET KheMeetAsst(KHE_MEET meet);
extern int KheMeetAsstOffset(KHE_MEET meet);

extern int KheMeetAssignedToCount(KHE_MEET target_meet);
extern KHE_MEET KheMeetAssignedTo(KHE_MEET target_meet, int i);

extern KHE_MEET KheMeetRoot(KHE_MEET meet, int *offset_in_root);
extern bool KheMeetOverlap(KHE_MEET meet1, KHE_MEET meet2);
extern bool KheMeetAdjacent(KHE_MEET meet1, KHE_MEET meet2, bool *swap);

extern void KheMeetAssignFix(KHE_MEET meet);
extern void KheMeetAssignUnFix(KHE_MEET meet);
extern bool KheMeetAssignIsFixed(KHE_MEET meet);

extern bool KheMeetIsMovable(KHE_MEET meet);
extern KHE_MEET KheMeetFirstMovable(KHE_MEET meet, int *offset_in_result);
extern KHE_MEET KheMeetLastFixed(KHE_MEET meet, int *offset_in_result);

/* 4.5.3 Cycle meets and time assignment */
extern bool KheMeetIsCycleMeet(KHE_MEET meet);
extern KHE_MEET KheSolnTimeCycleMeet(KHE_SOLN soln, KHE_TIME t);
extern int KheSolnTimeCycleMeetOffset(KHE_SOLN soln, KHE_TIME t);
extern KHE_TIME_GROUP KheSolnPackingTimeGroup(KHE_SOLN soln, int duration);
extern void KheSolnSplitCycleMeet(KHE_SOLN soln);

extern bool KheMeetMoveTimeCheck(KHE_MEET meet, KHE_TIME t);
extern bool KheMeetMoveTime(KHE_MEET meet, KHE_TIME t);

extern bool KheMeetAssignTimeCheck(KHE_MEET meet, KHE_TIME t);
extern bool KheMeetAssignTime(KHE_MEET meet, KHE_TIME t);
extern bool KheMeetUnAssignTimeCheck(KHE_MEET meet);
extern bool KheMeetUnAssignTime(KHE_MEET meet);
extern KHE_TIME KheMeetAsstTime(KHE_MEET meet);

/* 4.5.4 Meet domains and bounds */
extern KHE_TIME_GROUP KheMeetDomain(KHE_MEET meet);
extern KHE_MEET_BOUND KheMeetBoundMake(KHE_SOLN soln,
  bool occupancy, KHE_TIME_GROUP dft_tg);
extern bool KheMeetBoundDeleteCheck(KHE_MEET_BOUND mb);
extern bool KheMeetBoundDelete(KHE_MEET_BOUND mb);

extern KHE_SOLN KheMeetBoundSoln(KHE_MEET_BOUND mb);
/* extern int KheMeetBoundSolnIndex(KHE_MEET_BOUND mb); */
extern bool KheMeetBoundOccupancy(KHE_MEET_BOUND mb);
extern KHE_TIME_GROUP KheMeetBoundDefaultTimeGroup(KHE_MEET_BOUND mb);

extern void KheMeetBoundAddTimeGroup(KHE_MEET_BOUND mb,
  int duration, KHE_TIME_GROUP tg);
extern KHE_TIME_GROUP KheMeetBoundTimeGroup(KHE_MEET_BOUND mb, int duration);
extern KHE_TIME_GROUP KheSolnStartingTimeGroup(KHE_SOLN soln, int duration,
  KHE_TIME_GROUP tg);

extern void KheMeetBoundDebug(KHE_MEET_BOUND mb, int verbosity,
  int indent, FILE *fp);

extern bool KheMeetAddMeetBoundCheck(KHE_MEET meet, KHE_MEET_BOUND mb);
extern bool KheMeetAddMeetBound(KHE_MEET meet, KHE_MEET_BOUND mb);
extern bool KheMeetDeleteMeetBoundCheck(KHE_MEET meet, KHE_MEET_BOUND mb);
extern bool KheMeetDeleteMeetBound(KHE_MEET meet, KHE_MEET_BOUND mb);

extern int KheMeetMeetBoundCount(KHE_MEET meet);
extern KHE_MEET_BOUND KheMeetMeetBound(KHE_MEET meet, int i);
extern int KheMeetBoundMeetCount(KHE_MEET_BOUND mb);
extern KHE_MEET KheMeetBoundMeet(KHE_MEET_BOUND mb, int i);

/* 4.5.5 Automatic domains */
extern bool KheMeetSetAutoDomainCheck(KHE_MEET meet, bool automatic);
extern bool KheMeetSetAutoDomain(KHE_MEET meet, bool automatic);
extern KHE_TIME_GROUP KheMeetDescendantsDomain(KHE_MEET meet);

/* 4.6 Tasks */
extern KHE_TASK KheTaskMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_MEET meet, KHE_EVENT_RESOURCE er);
extern void KheTaskDelete(KHE_TASK task);

extern char *KheTaskId(KHE_TASK task);

/* extern void KheTaskCheckMagic(KHE_TASK task); */
extern void KheTaskSetBack(KHE_TASK task, void *back);
extern void *KheTaskBack(KHE_TASK task);

extern void KheTaskSetVisitNum(KHE_TASK task, int num);
extern int KheTaskVisitNum(KHE_TASK task);
extern bool KheTaskVisited(KHE_TASK task, int slack);
extern void KheTaskVisit(KHE_TASK task);
extern void KheTaskUnVisit(KHE_TASK task);

extern KHE_MEET KheTaskMeet(KHE_TASK task);
/* extern int KheTaskMeetIndex(KHE_TASK task); */
extern int KheTaskDuration(KHE_TASK task);
extern float KheTaskWorkload(KHE_TASK task);
extern float KheTaskWorkloadPerTime(KHE_TASK task);

extern KHE_SOLN KheTaskSoln(KHE_TASK task);
extern int KheTaskSolnIndex(KHE_TASK task);
extern KHE_RESOURCE_TYPE KheTaskResourceType(KHE_TASK task);
extern KHE_EVENT_RESOURCE KheTaskEventResource(KHE_TASK task);
extern bool KheTaskIsPreassigned(KHE_TASK task, KHE_RESOURCE *r);

/* *** withdrawn; use multi-tasks
extern bool KheTaskEquivalent(KHE_TASK task1, KHE_TASK task2);
*** */

/* ***
extern KHE_TASKING KheTaskTasking(KHE_TASK task);
extern int KheTaskTaskingIndex(KHE_TASK task);
*** */

extern void KheTaskDebug(KHE_TASK task, int verbosity,
  int indent, FILE *fp);

/* 4.6.1 Assignment */
extern bool KheTaskMoveCheck(KHE_TASK task, KHE_TASK target_task);
extern bool KheTaskMove(KHE_TASK task, KHE_TASK target_task);

extern bool KheTaskAssignCheck(KHE_TASK task, KHE_TASK target_task);
extern bool KheTaskAssign(KHE_TASK task, KHE_TASK target_task);

extern bool KheTaskUnAssignCheck(KHE_TASK task);
extern bool KheTaskUnAssign(KHE_TASK task);

extern bool KheTaskSwapCheck(KHE_TASK task1, KHE_TASK task2);
extern bool KheTaskSwap(KHE_TASK task1, KHE_TASK task2);

extern KHE_TASK KheTaskAsst(KHE_TASK task);

extern int KheTaskAssignedToCount(KHE_TASK target_task);
extern KHE_TASK KheTaskAssignedTo(KHE_TASK target_task, int i);
extern int KheTaskTotalDuration(KHE_TASK task);
extern float KheTaskTotalWorkload(KHE_TASK task);

extern KHE_TASK KheTaskRoot(KHE_TASK task);
extern KHE_TASK KheTaskProperRoot(KHE_TASK task);
extern bool KheTaskIsProperRoot(KHE_TASK task);

extern bool KheTaskNeedsAssignment(KHE_TASK task);
/* ***
extern KHE_COST KheTaskAssignmentCostReduction(KHE_TASK task);
extern bool KheTaskAssignmentHasCost(KHE_TASK task);
extern bool KheTaskNonAssignmentHasCost(KHE_TASK task, bool certain);
*** */

extern void KheTaskAssignFix(KHE_TASK task);
extern void KheTaskAssignUnFix(KHE_TASK task);
extern bool KheTaskAssignIsFixed(KHE_TASK task);
extern KHE_TASK KheTaskFirstUnFixed(KHE_TASK task);

/* 4.6.2 Cycle tasks and resource assignment */
extern bool KheTaskIsCycleTask(KHE_TASK task);
extern KHE_TASK KheSolnResourceCycleTask(KHE_SOLN soln, KHE_RESOURCE r);

extern bool KheTaskMoveResourceCheck(KHE_TASK task, KHE_RESOURCE r);
extern bool KheTaskMoveResource(KHE_TASK task, KHE_RESOURCE r);

extern bool KheTaskAssignResourceCheck(KHE_TASK task, KHE_RESOURCE r);
extern bool KheTaskAssignResource(KHE_TASK task, KHE_RESOURCE r);
extern bool KheTaskUnAssignResourceCheck(KHE_TASK task);
extern bool KheTaskUnAssignResource(KHE_TASK task);
extern KHE_RESOURCE KheTaskAsstResource(KHE_TASK task);

extern int KheResourceAssignedTaskCount(KHE_SOLN soln, KHE_RESOURCE r);
extern KHE_TASK KheResourceAssignedTask(KHE_SOLN soln, KHE_RESOURCE r, int i);

/* 4.6.3 Task domains and bounds */
extern KHE_RESOURCE_GROUP KheTaskDomain(KHE_TASK task);
extern KHE_TASK_BOUND KheTaskBoundMake(KHE_SOLN soln, KHE_RESOURCE_GROUP rg);
extern bool KheTaskBoundDeleteCheck(KHE_TASK_BOUND tb);
extern bool KheTaskBoundDelete(KHE_TASK_BOUND tb);

extern KHE_SOLN KheTaskBoundSoln(KHE_TASK_BOUND tb);
/* extern int KheTaskBoundSolnIndex(KHE_TASK_BOUND tb); */
extern KHE_RESOURCE_GROUP KheTaskBoundResourceGroup(KHE_TASK_BOUND tb);

extern bool KheTaskAddTaskBoundCheck(KHE_TASK task, KHE_TASK_BOUND tb);
extern bool KheTaskAddTaskBound(KHE_TASK task, KHE_TASK_BOUND tb);
extern bool KheTaskDeleteTaskBoundCheck(KHE_TASK task, KHE_TASK_BOUND tb);
extern bool KheTaskDeleteTaskBound(KHE_TASK task, KHE_TASK_BOUND tb);

extern int KheTaskTaskBoundCount(KHE_TASK task);
extern KHE_TASK_BOUND KheTaskTaskBound(KHE_TASK task, int i);
extern int KheTaskBoundTaskCount(KHE_TASK_BOUND tb);
extern KHE_TASK KheTaskBoundTask(KHE_TASK_BOUND tb, int i);

/* 4.7.1 Resource availability functions */
extern bool KheResourceMaxBusyTimes(KHE_SOLN soln, KHE_RESOURCE r, int *res);
extern bool KheResourceMaxWorkload(KHE_SOLN soln, KHE_RESOURCE r, float *res);
extern int KheResourceBusyTimes(KHE_SOLN soln, KHE_RESOURCE r);
extern float KheResourceWorkload(KHE_SOLN soln, KHE_RESOURCE r);
extern bool KheResourceAvailableBusyTimes(KHE_SOLN soln, KHE_RESOURCE r,
  int *res);
extern bool KheResourceAvailableWorkload(KHE_SOLN soln, KHE_RESOURCE r,
  float *res);

/* 4.7.3 Detailed querying of resource availability */
extern KHE_AVAIL_SOLVER KheSolnAvailSolver(KHE_SOLN soln);
/* ***
extern void KheAvailSolverSetResource(KHE_AVAIL_SOLVER as, KHE_RESOURCE r);
*** */

extern bool KheAvailSolverMaxBusyTimes(KHE_AVAIL_SOLVER as,
  KHE_RESOURCE r, int *res);
extern bool KheAvailSolverMaxWorkload(KHE_AVAIL_SOLVER as,
  KHE_RESOURCE r, float *res);

extern char *KheAvailNodeTypeShow(KHE_AVAIL_NODE_TYPE type);

extern int KheAvailSolverMaxBusyTimesAvailNodeCount(KHE_AVAIL_SOLVER as,
  KHE_RESOURCE r);
extern void KheAvailSolverMaxBusyTimesAvailNode(KHE_AVAIL_SOLVER as,
  KHE_RESOURCE r, int i, KHE_AVAIL_NODE_TYPE *type, int *limit,
  KHE_TIME_SET *ts, KHE_MONITOR *m);

extern int KheAvailSolverMaxWorkloadAvailNodeCount(KHE_AVAIL_SOLVER as,
  KHE_RESOURCE r);
extern void KheAvailSolverMaxWorkloadAvailNode(KHE_AVAIL_SOLVER as,
  KHE_RESOURCE r, int i, KHE_AVAIL_NODE_TYPE *type, float *limit,
  KHE_TIME_SET *ts, KHE_MONITOR *m);

/* 4.8 Marks and paths */
extern KHE_MARK KheMarkBegin(KHE_SOLN soln);
extern void KheMarkEnd(KHE_MARK mark, bool undo);
extern void KheMarkUndo(KHE_MARK mark);
extern KHE_SOLN KheMarkSoln(KHE_MARK mark);
extern KHE_COST KheMarkSolnCost(KHE_MARK mark);
extern KHE_PATH KheMarkAddPath(KHE_MARK mark);
extern KHE_PATH KheMarkAddBestPath(KHE_MARK mark, int k);
extern int KheMarkPathCount(KHE_MARK mark);
extern KHE_PATH KheMarkPath(KHE_MARK mark, int i);
extern void KheMarkPathSort(KHE_MARK mark,
  int(*compar)(const void *, const void *));

extern int KhePathIncreasingSolnCostCmp(const void *t1, const void *t2);
extern KHE_SOLN KhePathSoln(KHE_PATH path);
extern KHE_COST KhePathSolnCost(KHE_PATH path);
extern KHE_MARK KhePathMark(KHE_PATH path);
extern void KhePathDelete(KHE_PATH path);
extern void KhePathRedo(KHE_PATH path);


/*****************************************************************************/
/*                                                                           */
/*    Chapter 5.   Extra Types for Solving                                   */
/*                                                                           */
/*****************************************************************************/

/* 5.2 Nodes */
extern KHE_NODE KheNodeMake(KHE_SOLN soln);
extern void KheNodeSetBack(KHE_NODE node, void *back);
extern void *KheNodeBack(KHE_NODE node);

extern void KheNodeSetVisitNum(KHE_NODE n, int num);
extern int KheNodeVisitNum(KHE_NODE n);
extern bool KheNodeVisited(KHE_NODE n, int slack);
extern void KheNodeVisit(KHE_NODE n);
extern void KheNodeUnVisit(KHE_NODE n);

extern KHE_SOLN KheNodeSoln(KHE_NODE node);
extern int KheNodeSolnIndex(KHE_NODE node);

extern bool KheNodeDeleteCheck(KHE_NODE node);
extern bool KheNodeDelete(KHE_NODE node);

extern bool KheNodeAddParentCheck(KHE_NODE child_node, KHE_NODE parent_node);
extern bool KheNodeAddParent(KHE_NODE child_node, KHE_NODE parent_node);
extern bool KheNodeDeleteParentCheck(KHE_NODE child_node);
extern bool KheNodeDeleteParent(KHE_NODE child_node);
extern KHE_NODE KheNodeParent(KHE_NODE node);

extern int KheNodeChildCount(KHE_NODE node);
extern KHE_NODE KheNodeChild(KHE_NODE node, int i);

extern bool KheNodeIsDescendant(KHE_NODE node, KHE_NODE ancestor_node);
extern bool KheNodeIsProperDescendant(KHE_NODE node, KHE_NODE ancestor_node);

extern void KheNodeSwapChildNodesAndLayers(KHE_NODE node1, KHE_NODE node2);

extern bool KheNodeAddMeetCheck(KHE_NODE node, KHE_MEET meet);
extern bool KheNodeAddMeet(KHE_NODE node, KHE_MEET meet);
extern bool KheNodeDeleteMeetCheck(KHE_NODE node, KHE_MEET meet);
extern bool KheNodeDeleteMeet(KHE_NODE node, KHE_MEET meet);

extern int KheNodeMeetCount(KHE_NODE node);
extern KHE_MEET KheNodeMeet(KHE_NODE node, int i);
extern void KheNodeMeetSort(KHE_NODE node,
  int(*compar)(const void *, const void *));
extern int KheMeetDecreasingDurationCmp(const void *p1, const void *p2);
extern int KheMeetIncreasingAsstCmp(const void *p1, const void *p2);

extern bool KheNodeIsCycleNode(KHE_NODE node);

extern int KheNodeDuration(KHE_NODE node);
extern int KheNodeAssignedDuration(KHE_NODE node);
extern int KheNodeDemand(KHE_NODE node);

extern bool KheNodeSimilar(KHE_NODE node1, KHE_NODE node2);
extern bool KheNodeRegular(KHE_NODE node1, KHE_NODE node2, int *regular_count);
extern int KheNodeResourceDuration(KHE_NODE node, KHE_RESOURCE r);

void KheNodeDebug(KHE_NODE node, int verbosity, int indent, FILE *fp);
extern void KheNodePrintTimetable(KHE_NODE node, int cell_width,
  int indent, FILE *fp);

/* 5.3 Layers */
extern KHE_LAYER KheLayerMake(KHE_NODE parent_node);
extern void KheLayerSetBack(KHE_LAYER layer, void *back);
extern void *KheLayerBack(KHE_LAYER layer);

extern void KheLayerSetVisitNum(KHE_LAYER layer, int num);
extern int KheLayerVisitNum(KHE_LAYER layer);
extern bool KheLayerVisited(KHE_LAYER layer, int slack);
extern void KheLayerVisit(KHE_LAYER layer);
extern void KheLayerUnVisit(KHE_LAYER layer);

extern KHE_NODE KheLayerParentNode(KHE_LAYER layer);
extern int KheLayerParentNodeIndex(KHE_LAYER layer);
extern KHE_SOLN KheLayerSoln(KHE_LAYER layer);
extern void KheLayerDelete(KHE_LAYER layer);

extern void KheLayerAddChildNode(KHE_LAYER layer, KHE_NODE child_node);
extern void KheLayerDeleteChildNode(KHE_LAYER layer, KHE_NODE child_node);
extern int KheLayerChildNodeCount(KHE_LAYER layer);
extern KHE_NODE KheLayerChildNode(KHE_LAYER layer, int i);
extern void KheLayerChildNodesSort(KHE_LAYER layer,
  int(*compar)(const void *, const void *));

extern void KheLayerAddResource(KHE_LAYER layer, KHE_RESOURCE r);
extern void KheLayerDeleteResource(KHE_LAYER layer, KHE_RESOURCE r);
extern int KheLayerResourceCount(KHE_LAYER layer);
extern KHE_RESOURCE KheLayerResource(KHE_LAYER layer, int i);

extern int KheNodeChildLayerCount(KHE_NODE parent_node);
extern KHE_LAYER KheNodeChildLayer(KHE_NODE parent_node, int i);
extern void KheNodeChildLayersSort(KHE_NODE parent_node,
  int(*compar)(const void *, const void *));
extern void KheNodeChildLayersDelete(KHE_NODE parent_node);

extern int KheNodeParentLayerCount(KHE_NODE child_node);
extern KHE_LAYER KheNodeParentLayer(KHE_NODE child_node, int i);
extern bool KheNodeSameParentLayers(KHE_NODE node1, KHE_NODE node2);

extern int KheLayerDuration(KHE_LAYER layer);
extern int KheLayerMeetCount(KHE_LAYER layer);
extern int KheLayerAssignedDuration(KHE_LAYER layer);
extern int KheLayerDemand(KHE_LAYER layer);

extern bool KheLayerEqual(KHE_LAYER layer1, KHE_LAYER layer2);
extern bool KheLayerSubset(KHE_LAYER layer1, KHE_LAYER layer2);
extern bool KheLayerDisjoint(KHE_LAYER layer1, KHE_LAYER layer2);
extern bool KheLayerContains(KHE_LAYER layer, KHE_NODE n);

extern bool KheLayerSame(KHE_LAYER layer1, KHE_LAYER layer2, int *same_count);
extern bool KheLayerSimilar(KHE_LAYER layer1, KHE_LAYER layer2,
  int *similar_count);
extern bool KheLayerRegular(KHE_LAYER layer1, KHE_LAYER layer2,
  int *regular_count);
extern bool KheLayerAlign(KHE_LAYER layer1, KHE_LAYER layer2,
  bool (*node_equiv)(KHE_NODE node1, KHE_NODE node2), int *count);

extern void KheLayerMerge(KHE_LAYER layer1, KHE_LAYER layer2, KHE_LAYER *res);
extern void KheLayerDebug(KHE_LAYER layer, int verbosity, int indent, FILE *fp);

/* 5.4 Zones */
extern KHE_ZONE KheZoneMake(KHE_NODE node);
extern KHE_NODE KheZoneNode(KHE_ZONE zone);
extern int KheZoneNodeIndex(KHE_ZONE zone);
extern KHE_SOLN KheZoneSoln(KHE_ZONE zone);
extern void KheZoneSetBack(KHE_ZONE zone, void *back);
extern void *KheZoneBack(KHE_ZONE zone);

extern void KheZoneSetVisitNum(KHE_ZONE zone, int num);
extern int KheZoneVisitNum(KHE_ZONE zone);
extern bool KheZoneVisited(KHE_ZONE zone, int slack);
extern void KheZoneVisit(KHE_ZONE zone);
extern void KheZoneUnVisit(KHE_ZONE zone);

extern void KheZoneDelete(KHE_ZONE zone);
extern void KheNodeDeleteZones(KHE_NODE node);

extern void KheZoneAddMeetOffset(KHE_ZONE zone, KHE_MEET meet, int offset);
extern void KheZoneDeleteMeetOffset(KHE_ZONE zone, KHE_MEET meet, int offset);
extern KHE_ZONE KheMeetOffsetZone(KHE_MEET meet, int offset);
extern int KheNodeZoneCount(KHE_NODE node);
extern KHE_ZONE KheNodeZone(KHE_NODE node, int i);
extern int KheZoneMeetOffsetCount(KHE_ZONE zone);
extern void KheZoneMeetOffset(KHE_ZONE zone, int i,
  KHE_MEET *meet, int *offset);
extern void KheZoneDebug(KHE_ZONE zone, int verbosity, int indent, FILE *fp);
extern bool KheMeetMovePreservesZones(KHE_MEET meet1, int offset1,
  KHE_MEET meet2, int offset2, int durn);
extern int KheNodeIrregularity(KHE_NODE node);

/* 5.5 Taskings */
/* ***
extern KHE_TASKING KheTaskingMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt);
extern KHE_SOLN KheTaskingSoln(KHE_TASKING tasking);
extern KHE_RESOURCE_TYPE KheTaskingResourceType(KHE_TASKING tasking);
extern void KheTaskingDelete(KHE_TASKING tasking);

extern void KheTaskingAddTask(KHE_TASKING tasking, KHE_TASK task);
extern void KheTaskingDeleteTask(KHE_TASKING tasking, KHE_TASK task);
extern int KheTaskingTaskCount(KHE_TASKING tasking);
extern KHE_TASK KheTaskingTask(KHE_TASKING tasking, int i);

extern void KheTaskingDebug(KHE_TASKING tasking, int verbosity,
  int indent, FILE *fp);
*** */

/* 5.6 Task sets */
extern KHE_TASK_SET KheTaskSetMake(KHE_SOLN soln);
extern KHE_SOLN KheTaskSetSoln(KHE_TASK_SET ts);
extern void KheTaskSetDelete(KHE_TASK_SET ts);
extern void KheTaskSetClear(KHE_TASK_SET ts);
extern void KheTaskSetClearFromEnd(KHE_TASK_SET ts, int count);
extern void KheTaskSetDropFromEnd(KHE_TASK_SET ts, int n);
extern void KheTaskSetAddTask(KHE_TASK_SET ts, KHE_TASK task);
extern void KheTaskSetAddTaskSet(KHE_TASK_SET ts, KHE_TASK_SET ts2);
/* ***
extern void KheTaskSetAddTaskSorted(KHE_TASK_SET ts, KHE_TASK task,
  int(*compar)(const void *, const void *));
*** */
extern void KheTaskSetDeleteTask(KHE_TASK_SET ts, KHE_TASK task);
extern KHE_TASK KheTaskSetLastAndDelete(KHE_TASK_SET ts);
extern bool KheTaskSetContainsTask(KHE_TASK_SET ts, KHE_TASK task, int *pos);
extern int KheTaskSetTaskCount(KHE_TASK_SET ts);
extern KHE_TASK KheTaskSetTask(KHE_TASK_SET ts, int i);
extern KHE_TASK KheTaskSetFirst(KHE_TASK_SET ts);
extern KHE_TASK KheTaskSetLast(KHE_TASK_SET ts);
extern void KheTaskSetSort(KHE_TASK_SET ts,
  int(*compar)(const void *, const void *));
extern void KheTaskSetSortUnique(KHE_TASK_SET ts,
  int(*compar)(const void *, const void *));
extern int KheTaskSetTotalDuration(KHE_TASK_SET ts);
extern float KheTaskSetTotalWorkload(KHE_TASK_SET ts);
/* extern void KheTaskSetUnGroup(KHE_TASK_SET ts); */

extern bool KheTaskSetNeedsAssignment(KHE_TASK_SET ts);

extern void KheTaskSetSetVisitNum(KHE_TASK_SET ts, int num);
extern int KheTaskSetGetVisitNum(KHE_TASK_SET ts);
extern bool KheTaskSetAllVisited(KHE_TASK_SET ts, int slack);
extern bool KheTaskSetAnyVisited(KHE_TASK_SET ts, int slack);
extern void KheTaskSetAllVisit(KHE_TASK_SET ts);
extern void KheTaskSetAllUnVisit(KHE_TASK_SET ts);

extern bool KheTaskSetMoveResourceCheck(KHE_TASK_SET ts, KHE_RESOURCE r);
extern bool KheTaskSetMoveResource(KHE_TASK_SET ts, KHE_RESOURCE r);
extern bool KheTaskSetAssignResourceCheck(KHE_TASK_SET ts, KHE_RESOURCE r);
extern bool KheTaskSetAssignResource(KHE_TASK_SET ts, KHE_RESOURCE r);
extern bool KheTaskSetUnAssignResourceCheck(KHE_TASK_SET ts);
extern bool KheTaskSetUnAssignResource(KHE_TASK_SET ts);

extern bool KheTaskSetPartMoveResourceCheck(KHE_TASK_SET ts,
  int first_index, int last_index, KHE_RESOURCE r);
extern bool KheTaskSetPartMoveResource(KHE_TASK_SET ts,
  int first_index, int last_index, KHE_RESOURCE r);

extern void KheTaskSetAssignFix(KHE_TASK_SET ts);
extern void KheTaskSetAssignUnFix(KHE_TASK_SET ts);

extern void KheTaskSetDebug(KHE_TASK_SET ts, int verbosity, int indent,
  FILE *fp);

/* 5.7 Meet sets */
extern KHE_MEET_SET KheMeetSetMake(KHE_SOLN soln);
extern void KheMeetSetDelete(KHE_MEET_SET ms);
extern void KheMeetSetClear(KHE_MEET_SET ms);
extern void KheMeetSetDropFromEnd(KHE_MEET_SET ms, int n);
extern void KheMeetSetAddMeet(KHE_MEET_SET ms, KHE_MEET task);
extern void KheMeetSetDeleteMeet(KHE_MEET_SET ms, KHE_MEET task);
extern bool KheMeetSetContainsMeet(KHE_MEET_SET ms, KHE_MEET task, int *pos);
extern int KheMeetSetMeetCount(KHE_MEET_SET ms);
extern KHE_MEET KheMeetSetMeet(KHE_MEET_SET ms, int i);
extern void KheMeetSetSort(KHE_MEET_SET ms,
  int(*compar)(const void *, const void *));
extern void KheMeetSetSortUnique(KHE_MEET_SET ms,
  int(*compar)(const void *, const void *));
extern int KheMeetSetTotalDuration(KHE_MEET_SET ms);
extern float KheMeetSetTotalWorkload(KHE_MEET_SET ms);

extern void KheMeetSetSetVisitNum(KHE_MEET_SET ms, int num);
extern int KheMeetSetGetVisitNum(KHE_MEET_SET ms);
extern bool KheMeetSetVisited(KHE_MEET_SET ms, int slack);
extern void KheMeetSetVisit(KHE_MEET_SET ms);
extern void KheMeetSetUnVisit(KHE_MEET_SET ms);

extern void KheMeetSetDebug(KHE_MEET_SET ms, int verbosity, int indent,
  FILE *fp);

/* 5.8 Time sets */
extern KHE_TIME_SET KheTimeSetMake(KHE_INSTANCE ins, HA_ARENA a);
extern KHE_TIME_SET KheTimeSetCopy(KHE_TIME_SET ts, HA_ARENA a);
extern void KheTimeSetCopyElements(KHE_TIME_SET dst_ts, KHE_TIME_SET src_ts);
extern KHE_INSTANCE KheTimeSetInstance(KHE_TIME_SET ts);
extern void KheTimeSetClear(KHE_TIME_SET ts);
extern void KheTimeSetAddTime(KHE_TIME_SET ts, KHE_TIME t);
extern void KheTimeSetAddTimeGroup(KHE_TIME_SET ts, KHE_TIME_GROUP tg);
extern void KheTimeSetAddTaskTimes(KHE_TIME_SET ts, KHE_TASK task);
extern void KheTimeSetDeleteTime(KHE_TIME_SET ts, KHE_TIME t);
extern void KheTimeSetDeleteTimeGroup(KHE_TIME_SET ts, KHE_TIME_GROUP tg);
extern void KheTimeSetDeleteLastTime(KHE_TIME_SET ts);
extern int KheTimeSetTimeCount(KHE_TIME_SET ts);
extern KHE_TIME KheTimeSetTime(KHE_TIME_SET ts, int i);
extern int KheTimeSetTimeIndex(KHE_TIME_SET ts, int i);
extern void KheTimeSetUnion(KHE_TIME_SET ts1, KHE_TIME_SET ts2);
extern void KheTimeSetIntersect(KHE_TIME_SET ts1, KHE_TIME_SET ts2);
extern void KheTimeSetDifference(KHE_TIME_SET ts1, KHE_TIME_SET ts2);
extern int KheTimeSetUnionCount(KHE_TIME_SET ts1, KHE_TIME_SET ts2);
extern int KheTimeSetIntersectCount(KHE_TIME_SET ts1, KHE_TIME_SET ts2);
extern int KheTimeSetDifferenceCount(KHE_TIME_SET ts1, KHE_TIME_SET ts2);
extern bool KheTimeSetEmpty(KHE_TIME_SET ts);
extern bool KheTimeSetEqual(KHE_TIME_SET ts1, KHE_TIME_SET ts2);
extern bool KheTimeSetSubset(KHE_TIME_SET ts1, KHE_TIME_SET ts2);
extern bool KheTimeSetDisjoint(KHE_TIME_SET ts1, KHE_TIME_SET ts2);
extern bool KheTimeSetContainsTime(KHE_TIME_SET ts, KHE_TIME t);
extern int KheTimeSetHash(KHE_TIME_SET ts);
extern int KheTimeSetCmp(const void *t1, const void *t2);
extern int KheTimeSetTypedCmp(KHE_TIME_SET ts1, KHE_TIME_SET ts2);
extern int KheTimeSetCmpReverse(const void *t1, const void *t2);
extern int KheTimeSetTypedCmpReverse(KHE_TIME_SET ts1, KHE_TIME_SET ts2);
extern void KheTimeSetDebug(KHE_TIME_SET ts, int verbosity, int indent,
  FILE *fp);

/* 5.9 Resource sets */
extern KHE_RESOURCE_SET KheResourceSetMake(KHE_RESOURCE_TYPE rt, HA_ARENA a);
extern KHE_RESOURCE_SET KheResourceSetCopy(KHE_RESOURCE_SET rs, HA_ARENA a);
extern void KheResourceSetCopyElements(KHE_RESOURCE_SET dst_rs,
  KHE_RESOURCE_SET src_rs);
extern KHE_RESOURCE_TYPE KheResourceSetResourceType(KHE_RESOURCE_SET rs);
extern void KheResourceSetClear(KHE_RESOURCE_SET rs);

extern void KheResourceSetAddResource(KHE_RESOURCE_SET rs, KHE_RESOURCE r);
extern void KheResourceSetAddResourceGroup(KHE_RESOURCE_SET rs,
  KHE_RESOURCE_GROUP rg);
extern void KheResourceSetDeleteResource(KHE_RESOURCE_SET rs, KHE_RESOURCE r);
extern void KheResourceSetDeleteLastResource(KHE_RESOURCE_SET rs);

extern int KheResourceSetResourceCount(KHE_RESOURCE_SET rs);
extern KHE_RESOURCE KheResourceSetResource(KHE_RESOURCE_SET rs, int i);
/* extern int KheResourceSetResourceIndex(KHE_RESOURCE_SET rs, int i); */

extern void KheResourceSetUnion(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2);
extern void KheResourceSetUnionGroup(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_GROUP rg2);
extern int KheResourceSetUnionCount(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2);
extern int KheResourceSetUnionCountGroup(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_GROUP rg2);

extern void KheResourceSetIntersect(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2);
extern void KheResourceSetIntersectGroup(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_GROUP rg2);
extern int KheResourceSetIntersectCount(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_SET rs2);
extern int KheResourceSetIntersectCountGroup(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_GROUP rg2);

extern void KheResourceSetDifference(KHE_RESOURCE_SET rs1,KHE_RESOURCE_SET rs2);
extern void KheResourceSetDifferenceGroup(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_GROUP rg2);
extern int KheResourceSetDifferenceCount(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_SET rs2);
extern int KheResourceSetDifferenceCountGroup(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_GROUP rg2);

extern int KheResourceSetSymmetricDifferenceCount(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_SET rs2);
extern int KheResourceSetSymmetricDifferenceCountGroup(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_GROUP rg2);

extern bool KheResourceSetEqual(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2);
extern bool KheResourceSetEqualGroup(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_GROUP rg2);

extern bool KheResourceSetSubset(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2);
extern bool KheResourceSetSubsetGroup(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_GROUP rg2);

extern bool KheResourceSetDisjoint(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2);
extern bool KheResourceSetDisjointGroup(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_GROUP rg2);

extern bool KheResourceSetContainsResource(KHE_RESOURCE_SET rs, KHE_RESOURCE r);

extern int KheResourceSetCmp(const void *t1, const void *t2);
extern int KheResourceSetTypedCmp(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2);
extern void KheResourceSetDebug(KHE_RESOURCE_SET rs, int verbosity, int indent,
  FILE *fp);

/* 5.10 Frames */
extern KHE_FRAME KheFrameMakeBegin(KHE_INSTANCE ins, HA_ARENA a);
extern void KheFrameAddTimeGroup(KHE_FRAME frame, KHE_TIME_GROUP tg);
extern void KheFrameMakeEnd(KHE_FRAME frame, bool sort_time_groups);

extern KHE_INSTANCE KheFrameInstance(KHE_FRAME frame);

/* ***
extern KHE_FRAME_MAKE KheFrameMakeBegin(KHE_SOLN soln);
extern void KheFrameMakeAddTimeGroup(KHE_FRAME_MAKE fm, KHE_TIME_GROUP tg);
extern KHE_FRAME KheFrameMakeEnd(KHE_FRAME_MAKE fm, bool sort_time_groups);
extern void KheFrameDelete(KHE_FRAME frame);
extern KHE_SOLN KheFrameSoln(KHE_FRAME frame);
*** */

extern int KheFrameTimeGroupCount(KHE_FRAME frame);
extern KHE_TIME_GROUP KheFrameTimeGroup(KHE_FRAME frame, int i);

/* ***
extern KHE_FRAME KheFrameMakeNull(void);
extern bool KheFrameIsNull(KHE_FRAME frame);

extern KHE_FRAME KheFrameSlice(KHE_FRAME frame, int start_index,
  int stop_index);
extern KHE_FRAME KheFrameInsideLeftSlice(KHE_FRAME frame, int len);
extern KHE_FRAME KheFrameInsideRightSlice(KHE_FRAME frame, int len);
extern KHE_FRAME KheFrameOutsideLeftSlice(KHE_FRAME frame, int len);
extern KHE_FRAME KheFrameOutsideRightSlice(KHE_FRAME frame, int len);
*** */

extern bool KheFrameIsDisjoint(KHE_FRAME frame, int *problem_index1,
  int *problem_index2);
extern bool KheFrameIsComplete(KHE_FRAME frame, KHE_TIME *problem_time);
extern int KheFrameTimeIndex(KHE_FRAME frame, KHE_TIME t);
extern KHE_TIME_GROUP KheFrameTimeTimeGroup(KHE_FRAME frame, KHE_TIME t);
extern int KheFrameTimeOffset(KHE_FRAME frame, KHE_TIME t);
extern KHE_FRAME KheFrameMakeCommon(KHE_INSTANCE ins, HA_ARENA a);
extern KHE_FRAME KheFrameMakeSingletons(KHE_INSTANCE ins, HA_ARENA a);

extern bool KheFrameIntersectsTimeGroup(KHE_FRAME frame, KHE_TIME_GROUP tg);

/* ***
extern KHE_FRAME_WORKLOAD KheFrameWorkloadMake(KHE_FRAME frame,
  KHE_RESOURCE_TYPE rt, KHE_EVENT_TIMETABLE_MONITOR etm);
extern int KheFrameResourceMaxBusyTimes(KHE_FRAME frame,
  KHE_FRAME_WORKLOAD fw, KHE_RESOURCE r);
extern void KheFrameWorkloadDelete(KHE_FRAME_WORKLOAD fw);
*** */

extern void KheFrameDebug(KHE_FRAME frame, int verbosity, int indent, FILE *fp);
extern bool KheFrameResourceHasClashes(KHE_FRAME frame, KHE_RESOURCE r,
  KHE_SOLN soln);
extern void KheFrameResourceAssertNoClashes(KHE_FRAME frame, KHE_RESOURCE r,
  KHE_SOLN soln);
extern void KheFrameAssertNoClashes(KHE_FRAME frame, KHE_SOLN soln);


/*****************************************************************************/
/*                                                                           */
/*    Chapter 6.   Solution Monitoring                                       */
/*                                                                           */
/*****************************************************************************/

/* 6.1 Measuring cost */
extern KHE_COST KheSolnCost(KHE_SOLN soln);
extern KHE_COST KheCost(int hard_cost, int soft_cost);
extern int KheHardCost(KHE_COST combined_cost);
extern int KheSoftCost(KHE_COST combined_cost);
extern int KheCostCmp(KHE_COST cost1, KHE_COST cost2);
extern double KheCostShow(KHE_COST combined_cost);

/* 6.2 Monitors */
extern void KheMonitorSetBack(KHE_MONITOR m, void *back);
extern void *KheMonitorBack(KHE_MONITOR m);

extern void KheMonitorSetVisitNum(KHE_MONITOR m, int num);
extern int KheMonitorVisitNum(KHE_MONITOR m);
extern bool KheMonitorVisited(KHE_MONITOR m, int slack);
extern void KheMonitorVisit(KHE_MONITOR m);
extern void KheMonitorUnVisit(KHE_MONITOR m);

extern KHE_SOLN KheMonitorSoln(KHE_MONITOR m);
extern int KheMonitorSolnIndex(KHE_MONITOR m);
extern KHE_COST KheMonitorCost(KHE_MONITOR m);
extern KHE_COST KheMonitorLowerBound(KHE_MONITOR m);

extern KHE_COST KheMonitorCombinedWeight(KHE_MONITOR m);
extern void KheMonitorSetCombinedWeight(KHE_MONITOR m,
  KHE_COST combined_weight);
extern void KheMonitorResetCombinedWeight(KHE_MONITOR m);

extern KHE_COST_FUNCTION KheMonitorCostFunction(KHE_MONITOR m);
extern KHE_COST KheMonitorDevToCost(KHE_MONITOR m, int dev);

extern KHE_MONITOR_TAG KheMonitorTag(KHE_MONITOR m);
extern KHE_CONSTRAINT KheMonitorConstraint(KHE_MONITOR m);
extern char *KheMonitorAppliesToName(KHE_MONITOR m);
extern char *KheMonitorPointOfApplication(KHE_MONITOR m);
extern char *KheMonitorId(KHE_MONITOR m);

extern int KheMonitorDeviation(KHE_MONITOR m);
extern char *KheMonitorDeviationDescription(KHE_MONITOR m);

extern int KheSolnMonitorCount(KHE_SOLN soln);
extern KHE_MONITOR KheSolnMonitor(KHE_SOLN soln, int i);
extern bool KheSolnRetrieveMonitor(KHE_SOLN soln, char *id, KHE_MONITOR *m);

extern void KheMonitorDebug(KHE_MONITOR m, int verbosity, int indent, FILE *fp);
extern char *KheMonitorTagShow(KHE_MONITOR_TAG tag);
extern char *KheMonitorLabel(KHE_MONITOR m);

/* 6.3 Attaching, detaching, and provably zero fixed cost */
extern void KheMonitorDetachFromSoln(KHE_MONITOR m);
extern void KheMonitorAttachToSoln(KHE_MONITOR m);
extern bool KheMonitorAttachedToSoln(KHE_MONITOR m);
extern void KheSolnEnsureOfficialCost(KHE_SOLN soln);

/* 6.4 Time ranges and sweep times */
extern bool KheMonitorTimeRange(KHE_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time);
extern void KheMonitorSetSweepTime(KHE_MONITOR m, KHE_TIME t);
extern bool KheMonitorSweepTimeRange(KHE_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time);

/* 6.5 Event monitors */
extern int KheSolnEventMonitorCount(KHE_SOLN soln, KHE_EVENT e);
extern KHE_MONITOR KheSolnEventMonitor(KHE_SOLN soln, KHE_EVENT e, int i);

extern KHE_COST KheSolnEventCost(KHE_SOLN soln, KHE_EVENT e);
extern KHE_COST KheSolnEventMonitorCost(KHE_SOLN soln, KHE_EVENT e,
  KHE_MONITOR_TAG tag);

/* 6.5.1 Split events monitors */
extern KHE_SPLIT_EVENTS_CONSTRAINT KheSplitEventsMonitorConstraint(
  KHE_SPLIT_EVENTS_MONITOR m);
extern KHE_EVENT KheSplitEventsMonitorEvent(KHE_SPLIT_EVENTS_MONITOR m);
extern void KheSplitEventsMonitorLimits(KHE_SPLIT_EVENTS_MONITOR m,
  int *min_duration, int *max_duration, int *min_amount, int *max_amount);
extern void KheSplitEventsMonitorDebug(KHE_SPLIT_EVENTS_MONITOR m,
  int verbosity, int indent, FILE *fp);

/* 6.5.2 Distribute split events monitors */
extern KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT
  KheDistributeSplitEventsMonitorConstraint(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m);
extern KHE_EVENT KheDistributeSplitEventsMonitorEvent(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m);
extern void KheDistributeEventsMonitorLimits(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m,
  int *duration, int *minimum, int *maximum, int *meet_count);
extern void KheDistributeSplitEventsMonitorDebug(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, int verbosity, int indent, FILE *fp);

/* 6.5.3 Assign time monitors */
extern KHE_ASSIGN_TIME_CONSTRAINT KheAssignTimeMonitorConstraint(
  KHE_ASSIGN_TIME_MONITOR m);
extern KHE_EVENT KheAssignTimeMonitorEvent(KHE_ASSIGN_TIME_MONITOR m);
extern void KheAssignTimeMonitorDebug(KHE_ASSIGN_TIME_MONITOR m,
  int verbosity, int indent, FILE *fp);

/* 6.5.4 Prefer times monitors */
extern KHE_PREFER_TIMES_CONSTRAINT KhePreferTimesMonitorConstraint(
  KHE_PREFER_TIMES_MONITOR m);
extern KHE_EVENT KhePreferTimesMonitorEvent(KHE_PREFER_TIMES_MONITOR m);
extern void KhePreferTimesMonitorDebug(KHE_PREFER_TIMES_MONITOR m,
  int verbosity, int indent, FILE *fp);

/* 6.5.5 Spread events monitors */
extern KHE_SPREAD_EVENTS_CONSTRAINT KheSpreadEventsMonitorConstraint(
  KHE_SPREAD_EVENTS_MONITOR m);
extern KHE_EVENT_GROUP KheSpreadEventsMonitorEventGroup(
  KHE_SPREAD_EVENTS_MONITOR m);
extern int KheSpreadEventsMonitorTimeGroupCount(KHE_SPREAD_EVENTS_MONITOR m);
extern void KheSpreadEventsMonitorTimeGroup(KHE_SPREAD_EVENTS_MONITOR m, int i,
  KHE_TIME_GROUP *time_group, int *minimum, int *maximum, int *incidences);
extern void KheSpreadEventsMonitorDebug(KHE_SPREAD_EVENTS_MONITOR m,
  int verbosity, int indent, FILE *fp);

/* 6.5.6 Link events monitors */
extern KHE_LINK_EVENTS_CONSTRAINT KheLinkEventsMonitorConstraint(
  KHE_LINK_EVENTS_MONITOR m);
extern KHE_EVENT_GROUP KheLinkEventsMonitorEventGroup(
  KHE_LINK_EVENTS_MONITOR m);
extern void KheLinkEventsMonitorDebug(KHE_LINK_EVENTS_MONITOR m,
  int verbosity, int indent, FILE *fp);

/* 6.5.7 Order events monitors */
extern KHE_ORDER_EVENTS_CONSTRAINT KheOrderEventsMonitorConstraint(
  KHE_ORDER_EVENTS_MONITOR m);
extern KHE_EVENT KheOrderEventsMonitorFirstEvent(KHE_ORDER_EVENTS_MONITOR m);
extern KHE_EVENT KheOrderEventsMonitorSecondEvent(KHE_ORDER_EVENTS_MONITOR m);
extern int KheOrderEventsMonitorMinSeparation(KHE_ORDER_EVENTS_MONITOR m);
extern int KheOrderEventsMonitorMaxSeparation(KHE_ORDER_EVENTS_MONITOR m);
extern void KheOrderEventsMonitorDebug(KHE_ORDER_EVENTS_MONITOR m,
  int verbosity, int indent, FILE *fp);

/* 6.6 Event resource monitors */
extern int KheSolnEventResourceMonitorCount(KHE_SOLN soln,
  KHE_EVENT_RESOURCE er);
extern KHE_MONITOR KheSolnEventResourceMonitor(KHE_SOLN soln,
  KHE_EVENT_RESOURCE er, int i);

extern KHE_COST KheSolnEventResourceCost(KHE_SOLN soln, KHE_EVENT_RESOURCE er);
extern KHE_COST KheSolnEventResourceMonitorCost(KHE_SOLN soln,
  KHE_EVENT_RESOURCE er, KHE_MONITOR_TAG tag);

/* 6.6.1 Assign resource monitors */
extern KHE_ASSIGN_RESOURCE_CONSTRAINT KheAssignResourceMonitorConstraint(
  KHE_ASSIGN_RESOURCE_MONITOR m);
extern KHE_EVENT_RESOURCE KheAssignResourceMonitorEventResource(
  KHE_ASSIGN_RESOURCE_MONITOR m);

extern void KheAssignResourceMonitorDebug(KHE_ASSIGN_RESOURCE_MONITOR m,
  int verbosity, int indent, FILE *fp);

/* 6.6.2 Prefer resources monitors */
extern KHE_PREFER_RESOURCES_CONSTRAINT KhePreferResourcesMonitorConstraint(
  KHE_PREFER_RESOURCES_MONITOR m);
extern KHE_EVENT_RESOURCE KhePreferResourcesMonitorEventResource(
  KHE_PREFER_RESOURCES_MONITOR m);
extern KHE_RESOURCE_GROUP KhePreferResourcesMonitorDomain(
  KHE_PREFER_RESOURCES_MONITOR m);
extern KHE_RESOURCE_GROUP KhePreferResourcesMonitorDomainComplement(
  KHE_PREFER_RESOURCES_MONITOR m);

extern void KhePreferResourcesMonitorDebug(KHE_PREFER_RESOURCES_MONITOR m,
  int verbosity, int indent, FILE *fp);

/* 6.6.3 Avoid split assignments monitors */
extern KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT
  KheAvoidSplitAssignmentsMonitorConstraint(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m);
extern int KheAvoidSplitAssignmentsMonitorEventGroupIndex(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m);

extern int KheAvoidSplitAssignmentsMonitorResourceCount(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m);
extern KHE_RESOURCE KheAvoidSplitAssignmentsMonitorResource(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, int i);
extern int KheAvoidSplitAssignmentsMonitorResourceMultiplicity(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, int i);

extern void KheAvoidSplitAssignmentsMonitorDebug(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, int verbosity, int indent, FILE *fp);

/* 6.6.4 Limit resources monitors */
extern KHE_LIMIT_RESOURCES_CONSTRAINT KheLimitResourcesMonitorConstraint(
  KHE_LIMIT_RESOURCES_MONITOR m);
extern int KheLimitResourcesMonitorEventGroupIndex(
  KHE_LIMIT_RESOURCES_MONITOR m);
extern void KheLimitResourcesMonitorActiveDuration(
  KHE_LIMIT_RESOURCES_MONITOR m, int *minimum, int *maximum, int *active_durn);

extern void KheLimitResourcesMonitorDebug(KHE_LIMIT_RESOURCES_MONITOR m,
  int verbosity, int indent, FILE *fp);
/* ***
extern void KheLimitResourcesMonitorCheck(KHE_LIMIT_RESOURCES_MONITOR lrm,
  char *pos);
*** */

/* 6.7 Resource monitors */
extern int KheSolnResourceMonitorCount(KHE_SOLN soln, KHE_RESOURCE r);
extern KHE_MONITOR KheSolnResourceMonitor(KHE_SOLN soln, KHE_RESOURCE r, int i);

extern void KheSolnResourceMonitorSort(KHE_SOLN soln, KHE_RESOURCE r,
  int(*compar)(const void *, const void *));

extern KHE_COST KheSolnResourceCost(KHE_SOLN soln, KHE_RESOURCE r);
extern KHE_COST KheSolnResourceMonitorCost(KHE_SOLN soln, KHE_RESOURCE r,
  KHE_MONITOR_TAG tag);

/* 6.7.1 Avoid clashes monitors */
extern KHE_AVOID_CLASHES_CONSTRAINT KheAvoidClashesMonitorConstraint(
  KHE_AVOID_CLASHES_MONITOR m);
extern KHE_RESOURCE KheAvoidClashesMonitorResource(KHE_AVOID_CLASHES_MONITOR m);
extern void KheAvoidClashesMonitorDebug(KHE_AVOID_CLASHES_MONITOR m,
  int verbosity, int indent, FILE *fp);

/* 6.7.2 Avoid unavailable times monitors */
extern KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT
  KheAvoidUnavailableTimesMonitorConstraint(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m);
extern KHE_RESOURCE KheAvoidUnavailableTimesMonitorResource(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m);
extern void KheAvoidUnavailableTimesMonitorDebug(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m, int verbosity, int indent, FILE *fp);

/* 6.7.3 Limit idle times monitors */
extern KHE_LIMIT_IDLE_TIMES_CONSTRAINT KheLimitIdleTimesMonitorConstraint(
  KHE_LIMIT_IDLE_TIMES_MONITOR m);
extern KHE_RESOURCE KheLimitIdleTimesMonitorResource(
  KHE_LIMIT_IDLE_TIMES_MONITOR m);

extern int KheLimitIdleTimesMonitorTimeGroupCount(
  KHE_LIMIT_IDLE_TIMES_MONITOR m);
extern KHE_TIME_GROUP KheLimitIdleTimesMonitorTimeGroup(
  KHE_LIMIT_IDLE_TIMES_MONITOR m, int i);
extern KHE_TIME_GROUP KheLimitIdleTimesMonitorTimeGroupState(
  KHE_LIMIT_IDLE_TIMES_MONITOR m, int i, int *busy_count, int *idle_count,
  KHE_TIME extreme_busy_times[2], int *extreme_busy_times_count);

extern void KheLimitIdleTimesMonitorDebug(KHE_LIMIT_IDLE_TIMES_MONITOR m,
  int verbosity, int indent, FILE *fp);

/* 6.7.4 Cluster busy times monitors */
extern KHE_CLUSTER_BUSY_TIMES_CONSTRAINT KheClusterBusyTimesMonitorConstraint(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m);
extern KHE_RESOURCE KheClusterBusyTimesMonitorResource(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m);

extern int KheClusterBusyTimesMonitorHistoryBefore(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m);
extern int KheClusterBusyTimesMonitorHistoryAfter(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m);
extern int KheClusterBusyTimesMonitorHistory(KHE_CLUSTER_BUSY_TIMES_MONITOR m);

extern int KheClusterBusyTimesMonitorOffset(KHE_CLUSTER_BUSY_TIMES_MONITOR m);

extern int KheClusterBusyTimesMonitorTimeGroupCount(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m);
extern KHE_TIME_GROUP KheClusterBusyTimesMonitorTimeGroup(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int i, KHE_POLARITY *po);

extern void KheClusterBusyTimesMonitorActiveTimeGroupCount(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int *active_group_count,
  int *open_group_count, int *minimum, int *maximum, bool *allow_zero);

extern bool KheClusterBusyTimesMonitorTimeGroupIsActive(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int i, KHE_TIME_GROUP *tg,
  KHE_POLARITY *po, int *busy_count);

extern int KheClusterBusyTimesMonitorAtMaxLimitCount(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m);

extern void KheClusterBusyTimesMonitorSetNotBusyState(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int i, bool active);
extern void KheClusterBusyTimesMonitorClearNotBusyState(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int i);

/* ***
extern KHE_FRAME KheClusterBusyTimesMonitorFrame(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m);
*** */

/* ***
extern void KheClusterBusyTimesMonitorSetMultiplier(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int val);
extern int KheClusterBusyTimesMonitorMultiplier(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m);
*** */

extern int KheClusterBusyTimesMonitorMaximum(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m);
extern int KheClusterBusyTimesMonitorMinimum(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m);
extern void KheClusterBusyTimesMonitorSetMinimum(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int val);
extern void KheClusterBusyTimesMonitorResetMinimum(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m);

extern void KheClusterBusyTimesMonitorDebug(KHE_CLUSTER_BUSY_TIMES_MONITOR m,
  int verbosity, int indent, FILE *fp);

/* 6.7.5 Limit effort monitors */
/* *** postponed, possibly cancelled
extern KHE_LIMIT_EFFORT_CONSTRAINT KheLimitEffortMonitorConstraint(
  KHE_LIMIT_EFFORT_MONITOR m);
extern KHE_RESOURCE KheLimitEffortMonitorResource(
  KHE_LIMIT_EFFORT_MONITOR m);
extern int KheLimitEffortMonitorOffset(KHE_LIMIT_EFFORT_MONITOR m);

extern int KheLimitEffortMonitorTimeGroupCount(
  KHE_LIMIT_EFFORT_MONITOR m);
extern KHE_TIME_GROUP KheLimitEffortMonitorTimeGroup(
  KHE_LIMIT_EFFORT_MONITOR m, int i, int *busy_count, float *workload);
extern void KheLimitEffortMonitorRange(KHE_LIMIT_EFFORT_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time);

extern int KheLimitEffortMonitorDefectiveTimeGroupCount(
  KHE_LIMIT_EFFORT_MONITOR m);
extern void KheLimitEffortMonitorDefectiveTimeGroup(
  KHE_LIMIT_EFFORT_MONITOR m, int i, KHE_TIME_GROUP *tg, int *busy_count,
  float *workload, int *minimum, int *maximum, bool *allow_zero);
extern int KheLimitEffortMonitorDeviation(KHE_LIMIT_EFFORT_MONITOR m);

extern void KheLimitEffortMonitorSetCeiling(KHE_LIMIT_EFFORT_MONITOR m,
  int ceiling);
extern int KheLimitEffortMonitorCeiling(KHE_LIMIT_EFFORT_MONITOR m);

extern void KheLimitEffortMonitorDebug(KHE_LIMIT_EFFORT_MONITOR m,
  int verbosity, int indent, FILE *fp);
*** */

/* 6.7.5 Limit busy times monitors */
extern KHE_LIMIT_BUSY_TIMES_CONSTRAINT KheLimitBusyTimesMonitorConstraint(
  KHE_LIMIT_BUSY_TIMES_MONITOR m);
extern KHE_RESOURCE KheLimitBusyTimesMonitorResource(
  KHE_LIMIT_BUSY_TIMES_MONITOR m);
extern int KheLimitBusyTimesMonitorOffset(KHE_LIMIT_BUSY_TIMES_MONITOR m);

extern int KheLimitBusyTimesMonitorTimeGroupCount(
  KHE_LIMIT_BUSY_TIMES_MONITOR m);
extern KHE_TIME_GROUP KheLimitBusyTimesMonitorTimeGroup(
  KHE_LIMIT_BUSY_TIMES_MONITOR m, int i, int *busy_count);

extern int KheLimitBusyTimesMonitorDefectiveTimeGroupCount(
  KHE_LIMIT_BUSY_TIMES_MONITOR m);
extern void KheLimitBusyTimesMonitorDefectiveTimeGroup(
  KHE_LIMIT_BUSY_TIMES_MONITOR m, int i, KHE_TIME_GROUP *tg,
  int *busy_count, int *open_count, int *minimum, int *maximum,
  bool *allow_zero);
extern int KheLimitBusyTimesMonitorDeviation(KHE_LIMIT_BUSY_TIMES_MONITOR m);

extern void KheLimitBusyTimesMonitorSetCeiling(KHE_LIMIT_BUSY_TIMES_MONITOR m,
  int ceiling);
extern int KheLimitBusyTimesMonitorCeiling(KHE_LIMIT_BUSY_TIMES_MONITOR m);

extern void KheLimitBusyTimesMonitorDebug(KHE_LIMIT_BUSY_TIMES_MONITOR m,
  int verbosity, int indent, FILE *fp);

/* 6.7.6 Limit workload monitors */
extern KHE_LIMIT_WORKLOAD_CONSTRAINT KheLimitWorkloadMonitorConstraint(
  KHE_LIMIT_WORKLOAD_MONITOR m);
extern KHE_RESOURCE KheLimitWorkloadMonitorResource(
  KHE_LIMIT_WORKLOAD_MONITOR m);
extern int KheLimitWorkloadMonitorOffset(KHE_LIMIT_WORKLOAD_MONITOR m);

extern int KheLimitWorkloadMonitorTimeGroupCount(
  KHE_LIMIT_WORKLOAD_MONITOR m);
extern KHE_TIME_GROUP KheLimitWorkloadMonitorTimeGroup(
  KHE_LIMIT_WORKLOAD_MONITOR m, int i, float *workload);

extern int KheLimitWorkloadMonitorDefectiveTimeGroupCount(
  KHE_LIMIT_WORKLOAD_MONITOR m);
extern void KheLimitWorkloadMonitorDefectiveTimeGroup(
  KHE_LIMIT_WORKLOAD_MONITOR m, int i, KHE_TIME_GROUP *tg,
  float *workload, int *open_count, int *minimum, int *maximum,
  bool *allow_zero);
extern int KheLimitWorkloadMonitorDeviation(KHE_LIMIT_WORKLOAD_MONITOR m);

extern void KheLimitWorkloadMonitorSetCeiling(KHE_LIMIT_WORKLOAD_MONITOR m,
  int ceiling);
extern int KheLimitWorkloadMonitorCeiling(KHE_LIMIT_WORKLOAD_MONITOR m);

extern void KheLimitWorkloadMonitorDebug(KHE_LIMIT_WORKLOAD_MONITOR m,
  int verbosity, int indent, FILE *fp);

/* 6.7.7 Limit active intervals monitors */
extern KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT
 KheLimitActiveIntervalsMonitorConstraint(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m);
extern KHE_RESOURCE KheLimitActiveIntervalsMonitorResource(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m);
extern int KheLimitActiveIntervalsMonitorMinimum(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m);
extern int KheLimitActiveIntervalsMonitorMaximum(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m);
extern int KheLimitActiveIntervalsMonitorHistoryBefore(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m);
extern int KheLimitActiveIntervalsMonitorHistoryAfter(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m);
extern int KheLimitActiveIntervalsMonitorHistory(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m);
extern int KheLimitActiveIntervalsMonitorOffset(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m);

extern int KheLimitActiveIntervalsMonitorTimeGroupCount(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m);
extern KHE_TIME_GROUP KheLimitActiveIntervalsMonitorTimeGroup(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int i, KHE_POLARITY *po);
/* ***
extern void KheLimitActiveIntervalsMonitorRange(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time);
*** */

extern bool KheLimitActiveIntervalsMonitorTimeGroupIsActive(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int i, KHE_TIME_GROUP *tg,
  KHE_POLARITY *po, int *busy_count);
extern int KheLimitActiveIntervalsMonitorDefectiveIntervalCount(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m);
extern void KheLimitActiveIntervalsMonitorDefectiveInterval(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int i, int *history_before,
  int *first_index, int *last_index, int *history_after, bool *too_long);

extern int KheLimitActiveIntervalsMonitorAtMaxLimitCount(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m);

extern void KheLimitActiveIntervalsMonitorSetNotBusyState(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int i, bool active);
extern void KheLimitActiveIntervalsMonitorClearNotBusyState(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int i);

extern void KheLimitActiveIntervalsMonitorSetTilt(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m);
extern void KheLimitActiveIntervalsMonitorClearTilt(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m);
extern bool KheLimitActiveIntervalsMonitorTilt(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m);

/* ***
extern KHE_FRAME KheLimitActiveIntervalsMonitorFrame(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m);
*** */

extern void KheLimitActiveIntervalsMonitorDebug(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int verbosity, int indent, FILE *fp);

/* 6.8.1 Event timetable monitors */
extern KHE_EVENT_TIMETABLE_MONITOR KheEventTimetableMonitor(KHE_SOLN soln,
  KHE_EVENT e);

extern int KheEventTimetableMonitorTimeMeetCount(
  KHE_EVENT_TIMETABLE_MONITOR etm, KHE_TIME time);
extern KHE_MEET KheEventTimetableMonitorTimeMeet(
  KHE_EVENT_TIMETABLE_MONITOR etm, KHE_TIME time, int i);
extern bool KheEventTimetableMonitorTimeAvailable(
  KHE_EVENT_TIMETABLE_MONITOR etm, KHE_MEET meet, KHE_TIME time);

extern void KheEventTimetableMonitorPrintTimetable(
  KHE_EVENT_TIMETABLE_MONITOR etm, int cell_width, int indent, FILE *fp);

extern KHE_EVENT_TIMETABLE_MONITOR KheEventTimetableMonitorMake(KHE_SOLN soln,
  KHE_EVENT_GROUP eg);
extern void KheEventTimetableMonitorDelete(KHE_EVENT_TIMETABLE_MONITOR etm);

extern void KheEventTimetableMonitorDebug(KHE_EVENT_TIMETABLE_MONITOR etm,
  int verbosity, int indent, FILE *fp);

/* 6.8.2 Resource timetable monitors */
extern KHE_RESOURCE_TIMETABLE_MONITOR KheResourceTimetableMonitor(
  KHE_SOLN soln, KHE_RESOURCE r);
extern KHE_SOLN KheResourceTimetableMonitorSoln(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm);
extern KHE_RESOURCE KheResourceTimetableMonitorResource(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm);

extern int KheResourceTimetableMonitorTimeTaskCount(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME time);
extern KHE_TASK KheResourceTimetableMonitorTimeTask(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME time, int i);

/* ***
extern int KheResourceTimetableMonitorBusyTimes(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm);
*** */

extern bool KheResourceTimetableMonitorTimeAvailable(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_MEET meet, KHE_TIME time);

/* ***
extern bool KheResourceTimetableMonitorTimeGroupAvailable(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg,
  bool nocost_off, bool check_avoid_unavailable_times_monitors);
extern bool KheResourceTimetableMonitorTaskAvailableInFrame(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task, KHE_FRAME frame,
  KHE_TASK ignore_task);
*** */

extern int KheResourceTimetableMonitorBusyTimesForTimeGroup(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg);

extern bool KheResourceTimetableMonitorFreeForTime(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME t,
  KHE_FRAME frame, KHE_TASK ignore_task, bool ignore_nocost_tasks);
extern bool KheResourceTimetableMonitorFreeForTimeGroup(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg,
  KHE_FRAME frame, KHE_TASK ignore_task, bool ignore_nocost_tasks);
extern bool KheResourceTimetableMonitorFreeForTask(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task,
  KHE_FRAME frame, KHE_TASK ignore_task, bool ignore_nocost_tasks);

extern bool KheResourceTimetableMonitorAvailableForTime(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME t);
extern bool KheResourceTimetableMonitorAvailableForTimeGroup(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg);
extern bool KheResourceTimetableMonitorAvailableForTask(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task);

extern void KheResourceTimetableMonitorAddProperRootTasksInTimeGroup(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg,
  bool include_preassigned, KHE_TASK_SET ts);
extern void KheResourceTimetableMonitorAddProperRootTasksInTimeRange(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, int first_time_index,
  int last_time_index, bool include_preassigned, KHE_TASK_SET ts);
extern void KheResourceTimetableMonitorAddProperRootTasksInInterval(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_FRAME frame,
  int first_frame_index, int last_frame_index,
  bool include_preassigned, KHE_TASK_SET ts);

/* ***
typedef enum {
  KHE_BUSY_NONE = 0,
  KHE_BUSY_SOME = 1,
  KHE_BUSY_ALL = 2
} KHE_BUSY_TYPE;
*** */

/* *** obsolete, using khe_task_finder.c now
extern KHE_BUSY_TYPE KheResourceTimetableMonitorTaskBusyType(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task,
  KHE_FRAME days_frame, KHE_TASK_SET r_ts, bool nocost_off);
extern KHE_BUSY_TYPE KheResourceTimetableMonitorTaskSetBusyType(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK_SET task_set,
  KHE_FRAME days_frame, KHE_TASK_SET r_ts, bool nocost_off);
*** */

extern int KheResourceTimetableMonitorClashingTimeCount(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm);
extern KHE_TIME KheResourceTimetableMonitorClashingTime(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, int i);

extern int KheResourceTimetableMonitorAtMaxLimitCount(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME t);

extern void KheResourceTimetableMonitorAddRange(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, int first_time_index,
  int last_time_index, bool include_avoid_unavailable_times,
  KHE_GROUP_MONITOR gm);
extern void KheResourceTimetableMonitorAddInterval(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_FRAME frame,
  int first_frame_index, int last_frame_index,
  bool include_avoid_unavailable_times, KHE_GROUP_MONITOR gm);

extern void KheResourceTimetableMonitorPrintTimetable(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_FRAME days_frame,
  int cell_width, int indent, FILE *fp);

extern void KheResourceTimetableMonitorDebug(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, int verbosity, int indent, FILE *fp);
extern void KheResourceTimetableMonitorSetDebug(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg, bool val);

/* 6.9.1 Basic operations on group monitors */
extern KHE_GROUP_MONITOR KheGroupMonitorMake(KHE_SOLN soln, int sub_tag,
  char *sub_tag_label);
extern int KheGroupMonitorSubTag(KHE_GROUP_MONITOR gm);
extern char *KheGroupMonitorSubTagLabel(KHE_GROUP_MONITOR gm);
extern void KheGroupMonitorDelete(KHE_GROUP_MONITOR gm);

extern void KheGroupMonitorAddChildMonitor(KHE_GROUP_MONITOR gm, KHE_MONITOR m);
extern void KheGroupMonitorDeleteChildMonitor(KHE_GROUP_MONITOR gm,
  KHE_MONITOR m);
extern bool KheGroupMonitorHasChildMonitor(KHE_GROUP_MONITOR gm, KHE_MONITOR m);
extern void KheGroupMonitorBypassAndDelete(KHE_GROUP_MONITOR gm);
extern void KheSolnBypassAndDeleteAllGroupMonitors(KHE_SOLN soln);

extern int KheGroupMonitorChildMonitorCount(KHE_GROUP_MONITOR gm);
extern KHE_MONITOR KheGroupMonitorChildMonitor(KHE_GROUP_MONITOR gm, int i);
extern int KheSolnChildMonitorCount(KHE_SOLN soln);
extern KHE_MONITOR KheSolnChildMonitor(KHE_SOLN soln, int i);

extern int KheMonitorParentMonitorCount(KHE_MONITOR m);
extern KHE_GROUP_MONITOR KheMonitorParentMonitor(KHE_MONITOR m, int i);
extern bool KheMonitorDescendant(KHE_MONITOR m1, KHE_MONITOR m2);
extern void KheMonitorParentMonitorsSort(KHE_MONITOR m);

extern void KheGroupMonitorDebug(KHE_GROUP_MONITOR gm,
  int verbosity, int indent, FILE *fp);

/* 6.9.2 Defects */
extern int KheGroupMonitorDefectCount(KHE_GROUP_MONITOR gm);
extern KHE_MONITOR KheGroupMonitorDefect(KHE_GROUP_MONITOR gm, int i);
extern int KheSolnDefectCount(KHE_SOLN soln);
extern KHE_MONITOR KheSolnDefect(KHE_SOLN soln, int i);

extern void KheGroupMonitorDefectDebug(KHE_GROUP_MONITOR gm,
  int verbosity, int indent, FILE *fp);
extern void KheGroupMonitorDefectTypeDebug(KHE_GROUP_MONITOR gm,
  KHE_MONITOR_TAG tag, int verbosity, int indent, FILE *fp);

extern KHE_COST KheGroupMonitorCostByType(KHE_GROUP_MONITOR gm,
  KHE_MONITOR_TAG tag, int *defect_count);
extern KHE_COST KheSolnCostByType(KHE_SOLN soln, KHE_MONITOR_TAG tag,
  int *defect_count);

extern void KheGroupMonitorCostByTypeDebug(KHE_GROUP_MONITOR gm,
  int verbosity, int indent, FILE *fp);
extern void KheSolnCostByTypeDebug(KHE_SOLN soln,
  int verbosity, int indent, FILE *fp);

/* 6.9.3 Tracing */
extern KHE_TRACE KheTraceMake(KHE_GROUP_MONITOR gm);
extern KHE_GROUP_MONITOR KheTraceGroupMonitor(KHE_TRACE t);
extern void KheTraceDelete(KHE_TRACE t);
extern void KheTraceBegin(KHE_TRACE t);
extern void KheTraceEnd(KHE_TRACE t);

extern KHE_COST KheTraceInitCost(KHE_TRACE t);
extern int KheTraceMonitorCount(KHE_TRACE t);
extern KHE_MONITOR KheTraceMonitor(KHE_TRACE t, int i);
extern KHE_COST KheTraceMonitorInitCost(KHE_TRACE t, int i);

extern KHE_COST KheTraceMonitorCostIncrease(KHE_TRACE t, int i);
extern void KheTraceReduceByCostIncrease(KHE_TRACE t, int max_num);

extern void KheTraceDebug(KHE_TRACE t, int verbosity, int indent, FILE *fp);


/*****************************************************************************/
/*                                                                           */
/*    Chapter 7.   Matchings and Evenness                                    */
/*                                                                           */
/*****************************************************************************/

/* 7.2 Basic operations */
extern void KheSolnMatchingBegin(KHE_SOLN soln);
extern void KheSolnMatchingMarkCheck(KHE_SOLN soln);
extern void KheSolnMatchingEnd(KHE_SOLN soln);
extern bool KheSolnHasMatching(KHE_SOLN soln);

extern void KheSolnMatchingSetWeight(KHE_SOLN soln, KHE_COST weight);
extern KHE_COST KheSolnMatchingWeight(KHE_SOLN soln);
/* extern KHE_COST KheSolnMinMatchingWeight(KHE_SOLN soln); */

extern void KheSolnMatchingSetType(KHE_SOLN soln, KHE_MATCHING_TYPE mt);
extern KHE_MATCHING_TYPE KheSolnMatchingType(KHE_SOLN soln);

extern void KheSolnMatchingMarkBegin(KHE_SOLN soln);
extern void KheSolnMatchingMarkEnd(KHE_SOLN soln, bool undo);

extern void KheSolnMatchingDebug(KHE_SOLN soln, int verbosity,
  int indent, FILE *fp);

/* 7.3 Supply nodes and demand nodes */
extern KHE_ORDINARY_DEMAND_MONITOR KheOrdinaryDemandMonitorMake(
  KHE_TASK task, int offset, KHE_MONITOR orig_m);
extern void KheOrdinaryDemandMonitorDelete(KHE_ORDINARY_DEMAND_MONITOR odm);

extern KHE_TASK KheOrdinaryDemandMonitorTask(KHE_ORDINARY_DEMAND_MONITOR odm);
extern int KheOrdinaryDemandMonitorOffset(KHE_ORDINARY_DEMAND_MONITOR odm);
extern KHE_MONITOR KheOrdinaryDemandMonitorOriginatingMonitor(
  KHE_ORDINARY_DEMAND_MONITOR odm);
extern void KheOrdinaryDemandMonitorDebug(KHE_ORDINARY_DEMAND_MONITOR odm,
  int verbosity, int indent, FILE *fp);

extern int KheTaskDemandMonitorCount(KHE_TASK task);
extern KHE_ORDINARY_DEMAND_MONITOR KheTaskDemandMonitor(KHE_TASK task, int i);

extern KHE_WORKLOAD_DEMAND_MONITOR KheWorkloadDemandMonitorMake(
  KHE_SOLN soln, KHE_RESOURCE r, KHE_TIME_GROUP tg, KHE_MONITOR orig_m);
extern void KheWorkloadDemandMonitorDelete(KHE_WORKLOAD_DEMAND_MONITOR wdm);

extern KHE_RESOURCE KheWorkloadDemandMonitorResource(
  KHE_WORKLOAD_DEMAND_MONITOR wdm);
extern KHE_TIME_GROUP KheWorkloadDemandMonitorTimeGroup(
  KHE_WORKLOAD_DEMAND_MONITOR wdm);
extern KHE_MONITOR KheWorkloadDemandMonitorOriginatingMonitor(
  KHE_WORKLOAD_DEMAND_MONITOR wdm);
extern void KheWorkloadDemandMonitorDebug(KHE_WORKLOAD_DEMAND_MONITOR wdm,
  int verbosity, int indent, FILE *fp);

/* *** mistake
extern int KheResourceDemandMonitorCount(KHE_SOLN soln, KHE_RESOURCE r);
extern KHE_ORDINARY_DEMAND_MONITOR KheResourceDemandMonitor(KHE_SOLN soln,
  KHE_RESOURCE r, int i);
*** */

/* *** old interface
extern int KheTaskDemandMonitorCount(KHE_TASK task);
extern KHE_ORDINARY_DEMAND_MONITOR KheTaskDemandMonitor(KHE_TASK task, int i);
extern KHE_TASK KheOrdinaryDemandMonitorTask(KHE_ORDINARY_DEMAND_MONITOR m);
extern int KheOrdinaryDemandMonitorOffset(KHE_ORDINARY_DEMAND_MONITOR m);
extern void KheSolnMatchingAttachAllOrdinaryDemandMonitors(KHE_SOLN soln);
extern void KheSolnMatchingDetachAllOrdinaryDemandMonitors(KHE_SOLN soln);
extern void KheOrdinaryDemandMonitorDebug(KHE_ORDINARY_DEMAND_MONITOR m,
  int verbosity, int indent, FILE *fp);
*** */

/* 7.4 Workload demand nodes */
/* *** old interface
extern void KheSolnMatc hingAddAllWorkloadRequirements(KHE_SOLN soln);
extern int KheSolnMatchingWorkloadRequirementCount(KHE_SOLN soln,
  KHE_RESOURCE r);
extern void KheSolnMatchingWorkloadRequirement(KHE_SOLN soln,
  KHE_RESOURCE r, int i, int *num, KHE_TIME_GROUP *tg, KHE_MONITOR *m);
extern void KheSolnMatchingBeginWorkloadRequirements(KHE_SOLN soln,
  KHE_RESOURCE r);
extern void KheSolnMatchingAddWorkloadRequirement(KHE_SOLN soln,
  KHE_RESOURCE r, int num, KHE_TIME_GROUP tg, KHE_MONITOR m);
extern void KheSolnMatchingEndWorkloadRequirements(KHE_SOLN soln,
  KHE_RESOURCE r);
extern void KheSolnMatchingDeleteWorkloadRequirements(KHE_SOLN soln,
  KHE_RESOURCE r);
extern KHE_RESOURCE KheWorkloadDemandMonitorResource(
  KHE_WORKLOAD_DEMAND_MONITOR m);
extern KHE_TIME_GROUP KheWorkloadDemandMonitorTimeGroup(
  KHE_WORKLOAD_DEMAND_MONITOR m);
extern KHE_MONITOR KheWorkloadDemandMonitorOriginatingMonitor(
  KHE_WORKLOAD_DEMAND_MONITOR m);
extern void KheWorkloadDemandMonitorDebug(KHE_WORKLOAD_DEMAND_MONITOR m,
  int verbosity, int indent, FILE *fp);
*** */

/* 7.4.1 Visiting unmatched demand nodes */
extern int KheSolnMatchingDefectCount(KHE_SOLN soln);
extern KHE_MONITOR KheSolnMatchingDefect(KHE_SOLN soln, int i);

/* 7.4.2 Hall sets */
extern int KheSolnMatchingHallSetCount(KHE_SOLN soln);
extern int KheSolnMatchingHallSetSupplyNodeCount(KHE_SOLN soln, int i);
extern int KheSolnMatchingHallSetDemandNodeCount(KHE_SOLN soln, int i);
extern bool KheSolnMatchingHallSetSupplyNodeIsOrdinary(KHE_SOLN soln,
  int i, int j, KHE_MEET *meet, int *meet_offset, KHE_RESOURCE *r);
extern KHE_MONITOR KheSolnMatchingHallSetDemandNode(KHE_SOLN soln,
  int i, int j);
extern void KheSolnMatchingHallSetsDebug(KHE_SOLN soln,
  int verbosity, int indent, FILE *fp);

/* 7.4.3 Finding competitors */
extern void KheSolnMatchingSetCompetitors(KHE_SOLN soln, KHE_MONITOR m);
extern int KheSolnMatchingCompetitorCount(KHE_SOLN soln);
extern KHE_MONITOR KheSolnMatchingCompetitor(KHE_SOLN soln, int i);

/* 7.5 Evenness monitoring */
extern void KheSolnEvennessBegin(KHE_SOLN soln);
extern void KheSolnEvennessEnd(KHE_SOLN soln);
extern bool KheSolnHasEvenness(KHE_SOLN soln);
extern void KheSolnAttachAllEvennessMonitors(KHE_SOLN soln);
extern void KheSolnDetachAllEvennessMonitors(KHE_SOLN soln);
extern KHE_RESOURCE_GROUP KheEvennessMonitorPartition(KHE_EVENNESS_MONITOR m);
extern KHE_TIME KheEvennessMonitorTime(KHE_EVENNESS_MONITOR m);
extern int KheEvennessMonitorCount(KHE_EVENNESS_MONITOR m);
extern int KheEvennessMonitorLimit(KHE_EVENNESS_MONITOR m);
extern void KheEvennessMonitorSetLimit(KHE_EVENNESS_MONITOR m, int limit);
extern KHE_COST KheEvennessMonitorWeight(KHE_EVENNESS_MONITOR m);
extern void KheEvennessMonitorSetWeight(KHE_EVENNESS_MONITOR m,
  KHE_COST weight);
extern void KheSolnSetAllEvennessMonitorWeights(KHE_SOLN soln, KHE_COST weight);
extern void KheEvennessMonitorDebug(KHE_EVENNESS_MONITOR m,
  int verbosity, int indent, FILE *fp);

/* 7.6 Redundancy monitoring */
extern void KheSolnRedundancyBegin(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_COST_FUNCTION cf, KHE_COST combined_weight);
extern void KheSolnRedundancyEnd(KHE_SOLN soln);
extern bool KheSolnHasRedundancy(KHE_SOLN soln);
extern void KheSolnAttachAllRedundancyMonitors(KHE_SOLN soln);
extern void KheSolnDetachAllRedundancyMonitors(KHE_SOLN soln);

#endif

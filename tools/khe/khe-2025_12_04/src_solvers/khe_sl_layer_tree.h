
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
/*  FILE:         khe_sl_layer_tree.h                                        */
/*  DESCRIPTION:  Header file for KheLayerTreeMake() internals               */
/*                                                                           */
/*****************************************************************************/
#include <limits.h>
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"
#include "khe_partition.h"

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))


/*****************************************************************************/
/*                                                                           */
/*   Typedefs                                                                */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_EVENT) ARRAY_KHE_EVENT;
typedef HA_ARRAY(KHE_MEET) ARRAY_KHE_MEET;
/* typedef HA_ARRAY(KHE_LAYER) ARRAY_KHE_LAYER; */
typedef HA_ARRAY(KHE_SPLIT_EVENTS_CONSTRAINT) ARRAY_KHE_SPLIT_EVENTS_CONSTRAINT;
typedef HA_ARRAY(KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT)
  ARRAY_KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT;
typedef HA_ARRAY(KHE_SPREAD_EVENTS_CONSTRAINT)
  ARRAY_KHE_SPREAD_EVENTS_CONSTRAINT;
typedef HA_ARRAY(KHE_LINK_EVENTS_CONSTRAINT) ARRAY_KHE_LINK_EVENTS_CONSTRAINT;
typedef HA_ARRAY(KHE_TIME_GROUP) ARRAY_KHE_TIME_GROUP;

typedef struct khe_split_forest_rec *KHE_SPLIT_FOREST;

typedef struct khe_split_class_rec *KHE_SPLIT_CLASS;
typedef HA_ARRAY(KHE_SPLIT_CLASS) ARRAY_KHE_SPLIT_CLASS;

typedef struct khe_split_layer_rec *KHE_SPLIT_LAYER;
typedef HA_ARRAY(KHE_SPLIT_LAYER) ARRAY_KHE_SPLIT_LAYER;

/* ***
typedef struct khe_split_assigner_rec *KHE_SPLIT_ASSIGNER;
*** */

typedef enum {
  /* KHE_LAYER_SPLIT_JOB_TAG, */
  KHE_AVOID_CLASHES_SPLIT_JOB_TAG,
  KHE_PACK_SPLIT_JOB_TAG,
  KHE_PREASSIGNED_SPLIT_JOB_TAG,
  KHE_ASSIGNED_SPLIT_JOB_TAG,
  KHE_LINK_SPLIT_JOB_TAG,
  KHE_SPLIT_SPLIT_JOB_TAG,
  KHE_DISTRIBUTE_SPLIT_JOB_TAG,
  KHE_SPREAD_SPLIT_JOB_TAG,
  KHE_DOMAIN_SPLIT_JOB_TAG
} KHE_SPLIT_JOB_TAG;

typedef struct khe_split_job_rec *KHE_SPLIT_JOB;
typedef HA_ARRAY(KHE_SPLIT_JOB) ARRAY_KHE_SPLIT_JOB;

/* typedef struct khe_layer_split_job_rec *KHE_LAYER_SPLIT_JOB; */
typedef struct khe_avoid_clashes_split_job_rec *KHE_AVOID_CLASHES_SPLIT_JOB;
typedef struct khe_pack_split_job_rec *KHE_PACK_SPLIT_JOB;
typedef struct khe_preassigned_split_job_rec *KHE_PREASSIGNED_SPLIT_JOB;
typedef struct khe_assigned_split_job_rec *KHE_ASSIGNED_SPLIT_JOB;
typedef struct khe_link_split_job_rec *KHE_LINK_SPLIT_JOB;
typedef struct khe_split_split_job_rec *KHE_SPLIT_SPLIT_JOB;
typedef struct khe_distribute_split_job_rec *KHE_DISTRIBUTE_SPLIT_JOB;
typedef struct khe_spread_split_job_rec *KHE_SPREAD_SPLIT_JOB;
typedef struct khe_domain_split_job_rec *KHE_DOMAIN_SPLIT_JOB;


/*****************************************************************************/
/*                                                                           */
/*  Split forests                                                            */
/*                                                                           */
/*****************************************************************************/

/* construction and query */
extern KHE_SPLIT_FOREST KheSplitForestMake(KHE_SOLN soln, HA_ARENA a);
/* extern void KheSplitForestFree(KHE_SPLIT_FOREST sf); */
extern KHE_SOLN KheSplitForestSoln(KHE_SPLIT_FOREST sf);

/* trials */
/* ***
extern bool KheSplitForestTryAddToLayer(KHE_SPLIT_FOREST sf,
  KHE_EVENT e, KHE_LAYER layer);
*** */
extern bool KheSplitForestTryAddToResource(KHE_SPLIT_FOREST sf,
  KHE_EVENT e, KHE_RESOURCE r, HA_ARENA a);
extern bool KheSplitForestTryTotalAmount(KHE_SPLIT_FOREST sf,
  KHE_EVENT e, int minimum, int maximum, HA_ARENA a);
extern bool KheSplitForestTryDurnAmount(KHE_SPLIT_FOREST sf,
  KHE_EVENT e, int durn, int minimum, int maximum, HA_ARENA a);
extern bool KheSplitForestTryEventMerge(KHE_SPLIT_FOREST sf,
  KHE_EVENT e1, KHE_EVENT e2, HA_ARENA a);
extern bool KheSplitForestTryEventAssign(KHE_SPLIT_FOREST sf,
  KHE_EVENT e1, KHE_EVENT e2, HA_ARENA a);
extern bool KheSplitForestTryPackableInto(KHE_SPLIT_FOREST sf,
  KHE_EVENT e, KHE_PARTITION p, HA_ARENA a);
extern bool KheSplitForestTryEventDomain(KHE_SPLIT_FOREST sf,
  KHE_EVENT e, KHE_TIME_GROUP tg, int durn, HA_ARENA a);
extern bool KheSplitForestTryPreassignedTime(KHE_SPLIT_FOREST sf,
  KHE_EVENT e, KHE_TIME t, HA_ARENA a);
extern bool KheSplitForestTrySpread(KHE_SPLIT_FOREST sf,
  KHE_EVENT_GROUP eg, int min_amount, int max_amount, HA_ARENA a);

/* finalize */
extern KHE_NODE KheSplitForestFinalize(KHE_SPLIT_FOREST sf, HA_ARENA a);

/* debug */
extern void KheSplitForestDebug(KHE_SPLIT_FOREST sf, int indent, FILE *fp,
  HA_ARENA a);


/*****************************************************************************/
/*                                                                           */
/*  Split classes                                                            */
/*                                                                           */
/*****************************************************************************/

/* construction and query */
extern KHE_SPLIT_CLASS KheSplitClassMake(KHE_SPLIT_FOREST sf,
  int id_num, KHE_EVENT e, HA_ARENA a);
/* ***
extern KHE_SPLIT_CLASS KheSplitClassCycleLayerMake(KHE_SPLIT_FOREST sf,
  int id_num, KHE_LAYER cycle_layer);
extern void KheSplitClassFree(KHE_SPLIT_CLASS sc, bool free_child_classes);
*** */
extern KHE_SPLIT_FOREST KheSplitClassForest(KHE_SPLIT_CLASS sc);

/* parent class */
extern KHE_SPLIT_CLASS KheSplitClassParentClass(KHE_SPLIT_CLASS sc);
extern bool KheSplitClassAncestor(KHE_SPLIT_CLASS sc, KHE_SPLIT_CLASS anc_sc);

/* child classes */
extern int KheSplitClassChildClassCount(KHE_SPLIT_CLASS sc);
extern KHE_SPLIT_CLASS KheSplitClassChildClass(KHE_SPLIT_CLASS sc, int i);
extern void KheSplitClassAddChildClass(KHE_SPLIT_CLASS sc,
  KHE_SPLIT_CLASS child_sc, HA_ARENA a);
extern void KheSplitClassDeleteChildClass(KHE_SPLIT_CLASS sc,
  KHE_SPLIT_CLASS child_sc);

/* local requirements */
extern int KheSplitClassMinTotalAmount(KHE_SPLIT_CLASS sc);
extern int KheSplitClassMaxTotalAmount(KHE_SPLIT_CLASS sc);
extern KHE_PARTITION KheSplitClassMinPartition(KHE_SPLIT_CLASS sc);

/* events */
extern int KheSplitClassEventCount(KHE_SPLIT_CLASS sc);
extern KHE_EVENT KheSplitClassEvent(KHE_SPLIT_CLASS sc, int i);
extern bool KheSplitClassContainsEvent(KHE_SPLIT_CLASS sc, KHE_EVENT e);
extern int KheSplitClassDuration(KHE_SPLIT_CLASS sc);

/* trials */
extern void KheSplitClassTryBegin(KHE_SPLIT_CLASS sc);
/* ***
extern void KheSplitClassTryAddToLayer(KHE_SPLIT_CLASS sc, KHE_LAYER layer);
*** */
extern void KheSplitClassTryAddToResource(KHE_SPLIT_CLASS sc, KHE_RESOURCE r,
  HA_ARENA a);
extern void KheSplitClassTryEventDomain(KHE_SPLIT_CLASS sc,
  KHE_TIME_GROUP tg, int durn);
extern void KheSplitClassTryTotalAmount(KHE_SPLIT_CLASS sc,
  int min_total_amount, int max_total_amount);
extern void KheSplitClassTryDurnAmount(KHE_SPLIT_CLASS sc, int durn,
  int min_amount, int max_amount);
extern void KheSplitClassTryUnsplittable(KHE_SPLIT_CLASS sc);
extern void KheSplitClassTryPackableInto(KHE_SPLIT_CLASS sc, KHE_PARTITION p);
extern void KheSplitClassTryMerge(KHE_SPLIT_CLASS dst_sc,
  KHE_SPLIT_CLASS src_sc);
extern void KheSplitClassTryAssign(KHE_SPLIT_CLASS child_sc,
  KHE_SPLIT_CLASS prnt_sc, HA_ARENA a);
extern bool KheSplitClassTryAlive(KHE_SPLIT_CLASS sc, HA_ARENA a);
extern bool KheSplitClassTryEnd(KHE_SPLIT_CLASS sc, bool success);

/* finalize */
/* ***
extern void KheSplitClassFinalize(KHE_SPLIT_CLASS sc, KHE_SPLIT_ASSIGNER sa);
*** */
extern KHE_NODE KheSplitClassFinalize(KHE_SPLIT_CLASS sc, KHE_NODE parent_node,
  HA_ARENA a);

/* debug */
void KheSplitClassDebugBrief(KHE_SPLIT_CLASS sc, FILE *fp);
void KheSplitClassDebug(KHE_SPLIT_CLASS sc, int indent, FILE *fp, HA_ARENA a);


/*****************************************************************************/
/*                                                                           */
/*  Split layers                                                             */
/*                                                                           */
/*****************************************************************************/

/* construction and query */
/* ***
extern KHE_SPLIT_LAYER KheSplitLayerMake(KHE_LAYER layer, KHE_PARTITION p);
extern KHE_LAYER KheSplitLayerLayer(KHE_SPLIT_LAYER sl);
*** */
extern KHE_SPLIT_LAYER KheSplitLayerMake(/* KHE_LAYER layer */ KHE_RESOURCE r,
  KHE_PARTITION p, HA_ARENA a);
extern KHE_RESOURCE KheSplitLayerResource(KHE_SPLIT_LAYER sl);
/* extern void KheSplitLayerFree(KHE_SPLIT_LAYER sl); */

/* child classes */
extern void KheSplitLayerAddSplitClass(KHE_SPLIT_LAYER sl,
  KHE_SPLIT_CLASS sc, HA_ARENA a);
extern void KheSplitLayerDeleteSplitClass(KHE_SPLIT_LAYER sl,
  KHE_SPLIT_CLASS sc);
extern bool KheSplitLayerContainsSplitClass(KHE_SPLIT_LAYER sl,
  KHE_SPLIT_CLASS sc);
extern int KheSplitLayerSplitClassCount(KHE_SPLIT_LAYER sl);
extern KHE_SPLIT_CLASS KheSplitLayerSplitClass(KHE_SPLIT_LAYER sl, int i);

/* partition */
extern KHE_PARTITION KheSplitLayerMinPartition(KHE_SPLIT_LAYER sl);

/* trials */
extern void KheSplitLayerTryBegin(KHE_SPLIT_LAYER sl);
extern void KheSplitLayerTryMinPartitionChange(KHE_SPLIT_LAYER sl,
  KHE_PARTITION old_p, KHE_PARTITION new_p);
extern void KheSplitLayerTryMerge(KHE_SPLIT_LAYER dest_sl,
  KHE_SPLIT_LAYER src_sl);
extern void KheSplitLayerTryEnd(KHE_SPLIT_LAYER sl, bool success);

/* debug */
extern void KheSplitLayerDebugBrief(KHE_SPLIT_LAYER sl, int indent, FILE *fp,
  HA_ARENA a);
extern void KheSplitLayerDebug(KHE_SPLIT_LAYER sl, int indent, FILE *fp,
  HA_ARENA a);


/*****************************************************************************/
/*                                                                           */
/*  Split assigners                                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
extern KHE_SPLIT_ASSIGNER KheSplitAssignerMake(KHE_SOLN soln);
extern void KheSplitAssignerBegin(KHE_SPLIT_ASSIGNER sa);
extern void KheSplitAssignerAddChildSolnEvent(KHE_SPLIT_ASSIGNER sa,
  KHE_MEET child_meet);
extern void KheSplitAssignerAddParentSolnEvent(KHE_SPLIT_ASSIGNER sa,
  KHE_MEET prnt_meet);
extern bool KheSplitAssignerEnd(KHE_SPLIT_ASSIGNER sa);
extern void KheSplitAssignerFree(KHE_SPLIT_ASSIGNER sa);
*** */


/*****************************************************************************/
/*                                                                           */
/*  Split jobs                                                               */
/*                                                                           */
/*****************************************************************************/

extern KHE_SPLIT_JOB_TAG KheSplitJobTag(KHE_SPLIT_JOB st);
extern KHE_COST KheSplitJobPriority(KHE_SPLIT_JOB st);
extern int KheSplitJobDecreasingPriorityCmp(const void *t1, const void *t2);
extern void KheSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs, int start,
  int stop, KHE_SPLIT_FOREST sf, HA_ARENA a);
/* extern void KheSplitJobFree(KHE_SPLIT_JOB st); */
extern void KheSplitJobDebug(KHE_SPLIT_JOB st, int indent, FILE *fp,
  HA_ARENA a);


/*****************************************************************************/
/*                                                                           */
/*  Layer split jobs                                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
extern KHE_LAYER_SPLIT_JOB KheLayerSplitJobMake(KHE_LAYER layer);
extern void KheLayerSplitJobFree(KHE_LAYER_SPLIT_JOB st);
extern void KheLayerSplitJobTry(KHE_LAYER_SPLIT_JOB st, KHE_SPLIT_FOREST sf);
extern void KheLayerSplitJobDebug(KHE_LAYER_SPLIT_JOB st,
  int indent, FILE *fp);
*** */


/*****************************************************************************/
/*                                                                           */
/*  Avoid clashes split jobs                                                 */
/*                                                                           */
/*****************************************************************************/

extern KHE_AVOID_CLASHES_SPLIT_JOB KheAvoidClashesSplitJobMake(
  KHE_AVOID_CLASHES_CONSTRAINT c, HA_ARENA a);
/* extern void KheAvoidClashesSplitJobFree(KHE_AVOID_CLASHES_SPLIT_JOB st); */
extern void KheAvoidClashesSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,
  int start, int stop, KHE_SPLIT_FOREST sf, HA_ARENA a);
extern void KheAvoidClashesSplitJobDebug(KHE_AVOID_CLASHES_SPLIT_JOB st,
  int indent, FILE *fp, HA_ARENA a);


/*****************************************************************************/
/*                                                                           */
/*  Pack split jobs                                                          */
/*                                                                           */
/*****************************************************************************/

extern KHE_PACK_SPLIT_JOB KhePackSplitJobMake(KHE_EVENT e, KHE_PARTITION p,
  HA_ARENA a);
/* extern void KhePackSplitJobFree(KHE_PACK_SPLIT_JOB st); */
extern void KhePackSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,
  int start, int stop, KHE_SPLIT_FOREST sf, HA_ARENA a);
extern void KhePackSplitJobDebug(KHE_PACK_SPLIT_JOB st, int indent, FILE *fp,
  HA_ARENA a);


/*****************************************************************************/
/*                                                                           */
/*  Preassigned split jobs                                                   */
/*                                                                           */
/*****************************************************************************/

extern KHE_PREASSIGNED_SPLIT_JOB KhePreassignedSplitJobMake(KHE_EVENT e,
  HA_ARENA a);
/* extern void KhePreassignedSplitJobFree(KHE_PREASSIGNED_SPLIT_JOB st); */
extern void KhePreassignedSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,
  int start, int stop, KHE_SPLIT_FOREST sf, HA_ARENA a);
extern void KhePreassignedSplitJobDebug(KHE_PREASSIGNED_SPLIT_JOB st,
  int indent, FILE *fp, HA_ARENA a);


/*****************************************************************************/
/*                                                                           */
/*  Assigned split jobs                                                      */
/*                                                                           */
/*****************************************************************************/

extern KHE_ASSIGNED_SPLIT_JOB KheAssignedSplitJobMake(KHE_EVENT child_event,
  KHE_EVENT prnt_event, HA_ARENA a);
/* extern void KheAssignedSplitJobFree(KHE_ASSIGNED_SPLIT_JOB st); */
extern void KheAssignedSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,
  int start, int stop, KHE_SPLIT_FOREST sf, HA_ARENA a);
extern void KheAssignedSplitJobDebug(KHE_ASSIGNED_SPLIT_JOB st,
  int indent, FILE *fp, HA_ARENA a);


/*****************************************************************************/
/*                                                                           */
/*  Link split jobs                                                          */
/*                                                                           */
/*****************************************************************************/

extern KHE_LINK_SPLIT_JOB KheLinkSplitJobMake(KHE_LINK_EVENTS_CONSTRAINT c,
  HA_ARENA a);
/* extern void KheLinkSplitJobFree(KHE_LINK_SPLIT_JOB st); */
extern void KheLinkSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,
  int start, int stop, KHE_SPLIT_FOREST sf, HA_ARENA a);
extern void KheLinkSplitJobDebug(KHE_LINK_SPLIT_JOB st,
  int indent, FILE *fp, HA_ARENA a);


/*****************************************************************************/
/*                                                                           */
/*  Split split jobs                                                         */
/*                                                                           */
/*****************************************************************************/

extern KHE_SPLIT_SPLIT_JOB KheSplitSplitJobMake(
  KHE_SPLIT_EVENTS_CONSTRAINT c, HA_ARENA a);
/* extern void KheSplitSplitJobFree(KHE_SPLIT_SPLIT_JOB st); */
extern void KheSplitSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,
  int start, int stop, KHE_SPLIT_FOREST sf, HA_ARENA a);
extern void KheSplitSplitJobDebug(KHE_SPLIT_SPLIT_JOB st,
  int indent, FILE *fp, HA_ARENA a);


/*****************************************************************************/
/*                                                                           */
/*  Distribute split jobs                                                    */
/*                                                                           */
/*****************************************************************************/

extern KHE_DISTRIBUTE_SPLIT_JOB KheDistributeSplitJobMake(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, HA_ARENA a);
/* extern void KheDistributeSplitJobFree(KHE_DISTRIBUTE_SPLIT_JOB st); */
extern void KheDistributeSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,
  int start, int stop, KHE_SPLIT_FOREST sf, HA_ARENA a);
extern void KheDistributeSplitJobDebug(KHE_DISTRIBUTE_SPLIT_JOB st,
  int indent, FILE *fp, HA_ARENA a);


/*****************************************************************************/
/*                                                                           */
/*  Spread split jobs                                                        */
/*                                                                           */
/*****************************************************************************/

extern KHE_SPREAD_SPLIT_JOB KheSpreadSplitJobMake(
  KHE_SPREAD_EVENTS_CONSTRAINT c, HA_ARENA a);
/* extern void KheSpreadSplitJobFree(KHE_SPREAD_SPLIT_JOB st); */
extern void KheSpreadSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,
  int start, int stop, KHE_SPLIT_FOREST sf, HA_ARENA a);
extern void KheSpreadSplitJobDebug(KHE_SPREAD_SPLIT_JOB st,
  int indent, FILE *fp, HA_ARENA a);


/*****************************************************************************/
/*                                                                           */
/*  Domain split jobs                                                        */
/*                                                                           */
/*****************************************************************************/

extern KHE_DOMAIN_SPLIT_JOB KheDomainSplitJobMake(
  KHE_PREFER_TIMES_CONSTRAINT c, HA_ARENA a);
/* extern void KheDomainSplitJobFree(KHE_DOMAIN_SPLIT_JOB st); */
extern void KheDomainSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,
  int start, int stop, KHE_SPLIT_FOREST sf, HA_ARENA a);
extern void KheDomainSplitJobDebug(KHE_DOMAIN_SPLIT_JOB st,
  int indent, FILE *fp, HA_ARENA a);

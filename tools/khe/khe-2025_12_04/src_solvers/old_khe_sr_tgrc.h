
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
/*  FILE:         khe_sr_tgrc.h                                              */
/*  DESCRIPTION:  Task grouping by resource constraints - header file        */
/*                                                                           */
/*****************************************************************************/
#ifndef KHE_SR_TGRC_HEADER_FILE
#define KHE_SR_TGRC_HEADER_FILE
#include "howard_a.h"

/* types */
/* typedef struct khe_mtask_grouping_rec *KHE_MTASK_GROUP; */

typedef struct khe_comb_grouper_rec *KHE_COMB_GROUPER;

typedef enum {
  KHE_COMB_COVER_YES,
  KHE_COMB_COVER_NO,
  KHE_COMB_COVER_PREV,
  KHE_COMB_COVER_FREE
} KHE_COMB_COVER_TYPE;

typedef HA_ARRAY(KHE_COMB_COVER_TYPE) ARRAY_KHE_COMB_COVER_TYPE;

typedef enum {
  KHE_COMB_VARIANT_MIN,
  KHE_COMB_VARIANT_ZERO,
  KHE_COMB_VARIANT_SOLE_ZERO,
  KHE_COMB_VARIANT_SINGLES
} KHE_COMB_VARIANT_TYPE;

/* khe_sr_tgrc_group.c - construction and query */
/* ***
extern KHE_MTASK_GROUP KheMTaskGroupMake(KHE_COMB_GROUPER cg);
extern void KheMTaskGroupDelete(KHE_MTASK_GROUP mg);
extern void KheMTaskGroupClear(KHE_MTASK_GROUP mg);
extern void KheMTaskGroupOverwrite(KHE_MTASK_GROUP dst_mg,
  KHE_MTASK_GROUP src_mg);
extern int KheMTaskGroupMTaskCount(KHE_MTASK_GROUP mg);
extern KHE_MTASK KheMTaskGroupMTask(KHE_MTASK_GROUP mg, int i);
extern bool KheMTaskGroupIsEmpty(KHE_MTASK_GROUP mg);
extern bool KheMTaskGroupAddMTask(KHE_MTASK_GROUP mg, KHE_MTASK mt);
extern void KheMTaskGroupDeleteMTask(KHE_MTASK_GROUP mg, KHE_MTASK mt);
extern bool KheMTaskGroupContainsMTask(KHE_MTASK_GROUP mg, KHE_MTASK mt);
*** */

/* khe_sr_tgrc_group.c - cost and execution */
/* ***
extern bool KheMTaskGroupHasCost(KHE_MTASK_GROUP mg, KHE_COST *cost);
extern bool KheMTaskGroupIsBetter(KHE_MTASK_GROUP new_mg,
  KHE_MTASK_GROUP old_mg);
extern int KheMTaskGroupExecute(KHE_MTASK_GROUP mg, int max_num,
  KHE_SOLN_ADJUSTER sa, char *debug_str);

extern void KheMTaskGroupDebug(KHE_MTASK_GROUP mg,
  int verbosity, int indent, FILE *fp);
*** */

/* khe_sr_tgrc_comb.c - combinatorial grouper construction and query */
extern KHE_COMB_GROUPER KheCombGrouperMake(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt, HA_ARENA a);
extern KHE_MTASK_FINDER KheCombGrouperMTaskFinder(KHE_COMB_GROUPER cg);
extern KHE_SOLN KheCombGrouperSoln(KHE_COMB_GROUPER cg);
extern KHE_RESOURCE_TYPE KheCombGrouperResourceType(KHE_COMB_GROUPER cg);
extern HA_ARENA KheCombGrouperArena(KHE_COMB_GROUPER cg);

/* khe_sr_tgrc_comb.c - mtask group free list */
/* ***
extern KHE_MTASK_GROUP KheCombGrouperGetMTaskGroup(KHE_COMB_GROUPER cg);
extern void KheCombGrouperPutMTaskGroup(KHE_COMB_GROUPER cg,
  KHE_MTASK_GROUP mg);
*** */

/* khe_sr_tgrc_comb.c - loading requirements */
extern void KheCombGrouperClearRequirements(KHE_COMB_GROUPER cg);
extern void KheCombGrouperAddTimeGroupRequirement(KHE_COMB_GROUPER cg,
  KHE_TIME_GROUP tg, KHE_COMB_COVER_TYPE cover);
extern void KheCombGrouperDeleteTimeGroupRequirement(KHE_COMB_GROUPER cg,
  KHE_TIME_GROUP tg);
extern void KheCombGrouperAddMTaskRequirement(KHE_COMB_GROUPER cg,
  KHE_MTASK mt, KHE_COMB_COVER_TYPE cover);
extern void KheCombGrouperDeleteMTaskRequirement(KHE_COMB_GROUPER cg,
  KHE_MTASK mt);
extern void KheCombGrouperAddNoSinglesRequirement(KHE_COMB_GROUPER cg);
extern void KheCombGrouperDeleteNoSinglesRequirement(KHE_COMB_GROUPER cg);
extern void KheCombGrouperAddPreferredDomainRequirement(KHE_COMB_GROUPER cg,
  KHE_RESOURCE_GROUP rg);
extern void KheCombGrouperDeletePreferredDomainRequirement(KHE_COMB_GROUPER cg);

/* khe_sr_tgrc_comb.c - solving and debug */
extern bool KheCombGrouperSolve(KHE_COMB_GROUPER cg,
  KHE_COMB_VARIANT_TYPE cg_variant, KHE_MTASK_GROUPER mtg);
extern int KheCombGrouperSolveSingles(KHE_COMB_GROUPER cg);
extern void KheCombGrouperDebug(KHE_COMB_GROUPER cg, int verbosity,
  int indent, FILE *fp);

/* khe_sr_tgrc_elim.c- combination elimination */
extern void KheCombinationElimination(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt, KHE_FRAME days_frame,
  KHE_EVENT_TIMETABLE_MONITOR etm,
  ARRAY_KHE_COMB_COVER_TYPE *cover_types);

/* khe_sr_tgrc_interval_grouping.c - interval grouping by dynamic programming */
/* ***
extern int KheintervalGrouping(KHE_MTASK_FINDER mtf, KHE_RESOURCE_TYPE rt,
  KHE_SOLN_ADJUSTER sa);
*** */
#endif

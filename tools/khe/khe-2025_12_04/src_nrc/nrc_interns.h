
/*****************************************************************************/
/*                                                                           */
/*  THE NRCONV NURSE ROSTERING TO XESTT CONVERTER                            */
/*  COPYRIGHT (C) 2016, Jeffrey H. Kingston                                  */
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
/*  FILE:         nrc_interns.h                                              */
/*  MODULE:       Declarations internal to NRC.  Do not include this file.   */
/*                                                                           */
/*****************************************************************************/
#include "nrc.h"
#include "khe_platform.h"
#include "howard_a.h"
#include "howard_n.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>

#define is_letter(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
#define is_digit(c) ((c) >= '0' && (c) <= '9')
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define ceil(a, b) (((a) + (b) - 1)/(b))

#define WORKER_SET_ALL_NAME "RG:All"
#define WORKER_SET_EMPTY_NAME "RG:Empty"

/* ***
typedef enum {
  NRC_UNIFORMITY_UNKNOWN,
  NRC_UNIFORMITY_NONE,
  NRC_UNIFORMITY_NOT_LR,
  NRC_UNIFORMITY_NOT_R,
  NRC_UNIFORMITY_FULL,
} NRC_UNIFORMITY;
*** */

typedef HA_ARRAY(NRC_CONSTRAINT) ARRAY_NRC_CONSTRAINT;

typedef struct nrc_dc_converter_rec *NRC_DC_CONVERTER;
typedef struct nrc_constraint_condenser_rec *NRC_CONSTRAINT_CONDENSER;
typedef struct nrc_condensed_constraint_rec *NRC_CONDENSED_CONSTRAINT;

/* type declarations */
/* ***
typedef enum {
  NRC_ROLE_UNDER,
  NRC_ROLE_OPT,
  NRC_ROLE_OVER
} NRC_ROLE_TYPE;

typedef struct nrc_role_rec *NRC_ROLE;
typedef struct nrc_role_set_rec *NRC_ROLE_SET;
*** */

/* ***
typedef struct nrc_skill_set_rec *NRC_SKILL_SET;
typedef struct nrc_skill_rec *NRC_SKILL;
*** */

/* nrc_archive.c */
extern HA_ARENA NrcArchiveArenaBegin(NRC_ARCHIVE archive);
extern void NrcArchiveArenaEnd(NRC_ARCHIVE archive, HA_ARENA a);
extern void NrcArchiveAddSolnGroup(NRC_ARCHIVE archive,
  NRC_SOLN_GROUP soln_group);

/* nrc_soln_group.c */
extern void NrcSolnGroupDeleteSolnsForInstance(NRC_SOLN_GROUP soln_group,
  NRC_INSTANCE ins);
extern void NrcSolnGroupConvert(NRC_SOLN_GROUP soln_group, KHE_ARCHIVE archive);

/* nrc_soln.c */
extern void NrcSolnAddSolnGroup(NRC_SOLN soln, NRC_SOLN_GROUP soln_group);
extern void NrcSolnDeleteSolnGroup(NRC_SOLN soln, NRC_SOLN_GROUP soln_group);
extern KHE_SOLN NrcSolnConvert(NRC_SOLN soln);

/* nrc_instance.c */
extern bool NrcInstanceComplete(NRC_INSTANCE ins);
extern HA_ARENA NrcInstanceArena(NRC_INSTANCE ins);
extern char *NrcInstanceWorkerWord(NRC_INSTANCE ins);
extern void NrcInstanceAddArchive(NRC_INSTANCE ins, NRC_ARCHIVE archive);
extern void NrcInstanceDeleteArchive(NRC_INSTANCE ins, NRC_ARCHIVE archive);

extern void NrcInstanceAddDaySet(NRC_INSTANCE ins, NRC_DAY_SET ds);
extern void NrcInstanceAddDaySetSet(NRC_INSTANCE ins, NRC_DAY_SET_SET dss);
extern int NrcInstanceAddShiftType(NRC_INSTANCE ins, NRC_SHIFT_TYPE st);
extern void NrcInstanceAddShiftTypeSet(NRC_INSTANCE ins,NRC_SHIFT_TYPE_SET sts);
extern int NrcInstanceAddShift(NRC_INSTANCE ins, NRC_SHIFT s);
extern void NrcInstanceAddShiftSet(NRC_INSTANCE ins, NRC_SHIFT_SET ss);
extern void NrcInstanceAddShiftSetSet(NRC_INSTANCE ins, NRC_SHIFT_SET_SET sss);
extern void NrcInstanceStaffingAddWorker(NRC_INSTANCE ins, NRC_WORKER w);
extern void NrcInstanceAddWorkerSet(NRC_INSTANCE ins, NRC_WORKER_SET ws);
extern void NrcInstanceAddWorkerSetSet(NRC_INSTANCE ins,NRC_WORKER_SET_SET wss);
/* extern int NrcInstanceAddCover(NRC_INSTANCE ins, NRC_COVER c); */
/* extern void NrcInstanceAddCoverSet(NRC_INSTANCE ins, NRC_COVER_SET cs); */
extern bool NrcInstanceRetrieveDemand(NRC_INSTANCE ins, char *name,
  NRC_DEMAND *d);
extern void NrcInstanceAddDemand(NRC_INSTANCE ins, char *name, NRC_DEMAND d);
extern void NrcInstanceAddDemandSet(NRC_INSTANCE ins, NRC_DEMAND_SET ds);
extern void NrcInstanceAddDemandConstraint(NRC_INSTANCE ins,
  NRC_DEMAND_CONSTRAINT c);
extern int NrcInstanceAddPattern(NRC_INSTANCE ins, NRC_PATTERN p);
extern int NrcInstanceAddPatternSet(NRC_INSTANCE ins, NRC_PATTERN_SET ps);
extern void NrcInstanceAddConstraint(NRC_INSTANCE ins, NRC_CONSTRAINT c);
extern bool NrcInstanceHasWorkloadConstraint(NRC_INSTANCE ins);

/* ***
extern bool NrcInstanceRetrieveRole(NRC_INSTANCE ins, char *name, NRC_ROLE *r);
extern void NrcInstanceInsertRole(NRC_INSTANCE ins, char *name, NRC_ROLE r);
*** */

extern int NrcInstanceTotalWorkload(NRC_INSTANCE ins);

extern KHE_INSTANCE NrcInstanceKheInstance(NRC_INSTANCE ins);
extern KHE_RESOURCE_GROUP NrcInstanceEmptyResourceGroup(NRC_INSTANCE ins);
extern KHE_EVENT_GROUP NrcInstanceAllEventGroup(NRC_INSTANCE ins);
extern KHE_INSTANCE NrcInstanceConvert(NRC_INSTANCE ins);
/* ***
extern NRC_ROLE NrcInstanceGetRole(NRC_INSTANCE ins, int counter,
  NRC_SHIFT_TYPE st, NRC_WORKER_SET skill_ws, bool reqd, int weight);
*** */
extern bool NrcInstanceUniqueSkillPenalty(NRC_INSTANCE ins, NRC_PENALTY *p);

/* nrc_day.c */
extern NRC_DAY NrcDayMake(NRC_INSTANCE ins, char *ymd, int index_in_cycle,
  int index_in_week, int week_in_cycle, NRC_DAY_SET day_of_week);
/* ***
extern void NrcDayAddDayTimeGroup(NRC_DAY d, KHE_TIME_GROUP day_tg);
*** */
extern KHE_TIME_GROUP NrcDayTimeGroup(NRC_DAY d);
extern void NrcDayAddShift(NRC_DAY d, NRC_SHIFT s);

extern void NrcDayAddTimeIntervalShiftSet(NRC_DAY d, NRC_TIME_INTERVAL ti,
  NRC_SHIFT_SET ss);
extern bool NrcDayRetrieveTimeIntervalShiftSet(NRC_DAY d, NRC_TIME_INTERVAL ti,
  NRC_SHIFT_SET *ss);

/* nrc_day_set.c */
extern NRC_DAY_SET NrcDaySetMakeCycle(NRC_INSTANCE ins, int day_count,
  int first_day_index, NRC_DAY_SET_SET *days_of_week);
extern NRC_DAY_SET NrcDaySetMakeCalendarCycle(NRC_INSTANCE ins, char *start_ymd,
  char *end_ymd, NRC_DAY_SET_SET *days_of_week, char **err_str);

/* nrc_shift_type.c */
extern void NrcShiftTypeAddShift(NRC_SHIFT_TYPE st, NRC_SHIFT s);

/* nrc_shift.c */
extern int NrcShiftIndex(NRC_SHIFT s);
extern int NrcShiftTypedCmp(NRC_SHIFT s1, NRC_SHIFT s2);
extern int NrcShiftCmp(const void *p1, const void *p2);
extern NRC_SHIFT NrcShiftMake(NRC_INSTANCE ins, NRC_DAY day, NRC_SHIFT_TYPE st);
extern void NrcShiftAddDemandConstraint(NRC_SHIFT s, NRC_DEMAND_CONSTRAINT dc);
extern int NrcShiftDemandConstraintCount(NRC_SHIFT s);
extern void NrcShiftReplaceDemandConstraintsByDemands(NRC_SHIFT s,
  NRC_DC_CONVERTER dcc);
extern void NrcShiftConvertToTime(NRC_SHIFT s, KHE_INSTANCE ins);
extern KHE_TIME NrcShiftTime(NRC_SHIFT s);
extern void NrcShiftConvertToEvent(NRC_SHIFT s, KHE_INSTANCE ins);
extern KHE_EVENT NrcShiftEvent(NRC_SHIFT s);

/* nrc_shift_set.c */
extern NRC_SHIFT_SET NrcShiftSetMakeInternal(NRC_INSTANCE ins,
  KHE_TIME_GROUP_KIND kind);
extern bool NrcShiftSetMakeShifted(NRC_SHIFT_SET ss, int delta,
  NRC_SHIFT_SET *res);
extern void NrcShiftSetSortShifts(NRC_SHIFT_SET ss);
extern int NrcShiftSetTypedCmp(NRC_SHIFT_SET ss1, NRC_SHIFT_SET ss2);
extern int NrcShiftSetCmp(const void *p1, const void *p2);
/* ***
extern void NrcShiftSetAddDay(NRC_SHIFT_SET ss, NRC_DAY d);
extern bool NrcShiftSetHasNeighbour(NRC_SHIFT_SET ss, int delta);
extern NRC_SHIFT_SET NrcShiftSetNeighbour(NRC_SHIFT_SET ss, int delta);
*** */
extern KHE_TIME_GROUP NrcShiftSetTimeGroup(NRC_SHIFT_SET ss, KHE_INSTANCE ins);
extern KHE_EVENT_GROUP NrcShiftSetEventGroup(NRC_SHIFT_SET ss, char *name,
  KHE_INSTANCE ins);

/* nrc_worker.c */
extern void NrcWorkerConvertToResource(NRC_WORKER w, KHE_RESOURCE_TYPE rt);
extern KHE_RESOURCE NrcWorkerResource(NRC_WORKER w);
extern void NrcWorkerConvertDayAndShiftOffRequests(NRC_WORKER w,
  KHE_INSTANCE ins);
extern void NrcWorkerConvertDayAndShiftOnRequests(NRC_WORKER w,
  KHE_INSTANCE ins);
extern void NrcWorkerConvertStartAndEndDayRequests(NRC_WORKER w,
  KHE_INSTANCE ins);

/* nrc_worker_set.c */
extern int NrcWorkerSetTypedCmp(NRC_WORKER_SET ws1, NRC_WORKER_SET ws2);
extern int NrcWorkerSetCmp(const void *t1, const void *t2);
extern bool NrcWorkerSetContains(NRC_WORKER_SET ws, NRC_WORKER w, int *pos);
extern KHE_RESOURCE_GROUP NrcWorkerSetResourceGroup(NRC_WORKER_SET ws,
  KHE_INSTANCE ins);

/* nrc_demand.c */
/* ***
extern void NrcDemandAddPenalizer(NRC_DEMAND d, NRC_PENALIZER_TYPE pt,
  NRC_WORKER_SET ws, NRC_PENALTY_TYPE ptype, NRC_PENALTY p);
extern char *NrcDemandName(NRC_DEMAND d);
extern int NrcDemandTypedCmp(NRC_DEMAND d1, NRC_DEMAND d2);
extern int NrcDemandCmp(const void *t1, const void *t2);
extern bool NrcDemandCanUseDemands(NRC_DEMAND d);
*** */
extern void NrcDemandConvertToEventResource(NRC_DEMAND d, NRC_SHIFT s,
  NRC_WORKER preasst, KHE_EVENT e);
extern void NrcDemandConvertToEventResourceConstraints(NRC_DEMAND d,
  KHE_INSTANCE ins);

/* nrc_cover.c */
/* ***
extern void NrcCoverAddShift(NRC_COVER c, NRC_SHIFT s);
extern void NrcCoverAddSkill(NRC_COVER c, NRC_SKILL skill);
extern void NrcCoverConvertToEventResources(NRC_COVER c, KHE_EVENT e);
extern void NrcCoverConvertToEventResourceConstraints(NRC_COVER c,
  KHE_INSTANCE res);
extern int NrcCoverSkillTypedCmp(NRC_COVER c1, NRC_COVER c2);
extern int NrcCoverSkillCmp(const void *t1, const void *t2);
*** */

/* nrc_cover_set.c */
/* ***
extern void NrcCoverSetConvertToEventResources(NRC_COVER_SET cs, KHE_EVENT e);
*** */

/* nrc_role.c */
/* ***
extern NRC_ROLE NrcRoleGet(NRC_INSTANCE ins, NRC_ROLE_TYPE role_type,
  bool reqd, int weight, int index, NRC_WORKER_SET skill_ws,
  bool skill_reqd, bool skill_weight);
extern void NrcRoleAddCover(NRC_ROLE r, NRC_COVER c);
extern void NrcRoleConvertToEventResource(NRC_ROLE r, KHE_EVENT e);
extern void NrcRoleConvertToEventResourceConstraints(NRC_ROLE r,
  KHE_INSTANCE ins);
*** */

/* nrc_skill.c */
/* ***
extern NRC_SKILL_SET NrcSkillSetMakeBegin(NRC_INSTANCE ins);
extern void NrcSkillSetAddCover(NRC_SKILL_SET ss, NRC_COVER c);
extern void NrcSkillSetAddWorker(NRC_SKILL_SET ss, NRC_WORKER w);
extern void NrcSkillSetMakeEnd(NRC_SKILL_SET ss);

extern int NrcSkillPartCount(NRC_SKILL skill);
extern void NrcSkillPart(NRC_SKILL skill, int i, NRC_WORKER_SET *worker_set,
  bool *penalty_reqd, int *penalty_weight);
extern int NrcSkillSetSkillCount(NRC_SKILL_SET ss);
extern NRC_SKILL NrcSkillSetSkill(NRC_SKILL_SET ss, int i);
*** */

/* nrc_role.c */
/* ***
extern NRC_ROLE NrcRoleMake(int counter, NRC_SHIFT_TYPE st,
  NRC_WORKER_SET skill_ws, bool reqd, int weight);
extern int NrcRoleCounter(NRC_ROLE r);
extern NRC_SHIFT_TYPE NrcRoleShiftType(NRC_ROLE r);
extern NRC_WORKER_SET NrcRoleSkillWorkerSet(NRC_ROLE r);
extern bool NrcRoleReqd(NRC_ROLE r);
extern int NrcRoleWeight(NRC_ROLE r);
extern char *NrcRoleLabel(NRC_ROLE r);
extern bool NrcRoleHasAttributes(NRC_ROLE r, int counter,
  NRC_SHIFT_TYPE st, NRC_WORKER_SET skill_ws, bool reqd, int weight);
*** */

/* nrc_role_set.c */
/* ***
extern NRC_ROLE_SET NrcRoleSetMake(void);
extern void NrcRoleSetAddRole(NRC_ROLE_SET rs, NRC_ROLE r);
extern int NrcRoleSetRoleCount(NRC_ROLE_SET rs);
extern NRC_ROLE NrcRoleSetRole(NRC_ROLE_SET rs, int i);
** ***
extern bool NrcRoleSetRetrieveRole(NRC_ROLE_SET rs, int counter,
  NRC_SHIFT_TYPE st, NRC_WORKER_SET skill_ws, bool reqd, int weight,
  NRC_ROLE *r);
*** **
extern NRC_ROLE NrcRoleSetGetRole(NRC_ROLE_SET rs, int counter,
  NRC_SHIFT_TYPE st, NRC_WORKER_SET skill_ws, bool reqd, int weight);
*** */

/* nrc_penalty.c */
extern KHE_COST_FUNCTION NrcPenaltyKheCostFn(NRC_PENALTY p);
#define NrcPenaltyConvert(p)					\
  NrcPenaltyHard(p), NrcPenaltyWeight(p), NrcPenaltyKheCostFn(p)
extern char *NrcPenaltyId(NRC_PENALTY p);
/* ***
extern NRC_PENALTY NrcPenaltyAdjust(NRC_PENALTY p, int subtract_weight,
  NRC_INSTANCE ins);
*** */
extern int NrcPenaltyTypedCmp(NRC_PENALTY p1, NRC_PENALTY p2);
extern int NrcPenaltyCmp(const void *p1, const void *p2);

/* nrc_bound.c */
extern int NrcBoundTypedCmp(NRC_BOUND b1, NRC_BOUND b2);
extern int NrcBoundCmp(const void *p1, const void *p2);
extern bool NrcBoundMerge(NRC_BOUND bound1, NRC_BOUND bound2, NRC_BOUND *res);
/* ***
extern bool NrcBoundSpecialMerge(NRC_BOUND bounds[], int bound_count,
  NRC_INSTANCE ins, NRC_BOUND *res);
*** */
extern int NrcBoundItemCount(NRC_BOUND b);
extern void NrcBoundItem(NRC_BOUND b, int i, int *min_value, bool *allow_zero,
  int *max_value, NRC_PENALTY *penalty);

/* nrc_demand.c */
extern NRC_DEMAND NrcDemandMake(NRC_INSTANCE ins);
extern void NrcDemandAddPreference(NRC_DEMAND d,
  NRC_WORKER_SET prefer_ws, bool prefer_nonasst, NRC_PENALTY p);
/* ***
extern void NrcDemandAddPenalize(NRC_DEMAND d, NRC_PENALIZER_TYPE type,
  NRC_WORKER_SET ws, NRC_PENALTY p);
*** */
extern char *NrcDemandName(NRC_DEMAND d);

/* nrc_demand_constraint.c */
/* extern bool NrcDemandConstraintCanUseDemands(NRC_DEMAND_CONSTRAINT dc); */
/* extern bool NrcDemandConstraintTryDemands(NRC_DEMAND_CONSTRAINT dc); */
extern void NrcDemandConstraintSetDefunct(NRC_DEMAND_CONSTRAINT dc);
extern int NrcDemandConstraintTypedCmpBoundAndWorkerSets(
  NRC_DEMAND_CONSTRAINT dc1, NRC_DEMAND_CONSTRAINT dc2);
extern int NrcDemandConstraintCmpBoundAndWorkerSets(const void *p1,
  const void *p2);
extern void NrcDemandConstraintsConvert(NRC_DEMAND_CONSTRAINT dc,
  int name_index, int first_index, int last_index, KHE_INSTANCE ins);

/* nrc_dc_converter.c */
extern NRC_DC_CONVERTER NrcDCConverterMake(NRC_INSTANCE ins);
extern void NrcDCConverterClear(NRC_DC_CONVERTER dcc);
extern void NrcDCConverterAddDemandConstraint(NRC_DC_CONVERTER dcc,
  NRC_DEMAND_CONSTRAINT dc);
extern bool NrcDCConverterSolve(NRC_DC_CONVERTER dcc, int wanted,
  NRC_DEMAND_SET *res);
extern void NrcDCConverterDebug(NRC_DC_CONVERTER dcc, int indent, FILE *fp);

/* nrc_constraint.c */
extern NRC_CONDENSED_CONSTRAINT NrcConstraintCondensedConstraint(
  NRC_CONSTRAINT c);

extern int NrcConstraintTypedCmpIgnoreNamesAndWorkerSets(NRC_CONSTRAINT c1,
  NRC_CONSTRAINT c2);
extern int NrcConstraintCmpIgnoreNamesAndWorkerSets(const void *p1,
  const void *p2);
extern int NrcConstraintTypedCmpIgnoreNamesAndBounds(NRC_CONSTRAINT c1,
  NRC_CONSTRAINT c2);
extern int NrcConstraintCmpIgnoreNamesAndBounds(const void *p1, const void *p2);
extern void NrcConstraintMergeBounds(NRC_CONSTRAINT c, int first_index,
  int last_index);
extern void NrcConstraintConvert(NRC_CONSTRAINT c, int name_index,
  int first_index, int last_index, KHE_INSTANCE ins);
extern void NrcConstraintKill(NRC_CONSTRAINT c);
/* extern void NrcConstraintConvertActiveToConsecutiveMax(NRC_CONSTRAINT c); */

/* nrc_condensed.c */
extern NRC_CONDENSED_CONSTRAINT NrcCondensedConstraintMake(NRC_INSTANCE ins,
  char *name, NRC_WORKER_SET worker_set, NRC_BOUND bound,
  NRC_SHIFT_SET first_ss, int length, int offset, NRC_POLARITY po,
  bool limits_initial, bool limits_max, NRC_CONSTRAINT orig_constraint);
extern void NrcCondensedConstraintDebug(NRC_CONDENSED_CONSTRAINT cc,
  int indent, FILE *fp);

extern NRC_CONSTRAINT_CONDENSER NrcConstraintCondenserBegin(NRC_INSTANCE ins);
extern void NrcConstraintCondenserEnd(NRC_CONSTRAINT_CONDENSER cc);
extern void NrcConstraintCondenserAddCondensedConstraint(
  NRC_CONSTRAINT_CONDENSER cm, NRC_CONDENSED_CONSTRAINT cc);
extern void NrcConstraintCondenserMerge(NRC_CONSTRAINT_CONDENSER cm);
extern void NrcConstraintCondenserDebug(NRC_CONSTRAINT_CONDENSER cm, int indent,
  FILE *fp);

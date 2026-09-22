
/*****************************************************************************/
/*                                                                           */
/*  THE NRCONV NURSE ROSTERING TO XHSTT CONVERTER                            */
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
/*  FILE:         externs.h                                                  */
/*  MODULE:       External declarations                                      */
/*                                                                           */
/*****************************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>
/* ? #include "khe_platform.h" */
#include "howard_a.h"
#include "howard_n.h"
#include "kml.h"
#include "nrc.h"

#define is_letter(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
#define is_digit(c) ((c) >= '0' && (c) <= '9')
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define ceil(a, b) (((a) + (b) - 1)/(b))


/*****************************************************************************/
/*                                                                           */
/*  Type declarations                                                        */
/*                                                                           */
/*****************************************************************************/

typedef struct instance_model_rec *INSTANCE_MODEL;
typedef struct soln_model_rec *SOLN_MODEL;


/*****************************************************************************/
/*                                                                           */
/*  External functions - error.c (error handling)                            */
/*                                                                           */
/*****************************************************************************/

extern void Warning(char *file_name, int line_num, char *fmt, ...);
extern void FatalError(char *file_name, int line_num, char *fmt, ...);
extern void KmlWarning(KML_ERROR ke, char *file_name);
extern void KmlFatalError(KML_ERROR ke, char *file_name);
extern void KmlEltWarning(KML_ELT elt, char *file_name, char *fmt, ...);
extern void KmlEltFatalError(KML_ELT elt, char *file_name, char *fmt, ...);


/*****************************************************************************/
/*                                                                           */
/*  External functions - map.c                                               */
/*                                                                           */
/*****************************************************************************/

extern void HelpMap(int indent, FILE *fp);
extern void Map(char *map_file_name, HA_ARENA_SET as);


/*****************************************************************************/
/*                                                                           */
/*  External functions - ins_model.c                                         */
/*                                                                           */
/*****************************************************************************/

/* reading model files */
extern bool ReadOptionalModelLine(FILE *fp, char *file_name,
  char *expected_label, char **res, HA_ARENA a);
extern char *ReadModelLine(FILE *fp, char *file_name, char *expected_label,
  HA_ARENA a);

/* construction and query */
extern INSTANCE_MODEL InstanceModelMake(char *file_name, HA_ARENA a);
extern NRC_INSTANCE InstanceModelConvertInstance(INSTANCE_MODEL ins_model,
  char *instance_file_name, HA_ARENA_SET as);
/* ***
extern NRC_INSTANCE_METADATA InstanceModelMetaData(INSTANCE_MODEL ins_model,
  char *name, char *contributor, char *date, char *country, char *description,
  char *remarks);
*** */


/*****************************************************************************/
/*                                                                           */
/*  External functions - soln_model.c                                        */
/*                                                                           */
/*****************************************************************************/

/* construction and query */
extern SOLN_MODEL SolnModelMake(char *file_name, NRC_ARCHIVE archive,
  HA_ARENA a);
extern void SolnModelConvertSoln(SOLN_MODEL soln_model,
  char *soln_file_name, NRC_INSTANCE ins, NRC_ARCHIVE archive,
  HA_ARENA_SET as);
/* ***
extern NRC_SOLN_GROUP SolnModelSolnGroup(SOLN_MODEL soln_model,
  NRC_ARCHIVE archive, char *algorithm, float runtime);
*** */
extern void SolnModelAddSoln(SOLN_MODEL soln_model, NRC_ARCHIVE archive,
  char *algorithm, float runtime, NRC_SOLN soln);


/*****************************************************************************/
/*                                                                           */
/*  External functions - main.c                                              */
/*                                                                           */
/*****************************************************************************/

extern char *CurrentDate(void);


/*****************************************************************************/
/*                                                                           */
/*  External functions - bcv.c                                               */
/*                                                                           */
/*****************************************************************************/

/* *** abandoned
extern NRC_INSTANCE BCVConvertInstance(INSTANCE_MODEL ins_model,
  char *instance_file_name);
extern void BCVConvertSoln(SOLN_MODEL soln_model, char *soln_file_name,
  NRC_INSTANCE ins, NRC_ARCHIVE archive);
*** */


/*****************************************************************************/
/*                                                                           */
/*  External functions - coi_bound.c (COI bounds, including weights etc.)    */
/*                                                                           */
/*****************************************************************************/

typedef struct coi_bound_rec *COI_BOUND;

extern bool CoiMinBoundRead(KML_ELT bound_elt, bool dft_below_reqd,
  int dft_below_weight, COI_BOUND *res, char **err_mess, HA_ARENA a,
  NRC_INSTANCE ins);
extern bool CoiPreferredBoundRead(KML_ELT bound_elt, bool dft_below_reqd,
  int dft_below_weight, bool dft_above_reqd, int dft_above_weight,
  COI_BOUND *res, char **err_mess, HA_ARENA a, NRC_INSTANCE ins);
extern bool CoiMaxBoundRead(KML_ELT bound_elt, bool dft_above_reqd,
  int dft_above_weight, COI_BOUND *res, char **err_mess, HA_ARENA a,
  NRC_INSTANCE ins);

extern char *CoiBoundLabel(COI_BOUND b);
extern char *CoiBoundVar(COI_BOUND b);
extern NRC_BOUND CoiBoundBound(COI_BOUND b);

extern void CoiBoundWorkloadLimitConstraintMake(COI_BOUND b,
  NRC_INSTANCE ins, NRC_WORKER_SET ws, NRC_SHIFT_SET ss, NRC_CONSTRAINT *c);

/* ***
extern NRC_PENALTY CoiMinLimitPenaltyBelow(COI_BOUND b);
extern NRC_PENALTY CoiMaxLimitPenaltyAbove(COI_BOUND b);
extern bool CoiLimitAddToDemandConstraint(COI_BOUND lim,
  NRC_DEMAND_CONSTRAINT dc);
*** */


/*****************************************************************************/
/*                                                                           */
/*  External functions - coi_limit.c (COI limits, including weights etc.)    */
/*                                                                           */
/*****************************************************************************/

/* *** COI_LIMIT replaced by COI_BOUND
typedef struct coi_limit_rec *COI_LIMIT;

extern bool CoiMinLimitRead(KML_ELT lim_elt, bool dft_below_reqd,
  int dft_below_weight, COI_LIMIT *res, char **err_mess);
extern bool CoiPreferredLimitRead(KML_ELT lim_elt, bool dft_below_reqd,
  int dft_below_weight, bool dft_above_reqd, int dft_above_weight,
  COI_LIMIT *res, char **err_mess);
extern bool CoiMaxLimitRead(KML_ELT lim_elt, bool dft_above_reqd,
  int dft_above_weight, COI_LIMIT *res, char **err_mess);

extern int CoiLimitLimit(COI_LIMIT lim);
extern char *ColLimitLabel(COI_LIMIT lim);
extern char *ColLimitVar(COI_LIMIT lim);

extern NRC_PENALTY CoiMinLimitPenaltyBelow(COI_LIMIT lim);
extern NRC_PENALTY CoiMaxLimitPenaltyAbove(COI_LIMIT lim);

** ***
extern bool CoiMinLimitReqd(COI_LIMIT lim);
extern int CoiMinLimitWeight(COI_LIMIT lim);
extern NRC_COST_FUNCTION CoiMinLimitCostFn(COI_LIMIT lim);

extern bool CoiMaxLimitReqd(COI_LIMIT lim);
extern int CoiMaxLimitWeight(COI_LIMIT lim);
extern NRC_COST_FUNCTION CoiMaxLimitCostFn(COI_LIMIT lim);
*** **

extern bool CoiLimitAddToDemandConstraint(COI_LIMIT lim,
  NRC_DEMAND_CONSTRAINT dc);
extern void CoiLimitWorkloadLimitConstraintMake(COI_LIMIT lim,
  NRC_INSTANCE ins, NRC_WORKER_SET ws, NRC_SHIFT_SET ss, NRC_CONSTRAINT *c);
*** */


/*****************************************************************************/
/*                                                                           */
/*  External functions - coi_cover.c (COI covers)                            */
/*                                                                           */
/*****************************************************************************/

/* typedefs */
typedef struct coi_cover_rec *COI_COVER;
typedef struct coi_day_cover_rec *COI_DAY_COVER;
typedef struct coi_cover_reqts_rec *COI_COVER_REQTS;

/* cover */
extern COI_COVER CoiCoverMake(KML_ELT cover_elt, NRC_SHIFT_TYPE st,
  NRC_TIME_INTERVAL ti, NRC_WORKER_SET skill_ws, COI_BOUND min_b,
  COI_BOUND pref_b, COI_BOUND max_b, HA_ARENA a);
  /* NRC_SHIFT_TYPE_SET sts, char *sts_descr, */
/* ***
extern COI_COVER CoiCoverMake(KML_ELT cover_elt,
  NRC_SHIFT_TYPE_SET shift_type_set, COI_LIMIT min_lim,
  COI_LIMIT pref_lim, COI_LIMIT max_lim, NRC_WORKER_SET skill_ws);
*** */

/* day cover */
extern COI_DAY_COVER CoiDayCoverMake(KML_ELT day_cover_elt,
  NRC_DAY_SET day_set, HA_ARENA a);
extern void CoiDayCoverAddCover(COI_DAY_COVER dc, COI_COVER cover);

/* cover requirements */
extern COI_COVER_REQTS CoiCoverReqtsMake(KML_ELT cover_reqts_elt,
  int dft_min_below_weight, int dft_max_above_weight,
  int dft_pref_above_weight, int dft_pref_below_weight, HA_ARENA a);

extern KML_ELT CoiCoverReqtsElt(COI_COVER_REQTS cr);
extern int CoiCoverReqtsDftMinBelowWeight(COI_COVER_REQTS cr);
extern int CoiCoverReqtsDftMaxAboveWeight(COI_COVER_REQTS cr);
extern int CoiCoverReqtsDftPrefAboveWeight(COI_COVER_REQTS cr);
extern int CoiCoverReqtsDftPrefBelowWeight(COI_COVER_REQTS cr);

extern void CoiCoverReqtsAddDayCover(COI_COVER_REQTS cr, COI_DAY_COVER dc);
extern void CoiCoverReqtsMakeDayCoversDisjoint(COI_COVER_REQTS cr);
extern int CoiCoverReqtsDemandCountForShift(COI_COVER_REQTS cr, NRC_SHIFT s);
/* ***
extern bool CoiCoverReqtsCheckForOverlappingDaySets(COI_COVER_REQTS cr,
  char **err_mess);
extern COI_DAY_COVER CoiCoverReqtsDayCover(COI_COVER_REQTS cr,NRC_DAY d);
extern int CoiDayCoverDemandCountForShiftType(COI_DAY_COVER dc,
  NRC_SHIFT_TYPE st);
*** */
extern bool CoiCoverReqtsGenerateDemandConstraints(COI_COVER_REQTS cr,
  NRC_INSTANCE ins, char *soln_file_name, NRC_WORKER_SET dft_ws,
  char **err_mess, KML_ELT *problem_elt, HA_ARENA a);


/*****************************************************************************/
/*                                                                           */
/*  External functions - coi.c                                               */
/*                                                                           */
/*****************************************************************************/

extern NRC_INSTANCE COIConvertInstance(INSTANCE_MODEL ins_model,
  char *instance_file_name, HA_ARENA_SET as);
extern void COIConvertSoln(SOLN_MODEL soln_model, char *soln_file_name,
  NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as);


/*****************************************************************************/
/*                                                                           */
/*  External functions - inrc1.c                                             */
/*                                                                           */
/*****************************************************************************/

extern NRC_INSTANCE INRC1ConvertInstance(INSTANCE_MODEL ins_model,
  char *instance_file_name, HA_ARENA_SET as);
extern NRC_SOLN_GROUP INRC1FirstSolnGroupFn(NRC_ARCHIVE archive,
  SOLN_MODEL soln_model, char *algorithm, float runtime);
extern void INRC1ConvertSoln(SOLN_MODEL soln_model, char *soln_file_name,
  NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as);


/*****************************************************************************/
/*                                                                           */
/*  External functions - inrc2.c                                             */
/*                                                                           */
/*****************************************************************************/

extern NRC_INSTANCE INRC2ConvertInstance(INSTANCE_MODEL ins_model,
  char *instance_file_name, HA_ARENA_SET as);
extern void INRC2ConvertSoln(SOLN_MODEL soln_model, char *soln_file_name,
  NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as);
extern void INRC2ConvertSolnOmer(SOLN_MODEL soln_model, char *soln_file_name,
  NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as);
extern void INRC2ConvertSolnSchaerf(SOLN_MODEL soln_model, char *soln_file_name,
  NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as);

extern NRC_INSTANCE INRC2ConvertInstanceStatic(INSTANCE_MODEL ins_model,
  char *instance_file_name, HA_ARENA_SET as);


/*****************************************************************************/
/*                                                                           */
/*  External functions - curtois2014.c                                       */
/*                                                                           */
/*****************************************************************************/

extern NRC_INSTANCE CQ14ConvertInstance(INSTANCE_MODEL ins_model,
  char *instance_file_name, HA_ARENA_SET as);
/* extern void CQ14AddSolnGroups(NRC_ARCHIVE archive); */
extern void CQ14ConvertSoln(SOLN_MODEL soln_model, char *soln_file_name,
  NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as);
extern NRC_SOLN_GROUP CQ14SolnGroupFn(SOLN_MODEL soln_model,
  NRC_ARCHIVE archive, char *algorithm, float runtime);

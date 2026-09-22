
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
/*  FILE:         nrc.h                                                      */
/*  MODULE:       External declarations                                      */
/*                                                                           */
/*****************************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include "howard_a.h"

#define USE_MKTIME 1  /* change this to 0 if you don't have Unix mktime() */


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

typedef struct nrc_archive_rec *NRC_ARCHIVE;
/* typedef struct nrc_archive_metadata_rec *NRC_ARCHIVE_METADATA; */
typedef struct nrc_soln_group_rec *NRC_SOLN_GROUP;
/* typedef struct nrc_soln_group_metadata_rec *NRC_SOLN_GROUP_METADATA; */
typedef struct nrc_soln_rec *NRC_SOLN;


/*****************************************************************************/
/*                                                                           */
/*    Chapter 3.   Instances                                                 */
/*                                                                           */
/*****************************************************************************/

/* instances */
typedef struct nrc_instance_rec *NRC_INSTANCE;
/* typedef struct nrc_instance_metadata_rec *NRC_INSTANCE_METADATA; */

/* days */
typedef struct nrc_day_rec *NRC_DAY;
typedef struct nrc_day_set_rec *NRC_DAY_SET;
typedef struct nrc_day_set_set_rec *NRC_DAY_SET_SET;

/* workers */
typedef struct nrc_worker_rec *NRC_WORKER;
typedef struct nrc_worker_set_rec *NRC_WORKER_SET;
typedef struct nrc_worker_set_set_rec *NRC_WORKER_SET_SET;

/* worker-set trees */
typedef struct nrc_worker_set_tree_rec *NRC_WORKER_SET_TREE;

/* covers */
/* ***
typedef struct nrc_cover_rec *NRC_COVER;
typedef struct nrc_cover_set_rec *NRC_COVER_SET;
*** */

/* demands */
typedef struct nrc_demand_rec *NRC_DEMAND;

typedef enum {
  NRC_PENALTY_REPLACE,
  NRC_PENALTY_ADD,
  NRC_PENALTY_UNIQUE
} NRC_PENALTY_TYPE;

typedef enum {
  NRC_PENALIZER_NON_ASSIGNMENT,
  NRC_PENALIZER_WORKER_SET,
  NRC_PENALIZER_NOT_WORKER_SET
} NRC_PENALIZER_TYPE;

typedef struct nrc_demand_set_rec *NRC_DEMAND_SET;

/* demand constraints */
typedef struct nrc_demand_constraint_rec *NRC_DEMAND_CONSTRAINT;

/* time intervals */
typedef struct nrc_time_interval_rec *NRC_TIME_INTERVAL;

/* shift types */
#define NRC_NO_WORKLOAD 1
typedef struct nrc_shift_type_rec *NRC_SHIFT_TYPE;
typedef struct nrc_shift_type_set_rec *NRC_SHIFT_TYPE_SET;

/* shifts */
typedef struct nrc_shift_rec *NRC_SHIFT;
typedef struct nrc_shift_set_rec *NRC_SHIFT_SET;
typedef struct nrc_shift_set_set_rec *NRC_SHIFT_SET_SET;

/* patterns */
typedef enum {
  NRC_NEGATIVE,
  NRC_POSITIVE
} NRC_POLARITY;

typedef struct nrc_pattern_rec *NRC_PATTERN;
typedef struct nrc_pattern_set_rec *NRC_PATTERN_SET;
/* ***
typedef struct nrc_term_rec *NRC_TERM;
*** */

/* constraints */
typedef enum {
  NRC_COST_FUNCTION_STEP,
  NRC_COST_FUNCTION_LINEAR,
  NRC_COST_FUNCTION_QUADRATIC
} NRC_COST_FUNCTION;

typedef struct nrc_penalty_rec *NRC_PENALTY;

/* ***
typedef enum {
  NRC_LIMIT_MIN,
  NRC_LIMIT_MIN_OR_ZERO,
  NRC_LIMIT_MAX,
  NRC_LIMIT_MIN_CONSECUTIVE,
  NRC_LIMIT_MAX_CONSECUTIVE,
  NRC_LIMIT_MIN_WORKLOAD,
  NRC_LIMIT_MAX_WORKLOAD,
  NRC_LIMIT_MIN_AND_MAX,
  NRC_LIMIT_MIN_OR_ZERO_AND_MAX,
  NRC_LIMIT_MIN_AND_MAX_CONSECUTIVE,
  NRC_LIMIT_MIN_AND_MAX_WORKLOAD
} NRC_LIMIT_TYPE;
*** */

typedef struct nrc_bound_rec *NRC_BOUND;

/* ***
typedef struct nrc_limit_rec *NRC_LIMIT;
*** */

typedef enum {
  NRC_CONSTRAINT_ACTIVE,
  NRC_CONSTRAINT_CONSECUTIVE,
  NRC_CONSTRAINT_WORKLOAD
} NRC_CONSTRAINT_TYPE;

typedef struct nrc_constraint_rec *NRC_CONSTRAINT;


/*****************************************************************************/
/*                                                                           */
/*    Chapter 4.   Solutions                                                 */
/*                                                                           */
/*****************************************************************************/

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

extern char *NrcVersionNumber(void);
extern char *NrcVersionBanner(void);


/*****************************************************************************/
/*                                                                           */
/*    Chapter 2.   Archives and Solution Groups                              */
/*                                                                           */
/*****************************************************************************/

/* 2.1 Archives */
extern NRC_ARCHIVE NrcArchiveMake(char *id, HA_ARENA_SET as);

extern char *NrcArchiveId(NRC_ARCHIVE archive);

extern void NrcArchiveSetMetaData(NRC_ARCHIVE archive, char *name,
  char *contributor, char *date, char *description, char *remarks);
extern void NrcArchiveMetaData(NRC_ARCHIVE archive, char **name,
  char **contributor, char **date, char **description, char **remarks);
/* ***
extern NRC_ARCHIVE_METADATA NrcArchiveMetaData(NRC_ARCHIVE archive);
extern NRC_ARCHIVE_METADATA NrcArchiveMetaDataMake(char *name,
  char *contributor, char *date, char *description, char *remarks);
extern char *NrcArchiveMetaDataName(NRC_ARCHIVE_METADATA md);
extern char *NrcArchiveMetaDataContributor(NRC_ARCHIVE_METADATA md);
extern char *NrcArchiveMetaDataDate(NRC_ARCHIVE_METADATA md);
extern char *NrcArchiveMetaDataDescription(NRC_ARCHIVE_METADATA md);
extern char *NrcArchiveMetaDataRemarks(NRC_ARCHIVE_METADATA md);
*** */

extern bool NrcArchiveAddInstance(NRC_ARCHIVE archive, NRC_INSTANCE ins);
extern void NrcArchiveDeleteInstance(NRC_ARCHIVE archive, NRC_INSTANCE ins);

extern int NrcArchiveInstanceCount(NRC_ARCHIVE archive);
extern NRC_INSTANCE NrcArchiveInstance(NRC_ARCHIVE archive, int i);
extern bool NrcArchiveRetrieveInstance(NRC_ARCHIVE archive, char *id,
  NRC_INSTANCE *ins);

extern int NrcArchiveSolnGroupCount(NRC_ARCHIVE archive);
extern NRC_SOLN_GROUP NrcArchiveSolnGroup(NRC_ARCHIVE archive, int i);
extern bool NrcArchiveRetrieveSolnGroup(NRC_ARCHIVE archive, char *id,
  NRC_SOLN_GROUP *soln_group);

/* 2.2 Solution groups */
extern bool NrcSolnGroupMake(NRC_ARCHIVE archive, char *id,
  /* NRC_SOLN_GROUP_METADATA md, */ NRC_SOLN_GROUP *soln_group);

extern NRC_ARCHIVE NrcSolnGroupArchive(NRC_SOLN_GROUP soln_group);
extern char *NrcSolnGroupId(NRC_SOLN_GROUP soln_group);

extern void NrcSolnGroupSetMetaData(NRC_SOLN_GROUP soln_group,
  char *contributor, char *date, char *description, char *publication,
  char *remarks);
extern void NrcSolnGroupMetaData(NRC_SOLN_GROUP soln_group,
  char **contributor, char **date, char **description, char **publication,
  char **remarks);
/* ***
extern NRC_SOLN_GROUP_METADATA NrcSolnGroupMetaData(NRC_SOLN_GROUP soln_group);
extern NRC_SOLN_GROUP_METADATA NrcSolnGroupMetaDataMake(char *contributor,
  char *date, char *description, char *publication, char *remarks);
extern char *NrcSolnGroupMetaDataContributor(NRC_SOLN_GROUP_METADATA md);
extern char *NrcSolnGroupMetaDataDate(NRC_SOLN_GROUP_METADATA md);
extern char *NrcSolnGroupMetaDataDescription(NRC_SOLN_GROUP_METADATA md);
extern char *NrcSolnGroupMetaDataPublication(NRC_SOLN_GROUP_METADATA md);
extern char *NrcSolnGroupMetaDataRemarks(NRC_SOLN_GROUP_METADATA md);
*** */

extern void NrcSolnGroupAddSoln(NRC_SOLN_GROUP soln_group, NRC_SOLN soln);
extern void NrcSolnGroupDeleteSoln(NRC_SOLN_GROUP soln_group, NRC_SOLN soln);

extern int NrcSolnGroupSolnCount(NRC_SOLN_GROUP soln_group);
extern NRC_SOLN NrcSolnGroupSoln(NRC_SOLN_GROUP soln_group, int i);

/* 2.3 Writing archives */
extern void NrcArchiveWrite(NRC_ARCHIVE archive, bool with_reports, FILE *fp);


/*****************************************************************************/
/*                                                                           */
/*  Chapter 3.   Instances                                                   */
/*                                                                           */
/*****************************************************************************/

/* 3.3.1 Creation, metadata, and archives */
extern NRC_INSTANCE NrcInstanceMakeBegin(char *id,
  char *worker_word, HA_ARENA_SET as);
extern char *NrcInstanceId(NRC_INSTANCE ins);
extern void NrcInstanceMakeEnd(NRC_INSTANCE ins);

extern HA_ARENA NrcInstanceArenaBegin(NRC_INSTANCE ins);
extern void NrcInstanceArenaEnd(NRC_INSTANCE ins, HA_ARENA a);

extern void NrcInstanceSetAvoidClashesPenalty(NRC_INSTANCE ins, NRC_PENALTY p);
extern NRC_PENALTY NrcInstanceAvoidClashesPenalty(NRC_INSTANCE ins);
extern NRC_PENALTY NrcInstanceZeroPenalty(NRC_INSTANCE ins);

extern void NrcInstanceSetMetaData(NRC_INSTANCE ins, char *name,
  char *contributor, char *date, char *country, char *description,
  char *remarks);
extern void NrcInstanceMetaData(NRC_INSTANCE ins, char **name,
  char **contributor, char **date, char **country, char **description,
  char **remarks);

/* ***
extern NRC_INSTANCE_METADATA NrcInstanceMetaData(NRC_INSTANCE ins);
extern NRC_INSTANCE_METADATA NrcInstanceMetaDataMake(char *name,
  char *contributor, char *date, char *country, char *description,
  char *remarks);
extern char *NrcInstanceMetaDataName(NRC_INSTANCE_METADATA md);
extern char *NrcInstanceMetaDataContributor(NRC_INSTANCE_METADATA md);
extern char *NrcInstanceMetaDataDate(NRC_INSTANCE_METADATA md);
extern char *NrcInstanceMetaDataCountry(NRC_INSTANCE_METADATA md);
extern char *NrcInstanceMetaDataDescription(NRC_INSTANCE_METADATA md);
extern char *NrcInstanceMetaDataRemarks(NRC_INSTANCE_METADATA md);
*** */

extern int NrcInstanceArchiveCount(NRC_INSTANCE ins);
extern NRC_ARCHIVE NrcInstanceArchive(NRC_INSTANCE ins, int i);

/* 3.3.2 Day names, days, day-sets, and day-set sets */
extern void NrcInstanceSetDayNames(NRC_INSTANCE ins, char *short_names,
  char *long_names);
extern int NrcInstanceDayNameCount(NRC_INSTANCE ins);
extern char *NrcInstanceShortDayName(NRC_INSTANCE ins, int i);
extern char *NrcInstanceLongDayName(NRC_INSTANCE ins, int i);

extern int NrcInstanceDaySetCount(NRC_INSTANCE ins);
extern NRC_DAY_SET NrcInstanceDaySet(NRC_INSTANCE ins, int i);

extern int NrcInstanceDaySetSetCount(NRC_INSTANCE ins);
extern NRC_DAY_SET_SET NrcInstanceDaySetSet(NRC_INSTANCE ins, int i);

/* 3.3.3 The cycle and the days of the week */
extern NRC_DAY_SET NrcInstanceCycle(NRC_INSTANCE ins);
extern int NrcInstanceCycleDayCount(NRC_INSTANCE ins);
extern NRC_DAY NrcInstanceCycleDay(NRC_INSTANCE ins, int i);
extern bool NrcInstanceCycleRetrieveDay(NRC_INSTANCE ins, char *ymd,
  NRC_DAY *d);

extern NRC_DAY_SET_SET NrcInstanceDaysOfWeek(NRC_INSTANCE ins);
extern int NrcInstanceDaysOfWeekDaySetCount(NRC_INSTANCE ins);
extern NRC_DAY_SET NrcInstanceDaysOfWeekDaySet(NRC_INSTANCE ins, int i);
extern bool NrcInstanceDaysOfWeekRetrieveDaySetShort(NRC_INSTANCE ins,
  char *short_name, NRC_DAY_SET *ds);
extern bool NrcInstanceDaysOfWeekRetrieveDaySetLong(NRC_INSTANCE ins,
  char *long_name, NRC_DAY_SET *ds);

/* 3.3.4 Shift types and shift-type sets */
extern NRC_SHIFT_TYPE_SET NrcInstanceAllShiftTypes(NRC_INSTANCE ins);
extern int NrcInstanceShiftTypeCount(NRC_INSTANCE ins);
extern NRC_SHIFT_TYPE NrcInstanceShiftType(NRC_INSTANCE ins, int i);
extern bool NrcInstanceRetrieveShiftType(NRC_INSTANCE ins, char *name,
  NRC_SHIFT_TYPE *st);
extern bool NrcInstanceRetrieveShiftTypeByLabel(NRC_INSTANCE ins, char *label,
  NRC_SHIFT_TYPE *st);

extern int NrcInstanceShiftTypeSetCount(NRC_INSTANCE ins);
extern NRC_SHIFT_TYPE_SET NrcInstanceShiftTypeSet(NRC_INSTANCE ins, int i);
extern bool NrcInstanceRetrieveShiftTypeSet(NRC_INSTANCE ins, char *name,
  NRC_SHIFT_TYPE_SET *sts);

/* 3.3.5 Shifts, shift-sets, and shift-set sets */
extern NRC_SHIFT_SET NrcInstanceAllShifts(NRC_INSTANCE ins);
extern int NrcInstanceShiftCount(NRC_INSTANCE ins);
extern NRC_SHIFT NrcInstanceShift(NRC_INSTANCE ins, int i);

extern NRC_SHIFT_SET NrcInstanceDailyStartingShiftSet(NRC_INSTANCE ins);
extern NRC_SHIFT_SET NrcInstanceWeeklyStartingShiftSet(NRC_INSTANCE ins);

extern int NrcInstanceShiftSetCount(NRC_INSTANCE ins);
extern NRC_SHIFT_SET NrcInstanceShiftSet(NRC_INSTANCE ins, int i);

extern int NrcInstanceShiftSetSetCount(NRC_INSTANCE ins);
extern NRC_SHIFT_SET_SET NrcInstanceShiftSetSet(NRC_INSTANCE ins, int i);

extern NRC_SHIFT_SET_SET NrcInstanceDaysShiftSetSet(NRC_INSTANCE ins);
extern NRC_SHIFT_SET_SET NrcInstanceShiftsShiftSetSet(NRC_INSTANCE ins);

/* 3.3.6 Workers, worker-sets, and worker-set sets */
extern NRC_WORKER_SET NrcInstanceStaffing(NRC_INSTANCE ins);
extern int NrcInstanceStaffingWorkerCount(NRC_INSTANCE ins);
extern NRC_WORKER NrcInstanceStaffingWorker(NRC_INSTANCE ins, int i);
extern bool NrcInstanceStaffingRetrieveWorker(NRC_INSTANCE ins,
  char *name, NRC_WORKER *w);
extern NRC_WORKER_SET NrcInstanceEmptyWorkerSet(NRC_INSTANCE ins);

extern int NrcInstanceWorkerSetCount(NRC_INSTANCE ins);
extern NRC_WORKER_SET NrcInstanceWorkerSet(NRC_INSTANCE ins, int i);
extern bool NrcInstanceRetrieveWorkerSet(NRC_INSTANCE ins, char *name,
  NRC_WORKER_SET *ws);

extern int NrcInstanceWorkerSetSetCount(NRC_INSTANCE ins);
extern NRC_WORKER_SET_SET NrcInstanceWorkerSetSet(NRC_INSTANCE ins, int i);

/* 3.3.7 Contracts and skills */
extern void NrcInstanceContractsAddContract(NRC_INSTANCE ins,
  NRC_WORKER_SET contract_ws);
extern NRC_WORKER_SET_SET NrcInstanceContracts(NRC_INSTANCE ins);
extern int NrcInstanceContractsContractCount(NRC_INSTANCE ins);
extern NRC_WORKER_SET NrcInstanceContractsContract(NRC_INSTANCE ins, int i);
extern bool NrcInstanceContractsRetrieveContract(NRC_INSTANCE ins, char *name,
  NRC_WORKER_SET *contract_ws);

extern void NrcInstanceSkillsAddSkill(NRC_INSTANCE ins,
  NRC_WORKER_SET skill_ws);
extern NRC_WORKER_SET_SET NrcInstanceSkills(NRC_INSTANCE ins);
extern int NrcInstanceSkillsSkillCount(NRC_INSTANCE ins);
extern NRC_WORKER_SET NrcInstanceSkillsSkill(NRC_INSTANCE ins, int i);
extern bool NrcInstanceSkillsRetrieveSkill(NRC_INSTANCE ins, char *name,
  NRC_WORKER_SET *skill_ws);

/* 3.3.8 Demands, demand-sets, patterns, and constraints */
/* ***
extern int NrcInstanceCoverCount(NRC_INSTANCE ins);
extern NRC_COVER NrcInstanceCover(NRC_INSTANCE ins, int i);

extern int NrcInstanceCoverSetCount(NRC_INSTANCE ins);
extern NRC_COVER_SET NrcInstanceSetCover(NRC_INSTANCE ins, int i);
*** */

extern int NrcInstanceDemandCount(NRC_INSTANCE ins);
extern NRC_DEMAND NrcInstanceDemand(NRC_INSTANCE ins, int i);

extern int NrcInstanceDemandSetCount(NRC_INSTANCE ins);
extern NRC_DEMAND_SET NrcInstanceDemandSet(NRC_INSTANCE ins, int i);

extern int NrcInstancePatternCount(NRC_INSTANCE ins);
extern NRC_PATTERN NrcInstancePattern(NRC_INSTANCE ins, int i);
extern bool NrcInstanceRetrievePattern(NRC_INSTANCE ins, char *name,
  NRC_PATTERN *p);

extern int NrcInstanceDemandConstraintCount(NRC_INSTANCE ins);
extern NRC_DEMAND_CONSTRAINT NrcInstanceDemandConstraint(NRC_INSTANCE ins,
  int i);

extern int NrcInstanceConstraintCount(NRC_INSTANCE ins);
extern NRC_CONSTRAINT NrcInstanceConstraint(NRC_INSTANCE ins, int i);

/* 3.4.1 The cycle and the days of the week */
extern void NrcCycleMake(NRC_INSTANCE ins, int day_count, int first_day_index);
extern bool NrcCalendarCycleMake(NRC_INSTANCE ins,
  char *start_ymd, char *end_ymd, char **err_str);

/* 3.4.2 Days */
extern NRC_INSTANCE NrcDayInstance(NRC_DAY d);
extern char *NrcDayYMD(NRC_DAY d);
extern char *NrcDayShortName(NRC_DAY d);
extern char *NrcDayLongName(NRC_DAY d);

extern int NrcDayIndexInCycle(NRC_DAY d);
extern int NrcDayWeekInCycle(NRC_DAY d);
extern int NrcDayIndexInWeek(NRC_DAY d);

extern NRC_DAY NrcDayPrev(NRC_DAY d);
extern NRC_DAY NrcDayNext(NRC_DAY d);

extern NRC_DAY_SET NrcDayDayOfWeek(NRC_DAY d);
extern NRC_DAY_SET NrcDaySingletonDaySet(NRC_DAY d);

extern NRC_SHIFT_SET NrcDayShiftSet(NRC_DAY d);
extern int NrcDayShiftCount(NRC_DAY d);
extern NRC_SHIFT NrcDayShift(NRC_DAY d, int i);

extern NRC_SHIFT NrcDayShiftFromShiftType(NRC_DAY d, NRC_SHIFT_TYPE st);
extern NRC_SHIFT_SET NrcDayShiftSetFromShiftTypeSet(NRC_DAY d,
  NRC_SHIFT_TYPE_SET sts);
extern NRC_SHIFT_SET_SET NrcDayShiftSetSet(NRC_DAY d);

extern void NrcDayDebug(NRC_DAY d, int indent, FILE *fp);

/* 3.4.3 Day-sets */
extern NRC_DAY_SET NrcDaySetMake(NRC_INSTANCE ins, char *short_name,
  char *long_name);
extern void NrcDaySetAddDay(NRC_DAY_SET ds, NRC_DAY d);

extern NRC_INSTANCE NrcDaySetInstance(NRC_DAY_SET ds);
extern char *NrcDaySetShortName(NRC_DAY_SET ds);
extern char *NrcDaySetLongName(NRC_DAY_SET ds);

extern int NrcDaySetDayCount(NRC_DAY_SET ds);
extern NRC_DAY NrcDaySetDay(NRC_DAY_SET ds, int i);
extern bool NrcDaySetContainsDay(NRC_DAY_SET ds, NRC_DAY d);
extern bool NrcDaySetRetrieveDay(NRC_DAY_SET ds, char *ymd, NRC_DAY *d);

extern bool NrcDaySetsOverlap(NRC_DAY_SET ds1, NRC_DAY_SET ds2);
extern NRC_DAY_SET NrcDaySetDifference(NRC_DAY_SET ds1, NRC_DAY_SET ds2);

extern NRC_SHIFT_SET NrcDaySetShiftSet(NRC_DAY_SET ds);
extern NRC_SHIFT_SET NrcDaySetStartingShiftSet(NRC_DAY_SET ds);
extern NRC_SHIFT_SET_SET NrcDaySetShiftSetSet(NRC_DAY_SET ds);

extern NRC_SHIFT_SET NrcDaySetShiftSetFromShiftType(NRC_DAY_SET ds,
  NRC_SHIFT_TYPE st);
extern NRC_SHIFT_SET NrcDaySetShiftSetFromShiftTypeSet(NRC_DAY_SET ds,
  NRC_SHIFT_TYPE_SET sts);

extern void NrcDaySetDebug(NRC_DAY_SET ds, int indent, FILE *fp);

/* 3.4.4 Day-set sets */
extern NRC_DAY_SET_SET NrcDaySetSetMake(NRC_INSTANCE ins, char *short_name,
  char *long_name);
extern void NrcDaySetSetAddDaySet(NRC_DAY_SET_SET dss, NRC_DAY_SET ds);

extern NRC_INSTANCE NrcDaySetSetInstance(NRC_DAY_SET_SET dss);
extern char *NrcDaySetSetShortName(NRC_DAY_SET_SET dss);
extern char *NrcDaySetSetLongName(NRC_DAY_SET_SET dss);

extern int NrcDaySetSetDaySetCount(NRC_DAY_SET_SET dss);
extern NRC_DAY_SET NrcDaySetSetDaySet(NRC_DAY_SET_SET dss, int i);
extern bool NrcDaySetSetRetrieveDaySetLong(NRC_DAY_SET_SET dss,
  char *long_name, NRC_DAY_SET *ds);
extern bool NrcDaySetSetRetrieveDaySetShort(NRC_DAY_SET_SET dss,
  char *short_name, NRC_DAY_SET *ds);

extern void NrcDaySetSetDebug(NRC_DAY_SET_SET dss, int indent, FILE *fp);

/* 3.5 Time intervals */
extern bool NrcHMSToSecs(char *hms, int *res);
extern char *NrcSecsToHMS(int secs, HA_ARENA a);
extern NRC_TIME_INTERVAL NrcTimeIntervalMake(int start_secs, int end_secs,
  NRC_INSTANCE ins);
extern bool NrcTimeIntervalMakeFromHMS(char *start_hms, char *end_hms,
  NRC_TIME_INTERVAL *res, NRC_INSTANCE ins);
extern int NrcTimeIntervalStartSecs(NRC_TIME_INTERVAL ti);
extern int NrcTimeIntervalEndSecs(NRC_TIME_INTERVAL ti);
extern bool NrcTimeIntervalEqual(NRC_TIME_INTERVAL ti1, NRC_TIME_INTERVAL ti2);
extern bool NrcTimeIntervalDisjoint(NRC_TIME_INTERVAL ti1,
  NRC_TIME_INTERVAL ti2);
extern bool NrcTimeIntervalSubset(NRC_TIME_INTERVAL ti1, NRC_TIME_INTERVAL ti2);
extern NRC_SHIFT_SET NrcTimeIntervalShiftSet(NRC_TIME_INTERVAL ti, NRC_DAY d);
extern char *NrcTimeIntervalShow(NRC_TIME_INTERVAL ti, HA_ARENA a);

/* 3.6.1 Shift types */
extern NRC_SHIFT_TYPE NrcShiftTypeMake(NRC_INSTANCE ins, char *name,
  int workload);
extern void NrcShiftTypeUseLabelInEventName(NRC_SHIFT_TYPE st, char *label);
extern NRC_INSTANCE NrcShiftTypeInstance(NRC_SHIFT_TYPE st);
extern char *NrcShiftTypeName(NRC_SHIFT_TYPE st);
extern char *NrcShiftTypeLabel(NRC_SHIFT_TYPE st);
extern int NrcShiftTypeWorkload(NRC_SHIFT_TYPE st);
extern int NrcShiftTypeIndex(NRC_SHIFT_TYPE st);
extern NRC_SHIFT_TYPE_SET NrcShiftTypeSingletonShiftTypeSet(NRC_SHIFT_TYPE st);
extern NRC_SHIFT_SET NrcShiftTypeShiftSet(NRC_SHIFT_TYPE st);
extern NRC_SHIFT_SET_SET NrcShiftTypeShiftSetSet(NRC_SHIFT_TYPE st);

/* means time off */
/* ***
extern void NrcShiftTypeSetMeansTimeOff(NRC_SHIFT_TYPE st, bool val);
extern bool NrcShiftTypeMeansTimeOff(NRC_SHIFT_TYPE st);
*** */

/* time intervals */
extern void NrcShiftTypeAddTimeInterval(NRC_SHIFT_TYPE st,
  NRC_TIME_INTERVAL ti);
extern NRC_TIME_INTERVAL NrcShiftTypeTimeInterval(NRC_SHIFT_TYPE st);

/* start and end times */
/* ***
extern bool NrcHMSToSecs(char *hms, int *res);
extern char *NrcSecsToHMS(int secs);
extern void NrcShiftTypeAddStartAndEndTimes(NRC_SHIFT_TYPE st,
  int start_secs, int end_secs);
extern int NrcShiftTypeStartSecs(NRC_SHIFT_TYPE st);
extern int NrcShiftTypeEndSecs(NRC_SHIFT_TYPE st);
extern bool NrcShiftTypeIntersects(NRC_SHIFT_TYPE st,
  int start_secs, int end_secs);
*** */

/* 3.6.2 Shift-type sets */
extern NRC_SHIFT_TYPE_SET NrcShiftTypeSetMake(NRC_INSTANCE ins, char *name);
extern void NrcShiftTypeSetAddShiftType(NRC_SHIFT_TYPE_SET sts,
  NRC_SHIFT_TYPE st);
extern NRC_INSTANCE NrcShiftTypeSetInstance(NRC_SHIFT_TYPE_SET sts);
extern char *NrcShiftTypeSetName(NRC_SHIFT_TYPE_SET sts);
extern int NrcShiftTypeSetShiftTypeCount(NRC_SHIFT_TYPE_SET sts);
extern NRC_SHIFT_TYPE NrcShiftTypeSetShiftType(NRC_SHIFT_TYPE_SET sts, int i);
extern bool NrcShiftTypeSetContainsShiftType(NRC_SHIFT_TYPE_SET sts,
  NRC_SHIFT_TYPE st);
extern bool NrcShiftTypeSetEqual(NRC_SHIFT_TYPE_SET sts1,
  NRC_SHIFT_TYPE_SET sts2);
extern bool NrcShiftTypeSetDisjoint(NRC_SHIFT_TYPE_SET sts1,
  NRC_SHIFT_TYPE_SET sts2);
extern NRC_SHIFT_TYPE_SET NrcShiftTypeSetMerge(NRC_SHIFT_TYPE_SET sts1,
  NRC_SHIFT_TYPE_SET sts2);

/* 3.7.1 Shifts */
extern NRC_INSTANCE NrcShiftInstance(NRC_SHIFT s);
extern NRC_DAY NrcShiftDay(NRC_SHIFT s);
extern NRC_SHIFT_TYPE NrcShiftType(NRC_SHIFT s);
/* extern int NrcShiftIndexInInstance(NRC_SHIFT s); */
extern char *NrcShiftName(NRC_SHIFT s);

extern NRC_SHIFT_SET NrcShiftSingletonShiftSet(NRC_SHIFT s);

/* extern void NrcShiftSetWorkload(NRC_SHIFT s, int workload); */
extern int NrcShiftWorkload(NRC_SHIFT s);

/* ***
extern void NrcShiftAddCover(NRC_SHIFT s, NRC_COVER c);
extern int NrcShiftCoverCount(NRC_SHIFT s);
extern NRC_COVER NrcShiftCover(NRC_SHIFT s, int i);
extern void NrcShiftAddCoverSet(NRC_SHIFT s, NRC_COVER_SET cs);
extern int NrcShiftCoverSetCount(NRC_SHIFT s);
extern NRC_COVER_SET NrcShiftCoverSet(NRC_SHIFT s, int i);
*** */

extern void NrcShiftDebug(NRC_SHIFT s, int indent, FILE *fp);
extern void NrcShiftAddDemand(NRC_SHIFT s, NRC_DEMAND d);
extern void NrcShiftAddDemandMulti(NRC_SHIFT s, NRC_DEMAND d,
  int multiplicity);
extern void NrcShiftAddDemandSet(NRC_SHIFT s, NRC_DEMAND_SET ds);
extern int NrcShiftDemandCount(NRC_SHIFT s);
extern NRC_DEMAND NrcShiftDemand(NRC_SHIFT s, int i);
/* extern int NrcShiftDemandSetCount(NRC_SHIFT s); */
/* extern NRC_DEMAND_SET NrcShiftDemandSet(NRC_SHIFT s, int i); */

extern void NrcShiftAddPreassignment(NRC_SHIFT s, NRC_WORKER w);
extern int NrcShiftPreassignmentCount(NRC_SHIFT s);
extern NRC_WORKER NrcShiftPreassignment(NRC_SHIFT s, int i);

extern void NrcShiftDebug(NRC_SHIFT s, int indent, FILE *fp);

/* 3.7.2 Shift-sets */
extern NRC_SHIFT_SET NrcShiftSetMake(NRC_INSTANCE ins);
extern void NrcShiftSetAddShift(NRC_SHIFT_SET ss, NRC_SHIFT s);
extern void NrcShiftSetAddShiftSet(NRC_SHIFT_SET ss, NRC_SHIFT_SET ss2);
extern NRC_INSTANCE NrcShiftSetInstance(NRC_SHIFT_SET ss);
extern int NrcShiftSetShiftCount(NRC_SHIFT_SET ss);
extern NRC_SHIFT NrcShiftSetShift(NRC_SHIFT_SET ss, int i);
extern bool NrcShiftSetContainsShift(NRC_SHIFT_SET ss, NRC_SHIFT s);

extern bool NrcShiftSetUniform(NRC_SHIFT_SET ss, int *offset);
extern bool NrcShiftSetsUniform(NRC_SHIFT_SET ss1, NRC_SHIFT_SET ss2,
  int *offset);
extern bool NrcShiftSetsEqual(NRC_SHIFT_SET ss1, NRC_SHIFT_SET ss2);

extern void NrcShiftSetAndPolarityDebug(NRC_SHIFT_SET ss, NRC_POLARITY po,
  int indent, FILE *fp);
extern void NrcShiftSetDebug(NRC_SHIFT_SET ss, int indent, FILE *fp);

/* 3.7.3 Shift-set sets */
extern NRC_SHIFT_SET_SET NrcShiftSetSetMake(NRC_INSTANCE ins);
extern void NrcShiftSetSetAddShiftSet(NRC_SHIFT_SET_SET sss, NRC_SHIFT_SET ss);
extern NRC_INSTANCE NrcShiftSetSetInstance(NRC_SHIFT_SET_SET sss);
extern int NrcShiftSetSetShiftSetCount(NRC_SHIFT_SET_SET sss);
extern NRC_SHIFT_SET NrcShiftSetSetShiftSet(NRC_SHIFT_SET_SET sss, int i);

extern NRC_SHIFT_SET NrcShiftSetSetStartingShiftSet(NRC_SHIFT_SET_SET sss);

extern void NrcShiftSetSetDebug(NRC_SHIFT_SET_SET sss, int indent, FILE *fp);

/* 3.8.1 Workers */
extern NRC_WORKER NrcWorkerMake(NRC_INSTANCE ins, char *name);

extern NRC_INSTANCE NrcWorkerInstance(NRC_WORKER w);
extern char *NrcWorkerName(NRC_WORKER w);
extern char *NrcWorkerConvertedName(NRC_WORKER w);
extern int NrcWorkerIndex(NRC_WORKER w);

extern NRC_WORKER_SET NrcWorkerSingletonWorkerSet(NRC_WORKER w);

/* ***
extern void NrcWorkerAddSkillPenalty(NRC_WORKER w, NRC_PENALTY p);
extern NRC_PENALTY NrcWorkerSkillPenalty(NRC_WORKER w);
*** */

extern void NrcWorkerAddShiftOff(NRC_WORKER w, NRC_SHIFT s, NRC_PENALTY p);
extern void NrcWorkerAddShiftSetOff(NRC_WORKER w, NRC_SHIFT_SET ss,
  NRC_PENALTY p);
extern void NrcWorkerAddDayOff(NRC_WORKER w, NRC_DAY d, NRC_PENALTY p);

extern void NrcWorkerAddShiftOn(NRC_WORKER w, NRC_SHIFT s, NRC_PENALTY p);
extern void NrcWorkerAddShiftSetOn(NRC_WORKER w, NRC_SHIFT_SET ss,
  NRC_PENALTY p);
extern void NrcWorkerAddDayOn(NRC_WORKER w, NRC_DAY d, NRC_PENALTY p);

extern void NrcWorkerAddStartDay(NRC_WORKER w, NRC_DAY d, NRC_PENALTY p);
extern void NrcWorkerAddEndDay(NRC_WORKER w, NRC_DAY d, NRC_PENALTY p);

/* ***
extern void NrcWorkerAddHistoryPattern(NRC_WORKER w, NRC_PATTERN p);
extern bool NrcWorkerRetrieveHistoryPattern(NRC_WORKER w, NRC_PATTERN *p);
*** */
extern void NrcWorkerAddHistory(NRC_WORKER w, char *name, int value);
extern bool NrcWorkerRetrieveHistory(NRC_WORKER w, char *name, int *value);

extern void NrcWorkerDebug(NRC_WORKER w, int indent, FILE *fp);

/* 3.8.2 Worker-sets and contracts and skills */
extern NRC_WORKER_SET NrcWorkerSetMake(NRC_INSTANCE ins, char *name);
extern NRC_INSTANCE NrcWorkerSetInstance(NRC_WORKER_SET ws);
extern char *NrcWorkerSetName(NRC_WORKER_SET ws);
extern void NrcWorkerSetAddWorker(NRC_WORKER_SET ws, NRC_WORKER w);
extern void NrcWorkerSetAddWorkerSet(NRC_WORKER_SET ws1, NRC_WORKER_SET ws2);

extern int NrcWorkerSetWorkerCount(NRC_WORKER_SET ws);
extern NRC_WORKER NrcWorkerSetWorker(NRC_WORKER_SET ws, int i);
extern bool NrcWorkerSetContainsWorker(NRC_WORKER_SET ws, NRC_WORKER w);
extern bool NrcWorkerSetRetrieveWorker(NRC_WORKER_SET ws, char *name,
  NRC_WORKER *w);

extern bool NrcWorkerSetHasNoDuplicates(NRC_WORKER_SET ws);
extern bool NrcWorkerSetEqual(NRC_WORKER_SET ws1, NRC_WORKER_SET ws2);
extern bool NrcWorkerSetDisjoint(NRC_WORKER_SET ws1, NRC_WORKER_SET ws2);
extern bool NrcWorkerSetSubset(NRC_WORKER_SET lower_ws,
  NRC_WORKER_SET upper_ws);

extern NRC_WORKER_SET NrcWorkerSetComplement(NRC_WORKER_SET ws);

extern void NrcWorkerSetDebug(NRC_WORKER_SET ws, int indent, FILE *fp);

/* 3.8.3 Worker-set sets */
extern NRC_WORKER_SET_SET NrcWorkerSetSetMake(NRC_INSTANCE ins);
extern NRC_INSTANCE NrcWorkerSetSetInstance(NRC_WORKER_SET_SET wss);
extern void NrcWorkerSetSetAddWorkerSet(NRC_WORKER_SET_SET wss,
  NRC_WORKER_SET ws);

extern int NrcWorkerSetSetWorkerSetCount(NRC_WORKER_SET_SET wss);
extern NRC_WORKER_SET NrcWorkerSetSetWorkerSet(NRC_WORKER_SET_SET wss, int i);

extern bool NrcWorkerSetSetRetrieveWorkerSet(NRC_WORKER_SET_SET wss,
  char *name, NRC_WORKER_SET *ws);

extern void NrcWorkerSetSetDebug(NRC_WORKER_SET_SET wss, int indent, FILE *fp);

/* 3.8.4 Worker-set trees */
extern NRC_WORKER_SET_TREE NrcWorkerSetTreeMake(NRC_INSTANCE ins);
extern bool NrcWorkerSetTreeAddWorkerSet(NRC_WORKER_SET_TREE wst,
  NRC_WORKER_SET ws, NRC_WORKER_SET *incompatible_ws);
/* extern void NrcWorkerSetTreeFree(NRC_WORKER_SET_TREE wst); */
extern int NrcWorkerSetTreeRootWorkerSetCount(NRC_WORKER_SET_TREE wst);
extern NRC_WORKER_SET NrcWorkerSetTreeRootWorkerSet(NRC_WORKER_SET_TREE wst,
  int i);
extern int NrcWorkerSetTreeChildCount(NRC_WORKER_SET_TREE wst);
extern NRC_WORKER_SET_TREE NrcWorkerSetTreeChild(NRC_WORKER_SET_TREE wst,
  int i);
extern void NrcWorkerSetTreeDebug(NRC_WORKER_SET_TREE wst, int indent,
  FILE *fp);

/* 3.9.1 Penalties and costs */
extern NRC_PENALTY NrcPenalty(bool hard, int weight, NRC_COST_FUNCTION cost_fn,
  NRC_INSTANCE ins);
extern bool NrcPenaltyHard(NRC_PENALTY p);
extern int NrcPenaltyWeight(NRC_PENALTY p);
extern NRC_COST_FUNCTION NrcPenaltyCostFn(NRC_PENALTY p);
extern bool NrcPenaltyEqual(NRC_PENALTY p1, NRC_PENALTY p2);
extern bool NrcPenaltyLessThan(NRC_PENALTY p1, NRC_PENALTY p2);
extern NRC_PENALTY NrcPenaltyAdd(NRC_PENALTY p1, NRC_PENALTY p2,
  NRC_INSTANCE ins);
extern char *NrcCostFnShow(NRC_COST_FUNCTION cost_fn);
extern char *NrcPenaltyShow(NRC_PENALTY p);

/* 3.9.2 Bounds */
extern NRC_BOUND NrcBoundMake(NRC_INSTANCE ins);
extern NRC_INSTANCE NrcBoundInstance(NRC_BOUND b);

extern bool NrcBoundAddMin(NRC_BOUND b, int min_value, bool allow_zero,
  NRC_PENALTY below_min_penalty);
extern bool NrcBoundAddMax(NRC_BOUND b, int max_value,
  NRC_PENALTY above_max_penalty);
extern bool NrcBoundAddPreferred(NRC_BOUND b, int preferred_value,
  NRC_PENALTY below_preferred_penalty, NRC_PENALTY above_preferred_penalty);

extern NRC_BOUND NrcBoundMakeMin(int min_value, bool allow_zero,
  NRC_PENALTY below_min_penalty, NRC_INSTANCE ins);
extern NRC_BOUND NrcBoundMakeMax(int max_value, NRC_PENALTY above_max_penalty,
  NRC_INSTANCE ins);
extern NRC_BOUND NrcBoundMakePreferred(int preferred_value,
  NRC_PENALTY below_preferred_penalty, NRC_PENALTY above_preferred_penalty,
  NRC_INSTANCE ins);

extern bool NrcBoundMin(NRC_BOUND b, int *min_value, bool *allow_zero,
  NRC_PENALTY *below_min_penalty);
extern bool NrcBoundMax(NRC_BOUND b, int *max_value,
  NRC_PENALTY *above_max_penalty);
extern bool NrcBoundPreferred(NRC_BOUND b, int *preferred_value,
  NRC_PENALTY *below_preferred_penalty, NRC_PENALTY *above_preferred_penalty);

extern char *NrcBoundShow(NRC_BOUND b);

/* 3.9.3 Demands */
extern NRC_DEMAND NrcDemandMakeBegin(NRC_INSTANCE ins);
extern void NrcDemandMakeEnd(NRC_DEMAND d);
extern void NrcDemandPenalizeNonAssignment(NRC_DEMAND d,
  NRC_PENALTY_TYPE ptype, NRC_PENALTY p);
extern void NrcDemandPenalizeWorkerSet(NRC_DEMAND d, NRC_WORKER_SET ws,
  NRC_PENALTY_TYPE ptype, NRC_PENALTY p);
extern void NrcDemandPenalizeNotWorkerSet(NRC_DEMAND d, NRC_WORKER_SET ws,
  NRC_PENALTY_TYPE ptype, NRC_PENALTY p);

/* ***
extern void NrcInstanceDemandBegin(NRC_INSTANCE ins);
extern void NrcInstanceDemandPenalizeNonAssignment(NRC_INSTANCE ins,
  NRC_PENALTY_TYPE ptype, NRC_PENALTY p);
extern void NrcInstanceDemandPenalizeWorkerSet(NRC_INSTANCE ins,
  NRC_WORKER_SET ws, NRC_PENALTY_TYPE ptype, NRC_PENALTY p);
extern void NrcInstanceDemandPenalizeNotWorkerSet(NRC_INSTANCE ins,
  NRC_WORKER_SET ws, NRC_PENALTY_TYPE ptype, NRC_PENALTY p);
extern NRC_DEMAND NrcInstanceDemandEnd(NRC_INSTANCE ins);
*** */

extern char *NrcDemandName(NRC_DEMAND d);
extern NRC_INSTANCE NrcDemandInstance(NRC_DEMAND d);
extern int NrcDemandPenalizerCount(NRC_DEMAND d);
extern void NrcDemandPenalizer(NRC_DEMAND d, int i, NRC_PENALIZER_TYPE *pt,
  NRC_WORKER_SET *ws, NRC_PENALTY_TYPE *ptype, NRC_PENALTY *p);
extern NRC_PENALTY NrcDemandWorkerPenalty(NRC_DEMAND d, NRC_WORKER w);
extern void NrcDemandDebug(NRC_DEMAND d, int multiplicity,
  int indent, FILE *fp);

/* 3.9.4 Demand-sets */
extern NRC_DEMAND_SET NrcDemandSetMake(NRC_INSTANCE ins);
extern NRC_INSTANCE NrcDemandSetInstance(NRC_DEMAND_SET ds);
extern void NrcDemandSetAddDemand(NRC_DEMAND_SET ds, NRC_DEMAND d);
extern void NrcDemandSetAddDemandMulti(NRC_DEMAND_SET ds, NRC_DEMAND d,
  int multiplicity);

extern NRC_DEMAND_SET NrcDemandSetMakeFromBound(NRC_INSTANCE ins, NRC_BOUND b,
  int count, NRC_WORKER_SET preferred_ws, NRC_PENALTY not_preferred_penalty);

extern int NrcDemandSetDemandCount(NRC_DEMAND_SET ds);
extern NRC_DEMAND NrcDemandSetDemand(NRC_DEMAND_SET ds, int i);

extern void NrcDemandSetDebug(NRC_DEMAND_SET ds, int indent, FILE *fp);

/* 3.9.5 Demand constraints */
extern NRC_DEMAND_CONSTRAINT NrcDemandConstraintMake(NRC_BOUND b,
  NRC_SHIFT_SET ss, NRC_WORKER_SET ws, char *name);

/* ***
extern bool NrcDemandConstraintAddMin(NRC_DEMAND_CONSTRAINT dc,
  int min_number, NRC_PENALTY below_min_penalty);
extern bool NrcDemandConstraintAddMax(NRC_DEMAND_CONSTRAINT dc,
  int max_number, NRC_PENALTY above_max_penalty);
extern bool NrcDemandConstraintAddPreferred(NRC_DEMAND_CONSTRAINT dc,
  int pref_number, NRC_PENALTY below_pref_penalty,
  NRC_PENALTY above_pref_penalty);
*** */

extern NRC_BOUND NrcDemandConstraintBound(NRC_DEMAND_CONSTRAINT dc);
extern NRC_SHIFT_SET NrcDemandConstraintShiftSet(NRC_DEMAND_CONSTRAINT dc);
extern NRC_WORKER_SET NrcDemandConstraintWorkerSet(NRC_DEMAND_CONSTRAINT dc);
extern char *NrcDemandConstraintName(NRC_DEMAND_CONSTRAINT dc);
extern void NrcDemandConstraintDebug(NRC_DEMAND_CONSTRAINT dc,
  int indent, FILE *fp);

/* ***
extern bool NrcDemandConstraintMin(NRC_DEMAND_CONSTRAINT dc,
  int *min_number, NRC_PENALTY *below_min_penalty);
extern bool NrcDemandConstraintMax(NRC_DEMAND_CONSTRAINT dc,
  int *max_number, NRC_PENALTY *above_max_penalty);
extern bool NrcDemandConstraintPreferred(NRC_DEMAND_CONSTRAINT dc,
  int *pref_number, NRC_PENALTY *below_pref_penalty,
  NRC_PENALTY *above_pref_penalty);
*** */

/* 3.9.6 Patterns */
extern NRC_PATTERN NrcPatternMake(NRC_INSTANCE ins, char *name);
extern NRC_INSTANCE NrcPatternInstance(NRC_PATTERN p);
extern int NrcPatternIndexInInstance(NRC_PATTERN p);
extern char *NrcPatternName(NRC_PATTERN p);
extern void NrcPatternAddTerm(NRC_PATTERN p, NRC_SHIFT_TYPE_SET sts,
  NRC_POLARITY po);
extern int NrcPatternTermCount(NRC_PATTERN p);
extern void NrcPatternTerm(NRC_PATTERN p, int i, NRC_SHIFT_TYPE_SET *sts,
  NRC_POLARITY *po);

extern bool NrcPatternIsUniform(NRC_PATTERN p);
extern bool NrcPatternMerge(NRC_PATTERN dest_p, NRC_PATTERN source_p);

extern NRC_POLARITY NrcPolarityNegate(NRC_POLARITY po);

extern void NrcPatternDebug(NRC_PATTERN p, int indent, FILE *fp);

/* ***
extern NRC_PATTERN NrcPatternMake(NRC_INSTANCE ins, char *name, NRC_DAY_SET ds);
extern char *NrcPatternName(NRC_PATTERN p);
extern NRC_DAY_SET NrcPatternDaySet(NRC_PATTERN p);
extern void NrcPatternAddTerm(NRC_PATTERN p, NRC_TERM t);
extern NRC_TERM NrcTermMake(bool any, bool free);
extern void NrcTermAddShiftSet(NRC_TERM t, NRC_SHIFT_SET ss);

extern NRC_TERM NrcInstanceAnyTerm(NRC_INSTANCE ins);
extern NRC_TERM NrcInstanceFreeTerm(NRC_INSTANCE ins);
extern NRC_TERM NrcInstanceAnyOrFreeTerm(NRC_INSTANCE ins);
*** */

/* 3.9.7 Pattern sets */
extern NRC_PATTERN_SET NrcPatternSetMake(NRC_INSTANCE ins);
extern NRC_INSTANCE NrcPatternSetInstance(NRC_PATTERN_SET ps);
extern int NrcPatternSetIndexInInstance(NRC_PATTERN_SET ps);
extern void NrcPatternSetAddPattern(NRC_PATTERN_SET ps, NRC_PATTERN p);
extern int NrcPatternSetPatternCount(NRC_PATTERN_SET ps);
extern NRC_PATTERN NrcPatternSetPattern(NRC_PATTERN_SET ps, int i);
extern NRC_PATTERN_SET NrcPatternSetReduce(NRC_PATTERN_SET ps);
extern void NrcPatternSetMergePattern(NRC_PATTERN_SET ps, NRC_PATTERN p);
extern void NrcPatternSetDebug(NRC_PATTERN_SET ps, int indent, FILE *fp);

/* 3.9.8 Worker constraints */
extern NRC_CONSTRAINT NrcConstraintMake(NRC_INSTANCE ins, char *name,
  NRC_WORKER_SET ws, NRC_CONSTRAINT_TYPE type, NRC_BOUND bound,
  NRC_SHIFT_SET starting_ss);
/* ***
extern NRC_CONSTRAINT NrcConstraintMake(NRC_INSTANCE ins, char *name,
  NRC_WORKER_SET ws, NRC_PENALTY penalty, NRC_LIMIT limit,
  NRC_SHIFT_SET starting_ss);
*** */
extern NRC_INSTANCE NrcConstraintInstance(NRC_CONSTRAINT c);
extern char *NrcConstraintName(NRC_CONSTRAINT c);
extern NRC_WORKER_SET NrcConstraintWorkerSet(NRC_CONSTRAINT c);
extern NRC_CONSTRAINT_TYPE NrcConstraintType(NRC_CONSTRAINT c);
/* ***
extern NRC_PENALTY NrcConstraintPenalty(NRC_CONSTRAINT c);
extern NRC_LIMIT NrcConstraintLimit(NRC_CONSTRAINT c);
*** */
extern NRC_SHIFT_SET NrcConstraintStartingShiftSet(NRC_CONSTRAINT c);

extern void NrcConstraintAddShiftSet(NRC_CONSTRAINT c,
  NRC_SHIFT_SET ss, NRC_POLARITY po);
extern void NrcConstraintAddShiftSetSet(NRC_CONSTRAINT c,
  NRC_SHIFT_SET_SET sss, NRC_POLARITY po);
extern void NrcConstraintAddPattern(NRC_CONSTRAINT c, NRC_PATTERN p, NRC_DAY d);

extern int NrcConstraintShiftSetCount(NRC_CONSTRAINT c);
extern void NrcConstraintShiftSet(NRC_CONSTRAINT c, int i,
  NRC_SHIFT_SET *ss, NRC_POLARITY *po);

extern NRC_CONSTRAINT NrcUnwantedPatternConstraintMake(NRC_INSTANCE ins,
  char *name, NRC_WORKER_SET ws, NRC_PENALTY penalty, NRC_PATTERN p,
  NRC_DAY_SET starting_ds);

extern void NrcConstraintAddHistory(NRC_CONSTRAINT c, int history_before,
  int history_after);
extern void NrcConstraintAddHistoryWorker(NRC_CONSTRAINT c, NRC_WORKER w,
  int value);
extern void NrcConstraintAddHistoryAllWorkers(NRC_CONSTRAINT c,
  int history_before, int history_after, char *name);

extern void NrcConstraintDebug(NRC_CONSTRAINT c, int indent, FILE *fp);

/* 3.9.8 Limits */
/* ***
extern NRC_LIMIT NrcLimit(NRC_LIMIT_TYPE type, int limit);
extern NRC_LIMIT NrcLimit2(NRC_LIMIT_TYPE type, int min_limit, int max_limit);
extern NRC_LIMIT_TYPE NrcLimitType(NRC_LIMIT limit);
extern int NrcLimitMinLimit(NRC_LIMIT limit);
extern int NrcLimitMaxLimit(NRC_LIMIT limit);

extern char *NrcLimitTypeShow(NRC_LIMIT_TYPE limit_type);
extern char *NrcLimitShow(NRC_LIMIT limit);
*** */


/*****************************************************************************/
/*                                                                           */
/*  Chapter 4.   Solutions                                                   */
/*                                                                           */
/*****************************************************************************/

/* construction and query */
extern NRC_SOLN NrcSolnMake(NRC_INSTANCE ins, HA_ARENA_SET as);
/* extern void NrcSolnDelete(NRC_SOLN soln); */
extern NRC_INSTANCE NrcSolnInstance(NRC_SOLN soln);
extern void NrcSolnSetDescription(NRC_SOLN soln, char *description);
extern char *NrcSolnDescription(NRC_SOLN soln);
extern void NrcSolnSetRunningTime(NRC_SOLN soln, float running_time);
extern float NrcSolnRunningTime(NRC_SOLN soln);

/* assignments */
extern void NrcSolnAddAssignment(NRC_SOLN soln, NRC_SHIFT s, int i,
  NRC_WORKER w);
extern NRC_WORKER NrcSolnAssignment(NRC_SOLN soln, NRC_SHIFT s, int i);

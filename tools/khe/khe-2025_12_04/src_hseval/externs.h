
/*****************************************************************************/
/*                                                                           */
/*  THE HSEVAL HIGH SCHOOL TIMETABLE EVALUATOR                               */
/*  COPYRIGHT (C) 2009, Jeffrey H. Kingston                                  */
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
#ifndef EXTERNS_HEADER_FILE
#define EXTERNS_HEADER_FILE
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "khe_platform.h"
#include "howard_a.h"
#include "howard_n.h"
#include "kml.h"
#include "url.h"
#include "html.h"
#include "command.h"
#include "table.h"
#include "eval_table.h"

#define ALLOW_CORE_DUMP 1

/* print formats */
typedef enum {
  PRINT_HTML,
  PRINT_LATEX,
} PRINT_FORMAT;

/* days and weeks */
/* ***
typedef struct week_rec *WEEK;
typedef HA_ARRAY(WEEK) ARRAY_WEEK;
typedef struct day_rec *DAY;
typedef HA_ARRAY(DAY) ARRAY_DAY;
*** */

/* defect_info */
typedef struct defect_info_rec *DEFECT_INFO;

/* various arrays */
typedef HA_ARRAY(KHE_TIME) ARRAY_KHE_TIME;
typedef HA_ARRAY(KHE_EVENT) ARRAY_KHE_EVENT;
typedef HA_ARRAY(KHE_MONITOR) ARRAY_KHE_MONITOR;
typedef HA_ARRAY(KHE_CONSTRAINT) ARRAY_KHE_CONSTRAINT;
typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK;
typedef HA_ARRAY(ARRAY_KHE_TASK) ARRAY_ARRAY_KHE_TASK;

/* model */
typedef struct model_rec *MODEL;
typedef struct model_resource_type_rec *MODEL_RESOURCE_TYPE;


/*****************************************************************************/
/*                                                                           */
/*  External functions - model.c                                             */
/*                                                                           */
/*****************************************************************************/

/* model resource type */
extern MODEL_RESOURCE_TYPE ModelResourceTypeMake(char *init_name, HA_ARENA a);
extern char *ModelResourceTypeInitName(MODEL_RESOURCE_TYPE mrt);
extern void ModelResourceTypeAddName(MODEL_RESOURCE_TYPE mrt, char *name);
extern bool ModelResourceTypeContainsName(MODEL_RESOURCE_TYPE mrt, char *name);

/* model */
/* extern MODEL ModelMake(void); */
extern char *ModelInstancesSummaryHeader(MODEL model);
extern void ModelAddModelResourceType(MODEL model, MODEL_RESOURCE_TYPE mrt);
extern int ModelModelResourceTypeCount(MODEL model);
extern MODEL_RESOURCE_TYPE ModelModelResourceType(MODEL model, int i);
extern MODEL ModelBuild(KHE_MODEL model, HA_ARENA a);
extern MODEL_RESOURCE_TYPE ModelRetrieve(MODEL model, char *name);


/*****************************************************************************/
/*                                                                           */
/*  External functions - main.c                                              */
/*                                                                           */
/*****************************************************************************/

extern void ArchiveReportInvalidSolns(KHE_ARCHIVE archive, HTML html);
/* extern void Error(COMMAND c, char *text); */
extern HTML PageBegin(char *title);
extern void PageEnd(HTML html);
extern KHE_ARCHIVE ReadAndVerifyArchive(COMMAND c, bool allow_invalid_solns,
  bool resource_type_partitions, KHE_SOLN_TYPE soln_type, HA_ARENA_SET as);


/*****************************************************************************/
/*                                                                           */
/*  External functions - defects.c                                           */
/*                                                                           */
/*****************************************************************************/

extern DEFECT_INFO DefectInfoMake(KHE_SOLN soln, char *defects_str);
extern bool DefectPresent(DEFECT_INFO di, KHE_RESOURCE r, KHE_TIME_GROUP tg);


/*****************************************************************************/
/*                                                                           */
/*  External functions - timetable.c                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
extern void ArchiveTimetablesHTML(KHE_ARCHIVE archive, bool with_event_groups,
  HTML html, HA_ARENA_SET as);
extern void ArchivePlanningTimetablesHTML(KHE_ARCHIVE archive,
  bool highlight_splits, HTML html);
extern void PlanningTimetablesHTML(COMMAND c, bool highlight_splits,
  HA_ARENA_SET as);
*** */
extern void PlanningTimetablesHTML2(COMMAND c, bool highlight_splits,
  HA_ARENA_SET as);

extern void Timetables(COMMAND c, bool planning, bool highlight_splits,
  bool with_event_groups, char *constraints_str, HA_ARENA_SET as);


/*****************************************************************************/
/*                                                                           */
/*  External functions - avail.c                                             */
/*                                                                           */
/*****************************************************************************/

extern void AvailabilityReportHTML(COMMAND c, HA_ARENA_SET as);

/* *** obsolete
extern void ResourceAvail(KHE_SOLN soln, KHE_RESOURCE r, KHE_FRAME frame,
  HTML html, int *avail_times, float *avail_workload);
*** */


/*****************************************************************************/
/*                                                                           */
/*  External functions - glossary.c                                          */
/*                                                                           */
/*****************************************************************************/

extern void Glossary(void);


/*****************************************************************************/
/*                                                                           */
/*  External functions - availability.c                                      */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete
extern void Availability(void);
*** */


/*****************************************************************************/
/*                                                                           */
/*  External functions - spec.c                                              */
/*                                                                           */
/*****************************************************************************/

extern void SpecMain(void);
extern void ConstraintSpecFull(void);
extern void EmployeeSpecFull(void);


/*****************************************************************************/
/*                                                                           */
/*  External functions - summary.c                                           */
/*                                                                           */
/*****************************************************************************/

/* extern int ResourceTypeNamesIndex(char *rtype_name); */
extern TABLE EvalTableBuild(KHE_ARCHIVE archive, bool show_best_costs,
  bool soft_costs_only, bool average_costs, bool show_relative,
  bool show_times, bool show_averages, char *instance_id,
  bool show_best_count, bool *non_empty, HA_ARENA a);
/* ***
extern TABLE RunningTimeTableBuild(KHE_ARCHIVE archive,
  bool show_average, bool *non_empty);
*** */
extern void SummaryEvalPrint(KHE_ARCHIVE archive, PRINT_FORMAT print_format,
  HA_ARENA_SET as);


/*****************************************************************************/
/*                                                                           */
/*  External functions - ranking.c                                           */
/*                                                                           */
/*****************************************************************************/

extern void RankingEvalPrint(KHE_ARCHIVE archive, PRINT_FORMAT print_format,
  HA_ARENA_SET as);


/*****************************************************************************/
/*                                                                           */
/*  External functions - report.c                                            */
/*                                                                           */
/*****************************************************************************/

extern void MonitorsReportHTML(ARRAY_KHE_MONITOR *monitors, bool with_summary,
  HTML html, HA_ARENA a);
/* ***
extern void MonitorReportHTML(KHE_MONITOR m, int *points, KHE_COST *cost,
  HTML html);
*** */
extern void MonitorLowerBoundReportHTML(KHE_MONITOR m, bool applies_to_first,
  int *points, KHE_COST *cost, HTML html);
extern void SolnInvalidParagraph(KHE_SOLN soln, HTML html);
extern void ArchiveReportHTML(KHE_ARCHIVE archive, HTML html, HA_ARENA_SET as);


/*****************************************************************************/
/*                                                                           */
/*  External functions - week.c                                              */
/*                                                                           */
/*****************************************************************************/

/* weeks */
/* ***
extern void WeeksAddDay(ARRAY_WEEK *weeks, DAY day);
extern void WeeksAddTime(ARRAY_WEEK *weeks, KHE_TIME time);
extern bool WeeksMakeSense(ARRAY_WEEK *weeks);
extern bool WeekTimetableSkeleton(KHE_INSTANCE ins, ARRAY_WEEK *weeks,
  HA_ARENA a);
extern void WeeksDebug(ARRAY_WEEK *weeks, int indent, FILE *fp);

extern WEEK WeekMake(KHE_TIME_GROUP tg, HA_ARENA a);
extern KHE_TIME_GROUP WeekTimeGroup(WEEK week);
extern int WeekDayCount(WEEK week);
extern DAY WeekDay(WEEK week, int day_index);
extern int WeekMaxTimesPerDayCount(WEEK week);
extern KHE_TIME WeekTime(WEEK week, int day_index, int time_index);

extern DAY DayMake(KHE_TIME_GROUP tg);
extern KHE_TIME_GROUP DayTimeGroup(DAY day);
*** */

#endif

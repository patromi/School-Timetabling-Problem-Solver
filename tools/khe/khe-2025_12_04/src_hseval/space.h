
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
/*  FILE:         space.h                                                    */
/*  MODULE:       Spaces                                                     */
/*                                                                           */
/*****************************************************************************/
#include "html.h"
#include "khe_platform.h"

typedef struct space_rec *SPACE;
typedef struct space_time_dim_rec *SPACE_TIME_DIM;
typedef struct space_resource_dim_rec *SPACE_RESOURCE_DIM;

typedef enum {
  SPACE_CELL_TIME_GROUP_EMPTY,
  SPACE_CELL_TIME_GROUP_NON_EMPTY,
  SPACE_CELL_RESOURCE_GROUP_UNAVAIL_HARD,
  SPACE_CELL_RESOURCE_GROUP_UNAVAIL_SOFT,
  SPACE_CELL_RESOURCE_GROUP_UNAVAIL_PART,
  SPACE_CELL_RESOURCE_GROUP_MUST_BE_BUSY_HARD,
  SPACE_CELL_RESOURCE_GROUP_MUST_BE_BUSY_SOFT,
  /* SPACE_CELL_RESOURCE_GROUP_MUST_BE_BUSY_PART, */
  SPACE_CELL_RESOURCE_UNASSIGNED,
  SPACE_CELL_TASKS_NONE,
  SPACE_CELL_TASKS_ONE_OR_MORE,
  SPACE_CELL_TASKS_TWO_OR_MORE,
  SPACE_CELL_HAS_DEFECTIVE_TASK,
  SPACE_CELL_CONSTRAINT_VIOLATED,
  SPACE_CELL_CONSTRAINT_UNVIOLATED,
  SPACE_CELL_CONSTRAINT_SLACK,
  SPACE_CELL_UNREQUIRED_ASSIGNED
} SPACE_CELL_COND;

typedef enum {
  SPACE_TASK_UNREQUIRED_UNASSIGNED,
  SPACE_TASK_UNREQUIRED_ASSIGNED,
  SPACE_TASK_REQUIRED_UNASSIGNED,
  SPACE_TASK_REQUIRED_ASSIGNED,
  SPACE_TASK_DEFECTIVE,
} SPACE_TASK_COND;

typedef enum {
  SPACE_ERROR_DAY_IS_EMPTY,
  SPACE_ERROR_DAYS_NOT_DISJOINT,
  SPACE_ERROR_DAYS_DO_NOT_COVER_CYCLE,
  SPACE_ERROR_WEEK_IS_EMPTY,
  SPACE_ERROR_WEEKS_NOT_DISJOINT,
  SPACE_ERROR_WEEKS_DO_NOT_COVER_CYCLE,
  SPACE_ERROR_DAY_INTERSECTS_TWO_WEEKS
} SPACE_ERROR_TYPE;

typedef void (*SPACE_RG_FN)(KHE_RESOURCE_GROUP rg, KHE_SOLN soln,
  int total_durn, float total_workload, void *impl, HTML html, HA_ARENA a);

/* space errors */
extern int SpaceErrorCount(SPACE space);
extern SPACE_ERROR_TYPE SpaceError(SPACE space, int i,
  KHE_TIME_GROUP *tg1, KHE_TIME_GROUP *tg2, KHE_TIME_GROUP *tg3);
extern bool SpaceHasDays(SPACE space);
extern bool SpaceHasWeeks(SPACE space);

/* defining spaces and dimensions */
extern SPACE SpaceMake(KHE_SOLN soln, bool allow_spanning,
  char *dft_colour, bool dft_show, HTML_FONT dft_font);
extern void SpaceDelete(SPACE space);
extern SPACE_TIME_DIM SpaceAddTimeDim(SPACE space);
extern void SpaceTimeDimAddTimeGroup(SPACE_TIME_DIM std,
  KHE_TIME_GROUP tg, char *label);
extern SPACE_RESOURCE_DIM SpaceAddResourceDim(SPACE space,
  SPACE_RG_FN major_heading_rg_fn, SPACE_RG_FN avail_rg_fn,
  void *avail_rg_impl, SPACE_RG_FN after_rg_fn);
extern void SpaceResourceDimAddResourceGroup(SPACE_RESOURCE_DIM srd,
  KHE_RESOURCE_GROUP rg, char *label);

/* helper functions for frequently used dimensions */
extern void SpaceAddResourceTypeDim(SPACE space, KHE_RESOURCE_TYPE rt,
  SPACE_RG_FN major_heading_rg_fn, SPACE_RG_FN avail_rg_fn,
  void *avail_rg_impl, SPACE_RG_FN after_rg_fn);
extern void SpaceAddTimesOfCycleDim(SPACE space);
extern void SpaceAddTimesOfDayDim(SPACE space);
extern void SpaceAddDaysOfWeekDim(SPACE space);
extern void SpaceAddDaysOfCycleDim(SPACE space);
extern void SpaceAddWeeksOfCycleDim(SPACE space);
extern void SpaceAddDummyDim(SPACE space);

/* tasks */
extern void SpaceAddTask(SPACE space, KHE_TASK task);
extern void SpaceAddAllTasks(SPACE space);

/* background colour and font functions */
extern void SpaceCellFormat(SPACE space, SPACE_CELL_COND cc, char *colour);
extern void SpaceCellFormat2(SPACE space, SPACE_CELL_COND cc,
  char *constraints_str, char *colour);
extern void SpaceTaskFormat(SPACE space, SPACE_TASK_COND tc,
  bool show, HTML_FONT font);

/* display */
extern void SpaceDisplay(SPACE space, int *total_italic, HTML html);

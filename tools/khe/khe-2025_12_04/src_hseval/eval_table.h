
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
/*  FILE:         eval_table.h                                               */
/*  MODULE:       Building evaluation tables                                 */
/*                                                                           */
/*****************************************************************************/
#ifndef EVAL_TABLE_HEADER_FILE
#define EVAL_TABLE_HEADER_FILE
#include "externs.h"

typedef enum {
  EVAL_COLUMN_DATA_AVERAGE_COST,
  EVAL_COLUMN_DATA_RELATIVE_AVERAGE_COST,
  EVAL_COLUMN_DATA_BEST_COST,
  EVAL_COLUMN_DATA_RELATIVE_BEST_COST,
  EVAL_COLUMN_DATA_AVERAGE_RUNNING_TIME,
  EVAL_COLUMN_DATA_RELATIVE_AVERAGE_RUNNING_TIME,
  EVAL_COLUMN_DATA_BEST_RUNNING_TIME,
  EVAL_COLUMN_DATA_RELATIVE_BEST_RUNNING_TIME,
} EVAL_COLUMN_DATA_TYPE;

typedef struct eval_table_rec *EVAL_TABLE;

extern EVAL_TABLE EvalTableMake(KHE_ARCHIVE archive,
  char *show_averages_instance_id, HA_ARENA a);
extern void EvalTableAddColumnFormat(EVAL_TABLE et,
  EVAL_COLUMN_DATA_TYPE data_type, bool soft_costs_only);
extern void EvalTableClearColumnFormats(EVAL_TABLE et);
extern TABLE EvalTableMakeTable(EVAL_TABLE et, bool show_averages_row,
  bool show_bests_row);

#endif


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
/*  FILE:         eval_table.c                                               */
/*  MODULE:       Building evaluation tables                                 */
/*                                                                           */
/*****************************************************************************/
#include "eval_table.h"
#include "float.h"

#define NO_COST		-1
#define NO_FLOAT	-1.0
#define INF_FLOAT	FLT_MAX
#define	TIME_FMT	"%.1f"
#define	RELATIVE_FMT	"%.2f"

#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1	0
#define DEBUG2	0


/*****************************************************************************/
/*                                                                           */
/*  Types for holding individual values                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  EVAL_STATE - state of a value                                            */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  EVAL_STATE_OPEN,			/* still accumulating                */
  EVAL_STATE_VALUE_SET,			/* final value fixed                 */
  EVAL_STATE_VALUE_AND_RELATIVE_SET	/* finalized + compared with others  */
} EVAL_STATE;


/*****************************************************************************/
/*                                                                           */
/*  Type EVAL_AVERAGE_COST - a value which is an average cost                */
/*                                                                           */
/*****************************************************************************/

typedef struct eval_average_cost_rec {
  EVAL_STATE			state;
  int				no_of_included_values;
  KHE_COST			total_cost;
  KHE_COST			average_cost;
  bool				average_cost_is_min;
  float				relative_average_cost;
  int				no_of_bests;
} EVAL_AVERAGE_COST;


/*****************************************************************************/
/*                                                                           */
/*  Type EVAL_BEST_COST - a value which is a best cost                       */
/*                                                                           */
/*****************************************************************************/

typedef struct eval_best_cost_rec {
  EVAL_STATE			state;
  KHE_COST			best_cost;
  bool				best_cost_is_min;
  float				relative_best_cost;
} EVAL_BEST_COST;


/*****************************************************************************/
/*                                                                           */
/*  Type EVAL_AVERAGE_FLOAT - a value which is an average float              */
/*                                                                           */
/*****************************************************************************/

typedef struct eval_average_float_rec {
  EVAL_STATE			state;
  int				no_of_included_values;
  float				total_float;
  float				average_float;
  bool				average_float_is_min;
  float				relative_average_float;
  int				no_of_bests;
} EVAL_AVERAGE_FLOAT;


/*****************************************************************************/
/*                                                                           */
/*  Type EVAL_BEST_FLOAT - a value which is a best float                     */
/*                                                                           */
/*****************************************************************************/

typedef struct eval_best_float_rec {
  EVAL_STATE			state;
  float				best_float;
  bool				best_float_is_min;
  float				relative_best_float;
} EVAL_BEST_FLOAT;


/*****************************************************************************/
/*                                                                           */
/*  Type EVAL_SOLN_GROUP - a summary of the costs and running times of the   */
/*    solutions of one solution group for one instance.                      */
/*                                                                           */
/*****************************************************************************/

/* ***
typedef struct eval_soln_group_rec {
  KHE_SOLN_GROUP		soln_group;

  EVAL_AVERAGE_COST		average_cost;
  EVAL_BEST_COST		best_cost;
  EVAL_AVERAGE_FLOAT		average_time;
  EVAL_BEST_FLOAT		best_time;

  EVAL_AVERAGE_COST		average_average_cost;
  EVAL_AVERAGE_COST		average_best_cost;
  EVAL_AVERAGE_FLOAT		average_average_time;
  EVAL_AVERAGE_FLOAT		average_best_time;
} *EVAL_SOLN_GROUP;

typedef HA_ARRAY(EVAL_SOLN_GROUP) ARRAY_EVAL_SOLN_GROUP;
*** */


/*****************************************************************************/
/*                                                                           */
/*  Type EVAL_INSTANCE - costs and times for one instance                    */
/*                                                                           */
/*****************************************************************************/

/* ***
typedef struct eval_instance_rec {
  KHE_INSTANCE				ins;
  ARRAY_EVAL_SOLN_GROUP			soln_groups;
} *EVAL_INSTANCE;
*** */


/*****************************************************************************/
/*                                                                           */
/*  Types for holding a table of values                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  Type EVAL_ORDINARY_ENTRY - one ordinary (non-average) entry in the table */
/*                                                                           */
/*****************************************************************************/

typedef struct eval_ordinary_entry_rec {
  KHE_INSTANCE			ins;
  KHE_SOLN_GROUP		soln_group;
  KHE_SOLN_SET			soln_set;
  EVAL_AVERAGE_COST		average_cost;
  EVAL_BEST_COST		best_cost;
  EVAL_AVERAGE_FLOAT		average_time;
  EVAL_BEST_FLOAT		best_time;
} *EVAL_ORDINARY_ENTRY;

typedef HA_ARRAY(EVAL_ORDINARY_ENTRY) ARRAY_EVAL_ORDINARY_ENTRY;


/*****************************************************************************/
/*                                                                           */
/*  Type EVAL_AVERAGE_ENTRY - one average entry in the table                 */
/*                                                                           */
/*****************************************************************************/

typedef struct eval_average_entry_rec {
  KHE_SOLN_GROUP		soln_group;
  EVAL_AVERAGE_COST		average_average_cost;
  EVAL_AVERAGE_COST		average_best_cost;
  EVAL_AVERAGE_FLOAT		average_average_time;
  EVAL_AVERAGE_FLOAT		average_best_time;

  EVAL_AVERAGE_FLOAT		average_relative_average_cost;
  EVAL_AVERAGE_FLOAT		average_relative_best_cost;
  EVAL_AVERAGE_FLOAT		average_relative_average_time;
  EVAL_AVERAGE_FLOAT		average_relative_best_time;

} *EVAL_AVERAGE_ENTRY;

typedef HA_ARRAY(EVAL_AVERAGE_ENTRY) ARRAY_EVAL_AVERAGE_ENTRY;


/*****************************************************************************/
/*                                                                           */
/*  Type EVAL_ORDINARY_ROW - one ordinary (non-average) row in the table     */
/*                                                                           */
/*****************************************************************************/

typedef struct eval_ordinary_row_rec {
  KHE_INSTANCE			ins;
  ARRAY_EVAL_ORDINARY_ENTRY	ordinary_entries;
} *EVAL_ORDINARY_ROW;

typedef HA_ARRAY(EVAL_ORDINARY_ROW) ARRAY_EVAL_ORDINARY_ROW;


/*****************************************************************************/
/*                                                                           */
/*  Type EVAL_AVERAGE_ROW - one average row in the table                     */
/*                                                                           */
/*****************************************************************************/

typedef struct eval_average_row_rec {
  ARRAY_EVAL_AVERAGE_ENTRY	average_entries;
} *EVAL_AVERAGE_ROW;

typedef HA_ARRAY(EVAL_AVERAGE_ROW) ARRAY_EVAL_AVERAGE_ROW;


/*****************************************************************************/
/*                                                                           */
/*  Type EVAL_COL_FMT - the format (not the value) of one column             */
/*                                                                           */
/*****************************************************************************/

typedef struct eval_col_fmt_rec {
  EVAL_COLUMN_DATA_TYPE		data_type;
  bool				soft_costs_only;
} *EVAL_COL_FMT;

typedef HA_ARRAY(EVAL_COL_FMT) ARRAY_EVAL_COL_FMT;


/*****************************************************************************/
/*                                                                           */
/*  Type EVAL_COL - one column in the table (the format is held elsewhere)   */
/*                                                                           */
/*****************************************************************************/

typedef struct eval_col_rec {
  KHE_SOLN_GROUP		soln_group;
  ARRAY_EVAL_ORDINARY_ENTRY	ordinary_entries;
  EVAL_AVERAGE_ENTRY		average_entry;
} *EVAL_COL;

typedef HA_ARRAY(EVAL_COL) ARRAY_EVAL_COL;


/*****************************************************************************/
/*                                                                           */
/*  Type EVAL_TABLE - the whole table                                        */
/*                                                                           */
/*****************************************************************************/

struct eval_table_rec {

  /* free lists */
  ARRAY_EVAL_ORDINARY_ENTRY	ordinary_entry_free_list;
  ARRAY_EVAL_AVERAGE_ENTRY	average_entry_free_list;
  ARRAY_EVAL_ORDINARY_ROW	ordinary_row_free_list;
  ARRAY_EVAL_AVERAGE_ROW	average_row_free_list;
  ARRAY_EVAL_COL_FMT		col_fmt_free_list;
  ARRAY_EVAL_COL		col_free_list;

  /* non-tabular stuff */
  HA_ARENA			arena;
  KHE_ARCHIVE			archive;
  char				*show_averages_instance_id;
  ARRAY_EVAL_COL_FMT		eval_col_fmts;

  /* the actual table */
  ARRAY_EVAL_ORDINARY_ROW	ordinary_rows;
  EVAL_AVERAGE_ROW		average_row;
  ARRAY_EVAL_COL		columns;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "print helper functions"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void PrintCost(KHE_COST cost, bool is_min, bool soft_cost_only,
  TABLE_ROW row, HA_ARENA a)
{
  TABLE_ENTRY entry;  TABLE_FONT font;
  font = is_min ? TABLE_BOLD : TABLE_ROMAN;
  if( cost == NO_COST )
    entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, "", a);
  else if( !soft_cost_only )
    entry = TableEntryMakeDouble(TABLE_RIGHT, font, "%.5f",
      KheCostShow(cost), a);
  else if( KheHardCost(cost) > 0 )
    entry = TableEntryMakeText(TABLE_RIGHT, font, "inf.", a);
  else
    entry = TableEntryMakeInt(TABLE_RIGHT, font, "%d", KheSoftCost(cost), a);
  TableRowAddEntry(row, entry);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void PrintFloat(float val, bool is_min, bool allow_bold, char *fmt,
  TABLE_ROW row, HA_ARENA a)
{
  TABLE_ENTRY entry;  TABLE_FONT font;
  font = (allow_bold && is_min ? TABLE_BOLD : TABLE_ROMAN);
  if( val == NO_FLOAT )
    entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, "", a);
  else if( val == INF_FLOAT )
    entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, "inf.", a);
  else
    entry = TableEntryMakeDouble(TABLE_RIGHT, font, fmt, val, a);
  TableRowAddEntry(row, entry);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void PrintInt(int val, bool is_min, bool allow_bold,
  TABLE_ROW row, HA_ARENA a)
{
  TABLE_ENTRY entry;  TABLE_FONT font;
  font = (allow_bold && is_min ? TABLE_BOLD : TABLE_ROMAN);
  entry = TableEntryMakeInt(TABLE_RIGHT, font, "%d", val, a);
  TableRowAddEntry(row, entry);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "EVAL_AVERAGE_COST"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void EvalAverageCostInit(EVAL_AVERAGE_COST *eac)                         */
/*                                                                           */
/*  Initialize eac.  Not all fields are initialized by this function.        */
/*                                                                           */
/*****************************************************************************/

static void EvalAverageCostInit(EVAL_AVERAGE_COST *eac)
{
  eac->state = EVAL_STATE_OPEN;
  eac->no_of_included_values = 0;
  eac->total_cost = 0;
  eac->no_of_bests = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalAverageCostAddCost(EVAL_AVERAGE_COST *eac, KHE_COST cost)       */
/*                                                                           */
/*  Add one cost to eac; but make sure it's a real cost first.               */
/*                                                                           */
/*****************************************************************************/

static void EvalAverageCostAddCost(EVAL_AVERAGE_COST *eac, KHE_COST cost)
{
  HnAssert(eac->state == EVAL_STATE_OPEN,
    "EvalAverageCostAddCost internal error");
  if( cost != NO_COST )
  {
    eac->no_of_included_values++;
    eac->total_cost += cost;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalAverageCostSetValue(EVAL_AVERAGE_COST *eac)                     */
/*                                                                           */
/*  Finalize eac, including calculating the actual average.                  */
/*                                                                           */
/*****************************************************************************/

static void EvalAverageCostSetValue(EVAL_AVERAGE_COST *eac)
{
  HnAssert(eac->state == EVAL_STATE_OPEN,
    "EvalAverageCostSetValue internal error");
  if( eac->no_of_included_values == 0 )
    eac->average_cost = NO_COST;
  else
    eac->average_cost =
      KheCost(KheHardCost(eac->total_cost) / eac->no_of_included_values,
      KheSoftCost(eac->total_cost) / eac->no_of_included_values);
  eac->state = EVAL_STATE_VALUE_SET;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST EvalAverageCostValue(EVAL_AVERAGE_COST *eac)                    */
/*                                                                           */
/*  Return the value of eac, i.e. the average, or NO_COST if none.           */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static KHE_COST EvalAverageCostValue(EVAL_AVERAGE_COST *eac)
{
  HnAssert(eac->state >= EVAL_STATE_VALUE_SET,
    "EvalAverageCostValue internal error");
  return eac->average_cost;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalAverageCostAccumulateMin(EVAL_AVERAGE_COST *eac,
  KHE_COST *min_average_cost)
{
  HnAssert(eac->state >= EVAL_STATE_VALUE_SET,
    "EvalAverageCostAccumulateMin internal error");
  if( eac->average_cost != NO_COST )
  {
    if( *min_average_cost == NO_COST || eac->average_cost < *min_average_cost )
      *min_average_cost = eac->average_cost;
  }
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static float RelativeCost(KHE_COST num, KHE_COST denom)
{
  if( KheHardCost(num) > 0 )
  {
    if( KheHardCost(denom) > 0 )
    {
      /* KheHardCost(num) > 0 && KheHardCost(denom) > 0 */
      return (float) KheHardCost(num)/ (float) KheHardCost(denom);
    }
    else
    {
      /* KheHardCost(num) > 0 && KheHardCost(denom) == 0 */
      return INF_FLOAT;
    }
  }
  else
  {
    if( KheHardCost(denom) > 0 )
    {
      /* KheHardCost(num) == 0 && KheHardCost(denom) > 0 */
      return 0.0;
    }
    else
    {
      /* KheHardCost(num) == 0 && KheHardCost(denom) == 0 */
      if( KheSoftCost(denom) > 0 )
	return (float) KheSoftCost(num)/ (float) KheSoftCost(denom);
      else if( KheSoftCost(num) == 0 )
	return 1.0;          /* special case:  0/0 = 1.0 */
      else
	return INF_FLOAT;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalAverageCostSetRelative(EVAL_AVERAGE_COST *eac, KHE_COST best_cost)  */
/*                                                                           */
/*  Compare eac with best_cost.                                              */
/*                                                                           */
/*****************************************************************************/

static void EvalAverageCostSetRelative(EVAL_AVERAGE_COST *eac, KHE_COST best_cost)
{
  HnAssert(eac->state == EVAL_STATE_VALUE_SET,
    "EvalAverageCostSetRelative internal error 1");
  if( best_cost == NO_COST || eac->average_cost == NO_COST )
  {
    eac->average_cost_is_min = false;
    eac->relative_average_cost = NO_FLOAT;
  }
  else
  {
    eac->average_cost_is_min = (eac->average_cost <= best_cost);
    eac->relative_average_cost = RelativeCost(eac->average_cost, best_cost);
  }
  eac->state = EVAL_STATE_VALUE_AND_RELATIVE_SET;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalAverageCostReportToAverage(EVAL_AVERAGE_COST *eac,
  EVAL_AVERAGE_COST *average_eac)
{
  EvalAverageCostAddCost(average_eac, eac->average_cost);
  if( eac->average_cost_is_min )
    average_eac->no_of_bests++;
}

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
static void EvalAverageFloatAddFloat(EVAL_AVERAGE_FLOAT *eaf, float val);

static void EvalAverageCostReportRelativeToAverage(EVAL_AVERAGE_COST *eac,
  EVAL_AVERAGE_FLOAT *relative_average_eac)
{
  EvalAverageFloatAddFloat(relative_average_eac, eac->relative_average_cost);
  if( eac->average_cost_is_min )
    relative_average_eac->no_of_bests++;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalAverageCostPrint(EVAL_AVERAGE_COST *eac, bool soft_cost_only,
  TABLE_ROW row, HA_ARENA a)
{
  HnAssert(eac->state == EVAL_STATE_VALUE_AND_RELATIVE_SET,
    "EvalAverageCostPrint internal error");
  PrintCost(eac->average_cost, eac->average_cost_is_min, soft_cost_only,
    row, a);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalRelativeAverageCostPrint(EVAL_AVERAGE_COST *eac,
  TABLE_ROW row, HA_ARENA a)
{
  HnAssert(eac->state == EVAL_STATE_VALUE_AND_RELATIVE_SET,
    "EvalRelativeAverageCostPrint internal error");
  PrintFloat(eac->relative_average_cost, eac->average_cost_is_min,
    true, RELATIVE_FMT, row, a);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalAverageCostPrintNumberOfBests(EVAL_AVERAGE_COST *eac,
  TABLE_ROW row, HA_ARENA a)
{
  PrintInt(eac->no_of_bests, false, false, row, a);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "EVAL_BEST_COST"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void EvalBestCostInit(EVAL_BEST_COST *ebc)                               */
/*                                                                           */
/*  Initialize ebc.  Not all fields are initialized by this function.        */
/*                                                                           */
/*****************************************************************************/

static void EvalBestCostInit(EVAL_BEST_COST *ebc)
{
  ebc->state = EVAL_STATE_OPEN;
  ebc->best_cost = NO_COST;
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalBestCostAddCost(EVAL_BEST_COST *ebc, KHE_COST cost)             */
/*                                                                           */
/*  Add one cost to ebc; but make sure it's a real cost first.               */
/*                                                                           */
/*****************************************************************************/

static void EvalBestCostAddCost(EVAL_BEST_COST *ebc, KHE_COST cost)
{
  HnAssert(ebc->state == EVAL_STATE_OPEN, "EvalBestCostAddCost internal error");
  if( cost != NO_COST )
  {
    if( ebc->best_cost == NO_COST || cost < ebc->best_cost )
      ebc->best_cost = cost;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalBestCostSetValue(EVAL_BEST_COST *ebc)                           */
/*                                                                           */
/*  Finalize ebc.                                                            */
/*                                                                           */
/*****************************************************************************/

static void EvalBestCostSetValue(EVAL_BEST_COST *ebc)
{
  HnAssert(ebc->state == EVAL_STATE_OPEN,
    "EvalBestCostSetValue internal error");
  ebc->state = EVAL_STATE_VALUE_SET;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST EvalBestCostValue(EVAL_BEST_COST *ebc)                          */
/*                                                                           */
/*  Return the value of ebc, i.e. the average, or NO_COST if none.           */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static KHE_COST EvalBestCostValue(EVAL_BEST_COST *ebc)
{
  HnAssert(ebc->state >= EVAL_STATE_VALUE_SET,
    "EvalBestCostValue internal error");
  return ebc->best_cost;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalBestCostAccumulateMin(EVAL_BEST_COST *ebc,
  KHE_COST *min_best_cost)
{
  HnAssert(ebc->state >= EVAL_STATE_VALUE_SET,
    "EvalAverageCostAccumulateMin internal error");
  if( ebc->best_cost != NO_COST )
  {
    if( *min_best_cost == NO_COST || ebc->best_cost < *min_best_cost )
      *min_best_cost = ebc->best_cost;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalBestCostSetRelative(EVAL_BEST_COST *ebc, KHE_COST best_cost)    */
/*                                                                           */
/*  Compare ebc with best_cost.                                              */
/*                                                                           */
/*****************************************************************************/

static void EvalBestCostSetRelative(EVAL_BEST_COST *ebc, KHE_COST best_cost)
{
  HnAssert(ebc->state == EVAL_STATE_VALUE_SET,
    "EvalBestCostSetRelative internal error 1");
  if( best_cost == NO_COST || ebc->best_cost == NO_COST )
  {
    ebc->best_cost_is_min = false;
    ebc->relative_best_cost = NO_FLOAT;
  }
  else
  {
    ebc->best_cost_is_min = (ebc->best_cost <= best_cost);
    if( best_cost > 0 )
      ebc->relative_best_cost =
	(double) ebc->best_cost / (double) best_cost;
    else if( ebc->best_cost > 0 )
      ebc->relative_best_cost = INF_FLOAT;
    else
      ebc->relative_best_cost = 1.0;
  }
  ebc->state = EVAL_STATE_VALUE_AND_RELATIVE_SET;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalBestCostReportToAverage(EVAL_BEST_COST *ebc,
  EVAL_AVERAGE_COST *average_eac)
{
  EvalAverageCostAddCost(average_eac, ebc->best_cost);
  if( ebc->best_cost_is_min )
    average_eac->no_of_bests++;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/
static void EvalAverageFloatAddFloat(EVAL_AVERAGE_FLOAT *eaf, float val);

static void EvalBestCostReportRelativeToAverage(EVAL_BEST_COST *ebc,
  EVAL_AVERAGE_FLOAT *average_relative_best_cost)
{
  EvalAverageFloatAddFloat(average_relative_best_cost, ebc->relative_best_cost);
  if( ebc->best_cost_is_min )
    average_relative_best_cost->no_of_bests++;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalBestCostPrint(EVAL_BEST_COST *ebc, bool soft_cost_only,
  TABLE_ROW row, HA_ARENA a)
{
  HnAssert(ebc->state == EVAL_STATE_VALUE_AND_RELATIVE_SET,
    "EvalBestCostPrint internal error");
  PrintCost(ebc->best_cost, ebc->best_cost_is_min, soft_cost_only, row, a);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalRelativeBestCostPrint(EVAL_BEST_COST *ebc,
  TABLE_ROW row, HA_ARENA a)
{
  HnAssert(ebc->state == EVAL_STATE_VALUE_AND_RELATIVE_SET,
    "EvalRelativeBestCostPrint internal error");
  PrintFloat(ebc->relative_best_cost, ebc->best_cost_is_min,
    true, RELATIVE_FMT, row, a);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "EVAL_AVERAGE_FLOAT"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void EvalAverageFloatInit(EVAL_AVERAGE_FLOAT *eaf)                       */
/*                                                                           */
/*  Initialize eaf.  Not all fields are initialized by this function.        */
/*                                                                           */
/*****************************************************************************/

static void EvalAverageFloatInit(EVAL_AVERAGE_FLOAT *eaf)
{
  eaf->state = EVAL_STATE_OPEN;
  eaf->no_of_included_values = 0;
  eaf->total_float = 0;
  eaf->no_of_bests = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalAverageFloatAddFloat(EVAL_AVERAGE_FLOAT *eaf, float val)        */
/*                                                                           */
/*  Add one float to eaf; but make sure it's a real one first.               */
/*                                                                           */
/*****************************************************************************/

static void EvalAverageFloatAddFloat(EVAL_AVERAGE_FLOAT *eaf, float val)
{
  HnAssert(eaf->state == EVAL_STATE_OPEN,
    "EvalAverageFloatAddFloat internal error");
  if( val != NO_FLOAT )
  {
    eaf->no_of_included_values++;
    eaf->total_float += val;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalAverageFloatSetValue(EVAL_AVERAGE_FLOAT *eaf)                   */
/*                                                                           */
/*  Finalize eaf, including calculating the actual average.                  */
/*                                                                           */
/*****************************************************************************/

static void EvalAverageFloatSetValue(EVAL_AVERAGE_FLOAT *eaf)
{
  HnAssert(eaf->state == EVAL_STATE_OPEN,
    "EvalAverageFloatSetValue internal error");
  if( eaf->no_of_included_values == 0 )
    eaf->average_float = NO_FLOAT;
  else
    eaf->average_float = (eaf->total_float / eaf->no_of_included_values);
  eaf->state = EVAL_STATE_VALUE_SET;
}


/*****************************************************************************/
/*                                                                           */
/*  float EvalAverageFloatValue(EVAL_AVERAGE_FLOAT *eaf)                     */
/*                                                                           */
/*  Return the value of eaf, i.e. the average, or NO_FLOAT if none.          */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static float EvalAverageFloatValue(EVAL_AVERAGE_FLOAT *eaf)
{
  HnAssert(eaf->state >= EVAL_STATE_VALUE_SET,
    "EvalAverageFloatValue internal error");
  return eaf->average_float;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalAverageFloatAccumulateMin(EVAL_AVERAGE_FLOAT *eaf,
  float *min_average_float)
{
  HnAssert(eaf->state >= EVAL_STATE_VALUE_SET,
    "EvalAverageFloatAccumulateMin internal error");
  if( eaf->average_float != NO_FLOAT )
  {
    if( *min_average_float == NO_FLOAT ||
	eaf->average_float < *min_average_float )
      *min_average_float = eaf->average_float;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalAverageFloatSetRelative(EVAL_AVERAGE_FLOAT *eaf, float best_val)*/
/*                                                                           */
/*  Compare eaf with best_val.                                               */
/*                                                                           */
/*****************************************************************************/

static void EvalAverageFloatSetRelative(EVAL_AVERAGE_FLOAT *eaf, float best_val)
{
  HnAssert(eaf->state == EVAL_STATE_VALUE_SET,
    "EvalAverageFloatSetRelative internal error 1");
  if( best_val == NO_FLOAT || eaf->average_float == NO_FLOAT )
  {
    eaf->average_float_is_min = false;
    eaf->relative_average_float = NO_FLOAT;
  }
  else
  {
    eaf->average_float_is_min = (eaf->average_float <= best_val);
    if( best_val > 0 )
      eaf->relative_average_float = (eaf->average_float / best_val);
    else if( eaf->average_float > 0 )
      eaf->relative_average_float = INF_FLOAT;
    else
      eaf->relative_average_float = 1.0;
  }
  eaf->state = EVAL_STATE_VALUE_AND_RELATIVE_SET;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalAverageFloatReportToAverage(EVAL_AVERAGE_FLOAT *eaf,
  EVAL_AVERAGE_FLOAT *average_eaf)
{
  EvalAverageFloatAddFloat(average_eaf, eaf->average_float);
  if( eaf->average_float_is_min )
    average_eaf->no_of_bests++;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalAverageFloatReportRelativeToAverage(EVAL_AVERAGE_FLOAT *eaf,
  EVAL_AVERAGE_FLOAT *relative_average_eaf)
{
  EvalAverageFloatAddFloat(relative_average_eaf, eaf->relative_average_float);
  if( eaf->average_float_is_min )
    relative_average_eaf->no_of_bests++;
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalAverageFloatPrint(EVAL_AVERAGE_FLOAT *eaf, char *fmt,           */
/*    TABLE_ROW row, HA_ARENA a)                                             */
/*                                                                           */
/*****************************************************************************/

static void EvalAverageFloatPrint(EVAL_AVERAGE_FLOAT *eaf,
  bool allow_bold, char *fmt, TABLE_ROW row, HA_ARENA a)
{
  HnAssert(eaf->state == EVAL_STATE_VALUE_AND_RELATIVE_SET,
    "EvalAverageFloatPrint internal error");
  PrintFloat(eaf->average_float, eaf->average_float_is_min, allow_bold, fmt,
    row, a);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalRelativeAverageFloatPrint(EVAL_AVERAGE_FLOAT *eaf,
  bool allow_bold, char *fmt, TABLE_ROW row, HA_ARENA a)
{
  HnAssert(eaf->state == EVAL_STATE_VALUE_AND_RELATIVE_SET,
    "EvalAverageFloatPrint internal error");
  PrintFloat(eaf->relative_average_float, eaf->average_float_is_min,
    allow_bold, fmt, row, a);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalAverageFloatPrintNumberOfBests(EVAL_AVERAGE_FLOAT *eaf,
  TABLE_ROW row, HA_ARENA a)
{
  PrintInt(eaf->no_of_bests, false, false, row, a);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "EVAL_BEST_FLOAT"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void EvalBestFloatInit(EVAL_BEST_FLOAT *ebf)                             */
/*                                                                           */
/*  Initialize ebf.  Not all fields are initialized by this function.        */
/*                                                                           */
/*****************************************************************************/

static void EvalBestFloatInit(EVAL_BEST_FLOAT *ebf)
{
  ebf->state = EVAL_STATE_OPEN;
  ebf->best_float = NO_FLOAT;
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalBestFloatAddFloat(EVAL_BEST_FLOAT *ebf, float val)              */
/*                                                                           */
/*  Add one float to ebf; but make sure it's a real one first.               */
/*                                                                           */
/*****************************************************************************/

static void EvalBestFloatAddFloat(EVAL_BEST_FLOAT *ebf, float val)
{
  HnAssert(ebf->state == EVAL_STATE_OPEN,
    "EvalBestFloatAddFloat internal error");
  if( val != NO_FLOAT )
  {
    if( ebf->best_float == NO_FLOAT || val < ebf->best_float )
      ebf->best_float = val;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalBestFloatSetValue(EVAL_BEST_FLOAT *ebf)                         */
/*                                                                           */
/*  Finalize ebf.                                                            */
/*                                                                           */
/*****************************************************************************/

static void EvalBestFloatSetValue(EVAL_BEST_FLOAT *ebf)
{
  HnAssert(ebf->state == EVAL_STATE_OPEN,
    "EvalBestFloatSetValue internal error");
  ebf->state = EVAL_STATE_VALUE_SET;
}


/*****************************************************************************/
/*                                                                           */
/*  float EvalBestFloatValue(EVAL_BEST_FLOAT *ebf)                           */
/*                                                                           */
/*  Return the value of ebf, or NO_FLOAT if none.                            */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static float EvalBestFloatValue(EVAL_BEST_FLOAT *ebf)
{
  HnAssert(ebf->state >= EVAL_STATE_VALUE_SET,
    "EvalBestFloatValue internal error");
  return ebf->best_float;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalBestFloatAccumulateMin(EVAL_BEST_FLOAT *ebf,
  float *min_best_float)
{
  HnAssert(ebf->state >= EVAL_STATE_VALUE_SET,
    "EvalAverageFloatAccumulateMin internal error");
  if( ebf->best_float != NO_FLOAT )
  {
    if( *min_best_float == NO_FLOAT || ebf->best_float < *min_best_float )
      *min_best_float = ebf->best_float;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalBestFloatSetRelative(EVAL_BEST_FLOAT *ebf, float best_val)      */
/*                                                                           */
/*  Compare ebf with best_val.                                               */
/*                                                                           */
/*****************************************************************************/

static void EvalBestFloatSetRelative(EVAL_BEST_FLOAT *ebf, float best_val)
{
  HnAssert(ebf->state == EVAL_STATE_VALUE_SET,
    "EvalBestFloatSetRelative internal error 1");
  if( best_val == NO_FLOAT || ebf->best_float == NO_FLOAT )
  {
    ebf->best_float_is_min = false;
    ebf->relative_best_float = NO_FLOAT;
  }
  else
  {
    ebf->best_float_is_min = (ebf->best_float <= best_val);
    if( best_val > 0 )
      ebf->relative_best_float = (ebf->best_float / best_val);
    else if( ebf->best_float > 0 )
      ebf->relative_best_float = INF_FLOAT;
    else
      ebf->relative_best_float = 1.0;
  }
  ebf->state = EVAL_STATE_VALUE_AND_RELATIVE_SET;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalBestFloatReportToAverage(EVAL_BEST_FLOAT *ebf,
  EVAL_AVERAGE_FLOAT *average_eaf)
{
  EvalAverageFloatAddFloat(average_eaf, ebf->best_float);
  if( ebf->best_float_is_min )
    average_eaf->no_of_bests++;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalBestFloatReportRelativeToAverage(EVAL_BEST_FLOAT *ebf,
  EVAL_AVERAGE_FLOAT *relative_best_eaf)
{
  EvalAverageFloatAddFloat(relative_best_eaf, ebf->relative_best_float);
  if( ebf->best_float_is_min )
    relative_best_eaf->no_of_bests++;
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalBestFloatPrint(EVAL_BEST_FLOAT *ebf, char *fmt,                 */
/*    TABLE_ROW row, HA_ARENA a)                                             */
/*                                                                           */
/*****************************************************************************/

static void EvalBestFloatPrint(EVAL_BEST_FLOAT *ebf, bool allow_bold,
  char *fmt, TABLE_ROW row, HA_ARENA a)
{
  HnAssert(ebf->state == EVAL_STATE_VALUE_AND_RELATIVE_SET,
    "EvalBestFloatPrint internal error");
  PrintFloat(ebf->best_float, ebf->best_float_is_min, allow_bold, fmt, row, a);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalRelativeBestFloatPrint(EVAL_BEST_FLOAT *ebf, bool allow_bold,
  char *fmt, TABLE_ROW row, HA_ARENA a)
{
  HnAssert(ebf->state == EVAL_STATE_VALUE_AND_RELATIVE_SET,
    "EvalRelativeBestFloatPrint internal error");
  PrintFloat(ebf->relative_best_float, ebf->best_float_is_min,
    allow_bold, fmt, row, a);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "EVAL_COLUMN_DATA_TYPE"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *DataTypeTableHeader(EVAL_COLUMN_DATA_TYPE data_type)               */
/*                                                                           */
/*  Return a table header suited to this data type.                          */
/*                                                                           */
/*****************************************************************************/

static char *DataTypeTableHeader(EVAL_COLUMN_DATA_TYPE data_type)
{
  switch( data_type )
  {
    case EVAL_COLUMN_DATA_AVERAGE_COST:
    case EVAL_COLUMN_DATA_RELATIVE_AVERAGE_COST:

      return "Average costs";

    case EVAL_COLUMN_DATA_BEST_COST:
    case EVAL_COLUMN_DATA_RELATIVE_BEST_COST:

      return "Best costs";

    case EVAL_COLUMN_DATA_AVERAGE_RUNNING_TIME:
    case EVAL_COLUMN_DATA_RELATIVE_AVERAGE_RUNNING_TIME:

      return "Average running times";

    case EVAL_COLUMN_DATA_BEST_RUNNING_TIME:
    case EVAL_COLUMN_DATA_RELATIVE_BEST_RUNNING_TIME:

      return "Best running times";

    default:

      HnAbort("DataTypeTableHeader internal error");
      return "?";    /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *DataTypeEvalColumnHeader(EVAL_COLUMN_DATA_TYPE data_type)          */
/*                                                                           */
/*  Return the column header suited to this data type.                       */
/*                                                                           */
/*****************************************************************************/

static char *DataTypeEvalColumnHeader(EVAL_COLUMN_DATA_TYPE data_type)
{
  switch( data_type )
  {
    case EVAL_COLUMN_DATA_AVERAGE_COST:

      return "Cost";

    case EVAL_COLUMN_DATA_RELATIVE_AVERAGE_COST:

      return "Rel.";

    case EVAL_COLUMN_DATA_BEST_COST:

      return "Cost";

    case EVAL_COLUMN_DATA_RELATIVE_BEST_COST:

      return "Rel.";

    case EVAL_COLUMN_DATA_AVERAGE_RUNNING_TIME:

      return "Time";

    case EVAL_COLUMN_DATA_RELATIVE_AVERAGE_RUNNING_TIME:

      return "Rel.";

    case EVAL_COLUMN_DATA_BEST_RUNNING_TIME:

      return "Time";

    case EVAL_COLUMN_DATA_RELATIVE_BEST_RUNNING_TIME:

      return "Rel.";

    default:
      HnAbort("DataTypeEvalColumnHeader internal error");
      return "?";    /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "EVAL_ORDINARY_ENTRY"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  EVAL_ORDINARY_ENTRY EvalOrdinaryEntryMake(KHE_INSTANCE ins,              */
/*    KHE_SOLN_GROUP soln_group, KHE_SOLN_SET soln_set, EVAL_TABLE et)       */
/*                                                                           */
/*  Make an ordinary entry with these attributes.                            */
/*                                                                           */
/*****************************************************************************/

static EVAL_ORDINARY_ENTRY EvalOrdinaryEntryMake(KHE_INSTANCE ins,
  KHE_SOLN_GROUP soln_group, KHE_SOLN_SET soln_set, EVAL_TABLE et)
{
  EVAL_ORDINARY_ENTRY res;  int i;  KHE_SOLN soln;  KHE_COST cost;
  float running_time;

  /* initialize most of the object */
  if( HaArrayCount(et->ordinary_entry_free_list) > 0 )
    res = HaArrayLastAndDelete(et->ordinary_entry_free_list);
  else
    HaMake(res, et->arena);
  res->ins = ins;
  res->soln_group = soln_group;
  res->soln_set = soln_set;

  /* initialize the value fields, up to setting the value */
  EvalAverageCostInit(&res->average_cost);
  EvalBestCostInit(&res->best_cost);
  EvalAverageFloatInit(&res->average_time);
  EvalBestFloatInit(&res->best_time);
  for( i = 0;  i < KheSolnSetSolnCount(soln_set);  i++ )
  {
    soln = KheSolnSetSoln(soln_set, i);
    if( KheSolnType(soln) != KHE_SOLN_INVALID_PLACEHOLDER )
    {
      cost = KheSolnCost(soln);
      EvalAverageCostAddCost(&res->average_cost, cost);
      EvalBestCostAddCost(&res->best_cost, cost);
      if( KheSolnHasRunningTime(soln, &running_time) )
      {
	EvalAverageFloatAddFloat(&res->average_time, running_time);
	EvalBestFloatAddFloat(&res->best_time, running_time);
      }
    }
  }
  EvalAverageCostSetValue(&res->average_cost);
  EvalBestCostSetValue(&res->best_cost);
  EvalAverageFloatSetValue(&res->average_time);
  EvalBestFloatSetValue(&res->best_time);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalOrdinaryEntryPrint(EVAL_ORDINARY_ENTRY eoe, EVAL_COL_FMT ecf,   */
/*    TABLE_ROW row, HA_ARENA a)                                             */
/*                                                                           */
/*  Print eae in format ec onto row.                                         */
/*                                                                           */
/*****************************************************************************/

static void EvalOrdinaryEntryPrint(EVAL_ORDINARY_ENTRY eoe, EVAL_COL_FMT ecf,
  TABLE_ROW row, HA_ARENA a)
{
  switch( ecf->data_type )
  {
    case EVAL_COLUMN_DATA_AVERAGE_COST:

      EvalAverageCostPrint(&eoe->average_cost, ecf->soft_costs_only, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_AVERAGE_COST:

      EvalRelativeAverageCostPrint(&eoe->average_cost, row, a);
      break;

    case EVAL_COLUMN_DATA_BEST_COST:

      EvalBestCostPrint(&eoe->best_cost, ecf->soft_costs_only, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_BEST_COST:

      EvalRelativeBestCostPrint(&eoe->best_cost, row, a);
      break;

    case EVAL_COLUMN_DATA_AVERAGE_RUNNING_TIME:

      EvalAverageFloatPrint(&eoe->average_time, false, TIME_FMT, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_AVERAGE_RUNNING_TIME:

      EvalRelativeAverageFloatPrint(&eoe->average_time, false,
	RELATIVE_FMT, row, a);
      break;

    case EVAL_COLUMN_DATA_BEST_RUNNING_TIME:

      EvalBestFloatPrint(&eoe->best_time, false, TIME_FMT, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_BEST_RUNNING_TIME:

      EvalRelativeBestFloatPrint(&eoe->best_time, false, TIME_FMT, row, a);
      break;

    default:

      HnAbort("EvalOrdinaryEntryPrint internal error");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "EVAL_AVERAGE_ENTRY"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  EVAL_AVERAGE_ENTRY EvalAverageEntryMake(KHE_SOLN_GROUP soln_group,       */
/*    KHE_SOLN_SET soln_set, EVAL_TABLE et)                                  */
/*                                                                           */
/*  Make a new average entry object with these attributes.                   */
/*                                                                           */
/*****************************************************************************/

static EVAL_AVERAGE_ENTRY EvalAverageEntryMake(KHE_SOLN_GROUP soln_group,
  EVAL_TABLE et)
{
  EVAL_AVERAGE_ENTRY res;

  /* initialize the object and its non-value attributes */
  if( HaArrayCount(et->average_entry_free_list) > 0 )
    res = HaArrayLastAndDelete(et->average_entry_free_list);
  else
    HaMake(res, et->arena);
  res->soln_group = soln_group;

  /* initialize the average value fields */
  EvalAverageCostInit(&res->average_average_cost);
  EvalAverageCostInit(&res->average_best_cost);
  EvalAverageFloatInit(&res->average_average_time);
  EvalAverageFloatInit(&res->average_best_time);

  /* initialize the average relative value fields */
  EvalAverageFloatInit(&res->average_relative_average_cost);
  EvalAverageFloatInit(&res->average_relative_best_cost);
  EvalAverageFloatInit(&res->average_relative_average_time);
  EvalAverageFloatInit(&res->average_relative_best_time);

  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalAverageEntryPrint(EVAL_AVERAGE_ENTRY eae, EVAL_COL_FMT ecf,     */
/*    TABLE_ROW row, HA_ARENA a)                                             */
/*                                                                           */
/*  Print eae in format ec onto row.                                         */
/*                                                                           */
/*****************************************************************************/

static void EvalAverageEntryPrint(EVAL_AVERAGE_ENTRY eae, EVAL_COL_FMT ecf,
  TABLE_ROW row, HA_ARENA a)
{
  switch( ecf->data_type )
  {
    case EVAL_COLUMN_DATA_AVERAGE_COST:

      EvalAverageCostPrint(&eae->average_average_cost,
	ecf->soft_costs_only, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_AVERAGE_COST:

      EvalAverageFloatPrint(&eae->average_relative_average_cost,
	true, RELATIVE_FMT, row, a);
      break;

    case EVAL_COLUMN_DATA_BEST_COST:

      EvalAverageCostPrint(&eae->average_best_cost,
	ecf->soft_costs_only, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_BEST_COST:

      EvalAverageFloatPrint(&eae->average_relative_best_cost,
	true, RELATIVE_FMT, row, a);
      break;

    case EVAL_COLUMN_DATA_AVERAGE_RUNNING_TIME:

      EvalAverageFloatPrint(&eae->average_average_time,
	false, TIME_FMT, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_AVERAGE_RUNNING_TIME:

      EvalAverageFloatPrint(&eae->average_relative_average_time,
	true, RELATIVE_FMT, row, a);
      break;

    case EVAL_COLUMN_DATA_BEST_RUNNING_TIME:

      EvalAverageFloatPrint(&eae->average_best_time,
	false, TIME_FMT, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_BEST_RUNNING_TIME:

      EvalAverageFloatPrint(&eae->average_relative_best_time,
	true, RELATIVE_FMT, row, a);
      break;

    default:

      HnAbort("EvalAverageEntryPrint internal error");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalAverageEntryPrintNumberOfBests(EVAL_AVERAGE_ENTRY eae,          */
/*    EVAL_COL_FMT ecf, TABLE_ROW row, HA_ARENA a)                           */
/*                                                                           */
/*  Print the number of bests field of eac into row.                         */
/*                                                                           */
/*****************************************************************************/

static void EvalAverageEntryPrintNumberOfBests(EVAL_AVERAGE_ENTRY eae,
  EVAL_COL_FMT ecf, TABLE_ROW row, HA_ARENA a)
{
  switch( ecf->data_type )
  {
    case EVAL_COLUMN_DATA_AVERAGE_COST:

      EvalAverageCostPrintNumberOfBests(&eae->average_average_cost,
	row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_AVERAGE_COST:

      EvalAverageFloatPrintNumberOfBests(&eae->average_relative_average_cost,
	row, a);
      break;

    case EVAL_COLUMN_DATA_BEST_COST:

      EvalAverageCostPrintNumberOfBests(&eae->average_best_cost, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_BEST_COST:

      EvalAverageFloatPrintNumberOfBests(&eae->average_relative_best_cost,
	row, a);
      break;

    case EVAL_COLUMN_DATA_AVERAGE_RUNNING_TIME:

      EvalAverageFloatPrintNumberOfBests(&eae->average_average_time, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_AVERAGE_RUNNING_TIME:

      EvalAverageFloatPrintNumberOfBests(&eae->average_relative_average_time,
	row, a);
      break;

    case EVAL_COLUMN_DATA_BEST_RUNNING_TIME:

      EvalAverageFloatPrintNumberOfBests(&eae->average_best_time, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_BEST_RUNNING_TIME:

      EvalAverageFloatPrintNumberOfBests(&eae->average_relative_best_time,
	row, a);
      break;

    default:

      HnAbort("EvalAverageEntryPrint internal error");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "EVAL_ORDINARY_ROW"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  EVAL_ORDINARY_ROW EvalOrdinaryRowMake(KHE_INSTANCE ins, EVAL_TABLE et)   */
/*                                                                           */
/*  Make a new ordinary row object with these attributes and no entries.     */
/*                                                                           */
/*****************************************************************************/

static EVAL_ORDINARY_ROW EvalOrdinaryRowMake(KHE_INSTANCE ins, EVAL_TABLE et)
{
  EVAL_ORDINARY_ROW res;
  if( HaArrayCount(et->ordinary_row_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(et->ordinary_row_free_list);
    HaArrayClear(res->ordinary_entries);
  }
  else
  {
    HaMake(res, et->arena);
    HaArrayInit(res->ordinary_entries, et->arena);
  }
  res->ins = ins;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "EVAL_AVERAGE_ROW"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  EVAL_AVERAGE_ROW EvalAverageRowMake(EVAL_TABLE et)                       */
/*                                                                           */
/*  Make a new average row object with these attributes and no entries.      */
/*                                                                           */
/*****************************************************************************/

static EVAL_AVERAGE_ROW EvalAverageRowMake(EVAL_TABLE et)
{
  EVAL_AVERAGE_ROW res;
  if( HaArrayCount(et->average_row_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(et->average_row_free_list);
    HaArrayClear(res->average_entries);
  }
  else
  {
    HaMake(res, et->arena);
    HaArrayInit(res->average_entries, et->arena);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "EVAL_COL_FMT"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  EVAL_COL_FMT EvalColFmtMake(EVAL_COLUMN_DATA_TYPE data_type,             */
/*    bool soft_costs_only, EVAL_TABLE et)                                   */
/*                                                                           */
/*  Make and return a new column set object with these attributes.           */
/*                                                                           */
/*****************************************************************************/

static EVAL_COL_FMT EvalColFmtMake(EVAL_COLUMN_DATA_TYPE data_type,
  bool soft_costs_only, EVAL_TABLE et)
{
  EVAL_COL_FMT res;
  if( HaArrayCount(et->col_fmt_free_list) > 0 )
    res = HaArrayLastAndDelete(et->col_fmt_free_list);
  else
    HaMake(res, et->arena);
  res->data_type = data_type;
  res->soft_costs_only = soft_costs_only;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalColFmtAddSecondHeader(EVAL_COL_FMT ecf, TABLE_ROW row,          */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Add ecf's second-row header to row.                                      */
/*                                                                           */
/*****************************************************************************/

static void EvalColFmtAddSecondHeader(EVAL_COL_FMT ecf, TABLE_ROW row,
  HA_ARENA a)
{
  TABLE_ENTRY entry;
  entry = TableEntryMakeText(TABLE_RIGHT, TABLE_BOLD,
    DataTypeEvalColumnHeader(ecf->data_type), a);
  TableRowAddEntry(row, entry);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

/* ***
static void PrintRelative(float val, bool is_min, TABLE_ROW row, HA_ARENA a)
{
  TABLE_ENTRY entry;  TABLE_FONT font;
  font = is_min ? TABLE_BOLD : TABLE_ROMAN;
  if( val == NO_RELATIVE )
    entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, "", a);
  else
    entry = TableEntryMakeDouble(TABLE_RIGHT, font, "%.2f", val, a);
  TableRowAddEntry(row, entry);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

/* ***
static void PrintTime(float val, bool is_min, TABLE_ROW row, HA_ARENA a)
{
  TABLE_ENTRY entry;  TABLE_FONT font;
  font = is_min ? TABLE_BOLD : TABLE_ROMAN;
  if( val == MAX_TIME )
    entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, "", a);
  else
    entry = TableEntryMakeDouble(TABLE_RIGHT, font, "%.1f", val, a);
  TableRowAddEntry(row, entry);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

/* ***
static void EvalColFmtPrintValues(EVAL_COL_FMT ecf, EVAL_SOLN_GROUP esg,
  TABLE_ROW row, HA_ARENA a)
{
  switch( ecf->data_type )
  {
    case EVAL_COLUMN_DATA_AVERAGE_COST:

      EvalAverageCostPrint(&esg->average_cost, ecf->soft_costs_only, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_AVERAGE_COST:

      EvalRelativeAverageCostPrint(&esg->average_cost, row, a);
      break;

    case EVAL_COLUMN_DATA_BEST_COST:

      EvalBestCostPrint(&esg->best_cost, ecf->soft_costs_only, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_BEST_COST:

      EvalRelativeBestCostPrint(&esg->best_cost, row, a);
      break;

    case EVAL_COLUMN_DATA_AVERAGE_RUNNING_TIME:

      EvalAverageFloatPrint(&esg->average_time, false, TIME_FMT, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_AVERAGE_RUNNING_TIME:

      EvalRelativeAverageFloatPrint(&esg->average_time, false,
	RELATIVE_FMT, row, a);
      break;

    case EVAL_COLUMN_DATA_BEST_RUNNING_TIME:

      EvalBestFloatPrint(&esg->best_time, false, TIME_FMT, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_BEST_RUNNING_TIME:

      EvalRelativeBestFloatPrint(&esg->best_time, false, TIME_FMT, row, a);
      break;

    default:

      HnAbort("EvalColFmtPrintValues internal error");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

/* ***
static void EvalColFmtPrintAverageValues(EVAL_COL_FMT ecf,
  EVAL_SOLN_GROUP esg, TABLE_ROW row, HA_ARENA a)
{
  switch( ecf->data_type )
  {
    case EVAL_COLUMN_DATA_AVERAGE_COST:

      EvalAverageCostPrint(&esg->average_average_cost,
	ecf->soft_costs_only, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_AVERAGE_COST:

      ** sti ll to do **
      break;

    case EVAL_COLUMN_DATA_BEST_COST:

      EvalAverageCostPrint(&esg->average_best_cost,
	ecf->soft_costs_only, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_BEST_COST:

      ** sti ll to do **
      break;

    case EVAL_COLUMN_DATA_AVERAGE_RUNNING_TIME:

      EvalAverageFloatPrint(&esg->average_average_time,
	false, TIME_FMT, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_AVERAGE_RUNNING_TIME:

      ** sti ll to do **
      break;

    case EVAL_COLUMN_DATA_BEST_RUNNING_TIME:

      EvalAverageFloatPrint(&esg->average_best_time,
	false, TIME_FMT, row, a);
      break;

    case EVAL_COLUMN_DATA_RELATIVE_BEST_RUNNING_TIME:

      ** sti ll to do **
      break;

    default:

      HnAbort("EvalColFmtPrintValues internal error");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "EVAL_COL"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  EVAL_COL EvalColMake(KHE_SOLN_GROUP soln_group, EVAL_TABLE et)           */
/*                                                                           */
/*  Make a new eval col object with these attributes and no entries.         */
/*                                                                           */
/*****************************************************************************/

static EVAL_COL EvalColMake(KHE_SOLN_GROUP soln_group, EVAL_TABLE et)
{
  EVAL_COL res;
  if( HaArrayCount(et->col_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(et->col_free_list);
    HaArrayClear(res->ordinary_entries);
  }
  else
  {
    HaMake(res, et->arena);
    HaArrayInit(res->ordinary_entries, et->arena);
  }
  res->soln_group = soln_group;
  res->average_entry = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "EVAL_COST"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void EvalCostSetStage1(EVAL_COST cv, KHE_SOLN_SET ss)                    */
/*                                                                           */
/*  Set cv correspondong to ss.  The fields defined by this call are:        */
/*                                                                           */
/*    no_of_included_values                                                  */
/*    total_cost                                                             */
/*    average_cost                                                           */
/*    best_cost                                                              */
/*                                                                           */
/*  The other fields remain undefined when this function returns.            */
/*                                                                           */
/*****************************************************************************/

/* ***
static void EvalCostSetStage1(EVAL_COST *cv, KHE_SOLN_SET ss)
{
  int i;  KHE_SOLN soln;  KHE_COST cost;
  cv->no_of_included_values = 0;
  cv->total_cost = KheCost(0, 0);
  cv->best_cost = MAX_COST;
  for( i = 0;  i < KheSolnSetSolnCount(ss);  i++ )
  {
    soln = KheSolnSetSoln(ss, i);
    if( KheSolnType(soln) != KHE_SOLN_INVALID_PLACEHOLDER )
    {
      cv->no_of_included_values++;
      cost = KheSolnCost(soln);
      cv->total_cost += cost;
      if( cost < cv->best_cost )
	cv->best_cost = cost;
    }
  }
  if( cv->no_of_included_values > 0 )
    cv->average_cost =
      KheCost(KheHardCost(cv->total_cost) / cv->no_of_included_values,
      KheSoftCost(cv->total_cost) / cv->no_of_included_values);
  else
    cv->average_cost = MAX_COST;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void EvalCostSetStage2(EVAL_COST *cv, KHE_COST *min_average_cost,        */
/*    KHE_COST *min_best_cost)                                               */
/*                                                                           */
/*  Update *min_average_cost and *min_best_cost to reflect the values        */
/*  held in *cv.                                                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static void EvalCostSetStage2(EVAL_COST *cv, KHE_COST *min_average_cost,
  KHE_COST *min_best_cost)
{
  if( cv->no_of_included_values > 0 )
  {
    if( cv->average_cost < *min_average_cost )
      *min_average_cost = cv->average_cost;
    if( cv->best_cost < *min_best_cost )
      *min_best_cost = cv->best_cost;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  float RelativeCost(KHE_COST num, KHE_COST denom)                         */
/*                                                                           */
/*  Return num/denom, but taking care of various cases including absent      */
/*  values and zero denominator.  Return NO_RELATIVE when there is no        */
/*  clear value to return.                                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static float RelativeCost(KHE_COST num, KHE_COST denom)
{
  ** no result if either cost is undefined **
  if( num == MAX_COST || denom == MAX_COST )
    return NO_RELATIVE;

  ** separate into four cases depending on whether hard cost is zero **
  if( KheHardCost(num) > 0 )
  {
    if( KheHardCost(denom) > 0 )
    {
      ** KheHardCost(num) > 0 && KheHardCost(denom) > 0 **
      return (float) KheHardCost(num)/ (float) KheHardCost(denom);
    }
    else
    {
      ** KheHardCost(num) > 0 && KheHardCost(denom) == 0 **
      return NO_RELATIVE;
    }
  }
  else
  {
    if( KheHardCost(denom) > 0 )
    {
      ** KheHardCost(num) == 0 && KheHardCost(denom) > 0 **
      return 0.0;
    }
    else
    {
      ** KheHardCost(num) == 0 && KheHardCost(denom) == 0 **
      if( KheSoftCost(denom) > 0 )
	return (float) KheSoftCost(num)/ (float) KheSoftCost(denom);
      else if( KheSoftCost(num) == 0 )
	return 1.0;          ** special case:  0/0 = 1.0 **
      else
	return NO_RELATIVE;
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void EvalCostSetStage3(EVAL_COST *cv, KHE_COST min_average_cost,         */
/*    KHE_COST min_best_cost)                                                */
/*                                                                           */
/*  Set cv correspondong to ss.  The fields defined by this call are:        */
/*                                                                           */
/*    average_cost_is_min                                                    */
/*    relative_average_cost                                                  */
/*    best_cost_is_min                                                       */
/*    relative_best_cost                                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static void EvalCostSetStage3(EVAL_COST *cv, KHE_COST min_average_cost,
  KHE_COST min_best_cost)
{
  cv->average_cost_is_min =
    (cv->average_cost <= min_average_cost && cv->average_cost != MAX_COST);
  cv->relative_average_cost = RelativeCost(cv->average_cost, min_average_cost);
  cv->best_cost_is_min =
    (cv->best_cost <= min_best_cost && cv->best_cost != MAX_COST);
  cv->relative_best_cost = RelativeCost(cv->best_cost, min_best_cost);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "EVAL_TIME"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void EvalTimeSetStage1(EVAL_TIME *tv, KHE_SOLN_SET ss)                   */
/*                                                                           */
/*  Set cv correspondong to ss.  The fields defined by this call are:        */
/*                                                                           */
/*    no_of_included_values                                                  */
/*    total_time                                                             */
/*    best_time                                                              */
/*    average_time                                                           */
/*                                                                           */
/*  The other fields remain undefined when this function returns.            */
/*                                                                           */
/*****************************************************************************/

/* ***
static void EvalTimeSetStage1(EVAL_TIME *tv, KHE_SOLN_SET ss)
{
  int i;  KHE_SOLN soln;  float running_time;
  tv->no_of_included_values = 0;
  tv->total_time = 0.0;
  tv->best_time = MAX_TIME;
  for( i = 0;  i < KheSolnSetSolnCount(ss);  i++ )
  {
    soln = KheSolnSetSoln(ss, i);
    if( KheSolnType(soln) != KHE_SOLN_INVALID_PLACEHOLDER &&
        KheSolnHasRunningTime(soln, &running_time) )
    {
      tv->no_of_included_values++;
      tv->total_time += running_time;
      if( running_time < tv->best_time )
	tv->best_time = running_time;
    }
  }
  if( tv->no_of_included_values > 0 )
    tv->average_time = tv->total_time / tv->no_of_included_values;
  else
    tv->average_time = MAX_TIME;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void EvalTimeSetStage2(EVAL_TIME *tv, float *min_average_time,           */
/*    float *min_best_time)                                                  */
/*                                                                           */
/*  Update *min_average_time and *min_best_time to reflect the values        */
/*  held in *tv.                                                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static void EvalTimeSetStage2(EVAL_TIME *tv, float *min_average_time,
  float *min_best_time)
{
  if( tv->no_of_included_values > 0 )
  {
    if( tv->average_time < *min_average_time )
      *min_average_time = tv->average_time;
    if( tv->best_time < *min_best_time )
      *min_best_time = tv->best_time;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void EvalTimeSetStage3(EVAL_TIME *tv, float min_average_time,            */
/*    float min_best_time)                                                   */
/*                                                                           */
/*  Set cv correspondong to ss.  The fields defined by this call are:        */
/*                                                                           */
/*    average_time_is_min                                                    */
/*    relative_average_time                                                  */
/*    best_time_is_min                                                       */
/*    relative_best_time                                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static void EvalTimeSetStage3(EVAL_TIME *tv, float min_average_time,
  float min_best_time)
{
  tv->average_time_is_min =
    (tv->average_time <= min_average_time && tv->average_time != MAX_TIME);
  tv->relative_average_time = RelativeCost(tv->average_time, min_average_time);
  tv->best_time_is_min =
    (tv->best_time <= min_best_time && tv->best_time != MAX_TIME);
  tv->relative_best_time = RelativeCost(tv->best_time, min_best_time);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "EVAL_SOLN_GROUP"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  EVAL_SOLN_GROUP EvalSolnGroupMake(KHE_SOLN_GROUP soln_group, HA_ARENA a) */
/*                                                                           */
/*  Make a new eval soln group object.                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static EVAL_SOLN_GROUP EvalSolnGroupMake(KHE_SOLN_GROUP soln_group, HA_ARENA a)
{
  EVAL_SOLN_GROUP res;
  HaMake(res, a);
  res->soln_group = soln_group;
  ** res->average_cost is initially undefined **
  ** res->best_cost    is initially undefined **
  ** res->average_time is initially undefined **
  ** res->best_time    is initially undefined **
  EvalAverageCostInit(&res->average_average_cost);
  EvalAverageCostInit(&res->average_best_cost);
  EvalAverageFloatInit(&res->average_average_time);
  EvalAverageFloatInit(&res->average_best_time);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "EVAL_INSTANCE"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  EVAL_INSTANCE EvalInstanceMake(KHE_ARCHIVE archive, HA_ARENA a)          */
/*                                                                           */
/*  Make a new eval instance object for archive.  Its values field is for    */
/*  one particular instance, but that instance is not defined initially.     */
/*                                                                           */
/*****************************************************************************/

/* ***
static EVAL_INSTANCE EvalInstanceMake(KHE_ARCHIVE archive, HA_ARENA a)
{
  EVAL_INSTANCE res;  EVAL_SOLN_GROUP esg;  KHE_SOLN_GROUP soln_group;  int i;
  HaMake(res, a);
  res->ins = NULL;
  HaArrayInit(res->soln_groups, a);
  for( i = 0;  i < KheArchiveSolnGroupCount(archive);  i++ )
  {
    soln_group = KheArchiveSolnGroup(archive, i);
    esg = EvalSolnGroupMake(soln_group, a);
    HaArrayAddLast(res->soln_groups, esg);
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

/* ***
static void EvalSolnGroupSetStage1(EVAL_SOLN_GROUP esg, KHE_SOLN_SET ss)
{
  int i;  KHE_SOLN soln;  KHE_COST cost;  float running_time;
  EvalAverageCostInit(&esg->average_cost);
  EvalBestCostInit(&esg->best_cost);
  EvalAverageFloatInit(&esg->average_time);
  EvalBestFloatInit(&esg->best_time);
  for( i = 0;  i < KheSolnSetSolnCount(ss);  i++ )
  {
    soln = KheSolnSetSoln(ss, i);
    if( KheSolnType(soln) != KHE_SOLN_INVALID_PLACEHOLDER )
    {
      cost = KheSolnCost(soln);
      EvalAverageCostAddCost(&esg->average_cost, cost);
      EvalBestCostAddCost(&esg->best_cost, cost);
      if( KheSolnHasRunningTime(soln, &running_time) )
      {
	EvalAverageFloatAddFloat(&esg->average_time, running_time);
	EvalBestFloatAddFloat(&esg->best_time, running_time);
      }
    }
  }
  EvalAverageCostSetValue(&esg->average_cost);
  EvalBestCostSetValue(&esg->best_cost);
  EvalAverageFloatSetValue(&esg->average_time);
  EvalBestFloatSetValue(&esg->best_time);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

/* ***
static void EvalSolnGroupSetStage2(EVAL_SOLN_GROUP esg,
  KHE_COST *min_average_cost, KHE_COST *min_best_cost,
  float *min_average_time, float *min_best_time)
{
  EvalAverageCostAccumulateMin(&esg->average_cost, min_average_cost);
  EvalBestCostAccumulateMin(&esg->best_cost, min_best_cost);
  EvalAverageFloatAccumulateMin(&esg->average_time, min_average_time);
  EvalBestFloatAccumulateMin(&esg->best_time, min_best_time);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

/* ***
static void EvalSolnGroupSetStage2Average(EVAL_SOLN_GROUP esg,
  KHE_COST *min_average_cost, KHE_COST *min_best_cost,
  float *min_average_time, float *min_best_time)
{
  EvalAverageCostAccumulateMin(&esg->average_average_cost, min_average_cost);
  EvalAverageCostAccumulateMin(&esg->average_best_cost, min_best_cost);
  EvalAverageFloatAccumulateMin(&esg->average_average_time, min_average_time);
  EvalAverageFloatAccumulateMin(&esg->average_best_time, min_best_time);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

/* ***
static void EvalSolnGroupSetStage3(EVAL_SOLN_GROUP esg,
  KHE_COST min_average_cost, KHE_COST min_best_cost,
  float min_average_time, float min_best_time)
{
  EvalAverageCostSetRelative(&esg->average_cost, min_average_cost);
  EvalBestCostSetRelative(&esg->best_cost, min_best_cost);
  EvalAverageFloatSetRelative(&esg->average_time, min_average_time);
  EvalBestFloatSetRelative(&esg->best_time, min_best_time);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

/* ***
static void EvalSolnGroupSetStage3Average(EVAL_SOLN_GROUP esg,
  KHE_COST min_average_cost, KHE_COST min_best_cost,
  float min_average_time, float min_best_time)
{
  EvalAverageCostSetRelative(&esg->average_average_cost, min_average_cost);
  EvalAverageCostSetRelative(&esg->average_best_cost, min_best_cost);
  EvalAverageFloatSetRelative(&esg->average_average_time, min_average_time);
  EvalAverageFloatSetRelative(&esg->average_best_time, min_best_time);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

/* ***
static void EvalSolnGroupSetStage4(EVAL_SOLN_GROUP esg)
{
  EvalAverageCostReportToAverage(&esg->average_cost, &esg->average_average_cost);
  EvalBestCostReportToAverage(&esg->best_cost, &esg->average_best_cost);
  EvalAverageFloatReportToAverage(&esg->average_time, &esg->average_average_time);
  EvalBestFloatReportToAverage(&esg->best_time, &esg->average_best_time);

}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void EvalInstanceSet(EVAL_INSTANCE ei, KHE_INSTANCE ins,                 */
/*    int instance_index)                                                    */
/*                                                                           */
/*  Set ei to reflect the costs and times of instance ins, whose index in    */
/*  the enclosing archive is instance_index.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void EvalInstanceSet(EVAL_INSTANCE ei, KHE_INSTANCE ins,
  int instance_index)
{
  EVAL_SOLN_GROUP esg;  int i;  KHE_SOLN_SET ss;
  KHE_COST min_average_cost, min_best_cost;
  float min_average_time, min_best_time;

  ** Stage 1 - find summary values for individual solution sets **
  ei->ins = ins;
  HaArrayForEach(ei->soln_groups, esg, i)
  {
    ss = KheSolnGroupInstanceSolnSetByIndex(esg->soln_group, instance_index);
    EvalSolnGroupSetStage1(esg, ss);
    ** ***
    EvalCostSetStage1(&esg->cost, ss);
    EvalTimeSetStage1(&esg->time, ss);
    *** **
  }

  ** Stage 2 - find average and min cost and time **
  min_average_cost = min_best_cost = NO_COST;
  min_average_time = min_best_time = NO_FLOAT;
  HaArrayForEach(ei->soln_groups, esg, i)
    EvalSolnGroupSetStage2(esg, &min_average_cost, &min_best_cost,
      &min_average_time, &min_best_time);

  ** Stage 3 - set is_min and relative values for cost and time **
  HaArrayForEach(ei->soln_groups, esg, i)
    EvalSolnGroupSetStage3(esg, min_average_cost, min_best_cost,
      min_average_time, min_best_time);
  ** *** sti ll to do
  HaArrayForEach(ei->values, esg, i)
  {
    EvalCostSetStage3(&esg->cost, min_best_cost, min_average_cost);
    EvalTimeSetStage3(&esg->time, min_best_time, min_average_time);
  }
  *** **

  ** Stage 4 - report values to average row **
  HaArrayForEach(ei->soln_groups, esg, i)
    EvalSolnGroupSetStage4(esg);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void EvalInstanceSet(EVAL_INSTANCE ei, KHE_INSTANCE ins,                 */
/*    int instance_index)                                                    */
/*                                                                           */
/*  Set ei to reflect the costs and times of instance ins, whose index in    */
/*  the enclosing archive is instance_index.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void EvalAverageSet(EVAL_INSTANCE ei, KHE_INSTANCE ins,
  int instance_index)
{
  EVAL_SOLN_GROUP esg;  int i;
  KHE_COST min_average_cost, min_best_cost;
  float min_average_time, min_best_time;

  ** Stage 1 - find summary values for individual solution sets **
  ** already done for the average row **

  ** Stage 2 - find average and min cost and time **
  min_average_cost = min_best_cost = NO_COST;
  min_average_time = min_best_time = NO_FLOAT;
  HaArrayForEach(ei->soln_groups, esg, i)
    EvalSolnGroupSetStage2Average(esg, &min_average_cost, &min_best_cost,
      &min_average_time, &min_best_time);

  ** Stage 3 - set is_min and relative values for cost and time **
  HaArrayForEach(ei->soln_groups, esg, i)
    EvalSolnGroupSetStage3(esg, min_average_cost, min_best_cost,
      min_average_time, min_best_time);
  ** *** sti ll to do
  HaArrayForEach(ei->values, esg, i)
  {
    EvalCostSetStage3(&esg->cost, min_best_cost, min_average_cost);
    EvalTimeSetStage3(&esg->time, min_best_time, min_average_time);
  }
  *** **

  ** Stage 4 - report values to average row **
  HaArrayForEach(ei->soln_groups, esg, i)
    EvalSolnGroupSetStage4(esg);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "EVAL_TABLE"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void EvalTableAddRowsColumnsAndEntries(EVAL_TABLE et)                    */
/*                                                                           */
/*  Add rows, columns, and entries to et.                                    */
/*                                                                           */
/*****************************************************************************/

static void EvalTableAddRowsColumnsAndEntries(EVAL_TABLE et)
{
  KHE_SOLN_GROUP soln_group;  KHE_INSTANCE ins;  int i, j;
  KHE_SOLN_SET soln_set;  EVAL_ORDINARY_ENTRY eoe;  EVAL_ORDINARY_ROW eor;
  EVAL_COL ec;  EVAL_AVERAGE_ENTRY eae;

  /* one ordinary row for each instance */
  if( DEBUG2 )
    fprintf(stderr, "[ EvalTableAddRowsColumnsAndEntries(et)\n");
  for( i = 0;  i < KheArchiveInstanceCount(et->archive);  i++ )
  {
    ins = KheArchiveInstance(et->archive, i);
    eor = EvalOrdinaryRowMake(ins, et);
    HaArrayAddLast(et->ordinary_rows, eor);
  }
  if( DEBUG2 )
    fprintf(stderr, "  EvalTableAddRowsColumnsAndEntries at (a)\n");

  /* one column for each soln group */
  for( j = 0;  j < KheArchiveSolnGroupCount(et->archive);  j++ )
  {
    soln_group = KheArchiveSolnGroup(et->archive, j);
    ec = EvalColMake(soln_group, et);
    HaArrayAddLast(et->columns, ec);
  }
  if( DEBUG2 )
    fprintf(stderr, "  EvalTableAddRowsColumnsAndEntries at (b)\n");

  /* ordinary entries for the ordinary rows and columns */
  HaArrayForEach(et->ordinary_rows, eor, i)
    HaArrayForEach(et->columns, ec, j)
    {
      if( DEBUG2 )
	fprintf(stderr, "  EvalTableAddRowsColumnsAndEntries at (%d, %d) 1\n",
	  i, j);
      soln_set = KheSolnGroupInstanceSolnSetByIndex(ec->soln_group, i);
      if( DEBUG2 )
	fprintf(stderr, "  EvalTableAddRowsColumnsAndEntries at (%d, %d) 2\n",
	  i, j);
      eoe = EvalOrdinaryEntryMake(eor->ins, ec->soln_group, soln_set, et);
      if( DEBUG2 )
	fprintf(stderr, "  EvalTableAddRowsColumnsAndEntries at (%d, %d) 3\n",
	  i, j);
      HaArrayAddLast(eor->ordinary_entries, eoe);
      if( DEBUG2 )
	fprintf(stderr, "  EvalTableAddRowsColumnsAndEntries at (%d, %d) 4\n",
	  i, j);
      HaArrayAddLast(ec->ordinary_entries, eoe);
      if( DEBUG2 )
	fprintf(stderr, "  EvalTableAddRowsColumnsAndEntries at (%d, %d) 5\n",
	  i, j);
    }
  if( DEBUG2 )
    fprintf(stderr, "  EvalTableAddRowsColumnsAndEntries at (c)\n");

  /* one average row */
  et->average_row = EvalAverageRowMake(et);
  if( DEBUG2 )
    fprintf(stderr, "  EvalTableAddRowsColumnsAndEntries at (d)\n");

  /* average entries for the average row and the columns */
  HaArrayForEach(et->columns, ec, j)
  {
    eae = EvalAverageEntryMake(ec->soln_group, et);
    HaArrayAddLast(et->average_row->average_entries, eae);
    ec->average_entry = eae;
  }
  if( DEBUG2 )
    fprintf(stderr, "] EvalTableAddRowsColumnsAndEntries\n");
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalTableFindMinValues(EVAL_TABLE et)
{
  EVAL_ORDINARY_ROW eor;  int i, j;  EVAL_ORDINARY_ENTRY eoe;
  KHE_COST min_average_cost, min_best_cost;
  float min_average_time, min_best_time;
  HaArrayForEach(et->ordinary_rows, eor, i)
  {
    /* find min values across row eor */
    min_average_cost = min_best_cost = NO_COST;
    min_average_time = min_best_time = NO_FLOAT;
    HaArrayForEach(eor->ordinary_entries, eoe, j)
    {
      EvalAverageCostAccumulateMin(&eoe->average_cost, &min_average_cost);
      EvalBestCostAccumulateMin(&eoe->best_cost, &min_best_cost);
      EvalAverageFloatAccumulateMin(&eoe->average_time, &min_average_time);
      EvalBestFloatAccumulateMin(&eoe->best_time, &min_best_time);
    }

    /* distribute min values across row eor */
    HaArrayForEach(eor->ordinary_entries, eoe, j)
    {
      EvalAverageCostSetRelative(&eoe->average_cost, min_average_cost);
      EvalBestCostSetRelative(&eoe->best_cost, min_best_cost);
      EvalAverageFloatSetRelative(&eoe->average_time, min_average_time);
      EvalBestFloatSetRelative(&eoe->best_time, min_best_time);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalTableFindAverageRowValues(EVAL_TABLE et)
{
  EVAL_COL ec;  int i, j;  EVAL_AVERAGE_ENTRY eae;  EVAL_ORDINARY_ENTRY eoe;
  HaArrayForEach(et->columns, ec, i)
  {
    eae = ec->average_entry;
    HaArrayForEach(ec->ordinary_entries, eoe, j)
    {
      /* report costs and times to average */
      EvalAverageCostReportToAverage(&eoe->average_cost,
	&eae->average_average_cost);
      EvalBestCostReportToAverage(&eoe->best_cost,
	&eae->average_best_cost);
      EvalAverageFloatReportToAverage(&eoe->average_time,
	&eae->average_average_time);
      EvalBestFloatReportToAverage(&eoe->best_time,
	&eae->average_best_time);

      /* report relative costs and times to average */
      EvalAverageCostReportRelativeToAverage(&eoe->average_cost,
        &eae->average_relative_average_cost);
      EvalBestCostReportRelativeToAverage(&eoe->best_cost,
	&eae->average_relative_best_cost);
      EvalAverageFloatReportRelativeToAverage(&eoe->average_time,
	&eae->average_relative_average_time);
      EvalBestFloatReportRelativeToAverage(&eoe->best_time,
	&eae->average_relative_best_time);

      /* quit early if we've reached instance_id */
      if( et->show_averages_instance_id != NULL &&
	  strcmp(KheInstanceId(eoe->ins), et->show_averages_instance_id) == 0 )
	break;
    }

    /* set the values */
    EvalAverageCostSetValue(&eae->average_average_cost);
    EvalAverageCostSetValue(&eae->average_best_cost);
    EvalAverageFloatSetValue(&eae->average_average_time);
    EvalAverageFloatSetValue(&eae->average_best_time);

    EvalAverageFloatSetValue(&eae->average_relative_average_cost);
    EvalAverageFloatSetValue(&eae->average_relative_best_cost);
    EvalAverageFloatSetValue(&eae->average_relative_average_time);
    EvalAverageFloatSetValue(&eae->average_relative_best_time);
  }
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void EvalTableFindAverageRowMinValues(EVAL_TABLE et)
{
  EVAL_AVERAGE_ROW ear;  EVAL_AVERAGE_ENTRY eae;  int i;
  KHE_COST min_average_average_cost, min_average_best_cost;
  float min_average_average_time, min_average_best_time;
  float min_average_relative_average_cost, min_average_relative_best_cost;
  float min_average_relative_average_time, min_average_relative_best_time;
  ear = et->average_row;

  /* find min values across the row */
  min_average_average_cost = min_average_best_cost = NO_COST;
  min_average_average_time = min_average_best_time = NO_FLOAT;
  min_average_relative_average_cost = min_average_relative_best_cost = NO_FLOAT;
  min_average_relative_average_time = min_average_relative_best_time = NO_FLOAT;
  HaArrayForEach(ear->average_entries, eae, i)
  {
    EvalAverageCostAccumulateMin(&eae->average_average_cost,
      &min_average_average_cost);
    EvalAverageCostAccumulateMin(&eae->average_best_cost,
      &min_average_best_cost);
    EvalAverageFloatAccumulateMin(&eae->average_average_time,
      &min_average_average_time);
    EvalAverageFloatAccumulateMin(&eae->average_best_time,
      &min_average_best_time);

    EvalAverageFloatAccumulateMin(&eae->average_relative_average_cost,
      &min_average_relative_average_cost);
    EvalAverageFloatAccumulateMin(&eae->average_relative_best_cost,
      &min_average_relative_best_cost);
    EvalAverageFloatAccumulateMin(&eae->average_relative_average_time,
      &min_average_relative_average_time);
    EvalAverageFloatAccumulateMin(&eae->average_relative_best_time,
      &min_average_relative_best_time);
  }

  /* distribute min values across the row */
  HaArrayForEach(ear->average_entries, eae, i)
  {
    EvalAverageCostSetRelative(&eae->average_average_cost,
      min_average_average_cost);
    EvalAverageCostSetRelative(&eae->average_best_cost,
      min_average_best_cost);
    EvalAverageFloatSetRelative(&eae->average_average_time,
      min_average_average_time);
    EvalAverageFloatSetRelative(&eae->average_best_time,
      min_average_best_time);

    EvalAverageFloatSetRelative(&eae->average_relative_average_cost,
      min_average_relative_average_cost);
    EvalAverageFloatSetRelative(&eae->average_relative_best_cost,
      min_average_relative_best_cost);
    EvalAverageFloatSetRelative(&eae->average_relative_average_time,
      min_average_relative_average_time);
    EvalAverageFloatSetRelative(&eae->average_relative_best_time,
      min_average_relative_best_time);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  EVAL_TABLE EvalTableMake(KHE_ARCHIVE archive, bool show_averages_row,    */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Make a new eval table for archive, initially with no column sets.        */
/*                                                                           */
/*****************************************************************************/

EVAL_TABLE EvalTableMake(KHE_ARCHIVE archive,
  char *show_averages_instance_id, HA_ARENA a)
{
  EVAL_TABLE res;

  if( DEBUG1 )
    fprintf(stderr, "[ EvalTableMake(archive, %s, a)\n",
      show_averages_instance_id == NULL ? "NULL" : show_averages_instance_id);

  /* make the object and initialize the free lists */
  HaMake(res, a);
  HaArrayInit(res->ordinary_entry_free_list, a);
  HaArrayInit(res->average_entry_free_list, a);
  HaArrayInit(res->ordinary_row_free_list, a);
  HaArrayInit(res->average_row_free_list, a);
  HaArrayInit(res->col_fmt_free_list, a);
  HaArrayInit(res->col_free_list, a);

  /* initialize the non-tabular stuff */
  res->arena = a;
  res->archive = archive;
  res->show_averages_instance_id = show_averages_instance_id;
  HaArrayInit(res->eval_col_fmts, a);
  if( DEBUG1 )
    fprintf(stderr, "  EvalTableMake at (a)\n");

  /* initialize the actual table */
  HaArrayInit(res->ordinary_rows, a);
  HaArrayInit(res->columns, a);
  res->average_row = EvalAverageRowMake(res);

  /* add rows, columns, and entries, and initialize the entry values */
  EvalTableAddRowsColumnsAndEntries(res);
  if( DEBUG1 )
    fprintf(stderr, "  EvalTableMake at (b)\n");

  /* find min values across the columns */
  EvalTableFindMinValues(res);
  if( DEBUG1 )
    fprintf(stderr, "  EvalTableMake at (c)\n");

  /* find average values down the rows */
  EvalTableFindAverageRowValues(res);
  if( DEBUG1 )
    fprintf(stderr, "  EvalTableMake at (d)\n");

  /* find min values across the average row */
  EvalTableFindAverageRowMinValues(res);

  if( DEBUG1 )
    fprintf(stderr, "] EvalTableMake returning\n");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalTableAddColumnFormat(EVAL_TABLE et,                             */
/*    EVAL_COLUMN_DATA_TYPE data_type, bool soft_costs_only)                 */
/*                                                                           */
/*  Add a column format with these attributes to  et.                        */
/*                                                                           */
/*****************************************************************************/

void EvalTableAddColumnFormat(EVAL_TABLE et,
  EVAL_COLUMN_DATA_TYPE data_type, bool soft_costs_only)
{
  EVAL_COL_FMT ecf;
  ecf = EvalColFmtMake(data_type, soft_costs_only, et);
  HaArrayAddLast(et->eval_col_fmts, ecf);
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalTableClearColumnFormats(EVAL_TABLE et)                          */
/*                                                                           */
/*  Clear et's column formats.                                               */
/*                                                                           */
/*****************************************************************************/

void EvalTableClearColumnFormats(EVAL_TABLE et)
{
  int i;
  HaArrayAppend(et->col_fmt_free_list, et->eval_col_fmts, i);
  HaArrayClear(et->eval_col_fmts);
}


/*****************************************************************************/
/*                                                                           */
/*  char *RemovePercent(char *str)                                           */
/*                                                                           */
/*  Replace all '%' characters in str by spaces.                             */
/*                                                                           */
/*****************************************************************************/

static char *RemovePercent(char *str)
{
  char *p;
  for( p = str;  *p != '\0';  p++ )
    if( *p == '%' )
      *p = ' ';
  return str;
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalTableAddSecondHeaders(EVAL_TABLE et, TABLE_ROW row)             */
/*                                                                           */
/*  Add the second headers for et to row.                                    */
/*                                                                           */
/*****************************************************************************/

static void EvalTableAddSecondHeaders(EVAL_TABLE et, TABLE_ROW row)
{
  EVAL_COL_FMT ecf;  int i;
  HaArrayForEach(et->eval_col_fmts, ecf, i)
    EvalColFmtAddSecondHeader(ecf, row, et->arena);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

/* ***
static void EvalInstancePrintEvalColFmts(EVAL_INSTANCE ei,
  EVAL_TABLE et, TABLE_ROW row, HA_ARENA a)
{
  EVAL_SOLN_GROUP esg;  int i, j;  EVAL_COL_FMT ecf;
  HaArrayForEach(ei->soln_groups, esg, i)
    HaArrayForEach(et->eval_col_fmts, ecf, j)
      EvalColFmtPrintValues(ecf, esg, row, et->arena);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void EvalTableAddAveragesRow(EVAL_TABLE et, TABLE table)                 */
/*                                                                           */
/*  Add an averages row to table.                                            */
/*                                                                           */
/*****************************************************************************/

static void EvalTableAddAveragesRow(EVAL_TABLE et, TABLE table)
{
  TABLE_ROW row;  HA_ARENA a;  TABLE_ENTRY entry;
  int i, j;  EVAL_COL_FMT ecf;  EVAL_AVERAGE_ENTRY eae;

  /* make and add the averages row, with a header column */
  a = et->arena;
  row = TableRowMake(a);
  TableAddRow(table, row);
  entry = TableEntryMakeText(TABLE_INDENT, TABLE_BOLD, "Average", a);
  TableRowAddEntry(row, entry);

  /* add the averages entries */
  HaArrayForEach(et->average_row->average_entries, eae, i)
    HaArrayForEach(et->eval_col_fmts, ecf, j)
      EvalAverageEntryPrint(eae, ecf, row, a);
}


/*****************************************************************************/
/*                                                                           */
/*  void EvalTableAddBestsRow(EVAL_TABLE et, TABLE table)                    */
/*                                                                           */
/*  Add a bests row to table.                                                */
/*                                                                           */
/*****************************************************************************/

static void EvalTableAddBestsRow(EVAL_TABLE et, TABLE table)
{
  TABLE_ROW row;  HA_ARENA a;  TABLE_ENTRY entry;
  int i, j;  EVAL_COL_FMT ecf;  EVAL_AVERAGE_ENTRY eae;

  /* make and add the averages row, with a header column */
  a = et->arena;
  row = TableRowMake(a);
  TableAddRow(table, row);
  entry = TableEntryMakeText(TABLE_INDENT, TABLE_BOLD, "No. of bests", a);
  TableRowAddEntry(row, entry);

  /* add the averages entries */
  HaArrayForEach(et->average_row->average_entries, eae, i)
    HaArrayForEach(et->eval_col_fmts, ecf, j)
      EvalAverageEntryPrintNumberOfBests(eae, ecf, row, a);
}


/*****************************************************************************/
/*                                                                           */
/*  TABLE EvalTableMakeTable(EVAL_TABLE et)                                  */
/*                                                                           */
/*  Make a table out of et, showing an evaluation of its archive using       */
/*  its column sets.                                                         */
/*                                                                           */
/*****************************************************************************/

TABLE EvalTableMakeTable(EVAL_TABLE et, bool show_averages_row,
  bool show_bests_row)
{
  TABLE res;  EVAL_COL_FMT ecf;  int span, i, j, k;  HA_ARENA a;
  TABLE_ENTRY entry;  TABLE_ROW row;  KHE_ARCHIVE archive;
  EVAL_ORDINARY_ROW eor;  EVAL_ORDINARY_ENTRY eoe;  EVAL_COL ec;

  /* table and header */
  if( DEBUG1 )
    fprintf(stderr, "[ EvalTableMakeTable(et, %s, %s)\n",
      bool_show(show_averages_row), bool_show(show_bests_row));
  a = et->arena;
  archive = et->archive;
  HnAssert(HaArrayCount(et->columns) > 0,
    "EvalTableMakeTable internal error (no columns)");
  ecf = HaArrayFirst(et->eval_col_fmts);
  res = TableMake(DataTypeTableHeader(ecf->data_type), a);

  /* first header row */
  row = TableRowMake(a);
  TableAddRow(res, row);
  entry = TableEntryMakeInt(TABLE_LEFT, TABLE_BOLD, "Instances (%d)",
    KheArchiveInstanceCount(archive), a);
  TableRowAddEntry(row, entry);
  span = HaArrayCount(et->eval_col_fmts);
  HaArrayForEach(et->columns, ec, i)
  {
    entry = TableEntryMakeText(TABLE_CENTRE, TABLE_BOLD,
      RemovePercent(KheSolnGroupId(ec->soln_group)), a);
    if( span > 1 )
    {
      TableEntrySetHSpan(entry, span);
      TableEntrySetUnderline(entry, true);
    }
    TableRowAddEntry(row, entry);
  }

  /* optional second header row */
  if( span > 1 )
  {
    row = TableRowMake(a);
    TableAddRow(res, row);
    entry = TableEntryMakeText(TABLE_LEFT, TABLE_ROMAN, "", a);
    TableRowAddEntry(row, entry);
    HaArrayForEach(et->columns, ec, i)
      EvalTableAddSecondHeaders(et, row);
  }

  /* one row for each instance */
  HaArrayForEach(et->ordinary_rows, eor, i)
  {
    /* row with header column showing instance name */
    row = TableRowMake(a);
    TableAddRow(res, row);
    entry = TableEntryMakeText(TABLE_LEFT, TABLE_ROMAN,
      RemovePercent(KheInstanceId(eor->ins)), a);
    TableRowAddEntry(row, entry);

    /* one column for each soln group */
    HaArrayForEach(eor->ordinary_entries, eoe, j)
      HaArrayForEach(et->eval_col_fmts, ecf, k)
        EvalOrdinaryEntryPrint(eoe, ecf, row, a);

    /* average row after this instance, if requested */
    if( KheArchiveInstanceCount(archive) >= 2 &&
	et->show_averages_instance_id != NULL &&
	strcmp(et->show_averages_instance_id, KheInstanceId(eor->ins)) == 0 )
    {
      if( show_averages_row )
	EvalTableAddAveragesRow(et, res);
      if( show_bests_row )
	EvalTableAddBestsRow(et, res);
    }
  }

  /* optional average row */
  if( KheArchiveInstanceCount(archive) >= 2 &&
      et->show_averages_instance_id == NULL )
  {
    if( show_averages_row )
      EvalTableAddAveragesRow(et, res);
    if( show_bests_row )
      EvalTableAddBestsRow(et, res);
  }
  if( DEBUG1 )
    fprintf(stderr, "] EvalTableMakeTable returning\n");
  return res;
}


/* *** old version
TABLE EvalTableMakeTableOld(EVAL_TABLE et)
{
  TABLE res; EVAL_COL_FMT ecf;  int span, i;  HA_ARENA a;
  KHE_SOLN_GROUP soln_group;  TABLE_ENTRY entry;  TABLE_ROW row;
  KHE_ARCHIVE archive;  KHE_INSTANCE ins;

  ** table and header **
  a = et->arena;
  archive = et->archive;
  HnAssert(HaArrayCount(et->columns) > 0,
    "EvalTableMakeTable internal error (no columns)");
  ecf = HaArrayFirst(et->columns);
  res = TableMake(DataTypeTableHeader(ecf->data_type), a);

  ** first header row **
  row = TableRowMake(a);
  TableAddRow(res, row);
  entry = TableEntryMakeInt(TABLE_LEFT, TABLE_BOLD, "Instances (%d)",
    KheArchiveInstanceCount(archive), a);
  TableRowAddEntry(row, entry);
  span = EvalTableSpan(et);
  for( i = 0;  i < KheArchiveSolnGroupCount(archive);  i++ )
  {
    soln_group = KheArchiveSolnGroup(archive, i);
    entry = TableEntryMakeText(TABLE_CENTRE, TABLE_BOLD,
      RemovePercent(KheSolnGroupId(soln_group)), a);
    if( span > 1 )
    {
      TableEntrySetHSpan(entry, span);
      TableEntrySetUnderline(entry, true);
    }
    TableRowAddEntry(row, entry);
  }

  ** optional second header row **
  if( span > 1 )
  {
    row = TableRowMake(a);
    TableAddRow(res, row);
    entry = TableEntryMakeText(TABLE_LEFT, TABLE_ROMAN, "", a);
    TableRowAddEntry(row, entry);
    for( i = 0;  i < KheArchiveSolnGroupCount(archive);  i++ )
      EvalTableAddSecondHeaders(et, row);
  }

  ** one row for each instance **
  for( i = 0;  i < KheArchiveInstanceCount(archive);  i++ )
  {
    ** row with header column showing instance name **
    ins = KheArchiveInstance(archive, i);
    row = TableRowMake(a);
    TableAddRow(res, row);
    entry = TableEntryMakeText(TABLE_LEFT, TABLE_ROMAN,
      RemovePercent(KheInstanceId(ins)), a);
    TableRowAddEntry(row, entry);

    ** set the cost and time values for this row **
    EvalInstanceSet(et->curr_eval_instance, ins, i);

    ** print each soln group **
    EvalInstancePrintEvalColumns(et->curr_eval_instance, et, row, et->arena);

    ** average row after instance, if requested **
    if( et->show_averages_row && et->instance_id != NULL &&
	strcmp(et->instance_id, KheInstanceId(ins)) == 0 )
      EvalTableAddAveragesRow(et, res);
  }

  ** optional average row **
  if( et->show_averages_row && et->instance_id == NULL &&
      KheArchiveSolnGroupCount(archive) > 0 &&
      KheArchiveInstanceCount(archive) > 0 )
    EvalTableAddAveragesRow(et, res);
  return res;
}
*** */

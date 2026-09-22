
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
/*  FILE:         summary.c                                                  */
/*  MODULE:       Building and printing tabular summaries.                   */
/*                                                                           */
/*****************************************************************************/
#include "externs.h"

#define DEBUG1 0
#define DEBUG2 0


/*****************************************************************************/
/*                                                                           */
/*  Submodule "averages"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  AVERAGE                                                                  */
/*                                                                           */
/*  An object used for calculating and printing averages.                    */
/*                                                                           */
/*****************************************************************************/

typedef struct average_rec {
  double		total;
  int			count;
} *AVERAGE;

typedef HA_ARRAY(AVERAGE) ARRAY_AVERAGE;


/*****************************************************************************/
/*                                                                           */
/*  AVERAGE AverageMake(HA_ARENA a)                                          */
/*                                                                           */
/*  Make a new average object.                                               */
/*                                                                           */
/*****************************************************************************/

static AVERAGE AverageMake(HA_ARENA a)
{
  AVERAGE res;
  HaMake(res, a);
  res->total = 0.0;
  res->count = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void AverageAdd(AVERAGE av, double value)                                */
/*                                                                           */
/*  Add value to av.                                                         */
/*                                                                           */
/*****************************************************************************/

static void AverageAdd(AVERAGE av, double value)
{
  av->total += value;
  av->count++;
}


/*****************************************************************************/
/*                                                                           */
/*  double AverageValue(AVERAGE av)                                          */
/*                                                                           */
/*  Return the average of av, or 0.0 if count is 0.                          */
/*                                                                           */
/*****************************************************************************/

static double AverageValue(AVERAGE av)
{
  return av->count == 0 ? 0.0 : av->total / (double) av->count;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "average info"                                                 */
/*                                                                           */
/*****************************************************************************/

typedef struct average_info_rec {
  ARRAY_AVERAGE hard_cost_averages;
  ARRAY_AVERAGE soft_cost_averages;
  ARRAY_AVERAGE rel_cost_averages;
  ARRAY_AVERAGE time_averages;
  ARRAY_AVERAGE best_counts;
} *AVERAGE_INFO;


/*****************************************************************************/
/*                                                                           */
/*  AVERAGE_INFO AverageInfoMake(int len, HA_ARENA a)                        */
/*                                                                           */
/*  Make a new average info object of the given length, with no data in it.  */
/*                                                                           */
/*****************************************************************************/

static AVERAGE_INFO AverageInfoMake(int len, HA_ARENA a)
{
  AVERAGE_INFO res;  int i;
  HaMake(res, a);
  HaArrayInit(res->hard_cost_averages, a);
  HaArrayInit(res->soft_cost_averages, a);
  HaArrayInit(res->rel_cost_averages, a);
  HaArrayInit(res->time_averages, a);
  HaArrayInit(res->best_counts, a);
  for( i = 0;  i < len;  i++ )
  {
    HaArrayAddLast(res->hard_cost_averages, AverageMake(a));
    HaArrayAddLast(res->soft_cost_averages, AverageMake(a));
    HaArrayAddLast(res->rel_cost_averages, AverageMake(a));
    HaArrayAddLast(res->time_averages, AverageMake(a));
    HaArrayAddLast(res->best_counts, AverageMake(a));
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main function"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnGroupHasRunningTimes(KHE_SOLN_GROUP sg)                      */
/*                                                                           */
/*  Return true if at least one of the solutions of sg has a running time.   */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static bool KheSolnGroupHasRunningTimes(KHE_SOLN_GROUP sg)
{
  KHE_SOLN soln;  int i;  float junk;
  for( i = 0;  i < KheSolnGroupSolnCount(sg);  i++ )
  {
    soln = KheSolnGroupSoln(sg, i);
    if( KheSolnHasRunningTime(soln, &junk) )
      return true;
  }
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void TableAddAverageRow(TABLE table, KHE_ARCHIVE archive,
  bool show_costs, bool soft_costs_only, bool show_relative, bool show_times,
  AVERAGE_INFO ai, int wanted, bool underline, HA_ARENA a)
{
  int hard_cost, soft_cost, j;  TABLE_ENTRY entry;  TABLE_ROW row;
  bool best;  AVERAGE hard_av, soft_av, rel_av, time_av;
  double hard_av_val, soft_av_val, rel_av_val, time_av_val;
  double best_hard_av_val, best_soft_av_val /* , best_time_av_val */;
  /* KHE_SOLN_GROUP soln_group; */

  /* make row and "Average" entry */
  row = TableRowMake(a);
  TableRowSetOverline(row, true);
  if( underline )
    TableRowSetUnderline(row, true);
  TableAddRow(table, row);
  entry = TableEntryMakeText(TABLE_LEFT, TABLE_BOLD, "Average", a);
  TableRowAddEntry(row, entry);

  /* find the best average cost and best average time */
  /* always ignoring incomplete columns */
  best_hard_av_val = best_soft_av_val = /* best_time_av_val = */ -1.0;
  for( j = 0;  j < KheArchiveSolnGroupCount(archive);  j++ )
  {
    hard_av = HaArray(ai->hard_cost_averages, j);
    soft_av = HaArray(ai->soft_cost_averages, j);
    rel_av = HaArray(ai->rel_cost_averages, j);
    time_av = HaArray(ai->time_averages, j);
    hard_av_val = AverageValue(hard_av);
    soft_av_val = AverageValue(soft_av);
    rel_av_val = AverageValue(rel_av);
    time_av_val = AverageValue(time_av);
    if( hard_av->count == wanted )
    {
      if( best_hard_av_val == -1.0 ||
	  hard_av_val < best_hard_av_val ||
	  (hard_av_val==best_hard_av_val && soft_av_val < best_soft_av_val) )
	best_hard_av_val = hard_av_val, best_soft_av_val = soft_av_val;
    }
    /* ***
    if( time_av->count == wanted )
    {
      if( best_time_av_val == -1.0 || time_av_val < best_time_av_val )
	best_time_av_val = time_av_val;
    }
    *** */
  }

  /* one column for each main table column */
  for( j = 0;  j < KheArchiveSolnGroupCount(archive);  j++ )
  {
    /* soln_group = KheArchiveSolnGroup(archive, j); */
    hard_av = HaArray(ai->hard_cost_averages, j);
    soft_av = HaArray(ai->soft_cost_averages, j);
    rel_av = HaArray(ai->rel_cost_averages, j);
    time_av = HaArray(ai->time_averages, j);
    hard_av_val = AverageValue(hard_av);
    soft_av_val = AverageValue(soft_av);
    rel_av_val = AverageValue(rel_av);
    time_av_val = AverageValue(time_av);
    if( show_costs )
    {
      best = (hard_av_val == best_hard_av_val &&
	soft_av_val == best_soft_av_val);
      if( soft_costs_only )
      {
	if( hard_av->count == wanted && hard_av->total == 0.0 )
	  entry = TableEntryMakeDouble(TABLE_RIGHT,
	    best ? TABLE_BOLD : TABLE_ROMAN, "%.0f", soft_av_val, a);
	else
	  entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, "", a);
      }
      else
      {
	if( hard_av->count == wanted )
	{
	  hard_cost = (int) hard_av_val;
	  soft_cost = (int) soft_av_val;
	  entry = TableEntryMakeDouble(TABLE_RIGHT,
	    best ? TABLE_BOLD : TABLE_ROMAN,
	    "%.5f", KheCostShow(KheCost(hard_cost, soft_cost)), a);
	}
	else
	  entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, "", a);
      }
      TableRowAddEntry(row, entry);
    }
    if( show_relative )
    {
      if( rel_av->count == wanted )
      {
	entry = TableEntryMakeDouble(TABLE_RIGHT, TABLE_ROMAN, "%.2f",
	  rel_av_val, a);
      }
      else
	entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, "", a);
      TableRowAddEntry(row, entry);
    }
    if( show_times /* && KheSolnGroupHasRunningTimes(soln_group) */ )
    {
      if( time_av->count == wanted )
      {
	entry = TableEntryMakeDouble(TABLE_RIGHT, TABLE_ROMAN, "%.1f",
	  time_av_val, a);
	/* ***
	best = (time_av_val == best_time_av_val);
	entry = TableEntryMakeDouble(TABLE_RIGHT,
	  best ? TABLE_BOLD : TABLE_ROMAN, "%.1f", time_av_val, a);
	*** */
      }
      else
	entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, "", a);
      TableRowAddEntry(row, entry);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  TABLE EvalTableBuild(KHE_ARCHIVE archive, bool show_costs,               */
/*    bool soft_costs_only, bool show_relative, bool show_times,             */
/*    bool show_averages, char *instance_id, bool show_best_count,           */
/*    bool *non_empty, HA_ARENA a)                                           */
/*                                                                           */
/*  Build an evaluation table from archive.                                  */
/*                                                                           */
/*    show_costs                                                             */
/*      Show solution costs.                                                 */
/*                                                                           */
/*    soft_costs_only                                                        */
/*      Only effective when show_costs is true.  Show an integral soft       */
/*      cost only, except show "inf." when hard cost > 0.                    */
/*                                                                           */
/*    average_costs                                                          */
/*      Only effective when show_costs is true.  Show the average of the     */
/*      costs of the solutions of each instance, not the best costs.         */
/*                                                                           */
/*    show_relative                                                          */
/*      Show relative solution costs.                                        */
/*                                                                           */
/*    show_times                                                             */
/*      Show running times, unless there are none.                           */
/*                                                                           */
/*    show_averages                                                          */
/*      Show an averages row at the bottom of the table.                     */
/*                                                                           */
/*    instance_id                                                            */
/*      If non-NULL, add an average row after this instance rather than      */
/*      at the bottom.  Include only the rows above in the average.          */
/*                                                                           */
/*    show_best_count                                                        */
/*      Show a number of best solutions row at the bottom of the table.      */
/*                                                                           */
/*  Both of show_costs and show_times may be true, and then the table will   */
/*  show both in a suitable format.                                          */
/*                                                                           */
/*  On return, *non_empty is set to true when the table contains at least    */
/*  one number (counting "invalid" and "inf." as numbers).                   */
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

TABLE EvalTableBuild(KHE_ARCHIVE archive, bool show_costs,
  bool soft_costs_only, bool average_costs, bool show_relative,
  bool show_times, bool show_averages, char *instance_id,
  bool show_best_count, bool *non_empty, HA_ARENA a)
{
  TABLE res;  int i, j, best_best;   KHE_SOLN_GROUP soln_group;  KHE_SOLN soln;
  KHE_INSTANCE ins;  TABLE_ENTRY entry;  TABLE_ROW row;  TABLE_FONT font;
  KHE_COST best_cost, cost;  char *str;  KHE_SOLN_SET ss;  float rt, rel_cost;
  /* ***
  ARRAY_AVERAGE hard_cost_averages, soft_cost_averages, time_averages, best_counts;
  *** */
  AVERAGE hard_av, soft_av, rel_av, time_av, best_av;
  /* double hard_av_val, soft_av_val, time_av_val; */
  /* double best_hard_av_val, best_soft_av_val, best_time_av_val; */
  int hard_cost, soft_cost, best_hard_cost, best_soft_cost, span, max_span;
  /* bool best; */  AVERAGE_INFO ai;

  HnAssert(show_costs || show_times, "EvalTableBuild: neither costs nor times");

  /* sort the solution sets, so that the solution to report is at the front */
  for( i = 0;  i < KheArchiveInstanceCount(archive);  i++ )
    for( j = 0;  j < KheArchiveSolnGroupCount(archive);  j++ )
    {
      soln_group = KheArchiveSolnGroup(archive, j);
      ss = KheSolnGroupInstanceSolnSetByIndex(soln_group, i);
      KheSolnSetSort(ss, &KheIncreasingCostCmp);
    }

  /* table and header */
  res = TableMake(show_costs ? "Solution badness" : "Running time", a);
  *non_empty = false;

  /* initialize averages, one for each column except the first */
  ai = AverageInfoMake(KheArchiveSolnGroupCount(archive), a);
  /* ***
  HaArrayInit(hard_cost_averages, a);
  HaArrayInit(soft_cost_averages, a);
  HaArrayInit(time_averages, a);
  HaArrayInit(best_counts, a);
  for( i = 0;  i < KheArchiveSolnGroupCount(archive);  i++ )
  {
    HaArrayAddLast(hard_cost_averages, AverageMake(a));
    HaArrayAddLast(soft_cost_averages, AverageMake(a));
    HaArrayAddLast(time_averages, AverageMake(a));
    HaArrayAddLast(best_counts, AverageMake(a));
  }
  *** */

  /* first row */
  max_span = 1;
  row = TableRowMake(a);
  TableAddRow(res, row);
  entry = TableEntryMakeInt(TABLE_LEFT, TABLE_BOLD, "Instances (%d)",
    KheArchiveInstanceCount(archive), a);
  TableRowAddEntry(row, entry);
  for( i = 0;  i < KheArchiveSolnGroupCount(archive);  i++ )
  {
    soln_group = KheArchiveSolnGroup(archive, i);
    span = (show_costs ? 1 : 0) + (show_relative ? 1 : 0) +
      (show_times /* && KheSolnGroupHasRunningTimes(soln_group */ ? 1 : 0);
    str = RemovePercent(KheSolnGroupId(soln_group));
    entry = TableEntryMakeText(TABLE_CENTRE, TABLE_BOLD, str, a);
    if( span > 1 )
    {
      TableEntrySetHSpan(entry, span);
      TableEntrySetUnderline(entry, true);
    }
    if( span > max_span )
      max_span = span;
    TableRowAddEntry(row, entry);
  }

  /* optional second row */
  if( max_span > 1 )
  {
    row = TableRowMake(a);
    TableAddRow(res, row);
    entry = TableEntryMakeText(TABLE_LEFT, TABLE_ROMAN, "", a);
    TableRowAddEntry(row, entry);
    for( i = 0;  i < KheArchiveSolnGroupCount(archive);  i++ )
    {
      soln_group = KheArchiveSolnGroup(archive, i);
      if( show_costs )
      {
	entry = TableEntryMakeText(TABLE_RIGHT, TABLE_BOLD, "Cost", a);
	TableRowAddEntry(row, entry);
      }
      if( show_relative )
      {
	entry = TableEntryMakeText(TABLE_RIGHT, TABLE_BOLD, "Rel.", a);
	TableRowAddEntry(row, entry);
      }
      if( show_times /* && KheSolnGroupHasRunningTimes(soln_group) */ )
      {
	entry = TableEntryMakeText(TABLE_RIGHT, TABLE_BOLD, "Time", a);
	TableRowAddEntry(row, entry);
      }
    }
  }

  /* one row for each instance */
  for( i = 0;  i < KheArchiveInstanceCount(archive);  i++ )
  {
    /* row with header column showing instance name */
    ins = KheArchiveInstance(archive, i);
    row = TableRowMake(a);
    TableAddRow(res, row);
    str = RemovePercent(KheInstanceId(ins));
    entry = TableEntryMakeText(TABLE_LEFT, TABLE_ROMAN, str, a);
    TableRowAddEntry(row, entry);

    /* find the best cost across this row */
    best_cost = INT64_MAX;
    for( j = 0;  j < KheArchiveSolnGroupCount(archive);  j++ )
    {
      soln_group = KheArchiveSolnGroup(archive, j);
      ss = KheSolnGroupInstanceSolnSetByIndex(soln_group, i);
      if( KheSolnSetSolnCount(ss) > 0 )
      {
	soln = KheSolnSetSoln(ss, 0);
	if( KheSolnType(soln) != KHE_SOLN_INVALID_PLACEHOLDER )
	{
	  cost = KheSolnCost(soln);
	  if( cost < best_cost )
	    best_cost = cost;
	}
      }
    }
    best_hard_cost = KheHardCost(best_cost);
    best_soft_cost = KheSoftCost(best_cost);

    /* span entries of instance row for each solution group */
    for( j = 0;  j < KheArchiveSolnGroupCount(archive);  j++ )
    {
      soln_group = KheArchiveSolnGroup(archive, j);
      ss = KheSolnGroupInstanceSolnSetByIndex(soln_group, i);
      if( KheSolnSetSolnCount(ss) == 0 )
      {
	entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, "", a);
	if( show_costs )
	  TableRowAddEntry(row, entry);
	if( show_relative )
	  TableRowAddEntry(row, entry);
	if( show_times /* && KheSolnGroupHasRunningTimes(soln_group) */ )
	  TableRowAddEntry(row, entry);
      }
      else
      {
	soln = KheSolnSetSoln(ss, 0);
	if( KheSolnType(soln) == KHE_SOLN_INVALID_PLACEHOLDER )
	{
	  entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, "invalid", a);
	  if( show_costs )
	    TableRowAddEntry(row, entry);
	  if( show_relative )
	    TableRowAddEntry(row, entry);
	  if( show_times /* && KheSolnGroupHasRunningTimes(soln_group) */ )
	    TableRowAddEntry(row, entry);
	  *non_empty = true;
	}
	else
	{
	  /* find hard cost and add to average */
	  hard_cost = KheHardCost(KheSolnCost(soln));
	  hard_av = HaArray(ai->hard_cost_averages, j);
	  AverageAdd(hard_av, hard_cost);

	  /* find soft cost and add to average */
	  soft_cost = KheSoftCost(KheSolnCost(soln));
	  soft_av = HaArray(ai->soft_cost_averages, j);
	  AverageAdd(soft_av, soft_cost);

	  /* find relative cost and add to average, but only if not hard */
	  if( hard_cost == 0 )
	  {
	    HnAssert(best_hard_cost == 0, "EvalTableBuild internal error");
	    if( best_soft_cost > 0 )
	    {
	      rel_cost = (float) soft_cost / (float) best_soft_cost;
	      rel_av = HaArray(ai->rel_cost_averages, j);
	      AverageAdd(rel_av, rel_cost);
	    }
	    else if( soft_cost == 0 )
	    {
	      /* we're pretending that 0 / 0 = 1.0 */
	      rel_av = HaArray(ai->rel_cost_averages, j);
	      AverageAdd(rel_av, 1.0);
	    }
	  }

	  /* find optional running time and add to average if present */
	  if( KheSolnHasRunningTime(soln, &rt) )
	  {
	    time_av = HaArray(ai->time_averages, j);
	    AverageAdd(time_av, rt);
	  }

	  /* cost entry, if requested */
	  if( show_costs )
	  {
	    font = (KheSolnCost(soln) <= best_cost ? TABLE_BOLD : TABLE_ROMAN);
	    if( KheSolnCost(soln) <= best_cost )
	    {
	      best_av = HaArray(ai->best_counts, j);
	      AverageAdd(best_av, 0.0);
	    }
	    if( !soft_costs_only )
	      entry = TableEntryMakeDouble(TABLE_RIGHT, font,
		"%.5f", KheCostShow(KheSolnCost(soln)), a);
	    else if( hard_cost > 0 )
	      entry = TableEntryMakeText(TABLE_RIGHT, font, "inf.", a);
	    else
	      entry = TableEntryMakeInt(TABLE_RIGHT, font, "%d", soft_cost, a);
	    TableRowAddEntry(row, entry);
	    *non_empty = true;
	  }

	  /* relative cost entry, if requested */
	  if( show_relative )
	  {
	    if( best_hard_cost > 0 )
              entry = TableEntryMakeDouble(TABLE_RIGHT, TABLE_ROMAN, "%.2f",
		(float) hard_cost / (float) best_hard_cost, a);
	    else if( hard_cost > 0 )
	      entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, "inf.", a);
	    else if( best_soft_cost > 0 )
	      entry = TableEntryMakeDouble(TABLE_RIGHT, TABLE_ROMAN, "%.2f",
		(float) soft_cost / (float) best_soft_cost, a);
	    else if( soft_cost > 0 )
	      entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, "inf.", a);
	    else
	      entry = TableEntryMakeDouble(TABLE_RIGHT, TABLE_ROMAN, "%.2f",
		1.0, a);
	    TableRowAddEntry(row, entry);
	    *non_empty = true;
	  }

	  /* running time entry, if requested */
	  if( show_times /* && KheSolnGroupHasRunningTimes(soln_group) */ )
	  {
	    if( KheSolnHasRunningTime(soln, &rt) )
	    {
	      entry = TableEntryMakeDouble(TABLE_RIGHT, TABLE_ROMAN, "%.1f",
		rt, a);
	      *non_empty = true;
	    }
	    else
	      entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, "-", a);
	    TableRowAddEntry(row, entry);
	  }
	}
      }
    }

    /* average row after instance, if requested */
    if( instance_id != NULL && strcmp(instance_id, KheInstanceId(ins)) == 0 )
      TableAddAverageRow(res, archive, show_costs, soft_costs_only,
	show_relative, show_times, ai, i + 1, true, a);
  }

  /* turn off hline at foot when there is an average or best row */
  if( (show_averages && instance_id == NULL) || show_best_count )
    TableSetHLineAtFoot(res, false);

  /* average row, if requested */
  if( show_averages && instance_id == NULL )
    TableAddAverageRow(res, archive, show_costs, soft_costs_only,
      show_relative, show_times, ai, KheArchiveInstanceCount(archive), false,a);

  /* ***
  {
    ** make row and "Average" entry **
    row = TableRowMake(a);
    TableRowSetOverline(row, true);
    TableAddRow(res, row);
    entry = TableEntryMakeText(TABLE_LEFT, TABLE_BOLD, "Average", a);
    TableRowAddEntry(row, entry);

    ** find the best average cost and best average time **
    ** always ignoring incomplete columns **
    best_hard_av_val = best_soft_av_val = best_time_av_val = -1.0;
    for( j = 0;  j < KheArchiveSolnGroupCount(archive);  j++ )
    {
      hard_av = HaArray(ai->hard_cost_averages, j);
      soft_av = HaArray(ai->soft_cost_averages, j);
      time_av = HaArray(ai->time_averages, j);
      hard_av_val = AverageValue(hard_av);
      soft_av_val = AverageValue(soft_av);
      time_av_val = AverageValue(time_av);
      if( hard_av->count == KheArchiveInstanceCount(archive) )
      {
	if( best_hard_av_val == -1.0 ||
	    hard_av_val < best_hard_av_val ||
	    (hard_av_val==best_hard_av_val && soft_av_val < best_soft_av_val) )
	  best_hard_av_val = hard_av_val, best_soft_av_val = soft_av_val;
      }
      if( time_av->count == KheArchiveInstanceCount(archive) )
      {
	if( best_time_av_val == -1.0 || time_av_val < best_time_av_val )
	  best_time_av_val = time_av_val;
      }
    }

    ** one column for each main table column **
    for( j = 0;  j < KheArchiveSolnGroupCount(archive);  j++ )
    {
      soln_group = KheArchiveSolnGroup(archive, j);
      hard_av = HaArray(ai->hard_cost_averages, j);
      soft_av = HaArray(ai->soft_cost_averages, j);
      time_av = HaArray(ai->time_averages, j);
      hard_av_val = AverageValue(hard_av);
      soft_av_val = AverageValue(soft_av);
      time_av_val = AverageValue(time_av);
      if( show_costs )
      {
	best = (hard_av_val == best_hard_av_val &&
	  soft_av_val == best_soft_av_val);
	if( soft_costs_only )
	{
	  if( hard_av->count == KheArchiveInstanceCount(archive) &&
	      hard_av->total == 0.0 )
	    entry = TableEntryMakeDouble(TABLE_RIGHT,
	      best ? TABLE_BOLD : TABLE_ROMAN, "%.0f", soft_av_val, a);
	  else
	    entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, "", a);
	}
	else
	{
	  if( hard_av->count == KheArchiveInstanceCount(archive) )
	  {
	    hard_cost = (int) hard_av_val;
	    soft_cost = (int) soft_av_val;
	    entry = TableEntryMakeDouble(TABLE_RIGHT,
              best ? TABLE_BOLD : TABLE_ROMAN,
	      "%.5f", KheCostShow(KheCost(hard_cost, soft_cost)), a);
	  }
	  else
	    entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, "", a);
	}
	TableRowAddEntry(row, entry);
      }
      if( show_times )
      {
	if( time_av->count == KheArchiveInstanceCount(archive) )
	{
	  best = (time_av_val == best_time_av_val);
	  entry = TableEntryMakeDouble(TABLE_RIGHT,
	    best ? TABLE_BOLD : TABLE_ROMAN, "%.1f", time_av_val, a);
	}
	else
	  entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, "", a);
	TableRowAddEntry(row, entry);
      }
    }
  }
  *** */

  /* best count row, if requested */
  if( show_best_count )
  {
    /* make row and "No. of best costs" entry */
    row = TableRowMake(a);
    /* if( !show_averages ) */
      TableRowSetOverline(row, true);
    TableAddRow(res, row);
    entry = TableEntryMakeText(TABLE_LEFT, TABLE_BOLD,
      "No. of best costs", a);
    TableRowAddEntry(row, entry);

    /* find the best count */
    best_best = 0;
    for( j = 0;  j < KheArchiveSolnGroupCount(archive);  j++ )
    {
      best_av = HaArray(ai->best_counts, j);
      if( best_av->count > best_best )
	best_best = best_av->count;
    }

    /* one column for each main table column */
    for( j = 0;  j < KheArchiveSolnGroupCount(archive);  j++ )
    {
      best_av = HaArray(ai->best_counts, j);
      if( show_costs )
      {
        entry = TableEntryMakeInt(TABLE_RIGHT,
	  best_av->count >= best_best ? TABLE_BOLD : TABLE_ROMAN, "%d",
	  best_av->count, a);
	TableRowAddEntry(row, entry);
      }
      if( show_times )
      {
	entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, "", a);
	TableRowAddEntry(row, entry);
      }
    }
  }

  /* make the best solutions bold */
  /* *** obsolete - TableMakeBestEntriesBold is not a great plan
  if( show_costs )
    TableMakeBestEntriesBold(res);
  *** */

#define cl(str) TableAddCaptionLine(res, str);

  /* caption */
  if( show_costs && show_times )
  {
    cl("For each solution group, the cost and running time in seconds of");
    cl("the best solution of each instance.  Unlike costs, running");
    cl("times are not verified by HSEval, merely reported based on");
    cl("information in the archive file.");
  }
  else if( show_costs )
  {
    cl("For each solution group, the cost of the best solution of each");
    cl("instance.");
  }
  else if( show_times )
  {
    cl("For each solution group, the running time in seconds of the best");
    cl("solution of each instance, in seconds.  Unlike costs, running");
    cl("times are not verified by HSEval, merely reported based on");
    cl("information in the archive file.");
  }
  /* what is this? cl(NULL); */
  cl("Blank entries indicate that there were no solutions for the instance");
  cl("in the solution group.  `Invalid' means there were solutions, but no");
  cl("valid ones.");
  if( show_costs )
  {
    if( soft_costs_only )
    {
      cl("Each numeric cost entry shows the ObjectiveValue");
      cl("only; solutions with a non-zero Infeasibility");
      cl("value are shown as `inf.'.");
    }
    else
    {
      cl("Each numeric cost entry has format");
      cl("InfeasibilityValue.ObjectiveValue, with any");
      cl("ObjectiveValue larger than 99999 replaced by");
      cl("99999.");
    }
    cl("The minimum costs in each row appear in bold.");
    if( show_relative )
    {
      cl("Following each cost, in the `Rel.' column, is that cost divided");
      cl("by the lowest cost in that row, if the lowest cost is non-zero.");
    }
  }
  if( show_times )
  {
    cl("A dash for running time means that there is a best solution but");
    cl("no running time is recorded in it.");
  }

  /* caption */
  /* ***
  TableAddCaptionLine(res, "For each solution group, the badness of the best");
  TableAddCaptionLine(res, "solution of each instance.  The format of each");
  TableAddCaptionLine(res, "entry is InfeasibilityValue.ObjectiveValue, with");
  TableAddCaptionLine(res, "any ObjectiveValue larger than 99999 replaced by");
  TableAddCaptionLine(res, "99999.  Entries marked * are the best of two or");
  TableAddCaptionLine(res, "more solutions for the same instance in the same");
  TableAddCaptionLine(res, "solution group.  Blank entries indicate that");
  TableAddCaptionLine(res, "there were no solutions for the instance in the");
  TableAddCaptionLine(res, "solution group.  `Invalid' means there were");
  TableAddCaptionLine(res, "solutions, but no valid ones.  The best results");
  TableAddCaptionLine(res, "in each row appear in bold.");
  *** */

  /* average line */
  /* *** withdrawn
  if( show_averages )
    TableAddAv erageRow(res, TABLE_RIGHT, TABLE_BOLD, "%.5f");
  *** */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int ResourceTypeNamesIndex(char *rtype_name)                             */
/*                                                                           */
/*  Return an integer between 0 and 3 inclusive, classifying rtype_name.     */
/*                                                                           */
/*****************************************************************************/

/* ***
int ResourceTypeNamesIndex(char *rtype_name)
{
  int i, j;  char *name;  ARRAY_STRING names;
  sta tic ARRAY_ARRAY_STRING resource_type_names;
  sta tic bool initialized = false;
  if( !initialized )
  {
    initialized = true;
    HaArrayInit(resource_type_names);
    HaArrayInit(names);
    HaArrayAddLast(names, "Teachers");
    HaArrayAddLast(names, "teachers");
    HaArrayAddLast(names, "Teacher");
    HaArrayAddLast(names, "teacher");
    HaArrayAddLast(resource_type_names, names);
    HaArrayInit(names);
    HaArrayAddLast(names, "Rooms");
    HaArrayAddLast(names, "rooms");
    HaArrayAddLast(names, "Room");
    HaArrayAddLast(names, "room");
    HaArrayAddLast(resource_type_names, names);
    HaArrayInit(names);
    HaArrayAddLast(names, "Classes");
    HaArrayAddLast(names, "classes");
    HaArrayAddLast(names, "Class");
    HaArrayAddLast(names, "class");
    HaArrayAddLast(names, "SchoolClass");
    HaArrayAddLast(resource_type_names, names);
    HaArrayInit(names);
    HaArrayAddLast(names, "Students");
    HaArrayAddLast(names, "students");
    HaArrayAddLast(names, "Student");
    HaArrayAddLast(names, "student");
    HaArrayAddLast(resource_type_names, names);
  }
  MArrayForEach(resource_type_names, &names, &i)
  {
    MArrayForEach(names, &name, &j)
      if( strcmp(rtype_name, name) == 0 )
      {
	if( DEBUG1 )
	  fprintf(stderr, "ResourceTypeNamesIndex(%s) success %d\n",
	    rtype_name, i);
	return i;
      }
  }
  if( DEBUG1 )
    fprintf(stderr, "ResourceTypeNamesIndex(%s) fail %d\n",
      rtype_name, MArraySize(resource_type_names));
  return MArraySize(resource_type_names);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void AddIntegerEntry(TABLE_ROW row, int val, HA_ARENA a)                 */
/*                                                                           */
/*  Add an integer entry to row, making it blank if val is 0.                */
/*                                                                           */
/*****************************************************************************/

static void AddIntegerEntry(TABLE_ROW row, int val, HA_ARENA a)
{
  TABLE_ENTRY entry;
  if( val > 0 )
    entry = TableEntryMakeInt(TABLE_RIGHT, TABLE_ROMAN, "%d", val, a);
  else
    entry = TableEntryMakeText(TABLE_RIGHT, TABLE_ROMAN, NULL, a);
  TableRowAddEntry(row, entry);
}


/*****************************************************************************/
/*                                                                           */
/*  TABLE InstanceTableBuild(KHE_ARCHIVE archive, HA_ARENA a)                */
/*                                                                           */
/*  Build table of instance statistics.                                      */
/*                                                                           */
/*****************************************************************************/

static TABLE InstanceTableBuild(KHE_ARCHIVE archive, HA_ARENA a)
{
  TABLE res;  TABLE_ROW row;  TABLE_ENTRY entry;  char *str;  KHE_EVENT e;
  KHE_INSTANCE ins;  int i, j, k, count;  KHE_RESOURCE_TYPE rt;
  KHE_CONSTRAINT c; MODEL model; MODEL_RESOURCE_TYPE mrt, mrt2;
  static char buff[50];  /* used for caption in table, must persist */
  if( DEBUG1 )
    fprintf(stderr, "[ InstanceTableBuild(archive)\n");

  /* table */
  model = ModelBuild(KheArchiveModel(archive), a);
  res = TableMake(ModelInstancesSummaryHeader(model), a);

  /* header row */
  row = TableRowMake(a);
  TableAddRow(res, row);
  entry = TableEntryMakeText(TABLE_LEFT, TABLE_BOLD, "Instance", a);
  TableRowAddEntry(row, entry);
  entry = TableEntryMakeText(TABLE_CENTRE, TABLE_BOLD, "Times", a);
  TableRowAddEntry(row, entry);
  for( i = 0;  i < ModelModelResourceTypeCount(model);  i++ )
  {
    mrt = ModelModelResourceType(model, i);
    entry = TableEntryMakeText(TABLE_CENTRE, TABLE_BOLD,
      ModelResourceTypeInitName(mrt), a);
    TableRowAddEntry(row, entry);
  }
  entry = TableEntryMakeText(TABLE_CENTRE, TABLE_BOLD, "Events", a);
  TableRowAddEntry(row, entry);
  entry = TableEntryMakeText(TABLE_CENTRE, TABLE_BOLD, "D", a);
  TableRowAddEntry(row, entry);
  entry = TableEntryMakeText(TABLE_CENTRE, TABLE_BOLD, "C", a);
  TableRowAddEntry(row, entry);

  /* one row for each instance */
  for( i = 0;  i < KheArchiveInstanceCount(archive);  i++ )
  {
    /* row with header column showing instance name */
    ins = KheArchiveInstance(archive, i);
    if( DEBUG1 )
      fprintf(stderr, "  at instance %d (%s): %d types\n", i,
	KheInstanceId(ins), KheInstanceResourceTypeCount(ins));
    row = TableRowMake(a);
    TableAddRow(res, row);
    str = RemovePercent(KheInstanceId(ins));
    entry = TableEntryMakeText(TABLE_LEFT, TABLE_ROMAN, str, a);
    TableRowAddEntry(row, entry);

    /* Times */
    AddIntegerEntry(row, KheInstanceTimeCount(ins), a);

    /* Teachers, Rooms, Classes, Students, and Other resources */
    for( j = 0;  j < ModelModelResourceTypeCount(model);  j++ )
    {
      mrt = ModelModelResourceType(model, j);
      count = 0;
      for( k = 0;  k < KheInstanceResourceTypeCount(ins);  k++ )
      {
	rt = KheInstanceResourceType(ins, k);
	mrt2 = ModelRetrieve(model, KheResourceTypeName(rt));
	if( mrt2 == mrt )
	  count += KheResourceTypeResourceCount(rt);
      }
      AddIntegerEntry(row, count, a);
    }

    /* ***
    for( j = 0;  j < 5;  j++ )
    {
      count = 0;
      for( k = 0;  k < KheInstanceResourceTypeCount(ins);  k++ )
      {
	rt = KheInstanceResourceType(ins, k);
	if( ResourceTypeNamesIndex(KheResourceTypeName(rt)) == j )
	  count += KheResourceTypeResourceCount(rt);
      }
      AddIntegerEntry(row, count, a);
    }
    *** */

    /* Events */
    AddIntegerEntry(row, KheInstanceEventCount(ins), a);

    /* Event durations */
    count = 0;
    for( j = 0;  j < KheInstanceEventCount(ins);  j++ )
    {
      e = KheInstanceEvent(ins, j);
      count += KheEventDuration(e);
    }
    AddIntegerEntry(row, count, a);

    /* Points of application */
    count = 0;
    for( j = 0;  j < KheInstanceConstraintCount(ins);  j++ )
    {
      c = KheInstanceConstraint(ins, j);
      count += KheConstraintAppliesToCount(c);
    }
    AddIntegerEntry(row, count, a);
  }

  /* caption */
  TableAddCaptionLine(res, "For each instance, the number of times, resources");
  TableAddCaptionLine(res, "of various kinds, and events; plus the total");
  TableAddCaptionLine(res, "duration of events (D), and the total number of");
  TableAddCaptionLine(res, "points of application of constraints (C).");
  if( KheArchiveInstanceCount(archive) >= 6 )
  {
    snprintf(buff, 50, "There are %d instances altogether.",
      KheArchiveInstanceCount(archive));
    TableAddCaptionLine(res, buff);
  }
  if( DEBUG1 )
    fprintf(stderr, "] InstanceTableBuild returning\n");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  TABLE ConstraintTableBuild(KHE_ARCHIVE archive, bool first,              */
/*    KHE_CONSTRAINT_TAG first_tag, KHE_CONSTRAINT_TAG tag_limit, HA_ARENA a)*/
/*                                                                           */
/*  Return a table summarizing the constraints structure of each instance.   */
/*                                                                           */
/*****************************************************************************/

static TABLE ConstraintTableBuild(KHE_ARCHIVE archive, bool first,
  KHE_CONSTRAINT_TAG first_tag, KHE_CONSTRAINT_TAG tag_limit, HA_ARENA a)
{
  TABLE res;  KHE_CONSTRAINT_TAG tag;  TABLE_ROW row;  TABLE_ENTRY entry;
  KHE_INSTANCE ins;  char *str;  int i, j;  KHE_CONSTRAINT c;
  bool has_soft[KHE_CONSTRAINT_TAG_COUNT], has_hard[KHE_CONSTRAINT_TAG_COUNT];

  /* table */
  res = TableMake(first ? "Constraint summary" : NULL, a);

  /* header row */
  row = TableRowMake(a);
  TableAddRow(res, row);
  entry = TableEntryMakeText(TABLE_LEFT, TABLE_BOLD, "Instance", a);
  TableRowAddEntry(row, entry);
  for( tag = first_tag;  tag < tag_limit;  tag++ )
  {
    entry = TableEntryMakeText(TABLE_CENTRE, TABLE_BOLD,
      KheConstraintTagShowSpaced(tag), a);
    TableRowAddEntry(row, entry);
  }

  /* one row for each instance */
  for( i = 0;  i < KheArchiveInstanceCount(archive);  i++ )
  {
    /* initialize the data we want to print */
    ins = KheArchiveInstance(archive, i);
    for( tag = 0;  tag < KHE_CONSTRAINT_TAG_COUNT;  tag++ )
      has_soft[tag] = has_hard[tag] = 0;
    for( j = 0;  j < KheInstanceConstraintCount(ins);  j++ )
    {
      c = KheInstanceConstraint(ins, j);
      if( KheConstraintWeight(c) > 0 )
      {
	if( KheConstraintRequired(c) )
          has_hard[KheConstraintTag(c)] = true;
	else
          has_soft[KheConstraintTag(c)] = true;
      }
    }

    /* row with header column showing instance name */
    row = TableRowMake(a);
    TableAddRow(res, row);
    str = RemovePercent(KheInstanceId(ins));
    entry = TableEntryMakeText(TABLE_LEFT, TABLE_ROMAN, str, a);
    TableRowAddEntry(row, entry);

    /* one entry for each constraint tag */
    for( tag = first_tag;  tag < tag_limit;  tag++ )
    {
      if( has_hard[tag] )
	entry = TableEntryMakeText(TABLE_CENTRE, TABLE_ROMAN, "*", a);
      else if( has_soft[tag] )
	entry = TableEntryMakeText(TABLE_CENTRE, TABLE_ROMAN, ".", a);
      else
	entry = TableEntryMakeText(TABLE_LEFT, TABLE_ROMAN, NULL, a);
      TableRowAddEntry(row, entry);
    }
  }

  /* caption */
  if( !first )
  {
    TableAddCaptionLine(res, "For each type of constraint, the instances that");
    TableAddCaptionLine(res, "contain at least one constraint of that type");
    TableAddCaptionLine(res, "with non-zero weight.  An asterisk means that");
    TableAddCaptionLine(res, "at least one of these constraints is Required.");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void InstanceFindDensities(KHE_INSTANCE ins, int counts[], int totals[]) */
/*                                                                           */
/*  Find the constraint densities for all constraint types.                  */
/*                                                                           */
/*****************************************************************************/

/* *** equivalent code pushed down into KHE now
static void InstanceFindDensities(KHE_INSTANCE ins, int counts[], int totals[])
{
  int i, j, unpreass_event_resources, unpreass_events;  KHE_CONSTRAINT c;
  KHE_CONSTRAINT_TAG tag;  KHE_EVENT_GROUP eg;  KHE_EVENT_RESOURCE er;
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT asac;  KHE_EVENT e;
  KHE_SPREAD_EVENTS_CONSTRAINT sec;
  KHE_LINK_EVENTS_CONSTRAINT lec;

  ** visit every constraint to get the counts; with three exceptions **
  ** these are just the number of points of application              **
  for( tag = 0;  tag < KHE_CONSTRAINT_TAG_COUNT;  tag++ )
    counts[tag] = 0;
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    tag = KheConstraintTag(c);
    switch( tag )
    {
      case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:
      case KHE_ASSIGN_TIME_CONSTRAINT_TAG:
      case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:
      case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:
      case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:
      case KHE_PREFER_TIMES_CONSTRAINT_TAG:

	counts[tag] += KheConstraintAppliesToCount(c);
	break;

      case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:

	asac = (KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT) c;
	for(j=0; j<KheAvoidSplitAssignmentsConstraintEventGroupCount(asac); j++)
          counts[tag] +=
	    KheAvoidSplitAssignmentsConstraintEventResourceCount(asac, j);
	break;

      case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:

	sec = (KHE_SPREAD_EVENTS_CONSTRAINT) c;
	for( j = 0;  j < KheSpreadEventsConstraintEventGroupCount(sec);  j++ )
	{
	  eg = KheSpreadEventsConstraintEventGroup(sec, j);
          counts[tag] += KheEventGroupEventCount(eg);
	}
	break;

      case KHE_LINK_EVENTS_CONSTRAINT_TAG:

	lec = (KHE_LINK_EVENTS_CONSTRAINT) c;
	for( j = 0;  j < KheLinkEventsConstraintEventGroupCount(lec);  j++ )
	{
	  eg = KheLinkEventsConstraintEventGroup(lec, j);
          counts[tag] += KheEventGroupEventCount(eg);
	}
	break;

      case KHE_ORDER_EVENTS_CONSTRAINT_TAG:

	** still to do **
	break;

      case KHE_AVOID_CLASHES_CONSTRAINT_TAG:
      case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:
      case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:
      case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:
      case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:
      case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:

	counts[tag] += KheConstraintAppliesToCount(c);
	break;

      default:

	HnAssert(false, "hseval internal error (InstanceFindDensities)");
    }
  }

  ** find the number of unpreassigned event resources **
  unpreass_event_resources = 0;
  for( i = 0;  i < KheInstanceEventResourceCount(ins);  i++ )
  {
    er = KheInstanceEventResource(ins, i);
    if( KheEventResourcePreassignedResource(er) == NULL )
      unpreass_event_resources++;
  }

  ** find the number of unpreassigned events **
  unpreass_events = 0;
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    if( KheEventPreassignedTime(e) == NULL )
      unpreass_events++;
  }

  ** set the totals for all tags **
  totals[KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG] = unpreass_event_resources;
  totals[KHE_ASSIGN_TIME_CONSTRAINT_TAG] = unpreass_events;
  totals[KHE_SPLIT_EVENTS_CONSTRAINT_TAG] = KheInstanceEventCount(ins);
  totals[KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG] =
    KheInstanceEventCount(ins);
  totals[KHE_PREFER_RESOURCES_CONSTRAINT_TAG] = unpreass_event_resources;
  totals[KHE_PREFER_TIMES_CONSTRAINT_TAG] = unpreass_events;
  totals[KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG] = unpreass_event_resources;
  totals[KHE_SPREAD_EVENTS_CONSTRAINT_TAG] = KheInstanceEventCount(ins);
  totals[KHE_LINK_EVENTS_CONSTRAINT_TAG] = KheInstanceEventCount(ins);
  totals[KHE_ORDER_EVENTS_CONSTRAINT_TAG] = 0;
  totals[KHE_AVOID_CLASHES_CONSTRAINT_TAG] = KheInstanceResourceCount(ins);
  totals[KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG] =
    KheInstanceResourceCount(ins);
  totals[KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG] = KheInstanceResourceCount(ins);
  totals[KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG] = KheInstanceResourceCount(ins);
  totals[KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG] = KheInstanceResourceCount(ins);
  totals[KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG] = KheInstanceResourceCount(ins);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  TABLE DensityTableBuild(KHE_ARCHIVE archive, bool first,                 */
/*    KHE_CONSTRAINT_TAG first_tag, KHE_CONSTRAINT_TAG tag_limit)             */
/*                                                                           */
/*  Return a table summarizing the constraint density of each instance.      */
/*                                                                           */
/*****************************************************************************/

static TABLE DensityTableBuild(KHE_ARCHIVE archive, bool first,
  KHE_CONSTRAINT_TAG first_tag, KHE_CONSTRAINT_TAG tag_limit, HA_ARENA a)
{
  TABLE res;  KHE_CONSTRAINT_TAG tag;  TABLE_ROW row;  TABLE_ENTRY entry;
  /* int counts[KHE_CONSTRAINT_TAG_COUNT], totals[KHE_CONSTRAINT_TAG_COUNT]; */
  KHE_INSTANCE ins;  char *str;  int i, count, total;
  if( DEBUG2 )
    fprintf(stderr, "DensityTableBuild(archive, %s, %d, %d)\n",
      first ? "true" : "false", first_tag, tag_limit);

  /* table */
  res = TableMake(first ? "Constraint density" : NULL, a);

  /* header row */
  row = TableRowMake(a);
  TableAddRow(res, row);
  entry = TableEntryMakeText(TABLE_LEFT, TABLE_BOLD, "Instance", a);
  TableRowAddEntry(row, entry);
  for( tag = first_tag;  tag < tag_limit;  tag++ )
  {
    entry = TableEntryMakeText(TABLE_CENTRE, TABLE_BOLD,
      KheConstraintTagShowSpaced(tag), a);
    TableRowAddEntry(row, entry);
  }

  /* one row for each instance */
  for( i = 0;  i < KheArchiveInstanceCount(archive);  i++ )
  {
    /* row with header column showing instance name */
    ins = KheArchiveInstance(archive, i);
    /* InstanceFindDensities(ins, counts, totals); */
    row = TableRowMake(a);
    TableAddRow(res, row);
    str = RemovePercent(KheInstanceId(ins));
    entry = TableEntryMakeText(TABLE_LEFT, TABLE_ROMAN, str, a);
    TableRowAddEntry(row, entry);

    /* one entry for each constraint tag */
    for( tag = first_tag;  tag < tag_limit;  tag++ )
    {
      count = KheInstanceConstraintDensityCount(ins, tag);
      total = KheInstanceConstraintDensityTotal(ins, tag);
      if( count < 0 || total < 0 )
	entry = TableEntryMakeText(TABLE_CENTRE, TABLE_ROMAN, "still to do", a);
      else if( count == 0 || total == 0 )
	entry = TableEntryMakeText(TABLE_CENTRE, TABLE_ROMAN, NULL, a);
      else
	entry = TableEntryMakeDouble(TABLE_CENTRE, TABLE_ROMAN,
	  "%.1f", (double) count / total, a);
      TableRowAddEntry(row, entry);
    }
  }

  /* caption */
  if( !first )
  {
    TableAddCaptionLine(res, "For each type of constraint, the average number");
    TableAddCaptionLine(res, "of applications of that constraint.");

    TableAddCaptionLine(res, "For assign resource constraints and prefer");
    TableAddCaptionLine(res, "resources constraints, this is the total");
    TableAddCaptionLine(res, "number of points of application divided by");
    TableAddCaptionLine(res, "the total number of event resources without");
    TableAddCaptionLine(res, "preassigned resources.");

    TableAddCaptionLine(res, "For assign time and prefer times constraints,");
    TableAddCaptionLine(res, "this is the total number of points of");
    TableAddCaptionLine(res, "application divided by the total number");
    TableAddCaptionLine(res, "of events without preassigned times.");

    TableAddCaptionLine(res, "For split events and distribute split events");
    TableAddCaptionLine(res, "constraints, this is the total number of");
    TableAddCaptionLine(res, "points of application divided by the total");
    TableAddCaptionLine(res, "number of events.");

    TableAddCaptionLine(res, "For avoid split assignments constraints, this");
    TableAddCaptionLine(res, "is the total number of events in all the points");
    TableAddCaptionLine(res, "of application (i.e. the total number of event");
    TableAddCaptionLine(res, "resources in all points of application),");
    TableAddCaptionLine(res, "divided by the total number of event resources");
    TableAddCaptionLine(res, "without preassigned resources.");

    TableAddCaptionLine(res, "For spread events constraints and link events");
    TableAddCaptionLine(res, "constraints, this is the total number of events");
    TableAddCaptionLine(res, "in all the points of application, divided by");
    TableAddCaptionLine(res, "the total number of events.");

    TableAddCaptionLine(res, "For the six resource constraints, this is the");
    TableAddCaptionLine(res, "total number of points of application divided");
    TableAddCaptionLine(res, "by the total number of resources.");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void SummaryEvalPrint(KHE_ARCHIVE archive, PRINT_FORMAT print_format)    */
/*                                                                           */
/*  Build and print a table of type table_type, summarising the results in   */
/*  archive, which has been evaluated.                                       */
/*                                                                           */
/*****************************************************************************/

void SummaryEvalPrint(KHE_ARCHIVE archive, PRINT_FORMAT print_format,
  HA_ARENA_SET as)
{
  HTML html;  TABLE eval_table, ins_table;
  TABLE  constraint_table, density_table;
  char buff[100];  KHE_CONSTRAINT_TAG half_tag;  bool non_empty;
  HA_ARENA a;  EVAL_TABLE et;
  a = HaArenaMake(as);
  switch( print_format )
  {
    case PRINT_HTML:

      /* page header */
      snprintf(buff, 100, "%s Evaluations Summary",
        KheArchiveId(archive) != NULL && strlen(KheArchiveId(archive)) < 60 ?
        KheArchiveId(archive) : "HSEval");
      html = PageBegin(buff);
      HTMLBigHeading(html, buff);

      /* metadata, if present */
      HTMLParagraphBegin(html);
      HTMLText(html, KheArchiveMetaDataText(archive));
      HTMLParagraphEnd(html);
      /* ***
      md = KheArchiveMetaData(archive);
      if( md != NULL )
      {
	HTMLParagraphBegin(html);
	HTMLText(html, "This archive is");
	HTMLTextNoBreak(html, KheArchiveMetaDataName(md));
	HTMLText(html, ", assembled by");
	HTMLText(html, KheArchiveMetaDataContributor(md));
	HTMLText(html, "on");
	HTMLTextNoBreak(html, KheArchiveMetaDataDate(md));
	HTMLText(html, ".");
	HTMLText(html, KheArchiveMetaDataDescription(md));
	if( KheArchiveMetaDataRemarks(md) != NULL )
	  HTMLText(html, KheArchiveMetaDataRemarks(md));
	HTMLParagraphEnd(html);
      }
      *** */

      /* evaluation tables */
      if( KheArchiveInstanceCount(archive) == 0 )
      {
	HTMLParagraphBegin(html);
	HTMLText(html, "This archive contains no instances.");
	HTMLParagraphEnd(html);
      }
      else if( KheArchiveSolnGroupCount(archive) == 0 )
      {
	HTMLParagraphBegin(html);
	HTMLText(html, "This archive contains no solution groups.");
	HTMLParagraphEnd(html);
      }
      else
      {
	/* cost table */
	/* ***
	eval_table = EvalTableBuild(archive, true, false, false, true,
	  false, true, NULL, false, &non_empty, a);
	*** */
	et = EvalTableMake(archive, NULL, a);

	/* best costs table */
	EvalTableAddColumnFormat(et, EVAL_COLUMN_DATA_BEST_COST, false);
	EvalTableAddColumnFormat(et, EVAL_COLUMN_DATA_RELATIVE_BEST_COST,false);
	TablePrintHTML(EvalTableMakeTable(et, true, false), html);
	EvalTableClearColumnFormats(et);

	/* average costs table */
	/* *** correct but not wanted just now
	EvalTableAddColumnFormat(et, EVAL_COLUMN_DATA_AVERAGE_COST, false);
	EvalTableAddColumnFormat(et, EVAL_COLUMN_DATA_RELATIVE_AVERAGE_COST,
	  false);
	TablePrintHTML(EvalTableMakeTable(et, true, false), html);
	EvalTableClearColumnFormats(et);
	*** */

	/* eval_table after uncompetitive columns are reduced */
	/* ***
	if( TableRemoveNonBoldColumns(eval_table) > 0 )
	{
	  TableSetHeader(eval_table,
	    "Solution badness (competitive solution sets only)");
	  TableClearCaption(eval_table);
	  TableAddCaptionLine(eval_table, "The previous table minus columns");
	  TableAddCaptionLine(eval_table, "with no bold entries.");
	  TablePrintHTML(eval_table, html);
	}
	*** */

	/* running time table */
	/* et = EvalTableMake(archive, true, NULL, a); */
	EvalTableAddColumnFormat(et, EVAL_COLUMN_DATA_AVERAGE_RUNNING_TIME,
	  false);
	TablePrintHTML(EvalTableMakeTable(et, true, false), html);
	EvalTableClearColumnFormats(et);
	/* ***
	 EvalTableBuild(KHE_ARCHIVE archive, bool show_best_costs,
  bool soft_costs_only, bool average_costs, bool show_relative,
  bool show_times, bool show_averages, char *instance_id,
  bool show_best_count, bool *non_empty, HA_ARENA a);
	running_time_table = EvalTableBuild(archive, false, false, false,
	  false, true, true, NULL, false, &non_empty, a);
	running_time_table = RunningTimeTableBuild(archive, true, &non_empty);
	if( non_empty )
	  TablePrintHTML(running_time_table, html);
	else
	{
	  HTMLParagraphBegin(html);
	  HTMLText(html, "This archive contains no running times.");
	  HTMLParagraphEnd(html);
	}
	*** */
      }

      /* invalid solutions */
      ArchiveReportInvalidSolns(archive, html);

      /* instances summary */
      if( KheArchiveInstanceCount(archive) > 0 )
      {
	/* instances */
	ins_table = InstanceTableBuild(archive, a);
	TablePrintHTML(ins_table, html);

	/* constraint summary */
        half_tag = KHE_CONSTRAINT_TAG_COUNT / 2;
	constraint_table = ConstraintTableBuild(archive, true,
	  KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG, half_tag, a);
	TablePrintHTML(constraint_table, html);
	constraint_table = ConstraintTableBuild(archive, false,
	  half_tag, KHE_CONSTRAINT_TAG_COUNT, a);
	TablePrintHTML(constraint_table, html);

	/* constraint density */
	density_table = DensityTableBuild(archive, true,
	  KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG, half_tag, a);
	TablePrintHTML(density_table, html);
	density_table = DensityTableBuild(archive, false,
	  half_tag, KHE_CONSTRAINT_TAG_COUNT, a);
	TablePrintHTML(density_table, html);
      }

      /* page footer */
      HTMLParagraphBegin(html);
      HTMLText(html, "Return to the ");
      HTMLJumpFront(html);
      HTMLText(html, ".");
      HTMLParagraphEnd(html);
      PageEnd(html);
      break;

    case PRINT_LATEX:

      /* page header */
      fprintf(stdout, "Content-Type: text/latex\n");
      fprintf(stdout,
	"Content-Disposition: attachment; filename=hseval.tex\n\n");

      /* evaluation tables */
      eval_table = EvalTableBuild(archive, true, false, false, false,
	false, true, NULL, false, &non_empty, a);
      if( TableRowCount(eval_table) <= 1 )
	printf("%%%s\n", "This archive contains no instances.");
      else if( TableColCount(eval_table) <= 1 )
	printf("%%%s\n", "This archive contains no solution groups.");
      else
      {
	TablePrintLaTeX(eval_table, stdout);
	TableRemoveNonBoldColumns(eval_table);
	TableSetHeader(eval_table,
	  "Solution badness (competitive solution sets only)");
	TableClearCaption(eval_table);
	TableAddCaptionLine(eval_table, "The same table, but with columns");
	TableAddCaptionLine(eval_table, "with no bold entries removed.");
	TablePrintLaTeX(eval_table, stdout);
      }

      /* running time table */
      /* seems to have always been missing;  adding it is still to do */

      /* instance summary */
      if( TableRowCount(eval_table) > 1 )
      {
	/* instances */
	ins_table = InstanceTableBuild(archive, a);
	TablePrintLaTeX(ins_table, stdout);

	/* constraint summary */
        half_tag = KHE_CONSTRAINT_TAG_COUNT / 2;
	constraint_table = ConstraintTableBuild(archive, true,
	  KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG, half_tag, a);
	TablePrintLaTeX(constraint_table, stdout);
	constraint_table = ConstraintTableBuild(archive, false,
	  half_tag, KHE_CONSTRAINT_TAG_COUNT, a);
	TablePrintLaTeX(constraint_table, stdout);

	/* constraint density */
	density_table = DensityTableBuild(archive, true,
	  KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG, half_tag, a);
	TablePrintLaTeX(density_table, stdout);
	density_table = DensityTableBuild(archive, false,
	  half_tag, KHE_CONSTRAINT_TAG_COUNT, a);
	TablePrintLaTeX(density_table, stdout);
      }
      break;

    default:

      HnAssert(false, "hseval internal error (SummaryEvalPrint)");
  }
  HaArenaDelete(a);
}


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
/*  FILE:         report.c                                                   */
/*  MODULE:       ArchiveReportHTML                                          */
/*                                                                           */
/*****************************************************************************/
#include "externs.h"

/*****************************************************************************/
/*                                                                           */
/*  SUMMARY                                                                  */
/*                                                                           */
/*****************************************************************************/

typedef struct summary_rec {
  KHE_CONSTRAINT_TAG	constraint_type;	/* constraint type           */
  int			points;			/* points of application     */
  int			hard_cost;		/* hard cost                 */
  int			soft_cost;		/* soft cost                 */
} *SUMMARY;

typedef HA_ARRAY(SUMMARY) ARRAY_SUMMARY;


/*****************************************************************************/
/*                                                                           */
/*  void EndTable(KHE_CONSTRAINT c, int points, KHE_COST cost,               */
/*    HTML html, ARRAY_SUMMARY *summary_array)                               */
/*                                                                           */
/*  This function is called at the end of each table.  It prints the         */
/*  total row, ends the table, and adds an entry to the summary array.       */
/*                                                                           */
/*****************************************************************************/

static void EndTable(KHE_CONSTRAINT c, int points, KHE_COST cost,
  HTML html, ARRAY_SUMMARY *summary_array, HA_ARENA a)
{
  SUMMARY summary;  int hard_cost, soft_cost;

  /* end the table */
  hard_cost = KheHardCost(cost);
  soft_cost = KheSoftCost(cost);
  if( points > 1 )
  {
    HTMLTableRowBegin(html);
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 6);
    HTMLTextBold(html, "Total");
    if( points > 0 )
      HTMLText(html, points == 1 ? "(%d point)" : "(%d points)", points);
    HTMLTableEntryEnd(html);
    /* ***
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    *** */
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    HTMLTableEntryCentredBegin(html);
    if( hard_cost > 0 )
      HTMLText(html, "%d", hard_cost);
    else
      HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    HTMLTableEntryCentredBegin(html);
    if( soft_cost > 0 )
      HTMLText(html, "%d", soft_cost);
    else
      HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    HTMLTableRowEnd(html);
  }
  HTMLTableEnd(html);
  HTMLParagraphEnd(html);

  /* add to the summary */
  HaMake(summary, a);
  summary->constraint_type = KheConstraintTag(c);
  summary->points = points;
  summary->hard_cost = hard_cost;
  summary->soft_cost = soft_cost;
  HaArrayAddLast(*summary_array, summary);
}


/*****************************************************************************/
/*                                                                           */
/*  void EndLowerBoundTable(KHE_CONSTRAINT c, int points, KHE_COST cost,     */
/*    HTML html, ARRAY_SUMMARY *summary_array, HA_ARENA a)                   */
/*                                                                           */
/*  This function is called at the end of each lower bound table.  It prints */
/*  the total row, ends the table, and adds an entry to the summary array.   */
/*                                                                           */
/*****************************************************************************/

static void EndLowerBoundTable(KHE_CONSTRAINT c, int points, KHE_COST cost,
  HTML html, ARRAY_SUMMARY *summary_array, HA_ARENA a)
{
  SUMMARY summary;  int hard_cost, soft_cost;

  /* end the table */
  hard_cost = KheHardCost(cost);
  soft_cost = KheSoftCost(cost);
  if( points > 1 )
  {
    HTMLTableRowBegin(html);
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 6);
    HTMLTextBold(html, "Total");
    if( points > 0 )
      HTMLText(html, points == 1 ? "(%d point)" : "(%d points)", points);
    HTMLTableEntryEnd(html);
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    /* *** calculation
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    *** */
    HTMLTableEntryCentredBegin(html);
    if( hard_cost > 0 )
      HTMLText(html, "%d", hard_cost);
    else
      HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    HTMLTableEntryCentredBegin(html);
    if( soft_cost > 0 )
      HTMLText(html, "%d", soft_cost);
    else
      HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    HTMLTableRowEnd(html);
  }
  HTMLTableEnd(html);
  HTMLParagraphEnd(html);

  /* add to the summary */
  HaMake(summary, a);
  summary->constraint_type = KheConstraintTag(c);
  summary->points = points;
  summary->hard_cost = hard_cost;
  summary->soft_cost = soft_cost;
  HaArrayAddLast(*summary_array, summary);
}


/*****************************************************************************/
/*                                                                           */
/*  void SummaryTableReportHTML(ARRAY_SUMMARY *summary_array,                */
/*    char *bgcolor, HTML html)                                              */
/*                                                                           */
/*  Print a table showing summary_array onto html in bgcolor.                */
/*                                                                           */
/*****************************************************************************/

static void SummaryTableReportHTML(ARRAY_SUMMARY *summary_array,
  char *bgcolor, HTML html)
{
  SUMMARY summary;  int i, total_points, total_hard_cost, total_soft_cost;

  /* header row */
  HTMLParagraphBegin(html);
  HTMLTableBegin(html, bgcolor);
  HTMLTableRowBegin(html);
  HTMLTableEntryTextBold(html, "Summary");
  HTMLTableEntryTextBold(html, "Inf.");
  HTMLTableEntryTextBold(html, "Obj.");
  HTMLTableRowEnd(html);

  /* one row for each constraint that costed */
  total_points = total_hard_cost = total_soft_cost = 0;
  HaArrayForEach(*summary_array, summary, i)
  {
    HTMLTableRowBegin(html);
    HTMLTableEntryBegin(html);
    HTMLTextBold(html, KheConstraintTagShowSpaced(summary->constraint_type));
    if( summary->points > 0 )
      HTMLText(html,
	summary->points == 1 ? "(%d point)" : "(%d points)", summary->points);
    HTMLTableEntryEnd(html);
    HTMLTableEntryCentredBegin(html);
    if( summary->hard_cost > 0 )
      HTMLText(html, "%d", summary->hard_cost);
    else
      HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    HTMLTableEntryCentredBegin(html);
    if( summary->soft_cost > 0 )
      HTMLText(html, "%d", summary->soft_cost);
    else
      HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    HTMLTableRowEnd(html);
    total_points += summary->points;
    total_hard_cost += summary->hard_cost;
    total_soft_cost += summary->soft_cost;
  }

  /* concluding grand total row */
  HTMLTableRowBegin(html);
  HTMLTableEntryBegin(html);
  HTMLHSpace(html, 6);
  HTMLTextBold(html, "Grand total");
  if( total_points > 0 )
    HTMLText(html, "(%d points)", total_points);
  HTMLTableEntryEnd(html);
  HTMLTableEntryCentredBegin(html);
  if( total_hard_cost > 0 )
    HTMLText(html, "%d", total_hard_cost);
  else
    HTMLHSpace(html, 2);
  HTMLTableEntryEnd(html);
  HTMLTableEntryCentredBegin(html);
  if( total_soft_cost > 0 )
    HTMLText(html, "%d", total_soft_cost);
  else
    HTMLHSpace(html, 2);
  HTMLTableEntryEnd(html);
  HTMLTableRowEnd(html);
  HTMLTableEnd(html);
  HTMLParagraphEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void MonitorReportHTML(KHE_MONITOR m, bool applies_to_first,             */
/*    int *points, KHE_COST *cost, HTML html)                                */
/*                                                                           */
/*  Print a table row for m, and update *points and *cost.                   */
/*  This function assumes that KheMonitorConstraint(m) is non-NULL.          */
/*                                                                           */
/*****************************************************************************/

static void MonitorReportHTML(KHE_MONITOR m, int *points, KHE_COST *cost,
  HTML html)
{
  KHE_CONSTRAINT c;  char *description;

  /* boilerplate */
  c = KheMonitorConstraint(m);
  HTMLTableRowBegin(html);

  /* constraint Id */
  /* HTMLTableEntryText(html, KheConstraintId(c)); */

  /* constraint name */
  HTMLTableEntryText(html, KheConstraintName(c));

  /* point of application */
  HTMLTableEntryText(html, KheMonitorId(m));
  /* HTMLTableEntryText(html, KheMonitorPointOfApplication(m)); */

  /* calculation */
  HTMLTableEntryBegin(html);
  HTMLText(html, "%d * ", KheConstraintWeight(c));
  HTMLTextNoBreak(html, KheCostFunctionShow(KheConstraintCostFunction(c)));
  HTMLTextNoBreak(html, "(");
  description = KheMonitorDeviationDescription(m);
  HTMLTextNoBreak(html, description);
  /* MFree(description);  no longer allowed */
  HTMLText(html, ")");
  HTMLTableEntryEnd(html);

  /* number of points, and cost */
  *points += 1;
  *cost += KheMonitorCost(m);

  /* infeasibility value */
  HTMLTableEntryCentredBegin(html);
  if( KheConstraintRequired(c) )
    HTMLText(html, "%d", KheHardCost(KheMonitorCost(m)));
  else
    HTMLHSpace(html, 2);
  HTMLTableEntryEnd(html);

  /* objective value */
  HTMLTableEntryCentredBegin(html);
  if( !KheConstraintRequired(c) )
    HTMLText(html, "%d", KheSoftCost(KheMonitorCost(m)));
  else
    HTMLHSpace(html, 2);
  HTMLTableEntryEnd(html);

  HTMLTableRowEnd(html);
}


/* *** old version
void MonitorReportHTML(KHE_MONITOR m, bool applies_to_first,
  int *points, KHE_COST *cost, HTML html)
{
  KHE_CONSTRAINT c;  ** int i, dev, count; **  char *description;

  ** constraint tag or entity applies to **
  c = KheMonitorConstraint(m);
  HTMLTableRowBegin(html);
  HTMLTableEntryText(html, applies_to_first ? KheMonitorAppliesToName(m) :
    KheConstraintTagShowSpaced(KheConstraintTag(c)));

  ** constraint name **
  HTMLTableEntryText(html, KheConstraintName(c));

  ** calculation **
  HTMLTableEntryBegin(html);
  HTMLTextNoBreak(html, "%d * ", KheConstraintWeight(c));
  HTMLTextNoBreak(html, KheCostFunctionShow(KheConstraintCostFunction(c)));
  HTMLTextNoBreak(html, "(");
  description = KheMonitorDeviationDescription(m);
  HTMLTextNoBreak(html, description);
  MFree(description);
  HTMLText(html, ")");
  HTMLTableEntryEnd(html);

  ** number of points, and cost **
  *points += 1;
  *cost += KheMonitorCost(m);

  ** infeasibility value **
  HTMLTableEntryCentredBegin(html);
  if( KheConstraintRequired(c) )
    HTMLText(html, "%d", KheHardCost(KheMonitorCost(m)));
  else
    HTMLHSpace(html, 2);
  HTMLTableEntryEnd(html);

  ** objective value **
  HTMLTableEntryCentredBegin(html);
  if( !KheConstraintRequired(c) )
    HTMLText(html, "%d", KheSoftCost(KheMonitorCost(m)));
  else
    HTMLHSpace(html, 2);
  HTMLTableEntryEnd(html);

  HTMLTableRowEnd(html);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int MonitorCmp(const void *t1, const void *t2)                           */
/*                                                                           */
/*  Comparison functions for grouping monitors of the same type.             */
/*                                                                           */
/*****************************************************************************/

static KHE_EVENT_RESOURCE KheLimitResourcesMonitorFirstEventResource(
  KHE_LIMIT_RESOURCES_MONITOR lrm)
{
  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  int eg_index, er_count;
  lrc = KheLimitResourcesMonitorConstraint(lrm);
  eg_index = KheLimitResourcesMonitorEventGroupIndex(lrm);
  er_count = KheLimitResourcesConstraintEventResourceCount(lrc, eg_index);
  HnAssert(er_count > 0, "MonitorCmp internal error 1");
  return KheLimitResourcesConstraintEventResource(lrc, eg_index, 0);
}


static int MonitorCmp(const void *t1, const void *t2)
{
  KHE_MONITOR m1 = * (KHE_MONITOR *) t1;
  KHE_MONITOR m2 = * (KHE_MONITOR *) t2;
  KHE_LIMIT_RESOURCES_MONITOR lrm1, lrm2;
  KHE_ASSIGN_RESOURCE_MONITOR arm1, arm2;
  KHE_EVENT_RESOURCE er1, er2;
  if( KheMonitorTag(m1) != KheMonitorTag(m2) )
    return KheMonitorTag(m1) - KheMonitorTag(m2);
  switch( KheMonitorTag(m1) )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      arm1 = (KHE_ASSIGN_RESOURCE_MONITOR) m1;
      er1 = KheAssignResourceMonitorEventResource(arm1);
      arm2 = (KHE_ASSIGN_RESOURCE_MONITOR) m2;
      er2 = KheAssignResourceMonitorEventResource(arm2);
      return KheEventResourceInstanceIndex(er1) -
	KheEventResourceInstanceIndex(er2);

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      lrm1 = (KHE_LIMIT_RESOURCES_MONITOR) m1;
      er1 = KheLimitResourcesMonitorFirstEventResource(lrm1);
      lrm2 = (KHE_LIMIT_RESOURCES_MONITOR) m2;
      er2 = KheLimitResourcesMonitorFirstEventResource(lrm2);
      return KheEventResourceInstanceIndex(er1) -
	KheEventResourceInstanceIndex(er2);

    default:

      return KheMonitorSolnIndex(m1) - KheMonitorSolnIndex(m2);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MonitorsReportHTML(ARRAY_KHE_MONITOR *monitors, bool with_summary,  */
/*    HTML html, HA_ARENA a)                                                 */
/*                                                                           */
/*  Generate tables of reports on monitors.                                  */
/*                                                                           */
/*****************************************************************************/

void MonitorsReportHTML(ARRAY_KHE_MONITOR *monitors, bool with_summary,
  HTML html, HA_ARENA a)
{
  KHE_MONITOR m, prev_m;  int i, points;  KHE_COST cost;  ARRAY_SUMMARY summary;

  HaArraySort(*monitors, &MonitorCmp);
  HaArrayInit(summary, a);
  prev_m = NULL;
  points = 0;
  cost = 0;
  HaArrayForEach(*monitors, m, i)
  {
    /* if switching types, end any old table and begin a new one */
    if( prev_m == NULL || KheMonitorTag(m) != KheMonitorTag(prev_m) )
    {
      /* end old table if any */
      if( prev_m != NULL )
	EndTable(KheMonitorConstraint(prev_m), points, cost, html, &summary, a);

      /* start new table */
      HTMLParagraphBegin(html);
      HTMLTableBegin(html, LightGreen);
      HTMLTableRowBegin(html);
      HTMLTableEntryTextBold(html, KheConstraintTagShowSpaced(
	KheConstraintTag(KheMonitorConstraint(m))));
      /* HTMLTableEntryTextBold(html, "Constraint name"); */
      HTMLTableEntryTextBold(html, "Constraint Id / Point");
      HTMLTableEntryTextBold(html, "Calculation");
      HTMLTableEntryTextBold(html, "Inf.");
      HTMLTableEntryTextBold(html, "Obj.");
      HTMLTableRowEnd(html);
      points = 0;
      cost = 0;
    }

    /* print m and remember it as prev_m */
    MonitorReportHTML(m, /* true, */ &points, &cost, html);
    prev_m = m;
  }
  if( prev_m != NULL )
    EndTable(KheMonitorConstraint(prev_m), points, cost, html, &summary, a);

  /* summary table */
  if( with_summary )
  {
    if( HaArrayCount(summary) > 0 )
      SummaryTableReportHTML(&summary, LightGreen, html);
    else
    {
      HTMLParagraphBegin(html);
      HTMLTextNoBreak(html, "All constraints are satisfied, giving cost 0.");
      HTMLParagraphEnd(html);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void MonitorLowerBoundReportHTML(KHE_MONITOR m, bool applies_to_first,   */
/*    int *points, KHE_COST *cost, HTML html)                                */
/*                                                                           */
/*  Print a table row for m's lower bound, and update *points and *cost.     */
/*  This function assumes that KheMonitorConstraint(m) is non-NULL.          */
/*                                                                           */
/*****************************************************************************/

void MonitorLowerBoundReportHTML(KHE_MONITOR m, bool applies_to_first,
  int *points, KHE_COST *cost, HTML html)
{
  KHE_CONSTRAINT c;

  /* constraint tag or entity applies to */
  c = KheMonitorConstraint(m);
  HTMLTableRowBegin(html);
  HTMLTableEntryText(html, applies_to_first ? KheMonitorAppliesToName(m) :
    KheConstraintTagShowSpaced(KheConstraintTag(c)));

  /* constraint name */
  HTMLTableEntryText(html, KheConstraintName(c));

  /* number of points, and cost */
  *points += 1;
  *cost += KheMonitorLowerBound(m);

  /* infeasibility value */
  HTMLTableEntryCentredBegin(html);
  if( KheConstraintRequired(c) )
    HTMLText(html, "%d", KheHardCost(KheMonitorLowerBound(m)));
  else
    HTMLHSpace(html, 2);
  HTMLTableEntryEnd(html);

  /* objective value */
  HTMLTableEntryCentredBegin(html);
  if( !KheConstraintRequired(c) )
    HTMLText(html, "%d", KheSoftCost(KheMonitorLowerBound(m)));
  else
    HTMLHSpace(html, 2);
  HTMLTableEntryEnd(html);

  HTMLTableRowEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void SolnInvalidParagraph(KHE_SOLN soln, HTML html)                      */
/*                                                                           */
/*  Print a paragraph reporting that soln is invalid, and why.               */
/*                                                                           */
/*****************************************************************************/

void SolnInvalidParagraph(KHE_SOLN soln, HTML html)
{
  char buff[500];  KML_ERROR ke;
  HTMLParagraphBegin(html);
  ke = KheSolnInvalidError(soln);
  snprintf(buff, 500, "This solution is invalid (line %d col %d: %s).",
    KmlErrorLineNum(ke), KmlErrorColNum(ke), KmlErrorString(ke));
  HTMLLiteralText(html, buff);
  HTMLParagraphEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void SolutionHeader(KHE_SOLN soln, HTML html)                            */
/*                                                                           */
/*  Print a header paragraph (or two) for printing soln.                     */
/*                                                                           */
/*****************************************************************************/

static void SolutionHeader(KHE_SOLN soln, HTML html)
{
  KHE_INSTANCE ins;
  ins = KheSolnInstance(soln);
  HTMLParagraphBegin(html);
  HTMLHeadingBegin(html);
  HTMLTextNoBreak(html, "Solution of instance ");
  HTMLLiteralText(html, KheInstanceId(ins));
  if( KheSolnDescription(soln) != NULL )
    HTMLText(html, " (cost % .5f, %s)", KheCostShow(KheSolnCost(soln)),
       KheSolnDescription(soln));
  else
    HTMLText(html, " (cost % .5f)", KheCostShow(KheSolnCost(soln)));
  HTMLHeadingEnd(html);
  HTMLParagraphEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void AssignedTaskWorkloads(KHE_SOLN soln, HTML html)                     */
/*                                                                           */
/*  Return the number of assigned tasks with various workloads.              */
/*                                                                           */
/*****************************************************************************/

/* *** correct but not currently used
static void AssignedTaskWorkloads(KHE_SOLN soln, HTML html)
{
  HA_ARRAY_FLOAT workloads;  HA_ARRAY_INT frequencies;
  HA_ARENA a;  int i, j, freq;  KHE_TASK task;  float workload, wk;

  ** calculate the desired frequencies **
  a = KheSolnArenaBegin(soln);
  HaArrayInit(workloads, a);
  HaArrayInit(frequencies, a);
  for( i = 0;  i < KheSolnTaskCount(soln);  i++ )
  {
    task = KheSolnTask(soln, i);
    if( !KheTaskIsCycleTask(task) && KheTaskAsstResource(task) != NULL )
    {
      workload = KheTaskWorkload(task);
      HaArrayForEach(workloads, wk, j)
	if( wk == workload )
	  break;
      if( j >= HaArrayCount(workloads) )
      {
	HaArrayAddLast(workloads, workload);
	HaArrayAddLast(frequencies, 0);
      }
      HaArray(frequencies, j)++;
    }
  }

  ** print a paragraph holding the histogram, if any **
  if( HaArrayCount(workloads) > 0 )
  {
    HTMLParagraphBegin(html);
    HTMLText(html, "Assigned tasks: ");
    HaArrayForEach(workloads, wk, i)
    {
      freq = HaArray(frequencies, i);
      if( i > 0 )
	HTMLTextNoBreak(html, ", ");
      HTMLTextNoBreak(html, "%d with workload %.1f", freq, wk);
    }
    HTMLText(html, ".");
    HTMLParagraphEnd(html);
  }

  KheSolnArenaEnd(soln, a);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void SolnReportHTML(KHE_SOLN soln, HTML html)                            */
/*                                                                           */
/*  Report soln on html.                                                     */
/*                                                                           */
/*****************************************************************************/

static void SolnReportHTML(KHE_SOLN soln, HTML html, HA_ARENA a)
{
  ARRAY_KHE_MONITOR monitors;  KHE_MONITOR m, prev_m;
  int i, points;  KHE_COST cost;  ARRAY_SUMMARY summary;

  /* header */
  SolutionHeader(soln, html);

  if( KheSolnType(soln) == KHE_SOLN_INVALID_PLACEHOLDER )
    SolnInvalidParagraph(soln, html);
  else
  {
    /* display assigned task workloads */
    /* AssignedTaskWorkloads(soln, html); */

    /* build list of defective monitors, then display it as tables */
    HaArrayInit(monitors, a);
    for( i = 0;  i < KheSolnMonitorCount(soln);  i++ )
    {
      m = KheSolnMonitor(soln, i);
      if( KheMonitorConstraint(m) != NULL && KheMonitorCost(m) > 0 )
	HaArrayAddLast(monitors, m);
    }
    MonitorsReportHTML(&monitors, true, html, a);

    /* constraint tables */
    /* ***
    HaArrayInit(summary);
    prev_m = NULL;
    points = 0;
    cost = 0;
    for( i = 0;  i < KheSolnMonitorCount(soln);  i++ )
    {
      m = KheSolnMonitor(soln, i);
      if( KheMonitorConstraint(m) != NULL && KheMonitorCost(m) > 0 )
      {
	** if switching types, end any old table and begin a new one **
	if( prev_m == NULL || KheMonitorTag(m) != KheMonitorTag(prev_m) )
	{
	  ** end old table if any **
	  if( prev_m != NULL )
	    EndTable(KheMonitorConstraint(prev_m), points, cost, html,&summary);

	  ** start new table **
	  HTMLParagraphBegin(html);
	  HTMLTableBegin(html, LightGreen);
	  HTMLTableRowBegin(html);
	  HTMLTableEntryTextBold(html, KheConstraintTagShowSpaced(
	    KheConstraintTag(KheMonitorConstraint(m))));
	  HTMLTableEntryTextBold(html, "Point of application");
	  HTMLTableEntryTextBold(html, "Calculation");
	  HTMLTableEntryTextBold(html, "Inf.");
	  HTMLTableEntryTextBold(html, "Obj.");
	  HTMLTableRowEnd(html);
	  points = 0;
	  cost = 0;
	}

	** print m and remember it as prev **
	MonitorReportHTML(m, ** true, ** &points, &cost, html);
	prev_m = m;
      }
    }
    if( prev_m != NULL )
      EndTable(KheMonitorConstraint(prev_m), points, cost, html, &summary);

    ** summary table **
    if( HaArrayCount(summary) > 0 )
      SummaryTableReportHTML(&summary, LightGreen, html);
    else
    {
      HTMLParagraphBegin(html);
      HTMLTextNoBreak(html, "All constraints are satisfied by this solution.");
      HTMLParagraphEnd(html);
    }
    *** */

    /* lower bound tables */
    HaArrayInit(summary, a);
    prev_m = NULL;
    points = 0;
    cost = 0;
    for( i = 0;  i < KheSolnMonitorCount(soln);  i++ )
    {
      m = KheSolnMonitor(soln, i);
      if( KheMonitorConstraint(m) != NULL && KheMonitorLowerBound(m) > 0 )
      {
	/* if switching types, end any old table and begin a new one */
	if( prev_m == NULL || KheMonitorTag(m) != KheMonitorTag(prev_m) )
	{
	  /* end old table if any */
	  if( prev_m != NULL )
	    EndLowerBoundTable(KheMonitorConstraint(prev_m), points, cost,
	      html, &summary, a);
	  else
	  {
	    HTMLParagraphBegin(html);
	    HTMLTextBold(html, "Lower bound");
	    HTMLParagraphEnd(html);
	  }

	  /* start new table */
	  HTMLParagraphBegin(html);
	  HTMLTableBegin(html, LightRed);
	  HTMLTableRowBegin(html);
	  HTMLTableEntryTextBold(html, KheConstraintTagShowSpaced(
	    KheConstraintTag(KheMonitorConstraint(m))));
	  HTMLTableEntryTextBold(html, "Constraint name");
	  HTMLTableEntryTextBold(html, "Inf.");
	  HTMLTableEntryTextBold(html, "Obj.");
	  HTMLTableRowEnd(html);
	  points = 0;
	  cost = 0;
	}

	/* print m and remember it as prev */
	MonitorLowerBoundReportHTML(m, true, &points, &cost, html);
	prev_m = m;
      }
    }
    if( prev_m != NULL )
      EndLowerBoundTable(KheMonitorConstraint(prev_m), points, cost,
	html, &summary, a);

    /* summary table */
    if( HaArrayCount(summary) > 0 )
    {
      SummaryTableReportHTML(&summary, LightRed, html);

      HTMLParagraphBegin(html);
      HTMLText(html, "These lower bounds apply only to solutions in which");
      HTMLText(html, "every sub-event is assigned a time and there are no");
      HTMLText(html, "clashes between preassigned event resources.  Not");
      HTMLText(html, "assigning a time and introducing a clash often attract");
      HTMLText(html, "a high penalty, and it may be possible to use that fact");
      HTMLText(html, "to prove that the `Grand total' bound applies to all");
      HTMLText(html, "solutions; but HSEval does not assert that it does.");
      HTMLParagraphEnd(html);
    }
    else
    {
      HTMLParagraphBegin(html);
      HTMLTextBold(html, "Lower bound:");
      HTMLText(html, "HSEval's lower bound for this instance is 0.");
      HTMLParagraphEnd(html);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void SolnGroupReportHTML(KHE_SOLN_GROUP soln_group, HTML html,           */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Evaluate and report soln_group to html.                                  */
/*                                                                           */
/*****************************************************************************/

static void SolnGroupReportHTML(KHE_SOLN_GROUP soln_group, HTML html,
  HA_ARENA a)
{
  int i;  KHE_SOLN soln;  char buff[1000];

  /* start segment */
  sprintf(buff, "Solution Group %s", KheSolnGroupId(soln_group));
  HTMLSegmentBegin(html, KheSolnGroupId(soln_group), buff);

  /* solution group metadata */
  HTMLParagraphBegin(html);
  HTMLText(html, KheSolnGroupMetaDataText(soln_group));
  HTMLParagraphEnd(html);

  /* solutions */
  for( i = 0;  i < KheSolnGroupSolnCount(soln_group);  i++ )
  {
    if( i > 0 )
      HTMLHorizontalRule(html);
    soln = KheSolnGroupSoln(soln_group, i);
    SolnReportHTML(soln, html, a);
  }

  /* end segment */
  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ArchiveReportHTML(KHE_ARCHIVE archive, HTML html)                   */
/*                                                                           */
/*  Evaluate archive and print a report on html.                             */
/*                                                                           */
/*****************************************************************************/

void ArchiveReportHTML(KHE_ARCHIVE archive, HTML html, HA_ARENA_SET as)
{
  KHE_SOLN_GROUP soln_group;  int i;  HA_ARENA a;  char buff[1000];
  if( KheArchiveSolnGroupCount(archive) == 0 )
  {
    HTMLParagraphBegin(html);
    HTMLText(html, "The uploaded XML file contains no solution groups.");
    HTMLParagraphEnd(html);
  }
  else
  {
    a = HaArenaMake(as);

    /* print table of contents if more than one solution group */
    if( KheArchiveSolnGroupCount(archive) >= 2 )
    {
      HTMLParagraphBegin(html);
      for( i = 0;  i < KheArchiveSolnGroupCount(archive);  i++ )
      {
	if( i > 0 )
	  HTMLNewLine(html);
	soln_group = KheArchiveSolnGroup(archive, i);
        sprintf(buff, "Solution Group %s", KheSolnGroupId(soln_group));
	HTMLJumpInternal(html, KheSolnGroupId(soln_group), buff);
      }
      HTMLParagraphEnd(html);
    }
    HTMLHorizontalRule(html);

    /* print solution groups */
    for( i = 0;  i < KheArchiveSolnGroupCount(archive);  i++ )
    {
      soln_group = KheArchiveSolnGroup(archive, i);
      SolnGroupReportHTML(soln_group, html, a);
    }
    HaArenaDelete(a);
  }
}

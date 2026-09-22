
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
/*  FILE:         avail.c                                                    */
/*  MODULE:       AvailabilityReportHTML                                     */
/*                                                                           */
/*****************************************************************************/
#include "externs.h"
#include <float.h>

#define DEBUG1 0	/* SolutionAvailabilityReportHTML */
#define DEBUG2 0	/* EventGroupTimetablesHTML */
#define DEBUG3 0	/* LineDailyDisplay */
#define DEBUG4 0	/* ResourceTypeDailyPlanningTimetableHTML */
#define DEBUG5 0	/* soln_fn */
#define DEBUG6 0	/* LineDisplayTableEntry */
#define DEBUG7 0	/* TaskRunningOnDay */
#define DEBUG8 0	/* TaskEventResourceAsstIsDefective */


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
/*  void HTMLTimeSetShow(KHE_TIME_SET ts, HTML html)                         */
/*                                                                           */
/*  Show ts onto html.                                                       */
/*                                                                           */
/*****************************************************************************/

static void HTMLTimeSetShow(KHE_TIME_SET ts, HTML html)
{
  KHE_TIME ti, tj, prev_tj;  int i, j, groups;
  groups = 0;
  for( i = 0;  i < KheTimeSetTimeCount(ts);  i = j )
  {
    /* find the next group of consecutive times, from i incl to j excl */
    ti = KheTimeSetTime(ts, i);
    prev_tj = ti;
    for( j = i + 1;  j < KheTimeSetTimeCount(ts);  j++ )
    {
      tj = KheTimeSetTime(ts, j);
      if( KheTimeIndex(tj) > KheTimeIndex(prev_tj) + 1 )
	break;
      prev_tj = tj;
    }

    /* print preceding space */
    if( groups > 0 )
    {
      if( groups % 8 == 0 )
	HTMLText(html, ",");
      else
	HTMLTextNoBreak(html, ", ");
    }

    /* print the times from i inclusive to j exclusive */
    if( j - 1 > i )
    {
      tj = KheTimeSetTime(ts, j - 1);
      HTMLTextNoBreak(html, "%s - %s", KheTimeId(ti), KheTimeId(tj));
    }
    else
      HTMLTextNoBreak(html, "%s", KheTimeId(ti));
    groups++;
  }
  HTMLText(html, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void BusyTimesLimitReport(KHE_SOLN soln, KHE_AVAIL_SOLVER as,            */
/*    KHE_RESOURCE r, HTML html, int *total)                                 */
/*                                                                           */
/*  Print a table showing the limit on busy times, and return the total.     */
/*                                                                           */
/*****************************************************************************/

static void BusyTimesLimitReport(KHE_SOLN soln, KHE_AVAIL_SOLVER as,
  KHE_RESOURCE r, HTML html, int *total)
{
  int i, limit, leftovers, count;
  KHE_AVAIL_NODE_TYPE type;  KHE_TIME_SET ts;  KHE_MONITOR m;

  /* begin table */
  HTMLParagraphBegin(html);
  HTMLTableBegin(html, LightGreen);

  /* header row for limits */
  HTMLTableRowBegin(html);
  HTMLTableEntryTextBold(html, "Max busy times");
  HTMLTableEntryTextBold(html, "Times");
  HTMLTableEntryTextBold(html, "Type");
  HTMLTableEntryTextBold(html, "Point of application");
  HTMLTableRowEnd(html);

  /* one row for each avail node */
  *total = 0;
  leftovers = KheInstanceTimeCount(KheSolnInstance(soln));
  count = KheAvailSolverMaxBusyTimesAvailNodeCount(as, r);
  for( i = 0;  i < count;  i++ )
  {
    KheAvailSolverMaxBusyTimesAvailNode(as, r, i, &type, &limit, &ts, &m);
    HTMLTableRowBegin(html);

    /* limit */
    HTMLTableEntryRightBegin(html);
    HTMLText(html, "%d", limit);
    *total += limit;
    HTMLTableEntryEnd(html);

    /* times */
    HTMLTableEntryBegin(html);
    HTMLTimeSetShow(ts, html);
    leftovers -= KheTimeSetTimeCount(ts);
    HTMLTableEntryEnd(html);

    /* type */
    HTMLTableEntryText(html, KheAvailNodeTypeShow(type));

    /* point of application */
    HTMLTableEntryBegin(html);
    if( m != NULL )
      HTMLText(html, KheMonitorId(m));
    else
      HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    HTMLTableRowEnd(html);
  }

  /* one row for each leftover time (if any) */
  if( leftovers > 0 )
  {
    HTMLTableRowBegin(html);

    /* limit */
    HTMLTableEntryRightBegin(html);
    HTMLText(html, "%d", leftovers);
    *total += leftovers;
    HTMLTableEntryEnd(html);

    /* times */
    HTMLTableEntryText(html, "All other times");

    /* type */
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);

    /* point of application */
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    HTMLTableRowEnd(html);
  }

  /* total row (if two or more to add up) */
  if( count + (leftovers > 0 ? 1 : 0) >= 2 )
  {
    HTMLTableRowBegin(html);

    /* limit */
    HTMLTableEntryRightBegin(html);
    HTMLText(html, "Total %d", *total);
    HTMLTableEntryEnd(html);

    /* times */
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);

    /* type */
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);

    /* point of application */
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    HTMLTableRowEnd(html);
  }

  /* end table */
  HTMLTableEnd(html);
  HTMLParagraphEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void BusyTimesTotalReport(KHE_SOLN soln, KHE_RESOURCE r,                 */
/*    HTML html, int *total)                                                 */
/*                                                                           */
/*  Print a table showing the number of busy times, and return the total.    */
/*                                                                           */
/*****************************************************************************/

static void BusyTimesTotalReport(KHE_SOLN soln, KHE_RESOURCE r,
  HTML html, int *total)
{
  int i, count, durn;  KHE_TASK task;  KHE_EVENT_RESOURCE er;  KHE_EVENT e;
  KHE_MEET meet;

  /* begin table */
  HTMLParagraphBegin(html);
  HTMLTableBegin(html, LightGreen);

  /* header row for tasks */
  HTMLTableRowBegin(html);
  HTMLTableEntryTextBold(html, "Busy times");
  HTMLTableEntryTextBold(html, "Event");
  HTMLTableEntryTextBold(html, "Role");
  HTMLTableRowEnd(html);

  /* one row for each task assigned r of duration greater than 0 */
  *total = 0;
  count = KheResourceAssignedTaskCount(soln, r);
  for( i = 0;  i < count;  i++ )
  {
    task = KheResourceAssignedTask(soln, r, i);
    durn = KheTaskDuration(task);
    if( durn > 0 )
    {
      HTMLTableRowBegin(html);

      /* duration */
      HTMLTableEntryRightBegin(html);
      HTMLText(html, "%d", durn);
      *total += durn;
      HTMLTableEntryEnd(html);

      /* event */
      meet = KheTaskMeet(task);
      e = (meet == NULL ? NULL : KheMeetEvent(meet));
      HTMLTableEntryBegin(html);
      if( e != NULL )
	HTMLText(html, KheEventName(e));
      else
	HTMLHSpace(html, 2);
      HTMLTableEntryEnd(html);

      /* role */
      er = KheTaskEventResource(task);
      HTMLTableEntryBegin(html);
      if( er != NULL )
	HTMLText(html, KheEventResourceRole(er));
      else
	HTMLHSpace(html, 2);
      HTMLTableEntryEnd(html);
      HTMLTableRowEnd(html);
    }
  }

  /* total row if needed */
  if( count >= 2 )
  {
    HTMLTableRowBegin(html);

    /* duration */
    HTMLTableEntryRightBegin(html);
    HTMLText(html, "Total %d", *total);
    HTMLTableEntryEnd(html);

    /* event */
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);

    /* role */
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    HTMLTableRowEnd(html);
  }

  /* end table */
  HTMLTableEnd(html);
  HTMLParagraphEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void WorkloadLimitReport(KHE_SOLN soln, KHE_AVAIL_SOLVER as,             */
/*    KHE_RESOURCE r, HTML html, float *total)                               */
/*                                                                           */
/*  Print a table showing the limit on workload, and return the total.       */
/*                                                                           */
/*****************************************************************************/

static void WorkloadLimitReport(KHE_SOLN soln, KHE_AVAIL_SOLVER as,
  KHE_RESOURCE r, HTML html, float *total)
{
  int i, count;  KHE_AVAIL_NODE_TYPE type;  KHE_TIME_SET ts;
  KHE_MONITOR m;  float limit;

  /* begin table */
  HTMLParagraphBegin(html);
  HTMLTableBegin(html, LightGreen);

  /* header row for limits */
  HTMLTableRowBegin(html);
  HTMLTableEntryTextBold(html, "Max workload");
  HTMLTableEntryTextBold(html, "Times");
  HTMLTableEntryTextBold(html, "Type");
  HTMLTableEntryTextBold(html, "Point of application");
  HTMLTableRowEnd(html);

  /* one row for each avail node */
  *total = 0.0;
  count = KheAvailSolverMaxWorkloadAvailNodeCount(as, r);
  for( i = 0;  i < count;  i++ )
  {
    KheAvailSolverMaxWorkloadAvailNode(as, r, i, &type, &limit, &ts, &m);
    HTMLTableRowBegin(html);

    /* limit */
    HTMLTableEntryRightBegin(html);
    HTMLText(html, "%.1f", limit);
    *total += limit;
    HTMLTableEntryEnd(html);

    /* times */
    HTMLTableEntryBegin(html);
    HTMLTimeSetShow(ts, html);
    HTMLTableEntryEnd(html);

    /* type */
    HTMLTableEntryText(html, KheAvailNodeTypeShow(type));

    /* point of application */
    HTMLTableEntryBegin(html);
    if( m != NULL )
      HTMLText(html, KheMonitorId(m));
    else
      HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    HTMLTableRowEnd(html);
  }

  /* total row (if two or more to add up) */
  if( count >= 2 )
  {
    HTMLTableRowBegin(html);

    /* limit */
    HTMLTableEntryRightBegin(html);
    HTMLText(html, "Total %.1f", *total);
    HTMLTableEntryEnd(html);

    /* times */
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);

    /* type */
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);

    /* point of application */
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    HTMLTableRowEnd(html);
  }

  /* end table */
  HTMLTableEnd(html);
  HTMLParagraphEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void WorkloadTotalReport(KHE_SOLN soln, KHE_RESOURCE r,                  */
/*    HTML html, float *total)                                               */
/*                                                                           */
/*  Print a table showing the amount of workload, and return the total.      */
/*                                                                           */
/*****************************************************************************/

static void WorkloadTotalReport(KHE_SOLN soln, KHE_RESOURCE r,
  HTML html, float *total)
{
  int i, count;  KHE_TASK task;  KHE_EVENT_RESOURCE er;  KHE_EVENT e;
  KHE_MEET meet;  float workload;

  /* begin table */
  HTMLParagraphBegin(html);
  HTMLTableBegin(html, LightGreen);

  /* header row for tasks */
  HTMLTableRowBegin(html);
  HTMLTableEntryTextBold(html, "Workload");
  HTMLTableEntryTextBold(html, "Event");
  HTMLTableEntryTextBold(html, "Role");
  HTMLTableRowEnd(html);

  /* one row for each task assigned r of duration greater than 0 */
  *total = 0.0;
  count = KheResourceAssignedTaskCount(soln, r);
  for( i = 0;  i < count;  i++ )
  {
    task = KheResourceAssignedTask(soln, r, i);
    workload = KheTaskWorkload(task);
    if( workload > 0.0 )
    {
      HTMLTableRowBegin(html);

      /* workload */
      HTMLTableEntryRightBegin(html);
      HTMLText(html, "%.1f", workload);
      *total += workload;
      HTMLTableEntryEnd(html);

      /* event */
      meet = KheTaskMeet(task);
      e = (meet == NULL ? NULL : KheMeetEvent(meet));
      HTMLTableEntryBegin(html);
      if( e != NULL )
	HTMLText(html, KheEventName(e));
      else
	HTMLHSpace(html, 2);
      HTMLTableEntryEnd(html);

      /* role */
      er = KheTaskEventResource(task);
      HTMLTableEntryBegin(html);
      if( er != NULL )
	HTMLText(html, KheEventResourceRole(er));
      else
	HTMLHSpace(html, 2);
      HTMLTableEntryEnd(html);
      HTMLTableRowEnd(html);
    }
  }

  /* total row if needed */
  if( count >= 2 )
  {
    HTMLTableRowBegin(html);

    /* workload */
    HTMLTableEntryRightBegin(html);
    HTMLText(html, "Total %.1f", *total);
    HTMLTableEntryEnd(html);

    /* event */
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);

    /* role */
    HTMLTableEntryBegin(html);
    HTMLHSpace(html, 2);
    HTMLTableEntryEnd(html);
    HTMLTableRowEnd(html);
  }

  /* end table */
  HTMLTableEnd(html);
  HTMLParagraphEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ResourceAvailabilityReportHTML(KHE_SOLN soln, KHE_RESOURCE r,       */
/*    HTML html)                                                             */
/*                                                                           */
/*  Print an availability report for resource r in soln onto HTML.           */
/*                                                                           */
/*****************************************************************************/

static void ResourceAvailabilityReportHTML(KHE_SOLN soln, KHE_RESOURCE r,
  HTML html)
{
  KHE_AVAIL_SOLVER as;  int limit, done_count;
  int max_busy_times, busy_times;  float lim, workload, max_workload;

  /* set the avail solver */
  as = KheSolnAvailSolver(soln);
  /* KheAvailSolverSetResource(as, r); */

  /* display the resource name as a heading */
  HTMLParagraphBegin(html);
  HTMLTextBold(html, "Availability of %s", KheResourceName(r));
  HTMLParagraphEnd(html);

  /* print an optional max busy times report */
  done_count = 0;
  if( KheAvailSolverMaxBusyTimes(as, r, &limit) )
  {
    BusyTimesLimitReport(soln, as, r, html, &max_busy_times);
    BusyTimesTotalReport(soln, r, html, &busy_times);
    HTMLParagraphBegin(html);
    HTMLText(html, "Available busy times are max busy times minus busy times = "
      "%d - %d = %d.", max_busy_times, busy_times, max_busy_times - busy_times);
    HTMLParagraphEnd(html);
    done_count++;
  }

  /* print an optional max workload report */
  if( KheAvailSolverMaxWorkload(as, r, &lim) )
  {
    WorkloadLimitReport(soln, as, r, html, &max_workload);
    WorkloadTotalReport(soln, r, html, &workload);
    HTMLParagraphBegin(html);
    HTMLText(html, "Available workload is max workload minus workload = "
      "%.1f - %.1f = %.1f.", max_workload, workload, max_workload - workload);
    HTMLParagraphEnd(html);
    done_count++;
  }

  /* print a stub message if nothing else was printed */
  if( done_count == 0 )
  {
    HTMLParagraphBegin(html);
    HTMLText(html, "No limits on availability for %s.", KheResourceId(r));
    HTMLParagraphEnd(html);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceHasAvailability(KHE_SOLN soln, KHE_RESOURCE r)           */
/*                                                                           */
/*  Return true if there is something to say about the availability of       */
/*  resource r.                                                              */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceHasAvailability(KHE_SOLN soln, KHE_RESOURCE r)
{
  KHE_AVAIL_SOLVER as;  int limit;  float lim;
  as = KheSolnAvailSolver(soln);
  /* KheAvailSolverSetResource(as, r); */
  return KheAvailSolverMaxBusyTimes(as, r, &limit) ||
    KheAvailSolverMaxWorkload(as, r, &lim);
}


/*****************************************************************************/
/*                                                                           */
/*  bool ResourceTypeHasAvailability(KHE_SOLN soln, KHE_RESOURCE_TYPE rt)    */
/*                                                                           */
/*  Return true if there is something to say about the availability of       */
/*  resources of type rt.                                                    */
/*                                                                           */
/*****************************************************************************/

static bool ResourceTypeHasAvailability(KHE_SOLN soln, KHE_RESOURCE_TYPE rt)
{
  int i;  KHE_RESOURCE r;

  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    if( KheResourceHasAvailability(soln, r) )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void ResourceTypeAvailabilityReportHTML(KHE_SOLN soln,                   */
/*    KHE_RESOURCE_TYPE rt, HTML html)                                       */
/*                                                                           */
/*  Print an availability report for each resource of type rt.               */
/*                                                                           */
/*****************************************************************************/

static void ResourceTypeAvailabilityReportHTML(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, HTML html)
{
  int i;  KHE_RESOURCE r;

  /* heading */
  HTMLParagraphBegin(html);
  HTMLHeadingBegin(html);
  HTMLText(html, "Availability of %s", KheResourceTypeName(rt));
  HTMLHeadingEnd(html);
  HTMLParagraphEnd(html);

  if( ResourceTypeHasAvailability(soln, rt) )
  {
    /* print one report for each resource */
    for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
    {
      r = KheResourceTypeResource(rt, i);
      ResourceAvailabilityReportHTML(soln, r, html);
    }
  }
  else
  {
    /* say that nothing is needed */
    HTMLParagraphBegin(html);
    HTMLText(html, "There are no limits on the availability of resources "
     "of type %s.", KheResourceTypeId(rt));
    HTMLParagraphEnd(html);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void SolutionAvailabilityReportHTML(KHE_SOLN soln, HTML html)            */
/*                                                                           */
/*  Print one availability report for each resource of soln's instance.      */
/*                                                                           */
/*****************************************************************************/

static void SolutionAvailabilityReportHTML(KHE_SOLN soln, HTML html)
{
  int i;  KHE_INSTANCE ins;  KHE_RESOURCE_TYPE rt;

  /* header */
  ins = KheSolnInstance(soln);
  if( DEBUG1 || DEBUG3 )
    fprintf(stderr, "[ SolutionAvailabilityReportHTML(soln to %s, html)\n",
      KheInstanceId(ins));
  SolutionHeader(soln, html);

  /* one segment for each resource type */
  if( KheSolnType(soln) == KHE_SOLN_INVALID_PLACEHOLDER )
    SolnInvalidParagraph(soln, html);
  else
  {
    for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
    {
      rt = KheInstanceResourceType(ins, i);
      ResourceTypeAvailabilityReportHTML(soln, rt, html);
    }
  }
  if( DEBUG1 || DEBUG3 )
    fprintf(stderr, "] SolutionAvailabilityReportHTML returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void IntroParagraphs(KHE_ARCHIVE archive, HTML html)                     */
/*                                                                           */
/*  Print some introductory paragraphs.                                      */
/*                                                                           */
/*****************************************************************************/

static void IntroParagraphs(KHE_ARCHIVE archive, HTML html)
{
  HTMLParagraphBegin(html);
  HTMLText(html, "For each solution in the uploaded XML archive, this page");
  HTMLText(html, "contains one availability report for each resource of the");
  HTMLText(html, "solution's instance, showing how that resource's");
  HTMLText(html, "availability (as given in planning timetables) is");
  HTMLText(html, "calculated.");
  HTMLParagraphEnd(html);

  HTMLParagraphBegin(html);
  HTMLText(html, "Availability is maximum possible load minus actual load.");
  HTMLText(html, "The result could be negative, proving that the resource is");
  HTMLText(html, "overloaded.  Here `load' could refer to either busy times");
  HTMLText(html, "or workload.  For more information about how availability");
  HTMLText(html, "is calculated, see the `Resource availability' section of");
  HTMLText(html, "the KHE manual.");
  HTMLParagraphEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void AvailabilityReportHTML(COMMAND c, HA_ARENA_SET as)                  */
/*                                                                           */
/*  Print an availability report for each resource of each solution of       */
/*  the archive.                                                             */
/*                                                                           */
/*****************************************************************************/

void AvailabilityReportHTML(COMMAND c, HA_ARENA_SET as)
{
  KHE_ARCHIVE archive;  char buff[200];  HTML html;  int i, j;
  KHE_SOLN_GROUP soln_group;  KHE_SOLN soln;
  char *contributor, *date, *description, *publication, *remarks;

  if( DEBUG3 )
    fprintf(stderr, "[ AvailabilityReportHTML(cgi, as)\n");

  archive = ReadAndVerifyArchive(c, true, false, KHE_SOLN_ORDINARY, as);
  snprintf(buff, 100, "%s Availability Report",
    KheArchiveId(archive) != NULL && strlen(KheArchiveId(archive)) < 70 ?
    KheArchiveId(archive) : "HSEval");
  html = PageBegin(buff);
  HTMLBigHeading(html, buff);

  if( KheArchiveSolnGroupCount(archive) == 0 )
  {
    HTMLParagraphBegin(html);
    HTMLText(html, "The uploaded XML file contains no solution groups.");
    HTMLParagraphEnd(html);
  }
  else
  {
    /* print introductory stuff */
    IntroParagraphs(archive, html);

    for( i = 0;  i < KheArchiveSolnGroupCount(archive);  i++ )
    {
      /* print soln group metadata */
      soln_group = KheArchiveSolnGroup(archive, i);
      HTMLHorizontalRule(html);
      KheSolnGroupMetaData(soln_group, &contributor, &date, &description,
	&publication, &remarks);
      HTMLHeading(html, description);
      HTMLParagraphBegin(html);
      HTMLText(html, KheSolnGroupMetaDataText(soln_group));
      HTMLParagraphEnd(html);

      /* print solutions */
      for( j = 0;  j < KheSolnGroupSolnCount(soln_group);  j++ )
      {
	soln = KheSolnGroupSoln(soln_group, j);
	SolutionAvailabilityReportHTML(soln, html);
	if( KheSolnType(soln) == KHE_SOLN_ORDINARY )
	  KheSolnTypeReduce(soln, KHE_SOLN_BASIC_PLACEHOLDER, NULL);
	  /* KheSolnReduceToPlaceholder(soln, false); */
      }
    }
  }

  /* print a back jump link */
  HTMLParagraphBegin(html);
  HTMLText(html, "Return to the ");
  HTMLJumpFront(html);
  HTMLText(html, ".");
  HTMLParagraphEnd(html);

  /* and quit */
  PageEnd(html);
  if( DEBUG3 )
    fprintf(stderr, "] AvailabilityReportHTML\n");
}

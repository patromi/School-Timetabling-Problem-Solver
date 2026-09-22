
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
/*  FILE:         timetable.c                                                */
/*  MODULE:       Timetables                                                 */
/*                                                                           */
/*****************************************************************************/
#include "externs.h"
#include <float.h>
#include "space.h"

#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1 0	/* SolutionPlanningTimetablesHTML */
#define DEBUG3 0	/* LineDailyDisplay */


/*****************************************************************************/
/*                                                                           */
/*  AVAIL_INFO - information about available times and workload              */
/*                                                                           */
/*****************************************************************************/

typedef struct avail_info_rec {
  bool		show_avail_times;
  bool		show_avail_workload;
  int		total_positive_avail_times;
  int		total_negative_avail_times;
  float		total_positive_avail_workload;
  float		total_negative_avail_workload;
} *AVAIL_INFO;


/*****************************************************************************/
/*                                                                           */
/*  bool SpaceCheck(SPACE space, HTML html)                                  */
/*                                                                           */
/*  Check that space is OK.  If it is, return true.  Otherwise print an      */
/*  error message and return false.                                          */
/*                                                                           */
/*****************************************************************************/

static bool SpaceCheck(SPACE space, HTML html)
{
  int i, count;  KHE_TIME_GROUP tg1, tg2, tg3;
  count = SpaceErrorCount(space);
  if( count == 0 )
  {
    /* no problems, return true */
    return true;
  }
  else if( count == 1 )
  {
    /* one problem, say so */
    HTMLParagraphBegin(html);
    HTMLText(html, "HSEval cannot print this timetable, because it has");
    HTMLText(html, "found a problem with its Weeks and Days:");
    HTMLParagraphEnd(html);
  }
  else if( count <= 3 )
  {
    /* two or three problems, say so */
    HTMLParagraphBegin(html);
    HTMLText(html, "HSEval cannot print this timetable, because it has");
    HTMLText(html, "found %d problems with its Weeks and Days:", count);
    HTMLParagraphEnd(html);
  }
  else
  {
    /* four or more problems, say so and reduce count */
    HTMLParagraphBegin(html);
    HTMLText(html, "HSEval cannot print this timetable, because it has");
    HTMLText(html, "found %d problems with its Weeks and Days.", count);
    HTMLText(html, "Here are the first 3 of these problems:");
    HTMLParagraphEnd(html);
    count = 3;
  }

  /* print the first count problems and return false */
  for( i = 0;  i < count;  i++ )
  {
    switch( SpaceError(space, i, &tg1, &tg2, &tg3) )
    {
      case SPACE_ERROR_DAY_IS_EMPTY:

	HTMLParagraphBegin(html);
	HTMLText(html, "Day time group %s is empty.", KheTimeGroupId(tg1));
	HTMLParagraphEnd(html);
	break;

      case SPACE_ERROR_DAYS_NOT_DISJOINT:

	HTMLParagraphBegin(html);
	HTMLText(html, "Day time groups %s and %s are not disjoint.",
	  KheTimeGroupId(tg1), KheTimeGroupId(tg2));
	HTMLParagraphEnd(html);
	break;

      case SPACE_ERROR_DAYS_DO_NOT_COVER_CYCLE:

	HTMLParagraphBegin(html);
	HTMLText(html, "There are Day time groups but they do not cover");
	HTMLText(html, "the cycle.");
	HTMLParagraphEnd(html);
	break;

      case SPACE_ERROR_WEEK_IS_EMPTY:

	HTMLParagraphBegin(html);
	HTMLText(html, "Week time group %s is empty.", KheTimeGroupId(tg1));
	HTMLParagraphEnd(html);
	break;

      case SPACE_ERROR_WEEKS_NOT_DISJOINT:

	HTMLParagraphBegin(html);
	HTMLText(html, "Week time groups %s and %s are not disjoint.",
	  KheTimeGroupId(tg1), KheTimeGroupId(tg2));
	HTMLParagraphEnd(html);
	break;

      case SPACE_ERROR_WEEKS_DO_NOT_COVER_CYCLE:

	HTMLParagraphBegin(html);
	HTMLText(html, "There are Week time groups but they do not cover");
	HTMLText(html, "the cycle.");
	HTMLParagraphEnd(html);
	break;

      case SPACE_ERROR_DAY_INTERSECTS_TWO_WEEKS:

	HTMLParagraphBegin(html);
	HTMLText(html, "Day time group %s intersects", KheTimeGroupId(tg1));
	HTMLText(html, "with two Week time groups: %s", KheTimeGroupId(tg2));
	HTMLText(html, "and %s.", KheTimeGroupId(tg3));
	HTMLParagraphEnd(html);
	break;

      default:

	HnAbort("SpaceCheck internal error");
	break;
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void MajorHeadingResourceGroupFn(KHE_RESOURCE_GROUP rg, KHE_SOLN soln,   */
/*    int total_durn, float total_workload, void *impl, HTML html,HA_ARENA a)*/
/*                                                                           */
/*  Callback function for printing a major heading for each resource group.  */
/*                                                                           */
/*****************************************************************************/

static void MajorHeadingResourceGroupFn(KHE_RESOURCE_GROUP rg, KHE_SOLN soln,
  int total_durn, float total_workload, void *impl, HTML html, HA_ARENA a)
{
  KHE_RESOURCE r;  bool started;  int i, count, avail_times;
  float avail_workload;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;

  /* if rg is empty, print "Unassigned" and return */
  if( KheResourceGroupResourceCount(rg) == 0 )
  {
    HTMLParagraphBegin(html);
    HTMLTextBold(html, "Unassigned");
    HTMLParagraphEnd(html);
    return;
  }

  /* rg should contain just one resource; call it r */
  HnAssert(KheResourceGroupResourceCount(rg) == 1,
    "MajorHeadingResourceGroupFn internal error");
  r = KheResourceGroupResource(rg, 0);

  /* make sure the resource timetable monitor is attached */
  rtm = KheResourceTimetableMonitor(soln, r);
  if( !KheMonitorAttachedToSoln((KHE_MONITOR) rtm) )
    KheMonitorAttachToSoln((KHE_MONITOR) rtm);

  /* heading showing resource name, resource groups, and availability */
  HTMLParagraphBegin(html);
  HTMLTextBold(html, KheResourceName(r));
  KheResourceAvailableBusyTimes(soln, r, &avail_times);
  KheResourceAvailableWorkload(soln, r, &avail_workload);
  if( KheResourceResourceGroupCount(r) > 0 ||
      avail_times < INT_MAX || avail_workload < FLT_MAX )
  {
    HTMLTextNoBreak(html, "(");
    count = KheResourceResourceGroupCount(r);
    if( count > 5 )  count = 5;
    started = false;
    for( i = 0;  i < count;  i++ )
    {
      rg = KheResourceResourceGroup(r, i);
      if( started )
	HTMLTextNoBreak(html, ", ");
      HTMLTextNoBreak(html, KheResourceGroupName(rg));
      started = true;
    }
    if( count < KheResourceResourceGroupCount(r) )
      HTMLTextNoBreak(html, ", ...");
    if( avail_times < INT_MAX )
    {
      if( started )
	HTMLTextNoBreak(html, ", ");
      HTMLTextNoBreak(html, "Avail times %d", avail_times);
      started = true;
    }
    if( avail_workload < FLT_MAX )
    {
      if( started )
	HTMLTextNoBreak(html, ", ");
      HTMLTextNoBreak(html, "Avail workload %.1f", avail_workload);
      started = true;
    }
    HTMLText(html, ")");
  }
  HTMLParagraphEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void ResourceTypeInitAvail(KHE_RESOURCE_TYPE rt, KHE_SOLN soln,          */
/*    AVAIL_INFO ai)                                                         */
/*                                                                           */
/*  Initialize ai.                                                           */
/*                                                                           */
/*****************************************************************************/

static void ResourceTypeInitAvail(KHE_RESOURCE_TYPE rt, KHE_SOLN soln,
  AVAIL_INFO ai)
{
  float avail_workload;  int avail_times, i;  KHE_RESOURCE r;
  ai->show_avail_times = ai->show_avail_workload = false;
  ai->total_positive_avail_times = 0;
  ai->total_negative_avail_times = 0;
  ai->total_positive_avail_workload = 0.0;
  ai->total_negative_avail_workload = 0.0;
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    KheResourceAvailableBusyTimes(soln, r, &avail_times);
    if( avail_times < INT_MAX )
      ai->show_avail_times = true;
    KheResourceAvailableWorkload(soln, r, &avail_workload);
    if( avail_workload < FLT_MAX )
      ai->show_avail_workload = true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void DisplayAvailTimesAndWorkload(int avail_times,                       */
/*    float avail_workload, HTML html)                                       */
/*                                                                           */
/*  Display avail_times (if less than INT_MAX) and avail_workload (if        */
/*  less than FLT_MAX) onto html.                                            */
/*                                                                           */
/*  Also, if accumulate is true, accumulate totals.                          */
/*                                                                           */
/*****************************************************************************/

static void DisplayTimesAndWorkload(AVAIL_INFO ai, int avail_times,
  float avail_workload, bool accumulate, HTML html)
{
  bool show_avail_times, show_workload;
  show_avail_times = (ai->show_avail_times && avail_times < INT_MAX);
  show_workload = (ai->show_avail_workload && avail_workload < FLT_MAX);

  /* display */
  if( show_avail_times )
  {
    if( show_workload )
      HTMLText(html, "%d; %.1f", avail_times, avail_workload);
    else
      HTMLText(html, "%d", avail_times);
  }
  else
  {
    if( show_workload )
      HTMLText(html, "%.1f", avail_workload);
    else
      HTMLHSpace(html, 2);
  }

  /* optionally accumulate totals */
  if( accumulate )
  {
    if( show_avail_times )
    {
      if( avail_times > 0 )
	ai->total_positive_avail_times += avail_times;
      else if( avail_times < 0 )
	ai->total_negative_avail_times += avail_times;
    }
    if( show_workload )
    {
      if( avail_workload > 0.0 )
	ai->total_positive_avail_workload += avail_workload;
      else if( avail_workload < 0.0 )
	ai->total_negative_avail_workload += avail_workload;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void DisplayTotalTimesAndWorkload(AVAIL_INFO ai, HTML html)              */
/*                                                                           */
/*  Display a paragraph showing total available times and workload.          */
/*                                                                           */
/*****************************************************************************/

static void DisplayTotalTimesAndWorkload(AVAIL_INFO ai, HTML html)
{
  if( ai->show_avail_times || ai->show_avail_workload )
  {
    if( ai->show_avail_times )
      HTMLText(html, "Total available times: positive %d, negative %d.\n",
        ai->total_positive_avail_times, ai->total_negative_avail_times);
    if( ai->show_avail_workload )
      HTMLText(html,"Total available workload: positive %.1f, negative %.1f.\n",
        ai->total_positive_avail_workload, ai->total_negative_avail_workload);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void AvailResourceGroupFn(KHE_RESOURCE_GROUP rg, KHE_SOLN soln,          */
/*    int total_durn, float total_workload, HTML html, HA_ARENA a)           */
/*                                                                           */
/*  Avail resource group function.  Always prints something.                 */
/*                                                                           */
/*****************************************************************************/

static void AvailResourceGroupFn(KHE_RESOURCE_GROUP rg, KHE_SOLN soln,
  int total_durn, float total_workload, void *impl, HTML html, HA_ARENA a)
{
  KHE_RESOURCE r;  AVAIL_INFO ai;
  ai = (AVAIL_INFO) impl;
  if( KheResourceGroupResourceCount(rg) > 0 )
  {
    /* rg should contain just one resource; call it r */
    HnAssert(KheResourceGroupResourceCount(rg) == 1,
      "AvailResourceGroupFn internal error");
    r = KheResourceGroupResource(rg, 0);
    KheResourceAvailableBusyTimes(soln, r, &total_durn);
    KheResourceAvailableWorkload(soln, r, &total_workload);
    DisplayTimesAndWorkload(ai, total_durn, total_workload, true, html);
  }
  else
    DisplayTimesAndWorkload(ai, total_durn, total_workload, false, html);
}


/*****************************************************************************/
/*                                                                           */
/*  void AfterResourceGroupFn(KHE_RESOURCE_GROUP rg, KHE_SOLN soln,          */
/*    int total_durn, float total_workload, void *impl, HTML html,HA_ARENA a)*/
/*                                                                           */
/*  Callback function for printing after each resource group.                */
/*                                                                           */
/*****************************************************************************/

static void AfterResourceGroupFn(KHE_RESOURCE_GROUP rg, KHE_SOLN soln,
  int total_durn, float total_workload, void *impl, HTML html, HA_ARENA a)
{
  ARRAY_KHE_MONITOR monitors;  int i, j, pos, points;  KHE_EVENT_RESOURCE er;
  KHE_RESOURCE r;  KHE_MONITOR m;  KHE_COST cost;  bool header_printed;

  /* if rg has no resources, print nothing */
  if( KheResourceGroupResourceCount(rg) == 0 )
    return;

  /* now rg should contain just one resource; call it r */
  HnAssert(KheResourceGroupResourceCount(rg) == 1,
    "AfterResourceGroupFn internal error");
  r = KheResourceGroupResource(rg, 0);

  /* gather constraint violations and print a table of them */
  HaArrayInit(monitors, a);
  for( i = 0;  i < KheResourceAssignedTaskCount(soln, r);  i++ )
  {
    er = KheTaskEventResource(KheResourceAssignedTask(soln, r, i));
    if( er != NULL )
      for( j = 0;  j < KheSolnEventResourceMonitorCount(soln, er);  j++ )
      {
	m = KheSolnEventResourceMonitor(soln, er, j);
	if( KheMonitorTag(m) != KHE_LIMIT_RESOURCES_MONITOR_TAG &&
	    KheMonitorCost(m) > 0 && !HaArrayContains(monitors, m, &pos) )
	  HaArrayAddLast(monitors, m);
      }
  }
  for( i = 0;  i < KheSolnResourceMonitorCount(soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(soln, r, i);
    if( KheMonitorCost(m) > 0 && !HaArrayContains(monitors, m, &pos) )
      HaArrayAddLast(monitors, m);
  }
  MonitorsReportHTML(&monitors, false, html, a);

  /* lower bound table */
  header_printed = false;
  for( i = 0;  i < KheSolnResourceMonitorCount(soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(soln, r, i);
    if( KheMonitorLowerBound(m) > 0 )
    {
      if( !header_printed )
      {
	HTMLParagraphBegin(html);
	HTMLTableBegin(html, LightRed);
	HTMLTableRowBegin(html);
	HTMLTableEntryTextBold(html, "Lower bounds");
	HTMLTableEntryTextBold(html, "Constraint name");
	HTMLTableEntryTextBold(html, "Inf.");
	HTMLTableEntryTextBold(html, "Obj.");
	HTMLTableRowEnd(html);
	header_printed = true;
      }
      MonitorLowerBoundReportHTML(m, false, &points, &cost, html);
    }
  }
  if( header_printed )
  {
    HTMLTableEnd(html);
    HTMLParagraphEnd(html);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int EventDecreasingDurationCmp(const void *t1, const void *t2)           */
/*                                                                           */
/*  Comparison function for sorting an array of events by decreasing         */
/*  duration.                                                                */
/*                                                                           */
/*****************************************************************************/

static int EventDecreasingDurationCmp(const void *t1, const void *t2)
{
  KHE_EVENT e1 = * (KHE_EVENT *) t1;
  KHE_EVENT e2 = * (KHE_EVENT *) t2;
  if( KheEventDuration(e2) != KheEventDuration(e1) )
    return KheEventDuration(e2) - KheEventDuration(e1);
  else
    return KheEventIndex(e1) - KheEventIndex(e2);
}


/*****************************************************************************/
/*                                                                           */
/*  void ResourceBusyTimesAndEvents(KHE_SOLN soln, KHE_RESOURCE r,           */
/*    int *r_busy_times, ARRAY_KHE_EVENT *events)                            */
/*                                                                           */
/*  Set *r_busy_times to the number of times that r is busy in soln, and     */
/*  set *events to the events that it is assigned to, wholly or partially.   */
/*                                                                           */
/*****************************************************************************/

static void ResourceBusyTimesAndEvents(KHE_SOLN soln, KHE_RESOURCE r,
  int *r_busy_times, ARRAY_KHE_EVENT *events)
{
  int i, pos;  KHE_TASK task;  KHE_EVENT e;
  HaArrayClear(*events);
  *r_busy_times = 0;
  for( i = 0;  i < KheResourceAssignedTaskCount(soln, r);  i++ )
  {
    task = KheResourceAssignedTask(soln, r, i);
    *r_busy_times += KheTaskDuration(task);
    e = KheMeetEvent(KheTaskMeet(task));
    if( e != NULL && !HaArrayContains(*events, e, &pos) )
      HaArrayAddLast(*events, e);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void DisplayTotalItalic(int total_italic, HTML html)                     */
/*                                                                           */
/*  Display the total number of italic entries.                              */
/*                                                                           */
/*****************************************************************************/

static void DisplayTotalItalic(int total_italic, HTML html)
{
  HTMLText(html, "Total italic entries: %d.\n", total_italic);
}


/*****************************************************************************/
/*                                                                           */
/*  void BuildPermutedTimes(ARRAY_KHE_TIME *permuted_times,                  */
/*    KHE_SOLN soln, HA_ARENA a)                                             */
/*                                                                           */
/*  Build the permuted times array.                                          */
/*                                                                           */
/*****************************************************************************/

static void BuildPermutedTimes(ARRAY_KHE_TIME *permuted_times,
 KHE_SOLN soln, HA_ARENA a)
{
  KHE_RESOURCE_TYPE rt, best_rt;
  ARRAY_KHE_EVENT events;  KHE_EVENT e;  KHE_RESOURCE r, best_r;
  int i, j, k, r_busy_times, best_busy_times, best_ecount, pos;
  KHE_TIME start_time, time;  KHE_INSTANCE ins;  KHE_MEET meet;
  MODEL model;  MODEL_RESOURCE_TYPE classes_mrt, mrt;

  /* find a resource type called Class if there is one */
  ins = KheSolnInstance(soln);
  model = ModelBuild(KheInstanceModel(ins), a);
  classes_mrt = ModelRetrieve(model, "Classes");
  best_rt = NULL;
  for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
  {
    rt = KheInstanceResourceType(ins, i);
    mrt = ModelRetrieve(model, KheResourceTypeName(rt));
    if( mrt == classes_mrt )
    {
      best_rt = rt;
      break;
    }
  }

  /* if there is a resource type called Class, find its busiest resource */
  best_r = NULL;
  HaArrayInit(events, a);
  if( best_rt != NULL )
    for( i = 0;  i < KheResourceTypeResourceCount(best_rt);  i++ )
    {
      r = KheResourceTypeResource(best_rt, i);
      ResourceBusyTimesAndEvents(soln, r, &r_busy_times, &events);
      if( best_r == NULL || r_busy_times > best_busy_times ||
       (r_busy_times==best_busy_times && HaArrayCount(events) < best_ecount) )
      {
	best_r = r;
	best_busy_times = r_busy_times;
	best_ecount = HaArrayCount(events);
      }
    }

  /* if no best_r yet, search entire instance */
  if( best_r == NULL )
    for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
    {
      r = KheInstanceResource(ins, i);
      ResourceBusyTimesAndEvents(soln, r, &r_busy_times, &events);
      if( best_r == NULL || r_busy_times > best_busy_times ||
       (r_busy_times==best_busy_times && HaArrayCount(events) < best_ecount) )
      {
	best_r = r;
	best_busy_times = r_busy_times;
	best_ecount = HaArrayCount(events);
      }
    }

  /* if have best_r, add times based on best_r's events' times */
  if( best_r != NULL )
  {
    if( DEBUG1 )
      fprintf(stderr, "  best_r is %s\n", KheResourceId(best_r));
    ResourceBusyTimesAndEvents(soln, best_r, &r_busy_times, &events);
    HaArraySort(events, &EventDecreasingDurationCmp);

    /* build permuted times by following the busiest resource's timetable */
    HaArrayForEach(events, e, i)
    {
      if( DEBUG1 )
	fprintf(stderr, "  %s attends event %s\n", KheResourceId(best_r),
	  KheEventId(e));
      for( j = 0;  j < KheEventMeetCount(soln, e);  j++ )
      {
	meet = KheEventMeet(soln, e, j);
	start_time = KheMeetAsstTime(meet);
	if( start_time != NULL )
	{
	  for( k = 0;  k < KheMeetDuration(meet);  k++ )
	  {
	    time = KheTimeNeighbour(start_time, k);
	    if( !HaArrayContains(*permuted_times, time, &pos) )
	    {
	      HaArrayAddLast(*permuted_times, time);
	      if( DEBUG1 )
	      {
		fprintf(stderr, "  %s attends meet ", KheResourceId(best_r));
		KheMeetDebug(meet, 1, -1, stderr);
		fprintf(stderr, " at time %s\n", KheTimeId(time));
	      }
	    }
	  }
	}
      }
    }
  }

  /* add leftover times, not already in permuted_times */
  for( i = 0;  i < KheInstanceTimeCount(ins);  i++ )
  {
    time = KheInstanceTime(ins, i);
    if( !HaArrayContains(*permuted_times, time, &pos) )
    {
      HaArrayAddLast(*permuted_times, time);
      if( DEBUG1 )
	fprintf(stderr, "  extra time %s\n", KheTimeId(time));
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void ResourceTypeTimetables(KHE_RESOURCE_TYPE rt, KHE_SOLN soln,         */
/*    bool compress_days, bool planning, bool highlight_splits,              */
/*    bool with_event_groups, char *constraints_str, HTML html, HA_ARENA a)  */
/*                                                                           */
/*  Print a timetable or timetables for the resources of rt.                 */
/*                                                                           */
/*****************************************************************************/

static void ResourceTypeTimetables(KHE_RESOURCE_TYPE rt, KHE_SOLN soln,
  bool compress_days, bool planning, bool highlight_splits,
  bool with_event_groups, char *constraints_str, HTML html, HA_ARENA a)
{
  ARRAY_KHE_TIME permuted_times;  SPACE space;  KHE_INSTANCE ins;
  SPACE_TIME_DIM std;  int i, total_italic;  KHE_TIME time;
  bool allow_spanning;  struct avail_info_rec avail_rec;

  /* heading (omit if just one resource type) */
  ins = KheSolnInstance(soln);
  if( KheInstanceResourceTypeCount(ins) >= 2 )
  {
    HTMLParagraphBegin(html);
    HTMLHeadingBegin(html);
    HTMLTextNoBreak(html, "Resource type ");
    HTMLText(html, KheResourceTypeName(rt));
    HTMLHeadingEnd(html);
    HTMLParagraphEnd(html);
  }

  /* make a space and check it */
  allow_spanning = (KheInstanceModel(ins) != KHE_MODEL_EMPLOYEE_SCHEDULE);
  space = SpaceMake(soln, allow_spanning, White, false, HTML_ROMAN);
  if( !SpaceCheck(space, html) )
  {
    SpaceDelete(space);
    return;
  }

  if( planning )
  {
    /******************************************************/
    /*                                                    */
    /*  planning timetable                                */
    /*                                                    */
    /******************************************************/

    /* outer resource type dimension, with or without an Avail column */
    ResourceTypeInitAvail(rt, soln, &avail_rec);
    if( avail_rec.show_avail_times || avail_rec.show_avail_workload )
      SpaceAddResourceTypeDim(space, rt, NULL,
        &AvailResourceGroupFn, &avail_rec, NULL);
    else
      SpaceAddResourceTypeDim(space, rt, NULL, NULL, NULL, NULL);

    if( compress_days && SpaceHasDays(space) )
    {
      /* planning timetable with one column for each day */
      SpaceAddDaysOfCycleDim(space);
    }
    else
    {
      /* planning timetable with one column for each time */
      std = SpaceAddTimeDim(space);
      HaArrayInit(permuted_times, a);
      BuildPermutedTimes(&permuted_times, soln, a);
      HaArrayForEach(permuted_times, time, i)
	SpaceTimeDimAddTimeGroup(std, KheTimeSingletonTimeGroup(time),
	  KheTimeId(time));
    }
  }
  else
  {
    /******************************************************/
    /*                                                    */
    /*  individual timetables                             */
    /*                                                    */
    /******************************************************/

    if( !SpaceHasDays(space) )
    {
      HTMLParagraphBegin(html);
      HTMLText(html, "HSEval cannot print this timetable, because the");
      HTMLText(html, "instance does not have Days time groups.");
      HTMLParagraphEnd(html);
      SpaceDelete(space);
      return;
    }

    /* outer resource type dimension */
    SpaceAddResourceTypeDim(space, rt, &MajorHeadingResourceGroupFn,
      NULL, NULL, &AfterResourceGroupFn);

    if( compress_days )
    {
      if( SpaceHasWeeks(space) )
      {
	/* one table for each resource; rows are weeks, cols are days of week */
	SpaceAddWeeksOfCycleDim(space);
	SpaceAddDaysOfWeekDim(space);
      }
      else
      {
	/* one table for each resource; dummy row, cols are days of cycle */
        SpaceAddDummyDim(space);
	SpaceAddDaysOfCycleDim(space);
      }
    }
    else
    {
      if( SpaceHasWeeks(space) )
      {
	/* a sequence of tables for each resource, one per week */
	SpaceAddWeeksOfCycleDim(space);
	SpaceAddTimesOfDayDim(space);
	SpaceAddDaysOfWeekDim(space);
      }
      else
      {
	/* one table for each resource: rows are times of day, cols are days */
	SpaceAddTimesOfDayDim(space);
	SpaceAddDaysOfCycleDim(space);
      }
    }
  }

  /* add cell background colours; the first applicable one applies */
  if( highlight_splits )
  {
    /* highlighting split assignments */
    SpaceCellFormat(space, SPACE_CELL_TIME_GROUP_EMPTY,        Black);
    SpaceCellFormat(space, SPACE_CELL_TASKS_NONE,              LightGreen);
    SpaceCellFormat(space, SPACE_CELL_TASKS_TWO_OR_MORE,       NULL);
    SpaceCellFormat(space, SPACE_CELL_HAS_DEFECTIVE_TASK,      NULL);
  }
  else
  {
    /* regular */
    SpaceCellFormat(space, SPACE_CELL_TIME_GROUP_EMPTY,            Black);
    SpaceCellFormat(space, SPACE_CELL_RESOURCE_GROUP_UNAVAIL_HARD, Red);
    SpaceCellFormat(space, SPACE_CELL_RESOURCE_GROUP_UNAVAIL_SOFT, LightRed);
    SpaceCellFormat(space, SPACE_CELL_RESOURCE_GROUP_MUST_BE_BUSY_HARD, Blue);
    SpaceCellFormat(space, SPACE_CELL_RESOURCE_GROUP_MUST_BE_BUSY_SOFT,
      LightBlue);
    SpaceCellFormat(space, SPACE_CELL_UNREQUIRED_ASSIGNED,         LightYellow);
    SpaceCellFormat(space, SPACE_CELL_RESOURCE_GROUP_UNAVAIL_PART, LightPink);
    if( constraints_str != NULL )
    {
      SpaceCellFormat2(space, SPACE_CELL_CONSTRAINT_VIOLATED,
	constraints_str, Blue);
      SpaceCellFormat2(space, SPACE_CELL_CONSTRAINT_SLACK,
	constraints_str, LightBlue);
    }
    SpaceCellFormat(space, SPACE_CELL_TASKS_NONE,                  LightGreen);
    SpaceCellFormat(space, SPACE_CELL_TASKS_ONE_OR_MORE,           NULL);
  }

  /* add entry fonts; the first applicable one applies, else don't show */
  SpaceTaskFormat(space, SPACE_TASK_REQUIRED_UNASSIGNED,true, HTML_BOLD_ITALIC);
  SpaceTaskFormat(space, SPACE_TASK_DEFECTIVE,		true, HTML_BOLD_ITALIC);
  SpaceTaskFormat(space, SPACE_TASK_UNREQUIRED_ASSIGNED,true, HTML_ITALIC);
  SpaceTaskFormat(space, SPACE_TASK_REQUIRED_ASSIGNED,	true, HTML_ROMAN);

  /* add tasks, display, and delete */
  SpaceAddAllTasks(space);
  SpaceDisplay(space, &total_italic, html);
  SpaceDelete(space);

  /* print total available times and workload */
  if( planning )
  {
    HTMLParagraphBegin(html);
    DisplayTotalTimesAndWorkload(&avail_rec, html);
    DisplayTotalItalic(total_italic, html);
    HTMLParagraphEnd(html);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void SolutionTimetables(KHE_SOLN soln, bool planning,                    */
/*    bool highlight_splits, bool with_event_groups, char *constraints_str,  */
/*    HTML html, HA_ARENA_SET as)                                            */
/*                                                                           */
/*  Print a timetable for soln with these options.                           */
/*                                                                           */
/*****************************************************************************/

void SolutionTimetables(KHE_SOLN soln, bool planning,
  bool highlight_splits, bool with_event_groups, char *constraints_str,
  HTML html, HA_ARENA_SET as)
{
  HA_ARENA a;  /* KHE_INSTANCE ins; */  /* ARRAY_WEEK weeks; */
  KHE_RESOURCE_TYPE rt;  KHE_INSTANCE ins;  int i;  bool compress_days;
  a = HaArenaMake(as);
  /* ins = KheSolnInstance(soln); */

  /* print solution header */
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

  if( KheSolnType(soln) == KHE_SOLN_INVALID_PLACEHOLDER )
    SolnInvalidParagraph(soln, html);
  else
  {
    compress_days = (KheInstanceModel(ins) == KHE_MODEL_EMPLOYEE_SCHEDULE);
    for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
    {
      rt = KheInstanceResourceType(ins, i);
      if( KheResourceTypeResourceCount(rt) > 0 )
	ResourceTypeTimetables(rt, soln, compress_days, planning,
	  highlight_splits, with_event_groups, constraints_str, html, a);
    }
  }
  HaArenaDelete(a);
}


/*****************************************************************************/
/*                                                                           */
/*  void SolutionGroupTimetables(KHE_SOLN_GROUP soln_group, bool planning,   */
/*    bool highlight_splits, bool with_event_groups, char *constraints_str,  */
/*    HTML html, HA_ARENA_SET as)                                            */
/*                                                                           */
/*  Print timetables for the solutions of soln_group, preceded by a header.  */
/*                                                                           */
/*  Horizontal rules are printed between solutions, but not at the start     */
/*  or end of this whole print.                                              */
/*                                                                           */
/*****************************************************************************/

void SolutionGroupTimetables(KHE_SOLN_GROUP soln_group, bool planning,
  bool highlight_splits, bool with_event_groups, char *constraints_str,
  HTML html, HA_ARENA_SET as)
{
  char buff[1000];  int i;  KHE_SOLN soln;

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
    SolutionTimetables(soln, planning, highlight_splits, with_event_groups,
      constraints_str, html, as);
    if( KheSolnType(soln) == KHE_SOLN_ORDINARY )
      KheSolnTypeReduce(soln, KHE_SOLN_BASIC_PLACEHOLDER, NULL);
  }

  /* end segment */
  HTMLSegmentEnd(html);
}


/*****************************************************************************/
/*                                                                           */
/*  void Timetables(COMMAND c, bool planning, bool highlight_splits,         */
/*    bool with_event_groups, char *constraints_str, HA_ARENA_SET as)        */
/*                                                                           */
/*  Print some timetables.                                                   */
/*                                                                           */
/*  Horizontal rules are printed between solution groups, and at the end.    */
/*                                                                           */
/*****************************************************************************/

void Timetables(COMMAND c, bool planning, bool highlight_splits,
  bool with_event_groups, char *constraints_str, HA_ARENA_SET as)
{
  KHE_ARCHIVE archive;  char buff[1000];  HTML html;  int i;
  KHE_SOLN_GROUP soln_group;

  if( DEBUG3 )
    fprintf(stderr, "[ Timetables(c, plannning %s, highlight_splits %s, "
      "with_event_groups %s, constraints_str \"%s\")\n",
      bool_show(planning), bool_show(highlight_splits),
      bool_show(with_event_groups), constraints_str);

  /* get the archive */
  archive = ReadAndVerifyArchive(c, true, planning, KHE_SOLN_ORDINARY, as);

  /* page header */
  snprintf(buff, 1000, "%s %s Timetables",
    KheArchiveId(archive) != NULL && strlen(KheArchiveId(archive)) < 70 ?
    KheArchiveId(archive) : "HSEval", planning ? "Planning" : "Solution");
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
    /* intro paragraphs */
    if( planning )
    {
      HTMLParagraphBegin(html);
      HTMLText(html, "For each solution in the uploaded XML archive, this");
      HTMLText(html, "page contains one planning timetable for each resource");
      HTMLText(html, "type of the corresponding instance.  This is a large");
      HTMLText(html, "table with one row for each resource, and one column");
      HTMLText(html, "for each time or each day.  Where a resource type is");
      HTMLText(html, "divisible into completely independent parts (parts");
      HTMLText(html, "which could have been separate resource types), each");
      HTMLText(html, "part is given its own table.");
      HTMLParagraphEnd(html);

      HTMLParagraphBegin(html);
      HTMLText(html, "The Avail column, where present, shows an integer");
      HTMLText(html, "number of available times, determined by resource");
      HTMLText(html, "constraints, or a decimal amount of available workload,");
      HTMLText(html, "determined by limit workload constraints, or both.");
      HTMLText(html, "Only non-trivial values are shown.  These are heuristic");
      HTMLText(html, "estimates, and are occasionally higher than the true");
      HTMLText(html, "values, but never lower.  A negative value proves that");
      HTMLText(html, "its resource is overloaded.  In Unassigned rows the");
      HTMLText(html, "Avail column shows the unassigned times or workload.");
      HTMLText(html, "Below the table, Total available times shows the sum of");
      HTMLText(html, "the positive values in the Avail column (not counting");
      HTMLText(html, "Unassigned rows), and the sum of the negative values");
      HTMLText(html, "in the Avail column.  For more information about how");
      HTMLText(html, "availability is calculated, see the Availability");
      HTMLText(html, "Report, reached from the front page of HSEval.");
      HTMLParagraphEnd(html);
    }
    else
    {
      HTMLParagraphBegin(html);
      HTMLText(html, "For each solution in the uploaded XML archive, this");
      HTMLText(html, "page contains a timetable for each resource of the");
      HTMLText(html, "corresponding instance.  If any of the constraints");
      HTMLText(html, "that apply to that resource are violated, a table of");
      HTMLText(html, "those violations is shown below its timetable.");
      HTMLParagraphEnd(html);

      if( with_event_groups )
      {
	HTMLParagraphBegin(html);
	HTMLText(html, "Similarly, a timetable is shown for each event group");
	HTMLText(html, "of the instance to which at least one constraint");
	HTMLText(html, "applies; and if any of those constraints are");
	HTMLText(html, "violated, a table of those violations is shown.");
	HTMLParagraphEnd(html);
      }
    }

    HTMLParagraphBegin(html);
    HTMLText(html, "The background colour of each timetable cell is red if");
    HTMLText(html, "the resource is unavailable for some reason (possibly");
    HTMLText(html, "history) for all of the times that the cell represents");
    HTMLText(html, "(dark red for hard constraints, light red for soft ones),");
    HTMLText(html, "or else blue if history demands that the resource be busy");
    HTMLText(html, "during at least one of those times (dark blue for hard");
    HTMLText(html, "constraints, light blue for");
    HTMLText(html, "soft ones), or else light yellow if the cell contains a");
    HTMLText(html, "task printed in italics (see below), or else light pink");
    HTMLText(html, "if the resource is unavailable for some of those times");
    HTMLText(html, "but not all (hard or soft), or else light green if the");
    HTMLText(html, "cell contains no events, or else the colour of the first");
    HTMLText(html, "event depicted (white if that event has no colour).");
    if( constraints_str != NULL && constraints_str[0] != '\0' )
    {
      HTMLText(html, "As requested, constraints whose name or Id includes "
	"\"%s\"", constraints_str);
      HTMLText(html, "are highlighted:  the background colour of their");
      HTMLText(html, "cells is blue where one of them is violated, and");
      HTMLText(html, "for cluster busy times constraints it is light blue");
      HTMLText(html, "where one of them is strictly below a maximum limit.");
      /* ***
      HTMLText(html, "light blue when it is not violated unless that would");
      HTMLText(html, "colour the whole table.");
      *** */
    }
    HTMLParagraphEnd(html);

    HTMLParagraphBegin(html);
    HTMLText(html, "Each line of text in each cell represents one event");
    HTMLText(html, "resource, by giving the name of the enclosing event.");
    HTMLText(html, "Each line appears in bold italic font if the event");
    HTMLText(html, "resource it names is subject to an event resource");
    HTMLText(html, "constraint which is violated (the event resource may");
    HTMLText(html, "be unassigned when an assignment is wanted, assigned an");
    HTMLText(html, "unpreferred resource, part of a split assignment, etc.),");
    HTMLText(html, "or else in italic when unassigning the event resource");
    HTMLText(html, "would not violate an event resource constraint or");
    HTMLText(html, "preassignment, or else in roman.  The name may be");
    HTMLText(html, "abbreviated if it exceeds the column width, or begins");
    HTMLText(html, "with a time name which is redundant in the context of");
    HTMLText(html, "the printed table.");
    if( planning )
    {
      HTMLText(html, "Below the table, Total Italic is the number of entries");
      HTMLText(html,"printed in italic, signifying that unassigning would not");
      HTMLText(html, "violate an event resource constraint or preassignment.");
    }
    HTMLParagraphEnd(html);

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
      SolutionGroupTimetables(soln_group, planning, highlight_splits,
	with_event_groups, constraints_str, html, as);
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
    fprintf(stderr, "] Timetables\n");
}

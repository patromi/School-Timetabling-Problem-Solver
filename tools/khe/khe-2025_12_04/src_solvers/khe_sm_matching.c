
/*****************************************************************************/
/*                                                                           */
/*  THE KHE HIGH SCHOOL TIMETABLING ENGINE                                   */
/*  COPYRIGHT (C) 2010 Jeffrey H. Kingston                                   */
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
/*  FILE:         khe_sm_matching.c                                          */
/*  DESCRIPTION:  KheMatchingBegin() and KheMatchingEnd()                    */
/*                                                                           */
/*****************************************************************************/
#include <limits.h>
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG10 0

#define bool_show(x) ((x) ? "true" : "false")


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_CLASS - a set of events known to run simultaneously            */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_EVENT) ARRAY_KHE_EVENT;

typedef struct khe_event_class_rec {
  KHE_TIME			preassigned_time;	/* optional preasst  */
  ARRAY_KHE_EVENT		events;			/* simultaneous      */
} *KHE_EVENT_CLASS;

typedef HA_ARRAY(KHE_EVENT_CLASS) ARRAY_KHE_EVENT_CLASS;


/*****************************************************************************/
/*                                                                           */
/*  KHE_WORKLOAD_REQUIREMENT - one workload requirement                      */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_workload_requirement_rec *KHE_WORKLOAD_REQUIREMENT;
typedef HA_ARRAY(KHE_WORKLOAD_REQUIREMENT) ARRAY_KHE_WORKLOAD_REQUIREMENT;

struct khe_workload_requirement_rec {
  KHE_RESOURCE		resource;		/* the resource              */
  int			count;			/* number required           */
  KHE_TIME_GROUP	time_group;		/* time group required       */
  KHE_MONITOR		originating_monitor;	/* originating monitor       */
  ARRAY_KHE_WORKLOAD_REQUIREMENT children;	/* child requirements        */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_SOLVER - a solver for building the matching                 */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_MONITOR) ARRAY_KHE_MONITOR;

typedef struct khe_matching_solver_rec {
  HA_ARENA				arena;
  KHE_SOLN				soln;
  bool					integrated;
  KHE_COST				min_weight;
  ARRAY_KHE_EVENT_CLASS			event_classes_by_event;
  ARRAY_KHE_EVENT_CLASS			event_classes_by_time;
  ARRAY_KHE_WORKLOAD_REQUIREMENT	workload_requirements;
  ARRAY_KHE_WORKLOAD_REQUIREMENT	forest_roots;
  ARRAY_KHE_MONITOR			monitors;
  HA_ARRAY_BOOL				resource_type_has_avoid_clashes;
} *KHE_MATCHING_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MATCHING_SOLVER"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_SOLVER KheMatchingSolverMake(KHE_SOLN soln,                 */
/*    bool integrated, HA_ARENA a)                                           */
/*                                                                           */
/*  Make a matching solver with these attributes in arena a.                 */
/*                                                                           */
/*****************************************************************************/

static KHE_MATCHING_SOLVER KheMatchingSolverMake(KHE_SOLN soln,
  bool integrated, HA_ARENA a)
{
  KHE_MATCHING_SOLVER res;
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->integrated = integrated;
  res->min_weight = KheCost(1, 0);  /* could be changed */
  HaArrayInit(res->event_classes_by_event, a);
  HaArrayInit(res->event_classes_by_time, a);
  HaArrayInit(res->workload_requirements, a);
  HaArrayInit(res->forest_roots, a);
  HaArrayInit(res->monitors, a);
  HaArrayInit(res->resource_type_has_avoid_clashes, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_EVENT_CLASS"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_CLASS KheEventClassMake(KHE_EVENT e, HA_ARENA a)               */
/*                                                                           */
/*  Make a new event class containing only e.                                */
/*                                                                           */
/*****************************************************************************/

static KHE_EVENT_CLASS KheEventClassMake(KHE_EVENT e, HA_ARENA a)
{
  KHE_EVENT_CLASS res;
  HnAssert(e != NULL, "KheEventClassMake internal error");
  HaMake(res, a);
  res->preassigned_time = KheEventPreassignedTime(e);
  HaArrayInit(res->events, a);
  HaArrayAddLast(res->events, e);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_CLASS KheEventToEventClass(KHE_EVENT e, KHE_MATCHING_SOLVER ms)*/
/*                                                                           */
/*  Return the event class containing e.                                     */
/*                                                                           */
/*****************************************************************************/

static KHE_EVENT_CLASS KheEventToEventClass(KHE_EVENT e, KHE_MATCHING_SOLVER ms)
{
  return HaArray(ms->event_classes_by_event, KheEventIndex(e));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventClassMerge(KHE_EVENT_CLASS ec1, KHE_EVENT_CLASS ec2,        */
/*    ARRAY_KHE_EVENT_CLASS *classes_by_event)                               */
/*                                                                           */
/*  Merge ec1 and ec2 (assumed distinct) making ec2 unused.                  */
/*                                                                           */
/*****************************************************************************/

static void KheEventClassMerge(KHE_EVENT_CLASS ec1, KHE_EVENT_CLASS ec2,
  KHE_MATCHING_SOLVER ms)
{
  int i;  KHE_EVENT e;
  HaArrayAppend(ec1->events, ec2->events, i);
  HaArrayForEach(ec2->events, e, i)
    HaArrayPut(ms->event_classes_by_event, KheEventIndex(e), ec1);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventClassCmp(const void *t1, const void *t2)                     */
/*                                                                           */
/*  Comparison function for sorting an array of event classes into           */
/*  increasing order of the index of the first event.                        */
/*                                                                           */
/*****************************************************************************/
static void KheEventClassDebug(KHE_EVENT_CLASS ec, int indent, FILE *fp);

static int KheEventClassCmp(const void *t1, const void *t2)
{
  KHE_EVENT_CLASS ec1 = * (KHE_EVENT_CLASS *) t1;
  KHE_EVENT_CLASS ec2 = * (KHE_EVENT_CLASS *) t2;
  KHE_EVENT e1 = HaArrayFirst(ec1->events);
  KHE_EVENT e2 = HaArrayFirst(ec2->events);
  return KheEventIndex(e1) - KheEventIndex(e2);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventClassContainsSpecialWorkload(KHE_EVENT_CLASS ec)            */
/*                                                                           */
/*  Return true if ec contains at least one event with a special workload.   */
/*                                                                           */
/*****************************************************************************/

static bool KheEventClassContainsSpecialWorkload(KHE_EVENT_CLASS ec)
{
  int i, j;  KHE_EVENT e;  KHE_EVENT_RESOURCE er;
  HaArrayForEach(ec->events, e, i)
    for( j = 0;  j < KheEventResourceCount(e);  j++ )
    {
      er = KheEventResource(e, j);
      if( KheEventResourceWorkload(er) != KheEventDuration(e) )
	return true;
    }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventClassDebug(KHE_EVENT_CLASS ec, int indent, FILE *fp)        */
/*                                                                           */
/*  Debug print of ec (possibly NULL) onto fp with the given indent.         */
/*                                                                           */
/*****************************************************************************/

static void KheEventClassDebug(KHE_EVENT_CLASS ec, int indent, FILE *fp)
{
  int i;  KHE_EVENT e;
  if( ec == NULL )
    fprintf(stderr, "%*sNULL\n", indent, "");
  else
  {
    fprintf(stderr, "%*s[ Event class", indent, "");
    if( ec->preassigned_time != NULL )
      fprintf(stderr, " (%s)", KheTimeId(ec->preassigned_time) == NULL ?
	"-" : KheTimeId(ec->preassigned_time));
    HaArrayForEach(ec->events, e, i)
      fprintf(stderr, " %s", KheEventId(e) != NULL ? KheEventId(e) : "-");
    fprintf(stderr, " ]\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventClassesDebug(ARRAY_KHE_EVENT_CLASS *classes,                */
/*    char *header, int indent, FILE *fp)                                    */
/*                                                                           */
/*  Debug print of an array of event classes onto fp at the given indent.    */
/*                                                                           */
/*****************************************************************************/

static void KheEventClassesDebug(ARRAY_KHE_EVENT_CLASS *classes,
  char *header, int indent, FILE *fp)
{
  KHE_EVENT_CLASS ec;  int i;
  fprintf(stderr, "%*s[ %s (%d classes)\n", indent, "",
    header != NULL ? header : "", HaArrayCount(*classes));
  HaArrayForEach(*classes, ec, i)
    KheEventClassDebug(ec, indent + 2, fp);
  fprintf(stderr, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventClassesBuild(KHE_MATCHING_SOLVER ms)                        */
/*                                                                           */
/*  Set ms->event_classes_by_event to those classes of the events of ins     */
/*  that contain special workloads.                                          */
/*                                                                           */
/*  Implementation note.  Within this function, ms->event_classes_by_event   */
/*  is used to hold classes indexed by event.  At the end, clssses without   */
/*  special workloads are removed, and the array is uniqueified so that      */
/*  each class appears just once.                                            */
/*                                                                           */
/*****************************************************************************/

static void KheEventClassesBuild(KHE_MATCHING_SOLVER ms)
{
  KHE_EVENT_CLASS ec, prev_ec;
  int i, j, k, index;  KHE_EVENT e, prev_e;  KHE_EVENT_GROUP eg;
  KHE_CONSTRAINT c;  KHE_LINK_EVENTS_CONSTRAINT lec;  KHE_INSTANCE ins;

  if( DEBUG1 )
    fprintf(stderr, "[ KheEventClassesBuild()\n");

  /* one class holding the events preassigned each time, as required */
  HaArrayClear(ms->event_classes_by_time);
  ins = KheSolnInstance(ms->soln);
  HaArrayFill(ms->event_classes_by_time, KheInstanceTimeCount(ins), NULL);
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    if( KheEventPreassignedTime(e) != NULL )
    {
      index = KheTimeIndex(KheEventPreassignedTime(e));
      ec = HaArray(ms->event_classes_by_time, index);
      if( ec == NULL )
	HaArrayPut(ms->event_classes_by_time, index,
	  KheEventClassMake(e, ms->arena));
      else
	HaArrayAddLast(ec->events, e);
    }
  }

  /* one class for each event not yet classed; build event_classes_by_event */
  HaArrayClear(ms->event_classes_by_event);
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    if( KheEventPreassignedTime(e) != NULL )
    {
      /* e is already in a class, get that class */
      index = KheTimeIndex(KheEventPreassignedTime(e));
      ec = HaArray(ms->event_classes_by_time, index);
    }
    else
    {
      /* make a fresh class for e */
      ec = KheEventClassMake(e, ms->arena);
    }
    HaArrayAddLast(ms->event_classes_by_event, ec);
  }
  HaArrayClear(ms->event_classes_by_time);

  /* merge classes as indicated by link events constraints */
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintTag(c) == KHE_LINK_EVENTS_CONSTRAINT_TAG &&
	KheConstraintCombinedWeight(c) >= ms->min_weight )
	/* KheConstraintRequired(c) && KheConstraintWeight(c) > 0 */
    {
      lec = (KHE_LINK_EVENTS_CONSTRAINT) c;
      for( j = 0;  j < KheLinkEventsConstraintEventGroupCount(lec);  j++ )
      {
	eg = KheLinkEventsConstraintEventGroup(lec, j);
	for( k = 1;  k < KheEventGroupEventCount(eg);  k++ )
	{
	  prev_e = KheEventGroupEvent(eg, k - 1);
	  prev_ec = KheEventToEventClass(prev_e, ms);
	  e = KheEventGroupEvent(eg, k);
	  ec = KheEventToEventClass(e, ms);
	  if( ec != prev_ec )
            KheEventClassMerge(prev_ec, ec, ms);
	}
      }
    }
  }

  /* uniqueify classes */
  HaArraySortUnique(ms->event_classes_by_event, &KheEventClassCmp);
  if( DEBUG1 )
    KheEventClassesDebug(&ms->event_classes_by_event,
      "Classes after uniquefying", 0, stderr);
  /* ***
  if( DEBUG1 )
    KheEventClassesDebug(&ms->event_classes_by_event,
      "Classes before uniquefying", 0, stderr);
  HaArraySort(ms->event_classes_by_event, &KheEventClassCmp);
  if( DEBUG1 )
    KheEventClassesDebug(&ms->event_classes_by_event,
      "Classes after sorting", 0, stderr);
  HaArraySortUnique(ms->event_classes_by_event, &KheEventClassCmp);
  if( DEBUG1 )
    KheEventClassesDebug(&ms->event_classes_by_event,
      "Classes after uniquefying", 0, stderr);
  *** */

  /* remove classes that have no special workloads */
  HaArrayForEach(ms->event_classes_by_event, ec, i)
    if( !KheEventClassContainsSpecialWorkload(ec) )
    {
      HaArrayDeleteAndPlug(ms->event_classes_by_event, i);
      i--;
    }
  if( DEBUG1 )
    KheEventClassesDebug(&ms->event_classes_by_event,
      "Final classes", 0, stderr);
  if( DEBUG1 )
    fprintf(stderr, "] KheEventClassesBuild returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventClassResourceAdjustment(KHE_EVENT_CLASS ec, KHE_RESOURCE r)  */
/*                                                                           */
/*  Return the adjustment needed to r's overall workload limit to take       */
/*  account of event class ec.  Precisely, this is as follows:               */
/*                                                                           */
/*    * If r must be assigned to at least one event resource of an event     */
/*      of ec, then return the sum of d(e) - w(e) over all those events      */
/*      e of ec that r must be assigned to.                                  */
/*                                                                           */
/*    * Otherwise, if r may be assigned to at least one event resource of    */
/*      an event of ec, then return the maximum value of d(e) - w(e) over    */
/*      all those events e of ec that r may be assigned to.                  */
/*                                                                           */
/*    * Otherwise (if r may not be assigned to any event of ec) return 0.    */
/*                                                                           */
/*  Implementation note.  This function tests assignability of r to each     */
/*  event resource separately.  This is correct if r cannot be assigned to   */
/*  any event, but to distinguish "must" from "may" it would be better to    */
/*  match all the event resources against the resources of the instance      */
/*  with and without r.  However, that would be slower and much more work,   */
/*  probably not worth the trouble given that the return value is much the   */
/*  same in the two cases.                                                   */
/*                                                                           */
/*****************************************************************************/

static int KheEventClassResourceAdjustment(KHE_EVENT_CLASS ec, KHE_RESOURCE r)
{
  KHE_EVENT e;  bool must, may;  int i, j, must_sum, may_max;
  KHE_RESOURCE_GROUP domain;  KHE_EVENT_RESOURCE er;
  must_sum = may_max = 0;  must = may = false;
  HaArrayForEach(ec->events, e, i)
    for( j = 0;  j < KheEventResourceCount(e);  j++ )
    {
      er = KheEventResource(e, j);
      domain = KheEventResourceHardDomain(er);
      if( KheResourceGroupContains(domain, r) )
      {
	if( KheResourceGroupResourceCount(domain) == 1 )
	{
	  /* r must be assigned to er */
	  must_sum += (KheEventDuration(e) - KheEventResourceWorkload(er));
	  must = true;
	}
	else
	{
	  /* r may be assigned to er */
	  if( KheEventDuration(e) - KheEventResourceWorkload(er) > may_max )
	    may_max = KheEventDuration(e) - KheEventResourceWorkload(er);
	  may = true;
	}
      }
    }
  if( DEBUG3 )
  {
    fprintf(stderr, "    KheEventClassResourceAdjustment(ec, %s): %s %d for ",
      KheResourceId(r) == NULL ? "-" : KheResourceId(r),
    must ? "must" : may ? "may" : "not", must ? must_sum : may ? may_max : 0);
    KheEventClassDebug(ec, 0, stderr);
  }
  return must ? must_sum : may ? may_max : 0;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "ordinary demand monitors"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceHasAvoidClashesMonitors(KHE_SOLN soln, KHE_RESOURCE r,   */
/*    KHE_COST min_weight)                                                   */
/*                                                                           */
/*  Return true if r has attached avoid clashes monitors whose weight sums   */
/*  to at least min_weight.                                                  */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceHasAvoidClashesMonitors(KHE_SOLN soln, KHE_RESOURCE r,
  KHE_COST min_weight)
{
  KHE_COST weight;  int i;  KHE_CONSTRAINT c;  KHE_MONITOR m;
  weight = KheCost(0, 0);
  for( i = 0;  i < KheSolnResourceMonitorCount(soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(soln, r, i);
    if( KheMonitorTag(m) == KHE_AVOID_CLASHES_MONITOR_TAG &&
	KheMonitorAttachedToSoln(m) )
    {
      c = KheMonitorConstraint(m);  /* NB m is avoid clashes monitor */
      weight += KheConstraintCombinedWeight(c);
      if( weight >= min_weight )
	return true;
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTypeHasAvoidClashesMonitors(KHE_SOLN soln,               */
/*    KHE_RESOURCE_TYPE rt, KHE_COST min_weight)                             */
/*                                                                           */
/*  Return true if every resource of type rt has attached avoid clashes      */
/*  monitors whose weight sums to at least min_weight.                       */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceTypeHasAvoidClashesMonitors(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_COST min_weight)
{
  KHE_RESOURCE r;  int i;
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    if( !KheResourceHasAvoidClashesMonitors(soln, r, min_weight) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetAllowsDemandMonitors(KHE_MATCHING_SOLVER ms, KHE_MEET meet)  */
/*                                                                           */
/*  Return true if meet allows demand monitors, because it is not a cycle    */
/*  meet, and it has either a preassigned time, or attached assign time      */
/*  monitors whose weight sums to at least ms->min_weight.                   */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetAllowsDemandMonitors(KHE_MATCHING_SOLVER ms, KHE_MEET meet)
{
  KHE_COST weight;  int i;  KHE_CONSTRAINT c;  KHE_MONITOR m;
  KHE_EVENT e;
  if( !KheMeetIsCycleMeet(meet) )
  {
    e = KheMeetEvent(meet);
    if( e != NULL )
    {
      if( KheEventPreassignedTime(e) != NULL )
	return true;
      weight = KheCost(0, 0);
      for( i = 0;  i < KheSolnEventMonitorCount(ms->soln, e);  i++ )
      {
	m = KheSolnEventMonitor(ms->soln, e, i);
	if( KheMonitorTag(m) == KHE_ASSIGN_TIME_MONITOR_TAG &&
	    KheMonitorAttachedToSoln(m) )
	{
	  c = KheMonitorConstraint(m);  /* NB m is assign time monitor */
	  weight += KheConstraintCombinedWeight(c);
	  if( weight >= ms->min_weight )
	    return true;
	}
      }
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskResourceTypeHasAvoidClashes(KHE_TASK task,                   */
/*    KHE_MATCHING_SOLVER ms)                                                */
/*                                                                           */
/*  Return true if task's resource type has avoid clashes monitors,          */
/*  assuming that this has been worked out previously and stored in ms.      */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskResourceTypeHasAvoidClashes(KHE_TASK task,
  KHE_MATCHING_SOLVER ms)
{
  KHE_RESOURCE_TYPE rt;
  rt = KheTaskResourceType(task);
  return HaArray(ms->resource_type_has_avoid_clashes, KheResourceTypeIndex(rt));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskAllowsDemandMonitors(KHE_MATCHING_SOLVER ms, KHE_TASK task,  */
/*    KHE_MONITOR *orig_m)                                                   */
/*                                                                           */
/*  Return true if task allows demand monitors, because its resource type    */
/*  has avoid clashes constraint, and it has either a preassigned resource,  */
/*  or attached assign resource monitors whose weight sums to at least       */
/*  min_weight.  In the last case, set *m to any one of those monitors.      */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskAllowsDemandMonitors(KHE_MATCHING_SOLVER ms, KHE_TASK task,
  KHE_MONITOR *orig_m)
{
  KHE_COST weight;  int i;  KHE_CONSTRAINT c;  KHE_MONITOR m;
  KHE_EVENT_RESOURCE er;
  if( !KheTaskResourceTypeHasAvoidClashes(task, ms) )
    return *orig_m = NULL, false;
  er = KheTaskEventResource(task);
  if( er != NULL )
  {
    if( KheEventResourcePreassignedResource(er) != NULL )
      return *orig_m = NULL, true;
    weight = KheCost(0, 0);
    for( i = 0;  i < KheSolnEventResourceMonitorCount(ms->soln, er);  i++ )
    {
      m = KheSolnEventResourceMonitor(ms->soln, er, i);
      if( KheMonitorTag(m) == KHE_ASSIGN_RESOURCE_MONITOR_TAG &&
	  KheMonitorAttachedToSoln(m) )
      {
	c = KheMonitorConstraint(m);  /* NB m is assign resource monitor */
	weight += KheConstraintCombinedWeight(c);
	if( weight >= ms->min_weight )
	  return *orig_m = m, true;
      }
    }
  }
  return *orig_m = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingAddOrdinaryDemandMonitors(KHE_MATCHING_SOLVER ms)        */
/*                                                                           */
/*  Add all ordinary demand nodes.                                           */
/*                                                                           */
/*****************************************************************************/

static void KheMatchingAddOrdinaryDemandMonitors(KHE_MATCHING_SOLVER ms)
{
  KHE_INSTANCE ins;  KHE_RESOURCE_TYPE rt;  int i, j, offset;  KHE_TASK task;
  KHE_MONITOR m;  KHE_MEET meet;  KHE_ORDINARY_DEMAND_MONITOR odm;

  /* work out which resource types have avoid clashes monitors */
  ins = KheSolnInstance(ms->soln);
  HaArrayClear(ms->resource_type_has_avoid_clashes);
  for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
  {
    rt = KheInstanceResourceType(ins, i);
    HaArrayAddLast(ms->resource_type_has_avoid_clashes,
      KheResourceTypeHasAvoidClashesMonitors(ms->soln, rt, ms->min_weight));
  }

  /* add ordinary demand nodes to all suitable tasks */
  for( i = 0;  i < KheSolnMeetCount(ms->soln);  i++ )
  {
    meet = KheSolnMeet(ms->soln, i);
    if( KheMeetAllowsDemandMonitors(ms, meet) )
      for( j = 0;  j < KheMeetTaskCount(meet);  j++ )
      {
	task = KheMeetTask(meet, j);
	if( KheTaskAllowsDemandMonitors(ms, task, &m) )
	  for( offset = 0;  offset < KheTaskDuration(task);  offset++ )
	  {
	    odm = KheOrdinaryDemandMonitorMake(task, offset, m);
	    if( ms->integrated )
	      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) ms->soln,
		(KHE_MONITOR) odm);
	    KheMonitorAttachToSoln((KHE_MONITOR) odm);
	  }
      }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDeleteOrdinaryDemandMonitors(KHE_TASK task)                  */
/*                                                                           */
/*  Delete task's ordinary demand monitors.                                  */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDeleteOrdinaryDemandMonitors(KHE_TASK task)
{
  KHE_ORDINARY_DEMAND_MONITOR odm;
  while( KheTaskDemandMonitorCount(task) > 0 )
  {
    odm = KheTaskDemandMonitor(task, 0);
    if( DEBUG6 )
    {
      fprintf(stderr, "    deleting demand monitor ");
      KheMonitorDebug((KHE_MONITOR) odm, 1, 0, stderr);
    }
    KheOrdinaryDemandMonitorDelete(odm);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingDeleteOrdinaryDemandMonitors(KHE_SOLN soln)              */
/*                                                                           */
/*  Delete all ordinary demand nodes.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheMatchingDeleteOrdinaryDemandMonitors(KHE_SOLN soln)
{
  int i, j;  KHE_MEET meet;  KHE_TASK task;

  for( i = 0;  i < KheSolnMeetCount(soln);  i++ )
  {
    meet = KheSolnMeet(soln, i);
    if( DEBUG6 )
      fprintf(stderr, "    deleting demand monitors in meet %s\n",
	KheMeetId(meet));
    if( !KheMeetIsCycleMeet(meet) )
    {
      for( j = 0;  j < KheMeetTaskCount(meet);  j++ )
      {
	task = KheMeetTask(meet, j);
	if( DEBUG6 )
	  fprintf(stderr, "    deleting demand monitors in task %s\n",
	    KheTaskId(task));
	KheTaskDeleteOrdinaryDemandMonitors(task);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "workload requirements and workload demand monitors"           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_WORKLOAD_REQUIREMENT KheWorkloadRequirementMake(KHE_RESOURCE r,      */
/*    int count, KHE_TIME_GROUP tg, KHE_MONITOR orig_m, HA_ARENA a)          */
/*                                                                           */
/*  Make a new workload requirement object with these attributes.            */
/*                                                                           */
/*****************************************************************************/

static KHE_WORKLOAD_REQUIREMENT KheWorkloadRequirementMake(KHE_RESOURCE r,
  int count, KHE_TIME_GROUP tg, KHE_MONITOR orig_m, HA_ARENA a)
{
  KHE_WORKLOAD_REQUIREMENT res;
  HaMake(res, a);
  res->resource = r;
  res->count = count;
  res->time_group = tg;
  res->originating_monitor = orig_m;
  HaArrayInit(res->children, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingSolverAddWorkloadRequirement(KHE_MATCHING_SOLVER ms,     */
/*    KHE_RESOURCE r, int num, KHE_TIME_GROUP tg, KHE_MONITOR m)             */
/*                                                                           */
/*  Append a workload requirement to the workload requirements stored in rs. */
/*                                                                           */
/*****************************************************************************/

void KheMatchingSolverAddWorkloadRequirement(KHE_MATCHING_SOLVER ms,
  KHE_RESOURCE r, int num, KHE_TIME_GROUP tg, KHE_MONITOR m)
{
  KHE_WORKLOAD_REQUIREMENT wr;
  HnAssert(num >= 0 && num <= KheTimeGroupTimeCount(tg),
    "KheSolnMatchingAddWorkloadRequirement invalid num");
  wr = KheWorkloadRequirementMake(r, num, tg, m, ms->arena);
  HaArrayAddLast(ms->workload_requirements, wr);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWorkloadAvoidTimeGroup(KHE_SOLN soln,                            */
/*    KHE_RESOURCE r, KHE_TIME_GROUP tg, KHE_MONITOR m)                      */
/*                                                                           */
/*  Add requirements that cause r to avoid tg completely.                    */
/*                                                                           */
/*****************************************************************************/

static void KheWorkloadAvoidTimeGroup(KHE_MATCHING_SOLVER ms,
  KHE_RESOURCE r, KHE_TIME_GROUP tg, KHE_MONITOR m)
{
  KHE_TIME t;  int i;
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    KheMatchingSolverAddWorkloadRequirement(ms, r, 0,
      KheTimeSingletonTimeGroup(t), m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWorkloadAddAvoidUnavailableTimesRequirement(                     */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)                                 */
/*                                                                           */
/*  Add workload requirements for m to its resource's workload requirements. */
/*                                                                           */
/*****************************************************************************/

static void KheWorkloadAddAvoidUnavailableTimesRequirement(
  KHE_MATCHING_SOLVER ms, KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)
{
  KHE_RESOURCE r;
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c;  KHE_TIME_GROUP tg;

  r = KheAvoidUnavailableTimesMonitorResource(m);
  c = KheAvoidUnavailableTimesMonitorConstraint(m);
  tg = KheAvoidUnavailableTimesConstraintUnavailableTimes(c);
  KheWorkloadAvoidTimeGroup(ms, r, tg, (KHE_MONITOR) m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWorkloadAddLimitBusyTimesRequirement(                            */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Add workload requirements for m to r's workload requirements.            */
/*                                                                           */
/*****************************************************************************/

static void KheWorkloadAddLimitBusyTimesRequirement(
  KHE_MATCHING_SOLVER ms, KHE_LIMIT_BUSY_TIMES_MONITOR m)
{
  KHE_RESOURCE r;  int i, limit, offset;
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c;  KHE_TIME_GROUP tg;
  r = KheLimitBusyTimesMonitorResource(m);
  c = KheLimitBusyTimesMonitorConstraint(m);
  offset = KheLimitBusyTimesMonitorOffset(m);
  limit = KheLimitBusyTimesConstraintMaximum(c);
  for( i = 0;  i < KheLimitBusyTimesConstraintTimeGroupCount(c);  i++ )
  {
    tg = KheLimitBusyTimesConstraintTimeGroup(c, i, offset);
    if( DEBUG4 )
    {
      fprintf(stderr, "  limit busy times constraint %d: ", i);
      KheTimeGroupDebug(tg, 3, 0, stderr);
    }
    if( limit == 0 )
      KheWorkloadAvoidTimeGroup(ms, r, tg, (KHE_MONITOR) m);
    else if( limit < KheTimeGroupTimeCount(tg) )
      KheMatchingSolverAddWorkloadRequirement(ms, r, limit, tg, (KHE_MONITOR)m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWorkloadAddLimitWorkloadRequirement(KHE_MATCHING_SOLVER ms,      */
/*    KHE_LIMIT_WORKLOAD_MONITOR m)                                          */
/*                                                                           */
/*  Add workload requirements for m to r's workload requirements.            */
/*                                                                           */
/*****************************************************************************/

static void KheWorkloadAddLimitWorkloadRequirement(KHE_MATCHING_SOLVER ms,
  KHE_LIMIT_WORKLOAD_MONITOR m)
{
  int limit, i;  KHE_EVENT_CLASS ec;  KHE_TIME_GROUP full_tg;
  KHE_RESOURCE r;  KHE_LIMIT_WORKLOAD_CONSTRAINT c;
  r = KheLimitWorkloadMonitorResource(m);
  c = KheLimitWorkloadMonitorConstraint(m);
  limit = KheLimitWorkloadConstraintMaximum(c);
  full_tg = KheInstanceFullTimeGroup(KheResourceInstance(r));
  if( DEBUG3 )
  {
    fprintf(stderr, "[ KheWorkloadAddLimitWorkloadRequirement(%s, %d c)\n",
      KheResourceId(r) == NULL ? "-" : KheResourceId(r), limit);
    KheTimeGroupDebug(full_tg, 3, 2, stderr);
  }
  HaArrayForEach(ms->event_classes_by_event, ec, i)
    limit += KheEventClassResourceAdjustment(ec, r);
  if( limit < 0 )
    limit = 0;
  if( limit < KheTimeGroupTimeCount(full_tg) )
    KheMatchingSolverAddWorkloadRequirement(ms, r, limit, full_tg,
      (KHE_MONITOR) m);
  if( DEBUG3 )
    fprintf(stderr, "] KheWorkloadAddLimitWorkloadRequirement (final: %d c)\n",
      limit);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWorkloadRequirementInsertIntoForest(                             */
/*    ARRAY_KHE_WORKLOAD_REQUIREMENT *roots, KHE_WORKLOAD_REQUIREMENT wr)    */
/*                                                                           */
/*  Insert wr into *roots.                                                   */
/*                                                                           */
/*****************************************************************************/

static void KheWorkloadRequirementInsertIntoForest(
  ARRAY_KHE_WORKLOAD_REQUIREMENT *roots, KHE_WORKLOAD_REQUIREMENT wr)
{
  int touch_count, superset_count, i;  KHE_WORKLOAD_REQUIREMENT touch_wr, wr2;

  /* find out how many roots wr touches and is a superset of */
  touch_count = 0;
  touch_wr = NULL;
  superset_count = 0;
  HaArrayForEach(*roots, wr2, i)
    if( !KheTimeGroupDisjoint(wr->time_group, wr2->time_group) )
    {
      touch_count++;
      touch_wr = wr2;
      if( KheTimeGroupSubset(wr2->time_group, wr->time_group) )
	superset_count++;
    }

  if( superset_count == touch_count )
  {
    /* wr is a superset of everything it touches; move them to below wr */
    HaArrayForEach(*roots, wr2, i)
      if( KheTimeGroupSubset(wr2->time_group, wr->time_group) )
      {
	HaArrayAddLast(wr->children, wr2);
	HaArrayDeleteAndShift(*roots, i);
	i--;
      }
    HaArrayAddLast(*roots, wr);
  }
  else if( touch_count == 1 &&
      KheTimeGroupSubset(wr->time_group, touch_wr->time_group) )
  {
    /* wr belongs below touch_wr */
    KheWorkloadRequirementInsertIntoForest(&touch_wr->children, wr);
  }
  else
  {
    /* no other legal possibilities, so we abandon wr here */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheWorkloadRequirementToDemandNodes(KHE_WORKLOAD_REQUIREMENT wr,     */
/*    KHE_MATCHING_SOLVER ms)                                                */
/*                                                                           */
/*  Traverse the tree rooted at wr in postorder, adding demand chunks and    */
/*  nodes to rs as required, and returning the number of nodes added by      */
/*  wr and its descendants.                                                  */
/*                                                                           */
/*  Implementation note.  There is one chunk for each workload requirement   */
/*  giving rise to at least one demand node:                                 */
/*                                                                           */
/*     dc->impl       time group of requirement                              */
/*     dc->base       0                                                      */
/*     dc->increment  resource_count                                         */
/*     dc->domain     indexes of time group times                            */
/*                                                                           */
/*     dn->impl       resource                                               */
/*     dn->domain     indexes of resource's singleton resource group         */
/*                                                                           */
/*****************************************************************************/

static int KheWorkloadRequirementToDemandNodes(KHE_WORKLOAD_REQUIREMENT wr,
  KHE_MATCHING_SOLVER ms)
{
  int child_count, root_count, i /* , resource_count */;  /* KHE_MATCHING m; */
  KHE_WORKLOAD_REQUIREMENT child_wr;  /* KHE_MATCHING_DEMAND_CHUNK dc; */
  KHE_WORKLOAD_DEMAND_MONITOR wdm;

  /* build demand nodes in proper descendants and count how many */
  child_count = 0;
  HaArrayForEach(wr->children, child_wr, i)
    child_count += KheWorkloadRequirementToDemandNodes(child_wr, ms);

  /* find out how many we need here at the root */
  root_count = KheTimeGroupTimeCount(wr->time_group) - wr->count - child_count;
  if( root_count < 0 )
    root_count = 0;

  /* add root_count demand nodes */
  for( i = 0;  i < root_count;  i++ )
  {
    wdm = KheWorkloadDemandMonitorMake(ms->soln, wr->resource,
      wr->time_group, wr->originating_monitor);
    if( ms->integrated )
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) ms->soln,
	(KHE_MONITOR) wdm);
    KheMonitorAttachToSoln((KHE_MONITOR) wdm);
  }

  /* ***
  m = KheSolnMat ching(rs->soln);
  HnAssert(m != NULL, "KheWorkloadRequirementToDemandNodes: no matching");
  resource_count = KheInstanceResourceCount(KheSolnInstance(rs->soln));
  dc = KheMatchingDemandChunkMake(m, NULL, 0, resource_count,
    KheTimeGroupTimeSet(wr->time_group));
  HaArrayAddLast(rs->workload_demand_chunks, dc);
  *** */

  /* return the total number of demand nodes added at wr or below it */
  return child_count + root_count;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintCmp(const void *t1, const void *t2)                     */
/*                                                                           */
/*  Comparison function for sorting an array of constraints by decreasing    */
/*  combined weight.                                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheConstraintCmp(const void *t1, const void *t2)
{
  KHE_CONSTRAINT c1 = * (KHE_CONSTRAINT *) t1;
  KHE_CONSTRAINT c2 = * (KHE_CONSTRAINT *) t2;
  int cost_cmp = KheCostCmp(KheConstraintCombinedWeight(c2),
    KheConstraintCombinedWeight(c1));
  if( cost_cmp != 0 )
    return cost_cmp;
  else
    return KheConstrai ntIndex(c1) - KheConstra intIndex(c2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorCmp(const void *t1, const void *t2)                        */
/*                                                                           */
/*  Comparison function for sorting an array of monitors by decreasing       */
/*  combined weight.                                                         */
/*                                                                           */
/*****************************************************************************/

static int KheMonitorCmp(const void *t1, const void *t2)
{
  KHE_MONITOR m1 = * (KHE_MONITOR *) t1;
  KHE_MONITOR m2 = * (KHE_MONITOR *) t2;
  int cost_cmp;
  cost_cmp = KheCostCmp(KheMonitorCombinedWeight(m2),
    KheMonitorCombinedWeight(m1));
  if( cost_cmp != 0 )
    return cost_cmp;
  else
    return KheMonitorSolnIndex(m1) - KheMonitorSolnIndex(m2);
  /* *** old version that assumes that the monitors have constraints
  KHE_MONITOR m1 = * (KHE_MONITOR *) t1;
  KHE_MONITOR m2 = * (KHE_MONITOR *) t2;
  KHE_CONSTRAINT c1 = KheMonit orConstraint(m1);
  KHE_CONSTRAINT c2 = KheMoni torConstraint(m2);
  int cost_cmp;
  HnAssert(c1 != NULL, "KheMonitorCmp: monitor has no constraint");
  HnAssert(c2 != NULL, "KheMonitorCmp: monitor has no constraint");
  cost_cmp = KheCostCmp(KheConstraintCombinedWeight(c2),
    KheConstraintCombinedWeight(c1));
  if( cost_cmp != 0 )
    return cost_cmp;
  else
    return KheConstraintIndex(c1) - KheConstraintIndex(c2);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceAddWorkloadDemandMonitors(KHE_MATCHING_SOLVER ms,        */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Add workload demand monitors for resource r.                             */
/*                                                                           */
/*****************************************************************************/

static void KheResourceAddWorkloadDemandMonitors(KHE_MATCHING_SOLVER ms,
  KHE_RESOURCE r)
{
  KHE_MONITOR m;  int i;  KHE_CONSTRAINT c;  KHE_WORKLOAD_REQUIREMENT wr;
  if( DEBUG4 )
    fprintf(stderr, "[ KheResourceAddWorkloadDemandMonitors(m, %s)\n",
      KheResourceId(r) == NULL ? "-" : KheResourceId(r));

  /* select the relevant monitors and sort by decreasing weight */
  HaArrayClear(ms->monitors);
  for( i = 0;  i < KheSolnResourceMonitorCount(ms->soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(ms->soln, r, i);
    switch( KheMonitorTag(m) )
    {
      case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
      case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
      case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

	c = KheMonitorConstraint(m);  /* NB m is a resource monitor */
	if( KheConstraintCombinedWeight(c) >= ms->min_weight )
	  HaArrayAddLast(ms->monitors, m);
	break;

      default:

        /* nothing to do here */
        break;
    }
  }
  HaArraySort(ms->monitors, &KheMonitorCmp);

  /* add the requirements, taking monitors in decreasing weight order */
  HaArrayClear(ms->workload_requirements);
  HaArrayForEach(ms->monitors, m, i)
    switch( KheMonitorTag(m) )
    {
      case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

	KheWorkloadAddAvoidUnavailableTimesRequirement(ms,
	  (KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m);
	break;

      case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

	KheWorkloadAddLimitBusyTimesRequirement(ms,
	  (KHE_LIMIT_BUSY_TIMES_MONITOR) m);
	break;

      case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

	KheWorkloadAddLimitWorkloadRequirement(ms,
	  (KHE_LIMIT_WORKLOAD_MONITOR) m);
	break;

      default:

	HnAbort("KheResourceAddWorkloadDemandMonitors internal error");
    }

  /* build the forest */
  HaArrayClear(ms->forest_roots);
  HaArrayForEach(ms->workload_requirements, wr, i)
    KheWorkloadRequirementInsertIntoForest(&ms->forest_roots, wr);

  /* traverse the forest, adding workload demand nodes */
  HaArrayForEach(ms->forest_roots, wr, i)
    KheWorkloadRequirementToDemandNodes(wr, ms);
  if( DEBUG4 )
    fprintf(stderr, "] KheResourceAddWorkloadDemandMonitors returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceDeleteWorkloadDemandMonitors(KHE_SOLN soln,              */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Delete r's workload demand monitors.                                     */
/*                                                                           */
/*****************************************************************************/

static void KheResourceDeleteWorkloadDemandMonitors(KHE_SOLN soln,
  KHE_RESOURCE r)
{
  KHE_MONITOR m;  int i;  KHE_WORKLOAD_DEMAND_MONITOR wdm;
  for( i = 0;  i < KheSolnResourceMonitorCount(soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(soln, r, i);
    if( KheMonitorTag(m) == KHE_WORKLOAD_DEMAND_MONITOR_TAG )
    {
      wdm = (KHE_WORKLOAD_DEMAND_MONITOR) m;
      KheWorkloadDemandMonitorDelete(wdm);
      i--;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingAddWorkloadDemandMonitors(KHE_SOLN soln)                 */
/*                                                                           */
/*  Add all workload demand monitors to soln.                                */
/*                                                                           */
/*****************************************************************************/

static void KheMatchingAddWorkloadDemandMonitors(KHE_MATCHING_SOLVER ms)
{
  int i;  KHE_INSTANCE ins;  KHE_RESOURCE r;
  KheEventClassesBuild(ms);
  ins = KheSolnInstance(ms->soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    r = KheInstanceResource(ins, i);
    KheResourceAddWorkloadDemandMonitors(ms, r);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingDeleteWorkloadDemandMonitors(KHE_SOLN soln)              */
/*                                                                           */
/*  Delete all workload demand monitors.                                     */
/*                                                                           */
/*****************************************************************************/

static void KheMatchingDeleteWorkloadDemandMonitors(KHE_SOLN soln)
{
  KHE_INSTANCE ins;  int i;  KHE_RESOURCE r;

  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    r = KheInstanceResource(ins, i);
    KheResourceDeleteWorkloadDemandMonitors(soln, r);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "adjusting resource monitors"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheLimitWorkloadMonitorFindCeiling(KHE_LIMIT_WORKLOAD_MONITOR lwm)   */
/*                                                                           */
/*  Find a suitable value for lwm's ceiling attribute:  the number of        */
/*  times in the cycle minus the number of workload demand monitors for      */
/*  lwm's resource.                                                          */
/*                                                                           */
/*****************************************************************************/

static int KheLimitWorkloadMonitorFindCeiling(KHE_LIMIT_WORKLOAD_MONITOR lwm)
{
  KHE_RESOURCE r;  KHE_SOLN soln;  int i, res;  KHE_MONITOR m;
  r = KheLimitWorkloadMonitorResource(lwm);
  soln = KheMonitorSoln((KHE_MONITOR) lwm);
  res = KheInstanceTimeCount(KheSolnInstance(soln));
  for( i = 0;  i < KheSolnResourceMonitorCount(soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(soln, r, i);
    if( KheMonitorTag(m) == KHE_WORKLOAD_DEMAND_MONITOR_TAG )
      res--;
  }
  if( res < 0 )
    res = 0;
  if( DEBUG10 )
    fprintf(stderr, "  KheLimitWorkloadMonitorFindCeiling(%s) = %d\n",
      KheResourceId(r), res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDoAdjustResourceMonitors(KHE_SOLN soln, KHE_RESOURCE r)          */
/*                                                                           */
/*  Adjust r's resource monitors.  Only avoid clashes monitors and monitors  */
/*  which are the originating monitors of workload demand nodes are touched. */
/*                                                                           */
/*  Implementation note.  A monitor may be the originating monitor for       */
/*  several workload demand monitors, so the changes to om below may         */
/*  be carried out multiple times (unless they include detaching om).        */
/*  Luckily, that's safe.  Also variable done_lwm reduces the likelihood.    */
/*                                                                           */
/*****************************************************************************/

static void KheDoAdjustResourceMonitors(KHE_SOLN soln, KHE_RESOURCE r)
{
  int i;  KHE_MONITOR m, om;  KHE_LIMIT_WORKLOAD_MONITOR lwm, done_lwm;
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc;  KHE_LIMIT_BUSY_TIMES_MONITOR lbtm;
  done_lwm = NULL;
  for( i = 0;  i < KheSolnResourceMonitorCount(soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(soln, r, i);
    if( KheMonitorAttachedToSoln(m) ) switch( KheMonitorTag(m) )
    {
      case KHE_AVOID_CLASHES_MONITOR_TAG:

	KheMonitorDetachFromSoln(m);
	break;

      case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

	om = KheWorkloadDemandMonitorOriginatingMonitor(
	  (KHE_WORKLOAD_DEMAND_MONITOR) m);
	if( KheMonitorAttachedToSoln(om) ) switch( KheMonitorTag(om) )
	{
	  case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

            KheMonitorDetachFromSoln(om);
	    break;

	  case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

            lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) om;
	    lbtc = KheLimitBusyTimesMonitorConstraint(lbtm);
	    if( KheLimitBusyTimesConstraintMinimum(lbtc) == 0 )
              KheMonitorDetachFromSoln(om);
	    else
	      KheLimitBusyTimesMonitorSetCeiling(lbtm,
		KheLimitBusyTimesConstraintMaximum(lbtc));
	    break;

	  case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

            lwm = (KHE_LIMIT_WORKLOAD_MONITOR) om;
	    if( lwm != done_lwm )
	    {
	      KheLimitWorkloadMonitorSetCeiling(lwm,
		KheLimitWorkloadMonitorFindCeiling(lwm));
	      done_lwm = lwm;
	    }
	    break;

	  default:

	    /* nothing to do otherwise */
	    break;
	}
	break;

      default:

	/* nothing to do otherwise */
	break;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDoUnAdjustResourceMonitors(KHE_SOLN soln, KHE_RESOURCE r)        */
/*                                                                           */
/*  Unadjust r's resource monitors.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheDoUnAdjustResourceMonitors(KHE_SOLN soln, KHE_RESOURCE r)
{
  int i;  KHE_MONITOR m, om;  KHE_LIMIT_BUSY_TIMES_MONITOR lbtm;
  KHE_LIMIT_WORKLOAD_MONITOR lwm;
  for( i = 0;  i < KheSolnResourceMonitorCount(soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(soln, r, i);
    switch( KheMonitorTag(m) )
    {
      case KHE_AVOID_CLASHES_MONITOR_TAG:

	if( !KheMonitorAttachedToSoln(m) )
	  KheMonitorAttachToSoln(m);
	break;

      case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

	om = KheWorkloadDemandMonitorOriginatingMonitor(
	  (KHE_WORKLOAD_DEMAND_MONITOR) m);
	switch( KheMonitorTag(om) )
	{
	  case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

	    if( !KheMonitorAttachedToSoln(om) )
	      KheMonitorAttachToSoln(om);
	    break;

	  case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

            lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) om;
	    KheLimitBusyTimesMonitorSetCeiling(lbtm, INT_MAX);
	    if( !KheMonitorAttachedToSoln(om) )
	      KheMonitorAttachToSoln(om);
	    break;

	  case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

            lwm = (KHE_LIMIT_WORKLOAD_MONITOR) om;
	    KheLimitWorkloadMonitorSetCeiling(lwm, INT_MAX);
	    if( !KheMonitorAttachedToSoln(om) )
	      KheMonitorAttachToSoln(om);
	    break;

	  default:

	    /* nothing to do otherwise */
	    break;
	}
	break;

      default:

	/* nothing to do otherwise */
	break;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAdjustResourceMonitors(KHE_SOLN soln)                            */
/*                                                                           */
/*  Adjust resource monitors.                                                */
/*                                                                           */
/*****************************************************************************/

static void KheAdjustResourceMonitors(KHE_SOLN soln)
{
  int i;  KHE_INSTANCE ins;
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
    KheDoAdjustResourceMonitors(soln, KheInstanceResource(ins, i));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheUnAdjustResourceMonitors(KHE_SOLN soln)                          */
/*                                                                           */
/*  Unadjust resource monitors.                                              */
/*                                                                           */
/*  If KheAdjustResourceMonitors was not called, KheUnAdjustResourceMonitors */
/*  will change nothing, so it's safe to call it in that case.               */
/*                                                                           */
/*****************************************************************************/

static void KheUnAdjustResourceMonitors(KHE_SOLN soln)
{
  int i;  KHE_INSTANCE ins;
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
    KheDoUnAdjustResourceMonitors(soln, KheInstanceResource(ins, i));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "public functions"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingBegin(KHE_SOLN soln, bool integrated)                    */
/*                                                                           */
/*  Begin the matching in soln.  Make it a separate matching if integrated   */
/*  is false, and an integrated matching if integrated is true.              */
/*                                                                           */
/*  Implementation note.  KheAdjustResourceMonitors assumes that the         */
/*  workload demand monitors are present, so it must be called after         */
/*  KheMatchingAddWorkloadDemandMonitors.                                    */
/*                                                                           */
/*****************************************************************************/

void KheMatchingBegin(KHE_SOLN soln, bool integrated)
{
  KHE_MATCHING_SOLVER ms;  HA_ARENA a;
  if( DEBUG5 )
    fprintf(stderr, "[ KheMatchingBegin(soln of %s, %s)\n",
      KheInstanceId(KheSolnInstance(soln)), bool_show(integrated));
  a = KheSolnArenaBegin(soln);
  ms = KheMatchingSolverMake(soln, integrated, a);
  KheSolnMatchingBegin(soln);
  KheSolnMatchingSetWeight(soln, ms->min_weight);
  KheMatchingAddOrdinaryDemandMonitors(ms);
  KheMatchingAddWorkloadDemandMonitors(ms);
  KheSolnArenaEnd(soln, a);
  if( integrated )
    KheAdjustResourceMonitors(soln);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMatchingEnd(KHE_SOLN soln)                                       */
/*                                                                           */
/*  End the matching in soln.                                                */
/*                                                                           */
/*  Implementation note.  KheUnAdjustResourceMonitors assumes that the       */
/*  workload demand monitors are still present, so it must be called before  */
/*  KheMatchingDeleteWorkloadDemandMonitors.                                 */
/*                                                                           */
/*****************************************************************************/

void KheMatchingEnd(KHE_SOLN soln)
{
  if( DEBUG6 )
    fprintf(stderr, "  starting KheMatchingEnd\n");
  KheUnAdjustResourceMonitors(soln);
  KheMatchingDeleteWorkloadDemandMonitors(soln);
  KheMatchingDeleteOrdinaryDemandMonitors(soln);
  KheSolnMatchingEnd(soln);
  if( DEBUG5 )
    fprintf(stderr, "] KheMatchingEnd(soln of %s)\n",
      KheInstanceId(KheSolnInstance(soln)));
}

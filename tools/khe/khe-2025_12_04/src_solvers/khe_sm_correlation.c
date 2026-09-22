
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
/*  FILE:         khe_sm_correlation.c                                       */
/*  DESCRIPTION:  Monitor grouping for correlation                           */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0
#define DEBUG9 0
#define DEBUG10 0
#define DEBUG11 0
#define DEBUG12 0
#define DEBUG13 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET_SIG - the signature of one meet                                 */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_meet_sig_rec {
  KHE_MEET		leader;
  int			offset;
  int			duration;
} *KHE_MEET_SIG;

typedef HA_ARRAY(KHE_MEET_SIG) ARRAY_KHE_MEET_SIG;


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_SIG - the signature of one event                               */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_event_sig_rec {
  ARRAY_KHE_MEET_SIG	meet_sigs;
} *KHE_EVENT_SIG;


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_CLASS - a class of equivalent events                           */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_EVENT) ARRAY_KHE_EVENT;

typedef struct khe_event_class_rec {
  ARRAY_KHE_EVENT	events;			/* the events in the class */
  KHE_EVENT_SIG		sig;			/* their shared signature  */
} *KHE_EVENT_CLASS;

typedef HA_ARRAY(KHE_EVENT_CLASS) ARRAY_KHE_EVENT_CLASS;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_AND_CLASSES - a monitor and the event classes it monitors    */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_monitor_and_classes_rec {
  KHE_MONITOR			monitor;		/* the monitor       */
  ARRAY_KHE_EVENT_CLASS		event_classes;		/* its event classes */
} *KHE_MONITOR_AND_CLASSES;

typedef HA_ARRAY(KHE_MONITOR_AND_CLASSES) ARRAY_MONITOR_AND_CLASSES;


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE_SIG - the signature of one event resource             */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK;

typedef struct khe_event_resource_sig_rec
{
  ARRAY_KHE_TASK			leader_tasks;
} *KHE_EVENT_RESOURCE_SIG;


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE_CLASS - class of equivalent event resources           */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_EVENT_RESOURCE) ARRAY_KHE_EVENT_RESOURCE;

typedef struct khe_event_resource_class_rec {
  ARRAY_KHE_EVENT_RESOURCE	event_resources;  /* the event resources     */
  KHE_EVENT_RESOURCE_SIG	sig;		  /* their shared signature  */
} *KHE_EVENT_RESOURCE_CLASS;

typedef HA_ARRAY(KHE_EVENT_RESOURCE_CLASS) ARRAY_KHE_EVENT_RESOURCE_CLASS;


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SIG - the signature of one resource                         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_resource_sig_rec {
  ARRAY_KHE_EVENT		events;
} *KHE_RESOURCE_SIG;


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_CLASS - a class of equivalent resources                     */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_RESOURCE) ARRAY_KHE_RESOURCE;

typedef struct khe_resource_class_rec {
  ARRAY_KHE_RESOURCE		resources;
  KHE_RESOURCE_SIG		sig;
} *KHE_RESOURCE_CLASS;

typedef HA_ARRAY(KHE_RESOURCE_CLASS) ARRAY_KHE_RESOURCE_CLASS;


/*****************************************************************************/
/*                                                                           */
/*  KHE_CORRELATION_SOLVER - a solver for grouping correlated monitors       */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_MONITOR) ARRAY_KHE_MONITOR;

typedef struct khe_correlation_solver_rec {
  HA_ARENA				arena;
  KHE_SOLN				soln;
  ARRAY_KHE_EVENT_CLASS			tmp_event_classes;
  ARRAY_KHE_EVENT_CLASS			all_event_classes;
  ARRAY_KHE_EVENT_CLASS			event_classes_by_event;
  ARRAY_KHE_EVENT_RESOURCE_CLASS	tmp_event_resource_classes;
  ARRAY_KHE_EVENT_RESOURCE_CLASS	all_event_resource_classes;
  ARRAY_KHE_RESOURCE_CLASS		tmp_resource_classes;
  ARRAY_KHE_RESOURCE_CLASS		all_resource_classes;
  ARRAY_KHE_MONITOR			tmp_monitors;
  ARRAY_KHE_MONITOR			tmp_monitors2;
  ARRAY_MONITOR_AND_CLASSES		monitor_and_classes_array;
} *KHE_CORRELATION_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_CLASS_SOLVER - solver object for event monitor grouping        */
/*                                                                           */
/*****************************************************************************/

/* ***
typedef struct khe_event_class_solver_rec {
  HA_ARENA				arena;
  KHE_SOLN				soln;
  ARRAY_KHE_EVENT_CLASS			tmp_event_classes;
  ARRAY_KHE_EVENT_CLASS			all_event_classes;
  ARRAY_KHE_EVENT_CLASS			event_classes_by_event;
  ** ARRAY_KHE_MEET			tmp_leader_meets; **
  ** HA_ARRAY_INT			tmp_offsets; **
  ** HA_ARRAY_INT			tmp_durations; **
  ** HA_ARRAY_INT			tmp_multiplicities; **
  ARRAY_KHE_MONITOR			tmp_monitors;
  ARRAY_KHE_MONITOR			tmp_monitors2;
  ARRAY_KHE_RESOURCE_CLASS_LAYER	tmp_layers;
  ARRAY_MONITOR_AND_CLASSES		monitor_and_classes_array;
} *KHE_EVENT_CLASS_SOLVER;
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE_CLASS_SOLVER - a solver for event resources           */
/*                                                                           */
/*****************************************************************************/

/* ***
typedef struct khe_event_resource_class_solver_rec {
  HA_ARENA				arena;
  KHE_SOLN				soln;
  ARRAY_KHE_EVENT_RESOURCE_CLASS	tmp_event_resource_classes;
  ARRAY_KHE_EVENT_RESOURCE_CLASS	all_event_resource_classes;
  ** ARRAY_KHE_EVENT_RESOURCE_CLASS event_resource_classes_by_event_resource; **
  ** ARRAY_KHE_EVENT_RESOURCE_CLASS	final_classes; **
  ** ARRAY_KHE_TASK			tmp_leader_tasks; **
  ** HA_ARRAY_INT			tmp_multiplicities; **
  ARRAY_KHE_MONITOR			tmp_monitors;
  ARRAY_KHE_MONITOR			tmp_monitors2;
} *KHE_EVENT_RESOURCE_CLASS_SOLVER;
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MEET_SIG"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET_SIG KheMeetSigMake(KHE_MEET meet, HA_ARENA a)                   */
/*                                                                           */
/*  Make a new meet signature object for meet;  the leader could be NULL.    */
/*                                                                           */
/*****************************************************************************/

static KHE_MEET_SIG KheMeetSigMake(KHE_MEET meet, HA_ARENA a)
{
  KHE_MEET_SIG res;
  HaMake(res, a);
  res->leader = KheMeetFirstMovable(meet, &res->offset);
  res->duration = KheMeetDuration(meet);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetSigTypedCmp(KHE_MEET_SIG msig1, KHE_MEET_SIG msig2)           */
/*                                                                           */
/*  Typed comparison function for bringing equal meet signatures together.   */
/*                                                                           */
/*****************************************************************************/

static int KheMeetSigTypedCmp(KHE_MEET_SIG msig1, KHE_MEET_SIG msig2)
{
  int index1 = (msig1->leader != NULL ? KheMeetSolnIndex(msig1->leader) : -1);
  int index2 = (msig2->leader != NULL ? KheMeetSolnIndex(msig2->leader) : -1);
  if( index1 != index2 )
    return index1 - index2;
  else if( msig1->offset != msig2->offset )
    return msig1->offset - msig2->offset;
  else
    return msig1->duration - msig2->duration;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetSigCmp(const void *t1, const void *t2)                        */
/*                                                                           */
/*  Untyped comparison function for bringing equal meet signatures together. */
/*                                                                           */
/*****************************************************************************/

static int KheMeetSigCmp(const void *t1, const void *t2)
{
  KHE_MEET_SIG msig1 = * (KHE_MEET_SIG *) t1;
  KHE_MEET_SIG msig2 = * (KHE_MEET_SIG *) t2;
  return KheMeetSigTypedCmp(msig1, msig2);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_EVENT_SIG"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_SIG KheEventSigMake(KHE_EVENT e, KHE_SOLN soln, HA_ARENA a)    */
/*                                                                           */
/*  Make the signature of event e in soln.                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_EVENT_SIG KheEventSigMake(KHE_EVENT e, KHE_SOLN soln, HA_ARENA a)
{
  KHE_EVENT_SIG res;  int i;  KHE_MEET meet;
  HaMake(res, a);
  HaArrayInit(res->meet_sigs, a);
  for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
  {
    meet = KheEventMeet(soln, e, i);
    HaArrayAddLast(res->meet_sigs, KheMeetSigMake(meet, a));
  }
  HaArraySort(res->meet_sigs, &KheMeetSigCmp);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventSigTypedCmp(KHE_EVENT_SIG esig1, KHE_EVENT_SIG esig2)        */
/*                                                                           */
/*  Typed comparison function for comparing two event signatures.            */
/*                                                                           */
/*****************************************************************************/

static int KheEventSigTypedCmp(KHE_EVENT_SIG esig1, KHE_EVENT_SIG esig2)
{
  int count1, count2, i, cmp;  KHE_MEET_SIG msig1, msig2;
  count1 = HaArrayCount(esig1->meet_sigs);
  count2 = HaArrayCount(esig2->meet_sigs);
  if( count1 != count2 )
    return count1 - count2;
  for( i = 0;  i < count1;  i++ )
  {
    msig1 = HaArray(esig1->meet_sigs, i);
    msig2 = HaArray(esig2->meet_sigs, i);
    cmp = KheMeetSigTypedCmp(msig1, msig2);
    if( cmp != 0 )
      return cmp;
  }
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_EVENT_CLASS"                                              */
/*                                                                           */
/*  An event class is a set of events whose fixed meet assignments prove     */
/*  that they run simultaneously.                                            */
/*                                                                           */
/*  It is an invariant that each event e lies in exactly one class, and      */
/*  that cs->event_classes_by_event[KheEventIndex(e)] is that class.         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_CLASS KheEventClassMake(KHE_EVENT e, HA_ARENA a)               */
/*                                                                           */
/*  Make an event class containing e.                                        */
/*                                                                           */
/*****************************************************************************/

static KHE_EVENT_CLASS KheEventClassMake(KHE_EVENT e, KHE_SOLN soln, HA_ARENA a)
{
  KHE_EVENT_CLASS res;
  HaMake(res, a);
  HaArrayInit(res->events, a);
  HaArrayAddLast(res->events, e);
  res->sig = KheEventSigMake(e, soln, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_CLASS KheEventToEventClass(KHE_EVENT e,                        */
/*    KHE_CORRELATION_SOLVER ecs)                                            */
/*                                                                           */
/*  Return the event class containing e.  There always is one.               */
/*                                                                           */
/*****************************************************************************/

static KHE_EVENT_CLASS KheEventToEventClass(KHE_EVENT e,
  KHE_CORRELATION_SOLVER ecs)
{
  return HaArray(ecs->event_classes_by_event, KheEventIndex(e));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventClassTypedCmp(KHE_EVENT_CLASS ec1, KHE_EVENT_CLASS ec2)      */
/*                                                                           */
/*  Typed comparison function for bringing event classes with equal          */
/*  signatures together.                                                     */
/*                                                                           */
/*****************************************************************************/

static int KheEventClassTypedCmp(KHE_EVENT_CLASS ec1, KHE_EVENT_CLASS ec2)
{
  return KheEventSigTypedCmp(ec1->sig, ec2->sig);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventClassCmp(const void *t1, const void *t2)                     */
/*                                                                           */
/*  Untyped comparison function for bringing event classes with equal        */
/*  signatures together.                                                     */
/*                                                                           */
/*****************************************************************************/

static int KheEventClassCmp(const void *t1, const void *t2)
{
  KHE_EVENT_CLASS ec1 = * (KHE_EVENT_CLASS *) t1;
  KHE_EVENT_CLASS ec2 = * (KHE_EVENT_CLASS *) t2;
  return KheEventClassTypedCmp(ec1, ec2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventClassDebug(KHE_EVENT_CLASS ec, int indent, FILE *fp)        */
/*                                                                           */
/*  Debug print of ec onto fp with the given indent.                         */
/*                                                                           */
/*****************************************************************************/

static void KheEventClassDebug(KHE_EVENT_CLASS ec, int indent, FILE *fp)
{
  KHE_EVENT e;  int i;
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "{");
  HaArrayForEach(ec->events, e, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    fprintf(fp, "%s", KheEventName(e));
  }
  fprintf(fp, "}");
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MONITOR_AND_CLASSES"                                      */
/*                                                                           */
/*  A monitor-and-classes object is an event monitor plus the classes of     */
/*  the events it monitors.                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_AND_CLASSES KheMonitorAndClassesMake(KHE_MONITOR m)          */
/*                                                                           */
/*  Make a monitor-and-classes object for monitor m.                         */
/*                                                                           */
/*****************************************************************************/

static KHE_MONITOR_AND_CLASSES KheMonitorAndClassesMake(KHE_MONITOR m,
  HA_ARENA a)
{
  KHE_MONITOR_AND_CLASSES res;
  HaMake(res, a);
  res->monitor = m;
  HaArrayInit(res->event_classes, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorAndClassesAddClass(KHE_MONITOR_AND_CLASSES mac,           */
/*    KHE_EVENT_CLASS ec)                                                    */
/*                                                                           */
/*  Add ec to mac.                                                           */
/*                                                                           */
/*****************************************************************************/

static void KheMonitorAndClassesAddClass(KHE_MONITOR_AND_CLASSES mac,
  KHE_EVENT_CLASS ec)
{
  HaArrayAddLast(mac->event_classes, ec);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorAndClassesSortClasses(KHE_MONITOR_AND_CLASSES mac)        */
/*                                                                           */
/*  Sort the event classes of mac into a canonical order.                    */
/*                                                                           */
/*****************************************************************************/

static void KheMonitorAndClassesSortClasses(KHE_MONITOR_AND_CLASSES mac)
{
  HaArraySortUnique(mac->event_classes, &KheEventClassCmp);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_AND_CLASSES KheMonitorAndClassesSpreadMake(                  */
/*    KHE_SPREAD_EVENTS_MONITOR sem, KHE_CORRELATION_SOLVER cs)              */
/*                                                                           */
/*  Make a monitor-and-classes object for spread events monitor sem.         */
/*                                                                           */
/*****************************************************************************/

static KHE_MONITOR_AND_CLASSES KheMonitorAndClassesSpreadMake(
  KHE_SPREAD_EVENTS_MONITOR sem, KHE_CORRELATION_SOLVER cs)
{
  KHE_MONITOR_AND_CLASSES res;  KHE_EVENT_GROUP eg;  KHE_EVENT e;  int i;
  KHE_EVENT_CLASS ec;
  res = KheMonitorAndClassesMake((KHE_MONITOR) sem, cs->arena);
  eg = KheSpreadEventsMonitorEventGroup(sem);
  for( i = 0;  i < KheEventGroupEventCount(eg);  i++ )
  {
    e = KheEventGroupEvent(eg, i);
    ec = KheEventToEventClass(e, cs);
    HnAssert(ec != NULL, "KheMonitorAndClassesSpreadMake internal error "
      "(event %s at index %d has no class)", KheEventId(e), KheEventIndex(e));
    KheMonitorAndClassesAddClass(res, ec);
  }
  KheMonitorAndClassesSortClasses(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_AND_CLASSES KheMonitorAndClassesOrderMake(                   */
/*    KHE_ORDER_EVENTS_MONITOR oem, KHE_CORRELATION_SOLVER cs)               */
/*                                                                           */
/*  Make a monitor-and-classes object for order events monitor oem.          */
/*  Return NULL if the monitor's two events lie in the same event class.     */
/*                                                                           */
/*****************************************************************************/

static KHE_MONITOR_AND_CLASSES KheMonitorAndClassesOrderMake(
  KHE_ORDER_EVENTS_MONITOR oem, KHE_CORRELATION_SOLVER cs)
{
  KHE_MONITOR_AND_CLASSES res;  KHE_EVENT e;  KHE_EVENT_CLASS ec1, ec2;
  e = KheOrderEventsMonitorFirstEvent(oem);
  ec1 = KheEventToEventClass(e, cs);
  e = KheOrderEventsMonitorSecondEvent(oem);
  ec2 = KheEventToEventClass(e, cs);
  if( ec1 == ec2 )
    return NULL;
  res = KheMonitorAndClassesMake((KHE_MONITOR) oem, cs->arena);
  KheMonitorAndClassesAddClass(res, ec1);
  KheMonitorAndClassesAddClass(res, ec2);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorAndClassesTypedCmp(KHE_MONITOR_AND_CLASSES mac1,           */
/*    KHE_MONITOR_AND_CLASSES mac2)                                          */
/*                                                                           */
/*  Typed comparison function for sorting monitor-and-classes objects.       */
/*                                                                           */
/*****************************************************************************/

static int KheMonitorAndClassesTypedCmp(KHE_MONITOR_AND_CLASSES mac1,
  KHE_MONITOR_AND_CLASSES mac2)
{
  int i, cmp, count1, count2;  KHE_EVENT_CLASS ec1, ec2;
  count1 = HaArrayCount(mac1->event_classes);
  count2 = HaArrayCount(mac2->event_classes);
  if( count1 != count2 )
    return count1 - count2;
  for( i = 0;  i < count1;  i++ )
  {
    ec1 = HaArray(mac1->event_classes, i);
    ec2 = HaArray(mac2->event_classes, i);
    if( ec1 != ec2 )
    {
      cmp = KheEventClassTypedCmp(ec1, ec2);
      HnAssert(cmp != 0, "KheMonitorAndClassesTypedCmp internal error");
      return cmp;
    }
  }
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorAndClassesCmp(const void *t1, const void *t2)              */
/*                                                                           */
/*  Untyped comparison function for sorting monitor-and-classes objects.     */
/*                                                                           */
/*****************************************************************************/

static int KheMonitorAndClassesCmp(const void *t1, const void *t2)
{
  KHE_MONITOR_AND_CLASSES mac1 = * (KHE_MONITOR_AND_CLASSES *) t1;
  KHE_MONITOR_AND_CLASSES mac2 = * (KHE_MONITOR_AND_CLASSES *) t2;
  return KheMonitorAndClassesTypedCmp(mac1, mac2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorAndClassesDebug(KHE_MONITOR_AND_CLASSES mac,              */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of mac onto fp with the given indent.                        */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static void KheMonitorAndClassesDebug(KHE_MONITOR_AND_CLASSES mac,
  int indent, FILE *fp)
{
  KHE_EVENT_CLASS ec;  int i;
  fprintf(fp, "%*s[ MonitorAndClasses\n", indent, "");
  KheMonitorDebug(mac->monitor, 2, indent + 2, fp);
  HaArrayForEach(mac->event_classes, ec, i)
    KheEventClassDebug(ec, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_EVENT_RESOURCE_SIG"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheTaskTypedCmp(KHE_TASK task1, KHE_TASK task2)                      */
/*                                                                           */
/*  Typed comparison function for sorting tasks.                             */
/*                                                                           */
/*****************************************************************************/

static int KheTaskTypedCmp(KHE_TASK task1, KHE_TASK task2)
{
  return KheTaskSolnIndex(task1) - KheTaskSolnIndex(task2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskCmp(const void *t1, const void *t2)                           */
/*                                                                           */
/*  Untyped comparison function for sorting tasks.                           */
/*                                                                           */
/*****************************************************************************/

static int KheTaskCmp(const void *t1, const void *t2)
{
  KHE_TASK task1 = * (KHE_TASK *) t1;
  KHE_TASK task2 = * (KHE_TASK *) t2;
  return KheTaskTypedCmp(task1, task2);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE_SIG KheEventResourceSigMake(KHE_EVENT_RESOURCE er,    */
/*    KHE_SOLN soln, HA_ARENA a)                                             */
/*                                                                           */
/*  Make a signature for er in soln.                                         */
/*                                                                           */
/*****************************************************************************/

static KHE_EVENT_RESOURCE_SIG KheEventResourceSigMake(KHE_EVENT_RESOURCE er,
  KHE_SOLN soln, HA_ARENA a)
{
  KHE_EVENT_RESOURCE_SIG res;  int i;  KHE_TASK task, leader_task;
  HaMake(res, a);
  HaArrayInit(res->leader_tasks, a);
  for( i = 0;  i < KheEventResourceTaskCount(soln, er);  i++ )
  {
    task = KheEventResourceTask(soln, er, i);
    leader_task = KheTaskProperRoot(task);
    /* leader_task = KheTaskFirstUnFixed(task); */
    HaArrayAddLast(res->leader_tasks, leader_task);
  }
  HaArraySort(res->leader_tasks, &KheTaskCmp);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventResourceSigTypedCmp(KHE_EVENT_RESOURCE_SIG er_sig1,          */
/*    KHE_EVENT_RESOURCE_SIG er_sig2)                                        */
/*                                                                           */
/*  Typed comparison function for bringing equal event resource signatures   */
/*  together.                                                                */
/*                                                                           */
/*****************************************************************************/

static int KheEventResourceSigTypedCmp(KHE_EVENT_RESOURCE_SIG er_sig1,
  KHE_EVENT_RESOURCE_SIG er_sig2)
{
  int count1, count2, i;  KHE_TASK task1, task2;
  count1 = HaArrayCount(er_sig1->leader_tasks);
  count2 = HaArrayCount(er_sig2->leader_tasks);
  if( count1 != count2 )
    return count1 - count2;
  for( i = 0;  i < count1;  i++ )
  {
    task1 = HaArray(er_sig1->leader_tasks, i);
    task2 = HaArray(er_sig2->leader_tasks, i);
    if( task1 != task2 )
      return KheTaskSolnIndex(task1) - KheTaskSolnIndex(task2);
  }
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventResourceSigCmp(const void *t1, const void *t2)               */
/*                                                                           */
/*  Untyped comparison function for bringing equal event resource signatures */
/*  together.                                                                */
/*                                                                           */
/*****************************************************************************/

/* *** unused
static int KheEventResourceSigCmp(const void *t1, const void *t2)
{
  KHE_EVENT_RESOURCE_SIG er_sig1 = * (KHE_EVENT_RESOURCE_SIG *) t1;
  KHE_EVENT_RESOURCE_SIG er_sig2 = * (KHE_EVENT_RESOURCE_SIG *) t2;
  return KheEventResourceSigTypedCmp(er_sig1, er_sig2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_EVENT_RESOURCE_CLASS"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE_CLASS KheEventResourceClassMake(KHE_EVENT_RESOURCE er,*/
/*    KHE_SOLN soln, HA_ARENA a)                                             */
/*                                                                           */
/*  Make an event resource class containing er.                              */
/*                                                                           */
/*****************************************************************************/

static KHE_EVENT_RESOURCE_CLASS KheEventResourceClassMake(KHE_EVENT_RESOURCE er,
  KHE_SOLN soln, HA_ARENA a)
{
  KHE_EVENT_RESOURCE_CLASS res;
  HaMake(res, a);
  HaArrayInit(res->event_resources, a);
  HaArrayAddLast(res->event_resources, er);
  res->sig = KheEventResourceSigMake(er, soln, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventResourceClassTypedCmp(KHE_EVENT_RESOURCE_CLASS erc1,         */
/*    KHE_EVENT_RESOURCE_CLASS erc2)                                         */
/*                                                                           */
/*  Typed comparison function for bringing event resource classes with       */
/*  equal signatures together.                                               */
/*                                                                           */
/*****************************************************************************/

static int KheEventResourceClassTypedCmp(KHE_EVENT_RESOURCE_CLASS erc1,
  KHE_EVENT_RESOURCE_CLASS erc2)
{
  return KheEventResourceSigTypedCmp(erc1->sig, erc2->sig);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventResourceClassCmp(const void *t1, const void *t2)             */
/*                                                                           */
/*  Untyped comparison function for bringing event resource classes with     */
/*  equal signatures together.                                               */
/*                                                                           */
/*****************************************************************************/

static int KheEventResourceClassCmp(const void *t1, const void *t2)
{
  KHE_EVENT_RESOURCE_CLASS erc1 = * (KHE_EVENT_RESOURCE_CLASS *) t1;
  KHE_EVENT_RESOURCE_CLASS erc2 = * (KHE_EVENT_RESOURCE_CLASS *) t2;
  return KheEventResourceClassTypedCmp(erc1, erc2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventResourceCmp(const void *t1, const void *t2)                  */
/*                                                                           */
/*  Comparison function for sorting an array of event resources.             */
/*                                                                           */
/*****************************************************************************/

static int KheEventResourceCmp(const void *t1, const void *t2)
{
  KHE_EVENT_RESOURCE er1 = * (KHE_EVENT_RESOURCE *) t1;
  KHE_EVENT_RESOURCE er2 = * (KHE_EVENT_RESOURCE *) t2;
  return KheEventResourceInstanceIndex(er1)-KheEventResourceInstanceIndex(er2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceClassDebug(KHE_EVENT_RESOURCE_CLASS erc,            */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of erc onto fp with the given indent.                        */
/*                                                                           */
/*****************************************************************************/

static void KheEventResourceClassDebug(KHE_EVENT_RESOURCE_CLASS erc,
  int indent, FILE *fp)
{
  KHE_EVENT_RESOURCE er;  int i;
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "{");
  HaArrayForEach(erc->event_resources, er, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    fprintf(fp, "%s", KheEventName(KheEventResourceEvent(er)));
  }
  fprintf(fp, "}");
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_RESOURCE_SIG"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheEventCmp(const void *t1, const void *t2)                          */
/*                                                                           */
/*  Comparison function for sorting an array of events by event index.       */
/*                                                                           */
/*****************************************************************************/

static int KheEventCmp(const void *t1, const void *t2)
{
  KHE_EVENT e1 = * (KHE_EVENT *) t1;
  KHE_EVENT e2 = * (KHE_EVENT *) t2;
  return KheEventIndex(e1) - KheEventIndex(e2);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SIG KheResourceSigMake(KHE_RESOURCE r,                      */
/*    KHE_CORRELATION_SOLVER cs)                                             */
/*                                                                           */
/*  Make the resource signature for r.                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_SIG KheResourceSigMake(KHE_RESOURCE r,
  KHE_CORRELATION_SOLVER cs)
{
  KHE_RESOURCE_SIG res;  KHE_EVENT e;  KHE_EVENT_CLASS ec;  int i;

  /* make the basic object */
  HaMake(res, cs->arena);
  HaArrayInit(res->events, cs->arena);

  /* add uniqueified events */
  for( i = 0;  i < KheResourceLayerEventCount(r);  i++ )
  {
    e = KheResourceLayerEvent(r, i);
    ec = KheEventToEventClass(e, cs);
    e = HaArrayFirst(ec->events);
    HaArrayAddLast(res->events, e);
  }

  /* sort events by increasing event index */
  HaArraySort(res->events, &KheEventCmp);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceClassLayerTypedCmp(KHE_RESOURCE_CLASS_LAYER rcl1,         */
/*    KHE_RESOURCE_CLASS_LAYER rcl2)                                         */
/*                                                                           */
/*  Typed comparison function for sorting an array of resource class         */
/*  layers to bring elements with equal layers together.                     */
/*                                                                           */
/*****************************************************************************/

static int KheResourceSigTypedCmp(KHE_RESOURCE_SIG rs1,
  KHE_RESOURCE_SIG rs2)
{
  int count1, count2, i;  KHE_EVENT e1, e2;
  count1 = HaArrayCount(rs1->events);
  count2 = HaArrayCount(rs2->events);
  if( count1 != count2 )
    return count1 - count2;
  for( i = 0;  i < count1;  i++ )
  {
    e1 = HaArray(rs1->events, i);
    e2 = HaArray(rs2->events, i);
    if( e1 != e2 )
      return KheEventIndex(e1) - KheEventIndex(e2);
  }
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceSigCmp(const void *t1, const void *t2)                    */
/*                                                                           */
/*  Untyped comparison function for sorting an array of resource sigs to     */
/*  to bring equal elements together.                                        */
/*                                                                           */
/*****************************************************************************/

/* *** not used
static int KheResourceSigCmp(const void *t1, const void *t2)
{
  KHE_RESOURCE_SIG rs1 = * (KHE_RESOURCE_SIG *) t1;
  KHE_RESOURCE_SIG rs2 = * (KHE_RESOURCE_SIG *) t2;
  return KheResourceSigTypedCmp(rs1, rs2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_RESOURCE_CLASS"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_CLASS_LAYER KheResourceClassLayerMake(KHE_RESOURCE r,       */
/*    KHE_CORRELATION_SOLVER cs)                                             */
/*                                                                           */
/*  Make a new resource class layer for r, using cs for uniqueifying         */
/*  linked events.                                                           */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_CLASS KheResourceClassMake(KHE_RESOURCE r,
  KHE_CORRELATION_SOLVER cs)
{
  KHE_RESOURCE_CLASS res;
  HaMake(res, cs->arena);
  HaArrayInit(res->resources, cs->arena);
  HaArrayAddLast(res->resources, r);
  res->sig = KheResourceSigMake(r, cs);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceClassTypedCmp(KHE_RESOURCE_CLASS rc1,                     */
/*    KHE_RESOURCE_CLASS rc2)                                                */
/*                                                                           */
/*  Typed comparison function for bringing resource classes with equal       */
/*  signatures together.                                                     */
/*                                                                           */
/*****************************************************************************/

static int KheResourceClassTypedCmp(KHE_RESOURCE_CLASS rc1,
  KHE_RESOURCE_CLASS rc2)
{
  return KheResourceSigTypedCmp(rc1->sig, rc2->sig);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceClassCmp(const void *t1, const void *t2)                  */
/*                                                                           */
/*  Untyped comparison function for bringing resource classes with equal     */
/*  signatures together.                                                     */
/*                                                                           */
/*****************************************************************************/

static int KheResourceClassCmp(const void *t1, const void *t2)
{
  KHE_RESOURCE_CLASS rc1 = * (KHE_RESOURCE_CLASS *) t1;
  KHE_RESOURCE_CLASS rc2 = * (KHE_RESOURCE_CLASS *) t2;
  return KheResourceClassTypedCmp(rc1, rc2);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_CORRELATION_SOLVER"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CORRELATION_SOLVER KheCorrelationSolverMake()                         */
/*                                                                           */
/*  Make a new event class solver.                                           */
/*                                                                           */
/*****************************************************************************/

static KHE_CORRELATION_SOLVER KheCorrelationSolverMake(KHE_SOLN soln,
  HA_ARENA a)
{
  KHE_CORRELATION_SOLVER res;
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  HaArrayInit(res->tmp_event_classes, a);
  HaArrayInit(res->all_event_classes, a);
  HaArrayInit(res->event_classes_by_event, a);
  HaArrayInit(res->tmp_event_resource_classes, a);
  HaArrayInit(res->all_event_resource_classes, a);
  HaArrayInit(res->tmp_resource_classes, a);
  HaArrayInit(res->all_resource_classes, a);
  HaArrayInit(res->tmp_monitors, a);
  HaArrayInit(res->tmp_monitors2, a);
  HaArrayInit(res->monitor_and_classes_array, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheSolnAddMonitorGroups"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAddMonitorGroup(KHE_SOLN soln, ARRAY_KHE_MONITOR *monitors,  */
/*    int first, int last, KHE_SUBTAG_STANDARD_TYPE sub_tag, bool debug)     */
/*                                                                           */
/*  Link *monitors[first..last] to soln as a group.  If there are no         */
/*  monitors in the range first .. last inclusive, do nothing; if there      */
/*  is one monitor, link it directly as a child of soln; if there are two    */
/*  or more monitors, make a group monitor with the given sub-tag, link it   */
/*  to soln, and make *monitors[first..last] its children.                   */
/*                                                                           */
/*  The idea is that *monitors will all be unlinked from soln when this      */
/*  function is called.  But that is the caller's responsibility.            */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheSolnAddMonitorGroups below
static void KheSolnAddMonitorGroup(KHE_SOLN soln, ARRAY_KHE_MONITOR *monitors,
  int first, int last, KHE_SUBTAG_STANDARD_TYPE sub_tag, bool debug)
{
  KHE_MONITOR m;  int i, len;  KHE_GROUP_MONITOR gm;
  len = last - first + 1;
  if( len == 1 )
  {
    m = HaArray(*monitors, first);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, m);
    if( debug )
      KheMonitorDebug(m, 2, 2, stderr);
  }
  else if( len >= 2 )
  {
    gm = KheGroupMonitorMake(soln, sub_tag, KheSubTagLabel(sub_tag));
    for( i = first;  i <= last;  i++ )
    {
      m = HaArray(*monitors, i);
      KheGroupMonitorAddChildMonitor(gm, m);
    }
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) gm);
    if( debug )
      KheMonitorDebug((KHE_MONITOR) gm, 2, 2, stderr);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorParentsTypedCmp(KHE_MONITOR m1, KHE_MONITOR m2)            */
/*                                                                           */
/*  Typed comparison function for bringing together monitors with the        */
/*  same parents, assuming that the parents have been sorted by increasing   */
/*  solution index (as done by KheMonitorParentMonitorsSort) first.          */
/*                                                                           */
/*****************************************************************************/

static int KheMonitorParentsTypedCmp(KHE_MONITOR m1, KHE_MONITOR m2)
{
  int count1, count2, i, index1, index2;
  count1 = KheMonitorParentMonitorCount(m1);
  count2 = KheMonitorParentMonitorCount(m2);
  if( count1 != count2 )
    return count1 - count2;
  for( i = 0;  i < count1;  i++ )
  {
    index1 = KheMonitorSolnIndex((KHE_MONITOR) KheMonitorParentMonitor(m1, i));
    index2 = KheMonitorSolnIndex((KHE_MONITOR) KheMonitorParentMonitor(m2, i));
    if( index1 != index2 )
      return index1 - index2;
  }
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorParentsCmp(const void *t1, const void *t2)                 */
/*                                                                           */
/*  Untyped comparison function for bringing together monitors with the      */
/*  same parents, assuming that the parents have been sorted by increasing   */
/*  solution index (as done by KheMonitorParentMonitorsSort) first.          */
/*                                                                           */
/*****************************************************************************/

static int KheMonitorParentsCmp(const void *t1, const void *t2)
{
  KHE_MONITOR m1 = * (KHE_MONITOR *) t1;
  KHE_MONITOR m2 = * (KHE_MONITOR *) t2;
  return KheMonitorParentsTypedCmp(m1, m2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnAddMonitorGroups(KHE_SOLN soln, ARRAY_KHE_MONITOR *monitors, */
/*    KHE_SUBTAG_STANDARD_TYPE sub_tag)                                      */
/*                                                                           */
/*  Sort the parents of the monitors into a standard order, then sort the    */
/*  monitors so that those with the same parents are brought together.       */
/*  For each run of at least 2 monitors with the same parents, replace       */
/*  the monitors under each parent by a group monitor.                       */
/*                                                                           */
/*****************************************************************************/

static void KheSolnAddMonitorGroups(KHE_SOLN soln, ARRAY_KHE_MONITOR *monitors,
  KHE_SUBTAG_STANDARD_TYPE sub_tag)
{
  KHE_MONITOR m, m2;  int i, j, k;  KHE_GROUP_MONITOR gm, gm2;

  /* sort each monitor's parents */
  HaArrayForEach(*monitors, m, i)
    KheMonitorParentMonitorsSort(m);

  /* sort the monitors to bring together monitors with the same parents */
  HaArraySort(*monitors, &KheMonitorParentsCmp);

  /* find each run of monitors with the same parents and group them */
  for( i = 0;  i < HaArrayCount(*monitors);  i = j )
  {
    m = HaArray(*monitors, i);
    for( j = i + 1;  j < HaArrayCount(*monitors);  j++ )
    {
      m2 = HaArray(*monitors, j);
      if( KheMonitorParentsTypedCmp(m, m2) != 0 )
	break;
    }

    /* group monitors[i ... j - 1] if there are at least two of them */
    if( j - i >= 2 )
    {
      /* make gm and make it a child of each of m's parents */
      gm = KheGroupMonitorMake(soln, sub_tag, KheSubTagLabel(sub_tag));
      for( k = 0;  k < KheMonitorParentMonitorCount(m);  k++ )
      {
	gm2 = KheMonitorParentMonitor(m, k);
	KheGroupMonitorAddChildMonitor(gm2, (KHE_MONITOR) gm);
      }

      /* remove all the parents of the grouped monitors and add gm as parent */
      for( k = i;  k < j;  k++ )
      {
	m2 = HaArray(*monitors, k);
	while( KheMonitorParentMonitorCount(m2) > 0 )
	  KheGroupMonitorDeleteChildMonitor(KheMonitorParentMonitor(m2, 0), m2);
        KheGroupMonitorAddChildMonitor(gm, m2);
      }

      if( DEBUG13 )
	KheGroupMonitorDebug(gm, 2, 2, stderr);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "grouping correlated event monitors"                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAddEventMonitors(KHE_SOLN soln, KHE_EVENT e,                     */
/*    KHE_MONITOR_TAG tag1, KHE_MONITOR_TAG tag2,                            */
/*    ARRAY_KHE_MONITOR *monitors)                                           */
/*                                                                           */
/*  Add the event monitors with tag tag1 or tag2 that monitor e in soln      */
/*  to *monitors.                                                            */
/*                                                                           */
/*****************************************************************************/

static void KheAddEventMonitors(KHE_SOLN soln, KHE_EVENT e,
  KHE_MONITOR_TAG tag1, KHE_MONITOR_TAG tag2, ARRAY_KHE_MONITOR *monitors)
{
  int i;  KHE_MONITOR m;
  for( i = 0;  i < KheSolnEventMonitorCount(soln, e);  i++ )
  {
    m = KheSolnEventMonitor(soln, e, i);
    if( KheMonitorTag(m) == tag1 || KheMonitorTag(m) == tag2 )
      HaArrayAddLast(*monitors, m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorCmp(const void *t1, const void *t2)                        */
/*                                                                           */
/*  Comparison function for sorting monitors by increasing solution index.   */
/*                                                                           */
/*****************************************************************************/

static int KheMonitorCmp(const void *t1, const void *t2)
{
  KHE_MONITOR m1 = * (KHE_MONITOR *) t1;
  KHE_MONITOR m2 = * (KHE_MONITOR *) t2;
  return KheMonitorSolnIndex(m1) - KheMonitorSolnIndex(m2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventClassGetMonitors(KHE_EVENT_CLASS ec, KHE_SOLN soln,         */
/*    KHE_MONITOR_TAG tag1, KHE_MONITOR_TAG tag2,                            */
/*    ARRAY_KHE_MONITOR *monitors)                                           */
/*                                                                           */
/*  Set *monitors to the event monitors in soln with the given tag1 or tag2  */
/*  that monitor the events of ec.                                           */
/*                                                                           */
/*****************************************************************************/

static void KheEventClassGetMonitors(KHE_EVENT_CLASS ec, KHE_SOLN soln,
  KHE_MONITOR_TAG tag1, KHE_MONITOR_TAG tag2, ARRAY_KHE_MONITOR *monitors)
{
  KHE_EVENT e;  int i;
  HaArrayClear(*monitors);
  HaArrayForEach(ec->events, e, i)
    KheAddEventMonitors(soln, e, tag1, tag2, monitors);
  HaArraySortUnique(*monitors, &KheMonitorCmp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnGetMonitors(KHE_SOLN soln, KHE_MONITOR_TAG tag,              */
/*    ARRAY_KHE_MONITOR *monitors)                                           */
/*                                                                           */
/*  Set *monitors to the event monitors in soln with the given tag.          */
/*                                                                           */
/*****************************************************************************/

static void KheSolnGetMonitors(KHE_SOLN soln, KHE_MONITOR_TAG tag,
  ARRAY_KHE_MONITOR *monitors)
{
  KHE_EVENT e;  KHE_INSTANCE ins;  int i;
  HaArrayClear(*monitors);
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    KheAddEventMonitors(soln, e, tag, tag, monitors);
  }
  HaArraySortUnique(*monitors, &KheMonitorCmp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupSplitMonitors(KHE_CORRELATION_SOLVER cs,                    */
/*    KHE_EVENT_CLASS ec)                                                    */
/*                                                                           */
/*  Group the split events and distribute split events monitors of ec.       */
/*                                                                           */
/*****************************************************************************/

static void KheGroupSplitMonitors(KHE_CORRELATION_SOLVER cs,
  KHE_EVENT_CLASS ec)
{
  /* gather the monitors */
  KheEventClassGetMonitors(ec, cs->soln, KHE_SPLIT_EVENTS_MONITOR_TAG,
    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG, &cs->tmp_monitors);

  /* build group monitors for the gathered monitors */
  KheSolnAddMonitorGroups(cs->soln, &cs->tmp_monitors, KHE_SUBTAG_SPLIT_EVENTS);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupAssignTimeMonitors(KHE_CORRELATION_SOLVER cs,               */
/*    KHE_EVENT_CLASS ec)                                                    */
/*                                                                           */
/*  Group the assign time monitors of ec.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheGroupAssignTimeMonitors(KHE_CORRELATION_SOLVER cs,
  KHE_EVENT_CLASS ec)
{
  /* gather the monitors */
  KheEventClassGetMonitors(ec, cs->soln, KHE_ASSIGN_TIME_MONITOR_TAG,
    KHE_ASSIGN_TIME_MONITOR_TAG, &cs->tmp_monitors);

  /* build group monitors for the gathered monitors */
  KheSolnAddMonitorGroups(cs->soln, &cs->tmp_monitors, KHE_SUBTAG_ASSIGN_TIME);
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferTimesMonitorTypedCmp(KHE_PREFER_TIMES_MONITOR m1,           */
/*    KHE_PREFER_TIMES_MONITOR m2)                                           */
/*                                                                           */
/*  Typed comparison function for sorting an array of prefer times monitors  */
/*  so as to bring together monitors which request the same set of times.    */
/*                                                                           */
/*****************************************************************************/

static int KhePreferTimesMonitorTypedCmp(KHE_PREFER_TIMES_MONITOR m1,
  KHE_PREFER_TIMES_MONITOR m2)
{
  KHE_PREFER_TIMES_CONSTRAINT c1 = KhePreferTimesMonitorConstraint(m1);
  KHE_PREFER_TIMES_CONSTRAINT c2 = KhePreferTimesMonitorConstraint(m2);
  KHE_TIME_GROUP tg1 = KhePreferTimesConstraintDomain(c1);
  KHE_TIME_GROUP tg2 = KhePreferTimesConstraintDomain(c2);
  return KheTimeGroupTypedCmp(tg1, tg2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferTimesMonitorCmp(const void *t1, const void *t2)             */
/*                                                                           */
/*  Untyped comparison function for sorting an array of prefer times         */
/*  monitors to bring together monitors which request the same set of times. */
/*                                                                           */
/*****************************************************************************/

static int KhePreferTimesMonitorCmp(const void *t1, const void *t2)
{
  KHE_PREFER_TIMES_MONITOR m1 = * (KHE_PREFER_TIMES_MONITOR *) t1;
  KHE_PREFER_TIMES_MONITOR m2 = * (KHE_PREFER_TIMES_MONITOR *) t2;
  return KhePreferTimesMonitorTypedCmp(m1, m2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupPreferTimesMonitors(KHE_CORRELATION_SOLVER cs,              */
/*    KHE_EVENT_CLASS ec)                                                    */
/*                                                                           */
/*  Group those prefer times monitors of ec that request the same times.     */
/*                                                                           */
/*****************************************************************************/

static void KheGroupPreferTimesMonitors(KHE_CORRELATION_SOLVER cs,
  KHE_EVENT_CLASS ec)
{
  int i, j;  KHE_MONITOR m, m2;
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheGroupPreferTimesMonitors(cs, ");
    KheEventClassDebug(ec, -1, stderr);
    fprintf(stderr, ")\n");
  }

  /* gather the monitors */
  KheEventClassGetMonitors(ec, cs->soln, KHE_PREFER_TIMES_MONITOR_TAG,
    KHE_PREFER_TIMES_MONITOR_TAG, &cs->tmp_monitors);

  /* sort the monitors to bring equal time groups together */
  HaArraySort(cs->tmp_monitors, &KhePreferTimesMonitorCmp);

  /* visit each run of monitors with equal time groups and group them */
  for( i = 0;  i < HaArrayCount(cs->tmp_monitors);  i = j )
  {
    m = HaArray(cs->tmp_monitors, i);
    HaArrayClear(cs->tmp_monitors2);
    HaArrayAddLast(cs->tmp_monitors2, m);
    for( j = i + 1;  j < HaArrayCount(cs->tmp_monitors);  j++ )
    {
      m2 = HaArray(cs->tmp_monitors, j);
      if( KhePreferTimesMonitorTypedCmp((KHE_PREFER_TIMES_MONITOR) m,
	  (KHE_PREFER_TIMES_MONITOR) m2) != 0 )
	break;
      HaArrayAddLast(cs->tmp_monitors2, m2);
    }
    KheSolnAddMonitorGroups(cs->soln, &cs->tmp_monitors2,
      KHE_SUBTAG_PREFER_TIMES);
  }

  if( DEBUG2 )
    fprintf(stderr, "] KheGroupPreferTimesMonitors returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorAndClassesArray(KHE_CORRELATION_SOLVER cs,           */
/*    KHE_SUBTAG_STANDARD_TYPE sub_tag)                                      */
/*                                                                           */
/*  Group the monitors of cs->monitor_and_classes_array so that those        */
/*  that have the same event classes are grouped under a group monitor       */
/*  with sub-tag sub_tag.                                                    */
/*                                                                           */
/*****************************************************************************/

static void KheGroupMonitorAndClassesArray(KHE_CORRELATION_SOLVER cs,
  KHE_SUBTAG_STANDARD_TYPE sub_tag)
{
  int i, j;  KHE_MONITOR_AND_CLASSES mac, first_mac;

  /* sort monitor_and_classes_array to bring equal sets of classes together */
  HaArraySort(cs->monitor_and_classes_array, &KheMonitorAndClassesCmp);

  /* group adjacent monitor objects with equal sets of event classes */
  for( i = 0;  i < HaArrayCount(cs->monitor_and_classes_array);  i = j )
  {
    /* set cs->tmp_monitors to a maximum run of groupable macs starting at i */
    HaArrayClear(cs->tmp_monitors);
    first_mac = HaArray(cs->monitor_and_classes_array, i);
    for( j = i + 1;  j < HaArrayCount(cs->monitor_and_classes_array);  j++ )
    {
      mac = HaArray(cs->monitor_and_classes_array, j);
      if( KheMonitorAndClassesTypedCmp(first_mac, mac) != 0 )
	break;
      HaArrayAddLast(cs->tmp_monitors, mac->monitor);
    }

    /* do some grouping on this run of groupable monitors */
    KheSolnAddMonitorGroups(cs->soln, &cs->tmp_monitors, sub_tag);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupSpreadEventsMonitors(KHE_CORRELATION_SOLVER cs)             */
/*                                                                           */
/*  Group equivalent spread events monitors.                                 */
/*                                                                           */
/*****************************************************************************/

static void KheGroupSpreadEventsMonitors(KHE_CORRELATION_SOLVER cs)
{
  int i;  KHE_MONITOR m;  KHE_MONITOR_AND_CLASSES mac;
  if( DEBUG4 )
    fprintf(stderr, "[ KheGroupSpreadEventsMonitors(cs)\n");

  /* find all the spread events monitors */
  KheSolnGetMonitors(cs->soln, KHE_SPREAD_EVENTS_MONITOR_TAG,
    &cs->tmp_monitors);

  /* build one monitor-and-classes object for each monitor */
  HaArrayClear(cs->monitor_and_classes_array);
  HaArrayForEach(cs->tmp_monitors, m, i)
  {
    mac = KheMonitorAndClassesSpreadMake((KHE_SPREAD_EVENTS_MONITOR) m, cs);
    HaArrayAddLast(cs->monitor_and_classes_array, mac);
  }

  /* group monitors with equal sets of event classes */
  KheGroupMonitorAndClassesArray(cs, KHE_SUBTAG_SPREAD_EVENTS);
  if( DEBUG4 )
    fprintf(stderr, "] KheGroupSpreadEventsMonitors returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupOrderEventsMonitors(KHE_CORRELATION_SOLVER cs)              */
/*                                                                           */
/*  Group equivalent order events monitors.                                  */
/*                                                                           */
/*  Implementation note.  This code is untested.  It is similar to the       */
/*  code for spread events monitors, the main differences being that it      */
/*  does not sort the event classes associated with each monitor, and        */
/*  that it ignores monitors whose two classes are equal.                    */
/*                                                                           */
/*****************************************************************************/

static void KheGroupOrderEventsMonitors(KHE_CORRELATION_SOLVER cs)
{
  int i;  KHE_MONITOR m;  KHE_MONITOR_AND_CLASSES mac;
  if( DEBUG5 )
    fprintf(stderr, "[ KheGroupOrderEventsMonitors(cs)\n");

  /* find all the order events monitors */
  KheSolnGetMonitors(cs->soln, KHE_ORDER_EVENTS_MONITOR_TAG,
    &cs->tmp_monitors);

  /* build one monitor-and-classes object for each monitor */
  HaArrayClear(cs->monitor_and_classes_array);
  HaArrayForEach(cs->tmp_monitors, m, i)
  {
    mac = KheMonitorAndClassesOrderMake((KHE_ORDER_EVENTS_MONITOR) m, cs);
    if( mac != NULL )
      HaArrayAddLast(cs->monitor_and_classes_array, mac);
  }

  /* group monitors with equal sets of event classes */
  KheGroupMonitorAndClassesArray(cs, KHE_SUBTAG_ORDER_EVENTS);
  if( DEBUG5 )
    fprintf(stderr, "] KheGroupOrderEventsMonitors returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupCorrelatedEventMonitors(KHE_CORRELATION_SOLVER cs)          */
/*                                                                           */
/*  Group the correlated event monitors of soln.                             */
/*                                                                           */
/*****************************************************************************/

static void KheGroupCorrelatedEventMonitors(KHE_CORRELATION_SOLVER cs)
{
  int i, j, k, index;  KHE_INSTANCE ins;  KHE_EVENT e;
  KHE_EVENT_CLASS ec, ec2;

  if( DEBUG1 )
    fprintf(stderr, "[ KheGroupCorrelatedEventMonitors(soln)\n");

  /* build an initial set of event classes, one per event */
  HaArrayClear(cs->tmp_event_classes);
  ins = KheSolnInstance(cs->soln);
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    ec = KheEventClassMake(e, cs->soln, cs->arena);
    HaArrayAddLast(cs->tmp_event_classes, ec);
  }

  /* sort the event classes and merge equivalent ones */
  HaArraySort(cs->tmp_event_classes, &KheEventClassCmp);
  HaArrayClear(cs->all_event_classes);
  for( i = 0;  i < HaArrayCount(cs->tmp_event_classes);  i = j )
  {
    ec = HaArray(cs->tmp_event_classes, i);
    for( j = i + 1;  j < HaArrayCount(cs->tmp_event_classes);  j++ )
    {
      ec2 = HaArray(cs->tmp_event_classes, j);
      if( KheEventClassTypedCmp(ec, ec2) != 0 )
	break;
      HaArrayAppend(ec->events, ec2->events, k);
    }
    HaArrayAddLast(cs->all_event_classes, ec);
  }
  HaArrayClear(cs->tmp_event_classes);

  /* sort the events of the final classes */
  /* *** no need for this, surely?
  HaArrayForEach(cs->all_event_classes, ec, i)
    HaArraySort(ec->events, &KheEventCmp);
  if( DEBUG2 )
  {
    fprintf(stderr, "  all event classes:\n");
    HaArrayForEach(cs->all_event_classes, ec, i)
      KheEventClassDebug(ec, 2, stderr);
  }
  *** */

  /* set the event_classes_by_event array */
  HaArrayClear(cs->event_classes_by_event);
  HaArrayForEach(cs->all_event_classes, ec, i)
    HaArrayForEach(ec->events, e, j)
    {
      index = KheEventIndex(e);
      HaArrayFill(cs->event_classes_by_event, index + 1, NULL);
      HaArrayPut(cs->event_classes_by_event, index, ec);
    }

  /* group split and distribute split, assign time, and prefer times monitors */
  HaArrayForEach(cs->all_event_classes, ec, i)
  {
    KheGroupSplitMonitors(cs, ec);
    KheGroupAssignTimeMonitors(cs, ec);
    KheGroupPreferTimesMonitors(cs, ec);
  }

  /* group spread events monitors and order events monitors */
  KheGroupSpreadEventsMonitors(cs);
  KheGroupOrderEventsMonitors(cs);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheUnGroupCorrelatedEventMonitors(KHE_SOLN soln)                    */
/*                                                                           */
/*  Remove the event monitors added by KheGroupCorrelatedEventMonitors.      */
/*                                                                           */
/*****************************************************************************/

static void KheUnGroupCorrelatedEventMonitors(KHE_SOLN soln)
{
  KHE_INSTANCE ins;  int i, j;  KHE_EVENT e;  KHE_MONITOR m;
  KHE_GROUP_MONITOR gm;
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    for( j = 0;  j < KheSolnEventMonitorCount(soln, e);  j++ )
    {
      m = KheSolnEventMonitor(soln, e, j);
      switch( KheMonitorTag(m) )
      {
	case KHE_SPLIT_EVENTS_MONITOR_TAG:
	case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

	  if( KheMonitorHasParent(m, KHE_SUBTAG_SPLIT_EVENTS, &gm) )
	    KheGroupMonitorBypassAndDelete(gm);
	  break;

	case KHE_ASSIGN_TIME_MONITOR_TAG:

	  if( KheMonitorHasParent(m, KHE_SUBTAG_ASSIGN_TIME, &gm) )
	    KheGroupMonitorBypassAndDelete(gm);
	  break;

	case KHE_PREFER_TIMES_MONITOR_TAG:

	  if( KheMonitorHasParent(m, KHE_SUBTAG_PREFER_TIMES, &gm) )
	    KheGroupMonitorBypassAndDelete(gm);
	  break;

	case KHE_SPREAD_EVENTS_MONITOR_TAG:

	  if( KheMonitorHasParent(m, KHE_SUBTAG_SPREAD_EVENTS, &gm) )
	    KheGroupMonitorBypassAndDelete(gm);
	  break;

	case KHE_ORDER_EVENTS_MONITOR_TAG:

	  if( KheMonitorHasParent(m, KHE_SUBTAG_ORDER_EVENTS, &gm) )
	    KheGroupMonitorBypassAndDelete(gm);
	  break;

	default:

	  /* not interested */
	  break;
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "grouping correlated event resource monitors"                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAddEventResourceMonitors(KHE_SOLN soln, KHE_EVENT_RESOURCE er,   */
/*    KHE_MONITOR_TAG tag, ARRAY_KHE_MONITOR *monitors)                      */
/*                                                                           */
/*  Add the event resource monitors of type tag that monitor er in soln      */
/*  to *monitors.                                                            */
/*                                                                           */
/*****************************************************************************/

static void KheAddEventResourceMonitors(KHE_SOLN soln, KHE_EVENT_RESOURCE er,
  KHE_MONITOR_TAG tag, ARRAY_KHE_MONITOR *monitors)
{
  int i;  KHE_MONITOR m;
  for( i = 0;  i < KheSolnEventResourceMonitorCount(soln, er);  i++ )
  {
    m = KheSolnEventResourceMonitor(soln, er, i);
    if( KheMonitorTag(m) == tag )
      HaArrayAddLast(*monitors, m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceClassGetMonitors(KHE_EVENT_RESOURCE_CLASS erc,      */
/*    KHE_SOLN soln, KHE_MONITOR_TAG tag, ARRAY_KHE_MONITOR *monitors)       */
/*                                                                           */
/*  Set *monitors to the event resource monitors in soln with the given      */
/*  tag that monitor the event resources of erc.                             */
/*                                                                           */
/*****************************************************************************/

static void KheEventResourceClassGetMonitors(KHE_EVENT_RESOURCE_CLASS erc,
  KHE_SOLN soln, KHE_MONITOR_TAG tag, ARRAY_KHE_MONITOR *monitors)
{
  KHE_EVENT_RESOURCE er;  int i;
  HaArrayClear(*monitors);
  HaArrayForEach(erc->event_resources, er, i)
    KheAddEventResourceMonitors(soln, er, tag, monitors);
  HaArraySortUnique(*monitors, &KheMonitorCmp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupAssignResourceMonitors(KHE_CORRELATION_SOLVER cs,           */
/*    KHE_EVENT_RESOURCE_CLASS erc)                                          */
/*                                                                           */
/*  Group together the assign resource monitors of erc.                      */
/*                                                                           */
/*****************************************************************************/

static void KheGroupAssignResourceMonitors(KHE_CORRELATION_SOLVER cs,
  KHE_EVENT_RESOURCE_CLASS erc)
{
  if( DEBUG8 )
  {
    fprintf(stderr, "[ KheGroupAssignResourceMonitors(cs, ");
    KheEventResourceClassDebug(erc, -1, stderr);
    fprintf(stderr, ")\n");
  }

  /* gather the monitors */
  KheEventResourceClassGetMonitors(erc, cs->soln,
    KHE_ASSIGN_RESOURCE_MONITOR_TAG, &cs->tmp_monitors);

  /* build group monitors for the gathered monitors */
  KheSolnAddMonitorGroups(cs->soln, &cs->tmp_monitors,
    KHE_SUBTAG_ASSIGN_RESOURCE);
  if( DEBUG8 )
    fprintf(stderr, "] KheGroupAssignResourceMonitors returning\n");
}


/* *** old version, not really comprehensible
static void KheGroupAssignResourceMonitors(
  KHE_CORRELATION_SOLVER ercs, KHE_EVENT_RESOURCE_CLASS erc)
{
  KHE_EVENT_RESOURCE er;  int i, j, k;  KHE_MONITOR m;
  if( DEBUG8 )
  {
    fprintf(stderr, "[ KheGroupAssignResourceMonitors(ercs, ");
    KheEventResourceClassDebug(erc, -1, stderr);
    fprintf(stderr, ")\n");
  }

  ** for each monitor of erc with the given tag, remove parents, then if **
  ** unattached make it a child of soln, and if attached save in tmp_monitors **
  HaArrayClear(ercs->tmp_monitors);
  HaArrayForEach(erc->event_resources, er, i)
  {
    for( j = 0;  j < KheSolnEventResourceMonitorCount(ercs->soln, er);  j++ )
    {
      m = KheSolnEventResourceMonitor(ercs->soln, er, j);
      if( KheMonitorTag(m) == KHE_ASSIGN_RESOURCE_MONITOR_TAG )
      {
        ** KheMonitorDeleteAllParentsRecursive(m); **
	if( !KheMonitorAttachedToSoln(m) )
          KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) ercs->soln, m);
	else if( !HaArrayContains(ercs->tmp_monitors, m, &k) )
	  HaArrayAddLast(ercs->tmp_monitors, m);
      }
    }
  }

  KheSolnAddMonitorGroups(ercs->soln, &ercs->tmp_monitors,
    KHE_SUBTAG_ASSIGN_RESOURCE);
  if( DEBUG8 )
    fprintf(stderr, "] KheGroupAssignResourceMonitors returning\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferResourcesMonitorTypedCmp(KHE_PREFER_RESOURCES_MONITOR m1,   */
/*    KHE_PREFER_RESOURCES_MONITOR m2)                                       */
/*                                                                           */
/*  Typed comparison function for sorting an array of prefer resources       */
/*  monitors to bring together monitors which request the same resources.    */
/*                                                                           */
/*****************************************************************************/

static int KhePreferResourcesMonitorTypedCmp(KHE_PREFER_RESOURCES_MONITOR m1,
  KHE_PREFER_RESOURCES_MONITOR m2)
{
  KHE_RESOURCE_GROUP rg1 = KhePreferResourcesMonitorDomain(m1);
  KHE_RESOURCE_GROUP rg2 = KhePreferResourcesMonitorDomain(m2);
  return KheResourceGroupTypedCmp(rg1, rg2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferResourcesMonitorCmp(const void *t1, const void *t2)         */
/*                                                                           */
/*  Untyped comparison function for sorting an array of prefer resources     */
/*  monitors to bring together monitors which request the same resources.    */
/*                                                                           */
/*****************************************************************************/

static int KhePreferResourcesMonitorCmp(const void *t1, const void *t2)
{
  KHE_PREFER_RESOURCES_MONITOR m1 = * (KHE_PREFER_RESOURCES_MONITOR *) t1;
  KHE_PREFER_RESOURCES_MONITOR m2 = * (KHE_PREFER_RESOURCES_MONITOR *) t2;
  return KhePreferResourcesMonitorTypedCmp(m1, m2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupPreferResourcesMonitors(KHE_CORRELATION_SOLVER cs,          */
/*    KHE_EVENT_RESOURCE_CLASS erc)                                          */
/*                                                                           */
/*  Group together those prefer resources monitors of erc which request      */
/*  the same resources.                                                      */
/*                                                                           */
/*****************************************************************************/

static void KheGroupPreferResourcesMonitors(KHE_CORRELATION_SOLVER cs,
  KHE_EVENT_RESOURCE_CLASS erc)
{
  int i, j;  KHE_MONITOR m, m2;
  if( DEBUG8 )
  {
    fprintf(stderr, "[ KheGroupPreferResourcesMonitors(cs, ");
    KheEventResourceClassDebug(erc, -1, stderr);
    fprintf(stderr, ")\n");
  }

  /* gather the monitors */
  KheEventResourceClassGetMonitors(erc, cs->soln,
    KHE_PREFER_RESOURCES_MONITOR_TAG, &cs->tmp_monitors);

  /* sort cs->tmp_monitors to bring equal resource groups together */
  HaArraySort(cs->tmp_monitors, &KhePreferResourcesMonitorCmp);

  /* visit each run of monitors with equal resource groups and group them */
  for( i = 0;  i < HaArrayCount(cs->tmp_monitors);  i = j )
  {
    m = HaArray(cs->tmp_monitors, i);
    HaArrayClear(cs->tmp_monitors2);
    HaArrayAddLast(cs->tmp_monitors2, m);
    for( j = i + 1;  j < HaArrayCount(cs->tmp_monitors);  j++ )
    {
      m2 = HaArray(cs->tmp_monitors, j);
      if( KhePreferResourcesMonitorTypedCmp((KHE_PREFER_RESOURCES_MONITOR) m,
	    (KHE_PREFER_RESOURCES_MONITOR) m2) != 0 )
	break;
      HaArrayAddLast(cs->tmp_monitors2, m2);
    }
    KheSolnAddMonitorGroups(cs->soln, &cs->tmp_monitors2,
      KHE_SUBTAG_PREFER_RESOURCES);
  }
  if( DEBUG8 )
    fprintf(stderr, "] KheGroupPreferResourcesMonitors returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupLimitResourcesMonitors(                                     */
/*    KHE_CORRELATION_SOLVER ercs,                                           */
/*    KHE_EVENT e, KHE_RESOURCE_TYPE rt)                                     */
/*                                                                           */
/*  Group together the limit resources monitors of type rt in e.             */
/*                                                                           */
/*****************************************************************************/

/* *** not grouping limit resources monitors now
static void KheGroupLimitResourcesMonitors(
  KHE_CORRELATION_SOLVER ercs,
  KHE_EVENT e, KHE_RESOURCE_TYPE rt)
{
  KHE_EVENT_RESOURCE er;  int i, j, k;  KHE_MONITOR m;
  if( DEBUG11 )
  {
    fprintf(stderr, "[ KheGroupLimitResourcesMonitors(ercs, ");
    KheEventDebug(e, 1, -1, stderr);
    fprintf(stderr, ", %s)\n", KheResourceTypeId(rt));
  }

  ** for each limit resources monitor, remove parents, then if unattached **
  ** make it a child of soln, and if attached save in tmp_monitors **
  HaArrayClear(ercs->tmp_monitors);
  for( i = 0;  i < KheEventResourceCount(e);  i++ )
  {
    er = KheEventResource(e, i);
    if( rt == NULL || KheEventResourceResourceType(er) == rt )
      for( j = 0;  j < KheSolnEventResourceMonitorCount(ercs->soln, er);  j++ )
      {
	m = KheSolnEventResourceMonitor(ercs->soln, er, j);
	if( KheMonitorTag(m) == KHE_LIMIT_RESOURCES_MONITOR_TAG )
	{
	  ** KheMonitorDeleteAllParentsRecursive(m); **
	  if( !KheMonitorAttachedToSoln(m) )
	    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) ercs->soln, m);
	  else if( !HaArrayContains(ercs->tmp_monitors, m, &k) )
	    HaArrayAddLast(ercs->tmp_monitors, m);
	}
      }
  }

  KheSolnAddMonitorGroups(ercs->soln, &ercs->tmp_monitors,
    KHE_SUBTAG_LIMIT_RESOURCES);
  if( DEBUG11 )
    fprintf(stderr, "] KheGroupLimitResourcesMonitors returning\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeGroupDoLimitResourcesGroup(                                  */
/*    KHE_CORRELATION_SOLVER ercs, KHE_TIME_GROUP tg,                        */
/*    KHE_RESOURCE_TYPE rt, KHE_EVENT_TIMETABLE_MONITOR etm)                 */
/*                                                                           */
/*  Group together the limit resources monitors of type rt that              */
/*  monitor event resources running during tg.                               */
/*                                                                           */
/*****************************************************************************/

/* *** not grouping limit resources monitors now
static void KheTimeGroupDoLimitResourcesGroup(
  KHE_CORRELATION_SOLVER ercs, KHE_TIME_GROUP tg,
  KHE_RESOURCE_TYPE rt, KHE_EVENT_TIMETABLE_MONITOR etm)
{
  KHE_TIME t;  int i, j, k, n, junk;  KHE_MEET meet;
  KHE_EVENT e;  KHE_MONITOR m;  KHE_EVENT_RESOURCE er;
  HaArrayClear(ercs->tmp_monitors);
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    for( j = 0;  j < KheEventTimetableMonitorTimeMeetCount(etm, t);  j++ )
    {
      meet = KheEventTimetableMonitorTimeMeet(etm, t, j);
      e = KheMeetEvent(meet);
      if( e != NULL && KheMeetAsstTime(meet) == t )
      {
	for( k = 0;  k < KheEventResourceCount(e);  k++ )
	{
	  er = KheEventResource(e, k);
	  if( rt == NULL || KheEventResourceResourceType(er) == rt )
	    for( n=0; n < KheSolnEventResourceMonitorCount(ercs->soln, er); n++)
	    {
	      m = KheSolnEventResourceMonitor(ercs->soln, er, n);
	      if( KheMonitorTag(m) == KHE_LIMIT_RESOURCES_MONITOR_TAG )
	      {
		** KheMonitorDeleteAllParentsRecursive(m); **
		if( !KheMonitorAttachedToSoln(m) )
		  KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR)
		    ercs->soln, m);
		else if( !HaArrayContains(ercs->tmp_monitors, m, &junk) )
		  HaArrayAddLast(ercs->tmp_monitors, m);
	      }
	    }
	}
      }
    }
  }
  KheSolnAddMonitorGroups(ercs->soln, &ercs->tmp_monitors,
    KHE_SUBTAG_LIMIT_RESOURCES);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupCorrelatedEventResourceMonitors(KHE_CORRELATION_SOLVER cs)  */
/*                                                                           */
/*  Group correlated event resource monitors.                                */
/*                                                                           */
/*****************************************************************************/

static void KheGroupCorrelatedEventResourceMonitors(KHE_CORRELATION_SOLVER cs)
{
  int i, j, k;  KHE_INSTANCE ins;  KHE_EVENT_RESOURCE er;
  KHE_EVENT_RESOURCE_CLASS erc, erc2;

  if( DEBUG7 )
    fprintf(stderr, "[ KheGroupCorrelatedEventResourceMonitors(soln)\n");

  /* build an initial set of event resource classes, one per event resource */
  HaArrayClear(cs->tmp_event_resource_classes);
  ins = KheSolnInstance(cs->soln);
  for( i = 0;  i < KheInstanceEventResourceCount(ins);  i++ )
  {
    er = KheInstanceEventResource(ins, i);
    erc = KheEventResourceClassMake(er, cs->soln, cs->arena);
    HaArrayAddLast(cs->tmp_event_resource_classes, erc);
  }

  /* sort the event resource classes and merge equivalent ones */
  HaArraySort(cs->tmp_event_resource_classes, &KheEventResourceClassCmp);
  HaArrayClear(cs->all_event_resource_classes);
  for( i = 0;  i < HaArrayCount(cs->tmp_event_resource_classes);  i = j )
  {
    erc = HaArray(cs->tmp_event_resource_classes, i);
    for( j = i + 1;  j < HaArrayCount(cs->tmp_event_resource_classes);  j++ )
    {
      erc2 = HaArray(cs->tmp_event_resource_classes, j);
      if( KheEventResourceClassTypedCmp(erc, erc2) != 0 )
	break;
      HaArrayAppend(erc->event_resources, erc2->event_resources, k);
    }
    HaArrayAddLast(cs->all_event_resource_classes, erc);
  }
  HaArrayClear(cs->tmp_event_resource_classes);

  /* sort the event resources of the final classes */
  HaArrayForEach(cs->all_event_resource_classes, erc, i)
    HaArraySort(erc->event_resources, &KheEventResourceCmp);
  if( DEBUG7 )
  {
    fprintf(stderr, "  all event resource classes:\n");
    HaArrayForEach(cs->all_event_resource_classes, erc, i)
      KheEventResourceClassDebug(erc, 2, stderr);
  }

  /* group assign resource and prefer resource monitors */
  HaArrayForEach(cs->all_event_resource_classes, erc, i)
  {
    KheGroupAssignResourceMonitors(cs, erc);
    KheGroupPreferResourcesMonitors(cs, erc);
  }

  /* group limit resources monitors by frame, or else by event */
  /* *** not grouping limit resources monitors now
  es_group_limit_resources_off = KheOptionsGetBool(options,
    "es_group_limit_resources_off", false);
  if( !es_group_limit_resources_off )
  {
    frame = KheOptionsFrame(options, "gs_common_frame", soln);
    et m = (KHE_EVENT_TIMETABLE_MONITOR)
      KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);
    if( etm != NULL && frame != NULL )
    {
      ** group limit resources monitors by frame **
      for( i = 0;  i < KheFrameTimeGroupCount(frame);  i++ )
      {
	tg = KheFrameTimeGroup(frame, i);
	for( j = 0;  j < KheInstanceResourceTypeCount(ins);  j++ )
	{
	  rt = KheInstanceResourceType(ins, j);
	  KheTimeGroupDoLimitResourcesGroup(cs, tg, rt, etm);
	}
      }
    }
    else
    {
      ** group limit resources monitors by event **
      for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
      {
	e = KheInstanceEvent(ins, i);
	for( j = 0;  j < KheInstanceResourceTypeCount(ins);  j++ )
	{
	  rt = KheInstanceResourceType(ins, j);
	  KheGroupLimitResourcesMonitors(cs, e, rt);
	}
      }
    }
  }

  ** KheCorrelationSolverDelete(cs); **
  *** */
  if( DEBUG7 )
    fprintf(stderr, "] KheGroupCorrelatedEventResourceMonitors returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheUnGroupCorrelatedEventResourceMonitors(KHE_SOLN soln)            */
/*                                                                           */
/*  Undo what KheGroupCorrelatedEventResourceMonitors did.                   */
/*                                                                           */
/*****************************************************************************/

static void KheUnGroupCorrelatedEventResourceMonitors(KHE_SOLN soln)
{
  KHE_INSTANCE ins;  int i, j;  KHE_EVENT_RESOURCE er;  KHE_MONITOR m;
  KHE_GROUP_MONITOR gm;
  if( DEBUG7 )
    fprintf(stderr, "[ KheUnGroupCorrelatedEventResourceMonitors(soln)\n");
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceEventResourceCount(ins);  i++ )
  {
    er = KheInstanceEventResource(ins, i);
    for( j = 0;  j < KheSolnEventResourceMonitorCount(soln, er);  j++ )
    {
      m = KheSolnEventResourceMonitor(soln, er, j);
      switch( KheMonitorTag(m) )
      {
	case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

	  if( KheMonitorHasParent(m, KHE_SUBTAG_ASSIGN_RESOURCE, &gm) )
	    KheGroupMonitorBypassAndDelete(gm);
	  break;

	case KHE_PREFER_RESOURCES_MONITOR_TAG:

	  if( KheMonitorHasParent(m, KHE_SUBTAG_PREFER_RESOURCES, &gm) )
	    KheGroupMonitorBypassAndDelete(gm);
	  break;

	/* *** not grouping limit resources monitors now
	case KHE_LIMIT_RESOURCES_MONITOR_TAG:

	  if( KheMonitorHasParent(m, KHE_SUBTAG_LIMIT_RESOURCES, &gm) )
	    KheGroupMonitorBypassAndDelete(gm);
	  break;
	*** */

	default:

	  /* not interested */
	  break;
      }
    }
  }
  if( DEBUG7 )
    fprintf(stderr, "] KheUnGroupCorrelatedEventResourceMonitors returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "grouping correlated resource monitors"                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorConstraintTypedCmp(KHE_MONITOR m1, KHE_MONITOR m2)         */
/*                                                                           */
/*  Typed comparison function for bringing monitors derived from the         */
/*  same constraint together.                                                */
/*                                                                           */
/*****************************************************************************/

static int KheMonitorConstraintTypedCmp(KHE_MONITOR m1, KHE_MONITOR m2)
{
  KHE_CONSTRAINT c1, c2;  int index1, index2;
  c1 = KheMonitorConstraint(m1);
  c2 = KheMonitorConstraint(m2);
  index1 = (c1 == NULL ? -1 : KheConstraintIndex(c1));
  index2 = (c2 == NULL ? -1 : KheConstraintIndex(c2));
  return index1 - index2;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorConstraintCmp(const void *p1, const void *p2)              */
/*                                                                           */
/*  Untyped comparison function for bringing monitors derived from the       */
/*  same constraint together.                                                */
/*                                                                           */
/*****************************************************************************/

static int KheMonitorConstraintCmp(const void *p1, const void *p2)
{
  KHE_MONITOR m1 = * (KHE_MONITOR *) p1;
  KHE_MONITOR m2 = * (KHE_MONITOR *) p2;
  return KheMonitorConstraintTypedCmp(m1, m2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupResourceClass(KHE_CORRELATION_SOLVER cs,                    */
/*    KHE_RESOURCE_CLASS rc, KHE_MONITOR_TAG tag)                            */
/*                                                                           */
/*  Group together those resource monitors with the given tag that monitor   */
/*  resources of class rs and are derived from the same resource constraint. */
/*                                                                           */
/*****************************************************************************/

static void KheGroupResourceClass(KHE_CORRELATION_SOLVER cs,
  KHE_RESOURCE_CLASS rc, KHE_MONITOR_TAG tag)
{
  KHE_RESOURCE r;  int i, j;  KHE_MONITOR m, m2;

  /* gather the monitors */
  HaArrayClear(cs->tmp_monitors);
  HaArrayForEach(rc->resources, r, i)
    for( j = 0;  j < KheSolnResourceMonitorCount(cs->soln, r);  j++ )
    {
      m = KheSolnResourceMonitor(cs->soln, r, j);
      if( KheMonitorTag(m) == tag )
	HaArrayAddLast(cs->tmp_monitors, m);
    }

  /* sort them so that monitors derived from the same constraint are together */
  HaArraySort(cs->tmp_monitors, &KheMonitorConstraintCmp);

  /* find runs of monitors derived from the same constraint and group them */
  for( i = 0;  i < HaArrayCount(cs->tmp_monitors);  i = j )
  {
    m = HaArray(cs->tmp_monitors, i);
    HaArrayClear(cs->tmp_monitors2);
    HaArrayAddLast(cs->tmp_monitors2, m);
    for( j = i + 1;  j < HaArrayCount(cs->tmp_monitors);  j++ )
    {
      m2 = HaArray(cs->tmp_monitors, j);
      if( KheMonitorConstraintTypedCmp(m, m2) != 0 )
	break;
      HaArrayAddLast(cs->tmp_monitors2, m2);
    }
    KheSolnAddMonitorGroups(cs->soln, &cs->tmp_monitors2,
      KheSubTagFromTag(tag));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupResourceType(KHE_CORRELATION_SOLVER cs,                     */
/*    KHE_RESOURCE_TYPE rt)                                                  */
/*                                                                           */
/*  Group correlated resource monitors for resources of type rt, which is    */
/*  known to be all preassigned.                                             */
/*                                                                           */
/*****************************************************************************/

static void KheGroupResourceType(KHE_CORRELATION_SOLVER cs,
  KHE_RESOURCE_TYPE rt)
{
  int i, j, k;  KHE_RESOURCE r;  KHE_RESOURCE_CLASS rc, rc2;

  /* build an initial set of resource classes, one per resource */
  HaArrayClear(cs->tmp_resource_classes);
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    rc = KheResourceClassMake(r, cs);
    HaArrayAddLast(cs->tmp_resource_classes, rc);
  }

  /* sort the resource classes and merge equivalent ones */
  HaArraySort(cs->tmp_resource_classes, &KheResourceClassCmp);
  HaArrayClear(cs->all_resource_classes);
  for( i = 0;  i < HaArrayCount(cs->tmp_resource_classes);  i = j )
  {
    rc = HaArray(cs->tmp_resource_classes, i);
    for( j = i + 1;  j < HaArrayCount(cs->tmp_resource_classes);  j++ )
    {
      rc2 = HaArray(cs->tmp_resource_classes, j);
      if( KheResourceClassTypedCmp(rc, rc2) != 0 )
	break;
      HaArrayAppend(rc->resources, rc2->resources, k);
    }
    HaArrayAddLast(cs->all_resource_classes, rc);
  }
  HaArrayClear(cs->tmp_resource_classes);

  /* sort the resources of the final classes */
  /* *** no need for this?
  HaArrayForEach(cs->all_resource_classes, rc, i)
    HaArraySort(ec->resources, &KheResourceCmp);
  *** */

  /* do the grouping */
  HaArrayForEach(cs->all_resource_classes, rc, i)
  {
    KheGroupResourceClass(cs, rc, KHE_AVOID_CLASHES_MONITOR_TAG);
    KheGroupResourceClass(cs, rc, KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG);
    KheGroupResourceClass(cs, rc, KHE_LIMIT_IDLE_TIMES_MONITOR_TAG);
    KheGroupResourceClass(cs, rc, KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG);
    KheGroupResourceClass(cs, rc, KHE_LIMIT_BUSY_TIMES_MONITOR_TAG);
    KheGroupResourceClass(cs, rc, KHE_LIMIT_WORKLOAD_MONITOR_TAG);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnGroupResourceMonitors(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,   */
/*    KHE_CORRELATION_SOLVER cs)                                             */
/*                                                                           */
/*  Group resource monitors for resources of this type, which are known      */
/*  to be all preassigned.  Use cs for uniqueifying linked events.           */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSolnGroupResourceMonitors(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_CORRELATION_SOLVER cs)
{
  KHE_RESOURCE r;  int i, j, k, n, groups;
  KHE_RESOURCE_CLASS_LAYER rcli, rclj, rcl;  KHE_MONITOR m, m2;
  if( DEBUG6 )
    fprintf(stderr, "[ KheSolnGroupResourceMonitors(soln, %s)\n",
      KheResourceTypeId(rt) == NULL ? "-" : KheResourceTypeId(rt));

  ** get the resource class layers, sorted to bring equal layers together **
  HaArrayClear(cs->tmp_layers);
  HaArrayClear(cs->tmp_monitors);
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    HaArrayAddLast(cs->tmp_layers, KheResourceClassLayerMake(r, cs));
  }
  HaArraySort(cs->tmp_layers, &KheResourceClassLayerCmp);

  ** find runs of resources with equal layers **
  groups = 0;
  for( i = 0;  i < HaArrayCount(cs->tmp_layers);  i = j )
  {
    rcli = HaArray(cs->tmp_layers, i);
    for( j = i + 1;  j < HaArrayCount(cs->tmp_layers);  j++ )
    {
      rclj = HaArray(cs->tmp_layers, j);
      if( KheResourceClassLayerTypedCmp(rcli, rclj) != 0 )
	break;
    }
    groups++;

    ** at this point, rcli ... rclj -1 can be grouped **
    if( DEBUG6 )
    {
      fprintf(stderr, "  group of %d resources:", j - i);
      for( k = i;  k < j;  k++ )
      {
	rcl = HaArray(cs->tmp_layers, k);
	fprintf(stderr, " %s", KheResourceId(rcl->resource) == NULL ? "-" :
	  KheResourceId(rcl->resource));
      }
      fprintf(stderr, "\n");
    }

    if( j - i >= 2 )
    {
      ** grab relevant monitors of resources i ... j - 1 and sort **
      HaArrayClear(cs->tmp_monitors);
      for( k = i;  k < j;  k++ )
      {
	rcl = HaArray(cs->tmp_layers, k);
	for( n = 0; n < KheSolnResourceMonitorCount(soln, rcl->resource); n++ )
	{
	  m = KheSolnResourceMonitor(soln, rcl->resource, n);
	  switch( KheMonitorTag(m) )
	  {
	    case KHE_AVOID_CLASHES_MONITOR_TAG:
	    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
	    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
	    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
	    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
	    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

	      HaArrayAddLast(cs->tmp_monitors, m);
	      ** KheMonitorDeleteAllParentsRecursive(m); **
	      break;

	    default:

	      ** ignore any other monitors **
	      break;
	  }
	}
      }
      HaArraySort(cs->tmp_monitors, &KheMonitorConstraintCmp);

      ** group monitors for the same constraint together **
      for( k = 0;  k < HaArrayCount(cs->tmp_monitors);  k = n )
      {
	m = HaArray(cs->tmp_monitors, k);
        HaArrayClear(cs->tmp_monitors2);
	HaArrayAddLast(cs->tmp_monitors2, m);
	for( n = k + 1;  n < HaArrayCount(cs->tmp_monitors);  n++ )
	{
	  m2 = HaArray(cs->tmp_monitors, n);
	  if( KheMonitorConst raint(m) != KheMonitorCon straint(m2) )
	    break;
	  HaArrayAddLast(cs->tmp_monitors2, m2);
	}
	KheSolnAddMonitorGroups(soln, &cs->tmp_monitors2,
	  KheSubTagFromTag(KheMonitorTag(m)));
      }
    }
  }

  if( DEBUG6 )
  {
    fprintf(stderr, "  %d resources, %d groups\n",
      KheResourceTypeResourceCount(rt), groups);
    fprintf(stderr, "] KheSolnGroupResourceMonitors returning\n");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheUnGroupResourceType(KHE_SOLN soln, KHE_RESOURCE_TYPE rt)         */
/*                                                                           */
/*  Undo the effect of KheGroupResourceType.                                 */
/*                                                                           */
/*****************************************************************************/

static void KheUnGroupResourceType(KHE_SOLN soln, KHE_RESOURCE_TYPE rt)
{
  KHE_RESOURCE r;  int i, j;  KHE_MONITOR m;  KHE_GROUP_MONITOR gm;
  if( DEBUG6 )
    fprintf(stderr, "[ KheUnGroupResourceType(soln, %s)\n",
      KheResourceTypeId(rt) == NULL ? "-" : KheResourceTypeId(rt));
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    for( j = 0;  j < KheSolnResourceMonitorCount(soln, r);  j++ )
    {
      m = KheSolnResourceMonitor(soln, r, j);
      switch( KheMonitorTag(m) )
      {
	case KHE_AVOID_CLASHES_MONITOR_TAG:
	case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
	case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
	case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
	case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
	case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

	  if( KheMonitorHasParent(m, KheSubTagFromTag(KheMonitorTag(m)), &gm) )
	    KheGroupMonitorBypassAndDelete(gm);
	  break;

	default:

	  /* ignore any other monitors */
	  break;
      }
    }
  }
  if( DEBUG6 )
    fprintf(stderr, "] KheUnGroupResourceType returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupCorrelatedResourceMonitors(KHE_SOLN soln,                   */
/*    KHE_CORRELATION_SOLVER ecs)                                            */
/*                                                                           */
/*  Group resource monitors, including some detaching.                       */
/*                                                                           */
/*****************************************************************************/

static void KheGroupCorrelatedResourceMonitors(KHE_CORRELATION_SOLVER cs)
{
  int i;  KHE_INSTANCE ins;  KHE_RESOURCE_TYPE rt;

  /* group resource monitors with all-preassigned resource types */
  ins = KheSolnInstance(cs->soln);
  for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
  {
    rt = KheInstanceResourceType(ins, i);
    if( KheResourceTypeDemandIsAllPreassigned(rt) )
      KheGroupResourceType(cs, rt);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheUnGroupCorrelatedResourceMonitors(KHE_SOLN soln)                 */
/*                                                                           */
/*  Undo KheGroupCorrelatedResourceMonitors.                                 */
/*                                                                           */
/*****************************************************************************/

static void KheUnGroupCorrelatedResourceMonitors(KHE_SOLN soln)
{
  KHE_INSTANCE ins;  int i;  KHE_RESOURCE_TYPE rt;

  /* ungroup resource monitors with all-preassigned resource types */
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
  {
    rt = KheInstanceResourceType(ins, i);
    if( KheResourceTypeDemandIsAllPreassigned(rt) )
      KheUnGroupResourceType(soln, rt);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "grouping correlated demand monitors"                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMeetGroupDemandMonitors(KHE_MEET meet,                           */
/*    ARRAY_KHE_MONITOR *monitors)                                           */
/*                                                                           */
/*  Add to *monitors the demand monitors associated with meet and with any   */
/*  other meets whose assignment is (directly or indirectly) fixed to meet.  */
/*  Despite the function name, the actual grouping is done separately.       */
/*                                                                           */
/*****************************************************************************/

static void KheMeetGroupDemandMonitors(KHE_MEET meet,
  ARRAY_KHE_MONITOR *monitors)
{
  KHE_MEET sub_meet;  int i, j;  KHE_TASK task;  KHE_MONITOR m;

  if( DEBUG12 )
    fprintf(stderr, "[ KheMeetGroupDemandMonitors(%s)\n", KheMeetId(meet));

  /* handle meet itself */
  for( i = 0;  i < KheMeetTaskCount(meet);  i++ )
  {
    task = KheMeetTask(meet, i);
    for( j = 0;  j < KheTaskDemandMonitorCount(task);  j++ )
    {
      m = (KHE_MONITOR) KheTaskDemandMonitor(task, j);
      if( DEBUG12 )
	KheMonitorDebug(m, 2, 2, stderr);
      HaArrayAddLast(*monitors, m);
    }
  }

  /* handle meets whose assignment is fixed to meet */
  for( i = 0;  i < KheMeetAssignedToCount(meet);  i++ )
  {
    sub_meet = KheMeetAssignedTo(meet, i);
    if( KheMeetAssignIsFixed(sub_meet) )
      KheMeetGroupDemandMonitors(sub_meet, monitors);
  }
  if( DEBUG12 )
    fprintf(stderr, "] KheMeetGroupDemandMonitors returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetUnGroupDemandMonitors(KHE_MEET meet)                         */
/*                                                                           */
/*  Delete the demand group monitors associated with meet and with any       */
/*  other meets whose assignment is (directly or indirectly) fixed to meet.  */
/*                                                                           */
/*  Implementation note.  In practice there will only be one group           */
/*  monitor, but we don't stop searching when we find it, because in         */
/*  theory there could be several (if the demand monitors had different      */
/*  parents before they were grouped).  We could just visit all meets,       */
/*  but we prefer to visit meets in the way that grouping visited them.      */
/*                                                                           */
/*****************************************************************************/

static void KheMeetUnGroupDemandMonitors(KHE_MEET meet)
{
  KHE_MEET sub_meet;  int i, j;  KHE_TASK task;  KHE_MONITOR m;
  KHE_GROUP_MONITOR gm;

  /* handle meet itself */
  if( KheMeetTaskCount(meet) > 0 )
  {
    task = KheMeetTask(meet, 0);
    for( j = 0;  j < KheTaskDemandMonitorCount(task);  j++ )
    {
      m = (KHE_MONITOR) KheTaskDemandMonitor(task, j);
      if( KheMonitorHasParent(m, KHE_SUBTAG_ORDINARY_DEMAND, &gm) )
	KheGroupMonitorBypassAndDelete(gm);
    }
  }

  /* handle meets whose assignment is fixed to meet */
  for( i = 0;  i < KheMeetAssignedToCount(meet);  i++ )
  {
    sub_meet = KheMeetAssignedTo(meet, i);
    if( KheMeetAssignIsFixed(sub_meet) )
      KheMeetUnGroupDemandMonitors(sub_meet);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupCorrelatedDemandMonitors(KHE_CORRELATION_SOLVER cs)         */
/*                                                                           */
/*  Group correlated demand monitors by leader meet.  Only ordinary demand   */
/*  monitors (not workload demand monitors) are grouped.                     */
/*                                                                           */
/*****************************************************************************/

static void KheGroupCorrelatedDemandMonitors(KHE_CORRELATION_SOLVER cs)
{
  int i;  KHE_MEET meet;
  if( DEBUG9 )
    fprintf(stderr, "[ KheGroupCorrelatedDemandMonitors(soln %.5f)\n",
      KheCostShow(KheSolnCost(cs->soln)));
  for( i = 0;  i < KheSolnMeetCount(cs->soln);  i++ )
  {
    meet = KheSolnMeet(cs->soln, i);
    if( !KheMeetIsCycleMeet(meet) && !KheMeetAssignIsFixed(meet) )
    {
      HaArrayClear(cs->tmp_monitors);
      KheMeetGroupDemandMonitors(meet, &cs->tmp_monitors);
      KheSolnAddMonitorGroups(cs->soln, &cs->tmp_monitors,
	KHE_SUBTAG_ORDINARY_DEMAND);
    }
  }
  if( DEBUG9 )
    fprintf(stderr, "] KheGroupCorrelatedDemandMonitors ret %.5f\n",
      KheCostShow(KheSolnCost(cs->soln)));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheUnGroupCorrelatedDemandMonitors(KHE_SOLN soln)                   */
/*                                                                           */
/*  Undo what KheGroupCorrelatedDemandMonitors did.                          */
/*                                                                           */
/*****************************************************************************/

static void KheUnGroupCorrelatedDemandMonitors(KHE_SOLN soln)
{
  int i;  KHE_MEET meet;
  for( i = 0;  i < KheSolnMeetCount(soln);  i++ )
  {
    meet = KheSolnMeet(soln, i);
    if( !KheMeetIsCycleMeet(meet) && !KheMeetAssignIsFixed(meet) )
      KheMeetUnGroupDemandMonitors(meet);
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

/* *** moved to khe_sm_matching.c
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
  if( res < 0 )khe_sm_correlation.c
    res = 0;
  if( DEBUG10 )
    fprintf(stderr, "  KheLimitWorkloadMonitorFindCeiling(%s) = %d\n",
      KheResourceId(r), res);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheDoAdjustResourceMonitors(KHE_SOLN soln, KHE_RESOURCE r)          */
/*                                                                           */
/*  Adjust r's resource monitors.                                            */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_matching.c
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

	    ** nothing to do otherwise **
	    break;
	}
	break;

      default:

	** nothing to do otherwise **
	break;
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheDoUnAdjustResourceMonitors(KHE_SOLN soln, KHE_RESOURCE r)        */
/*                                                                           */
/*  Unadjust r's resource monitors.                                          */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_matching.c
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

	    ** nothing to do otherwise **
	    break;
	}
	break;

      default:

	** nothing to do otherwise **
	break;
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheAdjustResourceMonitors(KHE_SOLN soln)                            */
/*                                                                           */
/*  Adjust resource monitors.                                                */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_matching.c
static void KheAdjustResourceMonitors(KHE_SOLN soln)
{
  int i;  KHE_INSTANCE ins;
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
    KheDoAdjustResourceMonitors(soln, KheInstanceResource(ins, i));
}
*** */


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

/* *** moved to khe_sm_matching.c
static void KheUnAdjustResourceMonitors(KHE_SOLN soln)
{
  int i;  KHE_INSTANCE ins;
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
    KheDoUnAdjustResourceMonitors(soln, KheInstanceResource(ins, i));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "public functions"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheGroupCorrelatedMonitors(KHE_SOLN soln)                           */
/*                                                                           */
/*  Group the correlated monitors of soln.                                   */
/*                                                                           */
/*****************************************************************************/

void KheGroupCorrelatedMonitors(KHE_SOLN soln)
{
  KHE_CORRELATION_SOLVER cs;  HA_ARENA a;
  if( DEBUG1 )
    fprintf(stderr, "[ KheGroupCorrelatedMonitors(soln)\n");
  a = KheSolnArenaBegin(soln);
  cs = KheCorrelationSolverMake(soln, a);
  KheGroupCorrelatedEventMonitors(cs);
  KheGroupCorrelatedEventResourceMonitors(cs);
  KheGroupCorrelatedResourceMonitors(cs);
  KheGroupCorrelatedDemandMonitors(cs);
  /* ***
  if( with_adjust )
    KheAdjustResourceMonitors(soln);
  *** */
  KheSolnArenaEnd(soln, a);
  if( DEBUG1 )
    fprintf(stderr, "] KheGroupCorrelatedMonitors returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheUnGroupCorrelatedMonitors(KHE_SOLN soln)                         */
/*                                                                           */
/*  Undo KheGroupCorrelatedMonitors.                                         */
/*                                                                           */
/*****************************************************************************/

void KheUnGroupCorrelatedMonitors(KHE_SOLN soln)
{
  if( DEBUG1 )
    fprintf(stderr, "[ KheUnGroupCorrelatedMonitors(soln)\n");
  KheUnGroupCorrelatedEventMonitors(soln);
  KheUnGroupCorrelatedEventResourceMonitors(soln);
  KheUnGroupCorrelatedResourceMonitors(soln);
  KheUnGroupCorrelatedDemandMonitors(soln);
  /* ***
  if( true )
    KheUnAdjustResourceMonitors(soln);
  *** */
  if( DEBUG1 )
    fprintf(stderr, "] KheUnGroupCorrelatedMonitors returning\n");
}

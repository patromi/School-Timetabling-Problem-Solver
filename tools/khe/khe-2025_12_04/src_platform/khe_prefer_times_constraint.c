
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
/*  FILE:         khe_prefer_times_constraint.c                              */
/*  DESCRIPTION:  A prefer times constraint                                  */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_PREFER_TIMES_CONSTRAINT - a prefer times constraint                  */
/*                                                                           */
/*****************************************************************************/

struct khe_prefer_times_constraint_rec {
  INHERIT_CONSTRAINT
  ARRAY_KHE_TIME_GROUP		time_groups;		/* variable domain   */
  ARRAY_KHE_TIME		times;			/* variable domain   */
  KHE_TIME_GROUP		domain;			/* variable domain   */
  int				duration;		/* optional duration */
  ARRAY_KHE_EVENT		events;			/* Events            */
  ARRAY_KHE_EVENT_GROUP		event_groups;		/* EventGroups       */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KhePreferTimesConstraintMake(KHE_INSTANCE ins, char *id,            */
/*    char *name, bool required, int weight, KHE_COST_FUNCTION cf,           */
/*    int duration, KHE_PREFER_TIMES_CONSTRAINT *c)                          */
/*                                                                           */
/*  Make a prefer times constraint, add it to the instance, and return it.   */
/*                                                                           */
/*****************************************************************************/

bool KhePreferTimesConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  int duration, KHE_PREFER_TIMES_CONSTRAINT *c)
{
  KHE_PREFER_TIMES_CONSTRAINT res;  KHE_CONSTRAINT cc;  HA_ARENA a;
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KhePreferTimesConstraintMake called after KheInstanceMakeEnd");
  if( id != NULL && KheInstanceRetrieveConstraint(ins, id, &cc) )
  {
    *c = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  HaMake(res, a);
  KheConstraintInitCommonFields((KHE_CONSTRAINT) res,
    KHE_PREFER_TIMES_CONSTRAINT_TAG, ins, id, name, required, weight, cf, a);
  HaArrayInit(res->time_groups, a);
  HaArrayInit(res->times, a);
  res->domain = NULL;
  res->duration = duration;
  HaArrayInit(res->events, a);
  HaArrayInit(res->event_groups, a);
  KheInstanceAddConstraint(ins, (KHE_CONSTRAINT) res);
  *c = res;
  if( DEBUG1 )
    fprintf(stderr,
      "  %p = KhePreferTimesConstraintMake(%s, %s, ...) times.items = %p\n",
      (void *) res, KheInstanceId(ins), id, (void *) res->times.items);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferTimesConstraintDuration(KHE_PREFER_TIMES_CONSTRAINT c)      */
/*                                                                           */
/*  Return the duration attribute of c.  This could be the special value     */
/*  KHE_ANY_DURATION.                                                        */
/*                                                                           */
/*****************************************************************************/

int KhePreferTimesConstraintDuration(KHE_PREFER_TIMES_CONSTRAINT c)
{
  return c->duration;
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferTimesConstraintAppliesToCount(KHE_PREFER_TIMES_CONSTRAINT c)*/
/*                                                                           */
/*  Return the number of points of application of c.                         */
/*                                                                           */
/*****************************************************************************/

int KhePreferTimesConstraintAppliesToCount(KHE_PREFER_TIMES_CONSTRAINT c)
{
  int i, res;  KHE_EVENT_GROUP eg;
  res = HaArrayCount(c->events);
  HaArrayForEach(c->event_groups, eg, i)
    res += KheEventGroupEventCount(eg);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesConstraintFinalize(KHE_PREFER_TIMES_CONSTRAINT c)     */
/*                                                                           */
/*  Finalize c, since KheInstanceMakeEnd has been called.                    */
/*                                                                           */
/*  Implementation note.  What needs to be done is to set the domain of c.   */
/*                                                                           */
/*****************************************************************************/

void KhePreferTimesConstraintFinalize(KHE_PREFER_TIMES_CONSTRAINT c)
{
  int i;  KHE_TIME_GROUP tg;  KHE_TIME t;  SSET ss;  HA_ARENA a;
  if( HaArrayCount(c->time_groups) == 0 && HaArrayCount(c->times) == 0 )
    c->domain = KheInstanceEmptyTimeGroup(c->instance);
  else if( HaArrayCount(c->time_groups) == 0 && HaArrayCount(c->times) == 1 )
    c->domain = KheTimeSingletonTimeGroup(HaArrayFirst(c->times));
  else if( HaArrayCount(c->time_groups) == 1 && HaArrayCount(c->times) == 0 )
    c->domain = HaArrayFirst(c->time_groups);
  else
  {
    a = KheInstanceArena(c->instance);
    SSetInit(ss, a);
    HaArrayForEach(c->time_groups, tg, i)
      SSetUnion(ss, *KheTimeGroupTimeSet(tg));
    HaArrayForEach(c->times, t, i)
      SSetInsert(ss, KheTimeIndex(t));
    c->domain = KheTimeGroupMakeAndFinalize(c->instance,
      KHE_TIME_GROUP_KIND_AUTO, NULL, NULL, &ss, /* NULL, */ true, a);
    /* ***
    c->domain = KheTimeGroup MakeInternal(c->instance,
      ** KHE_TIME_GROUP_TYPE_CONSTRUCTED, ** NULL, KHE_TIME_GROUP_KIND_AUTO,
      NULL, NULL ** , LSetNew() **);
    HaArrayForEach(c->time_groups, tg, i)
      KheTimeGroupUnion Internal(c->domain, tg);
    HaArrayForEach(c->times, t, i)
      KheTimeGroupAdd TimeInternal(c->domain, t);
    KheTimeGroup Finalize(c->domain, NULL, true ** , NULL, -1 **);
    *** */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferTimesConstraintDensityCount(KHE_PREFER_TIMES_CONSTRAINT c)  */
/*                                                                           */
/*  Return the density count of c; just the applies to count in this case.   */
/*                                                                           */
/*****************************************************************************/

int KhePreferTimesConstraintDensityCount(KHE_PREFER_TIMES_CONSTRAINT c)
{
  return KhePreferTimesConstraintAppliesToCount(c);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time groups"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesConstraintAddTimeGroup(KHE_PREFER_TIMES_CONSTRAINT c, */
/*    KHE_TIME_GROUP tg)                                                     */
/*                                                                           */
/*  Add a time group to c.                                                   */
/*                                                                           */
/*****************************************************************************/

void KhePreferTimesConstraintAddTimeGroup(KHE_PREFER_TIMES_CONSTRAINT c,
  KHE_TIME_GROUP tg)
{
  HaArrayAddLast(c->time_groups, tg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferTimesConstraintTimeGroupCount(KHE_PREFER_TIMES_CONSTRAINT c)*/
/*                                                                           */
/*  Return the number of time groups in c.                                   */
/*                                                                           */
/*****************************************************************************/

int KhePreferTimesConstraintTimeGroupCount(KHE_PREFER_TIMES_CONSTRAINT c)
{
  return HaArrayCount(c->time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KhePreferTimesConstraintTimeGroup(                        */
/*    KHE_PREFER_TIMES_CONSTRAINT c, int i)                                  */
/*                                                                           */
/*  Return the i'th time group of c.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KhePreferTimesConstraintTimeGroup(
  KHE_PREFER_TIMES_CONSTRAINT c, int i)
{
  return HaArray(c->time_groups, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "times"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesConstraintAddTime(KHE_PREFER_TIMES_CONSTRAINT c,      */
/*    KHE_TIME t)                                                            */
/*                                                                           */
/*  Add a time to c.                                                         */
/*                                                                           */
/*****************************************************************************/

void KhePreferTimesConstraintAddTime(KHE_PREFER_TIMES_CONSTRAINT c, KHE_TIME t)
{
  HaArrayAddLast(c->times, t);
  if( DEBUG1 )
    fprintf(stderr,
      "  KhePreferTimesConstraintAddTime(%p %s, %s), %d times, items = %p\n",
      (void *) c, c->id, KheTimeId(t), HaArrayCount(c->times),
      (void *) c->times.items);
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferTimesConstraintTimeCount(KHE_PREFER_TIMES_CONSTRAINT c)     */
/*                                                                           */
/*  Return the number of times in c.                                         */
/*                                                                           */
/*****************************************************************************/

int KhePreferTimesConstraintTimeCount(KHE_PREFER_TIMES_CONSTRAINT c)
{
  return HaArrayCount(c->times);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KhePreferTimesConstraintTime(KHE_PREFER_TIMES_CONSTRAINT c,     */
/*    int i)                                                                 */
/*                                                                           */
/*  Return the i'th time of c.                                               */
/*                                                                           */
/*****************************************************************************/

KHE_TIME KhePreferTimesConstraintTime(KHE_PREFER_TIMES_CONSTRAINT c, int i)
{
  return HaArray(c->times, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "domain"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KhePreferTimesConstraintDomain(                           */
/*    KHE_PREFER_TIMES_CONSTRAINT c)                                         */
/*                                                                           */
/*  Return the domain of c.                                                  */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KhePreferTimesConstraintDomain(KHE_PREFER_TIMES_CONSTRAINT c)
{
  HnAssert(c->domain != NULL,
    "KhePreferTimesConstraintDomain called before KheInstanceMakeEnd");
  return c->domain;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "events"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesConstraintAddEvent(KHE_PREFER_TIMES_CONSTRAINT c,     */
/*    KHE_EVENT e)                                                           */
/*                                                                           */
/*  Add an event to c.                                                       */
/*                                                                           */
/*****************************************************************************/

void KhePreferTimesConstraintAddEvent(KHE_PREFER_TIMES_CONSTRAINT c,
  KHE_EVENT e)
{
  HaArrayAddLast(c->events, e);
  if( KheEventPreassignedTime(e) == NULL )
    KheEventAddConstraint(e, (KHE_CONSTRAINT) c);
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferTimesConstraintEventCount(KHE_PREFER_TIMES_CONSTRAINT c)    */
/*                                                                           */
/*  Return the number of events of c.                                        */
/*                                                                           */
/*****************************************************************************/

int KhePreferTimesConstraintEventCount(KHE_PREFER_TIMES_CONSTRAINT c)
{
  return HaArrayCount(c->events);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KhePreferTimesConstraintEvent(KHE_PREFER_TIMES_CONSTRAINT c,   */
/*    int i)                                                                 */
/*                                                                           */
/*  Return the i'th event of c.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KhePreferTimesConstraintEvent(KHE_PREFER_TIMES_CONSTRAINT c, int i)
{
  return HaArray(c->events, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "event groups"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesConstraintAddEventGroup(                              */
/*    KHE_PREFER_TIMES_CONSTRAINT c, KHE_EVENT_GROUP eg)                     */
/*                                                                           */
/*  Add an event group to c.                                                 */
/*                                                                           */
/*****************************************************************************/

void KhePreferTimesConstraintAddEventGroup(
  KHE_PREFER_TIMES_CONSTRAINT c, KHE_EVENT_GROUP eg)
{
  int i;  KHE_EVENT e;
  for( i = 0;  i < KheEventGroupEventCount(eg);  i++ )
  {
    e = KheEventGroupEvent(eg, i);
    if( KheEventPreassignedTime(e) == NULL )
      KheEventAddConstraint(e, (KHE_CONSTRAINT) c);
  }
  HaArrayAddLast(c->event_groups, eg);
}


/*****************************************************************************/
/*                                                                           */
/* int KhePreferTimesConstraintEventGroupCount(KHE_PREFER_TIMES_CONSTRAINT c)*/
/*                                                                           */
/*  Return the number of event groups in c.                                  */
/*                                                                           */
/*****************************************************************************/

int KhePreferTimesConstraintEventGroupCount(KHE_PREFER_TIMES_CONSTRAINT c)
{
  return HaArrayCount(c->event_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_GROUP KhePreferTimesConstraintEventGroup(                      */
/*    KHE_PREFER_TIMES_CONSTRAINT c, int i)                                  */
/*                                                                           */
/*  Return the i'th event group of c.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_GROUP KhePreferTimesConstraintEventGroup(
  KHE_PREFER_TIMES_CONSTRAINT c, int i)
{
  return HaArray(c->event_groups, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitors"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesConstraintMakeAndAttachMonitors(                      */
/*    KHE_PREFER_TIMES_CONSTRAINT c, KHE_SOLN soln)                          */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

void KhePreferTimesConstraintMakeAndAttachMonitors(
  KHE_PREFER_TIMES_CONSTRAINT c, KHE_SOLN soln)
{
  int i, j;  KHE_EVENT_GROUP eg;  KHE_EVENT e;
  KHE_EVENT_IN_SOLN es;  KHE_PREFER_TIMES_MONITOR m;
  HaArrayForEach(c->events, e, i)
  {
    es = KheSolnEventInSoln(soln, e);
    /* es = HaArray(soln->events_in_soln, KheEventIndex(e)); */
    m = KhePreferTimesMonitorMake(es, c);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
  HaArrayForEach(c->event_groups, eg, i)
  {
    for( j = 0;  j < KheEventGroupEventCount(eg);  j++ )
    {
      e = KheEventGroupEvent(eg, j);
      es = KheSolnEventInSoln(soln, e);
      /* es = HaArray(soln->events_in_soln, KheEventIndex(e)); */
      m = KhePreferTimesMonitorMake(es, c);
      KheMonitorAttachToSoln((KHE_MONITOR) m);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KhePreferTimesConstraintMakeFromKml(KML_ELT cons_elt,               */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Add a prefer times constraint based on cons_elt to ins.                  */
/*                                                                           */
/*****************************************************************************/

bool KhePreferTimesConstraintMakeFromKml(KML_ELT cons_elt,
  KHE_INSTANCE ins, KML_ERROR *ke)
{
  char *id, *name;  bool reqd;  int wt;  KHE_COST_FUNCTION cf;
  KML_ELT elt;  KHE_PREFER_TIMES_CONSTRAINT res;  int duration;  HA_ARENA a;

  /* verify cons_elt and get the common fields */
  a = KheInstanceArena(ins);
  if( !KmlCheck(cons_elt, "Id : $Name $Required #Weight "
      "$CostFunction AppliesTo +TimeGroups +Times +#Duration", ke) )
    return false;
  if( !KheConstraintCheckKml(cons_elt, &id, &name, &reqd, &wt, &cf, ke, a) )
    return false;

  /* get the duration */
  if( KmlContainsChild(cons_elt, "Duration", &elt) )
    sscanf(KmlText(elt), "%d", &duration);
  else
    duration = KHE_ANY_DURATION;

  /* make the constraint object and add it to ins */
  if( !KhePreferTimesConstraintMake(ins, id, name, reqd, wt, cf,
	duration, &res) )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<PreferTimesConstraint> Id \"%s\" used previously", id);

  /* add the event groups and events */
  elt = KmlChild(cons_elt, 4);
  if( !KmlCheck(elt, ": +EventGroups +Events", ke) )
    return false;
  if( !KheConstraintAddEventGroupsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( !KheConstraintAddEventsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( KhePreferTimesConstraintAppliesToCount(res) == 0 )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<PreferTimesConstraint> applies to 0 events");

  /* add the time groups and times */
  if( !KheConstraintAddTimeGroupsFromKml((KHE_CONSTRAINT) res, cons_elt, ke,a) )
    return false;
  if( !KheConstraintAddTimesFromKml((KHE_CONSTRAINT) res, cons_elt, ke, a) )
    return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesConstraintWrite(KHE_PREFER_TIMES_CONSTRAINT c,        */
/*    KML_FILE kf)                                                           */
/*                                                                           */
/*  Write c to kf.                                                           */
/*                                                                           */
/*****************************************************************************/

void KhePreferTimesConstraintWrite(KHE_PREFER_TIMES_CONSTRAINT c, KML_FILE kf)
{
  KHE_EVENT_GROUP eg;  KHE_EVENT e;  int i;
  KHE_TIME_GROUP tg;  KHE_TIME t;
  KmlBegin(kf, "PreferTimesConstraint");
  HnAssert(c->id != NULL,
    "KheArchiveWrite: Id missing in PreferTimesConstraint");
  KmlAttribute(kf, "Id", c->id);
  KheConstraintWriteCommonFields((KHE_CONSTRAINT) c, kf);
  KmlBegin(kf, "AppliesTo");
  if( HaArrayCount(c->event_groups) > 0 )
  {
    KmlBegin(kf, "EventGroups");
    HaArrayForEach(c->event_groups, eg, i)
    {
      HnAssert(KheEventGroupId(eg) != NULL, "KheArchiveWrite:  Id missing"
        " in EventGroup referenced from PreferTimesConstraint %s", c->id);
      KmlEltAttribute(kf, "EventGroup", "Reference", KheEventGroupId(eg));
    }
    KmlEnd(kf, "EventGroups");
  }
  if( HaArrayCount(c->events) > 0 )
  {
    KmlBegin(kf, "Events");
    HaArrayForEach(c->events, e, i)
    {
      HnAssert(KheEventId(e) != NULL, "KheArchiveWrite:  Id missing"
        " in Event referenced from PreferTimesConstraint %s", c->id);
      KmlEltAttribute(kf, "Event", "Reference", KheEventId(e));
    }
    KmlEnd(kf, "Events");
  }
  KmlEnd(kf, "AppliesTo");
  if( HaArrayCount(c->time_groups) > 0 )
  {
    KmlBegin(kf, "TimeGroups");
    HaArrayForEach(c->time_groups, tg, i)
    {
      HnAssert(KheTimeGroupId(tg) != NULL, "KheArchiveWrite:  Id missing"
        " in TimeGroup referenced from PreferTimesConstraint %s", c->id);
      KmlEltAttribute(kf, "TimeGroup", "Reference", KheTimeGroupId(tg));
    }
    KmlEnd(kf, "TimeGroups");
  }
  if( HaArrayCount(c->times) > 0 )
  {
    if( DEBUG1 )
      fprintf(stderr,
        "  KhePreferTimesConstraintWrite(%p %s), %d times, times.items = %p\n",
        (void *) c, c->id, HaArrayCount(c->times), (void *) c->times.items);
    KmlBegin(kf, "Times");
    HaArrayForEach(c->times, t, i)
    {
      HnAssert(KheTimeId(t) != NULL, "KheArchiveWrite:  Id missing"
        " in Time referenced from PreferTimesConstraint %s", c->id);
      KmlEltAttribute(kf, "Time", "Reference", KheTimeId(t));
    }
    KmlEnd(kf, "Times");
  }
  if( c->duration != KHE_ANY_DURATION )
    KmlEltFmtText(kf, "Duration", "%d", c->duration);
  KmlEnd(kf, "PreferTimesConstraint");
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesConstraintDebug(KHE_PREFER_TIMES_CONSTRAINT c,        */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of c onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KhePreferTimesConstraintDebug(KHE_PREFER_TIMES_CONSTRAINT c,
  int verbosity, int indent, FILE *fp)
{
  KHE_EVENT_GROUP eg;  KHE_EVENT e;  int i;
  KHE_TIME_GROUP tg;  KHE_TIME t;
  if( verbosity >= 1 )
  {
    KheConstraintDebugCommonFields((KHE_CONSTRAINT) c, indent, fp);
    if( indent >= 0 && verbosity >= 2 )
    {
      fprintf(fp, "%*s[\n", indent, "");
      HaArrayForEach(c->event_groups, eg, i)
	fprintf(fp, "%*s  %s\n", indent, "",
	  KheEventGroupId(eg) != NULL ? KheEventGroupId(eg) : "-");
      HaArrayForEach(c->events, e, i)
	fprintf(fp, "%*s  %s\n", indent, "",
	  KheEventId(e) != NULL ? KheEventId(e) : "-");
      HaArrayForEach(c->time_groups, tg, i)
        fprintf(fp, "%*s  %s\n", indent, "",
	  KheTimeGroupId(tg) != NULL ? KheTimeGroupId(tg) : "-");
      HaArrayForEach(c->times, t, i)
        fprintf(fp, "%*s  %s\n", indent, "",
	  KheTimeId(t) != NULL ? KheTimeId(t) : "-");
      fprintf(fp, "%*s]\n", indent, "");
    }
  }
}

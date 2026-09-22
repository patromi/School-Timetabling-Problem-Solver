
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
/*  FILE:         khs_distribute_split_events_constraint.c                   */
/*  DESCRIPTION:  A distribute split events constraint                       */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT - a distribute split events cons. */
/*                                                                           */
/*****************************************************************************/

struct khe_distribute_split_events_constraint_rec {
  INHERIT_CONSTRAINT
  int				duration;		/* Duration          */
  int				minimum;		/* Minimum           */
  int				maximum;		/* Maximum           */
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
/*  bool KheDistributeSplitEventsConstraintMake(KHE_INSTANCE ins, char *id,  */
/*    char *name, bool required, int weight, KHE_COST_FUNCTION cf,           */
/*    int duration, int minimum, int maximum,                                */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT *c)                             */
/*                                                                           */
/*  Make a distribute split events constraint, add it to the instance, and   */
/*  return it.                                                               */
/*                                                                           */
/*****************************************************************************/

bool KheDistributeSplitEventsConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf, int duration,
  int minimum, int maximum, KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT *c)
{
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT res;  KHE_CONSTRAINT cc;  HA_ARENA a;
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KheDistributeSplitEventsConstraintMake called after KheInstanceMakeEnd");
  if( id != NULL && KheInstanceRetrieveConstraint(ins, id, &cc) )
  {
    *c = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  HaMake(res, a);
  KheConstraintInitCommonFields((KHE_CONSTRAINT) res,
    KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG, ins, id, name, required,
    weight, cf, a);
  res->duration = duration;
  res->minimum = minimum;
  res->maximum = maximum;
  HaArrayInit(res->events, a);
  HaArrayInit(res->event_groups, a);
  KheInstanceAddConstraint(ins, (KHE_CONSTRAINT) res);
  *c = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDistributeSplitEventsConstraintDuration(                          */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the Duration attribute of c.                                      */
/*                                                                           */
/*****************************************************************************/

int KheDistributeSplitEventsConstraintDuration(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)
{
  return c->duration;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDistributeSplitEventsConstraintMinimum(                           */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the Minimum attribute of c.                                       */
/*                                                                           */
/*****************************************************************************/

int KheDistributeSplitEventsConstraintMinimum(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)
{
  return c->minimum;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDistributeSplitEventsConstraintMaximum(                           */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the Maximum attribute of c.                                       */
/*                                                                           */
/*****************************************************************************/

int KheDistributeSplitEventsConstraintMaximum(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)
{
  return c->maximum;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDistributeSplitEventsConstraintAppliesToCount(                    */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the number of points of application of c.                         */
/*                                                                           */
/*****************************************************************************/

int KheDistributeSplitEventsConstraintAppliesToCount(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)
{
  int i, res;  KHE_EVENT_GROUP eg;
  res = HaArrayCount(c->events);
  HaArrayForEach(c->event_groups, eg, i)
    res += KheEventGroupEventCount(eg);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeSplitEventsConstraintFinalize(                         */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)                              */
/*                                                                           */
/*  Finalize c, since KheInstanceMakeEnd has been called.                    */
/*                                                                           */
/*****************************************************************************/

void KheDistributeSplitEventsConstraintFinalize(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)
{
  /* nothing to do in this case */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDistributeSplitEventsConstraintDensityCount(                      */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the density count of c; just the applies to count in this case.   */
/*                                                                           */
/*****************************************************************************/

int KheDistributeSplitEventsConstraintDensityCount(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)
{
  return KheDistributeSplitEventsConstraintAppliesToCount(c);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "events"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeSplitEventsConstraintAddEvent(                         */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, KHE_EVENT e)                 */
/*                                                                           */
/*  Add e to c.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheDistributeSplitEventsConstraintAddEvent(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, KHE_EVENT e)
{
  HaArrayAddLast(c->events, e);
  KheEventAddConstraint(e, (KHE_CONSTRAINT) c);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDistributeSplitEventsConstraintEventCount(                        */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the number of events of c.                                        */
/*                                                                           */
/*****************************************************************************/

int KheDistributeSplitEventsConstraintEventCount(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)
{
  return HaArrayCount(c->events);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheDistributeSplitEventsConstraintEvent(                       */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, int i)                       */
/*                                                                           */
/*  Return the i'th event of c.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KheDistributeSplitEventsConstraintEvent(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, int i)
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
/*  void KheDistributeSplitEventsConstraintAddEventGroup(                    */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, KHE_EVENT_GROUP eg)          */
/*                                                                           */
/*  Add eg to c, and also add the corresponding event resources to c.        */
/*                                                                           */
/*****************************************************************************/

void KheDistributeSplitEventsConstraintAddEventGroup(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, KHE_EVENT_GROUP eg)
{
  int i;  KHE_EVENT e;
  HaArrayAddLast(c->event_groups, eg);
  for( i = 0;  i < KheEventGroupEventCount(eg);  i++ )
  {
    e = KheEventGroupEvent(eg, i);
    KheEventAddConstraint(e, (KHE_CONSTRAINT) c);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDistributeSplitEventsConstraintEventGroupCount(                   */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the number of event groups of c.                                  */
/*                                                                           */
/*****************************************************************************/

int KheDistributeSplitEventsConstraintEventGroupCount(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)
{
  return HaArrayCount(c->event_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_GROUP KheDistributeSplitEventsConstraintEventGroup(            */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, int i)                       */
/*                                                                           */
/*  Return the ith event group of c.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_GROUP KheDistributeSplitEventsConstraintEventGroup(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, int i)
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
/*  void KheDistributeSplitEventsConstraintMakeAndAttachMonitors(            */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, KHE_SOLN soln)               */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

void KheDistributeSplitEventsConstraintMakeAndAttachMonitors(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, KHE_SOLN soln)
{
  int i, j;  KHE_EVENT_GROUP eg;  KHE_EVENT e;
  KHE_EVENT_IN_SOLN es;  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m;
  HaArrayForEach(c->events, e, i)
  {
    es = KheSolnEventInSoln(soln, e);
    /* es = HaArray(soln->events_in_soln, KheEventIndex(e)); */
    m = KheDistributeSplitEventsMonitorMake(es, c);
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
      m = KheDistributeSplitEventsMonitorMake(es, c);
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
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheDistributeSplitEventsConstraintMakeFromKml(KML_ELT cons_elt,     */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Make a distribute split events constraint based on cons_elt and add      */
/*  it to ins.                                                               */
/*                                                                           */
/*****************************************************************************/

bool KheDistributeSplitEventsConstraintMakeFromKml(KML_ELT cons_elt,
  KHE_INSTANCE ins, KML_ERROR *ke)
{
  char *id, *name;  bool reqd;  int wt;  KHE_COST_FUNCTION cf;
  KML_ELT elt;  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT res;
  int duration, minimum, maximum;  HA_ARENA a;

  /* verify cons_elt and get the common fields */
  a = KheInstanceArena(ins);
  if( !KmlCheck(cons_elt,
      "Id : $Name $Required #Weight $CostFunction AppliesTo"
      " #Duration #Minimum #Maximum", ke) )
    return false;
  if( !KheConstraintCheckKml(cons_elt, &id, &name, &reqd, &wt, &cf, ke, a) )
    return false;

  /* find the numeric fields */
  sscanf(KmlText(KmlChild(cons_elt, 5)), "%d", &duration);
  sscanf(KmlText(KmlChild(cons_elt, 6)), "%d", &minimum);
  sscanf(KmlText(KmlChild(cons_elt, 7)), "%d", &maximum);

  /* make the constraint object and add it to ins */
  if( !KheDistributeSplitEventsConstraintMake(ins, id, name, reqd, wt, cf,
	duration, minimum, maximum, &res) )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<DistributeSplitEventsConstraint> Id \"%s\" used previously", id);

  /* add the event groups and events */
  elt = KmlChild(cons_elt, 4);
  if( !KmlCheck(elt, ": +EventGroups +Events", ke) )
    return false;
  if( !KheConstraintAddEventGroupsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( !KheConstraintAddEventsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( KheDistributeSplitEventsConstraintAppliesToCount(res) == 0 )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<DistributeSplitEventsConstraint> applies to 0 events");

  return true;
}

/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeSplitEventsConstraintWrite(                            */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, KML_FILE kf)                 */
/*                                                                           */
/*  Write c to kf.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheDistributeSplitEventsConstraintWrite(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, KML_FILE kf)
{
  KHE_EVENT_GROUP eg;  KHE_EVENT e;  int i;
  KmlBegin(kf, "DistributeSplitEventsConstraint");
  HnAssert(c->id != NULL,
    "KheArchiveWrite: Id missing in DistributeSplitEventsConstraint");
  KmlAttribute(kf, "Id", c->id);
  KheConstraintWriteCommonFields((KHE_CONSTRAINT) c, kf);
  KmlBegin(kf, "AppliesTo");
  if( HaArrayCount(c->event_groups) > 0 )
  {
    KmlBegin(kf, "EventGroups");
    HaArrayForEach(c->event_groups, eg, i)
    {
      HnAssert(KheEventGroupId(eg) != NULL, "KheArchiveWrite:  Id missing in "
        "EventGroup referenced from DistributeSplitEventsConstraint %s", c->id);
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
        " in Event referenced from DistributeSplitEventsConstraint %s", c->id);
      KmlEltAttribute(kf, "Event", "Reference", KheEventId(e));
    }
    KmlEnd(kf, "Events");
  }
  KmlEnd(kf, "AppliesTo");
  KmlEltFmtText(kf, "Duration", "%d", c->duration);
  KmlEltFmtText(kf, "Minimum", "%d", c->minimum);
  KmlEltFmtText(kf, "Maximum", "%d", c->maximum);
  KmlEnd(kf, "DistributeSplitEventsConstraint");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeSplitEventsConstraintDebug(                            */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, int verbosity,               */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of c onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheDistributeSplitEventsConstraintDebug(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, int verbosity,
  int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    KheConstraintDebugCommonFields((KHE_CONSTRAINT) c, indent, fp);
    if( indent >= 0 && verbosity >= 2 )
      fprintf(fp, "%*s[ duration %d, amount %d-%d ]\n", indent, "",
	c->duration, c->minimum, c->maximum);
  }
}

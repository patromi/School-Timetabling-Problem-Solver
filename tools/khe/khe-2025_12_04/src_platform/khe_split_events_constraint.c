
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
/*  FILE:         khs_split_events_constraint.c                              */
/*  DESCRIPTION:  A split events constraint                                  */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG6 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_EVENTS_CONSTRAINT - a split events constraint                  */
/*                                                                           */
/*****************************************************************************/

struct khe_split_events_constraint_rec {
  INHERIT_CONSTRAINT
  int				min_duration;		/* MinimumDuration   */
  int				max_duration;		/* MaximumDuration   */
  int				min_amount;		/* MinimumAmount     */
  int				max_amount;		/* MaximumAmount     */
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
/*  bool KheSplitEventsConstraintMake(KHE_INSTANCE ins, char *id,            */
/*    char *name, bool required, int weight, KHE_COST_FUNCTION cf,           */
/*    int min_duration, int max_duration, int min_amount, int max_amount,    */
/*    KHE_SPLIT_EVENTS_CONSTRAINT *c)                                        */
/*                                                                           */
/*  Make an split events constraint, add it to the instance, and return it.  */
/*                                                                           */
/*****************************************************************************/

bool KheSplitEventsConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  int min_duration, int max_duration, int min_amount, int max_amount,
  KHE_SPLIT_EVENTS_CONSTRAINT *c)
{
  KHE_SPLIT_EVENTS_CONSTRAINT res;  KHE_CONSTRAINT cc;  HA_ARENA a;
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KheSplitEventsConstraintMake called after KheInstanceMakeEnd");
  if( id != NULL && KheInstanceRetrieveConstraint(ins, id, &cc) )
  {
    *c = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  HaMake(res, a);
  KheConstraintInitCommonFields((KHE_CONSTRAINT) res,
    KHE_SPLIT_EVENTS_CONSTRAINT_TAG, ins, id, name, required, weight, cf, a);
  res->min_duration = min_duration;
  res->max_duration = max_duration;
  res->min_amount = min_amount;
  res->max_amount = max_amount;
  HaArrayInit(res->events, a);
  HaArrayInit(res->event_groups, a);
  KheInstanceAddConstraint(ins, (KHE_CONSTRAINT) res);
  *c = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSplitEventsConstraintMinDuration(KHE_SPLIT_EVENTS_CONSTRAINT c)   */
/*                                                                           */
/*  Return the MinDuration attribute of c.                                   */
/*                                                                           */
/*****************************************************************************/

int KheSplitEventsConstraintMinDuration(KHE_SPLIT_EVENTS_CONSTRAINT c)
{
  return c->min_duration;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSplitEventsConstraintMaxDuration(KHE_SPLIT_EVENTS_CONSTRAINT c)   */
/*                                                                           */
/*  Return the MaxDuration attribute of c.                                   */
/*                                                                           */
/*****************************************************************************/

int KheSplitEventsConstraintMaxDuration(KHE_SPLIT_EVENTS_CONSTRAINT c)
{
  return c->max_duration;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSplitEventsConstraintMinAmount(KHE_SPLIT_EVENTS_CONSTRAINT c)     */
/*                                                                           */
/*  Return the MinAmount attribute of c.                                     */
/*                                                                           */
/*****************************************************************************/

int KheSplitEventsConstraintMinAmount(KHE_SPLIT_EVENTS_CONSTRAINT c)
{
  return c->min_amount;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSplitEventsConstraintMaxAmount(KHE_SPLIT_EVENTS_CONSTRAINT c)     */
/*                                                                           */
/*  Return the MaxAmount attribute of c.                                     */
/*                                                                           */
/*****************************************************************************/

int KheSplitEventsConstraintMaxAmount(KHE_SPLIT_EVENTS_CONSTRAINT c)
{
  return c->max_amount;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSplitEventsConstraintAppliesToCount(KHE_SPLIT_EVENTS_CONSTRAINT c)*/
/*                                                                           */
/*  Return the number of points of application of c.                         */
/*                                                                           */
/*****************************************************************************/

int KheSplitEventsConstraintAppliesToCount(KHE_SPLIT_EVENTS_CONSTRAINT c)
{
  int i, res;  KHE_EVENT_GROUP eg;
  res = HaArrayCount(c->events);
  HaArrayForEach(c->event_groups, eg, i)
    res += KheEventGroupEventCount(eg);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitEventsConstraintFinalize(KHE_SPLIT_EVENTS_CONSTRAINT c)     */
/*                                                                           */
/*  Finalize c, since KheInstanceMakeEnd has been called.                    */
/*                                                                           */
/*****************************************************************************/

void KheSplitEventsConstraintFinalize(KHE_SPLIT_EVENTS_CONSTRAINT c)
{
  /* nothing to do in this case */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSplitEventsConstraintDensityCount(KHE_SPLIT_EVENTS_CONSTRAINT c)  */
/*                                                                           */
/*  Return the density count of c; just the applies to count in this case.   */
/*                                                                           */
/*****************************************************************************/

int KheSplitEventsConstraintDensityCount(KHE_SPLIT_EVENTS_CONSTRAINT c)
{
  return KheSplitEventsConstraintAppliesToCount(c);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "events"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSplitEventsConstraintAddEvent(KHE_SPLIT_EVENTS_CONSTRAINT c,     */
/*    KHE_EVENT e)                                                           */
/*                                                                           */
/*  Add e to c.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheSplitEventsConstraintAddEvent(KHE_SPLIT_EVENTS_CONSTRAINT c,
  KHE_EVENT e)
{
  HaArrayAddLast(c->events, e);
  KheEventAddConstraint(e, (KHE_CONSTRAINT) c);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSplitEventsConstraintEventCount(KHE_SPLIT_EVENTS_CONSTRAINT c)    */
/*                                                                           */
/*  Return the number of events of c.                                        */
/*                                                                           */
/*****************************************************************************/

int KheSplitEventsConstraintEventCount(KHE_SPLIT_EVENTS_CONSTRAINT c)
{
  return HaArrayCount(c->events);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheSplitEventsConstraintEvent(                                 */
/*    KHE_SPLIT_EVENTS_CONSTRAINT c, int i)                                  */
/*                                                                           */
/*  Return the i'th event of c.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KheSplitEventsConstraintEvent(KHE_SPLIT_EVENTS_CONSTRAINT c, int i)
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
/*  void KheSplitEventsConstraintAddEventGroup(KHE_SPLIT_EVENTS_CONSTRAINT c,*/
/*    KHE_EVENT_GROUP eg)                                                    */
/*                                                                           */
/*  Add eg to c, and also add the corresponding event resources to c.        */
/*                                                                           */
/*****************************************************************************/

void KheSplitEventsConstraintAddEventGroup(KHE_SPLIT_EVENTS_CONSTRAINT c,
  KHE_EVENT_GROUP eg)
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
/*  int KheSplitEventsConstraintEventGroupCount(                             */
/*    KHE_SPLIT_EVENTS_CONSTRAINT c)                                         */
/*                                                                           */
/*  Return the number of event groups of c.                                  */
/*                                                                           */
/*****************************************************************************/

int KheSplitEventsConstraintEventGroupCount(KHE_SPLIT_EVENTS_CONSTRAINT c)
{
  return HaArrayCount(c->event_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_GROUP KheSplitEventsConstraintEventGroup(                      */
/*    KHE_SPLIT_EVENTS_CONSTRAINT c, int i)                                  */
/*                                                                           */
/*  Return the ith event group of c.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_GROUP KheSplitEventsConstraintEventGroup(
  KHE_SPLIT_EVENTS_CONSTRAINT c, int i)
{
  return HaArray(c->event_groups, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitors"                                                     */
/*                                                                           */
/*****************************************************************************/

void KheSplitEventsConstraintMakeAndAttachMonitors(
  KHE_SPLIT_EVENTS_CONSTRAINT c, KHE_SOLN soln)
{
  int i, j;  KHE_EVENT_GROUP eg;  KHE_EVENT e;
  KHE_EVENT_IN_SOLN es;  KHE_SPLIT_EVENTS_MONITOR m;
  if( DEBUG6 )
    fprintf(stderr, "[ KheSolnMakeAndAttachSplitEventsConstraintMonitors()\n");
  HaArrayForEach(c->events, e, i)
  {
    if( DEBUG6 )
    {
      fprintf(stderr, "  event %d ", KheEventIndex(e));
      KheEventDebug(e, 1, 0, stderr);
    }
    /* es = HaArray(soln->events_in_soln, KheEventIndex(e)); */
    es = KheSolnEventInSoln(soln, e);
    m = KheSplitEventsMonitorMake(es, c);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
  HaArrayForEach(c->event_groups, eg, i)
  {
    for( j = 0;  j < KheEventGroupEventCount(eg);  j++ )
    {
      e = KheEventGroupEvent(eg, j);
      if( DEBUG6 )
      {
	fprintf(stderr, "    event group event %d ", KheEventIndex(e));
	KheEventDebug(e, 1, 0, stderr);
      }
      es = KheSolnEventInSoln(soln, e);
      /* es = HaArray(soln->events_in_soln, KheEventIndex(e)); */
      m = KheSplitEventsMonitorMake(es, c);
      KheMonitorAttachToSoln((KHE_MONITOR) m);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
    }
  }
  if( DEBUG6 )
    fprintf(stderr, "] KheSolnMakeAndAttachSplitEventsConstraintMonitors()\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitEventsConstraintMakeFromKml(KML_ELT cons_elt,               */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Add a split events constraint based on cons_elt to ins.                  */
/*                                                                           */
/*****************************************************************************/

bool KheSplitEventsConstraintMakeFromKml(KML_ELT cons_elt,
  KHE_INSTANCE ins, KML_ERROR *ke)
{
  char *id, *name;  bool reqd;  int wt;  KHE_COST_FUNCTION cf;
  KML_ELT elt;  KHE_SPLIT_EVENTS_CONSTRAINT res;  HA_ARENA a;
  int min_duration, max_duration, min_amount, max_amount;

  /* verify cons_elt and get the common fields */
  a = KheInstanceArena(ins);
  if( !KmlCheck(cons_elt,
      "Id : $Name $Required #Weight $CostFunction AppliesTo"
      " #MinimumDuration #MaximumDuration #MinimumAmount #MaximumAmount", ke) )
    return false;
  if( !KheConstraintCheckKml(cons_elt, &id, &name, &reqd, &wt, &cf, ke, a) )
    return false;

  /* find the numeric fields */
  sscanf(KmlText(KmlChild(cons_elt, 5)), "%d", &min_duration);
  sscanf(KmlText(KmlChild(cons_elt, 6)), "%d", &max_duration);
  sscanf(KmlText(KmlChild(cons_elt, 7)), "%d", &min_amount);
  sscanf(KmlText(KmlChild(cons_elt, 8)), "%d", &max_amount);

  /* make the constraint object and add it to ins */
  if( !KheSplitEventsConstraintMake(ins, id, name, reqd, wt, cf,
	min_duration, max_duration, min_amount, max_amount, &res) )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<SplitEventsConstraint> Id \"%s\" used previously", id);

  /* add the event groups and events */
  elt = KmlChild(cons_elt, 4);
  if( !KmlCheck(elt, ": +EventGroups +Events", ke) )
    return false;
  if( !KheConstraintAddEventGroupsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( !KheConstraintAddEventsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( KheSplitEventsConstraintAppliesToCount(res) == 0 )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<SplitEventsConstraint> applies to 0 events");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitEventsConstraintWrite(KHE_SPLIT_EVENTS_CONSTRAINT c,        */
/*    KML_FILE kf)                                                           */
/*                                                                           */
/*  Write c to kf.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheSplitEventsConstraintWrite(KHE_SPLIT_EVENTS_CONSTRAINT c,
  KML_FILE kf)
{
  KHE_EVENT_GROUP eg;  KHE_EVENT e;  int i;
  KmlBegin(kf, "SplitEventsConstraint");
  HnAssert(c->id != NULL,
    "KheArchiveWrite: Id missing in SplitEventsConstraint");
  KmlAttribute(kf, "Id", c->id);
  KheConstraintWriteCommonFields((KHE_CONSTRAINT) c, kf);
  KmlBegin(kf, "AppliesTo");
  if( HaArrayCount(c->event_groups) > 0 )
  {
    KmlBegin(kf, "EventGroups");
    HaArrayForEach(c->event_groups, eg, i)
    {
      HnAssert(KheEventGroupId(eg) != NULL, "KheArchiveWrite:  Id missing"
        " in EventGroup referenced from SplitEventsConstraint %s", c->id);
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
        " in Event referenced from SplitEventsConstraint %s", c->id);
      KmlEltAttribute(kf, "Event", "Reference", KheEventId(e));
    }
    KmlEnd(kf, "Events");
  }
  KmlEnd(kf, "AppliesTo");
  KmlEltFmtText(kf, "MinimumDuration", "%d", c->min_duration);
  KmlEltFmtText(kf, "MaximumDuration", "%d", c->max_duration);
  KmlEltFmtText(kf, "MinimumAmount", "%d", c->min_amount);
  KmlEltFmtText(kf, "MaximumAmount", "%d", c->max_amount);
  KmlEnd(kf, "SplitEventsConstraint");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSplitEventsConstraintDebug(KHE_SPLIT_EVENTS_CONSTRAINT c,        */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of c onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheSplitEventsConstraintDebug(KHE_SPLIT_EVENTS_CONSTRAINT c,
  int verbosity, int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    KheConstraintDebugCommonFields((KHE_CONSTRAINT) c, indent, fp);
    if( indent >= 0 && verbosity >= 2 )
      fprintf(fp, "%*s[ duration %d-%d, amount %d-%d ]\n", indent, "",
	c->min_duration, c->max_duration, c->min_amount, c->max_amount);
  }
}

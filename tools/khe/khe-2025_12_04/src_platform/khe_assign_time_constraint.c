
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
/*  FILE:         khs_assign_time_constraint.c                               */
/*  DESCRIPTION:  An assign time constraint                                  */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_ASSIGN_TIME_CONSTRAINT - an assign time constraint                   */
/*                                                                           */
/*****************************************************************************/

struct khe_assign_time_constraint_rec {
  INHERIT_CONSTRAINT
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
/*  bool KheAssignTimeConstraintMake(KHE_INSTANCE ins, char *id,             */
/*    char *name, bool required, int weight, KHE_COST_FUNCTION cf,           */
/*    KHE_ASSIGN_TIME_CONSTRAINT *c)                                         */
/*                                                                           */
/*  Make an assign resource constraint, add it to the instance, and          */
/*  return it.                                                               */
/*                                                                           */
/*****************************************************************************/

bool KheAssignTimeConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_ASSIGN_TIME_CONSTRAINT *c)
{
  KHE_ASSIGN_TIME_CONSTRAINT res;  KHE_CONSTRAINT cc;  HA_ARENA a;
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KheAssignTimeConstraintMake called after KheInstanceMakeEnd");
  if( id != NULL && KheInstanceRetrieveConstraint(ins, id, &cc) )
  {
    *c = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  HaMake(res, a);
  KheConstraintInitCommonFields((KHE_CONSTRAINT) res,
    KHE_ASSIGN_TIME_CONSTRAINT_TAG, ins, id, name, required, weight, cf, a);
  HaArrayInit(res->events, a);
  HaArrayInit(res->event_groups, a);
  KheInstanceAddConstraint(ins, (KHE_CONSTRAINT) res);
  *c = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAssignTimeConstraintAppliesToCount(KHE_ASSIGN_TIME_CONSTRAINT c)  */
/*                                                                           */
/*  Return the number of points of application of c.                         */
/*                                                                           */
/*****************************************************************************/

int KheAssignTimeConstraintAppliesToCount(KHE_ASSIGN_TIME_CONSTRAINT c)
{
  int i, res;  KHE_EVENT_GROUP eg;
  res = HaArrayCount(c->events);
  HaArrayForEach(c->event_groups, eg, i)
    res += KheEventGroupEventCount(eg);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignTimeConstraintFinalize(KHE_ASSIGN_TIME_CONSTRAINT c)       */
/*                                                                           */
/*  Finalize c, since KheInstanceMakeEnd has been called.                    */
/*                                                                           */
/*****************************************************************************/

void KheAssignTimeConstraintFinalize(KHE_ASSIGN_TIME_CONSTRAINT c)
{
  /* nothing to do in this case */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAssignTimeConstraintDensityCount(KHE_ASSIGN_TIME_CONSTRAINT c)    */
/*                                                                           */
/*  Return the density count of c; the same as AppliesToCount.               */
/*                                                                           */
/*****************************************************************************/

int KheAssignTimeConstraintDensityCount(KHE_ASSIGN_TIME_CONSTRAINT c)
{
  return KheAssignTimeConstraintAppliesToCount(c);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "events"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAssignTimeConstraintAddEvent(KHE_ASSIGN_TIME_CONSTRAINT c,       */
/*    KHE_EVENT e)                                                           */
/*                                                                           */
/*  Add e to c.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheAssignTimeConstraintAddEvent(KHE_ASSIGN_TIME_CONSTRAINT c, KHE_EVENT e)
{
  HaArrayAddLast(c->events, e);
  if( KheEventPreassignedTime(e) == NULL )
    KheEventAddConstraint(e, (KHE_CONSTRAINT) c);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAssignTimeConstraintEventCount(KHE_ASSIGN_TIME_CONSTRAINT c)      */
/*                                                                           */
/*  Return the number of events of c.                                        */
/*                                                                           */
/*****************************************************************************/

int KheAssignTimeConstraintEventCount(KHE_ASSIGN_TIME_CONSTRAINT c)
{
  return HaArrayCount(c->events);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheAssignTimeConstraintEvent(                                  */
/*    KHE_ASSIGN_TIME_CONSTRAINT c, int i)                                   */
/*                                                                           */
/*  Return the i'th event of c.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KheAssignTimeConstraintEvent(KHE_ASSIGN_TIME_CONSTRAINT c, int i)
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
/*  void KheAssignTimeConstraintAddEventGroup(KHE_ASSIGN_TIME_CONSTRAINT c,  */
/*    KHE_EVENT_GROUP eg)                                                    */
/*                                                                           */
/*  Add eg to c, and also add the corresponding event resources to c.        */
/*                                                                           */
/*****************************************************************************/

void KheAssignTimeConstraintAddEventGroup(KHE_ASSIGN_TIME_CONSTRAINT c,
  KHE_EVENT_GROUP eg)
{
  int i;  KHE_EVENT e;
  HaArrayAddLast(c->event_groups, eg);
  for( i = 0;  i < KheEventGroupEventCount(eg);  i++ )
  {
    e = KheEventGroupEvent(eg, i);
    if( KheEventPreassignedTime(e) == NULL )
      KheEventAddConstraint(e, (KHE_CONSTRAINT) c);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAssignTimeConstraintEventGroupCount(KHE_ASSIGN_TIME_CONSTRAINT c) */
/*                                                                           */
/*  Return the number of event groups of c.                                  */
/*                                                                           */
/*****************************************************************************/

int KheAssignTimeConstraintEventGroupCount(KHE_ASSIGN_TIME_CONSTRAINT c)
{
  return HaArrayCount(c->event_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_GROUP KheAssignTimeConstraintEventGroup(                       */
/*    KHE_ASSIGN_TIME_CONSTRAINT c, int i)                                   */
/*                                                                           */
/*  Return the ith event group of c.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_GROUP KheAssignTimeConstraintEventGroup(
  KHE_ASSIGN_TIME_CONSTRAINT c, int i)
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
/*  void KheAssignTimeConstraintMakeAndAttachMonitors(                       */
/*    KHE_ASSIGN_TIME_CONSTRAINT c, KHE_SOLN soln)                           */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

void KheAssignTimeConstraintMakeAndAttachMonitors(
  KHE_ASSIGN_TIME_CONSTRAINT c, KHE_SOLN soln)
{
  int i, j;  KHE_EVENT_GROUP eg;  KHE_EVENT e;
  KHE_EVENT_IN_SOLN es;  KHE_ASSIGN_TIME_MONITOR m;
  HaArrayForEach(c->events, e, i)
  {
    es = KheSolnEventInSoln(soln, e);
    /* es = HaArray(soln->events_in_soln, KheEventIndex(e)); */
    m = KheAssignTimeMonitorMake(es, c);
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
      m = KheAssignTimeMonitorMake(es, c);
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
/*  bool KheAssignTimeConstraintMakeFromKml(KML_ELT cons_elt,                */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Make an assign time constraint based on cons_elt and add it to ins.      */
/*                                                                           */
/*****************************************************************************/

bool KheAssignTimeConstraintMakeFromKml(KML_ELT cons_elt,
  KHE_INSTANCE ins, KML_ERROR *ke)
{
  char *id, *name;  bool reqd;  int wt;  KHE_COST_FUNCTION cf;
  KML_ELT elt;  KHE_ASSIGN_TIME_CONSTRAINT res;  HA_ARENA a;

  /* check cons_elt and get the common fields */
  a = KheInstanceArena(ins);
  if( !KmlCheck(cons_elt,
	"Id : $Name $Required #Weight $CostFunction AppliesTo", ke) )
    return false;
  if( !KheConstraintCheckKml(cons_elt, &id, &name, &reqd, &wt, &cf, ke, a) )
    return false;

  /* make the constraint object and add it to ins */
  if( !KheAssignTimeConstraintMake(ins, id, name, reqd, wt, cf, &res) )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<AssignTimeConstraint> Id \"%s\" used previously", id);

  /* add the event groups and events */
  elt = KmlChild(cons_elt, 4);
  if( !KmlCheck(elt, ": +EventGroups +Events", ke) )
    return false;
  if( !KheConstraintAddEventGroupsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( !KheConstraintAddEventsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( KheAssignTimeConstraintAppliesToCount(res) == 0 )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<AssignTimeConstraint> applies to 0 events");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignTimeConstraintWrite(KHE_ASSIGN_TIME_CONSTRAINT c,          */
/*    KML_FILE kf)                                                           */
/*                                                                           */
/*  Write c to kf.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheAssignTimeConstraintWrite(KHE_ASSIGN_TIME_CONSTRAINT c, KML_FILE kf)
{
  KHE_EVENT_GROUP eg;  KHE_EVENT e;  int i;
  KmlBegin(kf, "AssignTimeConstraint");
  HnAssert(c->id != NULL,
    "KheArchiveWrite: Id missing in AssignTimeConstraint");
  KmlAttribute(kf, "Id", c->id);
  KheConstraintWriteCommonFields((KHE_CONSTRAINT) c, kf);
  KmlBegin(kf, "AppliesTo");
  if( HaArrayCount(c->event_groups) > 0 )
  {
    KmlBegin(kf, "EventGroups");
    HaArrayForEach(c->event_groups, eg, i)
    {
      HnAssert(KheEventGroupId(eg) != NULL, "KheArchiveWrite:  Id missing"
        " in EventGroup referenced from AssignTimeConstraint %s", c->id);
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
        " in Event referenced from AssignTimeConstraint %s", c->id);
      KmlEltAttribute(kf, "Event", "Reference", KheEventId(e));
    }
    KmlEnd(kf, "Events");
  }
  KmlEnd(kf, "AppliesTo");
  KmlEnd(kf, "AssignTimeConstraint");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignTimeConstraintDebug(KHE_ASSIGN_TIME_CONSTRAINT c,          */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of c onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheAssignTimeConstraintDebug(KHE_ASSIGN_TIME_CONSTRAINT c,
  int verbosity, int indent, FILE *fp)
{
  KHE_EVENT_GROUP eg;  KHE_EVENT e;  int i;
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
      fprintf(fp, "%*s]\n", indent, "");
    }
  }
}

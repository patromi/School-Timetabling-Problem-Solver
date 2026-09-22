
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
/*  FILE:         khe_link_events_constraint.c                               */
/*  DESCRIPTION:  A link events constraint                                   */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_LINK_EVENTS_CONSTRAINT - a link events constraint                    */
/*                                                                           */
/*****************************************************************************/

struct khe_link_events_constraint_rec {
  INHERIT_CONSTRAINT
  ARRAY_KHE_EVENT_GROUP		event_groups;		/* applies to        */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLinkEventsConstraintMake(KHE_INSTANCE ins, char *id,             */
/*    char *name, bool required, int weight, KHE_COST_FUNCTION cf,           */
/*    KHE_LINK_EVENTS_CONSTRAINT *c)                                         */
/*                                                                           */
/*  Make a new link events constraint, add it to ins, and return it.         */
/*                                                                           */
/*****************************************************************************/

bool KheLinkEventsConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_LINK_EVENTS_CONSTRAINT *c)
{
  KHE_LINK_EVENTS_CONSTRAINT res;  KHE_CONSTRAINT cc;  HA_ARENA a;
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KheLinkEventsConstraintMake called after KheInstanceMakeEnd");
  if( id != NULL && KheInstanceRetrieveConstraint(ins, id, &cc) )
  {
    *c = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  HaMake(res, a);
  KheConstraintInitCommonFields((KHE_CONSTRAINT) res,
    KHE_LINK_EVENTS_CONSTRAINT_TAG, ins, id, name, required, weight, cf, a);
  res->index = KheInstanceConstraintCount(ins);
  HaArrayInit(res->event_groups, a);
  KheInstanceAddConstraint(ins, (KHE_CONSTRAINT) res);
  *c = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLinkEventsConstraintAppliesToCount(KHE_LINK_EVENTS_CONSTRAINT c)  */
/*                                                                           */
/*  Return the number of points of application of c.                         */
/*                                                                           */
/*****************************************************************************/

int KheLinkEventsConstraintAppliesToCount(KHE_LINK_EVENTS_CONSTRAINT c)
{
  return HaArrayCount(c->event_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLinkEventsConstraintFinalize(KHE_LINK_EVENTS_CONSTRAINT c)       */
/*                                                                           */
/*  Finalize c, since KheInstanceMakeEnd has been called.                    */
/*                                                                           */
/*****************************************************************************/

void KheLinkEventsConstraintFinalize(KHE_LINK_EVENTS_CONSTRAINT c)
{
  /* nothing to do in this case */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLinkEventsConstraintDensityCount(KHE_LINK_EVENTS_CONSTRAINT c)    */
/*                                                                           */
/*  Return the density count of c.                                           */
/*                                                                           */
/*****************************************************************************/

int KheLinkEventsConstraintDensityCount(KHE_LINK_EVENTS_CONSTRAINT c)
{
  int i, res;  KHE_EVENT_GROUP eg;
  res = 0;
  for( i = 0;  i < KheLinkEventsConstraintEventGroupCount(c);  i++ )
  {
    eg = KheLinkEventsConstraintEventGroup(c, i);
    res += KheEventGroupEventCount(eg);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "event groups"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLinkEventsConstraintAddEventGroup(                               */
/*    KHE_LINK_EVENTS_CONSTRAINT c, KHE_EVENT_GROUP eg)                      */
/*                                                                           */
/*  Add an event group to c.                                                 */
/*                                                                           */
/*****************************************************************************/

void KheLinkEventsConstraintAddEventGroup(KHE_LINK_EVENTS_CONSTRAINT c,
  KHE_EVENT_GROUP eg)
{
  KheEventGroupAddConstraint(eg, (KHE_CONSTRAINT) c);
  HaArrayAddLast(c->event_groups, eg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLinkEventsConstraintEventGroupCount(                              */
/*    KHE_LINK_EVENTS_CONSTRAINT c)                                          */
/*                                                                           */
/*  Return the number of event groups in c.                                  */
/*                                                                           */
/*****************************************************************************/

int KheLinkEventsConstraintEventGroupCount(KHE_LINK_EVENTS_CONSTRAINT c)
{
  return HaArrayCount(c->event_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_GROUP KheLinkEventsConstraintEventGroup(                       */
/*    KHE_LINK_EVENTS_CONSTRAINT c, int i)                                   */
/*                                                                           */
/*  Return the i'th event group of c.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_GROUP KheLinkEventsConstraintEventGroup(
  KHE_LINK_EVENTS_CONSTRAINT c, int i)
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
/*  void KheLinkEventsConstraintMakeAndAttachMonitors(                       */
/*    KHE_LINK_EVENTS_CONSTRAINT c, KHE_SOLN soln)                           */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

void KheLinkEventsConstraintMakeAndAttachMonitors(
  KHE_LINK_EVENTS_CONSTRAINT c, KHE_SOLN soln)
{
  int i;  KHE_EVENT_GROUP eg;  KHE_LINK_EVENTS_MONITOR m;
  HaArrayForEach(c->event_groups, eg, i)
  {
    m = KheLinkEventsMonitorMake(soln, c, eg);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLinkEventsConstraintMakeFromKml(KML_ELT cons_elt,                */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Make a link events constraint based on cons_elt and add it to ins.       */
/*                                                                           */
/*****************************************************************************/

bool KheLinkEventsConstraintMakeFromKml(KML_ELT cons_elt,
  KHE_INSTANCE ins, KML_ERROR *ke)
{
  char *id, *name;  bool reqd;  int wt;  KHE_COST_FUNCTION cf;
  KML_ELT elt;  KHE_LINK_EVENTS_CONSTRAINT res;  HA_ARENA a;

  /* verify cons_elt and get the common fields */
  a = KheInstanceArena(ins);
  if( !KmlCheck(cons_elt, "Id : $Name $Required #Weight "
      "$CostFunction AppliesTo", ke) )
    return false;
  if( !KheConstraintCheckKml(cons_elt, &id, &name, &reqd, &wt, &cf, ke, a) )
    return false;

  /* build and insert the constraint object */
  if( !KheLinkEventsConstraintMake(ins, id, name, reqd, wt, cf, &res) )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<LinkEventsConstraint> Id \"%s\" used previously", id);

  /* find event subgroups */
  elt = KmlChild(cons_elt, 4);
  if( !KmlCheck(elt, ": EventGroups", ke) )
    return false;
  if( !KheConstraintAddEventGroupsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( KheLinkEventsConstraintAppliesToCount(res) == 0 )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<LinkEventsConstraint> applies to 0 event groups");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLinkEventsConstraintWrite(KHE_LINK_EVENTS_CONSTRAINT c,          */
/*    KML_FILE kf)                                                           */
/*                                                                           */
/*  Write c to kf.                                                          */
/*                                                                           */
/*****************************************************************************/

void KheLinkEventsConstraintWrite(KHE_LINK_EVENTS_CONSTRAINT c, KML_FILE kf)
{
  KHE_EVENT_GROUP eg;  int i;
  KmlBegin(kf, "LinkEventsConstraint");
  HnAssert(c->id != NULL,
    "KheArchiveWrite: Id missing in LinkEventsConstraint");
  KmlAttribute(kf, "Id", c->id);
  KheConstraintWriteCommonFields((KHE_CONSTRAINT) c, kf);
  KmlBegin(kf, "AppliesTo");
  if( HaArrayCount(c->event_groups) > 0 )
  {
    KmlBegin(kf, "EventGroups");
    HaArrayForEach(c->event_groups, eg, i)
    {
      HnAssert(KheEventGroupId(eg) != NULL, "KheArchiveWrite:  Id missing"
        " in EventGroup referenced from LinkEventsConstraint %s", c->id);
      KmlEltAttribute(kf, "EventGroup", "Reference", KheEventGroupId(eg));
    }
    KmlEnd(kf, "EventGroups");
  }
  KmlEnd(kf, "AppliesTo");
  KmlEnd(kf, "LinkEventsConstraint");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLinkEventsConstraintDebug(KHE_LINK_EVENTS_CONSTRAINT c,          */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of c onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheLinkEventsConstraintDebug(KHE_LINK_EVENTS_CONSTRAINT c,
  int verbosity, int indent, FILE *fp)
{
  KHE_EVENT_GROUP eg;  int i;
  if( verbosity >= 1 )
  {
    KheConstraintDebugCommonFields((KHE_CONSTRAINT) c, indent, fp);
    if( indent >= 0 && verbosity >= 2 )
    {
      fprintf(fp, "%*s[\n", indent, "");
      HaArrayForEach(c->event_groups, eg, i)
	fprintf(fp, "%*s  %s\n", indent, "",
	  KheEventGroupId(eg) != NULL ? KheEventGroupId(eg) : "-");
      fprintf(fp, "%*s]\n", indent, "");
    }
  }
}

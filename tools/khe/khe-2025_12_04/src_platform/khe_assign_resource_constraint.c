
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
/*  FILE:         khs_assign_resource_constraint.c                           */
/*  DESCRIPTION:  An assign resource constraint                              */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG5 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_ASSIGN_RESOURCE_CONSTRAINT - an assign resource constraint           */
/*                                                                           */
/*****************************************************************************/

struct khe_assign_resource_constraint_rec {
  INHERIT_CONSTRAINT
  char				*role;			/* Role              */
  ARRAY_KHE_EVENT_RESOURCE	event_resources;	/* applies to        */
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
/*  bool KheAssignResourceConstraintMake(KHE_INSTANCE ins, char *id,         */
/*    char *name, bool required, int weight, KHE_COST_FUNCTION cf,           */
/*    char *role, KHE_ASSIGN_RESOURCE_CONSTRAINT *c)                         */
/*                                                                           */
/*  Make an assign resource constraint, add it to the instance, and          */
/*  return it.                                                               */
/*                                                                           */
/*****************************************************************************/

bool KheAssignResourceConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  char *role, KHE_ASSIGN_RESOURCE_CONSTRAINT *c)
{
  KHE_ASSIGN_RESOURCE_CONSTRAINT res;  KHE_CONSTRAINT cc;  HA_ARENA a;
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KheAssignResourceConstraintMake called after KheInstanceMakeEnd");
  if( id != NULL && KheInstanceRetrieveConstraint(ins, id, &cc) )
  {
    *c = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  HaMake(res, a);
  KheConstraintInitCommonFields((KHE_CONSTRAINT) res,
    KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG, ins, id, name, required, weight, cf, a);
  res->role = HnStringCopy(role, a);
  HaArrayInit(res->events, a);
  HaArrayInit(res->event_groups, a);
  HaArrayInit(res->event_resources, a);
  KheInstanceAddConstraint(ins, (KHE_CONSTRAINT) res);
  *c = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheAssignResourceConstraintRole(KHE_ASSIGN_RESOURCE_CONSTRAINT c)  */
/*                                                                           */
/*  Return the role attribute of c (possibly NULL).                          */
/*                                                                           */
/*****************************************************************************/

char *KheAssignResourceConstraintRole(KHE_ASSIGN_RESOURCE_CONSTRAINT c)
{
  return c->role;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAssignResourceConstraintAppliesToCount(                           */
/*    KHE_ASSIGN_RESOURCE_CONSTRAINT c)                                      */
/*                                                                           */
/*  Return the number of points of application of c.                         */
/*                                                                           */
/*****************************************************************************/

int KheAssignResourceConstraintAppliesToCount(KHE_ASSIGN_RESOURCE_CONSTRAINT c)
{
  return HaArrayCount(c->event_resources);
}


/*****************************************************************************/
/*                                                                           */
/* void KheAssignResourceConstraintFinalize(KHE_ASSIGN_RESOURCE_CONSTRAINT c)*/
/*                                                                           */
/*  Finalize c, since KheInstanceMakeEnd has been called.                    */
/*                                                                           */
/*****************************************************************************/

void KheAssignResourceConstraintFinalize(KHE_ASSIGN_RESOURCE_CONSTRAINT c)
{
  /* nothing to do in this case */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAssignResourceConstraintDoEquiv(                                 */
/*    KHE_ASSIGN_RESOURCE_CONSTRAINT c1, KHE_ASSIGN_RESOURCE_CONSTRAINT c2)  */
/*                                                                           */
/*  Assuming that their weight stuff is the same, return true if c1 and      */
/*  c1 are equivalent, in the sense that a penalty from one is the same      */
/*  as a penalty from the other.                                             */
/*                                                                           */
/*****************************************************************************/

bool KheAssignResourceConstraintDoEquiv(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c1, KHE_ASSIGN_RESOURCE_CONSTRAINT c2)
{
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAssignResourceConstraintDensityCount(                             */
/*    KHE_ASSIGN_RESOURCE_CONSTRAINT c)                                      */
/*                                                                           */
/*  Return the density count of c; just the number of event resources.       */
/*                                                                           */
/*****************************************************************************/

int KheAssignResourceConstraintDensityCount(KHE_ASSIGN_RESOURCE_CONSTRAINT c)
{
  return HaArrayCount(c->event_resources);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "event resources"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAssignResourceConstraintAddEventResource(                        */
/*    KHE_ASSIGN_RESOURCE_CONSTRAINT c, KHE_EVENT_RESOURCE er)               */
/*                                                                           */
/*  Add er to c, and also add c to er.                                       */
/*                                                                           */
/*****************************************************************************/

void KheAssignResourceConstraintAddEventResource(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c, KHE_EVENT_RESOURCE er)
{
  HaArrayAddLast(c->event_resources, er);
  KheEventResourceAddConstraint(er, (KHE_CONSTRAINT) c, -1);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAssignResourceConstraintEventResourceCount(                       */
/*    KHE_ASSIGN_RESOURCE_CONSTRAINT c)                                      */
/*                                                                           */
/*  Return the number of event resources that c applies to.                  */
/*                                                                           */
/*****************************************************************************/

int KheAssignResourceConstraintEventResourceCount(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c)
{
  return HaArrayCount(c->event_resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE KheAssignResourceConstraintEventResource(             */
/*    KHE_ASSIGN_RESOURCE_CONSTRAINT c, int i)                               */
/*                                                                           */
/*  Return the i'th event resource of c.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_RESOURCE KheAssignResourceConstraintEventResource(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c, int i)
{
  return HaArray(c->event_resources, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void AddEventResource(KHE_ASSIGN_RESOURCE_CONSTRAINT c, KHE_EVENT e)     */
/*                                                                           */
/*  Add the event resource corresponding to e to c, if any.                  */
/*                                                                           */
/*****************************************************************************/

static void AddEventResource(KHE_ASSIGN_RESOURCE_CONSTRAINT c, KHE_EVENT e)
{
  KHE_EVENT_RESOURCE er;
  if( c->role != NULL && KheEventRetrieveEventResource(e, c->role, &er) &&
      KheEventResourcePreassignedResource(er) == NULL )
    KheAssignResourceConstraintAddEventResource(c, er);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "events"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAssignResourceConstraintAddEvent(                                */
/*    KHE_ASSIGN_RESOURCE_CONSTRAINT c, KHE_EVENT e)                         */
/*                                                                           */
/*  Add e to c, and also add the corresponding event resource.               */
/*                                                                           */
/*****************************************************************************/

void KheAssignResourceConstraintAddEvent(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c, KHE_EVENT e)
{
  HaArrayAddLast(c->events, e);
  AddEventResource(c, e);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAssignResourceConstraintEventCount(                               */
/*    KHE_ASSIGN_RESOURCE_CONSTRAINT c)                                      */
/*                                                                           */
/*  Return the number of events of c.                                        */
/*                                                                           */
/*****************************************************************************/

int KheAssignResourceConstraintEventCount(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c)
{
  return HaArrayCount(c->events);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheAssignResourceConstraintEvent(                              */
/*    KHE_ASSIGN_RESOURCE_CONSTRAINT c, int i)                               */
/*                                                                           */
/*  Return the i'th event of c.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KheAssignResourceConstraintEvent(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c, int i)
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
/*  void KheAssignResourceConstraintAddEventGroup(                           */
/*    KHE_ASSIGN_RESOURCE_CONSTRAINT c, KHE_EVENT_GROUP eg)                  */
/*                                                                           */
/*  Add eg to c, and also add the corresponding event resources to c.        */
/*                                                                           */
/*****************************************************************************/

void KheAssignResourceConstraintAddEventGroup(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c, KHE_EVENT_GROUP eg)
{
  int i;
  HaArrayAddLast(c->event_groups, eg);
  for( i = 0;  i < KheEventGroupEventCount(eg);  i++ )
    AddEventResource(c, KheEventGroupEvent(eg, i));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAssignResourceConstraintEventGroupCount(                          */
/*    KHE_ASSIGN_RESOURCE_CONSTRAINT c)                                      */
/*                                                                           */
/*  Return the number of event groups of c.                                  */
/*                                                                           */
/*****************************************************************************/

int KheAssignResourceConstraintEventGroupCount(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c)
{
  return HaArrayCount(c->event_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_GROUP KheAssignResourceConstraintEventGroup(                   */
/*    KHE_ASSIGN_RESOURCE_CONSTRAINT c, int i)                               */
/*                                                                           */
/*  Return the ith event group of c.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_GROUP KheAssignResourceConstraintEventGroup(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c, int i)
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
/*  void KheSolnMakeAndAttachAssignResourceConstraintMonitors(               */
/*    KHE_SOLN soln, KHE_ASSIGN_RESOURCE_CONSTRAINT c)                       */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

void KheAssignResourceConstraintMakeAndAttachMonitors(
  KHE_ASSIGN_RESOURCE_CONSTRAINT c, KHE_SOLN soln)
{
  int i;  KHE_EVENT_RESOURCE er;
  KHE_EVENT_RESOURCE_IN_SOLN ers;  KHE_ASSIGN_RESOURCE_MONITOR m;
  HaArrayForEach(c->event_resources, er, i)
  {
    ers = KheSolnEventResourceInSoln(soln, er);
    m = KheAssignResourceMonitorMake(ers, c);
    if( DEBUG5 )
      fprintf(stderr, "new KheAssignResourceMonitor %p (attached %d)\n",
	(void *) m, (int) KheMonitorAttachedToSoln((KHE_MONITOR) m));
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
/*  bool KheAssignResourceConstraintMakeFromKml(KML_ELT cons_elt,            */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Make an assign resource constraint from cons_elt and add it to ins.      */
/*                                                                           */
/*****************************************************************************/

bool KheAssignResourceConstraintMakeFromKml(KML_ELT cons_elt,
  KHE_INSTANCE ins, KML_ERROR *ke)
{
  char *id, *name;  bool reqd;  int wt;  KHE_COST_FUNCTION cf;
  KML_ELT elt;  KHE_ASSIGN_RESOURCE_CONSTRAINT res;  HA_ARENA a;

  /* check cons_elt and get the common fields */
  a = KheInstanceArena(ins);
  if( !KmlCheck(cons_elt,
	"Id : $Name $Required #Weight $CostFunction AppliesTo $Role", ke) )
    return false;
  if( !KheConstraintCheckKml(cons_elt, &id, &name, &reqd, &wt, &cf, ke, a) )
    return false;

  /* make the constraint object and add it to ins */
  if( !KheAssignResourceConstraintMake(ins, id, name, reqd, wt, cf,
	KmlText(KmlChild(cons_elt, 5)), &res) )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<AssignResourceConstraint> Id \"%s\" used previously", id);

  /* add the event groups and events */
  elt = KmlChild(cons_elt, 4);
  if( !KmlCheck(elt, ": +EventGroups +Events", ke) )
    return false;
  if( !KheConstraintAddEventGroupsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( !KheConstraintAddEventsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( KheAssignResourceConstraintAppliesToCount(res) == 0 )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<AssignResourceConstraint> applies to 0 event resources");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignResourceConstraintWrite(KHE_ASSIGN_RESOURCE_CONSTRAINT c,  */
/*    KML_FILE kf)                                                           */
/*                                                                           */
/*  Write c to kf.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheAssignResourceConstraintWrite(KHE_ASSIGN_RESOURCE_CONSTRAINT c,
  KML_FILE kf)
{
  KHE_EVENT_GROUP eg;  KHE_EVENT e;  int i;
  KmlBegin(kf, "AssignResourceConstraint");
  HnAssert(c->id != NULL,
    "KheArchiveWrite: Id missing in AssignResourceConstraint");
  KmlAttribute(kf, "Id", c->id);
  KheConstraintWriteCommonFields((KHE_CONSTRAINT) c, kf);
  KmlBegin(kf, "AppliesTo");
  if( HaArrayCount(c->event_groups) > 0 )
  {
    KmlBegin(kf, "EventGroups");
    HaArrayForEach(c->event_groups, eg, i)
    {
      HnAssert(KheEventGroupId(eg) != NULL, "KheArchiveWrite:  Id missing"
        " in EventGroup referenced from AssignResourceConstraint %s", c->id);
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
        " in Event referenced from AssignResourceConstraint %s", c->id);
      KmlEltAttribute(kf, "Event", "Reference", KheEventId(e));
    }
    KmlEnd(kf, "Events");
  }
  KmlEnd(kf, "AppliesTo");
  HnAssert(c->role != NULL,
    "KheArchiveWrite: Role missing in AssignResourceConstraint %s", c->id);
  KmlEltPlainText(kf, "Role", c->role);
  KmlEnd(kf, "AssignResourceConstraint");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignResourceConstraintDebug(KHE_ASSIGN_RESOURCE_CONSTRAINT c,  */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of c onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheAssignResourceConstraintDebug(KHE_ASSIGN_RESOURCE_CONSTRAINT c,
  int verbosity, int indent, FILE *fp)
{
  KHE_EVENT_GROUP eg;  KHE_EVENT e;  int i;
  if( verbosity >= 1 )
  {
    KheConstraintDebugCommonFields((KHE_CONSTRAINT) c, indent, fp);
    if( indent >= 0 && verbosity >= 2 )
    {
      fprintf(fp, "%*s[ %s:\n", indent, "", c->role);
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

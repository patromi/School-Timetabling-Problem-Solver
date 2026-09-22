
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
/*  FILE:         khe_avoid_split_assignments_constraint.c                   */
/*  DESCRIPTION:  An avoid split assignments constraint                      */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"


/*****************************************************************************/
/*                                                                           */
/*  ASAC_POINT - one point of application of an avoid split assignments c.   */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_asac_point_rec {
  KHE_EVENT_GROUP		event_group;		/* event group       */
  ARRAY_KHE_EVENT_RESOURCE	event_resources;	/* event resources   */
} *ASAC_POINT;

typedef HA_ARRAY(ASAC_POINT) ARRAY_ASAC_POINT;


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT - an avoid split assts constraint */
/*                                                                           */
/*****************************************************************************/

struct khe_avoid_split_assignments_constraint_rec {
  INHERIT_CONSTRAINT
  char				*role;			/* Role              */
  ARRAY_ASAC_POINT		asac_points;		/* points of applc.  */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidSplitAssignmentsConstraintMake(KHE_INSTANCE ins, char *id,  */
/*    char *name, bool required, int weight, KHE_COST_FUNCTION cf,           */
/*    char *role, KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT *c)                 */
/*                                                                           */
/*  Make an avoid split assignments constraint, add it to the instance, and  */
/*  return it.                                                               */
/*                                                                           */
/*****************************************************************************/

bool KheAvoidSplitAssignmentsConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  char *role, KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT *c)
{
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT res;  KHE_CONSTRAINT cc;  HA_ARENA a;
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KheAvoidSplitAssignmentsConstraintMake called after KheInstanceMakeEnd");
  if( id != NULL && KheInstanceRetrieveConstraint(ins, id, &cc) )
  {
    *c = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  HaMake(res, a);
  KheConstraintInitCommonFields((KHE_CONSTRAINT) res,
   KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG, ins, id, name, required,
   weight, cf, a);
  res->role = HnStringCopy(role, a);
  HaArrayInit(res->asac_points, a);
  KheInstanceAddConstraint(ins, (KHE_CONSTRAINT) res);
  *c = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheAvoidSplitAssignmentsConstraintRole(                            */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the role attribute of c.                                          */
/*                                                                           */
/*****************************************************************************/

char *KheAvoidSplitAssignmentsConstraintRole(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c)
{
  return c->role;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidSplitAssignmentsConstraintAppliesToCount(                    */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the number of points of application of c.                         */
/*                                                                           */
/*****************************************************************************/

int KheAvoidSplitAssignmentsConstraintAppliesToCount(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c)
{
  return HaArrayCount(c->asac_points);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsConstraintFinalize(                         */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c)                              */
/*                                                                           */
/*  Finalize c, since KheInstanceMakeEnd has been called.                    */
/*                                                                           */
/*****************************************************************************/

void KheAvoidSplitAssignmentsConstraintFinalize(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c)
{
  /* nothing to do in this case */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidSplitAssignmentsConstraintDoEquiv(                          */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c1,                             */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c2)                             */
/*                                                                           */
/*  These can't be equivalent, because pre-existing assignments to one       */
/*  but not the other can lead to different costs.                           */
/*                                                                           */
/*****************************************************************************/

bool KheAvoidSplitAssignmentsConstraintDoEquiv(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c1,
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c2)
{
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidSplitAssignmentsConstraintDensityCount(                      */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the density count of c.                                           */
/*                                                                           */
/*****************************************************************************/

int KheAvoidSplitAssignmentsConstraintDensityCount(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c)
{
  int i, res;
  res = 0;
  for( i = 0; i < KheAvoidSplitAssignmentsConstraintEventGroupCount(c);  i++ )
    res += KheAvoidSplitAssignmentsConstraintEventResourceCount(c, i);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "event groups"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidSplitAssignmentsConstraintAddEventGroup(                    */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, KHE_EVENT_GROUP eg,          */
/*    KHE_EVENT *problem_event)                                              */
/*                                                                           */
/*  Add eg to c.                                                             */
/*                                                                           */
/*****************************************************************************/

bool KheAvoidSplitAssignmentsConstraintAddEventGroup(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, KHE_EVENT_GROUP eg,
  KHE_EVENT *problem_event)
{
  ASAC_POINT ap;  int i;  KHE_EVENT e;  KHE_EVENT_RESOURCE er;  HA_ARENA a;
  a = KheInstanceArena(c->instance);
  HaMake(ap, a);
  ap->event_group = eg;  /* could be NULL */
  HaArrayInit(ap->event_resources, a);
  HaArrayAddLast(c->asac_points, ap);
  if( eg != NULL )
  {
    KheEventGroupAddConstraint(eg, (KHE_CONSTRAINT) c);
    if( c->role != NULL )
    {
      for( i = 0;  i < KheEventGroupEventCount(eg);  i++ )
      {
	e = KheEventGroupEvent(eg, i);
	if( !KheEventRetrieveEventResource(e, c->role, &er) )
	{
	  *problem_event = e;
	  return false;
	}
	KheAvoidSplitAssignmentsConstraintAddEventResource(c,
	  HaArrayCount(c->asac_points) - 1, er);
      }
    }
  }
  *problem_event = NULL;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidSplitAssignmentsConstraintEventGroupCount(                   */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the number of event groups of c.                                  */
/*                                                                           */
/*****************************************************************************/

int KheAvoidSplitAssignmentsConstraintEventGroupCount(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c)
{
  return HaArrayCount(c->asac_points);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_GROUP KheAvoidSplitAssignmentsConstraintEventGroup(            */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, int i)                       */
/*                                                                           */
/*  Return the                                                               */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_GROUP KheAvoidSplitAssignmentsConstraintEventGroup(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, int i)
{
  return HaArray(c->asac_points, i)->event_group;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "event resources"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsConstraintAddEventResource(                 */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, int eg_index,                */
/*    KHE_EVENT_RESOURCE er)                                                 */
/*                                                                           */
/*  Add er to the eg_index's set of event resources of c.                    */
/*                                                                           */
/*****************************************************************************/

void KheAvoidSplitAssignmentsConstraintAddEventResource(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, int eg_index,
  KHE_EVENT_RESOURCE er)
{
  ASAC_POINT ap;
  ap = HaArray(c->asac_points, eg_index);
  HaArrayAddLast(ap->event_resources, er);
  if( HaArrayCount(ap->event_resources) == 1 )
    KheResourceTypeIncrementAvoidSplitAssignmentsCount(
      KheEventResourceResourceType(er));
  KheEventResourceAddConstraint(er, (KHE_CONSTRAINT) c, eg_index);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidSplitAssignmentsConstraintEventResourceCount(                */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, int eg_index)                */
/*                                                                           */
/*  Return the number of event resources of the eg_index'th set of event     */
/*  resources of c.                                                          */
/*                                                                           */
/*****************************************************************************/

int KheAvoidSplitAssignmentsConstraintEventResourceCount(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, int eg_index)
{
  ASAC_POINT ap;
  ap = HaArray(c->asac_points, eg_index);
  return HaArrayCount(ap->event_resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE KheAvoidSplitAssignmentsConstraintEventResource(      */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, int eg_index, int er_index)  */
/*                                                                           */
/*  Return the er_index'th event resource of the eg_index'th set of event    */
/*  resources of c.                                                          */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_RESOURCE KheAvoidSplitAssignmentsConstraintEventResource(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, int eg_index, int er_index)
{
  ASAC_POINT ap;
  ap = HaArray(c->asac_points, eg_index);
  return HaArray(ap->event_resources, er_index);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitors"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsConstraintMakeAndAttachMonitors(            */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, KHE_SOLN soln)               */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

void KheAvoidSplitAssignmentsConstraintMakeAndAttachMonitors(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, KHE_SOLN soln)
{
  int i;  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m;
  for( i = 0;  i < KheAvoidSplitAssignmentsConstraintEventGroupCount(c);  i++ )
  {
    m = KheAvoidSplitAssignmentsMonitorMake(soln, c, i);
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
/*  bool KheAvoidSplitAssignmentsConstraintMakeFromKml(KML_ELT cons_elt,     */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Make an avoid split assignments constraint based on cons_elt and         */
/*  add it to ins.                                                           */
/*                                                                           */
/*****************************************************************************/

bool KheAvoidSplitAssignmentsConstraintMakeFromKml(KML_ELT cons_elt,
  KHE_INSTANCE ins, KML_ERROR *ke)
{
  char *id, *name;  bool reqd;  int wt;  KHE_COST_FUNCTION cf;
  KML_ELT elt;  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT res;  HA_ARENA a;

  /* verify cons_elt and get the common fields */
  a = KheInstanceArena(ins);
  if( !KmlCheck(cons_elt, "Id : $Name $Required #Weight "
      "$CostFunction AppliesTo $Role", ke) )
    return false;
  if( !KheConstraintCheckKml(cons_elt, &id, &name, &reqd, &wt, &cf, ke, a) )
    return false;

  /* build and insert the constraint object */
  if( !KheAvoidSplitAssignmentsConstraintMake(ins, id, name, reqd, wt, cf,
        KmlText(KmlChild(cons_elt, 5)), &res) )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<AvoidSplitAssignmentsConstraint> Id \"%s\" used previously", id);

  /* add the event groups */
  elt = KmlChild(cons_elt, 4);
  if( !KmlCheck(elt, ": EventGroups", ke) )
    return false;
  if( !KheConstraintAddEventGroupsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( KheAvoidSplitAssignmentsConstraintAppliesToCount(res) == 0 )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<AvoidSplitAssignmentsConstraint> applies to 0 event groups");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsConstraintWrite(                            */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, KML_FILE kf)                 */
/*                                                                           */
/*  Write c to kf.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheAvoidSplitAssignmentsConstraintWrite(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, KML_FILE kf)
{
  ASAC_POINT ap;  int i;
  HnAssert(c->id != NULL,
    "KheArchiveWrite: Id missing in AvoidSplitAssignmentsConstraint");
  KmlBegin(kf, "AvoidSplitAssignmentsConstraint");
  KmlAttribute(kf, "Id", c->id);
  KheConstraintWriteCommonFields((KHE_CONSTRAINT) c, kf);
  KmlBegin(kf, "AppliesTo");
  if( HaArrayCount(c->asac_points) > 0 )
  {
    KmlBegin(kf, "EventGroups");
    HaArrayForEach(c->asac_points, ap, i)
    {
      HnAssert(ap->event_group!=NULL && KheEventGroupId(ap->event_group) != NULL,
	"KheArchiveWrite:  Id missing in EventGroup referenced from"
	" AvoidSplitAssignmentsConstraint %s", c->id);
      KmlEltAttribute(kf, "EventGroup", "Reference",
	KheEventGroupId(ap->event_group));
    }
    KmlEnd(kf, "EventGroups");
  }
  KmlEnd(kf, "AppliesTo");
  HnAssert(c->role != NULL, "KheArchiveWrite: Role missing in "
    "AvoidSplitAssignmentsConstraint %s", c->id);
  KmlEltPlainText(kf, "Role", c->role);
  KmlEnd(kf, "AvoidSplitAssignmentsConstraint");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsConstraintDebug(                            */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c,                              */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of c onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheAvoidSplitAssignmentsConstraintDebug(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c,
  int verbosity, int indent, FILE *fp)
{
  ASAC_POINT ap;  int i;
  if( verbosity >= 1 )
  {
    KheConstraintDebugCommonFields((KHE_CONSTRAINT) c, indent, fp);
    if( indent >= 0 && verbosity >= 2 )
    {
      fprintf(fp, "%*s[ %s:\n", indent, "", c->role);
      HaArrayForEach(c->asac_points, ap, i)
	fprintf(fp, "%*s  %s\n", indent, "",
	  KheEventGroupId(ap->event_group) != NULL ?
	  KheEventGroupId(ap->event_group) : "-");
      fprintf(fp, "%*s]\n", indent, "");
    }
  }
}

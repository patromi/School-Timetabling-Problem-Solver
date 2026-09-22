
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
/*  FILE:         khe_constraint.c                                           */
/*  DESCRIPTION:  A constraint (abstract supertype)                          */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG12 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT - a constraint (abstract supertype)                       */
/*                                                                           */
/*****************************************************************************/

struct khe_constraint_rec {
  INHERIT_CONSTRAINT
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintInitCommonFields(KHE_CONSTRAINT c,                     */
/*    KHE_CONSTRAINT_TAG tag, KHE_INSTANCE ins, char *id, char *name,        */
/*    bool required, int weight, KHE_COST_FUNCTION cf)                       */
/*                                                                           */
/*  Initialize the fields of c common to all constraints.                    */
/*                                                                           */
/*****************************************************************************/

void KheConstraintInitCommonFields(KHE_CONSTRAINT c,
  KHE_CONSTRAINT_TAG tag, KHE_INSTANCE ins, char *id, char *name,
  bool required, int weight, KHE_COST_FUNCTION cf, HA_ARENA a)
{
  c->back = NULL;
  c->tag = tag;
  c->instance = ins;
  c->id = HnStringCopy(id, a);
  c->name = HnStringCopy(name, a);
  c->required = required;
  c->weight = weight;
  c->combined_weight = required ? KheCost(weight, 0) : KheCost(0, weight);
  c->cost_function = cf;
  c->index = KheInstanceConstraintCount(ins);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintSetBack(KHE_CONSTRAINT c, void *back)                  */
/*                                                                           */
/*  Set the back pointer of c.                                               */
/*                                                                           */
/*****************************************************************************/

void KheConstraintSetBack(KHE_CONSTRAINT c, void *back)
{
  HnAssert(KheInstanceFinalized(c->instance) == KHE_FINALIZED_NONE,
    "KheConstraintSetBack called after KheInstanceMakeEnd");
  c->back = back;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheConstraintBack(KHE_CONSTRAINT c)                                */
/*                                                                           */
/*  Return the back pointer of c.                                            */
/*                                                                           */
/*****************************************************************************/

void *KheConstraintBack(KHE_CONSTRAINT c)
{
  return c->back;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INSTANCE KheConstraintInstance(KHE_CONSTRAINT c)                     */
/*                                                                           */
/*  Return the instance attribute of c.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_INSTANCE KheConstraintInstance(KHE_CONSTRAINT c)
{
  return c->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheConstraintId(KHE_CONSTRAINT c)                                  */
/*                                                                           */
/*  Return the id attribute of c.                                            */
/*                                                                           */
/*****************************************************************************/

char *KheConstraintId(KHE_CONSTRAINT c)
{
  return c->id;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheConstraintName(KHE_CONSTRAINT c)                                */
/*                                                                           */
/*  Return the name attribute of c.                                          */
/*                                                                           */
/*****************************************************************************/

char *KheConstraintName(KHE_CONSTRAINT c)
{
  return c->name;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintRequired(KHE_CONSTRAINT c)                             */
/*                                                                           */
/*  Return the required attribute of c.                                      */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintRequired(KHE_CONSTRAINT c)
{
  return c->required;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintWeight(KHE_CONSTRAINT c)                                */
/*                                                                           */
/*  Return the weight of c.                                                  */
/*                                                                           */
/*****************************************************************************/

int KheConstraintWeight(KHE_CONSTRAINT c)
{
  return c->weight;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheConstraintCombinedWeight(KHE_CONSTRAINT c)                   */
/*                                                                           */
/*  Return the combined weight of c.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheConstraintCombinedWeight(KHE_CONSTRAINT c)
{
  return c->combined_weight;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST_FUNCTION KheConstraintCostFunction(KHE_CONSTRAINT c)            */
/*                                                                           */
/*  Return the cost function of c.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_COST_FUNCTION KheConstraintCostFunction(KHE_CONSTRAINT c)
{
  return c->cost_function;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheConstraintDevToCost(KHE_CONSTRAINT c, int dev)               */
/*                                                                           */
/*  Convert dev into a cost for c.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheConstraintDevToCost(KHE_CONSTRAINT c, int dev)
{
  switch( c->cost_function )
  {
    case KHE_STEP_COST_FUNCTION:

        return dev > 0 ? c->combined_weight : 0;

    case KHE_LINEAR_COST_FUNCTION:

        return dev * c->combined_weight;

    case KHE_QUADRATIC_COST_FUNCTION:

	return dev * dev * c->combined_weight;

    default:

	HnAbort("KheConstraintDevToCost internal error");
	return 0;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintIndex(KHE_CONSTRAINT c)                                 */
/*                                                                           */
/*  Return the index number of c.                                            */
/*                                                                           */
/*****************************************************************************/

int KheConstraintIndex(KHE_CONSTRAINT c)
{
  return c->index;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT_TAG KheConstraintTag(KHE_CONSTRAINT c)                    */
/*                                                                           */
/*  Return the tag of c, indicating which kind of constraint it is.          */
/*                                                                           */
/*****************************************************************************/

KHE_CONSTRAINT_TAG KheConstraintTag(KHE_CONSTRAINT c)
{
  return c->tag;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintAppliesToCount(KHE_CONSTRAINT c)                        */
/*                                                                           */
/*  Return the number of points of application of c.                         */
/*                                                                           */
/*****************************************************************************/

int KheConstraintAppliesToCount(KHE_CONSTRAINT c)
{
  switch( c->tag )
  {
    case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:
      return KheAssignResourceConstraintAppliesToCount(
	(KHE_ASSIGN_RESOURCE_CONSTRAINT) c);

    case KHE_ASSIGN_TIME_CONSTRAINT_TAG:
      return KheAssignTimeConstraintAppliesToCount(
	(KHE_ASSIGN_TIME_CONSTRAINT) c);

    case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:
      return KheSplitEventsConstraintAppliesToCount(
	(KHE_SPLIT_EVENTS_CONSTRAINT) c);

    case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:
      return KheDistributeSplitEventsConstraintAppliesToCount(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT) c);

    case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:
      return KhePreferResourcesConstraintAppliesToCount(
	(KHE_PREFER_RESOURCES_CONSTRAINT) c);

    case KHE_PREFER_TIMES_CONSTRAINT_TAG:
      return KhePreferTimesConstraintAppliesToCount(
	(KHE_PREFER_TIMES_CONSTRAINT) c);

    case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:
      return KheAvoidSplitAssignmentsConstraintAppliesToCount(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT) c);

    case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:
      return KheSpreadEventsConstraintAppliesToCount(
	(KHE_SPREAD_EVENTS_CONSTRAINT) c);

    case KHE_LINK_EVENTS_CONSTRAINT_TAG:
      return KheLinkEventsConstraintAppliesToCount(
	(KHE_LINK_EVENTS_CONSTRAINT) c);

    case KHE_ORDER_EVENTS_CONSTRAINT_TAG:
      return KheOrderEventsConstraintAppliesToCount(
	(KHE_ORDER_EVENTS_CONSTRAINT) c);

    case KHE_AVOID_CLASHES_CONSTRAINT_TAG:
      return KheAvoidClashesConstraintAppliesToCount(
	(KHE_AVOID_CLASHES_CONSTRAINT) c);

    case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:
      return KheAvoidUnavailableTimesConstraintAppliesToCount(
	(KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT) c);

    case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:
      return KheLimitIdleTimesConstraintAppliesToCount(
	(KHE_LIMIT_IDLE_TIMES_CONSTRAINT) c);

    case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:
      return KheClusterBusyTimesConstraintAppliesToCount(
	(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c);

    case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:
      return KheLimitBusyTimesConstraintAppliesToCount(
	(KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c);

    case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:
      return KheLimitWorkloadConstraintAppliesToCount(
	(KHE_LIMIT_WORKLOAD_CONSTRAINT) c);

    case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:
      return KheLimitActiveIntervalsConstraintAppliesToCount(
	(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c);

    case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:
      return KheLimitResourcesConstraintAppliesToCount(
	(KHE_LIMIT_RESOURCES_CONSTRAINT) c);

    default:
      HnAbort("KheConstraintAppliesToCount internal error");
      return 0;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheConstraintTagShow(KHE_CONSTRAINT_TAG tag)                       */
/*                                                                           */
/*  Show the name of tag.                                                    */
/*                                                                           */
/*****************************************************************************/

char *KheConstraintTagShow(KHE_CONSTRAINT_TAG tag)
{
  switch( tag )
  {
    case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:
      return "AssignResourceConstraint";

    case KHE_ASSIGN_TIME_CONSTRAINT_TAG:
      return "AssignTimeConstraint";

    case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:
      return "SplitEventsConstraint";

    case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:
      return "DistributeSplitEventsConstraint";

    case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:
      return "PreferResourcesConstraint";

    case KHE_PREFER_TIMES_CONSTRAINT_TAG:
      return "PreferTimesConstraint";

    case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:
      return "AvoidSplitAssignmentsConstraint";

    case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:
      return "SpreadEventsConstraint";

    case KHE_LINK_EVENTS_CONSTRAINT_TAG:
      return "LinkEventsConstraint";

    case KHE_ORDER_EVENTS_CONSTRAINT_TAG:
      return "OrderEventsConstraint";

    case KHE_AVOID_CLASHES_CONSTRAINT_TAG:
      return "AvoidClashesConstraint";

    case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:
      return "AvoidUnavailableTimesConstraint";

    case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:
      return "LimitIdleTimesConstraint";

    case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:
      return "ClusterBusyTimesConstraint";

    case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:
      return "LimitBusyTimesConstraint";

    case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:
      return "LimitWorkloadConstraint";

    case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:
      return "LimitActiveIntervalsConstraint";

    case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:
      return "LimitResourcesConstraint";

    default: return "?";
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheConstraintTagShowSpaced(KHE_CONSTRAINT_TAG tag)                 */
/*                                                                           */
/*  Show the name of tag.                                                    */
/*                                                                           */
/*****************************************************************************/

char *KheConstraintTagShowSpaced(KHE_CONSTRAINT_TAG tag)
{
  switch( tag )
  {
    case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:
      return "Assign Resource Constraint";

    case KHE_ASSIGN_TIME_CONSTRAINT_TAG:
      return "Assign Time Constraint";

    case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:
      return "Split Events Constraint";

    case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:
      return "Distribute Split Events Constraint";

    case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:
      return "Prefer Resources Constraint";

    case KHE_PREFER_TIMES_CONSTRAINT_TAG:
      return "Prefer Times Constraint";

    case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:
      return "Avoid Split Assignments Constraint";

    case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:
      return "Spread Events Constraint";

    case KHE_LINK_EVENTS_CONSTRAINT_TAG:
      return "Link Events Constraint";

    case KHE_ORDER_EVENTS_CONSTRAINT_TAG:
      return "Order Events Constraint";

    case KHE_AVOID_CLASHES_CONSTRAINT_TAG:
      return "Avoid Clashes Constraint";

    case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:
      return "Avoid Unavailable Times Constraint";

    case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:
      return "Limit Idle Times Constraint";

    case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:
      return "Cluster Busy Times Constraint";

    case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:
      return "Limit Busy Times Constraint";

    case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:
      return "Limit Workload Constraint";

    case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:
      return "Limit Active Intervals Constraint";

    case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:
      return "Limit Resources Constraint";

    default: return "?";
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT_TAG KheStringToConstraintTag(char *str)                   */
/*                                                                           */
/*  Return the constraint tag corresponding with str.                        */
/*                                                                           */
/*****************************************************************************/

KHE_CONSTRAINT_TAG KheStringToConstraintTag(char *str)
{
  switch( str[0] )
  {
    case 'A':

      if( strcmp(str, "AssignResourceConstraint") == 0 )
	return KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG;
      else if( strcmp(str, "AssignTimeConstraint") == 0 )
	return KHE_ASSIGN_TIME_CONSTRAINT_TAG;
      else if( strcmp(str, "AvoidClashesConstraint") == 0 )
	return KHE_AVOID_CLASHES_CONSTRAINT_TAG;
      else if( strcmp(str, "AvoidSplitAssignmentsConstraint") == 0 )
	return KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG;
      else if( strcmp(str, "AvoidUnavailableTimesConstraint") == 0 )
	return KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG;
      break;

    case 'C':

      if( strcmp(str, "ClusterBusyTimesConstraint") == 0 )
	return KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG;
      break;

    case 'D':

      if( strcmp(str, "DistributeSplitEventsConstraint") == 0 )
	return KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG;
      break;

    case 'L':

      if( strcmp(str, "LimitBusyTimesConstraint") == 0 )
	return KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG;
      else if( strcmp(str, "LimitIdleTimesConstraint") == 0 )
	return KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG;
      else if( strcmp(str, "LimitWorkloadConstraint") == 0 )
	return KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG;
      else if( strcmp(str, "LinkEventsConstraint") == 0 )
	return KHE_LINK_EVENTS_CONSTRAINT_TAG;
      else if( strcmp(str, "LimitActiveIntervalsConstraint") == 0 )
	return KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG;
      else if( strcmp(str, "LimitResourcesConstraint") == 0 )
	return KHE_LIMIT_RESOURCES_CONSTRAINT_TAG;
      break;

    case 'O':

      if( strcmp(str, "OrderEventsConstraint") == 0 )
	return KHE_ORDER_EVENTS_CONSTRAINT_TAG;
      break;

    case 'P':

      if( strcmp(str, "PreferResourcesConstraint") == 0 )
	return KHE_PREFER_RESOURCES_CONSTRAINT_TAG;
      else if( strcmp(str, "PreferTimesConstraint") == 0 )
	return KHE_PREFER_TIMES_CONSTRAINT_TAG;
      break;

    case 'S':
      if( strcmp(str, "SplitEventsConstraint") == 0 )
	return KHE_SPLIT_EVENTS_CONSTRAINT_TAG;
      else if( strcmp(str, "SpreadEventsConstraint") == 0 )
	return KHE_SPREAD_EVENTS_CONSTRAINT_TAG;
      break;
  }
  HnAbort("KheStringToConstraintTag: unknown str");
  return 0;  /* keep compiler happy */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintDebugCommonFields(KHE_CONSTRAINT c, int indent,        */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of the fields common to all constraints onto fp with the     */
/*  given indent.                                                            */
/*                                                                           */
/*****************************************************************************/

void KheConstraintDebugCommonFields(KHE_CONSTRAINT c, int indent, FILE *fp)
{
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "%s(%s, %s:%d:%s)", KheConstraintTagShow(c->tag),
    c->id != NULL ? c->id : "-", c->required ? "true" : "false", c->weight,
    KheCostFunctionShow(c->cost_function));
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintDebug(KHE_CONSTRAINT c, int verbosity,                 */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of c onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheConstraintDebug(KHE_CONSTRAINT c, int verbosity,
  int indent, FILE *fp)
{
  switch( c->tag )
  {
    case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:

      KheAssignResourceConstraintDebug((KHE_ASSIGN_RESOURCE_CONSTRAINT) c,
	verbosity, indent, fp);
      break;

    case KHE_ASSIGN_TIME_CONSTRAINT_TAG:

      KheAssignTimeConstraintDebug((KHE_ASSIGN_TIME_CONSTRAINT) c,
	verbosity, indent, fp);
      break;

    case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:

      KheSplitEventsConstraintDebug((KHE_SPLIT_EVENTS_CONSTRAINT) c,
	verbosity, indent, fp);
      break;

    case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:

      KheDistributeSplitEventsConstraintDebug(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT) c, verbosity, indent, fp);
      break;

    case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:

      KhePreferResourcesConstraintDebug((KHE_PREFER_RESOURCES_CONSTRAINT) c,
	verbosity, indent, fp);
      break;

    case KHE_PREFER_TIMES_CONSTRAINT_TAG:

      KhePreferTimesConstraintDebug((KHE_PREFER_TIMES_CONSTRAINT) c,
	verbosity, indent, fp);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:

      KheAvoidSplitAssignmentsConstraintDebug(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT) c, verbosity, indent, fp);
      break;

    case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:

      KheSpreadEventsConstraintDebug((KHE_SPREAD_EVENTS_CONSTRAINT) c,
	verbosity, indent, fp);
      break;

    case KHE_LINK_EVENTS_CONSTRAINT_TAG:

      KheLinkEventsConstraintDebug((KHE_LINK_EVENTS_CONSTRAINT) c,
	verbosity, indent, fp);
      break;

    case KHE_ORDER_EVENTS_CONSTRAINT_TAG:

      KheOrderEventsConstraintDebug((KHE_ORDER_EVENTS_CONSTRAINT) c,
	verbosity, indent, fp);
      break;

    case KHE_AVOID_CLASHES_CONSTRAINT_TAG:

      KheAvoidClashesConstraintDebug((KHE_AVOID_CLASHES_CONSTRAINT) c,
	verbosity, indent, fp);
      break;

    case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:

      KheAvoidUnavailableTimesConstraintDebug(
	(KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT) c, verbosity, indent, fp);
      break;

    case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:

      KheLimitIdleTimesConstraintDebug((KHE_LIMIT_IDLE_TIMES_CONSTRAINT) c,
	verbosity, indent, fp);
      break;

    case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:

      KheClusterBusyTimesConstraintDebug((KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c,
	verbosity, indent, fp);
      break;

    case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:

      KheLimitBusyTimesConstraintDebug((KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c,
	verbosity, indent, fp);
      break;

    case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:

      KheLimitWorkloadConstraintDebug((KHE_LIMIT_WORKLOAD_CONSTRAINT) c,
	verbosity, indent, fp);
      break;

    case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:

      KheLimitActiveIntervalsConstraintDebug(
	(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c, verbosity, indent, fp);
      break;

    case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:

      KheLimitResourcesConstraintDebug((KHE_LIMIT_RESOURCES_CONSTRAINT) c,
	verbosity, indent, fp);
      break;

    default:

      HnAbort("KheConstraintDebug internal error");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "upcasts"                                                      */
/*                                                                           */
/*****************************************************************************/

KHE_CONSTRAINT KheFromAssignResourceConstraint(KHE_ASSIGN_RESOURCE_CONSTRAINT c)
{
  return (KHE_CONSTRAINT) c;
}

KHE_CONSTRAINT KheFromAssignTimeConstraint(KHE_ASSIGN_TIME_CONSTRAINT c)
{
  return (KHE_CONSTRAINT) c;
}

KHE_CONSTRAINT KheFromSplitEventsConstraint(KHE_SPLIT_EVENTS_CONSTRAINT c)
{
  return (KHE_CONSTRAINT) c;
}

KHE_CONSTRAINT KheFromDistributeSplitEventsConstraint(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)
{
  return (KHE_CONSTRAINT) c;
}

KHE_CONSTRAINT KheFromPreferResourcesConstraint(
  KHE_PREFER_RESOURCES_CONSTRAINT c)
{
  return (KHE_CONSTRAINT) c;
}

KHE_CONSTRAINT KheFromPreferTimesConstraint(KHE_PREFER_TIMES_CONSTRAINT c)
{
  return (KHE_CONSTRAINT) c;
}

KHE_CONSTRAINT KheFromAvoidSplitAssignmentsConstraint(
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c)
{
  return (KHE_CONSTRAINT) c;
}

KHE_CONSTRAINT KheFromSpreadEventsConstraint(KHE_SPREAD_EVENTS_CONSTRAINT c)
{
  return (KHE_CONSTRAINT) c;
}

KHE_CONSTRAINT KheFromLinkEventsConstraint(KHE_LINK_EVENTS_CONSTRAINT c)
{
  return (KHE_CONSTRAINT) c;
}

KHE_CONSTRAINT KheFromOrderEventsConstraint(KHE_ORDER_EVENTS_CONSTRAINT c)
{
  return (KHE_CONSTRAINT) c;
}

KHE_CONSTRAINT KheFromAvoidClashesConstraint(KHE_AVOID_CLASHES_CONSTRAINT c)
{
  return (KHE_CONSTRAINT) c;
}

KHE_CONSTRAINT KheFromAvoidUnavailableTimesConstraint(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)
{
  return (KHE_CONSTRAINT) c;
}

KHE_CONSTRAINT KheFromLimitIdleTimesConstraint(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)
{
  return (KHE_CONSTRAINT) c;
}

KHE_CONSTRAINT KheFromClusterBusyTimesConstraint(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  return (KHE_CONSTRAINT) c;
}

KHE_CONSTRAINT KheFromLimitBusyTimesConstraint(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
{
  return (KHE_CONSTRAINT) c;
}

KHE_CONSTRAINT KheFromLimitWorkloadConstraint(KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  return (KHE_CONSTRAINT) c;
}

KHE_CONSTRAINT KheFromLimitActiveIntervalsConstraint(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  return (KHE_CONSTRAINT) c;
}

KHE_CONSTRAINT KheFromLimitResourcesConstraint(KHE_LIMIT_RESOURCES_CONSTRAINT c)
{
  return (KHE_CONSTRAINT) c;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "downcasts"                                                    */
/*                                                                           */
/*****************************************************************************/

KHE_ASSIGN_RESOURCE_CONSTRAINT KheToAssignResourceConstraint(KHE_CONSTRAINT c)
{
  HnAssert(c->tag == KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG,
    "KheToAssignResourceConstraint: downcast failed");
  return (KHE_ASSIGN_RESOURCE_CONSTRAINT) c;
}

KHE_ASSIGN_TIME_CONSTRAINT KheToAssignTimeConstraint(KHE_CONSTRAINT c)
{
  HnAssert(c->tag == KHE_ASSIGN_TIME_CONSTRAINT_TAG,
    "KheToAssignTimeConstraint: downcast failed");
  return (KHE_ASSIGN_TIME_CONSTRAINT) c;
}

KHE_SPLIT_EVENTS_CONSTRAINT KheToSplitEventsConstraint(KHE_CONSTRAINT c)
{
  HnAssert(c->tag == KHE_SPLIT_EVENTS_CONSTRAINT_TAG,
    "KheToSplitEventsConstraint: downcast failed");
  return (KHE_SPLIT_EVENTS_CONSTRAINT) c;
}

KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT
  KheToDistributeSplitEventsConstraint(KHE_CONSTRAINT c)
{
  HnAssert(c->tag == KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG,
    "KheToDistributeSplitEventsConstraint: downcast failed");
  return (KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT) c;
}

KHE_PREFER_RESOURCES_CONSTRAINT KheToPreferResourcesConstraint(KHE_CONSTRAINT c)
{
  HnAssert(c->tag == KHE_PREFER_RESOURCES_CONSTRAINT_TAG,
    "KheToPreferResourcesConstraint: downcast failed");
  return (KHE_PREFER_RESOURCES_CONSTRAINT) c;
}

KHE_PREFER_TIMES_CONSTRAINT KheToPreferTimesConstraint(KHE_CONSTRAINT c)
{
  HnAssert(c->tag == KHE_PREFER_TIMES_CONSTRAINT_TAG,
    "KheToPreferTimesConstraint: downcast failed");
  return (KHE_PREFER_TIMES_CONSTRAINT) c;
}

KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT
  KheToAvoidSplitAssignmentsConstraint(KHE_CONSTRAINT c)
{
  HnAssert(c->tag == KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG,
    "KheToAvoidSplitAssignmentsConstraint: downcast failed");
  return (KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT) c;
}

KHE_SPREAD_EVENTS_CONSTRAINT KheToSpreadEventsConstraint(KHE_CONSTRAINT c)
{
  HnAssert(c->tag == KHE_SPREAD_EVENTS_CONSTRAINT_TAG,
    "KheToSpreadEventsConstraint: downcast failed");
  return (KHE_SPREAD_EVENTS_CONSTRAINT) c;
}

KHE_LINK_EVENTS_CONSTRAINT KheToLinkEventsConstraint(KHE_CONSTRAINT c)
{
  HnAssert(c->tag == KHE_LINK_EVENTS_CONSTRAINT_TAG,
    "KheToLinkEventsConstraint: downcast failed");
  return (KHE_LINK_EVENTS_CONSTRAINT) c;
}

KHE_ORDER_EVENTS_CONSTRAINT KheToOrderEventsConstraint(KHE_CONSTRAINT c)
{
  HnAssert(c->tag == KHE_ORDER_EVENTS_CONSTRAINT_TAG,
    "KheToOrderEventsConstraint: downcast failed");
  return (KHE_ORDER_EVENTS_CONSTRAINT) c;
}

KHE_AVOID_CLASHES_CONSTRAINT KheToAvoidClashesConstraint(KHE_CONSTRAINT c)
{
  HnAssert(c->tag == KHE_AVOID_CLASHES_CONSTRAINT_TAG,
    "KheToAvoidClashesConstraint: downcast failed");
  return (KHE_AVOID_CLASHES_CONSTRAINT) c;
}

KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT
  KheToAvoidUnavailableTimesConstraint(KHE_CONSTRAINT c)
{
  HnAssert(c->tag == KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG,
    "KheToAvoidUnavailableTimesConstraint: downcast failed");
  return (KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT) c;
}

KHE_LIMIT_IDLE_TIMES_CONSTRAINT KheToLimitIdleTimesConstraint(KHE_CONSTRAINT c)
{
  HnAssert(c->tag == KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG,
    "KheToLimitIdleTimesConstraint: downcast failed");
  return (KHE_LIMIT_IDLE_TIMES_CONSTRAINT) c;
}

KHE_CLUSTER_BUSY_TIMES_CONSTRAINT
  KheToClusterBusyTimesConstraint(KHE_CONSTRAINT c)
{
  HnAssert(c->tag == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG,
    "KheToClusterBusyTimesConstraint: downcast failed");
  return (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c;
}

KHE_LIMIT_BUSY_TIMES_CONSTRAINT KheToLimitBusyTimesConstraint(KHE_CONSTRAINT c)
{
  HnAssert(c->tag == KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG,
    "KheToLimitBusyTimesConstraint: downcast failed");
  return (KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c;
}

KHE_LIMIT_WORKLOAD_CONSTRAINT KheToLimitWorkloadConstraint(KHE_CONSTRAINT c)
{
  HnAssert(c->tag == KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG,
    "KheToLimitWorkloadConstraint: downcast failed");
  return (KHE_LIMIT_WORKLOAD_CONSTRAINT) c;
}

KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT KheToLimitActiveIntervalsConstraint(
  KHE_CONSTRAINT c)
{
  HnAssert(c->tag == KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG,
    "KheToLimitActiveIntervalsConstraint: downcast failed");
  return (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c;
}

KHE_LIMIT_RESOURCES_CONSTRAINT KheToLimitResourcesConstraint(KHE_CONSTRAINT c)
{
  HnAssert(c->tag == KHE_LIMIT_RESOURCES_CONSTRAINT_TAG,
    "KheToLimitResourcesConstraint: downcast failed");
  return (KHE_LIMIT_RESOURCES_CONSTRAINT) c;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource type partitioning"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintAffectsResourceTypePartitioning(KHE_CONSTRAINT c,      */
/*    KHE_RESOURCE_GROUP *domain)                                            */
/*                                                                           */
/*  If c affects resource type partitioning (if it is a hard prefer          */
/*  resources constraint with non-zero cost), set *domain to its domain      */
/*  and return true; otherwise return false;                                 */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintAffectsResourceTypePartitioning(KHE_CONSTRAINT c,
  KHE_RESOURCE_GROUP *domain)
{
  KHE_PREFER_RESOURCES_CONSTRAINT prc;
  if( KheConstraintTag(c) == KHE_PREFER_RESOURCES_CONSTRAINT_TAG &&
	KheConstraintRequired(c) && KheConstraintWeight(c) > 0 )
  {
    prc = (KHE_PREFER_RESOURCES_CONSTRAINT) c;
    *domain = KhePreferResourcesConstraintDomain(prc);
    return true;
  }
  else
  {
    *domain = NULL;
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "finalizing"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintResetResourceOfType(KHE_CONSTRAINT c)                  */
/*                                                                           */
/*  Reset the resource_of_type values of c.  This is done only for those     */
/*  constraints that have these values, and only when resource type          */
/*  partitioning actually happens.                                           */
/*                                                                           */
/*****************************************************************************/

void KheConstraintResetResourceOfType(KHE_CONSTRAINT c)
{
  switch( c->tag )
  {
    case KHE_AVOID_CLASHES_CONSTRAINT_TAG:
      KheAvoidClashesConstraintResetResourceOfType(
	(KHE_AVOID_CLASHES_CONSTRAINT) c);
      break;

    case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:
      KheClusterBusyTimesConstraintResetResourceOfType(
	(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c);
      break;

    case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:
      KheLimitBusyTimesConstraintResetResourceOfType(
	(KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c);
      break;

    case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:
      KheLimitActiveIntervalsConstraintResetResourceOfType(
	(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c);
      break;

    default:
      /* nothing to do in other cases */
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintFinalize(KHE_CONSTRAINT c, KML_ERROR *ke)              */
/*                                                                           */
/*  Finalize c (at end of instance read).                                    */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintFinalize(KHE_CONSTRAINT c, KML_ERROR *ke)
{
  if( DEBUG1 )
    fprintf(stderr, "    KheConstraintFinalize(%s %s)\n",
      KheConstraintTagShow(c->tag), KheConstraintId(c));
  switch( c->tag )
  {
    case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:
      KheAssignResourceConstraintFinalize(
	(KHE_ASSIGN_RESOURCE_CONSTRAINT) c);
      break;

    case KHE_ASSIGN_TIME_CONSTRAINT_TAG:
      KheAssignTimeConstraintFinalize(
	(KHE_ASSIGN_TIME_CONSTRAINT) c);
      break;

    case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:
      KheSplitEventsConstraintFinalize(
	(KHE_SPLIT_EVENTS_CONSTRAINT) c);
      break;

    case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:
      KheDistributeSplitEventsConstraintFinalize(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT) c);
      break;

    case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:
      KhePreferResourcesConstraintFinalize(
	(KHE_PREFER_RESOURCES_CONSTRAINT) c);
      break;

    case KHE_PREFER_TIMES_CONSTRAINT_TAG:
      KhePreferTimesConstraintFinalize(
	(KHE_PREFER_TIMES_CONSTRAINT) c);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:
      KheAvoidSplitAssignmentsConstraintFinalize(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT) c);
      break;

    case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:
      KheSpreadEventsConstraintFinalize(
	(KHE_SPREAD_EVENTS_CONSTRAINT) c);
      break;

    case KHE_LINK_EVENTS_CONSTRAINT_TAG:
      KheLinkEventsConstraintFinalize(
	(KHE_LINK_EVENTS_CONSTRAINT) c);
      break;

    case KHE_ORDER_EVENTS_CONSTRAINT_TAG:
      KheOrderEventsConstraintFinalize(
	(KHE_ORDER_EVENTS_CONSTRAINT) c);
      break;

    case KHE_AVOID_CLASHES_CONSTRAINT_TAG:
      KheAvoidClashesConstraintFinalize(
	(KHE_AVOID_CLASHES_CONSTRAINT) c);
      break;

    case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:
      KheAvoidUnavailableTimesConstraintFinalize(
	(KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT) c);
      break;

    case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:
      return KheLimitIdleTimesConstraintFinalize(
	(KHE_LIMIT_IDLE_TIMES_CONSTRAINT) c, ke);
      break;

    case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:
      KheClusterBusyTimesConstraintFinalize(
	(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c);
      break;

    case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:
      KheLimitBusyTimesConstraintFinalize(
	(KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c);
      break;

    case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:
      KheLimitWorkloadConstraintFinalize(
	(KHE_LIMIT_WORKLOAD_CONSTRAINT) c);
      break;

    case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:
      KheLimitActiveIntervalsConstraintFinalize(
	(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c);
      break;

    case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:
      KheLimitResourcesConstraintFinalize((KHE_LIMIT_RESOURCES_CONSTRAINT) c);
      break;

    default:
      HnAbort("KheConstraintFinalize internal error");
  }
  *ke = NULL;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintDoEquiv(KHE_CONSTRAINT c1, KHE_CONSTRAINT c2)          */
/*                                                                           */
/*  Return true if c1 and c2 are equivalent.                                 */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintDoEquiv(KHE_CONSTRAINT c1, KHE_CONSTRAINT c2)
{
  /* same constraint means equivalent */
  if( c1 == c2 )
    return true;

  /* several attributes must be the same */
  if( c1->tag != c2->tag )
    return false;
  if( c1->cost_function != c2->cost_function )
    return false;
  if( c1->required != c2->required )
    return false;
  if( c1->weight != c2->weight )
    return false;

  /* individual checks */
  switch( c1->tag )
  {
    case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:

      return KheAssignResourceConstraintDoEquiv(
	(KHE_ASSIGN_RESOURCE_CONSTRAINT) c1,
	(KHE_ASSIGN_RESOURCE_CONSTRAINT) c2);

    case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:

      return KhePreferResourcesConstraintDoEquiv(
	(KHE_PREFER_RESOURCES_CONSTRAINT) c1,
	(KHE_PREFER_RESOURCES_CONSTRAINT) c2);

    case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:

      return KheAvoidSplitAssignmentsConstraintDoEquiv(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT) c1,
	(KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT) c2);

    case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:

      return KheLimitResourcesConstraintDoEquiv(
	(KHE_LIMIT_RESOURCES_CONSTRAINT) c1,
	(KHE_LIMIT_RESOURCES_CONSTRAINT) c2);

    default:

      /* other types not implemented (not currently needed), so we say no */
      return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintDensityCount(KHE_CONSTRAINT c)                          */
/*                                                                           */
/*  Return the density count of c.                                           */
/*                                                                           */
/*****************************************************************************/

int KheConstraintDensityCount(KHE_CONSTRAINT c)
{
  switch( c->tag )
  {
    case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:
      return KheAssignResourceConstraintDensityCount(
	(KHE_ASSIGN_RESOURCE_CONSTRAINT) c);

    case KHE_ASSIGN_TIME_CONSTRAINT_TAG:
      return KheAssignTimeConstraintDensityCount(
	(KHE_ASSIGN_TIME_CONSTRAINT) c);

    case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:
      return KheSplitEventsConstraintDensityCount(
	(KHE_SPLIT_EVENTS_CONSTRAINT) c);

    case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:
      return KheDistributeSplitEventsConstraintDensityCount(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT) c);

    case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:
      return KhePreferResourcesConstraintDensityCount(
	(KHE_PREFER_RESOURCES_CONSTRAINT) c);

    case KHE_PREFER_TIMES_CONSTRAINT_TAG:
      return KhePreferTimesConstraintDensityCount(
	(KHE_PREFER_TIMES_CONSTRAINT) c);

    case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:
      return KheAvoidSplitAssignmentsConstraintDensityCount(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT) c);

    case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:
      return KheSpreadEventsConstraintDensityCount(
	(KHE_SPREAD_EVENTS_CONSTRAINT) c);

    case KHE_LINK_EVENTS_CONSTRAINT_TAG:
      return KheLinkEventsConstraintDensityCount(
	(KHE_LINK_EVENTS_CONSTRAINT) c);

    case KHE_ORDER_EVENTS_CONSTRAINT_TAG:
      return KheOrderEventsConstraintDensityCount(
	(KHE_ORDER_EVENTS_CONSTRAINT) c);

    case KHE_AVOID_CLASHES_CONSTRAINT_TAG:
      return KheAvoidClashesConstraintDensityCount(
	(KHE_AVOID_CLASHES_CONSTRAINT) c);

    case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:
      return KheAvoidUnavailableTimesConstraintDensityCount(
	(KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT) c);

    case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:
      return KheLimitIdleTimesConstraintDensityCount(
	(KHE_LIMIT_IDLE_TIMES_CONSTRAINT) c);

    case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:
      return KheClusterBusyTimesConstraintDensityCount(
	(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c);

    case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:
      return KheLimitBusyTimesConstraintDensityCount(
	(KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c);

    case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:
      return KheLimitWorkloadConstraintDensityCount(
	(KHE_LIMIT_WORKLOAD_CONSTRAINT) c);

    case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:
      return KheLimitActiveIntervalsConstraintDensityCount(
	(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c);

    case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:
      return KheLimitResourcesConstraintDensityCount(
	(KHE_LIMIT_RESOURCES_CONSTRAINT) c);

    default:
      HnAbort("KheConstraintDensityCount internal error");
      return 0;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitors"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintMakeAndAttachMonitors(KHE_CONSTRAINT c, KHE_SOLN soln) */
/*                                                                           */
/*  Make and attach the constraint monitors of soln.                         */
/*                                                                           */
/*****************************************************************************/

void KheConstraintMakeAndAttachMonitors(KHE_CONSTRAINT c, KHE_SOLN soln)
{
  if( DEBUG12 )
    fprintf(stderr, "[ KheConstraintMakeAndAttachMonitors(%s %s)\n",
      KheConstraintTagShow(KheConstraintTag(c)), KheConstraintId(c));
  switch( KheConstraintTag(c) )
  {
    case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:

      KheAssignResourceConstraintMakeAndAttachMonitors(
	(KHE_ASSIGN_RESOURCE_CONSTRAINT) c, soln);
      break;

    case KHE_ASSIGN_TIME_CONSTRAINT_TAG:

      KheAssignTimeConstraintMakeAndAttachMonitors(
	(KHE_ASSIGN_TIME_CONSTRAINT) c, soln);
      break;

    case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:

      KheSplitEventsConstraintMakeAndAttachMonitors(
	(KHE_SPLIT_EVENTS_CONSTRAINT) c, soln);
      break;

    case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:

      KheDistributeSplitEventsConstraintMakeAndAttachMonitors(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT) c, soln);
      break;

    case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:

      KhePreferResourcesConstraintMakeAndAttachMonitors(
	(KHE_PREFER_RESOURCES_CONSTRAINT) c, soln);
      break;

    case KHE_PREFER_TIMES_CONSTRAINT_TAG:

      KhePreferTimesConstraintMakeAndAttachMonitors(
	(KHE_PREFER_TIMES_CONSTRAINT) c, soln);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:

      KheAvoidSplitAssignmentsConstraintMakeAndAttachMonitors(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT) c, soln);
      break;

    case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:

      KheSpreadEventsConstraintMakeAndAttachMonitors(
	(KHE_SPREAD_EVENTS_CONSTRAINT) c, soln);
      break;

    case KHE_LINK_EVENTS_CONSTRAINT_TAG:

      KheLinkEventsConstraintMakeAndAttachMonitors(
	(KHE_LINK_EVENTS_CONSTRAINT) c, soln);
      break;

    case KHE_ORDER_EVENTS_CONSTRAINT_TAG:

      KheOrderEventsConstraintMakeAndAttachMonitors(
	(KHE_ORDER_EVENTS_CONSTRAINT) c, soln);
      break;

    case KHE_AVOID_CLASHES_CONSTRAINT_TAG:

      KheAvoidClashesConstraintMakeAndAttachMonitors(
	(KHE_AVOID_CLASHES_CONSTRAINT) c, soln);
      break;

    case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:

      KheAvoidUnavailableTimesConstraintMakeAndAttachMonitors(
	(KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT) c, soln);
      break;

    case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:

      KheLimitIdleTimesConstraintMakeAndAttachMonitors(
	(KHE_LIMIT_IDLE_TIMES_CONSTRAINT) c, soln);
      break;

    case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:

      KheClusterBusyTimesConstraintMakeAndAttachMonitors(
	(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c, soln);
      break;

    case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:

      KheLimitBusyTimesConstraintMakeAndAttachMonitors(
	(KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c, soln);
      break;

    case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:

      KheLimitWorkloadConstraintMakeAndAttachMonitors(
	(KHE_LIMIT_WORKLOAD_CONSTRAINT) c, soln);
      break;

    case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:

      KheLimitActiveIntervalsConstraintMakeAndAttachMonitors(
	(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c, soln);
      break;

    case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:

      KheLimitResourcesConstraintMakeAndAttachMonitors(
	(KHE_LIMIT_RESOURCES_CONSTRAINT) c, soln);
      break;

    default:

      HnAbort("KheConstraintMakeAndAttachMonitors illegal constraint tag");
      break;
  }
  if( DEBUG12 )
    fprintf(stderr, "] KheConstraintMakeAndAttachMonitors returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintMakeFromKml(KML_ELT cons_elt, KHE_INSTANCE ins,        */
/*    KML_ERROR *ke)                                                         */
/*                                                                           */
/*  Make a constraint from cons_elt and add it to ins.                       */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintMakeFromKml(KML_ELT cons_elt, KHE_INSTANCE ins,
  KML_ERROR *ke)
{
  bool res;  KHE_CONSTRAINT_TAG tag;
  tag = KheStringToConstraintTag(KmlLabel(cons_elt));
  switch( tag )
  {
    case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:
      res = KheAssignResourceConstraintMakeFromKml(cons_elt, ins, ke);
      break;

    case KHE_ASSIGN_TIME_CONSTRAINT_TAG:
      res = KheAssignTimeConstraintMakeFromKml(cons_elt, ins, ke);
      break;

    case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:
      res = KheSplitEventsConstraintMakeFromKml(cons_elt, ins, ke);
      break;

    case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:
      res = KheDistributeSplitEventsConstraintMakeFromKml(cons_elt, ins, ke);
      break;

    case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:
      res = KhePreferResourcesConstraintMakeFromKml(cons_elt, ins, ke);
      break;

    case KHE_PREFER_TIMES_CONSTRAINT_TAG:
      res = KhePreferTimesConstraintMakeFromKml(cons_elt, ins, ke);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:
      res = KheAvoidSplitAssignmentsConstraintMakeFromKml(cons_elt, ins, ke);
      break;

    case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:
      res = KheSpreadEventsConstraintMakeFromKml(cons_elt, ins, ke);
      break;

    case KHE_LINK_EVENTS_CONSTRAINT_TAG:
      res = KheLinkEventsConstraintMakeFromKml(cons_elt, ins, ke);
      break;

    case KHE_ORDER_EVENTS_CONSTRAINT_TAG:
      res = KheOrderEventsConstraintMakeFromKml(cons_elt, ins, ke);
      break;

    case KHE_AVOID_CLASHES_CONSTRAINT_TAG:
      res = KheAvoidClashesConstraintMakeFromKml(cons_elt, ins, ke);
      break;

    case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:
      res = KheAvoidUnavailableTimesConstraintMakeFromKml(cons_elt, ins, ke);
      break;

    case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:
      res = KheLimitIdleTimesConstraintMakeFromKml(cons_elt, ins, ke);
      break;

    case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:
      res = KheClusterBusyTimesConstraintMakeFromKml(cons_elt, ins, ke);
      break;

    case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:
      res = KheLimitBusyTimesConstraintMakeFromKml(cons_elt, ins, ke);
      break;

    case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:
      res = KheLimitWorkloadConstraintMakeFromKml(cons_elt, ins, ke);
      break;

    case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:
      res = KheLimitActiveIntervalsConstraintMakeFromKml(cons_elt, ins, ke);
      break;

    case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:
      res = KheLimitResourcesConstraintMakeFromKml(cons_elt, ins, ke);
      break;

    default:
      HnAbort("KheConstraintMakeFromKml: unexpected tag (internal error)");
      res = false;
  }
  if( !res )
    HnAssert(*ke != NULL, "KheConstraintMakeFromKml internal error 2 (%s)",
      KheConstraintTagShow(tag));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintCheckKml(KML_ELT cons_elt, char **id, char **name,     */
/*    bool *required, int *weight, KHE_COST_FUNCTION *cf, KML_ERROR *ke)     */
/*                                                                           */
/*  Read the common elements from cons_elt.  It has already been checked     */
/*  to conform to "Id : $Name $Required #Weight $CostFunction" at least.     */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintCheckKml(KML_ELT cons_elt, char **id, char **name,
  bool *required, int *weight, KHE_COST_FUNCTION *cf, KML_ERROR *ke,
  HA_ARENA a)
{
  KML_ELT e;  char *text;

  /* Id and Name */
  *id = KmlAttributeValue(cons_elt, 0);
  *name = KmlText(KmlChild(cons_elt, 0));

  /* Required */
  e = KmlChild(cons_elt, 1);
  text = KmlText(e);
  if( strcmp(text, "true") == 0 )
    *required = true;
  else if( strcmp(text, "false") == 0 )
    *required = false;
  else
    return KmlError(ke, a, KmlLineNum(e), KmlColNum(e),
      "<Required> is neither true nor false");

  /* Weight */
  e = KmlChild(cons_elt, 2);
  sscanf(KmlText(e), "%d", weight);
  if( *weight < 0 )
    return KmlError(ke, a, KmlLineNum(e), KmlColNum(e), "<Weight> is negative");
  if( *weight > 1000 )
    return KmlError(ke, a, KmlLineNum(e), KmlColNum(e),
      "<Weight> is too large (maximum is 1000)");

  /* CostFunction */
  e = KmlChild(cons_elt, 3);
  text = KmlText(e);
  if( strcmp(text, "SumSteps") == 0 )
    return KmlError(ke, a, KmlLineNum(e), KmlColNum(e),
      "<CostFunction> has obsolete value \"%s\"", text);
  else if( strcmp(text, "StepSum") == 0 )
    return KmlError(ke, a, KmlLineNum(e), KmlColNum(e),
      "<CostFunction> has obsolete value \"%s\"", text);
  else if( strcmp(text, "Step") == 0 )
    *cf = KHE_STEP_COST_FUNCTION;
  else if( strcmp(text, "Sum") == 0 )
    return KmlError(ke, a, KmlLineNum(e), KmlColNum(e),
      "<CostFunction> has obsolete value \"%s\"", text);
  else if( strcmp(text, "Linear") == 0 )
    *cf = KHE_LINEAR_COST_FUNCTION;
  else if( strcmp(text, "SumSquares") == 0 )
    return KmlError(ke, a, KmlLineNum(e), KmlColNum(e),
      "<CostFunction> has obsolete value \"%s\"", text);
  else if( strcmp(text, "SquareSum") == 0 )
    return KmlError(ke, a, KmlLineNum(e), KmlColNum(e),
      "<CostFunction> has obsolete value \"%s\"", text);
  else if( strcmp(text, "Quadratic") == 0 )
    *cf = KHE_QUADRATIC_COST_FUNCTION;
  else
    return KmlError(ke, a, KmlLineNum(e), KmlColNum(e),
      "<CostFunction> has unknown value \"%s\"", text);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintAddTimeGroupsFromKml(KHE_CONSTRAINT c, KML_ELT elt,    */
/*    KML_ERROR *ke)                                                         */
/*                                                                           */
/*  If elt contains an TimeGroups child, add those time groups to c.         */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintAddTimeGroupsFromKml(KHE_CONSTRAINT c, KML_ELT elt,
  KML_ERROR *ke, HA_ARENA a)
{
  KML_ELT time_groups_elt, time_group_elt;  KHE_TIME_GROUP tg;
  int i;  char *ref;  KHE_POLARITY po;
  if( KmlContainsChild(elt, "TimeGroups", &time_groups_elt) )
  {
    if( !KmlCheck(time_groups_elt, ": *TimeGroup", ke) )
      return false;
    for( i = 0;  i < KmlChildCount(time_groups_elt);  i++ )
    {
      /* check syntax and retrieve time group */
      time_group_elt = KmlChild(time_groups_elt, i);
      if( c->tag == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG ||
          c->tag == KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG )
      {
	if( !KmlCheck(time_group_elt, "Reference +Polarity", ke) )
	  return false;
      }
      else
      {
	if( !KmlCheck(time_group_elt, "Reference", ke) )
	  return false;
      }
      ref = KmlAttributeValue(time_group_elt, 0);
      if( !KheInstanceRetrieveTimeGroup(c->instance, ref, &tg) )
	return KmlError(ke, a, KmlLineNum(time_group_elt),
	  KmlColNum(time_group_elt),
	  "<TimeGroup> Reference \"%s\" unknown", ref);

      /* add time group to c */
      switch( c->tag )
      {
	case KHE_PREFER_TIMES_CONSTRAINT_TAG:

	  KhePreferTimesConstraintAddTimeGroup(
	    (KHE_PREFER_TIMES_CONSTRAINT) c, tg);
	  break;

	case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:

	  /* *** checking compactness here is premature
	  if( !KheTimeGroupIsCom pact(tg) )
	    return KmlError(ke, a, KmlLineNum(time_group_elt),
	      KmlColNum(time_group_elt),
	      "<TimeGroup> \"%s\" not compact", ref);
	  *** */
	  KheLimitIdleTimesConstraintAddTimeGroup(
	    (KHE_LIMIT_IDLE_TIMES_CONSTRAINT) c, tg);
	  break;

	case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:

	  KheAvoidUnavailableTimesConstraintAddTimeGroup(
	    (KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT) c, tg);
	  break;

	case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:

	  if( KmlAttributeCount(time_group_elt) == 1 )
	    po = KHE_POSITIVE;
	  else if( strcmp(KmlAttributeValue(time_group_elt, 1), "positive")==0 )
	    po = KHE_POSITIVE;
	  else if( strcmp(KmlAttributeValue(time_group_elt, 1), "negative")==0 )
	    po = KHE_NEGATIVE;
	  else
	    return KmlError(ke, a, KmlLineNum(time_group_elt),
	      KmlColNum(time_group_elt),
	      "Polarity \"%s\" neither \"positive\" or \"negative\"",
	      KmlAttributeValue(time_group_elt, 1));
	  KheClusterBusyTimesConstraintAddTimeGroup(
	    (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c, tg, po);
	  break;

	case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:

	  if( KmlAttributeCount(time_group_elt) == 1 )
	    po = KHE_POSITIVE;
	  else if( strcmp(KmlAttributeValue(time_group_elt, 1), "positive")==0 )
	    po = KHE_POSITIVE;
	  else if( strcmp(KmlAttributeValue(time_group_elt, 1), "negative")==0 )
	    po = KHE_NEGATIVE;
	  else
	    return KmlError(ke, a, KmlLineNum(time_group_elt),
	      KmlColNum(time_group_elt),
	      "Polarity \"%s\" neither \"positive\" or \"negative\"",
	      KmlAttributeValue(time_group_elt, 1));
	  KheLimitActiveIntervalsConstraintAddTimeGroup(
	    (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c, tg, po);
	  break;

	case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:

	  KheLimitBusyTimesConstraintAddTimeGroup(
	    (KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c, tg);
	  break;

	case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:

	  KheLimitWorkloadConstraintAddTimeGroup(
	    (KHE_LIMIT_WORKLOAD_CONSTRAINT) c, tg);
	  break;

	case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:
	case KHE_ASSIGN_TIME_CONSTRAINT_TAG:
	case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:
	case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:
	case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:
	case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:
	case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:
	case KHE_LINK_EVENTS_CONSTRAINT_TAG:
	case KHE_ORDER_EVENTS_CONSTRAINT_TAG:
	case KHE_AVOID_CLASHES_CONSTRAINT_TAG:
	case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:
	default:

	  HnAbort("KheConstraintAddTimeGroup internal error");
      }
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintAddNegativeTimeGroupsFromKml(KHE_CONSTRAINT c,         */
/*    KML_ELT elt, KML_ERROR *ke)                                            */
/*                                                                           */
/*  If elt contains a NegativeTimeGroups child, add those time groups to c.  */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheConstraintAddNegativeTimeGroupsFromKml(KHE_CONSTRAINT c,
  KML_ELT elt, KML_ERROR *ke)
{
  KML_ELT time_groups_elt, time_group_elt;  KHE_TIME_GROUP tg;
  int i;  char *ref;
  if( KmlContainsChild(elt, "NegativeTimeGroups", &time_groups_elt) )
  {
    if( !KmlCheck(time_groups_elt, ": *NegativeTimeGroup", ke) )
      return false;
    for( i = 0;  i < KmlChildCount(time_groups_elt);  i++ )
    {
      time_group_elt = KmlChild(time_groups_elt, i);
      if( !KmlCheck(time_group_elt, "Reference", ke) )
	return false;
      ref = KmlAttributeValue(time_group_elt, 0);
      if( !KheInstanceRetrieveTimeGroup(c->instance, ref, &tg) )
	return KmlError(ke, a, KmlLineNum(time_group_elt),
	  KmlColNum(time_group_elt),
	  "<NegativeTimeGroup> Reference \"%s\" unknown", ref);
      switch( c->tag )
      {
	case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:
	  KheClusterBusyTimesConstraintAddNegativeTimeGroup(
	    (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c, tg);
	  break;

	case KHE_PREFER_TIMES_CONSTRAINT_TAG:
	case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:
	case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:
	case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:
	case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:
	case KHE_ASSIGN_TIME_CONSTRAINT_TAG:
	case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:
	case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:
	case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:
	case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:
	case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:
	case KHE_LINK_EVENTS_CONSTRAINT_TAG:
	case KHE_ORDER_EVENTS_CONSTRAINT_TAG:
	case KHE_AVOID_CLASHES_CONSTRAINT_TAG:
	case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:
	default:

	  HnAbort("KheConstraintAddNegativeTimeGroup internal error");
      }
    }
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintAddTimesFromKml(KHE_CONSTRAINT c, KML_ELT elt,         */
/*    KML_ERROR *ke)                                                         */
/*                                                                           */
/*  If elt contains an Times child, add those times to c.                    */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintAddTimesFromKml(KHE_CONSTRAINT c, KML_ELT elt,
  KML_ERROR *ke, HA_ARENA a)
{
  KML_ELT times_elt, time_elt;  KHE_TIME t;  int i;  char *ref;
  a = KheInstanceArena(KheConstraintInstance(c));
  if( KmlContainsChild(elt, "Times", &times_elt) )
  {
    if( !KmlCheck(times_elt, ": *Time", ke) )
      return false;
    for( i = 0;  i < KmlChildCount(times_elt);  i++ )
    {
      time_elt = KmlChild(times_elt, i);
      if( !KmlCheck(time_elt, "Reference", ke) )
	return false;
      ref = KmlAttributeValue(time_elt, 0);
      if( !KheInstanceRetrieveTime(c->instance, ref, &t) )
	return KmlError(ke, a, KmlLineNum(time_elt), KmlColNum(time_elt),
          "<Time> Reference \"%s\" unknown", ref);
      switch( c->tag )
      {
	case KHE_PREFER_TIMES_CONSTRAINT_TAG:
	  KhePreferTimesConstraintAddTime(
	    (KHE_PREFER_TIMES_CONSTRAINT) c, t);
	  break;

	case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:
	  KheAvoidUnavailableTimesConstraintAddTime(
	    (KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT) c, t);
	  break;

	case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:
	case KHE_ASSIGN_TIME_CONSTRAINT_TAG:
	case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:
	case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:
	case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:
	case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:
	case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:
	case KHE_LINK_EVENTS_CONSTRAINT_TAG:
	case KHE_ORDER_EVENTS_CONSTRAINT_TAG:
	case KHE_AVOID_CLASHES_CONSTRAINT_TAG:
	case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:
	case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:
	case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:
	case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:
	case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:
	case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:
	default:

	  HnAbort("KheConstraintAddTime internal error");
      }
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintAddResourceGroupsFromKml(KHE_CONSTRAINT c,             */
/*    KML_ELT elt, KML_ERROR *ke)                                            */
/*                                                                           */
/*  If elt contains an ResourceGroups child, add those resource groups to c. */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintAddResourceGroupsFromKml(KHE_CONSTRAINT c, KML_ELT elt,
  KML_ERROR *ke, HA_ARENA a)
{
  KML_ELT resource_groups_elt, resource_group_elt;  KHE_RESOURCE_GROUP rg;
  int i;  char *ref;
  if( KmlContainsChild(elt, "ResourceGroups", &resource_groups_elt) )
  {
    if( !KmlCheck(resource_groups_elt, ": *ResourceGroup", ke) )
      return false;
    for( i = 0;  i < KmlChildCount(resource_groups_elt);  i++ )
    {
      resource_group_elt = KmlChild(resource_groups_elt, i);
      if( !KmlCheck(resource_group_elt, "Reference", ke) )
	return false;
      ref = KmlAttributeValue(resource_group_elt, 0);
      if( !KheInstanceRetrieveResourceGroup(c->instance, ref, &rg) )
	return KmlError(ke, a, KmlLineNum(resource_group_elt),
	  KmlColNum(resource_group_elt),
	  "<ResourceGroup> Reference \"%s\" unknown", ref);
      switch( c->tag )
      {
	case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:
	  if( !KhePreferResourcesConstraintAddResourceGroup(
	      (KHE_PREFER_RESOURCES_CONSTRAINT) c, rg) )
	    return KmlError(ke, a, KmlLineNum(resource_group_elt),
	      KmlColNum(resource_group_elt),
	      "<ResourceGroup> \"%s\" has inconsistent <ResourceType>", ref);
	  break;

	case KHE_AVOID_CLASHES_CONSTRAINT_TAG:
	  KheAvoidClashesConstraintAddResourceGroup(
	    (KHE_AVOID_CLASHES_CONSTRAINT) c, rg);
	  break;

	case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:
	  KheAvoidUnavailableTimesConstraintAddResourceGroup(
	    (KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT) c, rg);
	  break;

	case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:
	  KheLimitIdleTimesConstraintAddResourceGroup(
	    (KHE_LIMIT_IDLE_TIMES_CONSTRAINT) c, rg);
	  break;

	case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:
	  KheClusterBusyTimesConstraintAddResourceGroup(
	    (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c, rg);
	  break;

	case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:
	  KheLimitBusyTimesConstraintAddResourceGroup(
	    (KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c, rg);
	  break;

	case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:
	  KheLimitWorkloadConstraintAddResourceGroup(
	    (KHE_LIMIT_WORKLOAD_CONSTRAINT) c, rg);
	  break;

	case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:
	  KheLimitActiveIntervalsConstraintAddResourceGroup(
	    (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c, rg);
	  break;

	case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:
	  if( !KheLimitResourcesConstraintAddResourceGroup(
	      (KHE_LIMIT_RESOURCES_CONSTRAINT) c, rg) )
	    return KmlError(ke, a, KmlLineNum(resource_group_elt),
	      KmlColNum(resource_group_elt),
	      "<ResourceGroup> \"%s\" has inconsistent <ResourceType>", ref);
	  break;

	case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:
	case KHE_ASSIGN_TIME_CONSTRAINT_TAG:
	case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:
	case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:
	case KHE_PREFER_TIMES_CONSTRAINT_TAG:
	case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:
	case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:
	case KHE_LINK_EVENTS_CONSTRAINT_TAG:
	case KHE_ORDER_EVENTS_CONSTRAINT_TAG:
	default:

	  HnAbort("KheConstraintAddResourceGroup internal error");
      }
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintAddResourcesFromKml(KHE_CONSTRAINT c, KML_ELT elt,     */
/*    KML_ERROR *ke)                                                         */
/*                                                                           */
/*  If elt contains an Resources child, add those resources to c.            */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintAddResourcesFromKml(KHE_CONSTRAINT c, KML_ELT elt,
  KML_ERROR *ke, HA_ARENA a)
{
  KML_ELT resources_elt, resource_elt;  KHE_RESOURCE r;  int i;  char *ref;
  if( KmlContainsChild(elt, "Resources", &resources_elt) )
  {
    if( !KmlCheck(resources_elt, ": *Resource", ke) )
      return false;
    for( i = 0;  i < KmlChildCount(resources_elt);  i++ )
    {
      resource_elt = KmlChild(resources_elt, i);
      if( !KmlCheck(resource_elt, "Reference", ke) )
	return false;
      ref = KmlAttributeValue(resource_elt, 0);
      if( !KheInstanceRetrieveResource(c->instance, ref, &r) )
	return KmlError(ke, a, KmlLineNum(resource_elt),
	  KmlColNum(resource_elt), "<Resource> Reference \"%s\" unknown", ref);
      switch( c->tag )
      {
	case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:
	  if( !KhePreferResourcesConstraintAddResource(
	      (KHE_PREFER_RESOURCES_CONSTRAINT) c, r) )
	    return KmlError(ke, a, KmlLineNum(resource_elt),
	      KmlColNum(resource_elt),
	      "<Resource> \"%s\" has inconsistent <ResourceType>", ref);
	  break;

	case KHE_AVOID_CLASHES_CONSTRAINT_TAG:
	  KheAvoidClashesConstraintAddResource(
	    (KHE_AVOID_CLASHES_CONSTRAINT) c, r);
	  break;

	case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:
	  KheAvoidUnavailableTimesConstraintAddResource(
	    (KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT) c, r);
	  break;

	case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:
	  KheLimitIdleTimesConstraintAddResource(
	    (KHE_LIMIT_IDLE_TIMES_CONSTRAINT) c, r);
	  break;

	case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:
	  KheClusterBusyTimesConstraintAddResource(
	    (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c, r);
	  break;

	case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:
	  KheLimitBusyTimesConstraintAddResource(
	    (KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c, r);
	  break;

	case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:
	  KheLimitWorkloadConstraintAddResource(
	    (KHE_LIMIT_WORKLOAD_CONSTRAINT) c, r);
	  break;

	case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:
	  KheLimitActiveIntervalsConstraintAddResource(
	    (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c, r);
	  break;

	case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:
	  if( !KheLimitResourcesConstraintAddResource(
	      (KHE_LIMIT_RESOURCES_CONSTRAINT) c, r) )
	    return KmlError(ke, a, KmlLineNum(resource_elt),
	      KmlColNum(resource_elt),
	      "<Resource> \"%s\" has inconsistent <ResourceType>", ref);
	  break;

	case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:
	case KHE_ASSIGN_TIME_CONSTRAINT_TAG:
	case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:
	case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:
	case KHE_PREFER_TIMES_CONSTRAINT_TAG:
	case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:
	case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:
	case KHE_LINK_EVENTS_CONSTRAINT_TAG:
	case KHE_ORDER_EVENTS_CONSTRAINT_TAG:
	default:

	  HnAbort("KheConstraintAddResource internal error");
      }
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintAddEventGroupsFromKml(KHE_CONSTRAINT c, KML_ELT elt,   */
/*    KML_ERROR *ke)                                                         */
/*                                                                           */
/*  If elt contains an EventGroups child, add those event groups to c.       */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintAddEventGroupsFromKml(KHE_CONSTRAINT c, KML_ELT elt,
  KML_ERROR *ke, HA_ARENA a)
{
  KML_ELT event_groups_elt, event_group_elt;  KHE_EVENT_GROUP eg;
  KHE_EVENT e;  int i;  char *ref;
  if( KmlContainsChild(elt, "EventGroups", &event_groups_elt) )
  {
    if( !KmlCheck(event_groups_elt, ": *EventGroup", ke) )
      return false;
    for( i = 0;  i < KmlChildCount(event_groups_elt);  i++ )
    {
      event_group_elt = KmlChild(event_groups_elt, i);
      if( !KmlCheck(event_group_elt, "Reference", ke) )
	return false;
      ref = KmlAttributeValue(event_group_elt, 0);
      if( !KheInstanceRetrieveEventGroup(c->instance, ref, &eg) )
	return KmlError(ke, a, KmlLineNum(event_group_elt),
	  KmlColNum(event_group_elt),
	  "<EventGroup> Reference \"%s\" unknown", ref);
      switch( c->tag )
      {
	case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:
	  KheAssignResourceConstraintAddEventGroup(
	    (KHE_ASSIGN_RESOURCE_CONSTRAINT) c, eg);
	  break;

	case KHE_ASSIGN_TIME_CONSTRAINT_TAG:
	  KheAssignTimeConstraintAddEventGroup(
	    (KHE_ASSIGN_TIME_CONSTRAINT) c, eg);
	  break;

	case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:
	  KheSplitEventsConstraintAddEventGroup(
	    (KHE_SPLIT_EVENTS_CONSTRAINT) c, eg);
	  break;

	case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:
	  KheDistributeSplitEventsConstraintAddEventGroup(
	    (KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT) c,eg);
	  break;

	case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:
	  if( !KhePreferResourcesConstraintAddEventGroup(
	    (KHE_PREFER_RESOURCES_CONSTRAINT) c, eg, &e) )
	    return KmlError(ke, a, KmlLineNum(event_group_elt),
	      KmlColNum(event_group_elt), "<EventGroup> \"%s\" contains event "
	      "\"%s\" whose Resource has inconsistent <ResourceType>", ref,
	      KheEventId(e));
	  break;

	case KHE_PREFER_TIMES_CONSTRAINT_TAG:
	  KhePreferTimesConstraintAddEventGroup(
	    (KHE_PREFER_TIMES_CONSTRAINT) c, eg);
	  break;

	case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:
	  if( !KheAvoidSplitAssignmentsConstraintAddEventGroup(
	    (KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT) c, eg, &e) )
	    return KmlError(ke, a, KmlLineNum(event_group_elt),
	      KmlColNum(event_group_elt), "<EventGroup> \"%s\" contains "
	      "event \"%s\" without the given role", ref, KheEventId(e));
	  break;

	case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:
	  KheSpreadEventsConstraintAddEventGroup(
	    (KHE_SPREAD_EVENTS_CONSTRAINT) c, eg);
	  break;

	case KHE_LINK_EVENTS_CONSTRAINT_TAG:
	  KheLinkEventsConstraintAddEventGroup(
	    (KHE_LINK_EVENTS_CONSTRAINT) c, eg);
	  break;

	case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:
	  KheLimitResourcesConstraintAddEventGroup(
	    (KHE_LIMIT_RESOURCES_CONSTRAINT) c, eg);
	  break;

	case KHE_ORDER_EVENTS_CONSTRAINT_TAG:
	case KHE_AVOID_CLASHES_CONSTRAINT_TAG:
	case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:
	case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:
	case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:
	case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:
	case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:
	case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:
	default:

	  HnAbort("KheConstraintAddEventGroup internal error");
	  return true;  /* keep compiler happy */
      }
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintAddEventsFromKml(KHE_CONSTRAINT c, KML_ELT elt,        */
/*    KML_ERROR *ke)                                                         */
/*                                                                           */
/*  If elt contains an Events child, add those events to c.                  */
/*                                                                           */
/*****************************************************************************/

bool KheConstraintAddEventsFromKml(KHE_CONSTRAINT c, KML_ELT elt,
  KML_ERROR *ke, HA_ARENA a)
{
  KML_ELT events_elt, event_elt;  KHE_EVENT e;  int i;  char *ref;
  if( KmlContainsChild(elt, "Events", &events_elt) )
  {
    if( !KmlCheck(events_elt, ": *Event", ke) )
      return false;
    for( i = 0;  i < KmlChildCount(events_elt);  i++ )
    {
      event_elt = KmlChild(events_elt, i);
      if( !KmlCheck(event_elt, "Reference", ke) )
	return false;
      ref = KmlAttributeValue(event_elt, 0);
      if( !KheInstanceRetrieveEvent(c->instance, ref, &e) )
	return KmlError(ke, a, KmlLineNum(event_elt), KmlColNum(event_elt),
	  "<Event> Reference \"%s\" unknown", ref);
      switch( c->tag )
      {
	case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:
	  KheAssignResourceConstraintAddEvent(
	    (KHE_ASSIGN_RESOURCE_CONSTRAINT) c, e);
	  break;

	case KHE_ASSIGN_TIME_CONSTRAINT_TAG:
	  KheAssignTimeConstraintAddEvent((KHE_ASSIGN_TIME_CONSTRAINT) c, e);
	  break;

	case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:
	  KheSplitEventsConstraintAddEvent((KHE_SPLIT_EVENTS_CONSTRAINT) c, e);
	  break;

	case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:
	  KheDistributeSplitEventsConstraintAddEvent(
	    (KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT) c, e);
	  break;

	case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:
	  if( !KhePreferResourcesConstraintAddEvent(
	      (KHE_PREFER_RESOURCES_CONSTRAINT) c, e) )
	    return KmlError(ke, a, KmlLineNum(event_elt), KmlColNum(event_elt),
	      "<Event> \"%s\" Resource has inconsistent <ResourceType>", ref);
	  break;

	case KHE_PREFER_TIMES_CONSTRAINT_TAG:
	  KhePreferTimesConstraintAddEvent((KHE_PREFER_TIMES_CONSTRAINT) c, e);
	  break;

	case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:
	  KheLimitResourcesConstraintAddEventGroup(
	    (KHE_LIMIT_RESOURCES_CONSTRAINT) c, KheEventSingletonEventGroup(e));
	  break;

	case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:
	case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:
	case KHE_LINK_EVENTS_CONSTRAINT_TAG:
	case KHE_ORDER_EVENTS_CONSTRAINT_TAG:
	case KHE_AVOID_CLASHES_CONSTRAINT_TAG:
	case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:
	case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:
	case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:
	case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:
	case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:
	case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:
	default:

	  HnAbort("KheConstraintAddEventsFromKml internal error");
      }
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintWrite(KHE_CONSTRAINT c, KML_FILE kf)                   */
/*                                                                           */
/*  Write c onto kf.                                                        */
/*                                                                           */
/*****************************************************************************/

void KheConstraintWrite(KHE_CONSTRAINT c, KML_FILE kf)
{
  switch( c->tag )
  {
    case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:
      KheAssignResourceConstraintWrite((KHE_ASSIGN_RESOURCE_CONSTRAINT) c, kf);
      break;

    case KHE_ASSIGN_TIME_CONSTRAINT_TAG:
      KheAssignTimeConstraintWrite((KHE_ASSIGN_TIME_CONSTRAINT) c, kf);
      break;

    case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:
      KheSplitEventsConstraintWrite((KHE_SPLIT_EVENTS_CONSTRAINT) c,kf);
      break;

    case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:
      KheDistributeSplitEventsConstraintWrite(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT) c, kf);
      break;

    case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:
      KhePreferResourcesConstraintWrite((KHE_PREFER_RESOURCES_CONSTRAINT) c,kf);
      break;

    case KHE_PREFER_TIMES_CONSTRAINT_TAG:
      KhePreferTimesConstraintWrite((KHE_PREFER_TIMES_CONSTRAINT) c,kf);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:
      KheAvoidSplitAssignmentsConstraintWrite(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT) c, kf);
      break;

    case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:
      KheSpreadEventsConstraintWrite((KHE_SPREAD_EVENTS_CONSTRAINT) c, kf);
      break;

    case KHE_LINK_EVENTS_CONSTRAINT_TAG:
      KheLinkEventsConstraintWrite((KHE_LINK_EVENTS_CONSTRAINT) c, kf);
      break;

    case KHE_ORDER_EVENTS_CONSTRAINT_TAG:
      KheOrderEventsConstraintWrite((KHE_ORDER_EVENTS_CONSTRAINT) c, kf);
      break;

    case KHE_AVOID_CLASHES_CONSTRAINT_TAG:
      KheAvoidClashesConstraintWrite((KHE_AVOID_CLASHES_CONSTRAINT) c, kf);
      break;

    case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:
      KheAvoidUnavailableTimesConstraintWrite(
	(KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT) c, kf);
      break;

    case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:
      KheLimitIdleTimesConstraintWrite((KHE_LIMIT_IDLE_TIMES_CONSTRAINT) c, kf);
      break;

    case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:
      KheClusterBusyTimesConstraintWrite(
	(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c, kf);
      break;

    case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:
      KheLimitBusyTimesConstraintWrite((KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c, kf);
      break;

    case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:
      KheLimitWorkloadConstraintWrite((KHE_LIMIT_WORKLOAD_CONSTRAINT) c, kf);
      break;

    case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:
      KheLimitActiveIntervalsConstraintWrite(
	(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c, kf);
      break;

    case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:
      KheLimitResourcesConstraintWrite((KHE_LIMIT_RESOURCES_CONSTRAINT) c, kf);
      break;

    default:
      HnAbort("KheConstraintWrite: unknown tag (internal error)");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheCostFunctionShow(KHE_COST_FUNCTION cf)                          */
/*                                                                           */
/*  Return the XML string value of cf.                                       */
/*                                                                           */
/*****************************************************************************/

char *KheCostFunctionShow(KHE_COST_FUNCTION cf)
{
  switch( cf )
  {
    case KHE_STEP_COST_FUNCTION:	  return "Step";
    case KHE_LINEAR_COST_FUNCTION:  	  return "Linear";
    case KHE_QUADRATIC_COST_FUNCTION:	  return "Quadratic";

    default:	HnAbort("unknown cost function value");
		return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintWriteCommonFields(KHE_CONSTRAINT c, KML_FILE kf)       */
/*                                                                           */
/*  Write the fields of c common to all constraints onto kf.                 */
/*                                                                           */
/*****************************************************************************/

void KheConstraintWriteCommonFields(KHE_CONSTRAINT c, KML_FILE kf)
{
  HnAssert(c->name != NULL, "KheArchiveWrite:  Name missing in constraint %s",
    c->id);
  KmlEltPlainText(kf, "Name", c->name);
  KmlEltPlainText(kf, "Required", c->required ? "true" : "false");
  KmlEltFmtText(kf, "Weight", "%d", c->weight);
  KmlEltPlainText(kf, "CostFunction", KheCostFunctionShow(c->cost_function));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "evaluation"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheConstraintCost(KHE_CONSTRAINT c, int dev)                    */
/*                                                                           */
/*  Calculate the cost when c has dev.                                       */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheMonitorDevToCost
KHE_COST KheConstraintCost(KHE_CONSTRAINT c, int dev)
{
  switch( c->cost_function )
  {
    case KHE_STEP_COST_FUNCTION:

        return dev > 0 ? c->combined_weight : 0;

    case KHE_LINEAR_COST_FUNCTION:

        return dev * c->combined_weight;

    case KHE_QUADRATIC_COST_FUNCTION:

	return dev * dev * c->combined_weight;

    default:

	HnAbort("KheConstraintCost internal error");
	return 0;  ** keep compiler happy **
  }
}
*** */

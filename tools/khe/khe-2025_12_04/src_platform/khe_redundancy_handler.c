
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
/*  FILE:         khe_redundancy_handler.c                                   */
/*  DESCRIPTION:  A redundancy handler                                       */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_REDUNDANCY_HANDLER                                                   */
/*                                                                           */
/*****************************************************************************/

struct khe_redundancy_handler_rec {
  KHE_SOLN				soln;		/* encl. solution    */
  KHE_RESOURCE_TYPE			resource_type;
  KHE_COST_FUNCTION			cost_fn;
  KHE_COST				combined_weight;
  ARRAY_KHE_PREFER_RESOURCES_MONITOR	monitors;
  KHE_REDUNDANCY_HANDLER		copy;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheEventResourceIsRedundant(KHE_EVENT_RESOURCE er)                  */
/*                                                                           */
/*  Return true if er is redundant.                                          */
/*                                                                           */
/*****************************************************************************/

static bool KheEventResourceIsRedundant(KHE_EVENT_RESOURCE er)
{
  int i;  KHE_CONSTRAINT c;  KHE_PREFER_RESOURCES_CONSTRAINT prc;
  KHE_RESOURCE_GROUP domain;
  for( i = 0;  i < KheEventResourceConstraintCount(er);  i++ )
  {
    c = KheEventResourceConstraint(er, i);
    if( KheConstraintCombinedWeight(c) > 0 ) switch( KheConstraintTag(c) )
    {
      case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:

	return false;

      case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:

	prc = (KHE_PREFER_RESOURCES_CONSTRAINT) c;
	domain = KhePreferResourcesConstraintDomain(prc);
	if( KheResourceGroupResourceCount(domain) == 0 )
	  return false;
	break;

      default:

	/* nothing to do here */
	break;
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRedundancyHandlerAddMonitors(KHE_REDUNDANCY_HANDLER rh)          */
/*                                                                           */
/*  Make and add monitors to rh.                                             */
/*                                                                           */
/*****************************************************************************/

static void KheRedundancyHandlerAddMonitors(KHE_REDUNDANCY_HANDLER rh)
{
  KHE_INSTANCE ins;  int i;  KHE_EVENT_RESOURCE er;
  KHE_PREFER_RESOURCES_MONITOR prm;  KHE_EVENT_RESOURCE_IN_SOLN ers;
  ins = KheSolnInstance(rh->soln);
  for( i = 0;  i < KheInstanceEventResourceCount(ins);  i++ )
  {
    er = KheInstanceEventResource(ins, i);
    if( KheEventResourceResourceType(er) == rh->resource_type &&
	KheEventResourceIsRedundant(er) )
    {
      ers = KheSolnEventResourceInSoln(rh->soln, er);
      prm = KhePreferResourcesMonitorMakeInternal(ers, NULL,
	rh->cost_fn, rh->combined_weight);
      KheMonitorAttachToSoln((KHE_MONITOR) prm);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) rh->soln,
	(KHE_MONITOR) prm);
      HaArrayAddLast(rh->monitors, prm);
      if( DEBUG1 )
	KhePreferResourcesMonitorDebug(prm, 2, 2, stderr);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_REDUNDANCY_HANDLER KheRedundancyHandlerMake(KHE_SOLN soln,           */
/*    KHE_RESOURCE_TYPE rt, KHE_COST_FUNCTION cf, KHE_COST weight)           */
/*                                                                           */
/*  Make a redundancy handler with these attributes.                         */
/*                                                                           */
/*****************************************************************************/

KHE_REDUNDANCY_HANDLER KheRedundancyHandlerMake(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_COST_FUNCTION cf, KHE_COST weight)
{
  HA_ARENA a;  KHE_REDUNDANCY_HANDLER res;

  /* build the object */
  if( DEBUG1 )
    fprintf(stderr, "[ KheRedundancyHandlerMake(soln of %s, %s, %s, %.5f)\n",
      KheInstanceId(KheSolnInstance(soln)), KheResourceTypeId(rt),
      KheCostFunctionShow(cf), KheCostShow(weight));
  a = KheSolnArena(soln);
  HaMake(res, a);
  res->soln = soln;
  res->resource_type = rt;
  res->cost_fn = cf;
  res->combined_weight = weight;
  HaArrayInit(res->monitors, a);
  res->copy = NULL;

  /* add the monitors and make sure they are attached */
  KheRedundancyHandlerAddMonitors(res);
  KheRedundancyHandlerAttachAllRedundancyMonitors(res);
  if( DEBUG1 )
  {
    KheRedundancyHandlerDebug(res, 2, 2, stderr);
    fprintf(stderr, "] KheRedundancyHandlerMake returning\n");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_REDUNDANCY_HANDLER KheRedundancyHandlerCopyPhase1(                   */
/*    KHE_REDUNDANCY_HANDLER rh, HA_ARENA a)                                 */
/*                                                                           */
/*  Carry out Phase 1 of copying for rh.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_REDUNDANCY_HANDLER KheRedundancyHandlerCopyPhase1(
  KHE_REDUNDANCY_HANDLER rh, HA_ARENA a)
{
  KHE_REDUNDANCY_HANDLER copy;  KHE_PREFER_RESOURCES_MONITOR prm, prm2;  int i;
  if( rh->copy == NULL )
  {
    HaMake(copy, a);
    copy->soln = KheSolnCopyPhase1(rh->soln);
    copy->resource_type = rh->resource_type;
    HaArrayInit(copy->monitors, a);
    HaArrayForEach(rh->monitors, prm, i)
    {
      prm2 = KhePreferResourcesMonitorCopyPhase1(prm, a);
      HaArrayAddLast(copy->monitors, prm2);
    }
    copy->copy = NULL;
  }
  return rh->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRedundancyHandlerCopyPhase2(KHE_REDUNDANCY_HANDLER rh)           */
/*                                                                           */
/*  Carry out Phase 2 of copying for rh.                                     */
/*                                                                           */
/*****************************************************************************/

void KheRedundancyHandlerCopyPhase2(KHE_REDUNDANCY_HANDLER rh)
{
  KHE_PREFER_RESOURCES_MONITOR prm;  int i;
  if( rh->copy != NULL )
  {
    rh->copy = NULL;
    HaArrayForEach(rh->monitors, prm, i)
      KhePreferResourcesMonitorCopyPhase2(prm);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRedundancyHandlerDelete(KHE_REDUNDANCY_HANDLER rh)               */
/*                                                                           */
/*  Delete rh.                                                               */
/*                                                                           */
/*****************************************************************************/

void KheRedundancyHandlerDelete(KHE_REDUNDANCY_HANDLER rh)
{
  if( DEBUG1 )
    fprintf(stderr, "[ KheRedundancyHandlerDelete(soln of %s, %s)\n",
      KheInstanceId(KheSolnInstance(rh->soln)),
      KheResourceTypeId(rh->resource_type));
  /* can't really delete, but detaching is good enough */
  KheRedundancyHandlerDetachAllRedundancyMonitors(rh);
  if( DEBUG1 )
    fprintf(stderr, "]\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "configuration"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheRedundancyHandlerAttachAllRedundancyMonitors(                    */
/*    KHE_REDUNDANCY_HANDLER rh)                                             */
/*                                                                           */
/*  Attach all redundancy monitors.                                          */
/*                                                                           */
/*****************************************************************************/

void KheRedundancyHandlerAttachAllRedundancyMonitors(
  KHE_REDUNDANCY_HANDLER rh)
{
  KHE_PREFER_RESOURCES_MONITOR prm;  int i;
  HaArrayForEach(rh->monitors, prm, i)
    if( !KheMonitorAttachedToSoln((KHE_MONITOR) prm) )
      KheMonitorAttachToSoln((KHE_MONITOR) prm);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRedundancyHandlerDetachAllRedundancyMonitors(                    */
/*    KHE_REDUNDANCY_HANDLER rh)                                             */
/*                                                                           */
/*  Detach all redundancy monitors.                                          */
/*                                                                           */
/*****************************************************************************/

void KheRedundancyHandlerDetachAllRedundancyMonitors(
  KHE_REDUNDANCY_HANDLER rh)
{
  KHE_PREFER_RESOURCES_MONITOR prm;  int i;
  HaArrayForEach(rh->monitors, prm, i)
    if( KheMonitorAttachedToSoln((KHE_MONITOR) prm) )
      KheMonitorDetachFromSoln((KHE_MONITOR) prm);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheRedundancyHandlerDebug(KHE_REDUNDANCY_HANDLER rh,                */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of rh onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

void KheRedundancyHandlerDebug(KHE_REDUNDANCY_HANDLER rh,
  int verbosity, int indent, FILE *fp)
{
  KHE_PREFER_RESOURCES_MONITOR prm;  int i;
  if( indent >= 0 )
  {
    fprintf(stderr,"%*s[ Redundancy Handler (%s, %s, %.5f, %d monitors):\n",
      indent, "", KheResourceTypeId(rh->resource_type),
      KheCostFunctionShow(rh->cost_fn), KheCostShow(rh->combined_weight),
      HaArrayCount(rh->monitors));
    if( verbosity >= 2 )
      HaArrayForEach(rh->monitors, prm, i)
	KhePreferResourcesMonitorDebug(prm, verbosity, indent + 2, fp);
    fprintf(stderr, "%*s]\n", indent, "");
  }
}

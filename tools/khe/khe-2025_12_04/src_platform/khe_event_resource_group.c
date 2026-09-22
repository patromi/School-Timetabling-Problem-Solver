
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
/*  FILE:         khe_event_resource_group.c                                 */
/*  DESCRIPTION:  An event resource group                                    */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE_GROUP - an event resource group                       */
/*                                                                           */
/*****************************************************************************/

struct khe_event_resource_group_rec {
  KHE_EVENT		event;			/* enclosing event           */
  KHE_RESOURCE_GROUP	resource_group;		/* resource group            */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE_GROUP KheEventResourceGroupMake(KHE_EVENT event,      */
/*    KHE_RESOURCE_GROUP rg)                                                 */
/*                                                                           */
/*  Make an event resource group with these attributes and add it to event.  */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_RESOURCE_GROUP KheEventResourceGroupMake(KHE_EVENT event,
  KHE_RESOURCE_GROUP rg)
{
  KHE_EVENT_RESOURCE_GROUP res;  KHE_RESOURCE r;  int i;
  KHE_EVENT_RESOURCE er;  HA_ARENA a;

  /* make the event resource group object */
  HnAssert(event != NULL, "KheEventResourceGroupMake: event is NULL");
  HnAssert(rg != NULL, "KheEventResourceGroupMake: rg is NULL");
  a = KheInstanceArena(KheEventInstance(event));
  HaMake(res, a);
  res->event = event;
  res->resource_group = rg;
  KheEventAddEventResourceGroup(event, res);

  /* add one event resource for each resource of rg */
  for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
  {
    r = KheResourceGroupResource(rg, i);
    KheEventResourceMake(event, KheResourceGroupResourceType(rg), r, NULL,
      KheEventWorkload(event), &er);
    HnAssert(er != NULL, "KheEventResourceGroupMake internal error");
    KheEventResourceSetEventResourceGroup(er, res);
  }

  /* return the event resource group */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheEventResourceGroupEvent(KHE_EVENT_RESOURCE_GROUP erg)       */
/*                                                                           */
/*  Return the event attribute of erg.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KheEventResourceGroupEvent(KHE_EVENT_RESOURCE_GROUP erg)
{
  return erg->event;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheEventResourceGroupResourceGroup(                   */
/*    KHE_EVENT_RESOURCE_GROUP erg)                                          */
/*                                                                           */
/*  Return the resource group attribute of erg.                              */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheEventResourceGroupResourceGroup(
  KHE_EVENT_RESOURCE_GROUP erg)
{
  return erg->resource_group;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceGroupDebug(KHE_EVENT_RESOURCE_GROUP erg,            */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of erg onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceGroupDebug(KHE_EVENT_RESOURCE_GROUP erg,
  int verbosity, int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    KheEventDebug(erg->event, 1, -1, fp);
    fprintf(fp, ":");
    KheResourceGroupDebug(erg->resource_group, 1, -1, fp);
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


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
/*  FILE:         khe_event.c                                                */
/*  DESCRIPTION:  An event                                                   */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"
#define DEBUG2 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT - an event                                                     */
/*                                                                           */
/*****************************************************************************/

struct khe_event_rec {
  void				*back;			/* back pointer      */
  KHE_INSTANCE			instance;		/* enclosing instance*/
  char				*id;			/* Id                */
  char				*name;			/* Name              */
  char				*color;			/* optional Color    */
  int				duration;		/* duration          */
  int				workload;		/* workload          */
  KHE_TIME			preassigned_time;	/* optional time     */
  int				index;			/* index in ins      */
  bool				partition_admissible;	/* when inferring    */
  ARRAY_KHE_EVENT_RESOURCE	event_resources;	/* event resources   */
  ARRAY_KHE_EVENT_RESOURCE_GROUP event_resource_groups;	/* resource groups   */
  ARRAY_KHE_CONSTRAINT		constraints;		/* constraints       */
  ARRAY_KHE_EVENT_GROUP		user_event_groups;	/* event groups      */
  KHE_EVENT_GROUP		singleton_event_group;	/* singleton         */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheEventMake(KHE_INSTANCE ins, char *id, char *name, char *color,   */
/*    int workload, KHE_TIME preassigned_time, KHE_EVENT *e)                 */
/*                                                                           */
/*  Make a new event, add it to the instance, and return it.                 */
/*                                                                           */
/*****************************************************************************/

bool KheEventMake(KHE_INSTANCE ins, char *id, char *name, char *color,
  int duration, int workload, KHE_TIME preassigned_time, KHE_EVENT *e)
{
  KHE_EVENT res;  HA_ARENA a;
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KheEventMake called after KheInstanceMakeEnd");
  if( id != NULL && KheInstanceRetrieveEvent(ins, id, &res) )
  {
    *e = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  HaMake(res, a);
  res->back = NULL;
  res->instance = ins;
  res->id = HnStringCopy(id, a);
  res->name = HnStringCopy(name, a);
  res->color = HnStringCopy(color, a);
  res->duration = duration;
  res->workload = workload;
  res->preassigned_time = preassigned_time;
  if( preassigned_time != NULL )
    KheTimeAddPreassignedEvent(preassigned_time, res);
  res->index = KheInstanceEventCount(ins);
  res->partition_admissible = false;
  HaArrayInit(res->event_resources, a);
  HaArrayInit(res->event_resource_groups, a);
  HaArrayInit(res->constraints, a);
  HaArrayInit(res->user_event_groups, a);
  res->singleton_event_group = KheEventGroupMakeInternal(ins,
    KHE_EVENT_GROUP_TYPE_SINGLETON, NULL, KHE_EVENT_GROUP_KIND_ORDINARY,
    NULL, NULL);
  KheEventGroupAddEventInternal(res->singleton_event_group, res);
  KheEventGroupAddEventInternal(KheInstanceFullEventGroup(ins), res);
  KheInstanceAddEvent(ins, res);
  *e = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventSetBack(KHE_EVENT e, void *back)                            */
/*                                                                           */
/*  Set the back pointer of e.                                               */
/*                                                                           */
/*****************************************************************************/

void KheEventSetBack(KHE_EVENT e, void *back)
{
  HnAssert(KheInstanceFinalized(e->instance) == KHE_FINALIZED_NONE,
    "KheEventSetBack called after KheInstanceMakeEnd");
  e->back = back;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheEventBack(KHE_EVENT e)                                          */
/*                                                                           */
/*  Return the back pointer of e.                                            */
/*                                                                           */
/*****************************************************************************/

void *KheEventBack(KHE_EVENT e)
{
  return e->back;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventAddUserEventGroup(KHE_EVENT e, KHE_EVENT_GROUP eg)          */
/*                                                                           */
/*  Add user event group eg to e.                                            */
/*                                                                           */
/*****************************************************************************/

void KheEventAddUserEventGroup(KHE_EVENT e, KHE_EVENT_GROUP eg)
{
  HaArrayAddLast(e->user_event_groups, eg);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INSTANCE KheEventInstance(KHE_EVENT e)                               */
/*                                                                           */
/*  Return the instance attribute of e.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_INSTANCE KheEventInstance(KHE_EVENT e)
{
  return e->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheEventId(KHE_EVENT e)                                            */
/*                                                                           */
/*  Return the id attribute of e.                                            */
/*                                                                           */
/*****************************************************************************/

char *KheEventId(KHE_EVENT e)
{
  return e->id;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheEventName(KHE_EVENT e)                                          */
/*                                                                           */
/*  Return the name attribute of e.                                          */
/*                                                                           */
/*****************************************************************************/

char *KheEventName(KHE_EVENT e)
{
  return e->name;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheEventColor(KHE_EVENT e)                                         */
/*                                                                           */
/*  Return the color associated with e, possibly NULL.                       */
/*                                                                           */
/*****************************************************************************/

char *KheEventColor(KHE_EVENT e)
{
  return e->color;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventDuration(KHE_EVENT e)                                        */
/*                                                                           */
/*  Return the duration attribute of e.                                      */
/*                                                                           */
/*****************************************************************************/

int KheEventDuration(KHE_EVENT e)
{
  return e->duration;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventWorkload(KHE_EVENT e)                                        */
/*                                                                           */
/*  Return the workload attribute of e.                                      */
/*                                                                           */
/*****************************************************************************/

int KheEventWorkload(KHE_EVENT e)
{
  return e->workload;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventDemand(KHE_EVENT e)                                          */
/*                                                                           */
/*  Return the demand of e (duration times number of event resources).       */
/*                                                                           */
/*****************************************************************************/

int KheEventDemand(KHE_EVENT e)
{
  return e->duration * HaArrayCount(e->event_resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheEventPreassignedTime(KHE_EVENT e)                            */
/*                                                                           */
/*  Return the preassigned time of e, possibly NULL.                         */
/*                                                                           */
/*****************************************************************************/

KHE_TIME KheEventPreassignedTime(KHE_EVENT e)
{
  return e->preassigned_time;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventIndex(KHE_EVENT e)                                           */
/*                                                                           */
/*  Return the index number of e.                                            */
/*                                                                           */
/*****************************************************************************/

int KheEventIndex(KHE_EVENT e)
{
  return e->index;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time domains"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventFinalize(KHE_EVENT e)                                       */
/*                                                                           */
/*  Finalize e, which means finalizing its event resources and setting their */
/*  equivalence classes.  This function is called by KheInstanceMakeEnd.     */
/*                                                                           */
/*****************************************************************************/

void KheEventFinalize(KHE_EVENT e)
{
  KHE_EVENT_RESOURCE er /* , prev_er */;  int i /* , equiv_class */;
  if( DEBUG2 )
    fprintf(stderr, "[ KheEventFinalize(%s)\n", e->id != NULL ? e->id : "-");

  /* finalize the event resources and set their equivalence classes */
  HaArrayForEach(e->event_resources, er, i)
    KheEventResourceFinalize(er);

  /* *** old version that created equivalence classes
  prev_er = NULL;
  equiv_class = -1;
  HaArrayForEach(e->event_resources, er, i)
  {
    KheEventResourceFinalize(er);
    if( prev_er == NULL || !KheEventResourceDoEquiv(prev_er, er) )
      equiv_class++;
    KheEventResourceSetEquivClass(er, equiv_class);
    prev_er = er;
  }
  *** */

  if( DEBUG2 )
    fprintf(stderr, "] KheEventFinalize(%s)\n", e->id != NULL ? e->id : "-");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "similarity"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventPartitionSetAdmissible(KHE_EVENT e)                         */
/*                                                                           */
/*  Decide whether e is admissible and set its flag if so.                   */
/*                                                                           */
/*****************************************************************************/

void KheEventPartitionSetAdmissible(KHE_EVENT e)
{
  KHE_EVENT_RESOURCE er;  int i;
  HaArrayForEach(e->event_resources, er, i)
    if( KheResourceGroupPartitionAdmissible(KheEventResourceHardDomain(er)) )
    {
      e->partition_admissible = true;
      return;
    }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventPartitionAdmissible(KHE_EVENT e)                            */
/*                                                                           */
/*  Return true if e is admissible when inferring resource partitions.       */
/*                                                                           */
/*****************************************************************************/

bool KheEventPartitionAdmissible(KHE_EVENT e)
{
  return e->partition_admissible;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventPartitionSimilar(KHE_EVENT e1, KHE_EVENT e2,                */
/*    ARRAY_KHE_RESOURCE_GROUP *domains1, ARRAY_KHE_RESOURCE_GROUP *domains2)*/
/*                                                                           */
/*  Return true if e1 and e2 are similar for the purposes of inferring       */
/*  resource partitions.  Parameters *domains1 and *domains2 are scratch     */
/*  arrays that are used to hold the admissible hard domains of the events.  */
/*                                                                           */
/*****************************************************************************/

bool KheEventPartitionSimilar(KHE_EVENT e1, KHE_EVENT e2,
  ARRAY_KHE_RESOURCE_GROUP *domains1, ARRAY_KHE_RESOURCE_GROUP *domains2)
{
  KHE_RESOURCE_GROUP rg1, rg2;  KHE_EVENT_RESOURCE er;  int i1, i2;

  /* carry out simple tests */
  if( e1 == e2 )
    return true;
  if( e1->duration != e2->duration )
    return false;
  if( !e1->partition_admissible || !e2->partition_admissible )
    return false;

  /* get the domains of the admissible event resources of e1 and e2 */
  HaArrayClear(*domains1);
  HaArrayForEach(e1->event_resources, er, i1)
    if( KheResourceGroupPartitionAdmissible(KheEventResourceHardDomain(er)) )
      HaArrayAddLast(*domains1, KheEventResourceHardDomain(er));
  HaArrayClear(*domains2);
  HaArrayForEach(e2->event_resources, er, i2)
    if( KheResourceGroupPartitionAdmissible(KheEventResourceHardDomain(er)) )
      HaArrayAddLast(*domains2, KheEventResourceHardDomain(er));

  /* check them for equality, in any order */
  if( HaArrayCount(*domains1) != HaArrayCount(*domains2) )
    return false;
  HaArrayForEach(*domains1, rg1, i1)
  {
    HaArrayForEach(*domains2, rg2, i2)
      if( KheResourceGroupEqual(rg1, rg2) )
	break;
    if( i2 >= HaArrayCount(*domains2) )
      return false;
    HaArrayDeleteAndShift(*domains2, i2);
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventSimilar(KHE_EVENT e1, KHE_EVENT e2)                         */
/*                                                                           */
/*  Return true if e1 and e2 are similar.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheEventSimilar(KHE_EVENT e1, KHE_EVENT e2)
{
  ARRAY_KHE_RESOURCE_GROUP domains1, domains2;  bool res;  HA_ARENA a;
  if( e1 == e2 )
    return true;
  a = KheInstanceArenaBegin(e1->instance);
  HaArrayInit(domains1, a);
  HaArrayInit(domains2, a);
  res = KheEventPartitionSimilar(e1, e2, &domains1, &domains2);
  KheInstanceArenaEnd(e1->instance, a);
  /* ***
  MArrayFree(domains1);
  MArrayFree(domains2);
  *** */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventResourceCorrespond(KHE_EVENT_RESOURCE er1,                  */
/*    KHE_EVENT_RESOURCE er2)                                                */
/*                                                                           */
/*  Return true if er1 and er2 correspond:  if they have the same            */
/*  resource type, the same preassigned resource (or NULL), the same         */
/*  hard domain, and the same hard-and-soft domain.                          */
/*                                                                           */
/*****************************************************************************/

static bool KheEventResourceCorrespond(KHE_EVENT_RESOURCE er1,
  KHE_EVENT_RESOURCE er2)
{
  return KheEventResourceResourceType(er1) == KheEventResourceResourceType(er2)
    && KheEventResourcePreassignedResource(er1) ==
         KheEventResourcePreassignedResource(er2)
    && KheResourceGroupEqual(KheEventResourceHardDomain(er1),
	 KheEventResourceHardDomain(er2))
    && KheResourceGroupEqual(KheEventResourceHardAndSoftDomain(er1),
	 KheEventResourceHardAndSoftDomain(er2));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventHasCorrespondingEventResource(KHE_EVENT e,                  */
/*    KHE_EVENT_RESOURCE er)                                                 */
/*                                                                           */
/*  Return true if e has an event resource corresponding to er.              */
/*                                                                           */
/*****************************************************************************/

static bool KheEventHasCorrespondingEventResource(KHE_EVENT e,
  KHE_EVENT_RESOURCE er)
{
  KHE_EVENT_RESOURCE er1;  int i;
  HaArrayForEach(e->event_resources, er1, i)
    if( KheEventResourceCorrespond(er, er1) )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventMergeable(KHE_EVENT e1, KHE_EVENT e2, int slack)            */
/*                                                                           */
/*  Return true if e1 and e2 are mergeable:  if their event resources        */
/*  correspond, to within slack.                                             */
/*                                                                           */
/*****************************************************************************/
#define swap(a, b, tmp) (tmp = a, a = b, b = tmp)

bool KheEventMergeable(KHE_EVENT e1, KHE_EVENT e2, int slack)
{
  int size1, size2, bad_e2, i;  KHE_EVENT tmp_e;  KHE_EVENT_RESOURCE er1;

  /* make sure e2 has at least as many event resources as e1 */
  size1 = HaArrayCount(e1->event_resources);
  size2 = HaArrayCount(e2->event_resources);
  if( size1 > size2 )
  {
    swap(e1, e2, tmp_e);
    swap(size1, size2, i);
  }

  /* now e2's problems must be at least as great as e1's */
  bad_e2 = size2 - size1;
  if( bad_e2 > slack )
    return false;
  HaArrayForEach(e1->event_resources, er1, i)
    if( !KheEventHasCorrespondingEventResource(e2, er1) )
    {
      bad_e2++;
      if( bad_e2 > slack )
	return false;
    }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventSharePreassignedResource(KHE_EVENT e1, KHE_EVENT e2,        */
/*    KHE_RESOURCE *r)                                                       */
/*                                                                           */
/*  If e1 and e2 have a preassigned resource in common for which there is    */
/*  a hard avoid clashes constraint, return true and set *r to one such      */
/*  resource.  Otherwise return false and set *r to NULL.                    */
/*                                                                           */
/*****************************************************************************/

bool KheEventSharePreassignedResource(KHE_EVENT e1, KHE_EVENT e2,
  KHE_RESOURCE *r)
{
  int i1, i2;  KHE_EVENT_RESOURCE er1, er2;  KHE_RESOURCE r1;
  HaArrayForEach(e1->event_resources, er1, i1)
  {
    r1 = KheEventResourcePreassignedResource(er1);
    if( r1 != NULL && KheResourceLayerDuration(r1) > 0 )
    {
      HaArrayForEach(e2->event_resources, er2, i2)
	if( KheEventResourcePreassignedResource(er2) == r1 )
	{
	  *r = r1;
	  return true;
	}
    }
  }
  *r = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "event resources"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventAddEventResource(KHE_EVENT e, KHE_EVENT_RESOURCE er,        */
/*    int *index)                                                            */
/*                                                                           */
/*  Add er to e, and set *index to its index number in e.                    */
/*                                                                           */
/*****************************************************************************/

void KheEventAddEventResource(KHE_EVENT e, KHE_EVENT_RESOURCE er,
  int *index)
{
  *index = HaArrayCount(e->event_resources);
  HaArrayAddLast(e->event_resources, er);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventResourceCount(KHE_EVENT e)                                   */
/*                                                                           */
/*  Return the number of event resources of e.                               */
/*                                                                           */
/*****************************************************************************/

int KheEventResourceCount(KHE_EVENT e)
{
  return HaArrayCount(e->event_resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE KheEventResource(KHE_EVENT e, int i)                  */
/*                                                                           */
/*  Return the i'th event resource of e.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_RESOURCE KheEventResource(KHE_EVENT e, int i)
{
  return HaArray(e->event_resources, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventRetrieveEventResource(KHE_EVENT e, char *role,              */
/*    KHE_EVENT_RESOURCE *er)                                                */
/*                                                                           */
/*  If e contains an event resource with the given role, set *er to it and   */
/*  return true; otherwise, return false without changing *er.               */
/*                                                                           */
/*  Implementation note.  We don't bother with a hash table, because         */
/*  most events have only a few event resources.                             */
/*                                                                           */
/*****************************************************************************/

bool KheEventRetrieveEventResource(KHE_EVENT e, char *role,
  KHE_EVENT_RESOURCE *er)
{
  int i;  KHE_EVENT_RESOURCE er2;  char *role2;
  for( i = 0;  i < HaArrayCount(e->event_resources);  i++ )
  {
    er2 = HaArray(e->event_resources, i);
    role2 = KheEventResourceRole(er2);
    if( role2 != NULL && strcmp(role2, role) == 0 )
    {
      *er = er2;
      return true;
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "event resource groups"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventAddEventResourceGroup(KHE_EVENT event,                      */
/*    KHE_EVENT_RESOURCE_GROUP erg)                                          */
/*                                                                           */
/*  Add erg to event.                                                        */
/*                                                                           */
/*****************************************************************************/

void KheEventAddEventResourceGroup(KHE_EVENT event,
  KHE_EVENT_RESOURCE_GROUP erg)
{
  HaArrayAddLast(event->event_resource_groups, erg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventResourceGroupCount(KHE_EVENT e)                              */
/*                                                                           */
/*  Return the number of event resource groups of e.                         */
/*                                                                           */
/*****************************************************************************/

int KheEventResourceGroupCount(KHE_EVENT e)
{
  return HaArrayCount(e->event_resource_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE_GROUP KheEventResourceGroup(KHE_EVENT e, int i)       */
/*                                                                           */
/*  Return the i'th event resource group of e.                               */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_RESOURCE_GROUP KheEventResourceGroup(KHE_EVENT e, int i)
{
  return HaArray(e->event_resource_groups, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "auto-generated event groups"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_GROUP KheEventSingletonEventGroup(KHE_EVENT e)                 */
/*                                                                           */
/*  Return the event group containing just e.                                */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_GROUP KheEventSingletonEventGroup(KHE_EVENT e)
{
  return e->singleton_event_group;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "constraints"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventAddConstraint(KHE_EVENT e, KHE_CONSTRAINT c)                */
/*                                                                           */
/*  Add c to e.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheEventAddConstraint(KHE_EVENT e, KHE_CONSTRAINT c)
{
  HaArrayAddLast(e->constraints, c);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventConstraintCount(KHE_EVENT e)                                 */
/*                                                                           */
/*  Return the number of constraints applicable to e.                        */
/*                                                                           */
/*****************************************************************************/

int KheEventConstraintCount(KHE_EVENT e)
{
  return HaArrayCount(e->constraints);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT KheEventConstraint(KHE_EVENT e, int i)                    */
/*                                                                           */
/*  Return the i'th constraint applicable to e.                              */
/*                                                                           */
/*****************************************************************************/

KHE_CONSTRAINT KheEventConstraint(KHE_EVENT e, int i)
{
  return HaArray(e->constraints, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time ranges and sweep times"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheEventTimeRange(KHE_EVENT e, KHE_SOLN soln,                       */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Return the time range of e in soln.                                      */
/*                                                                           */
/*****************************************************************************/

bool KheEventTimeRange(KHE_EVENT e, KHE_SOLN soln,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  int i;  KHE_MEET meet;  KHE_TIME ft, lt;
  *first_time = *last_time = NULL;
  for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
  {
    meet = KheEventMeet(soln, e, i);
    HnAssert(meet != NULL, "KheEventTimeRange internal error");
    ft = KheMeetAsstTime(meet);
    if( ft == NULL )
      return false;
    lt = KheTimeNeighbour(ft, KheMeetDuration(meet) - 1);
    if( *first_time == NULL || KheTimeIndex(ft) < KheTimeIndex(*first_time) )
      *first_time = ft;
    if( *last_time == NULL || KheTimeIndex(lt) > KheTimeIndex(*last_time) )
      *last_time = lt;
  }
  return *first_time != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheEventMakeFromKml(KML_ELT event_elt, KHE_INSTANCE ins,            */
/*    KML_ERROR *ke)                                                         */
/*                                                                           */
/*  Make an event from event_elt and add it to ins.                          */
/*                                                                           */
/*****************************************************************************/

bool KheEventMakeFromKml(KML_ELT event_elt, KHE_INSTANCE ins,
  KML_ERROR *ke)
{
  KML_ELT workload_elt, time_elt, event_groups_elt, event_group_elt;
  KML_ELT course_elt, resources_elt, resource_elt, rgroups_elt, rgroup_elt;
  int i, workload, duration;  char *id, *name, *color, *ref, *syntax;
  KHE_EVENT e;  KHE_EVENT_GROUP eg;  KHE_TIME preassigned_time;
  KHE_RESOURCE_GROUP rg;  HA_ARENA a;

  /* check event_elt */
  a = KheInstanceArena(ins);
  if( !KmlCheck(event_elt, "Id +Color : $Name #Duration +#Workload +Course"
      " +Time +Resources +ResourceGroups +EventGroups", ke) )
    return false;

  /* Id, Name, and Color */
  id = KmlAttributeValue(event_elt, 0);
  name = KmlText(KmlChild(event_elt, 0));
  if( KmlAttributeCount(event_elt) == 2 )
    color = KmlAttributeValue(event_elt, 1);
  else
    color = NULL;

  /* Duration and Workload */
  sscanf(KmlText(KmlChild(event_elt, 1)), "%d", &duration);
  if( KmlContainsChild(event_elt, "Workload", &workload_elt) )
    sscanf(KmlText(workload_elt), "%d", &workload);
  else
    workload = duration;

  /* Time */
  preassigned_time = NULL;
  if( KmlContainsChild(event_elt, "Time", &time_elt) )
  {
    if( !KmlCheck(time_elt, "Reference", ke) )
      return false;
    ref = KmlAttributeValue(time_elt, 0);
    if( !KheInstanceRetrieveTime(ins, ref, &preassigned_time) )
      return KmlError(ke, a, KmlLineNum(time_elt), KmlColNum(time_elt),
	"in <Time>, Reference \"%s\" unknown", ref);
  }

  /* make and add the event */
  if( !KheEventMake(ins,id,name,color,duration,workload,preassigned_time, &e) )
    return KmlError(ke, a, KmlLineNum(event_elt), KmlColNum(event_elt),
      "in <Event>, Id \"%s\" used previously", id);

  /* Course */
  if( KmlContainsChild(event_elt, "Course", &course_elt) )
  {
    if( !KmlCheck(course_elt, "Reference", ke) )
      return false;
    ref = KmlAttributeValue(course_elt, 0);
    if( !KheInstanceRetrieveEventGroup(ins, ref, &eg) )
      return KmlError(ke, a, KmlLineNum(course_elt), KmlColNum(course_elt),
	"in <Course>, Reference \"%s\" unknown", ref);
    KheEventGroupAddEvent(eg, e);
  }

  /* Resources */
  if( KmlContainsChild(event_elt, "Resources", &resources_elt) )
  {
    syntax = (KheInstanceModel(ins) == KHE_MODEL_HIGH_SCHOOL_TIMETABLE ?
      ": *Resource" : ": *Resource *R");
    if( !KmlCheck(resources_elt, syntax, ke) )
      return false;
    for( i = 0;  i < KmlChildCount(resources_elt);  i++ )
    {
      resource_elt = KmlChild(resources_elt, i);
      if( !KheEventResourceMakeFromKml(resource_elt, e, ke) )
	return false;
    }
  }

  /* ResourceGroups */
  if( KmlContainsChild(event_elt, "ResourceGroups", &rgroups_elt) )
  {
    if( !KmlCheck(rgroups_elt, ": *ResourceGroup", ke) )
      return false;
    for( i = 0;  i < KmlChildCount(rgroups_elt);  i++ )
    {
      rgroup_elt = KmlChild(rgroups_elt, i);
      if( !KmlCheck(rgroup_elt, "Reference", ke) )
	return false;
      ref = KmlAttributeValue(rgroup_elt, 0);
      if( !KheInstanceRetrieveResourceGroup(ins, ref, &rg) )
	return KmlError(ke, a, KmlLineNum(rgroup_elt), KmlColNum(rgroup_elt),
	  "in <ResourceGroup>, Reference \"%s\" unknown", ref);
      KheEventResourceGroupMake(e, rg);
    }
  }

  /* EventGroups */
  if( KmlContainsChild(event_elt, "EventGroups", &event_groups_elt) )
  {
    if( !KmlCheck(event_groups_elt, ": *EventGroup", ke) )
      return false;
    for( i = 0;  i < KmlChildCount(event_groups_elt);  i++ )
    {
      event_group_elt = KmlChild(event_groups_elt, i);
      if( !KmlCheck(event_group_elt, "Reference", ke) )
	return false;
      ref = KmlAttributeValue(event_group_elt, 0);
      if( !KheInstanceRetrieveEventGroup(ins, ref, &eg) )
	return KmlError(ke, a, KmlLineNum(event_group_elt),
	  KmlColNum(event_group_elt),
	  "in <EventGroup>, Reference \"%s\" unknown", ref);
      KheEventGroupAddEvent(eg, e);
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventWriteAbbreviatedEventResources(KHE_EVENT e)                 */
/*                                                                           */
/*  Return true if the event resources of e can be written abbreviated.      */
/*                                                                           */
/*****************************************************************************/

static bool KheEventWriteAbbreviatedEventResources(KHE_EVENT e)
{
  KHE_EVENT_RESOURCE er;  int i;
  if( KheInstanceModel(e->instance) == KHE_MODEL_HIGH_SCHOOL_TIMETABLE )
    return false;
  HaArrayForEach(e->event_resources, er, i)
    if( !KheEventResourceWriteAbbreviated(er) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventWrite(KHE_EVENT e, KML_FILE kf)                             */
/*                                                                           */
/*  Write e to kf.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheEventWrite(KHE_EVENT e, KML_FILE kf)
{
  KHE_EVENT_RESOURCE_GROUP erg;  int i;  char *id;  KHE_EVENT_GROUP eg, ceg;
  bool abbreviated, resources_started;  KHE_EVENT_RESOURCE er;

  /* header and name */
  KmlBegin(kf, "Event");
  HnAssert(e->id != NULL, "KheArchiveWrite: Id missing from Event");
  KmlAttribute(kf, "Id", e->id);
  if( e->color != NULL )
    KmlAttribute(kf, "Color", e->color);
  HnAssert(e->name != NULL, "KheArchiveWrite: Name missing from Event");
  KmlEltPlainText(kf, "Name", e->name);

  /* duration and workload */
  KmlEltFmtText(kf, "Duration", "%d", e->duration);
  if( e->workload != e->duration )
    KmlEltFmtText(kf, "Workload", "%d", e->workload);

  /* optional Course (does not necessarily come first in list, sadly; */
  /* this is because each event group gets added when the event group */
  /* is finalized, and when an event group is used within a constraint, */
  /* it gets finalized early) */
  ceg = NULL;
  HaArrayForEach(e->user_event_groups, eg, i)
    if( KheEventGroupKind(eg) == KHE_EVENT_GROUP_KIND_COURSE )
    {
      /* make this one the course; it's heuristic but it works */
      ceg = eg;
      HnAssert(KheEventGroupId(eg) != NULL, "KheArchiveWrite: Id missing"
	" from Course referenced by event %s", e->id);
      KmlEltAttribute(kf, "Course", "Reference", KheEventGroupId(eg));
      break;
    }

  /* ***
  start = 0;
  if( HaArrayCount(e->user_event_groups) > 0 )
  {
    eg = MArrayFirst(e->user_event_groups);
    if( KheEventGroupKind(eg) == KHE_EVENT_GROUP_KIND_COURSE )
    {
      HnAssert(KheEventGroupId(eg) != NULL, "KheArchiveWrite: Id missing"
	" from Course referenced by event %s", e->id);
      KmlEltAttribute(kf, "Course", "Reference", KheEventGroupId(eg));
      start = 1;
    }
  }
  *** */

  /* optional preassigned time */
  if( e->preassigned_time != NULL )
    KmlEltAttribute(kf, "Time", "Reference", KheTimeId(e->preassigned_time));

  /* optional event resources */
  abbreviated = KheEventWriteAbbreviatedEventResources(e);
  resources_started = false;
  HaArrayForEach(e->event_resources, er, i)
  {
    if( KheEventResourceEventResourceGroup(er) == NULL )
    {
      if( !resources_started )
      {
	KmlBegin(kf, "Resources");
        resources_started = true;
      }
      KheEventResourceWrite(er, abbreviated, kf);
    }
  }
  if( resources_started )
    KmlEnd(kf, "Resources");

  /* optional resource groups */
  if( HaArrayCount(e->event_resource_groups) > 0 )
  {
    KmlBegin(kf, "ResourceGroups");
    HaArrayForEach(e->event_resource_groups, erg, i)
    {
      id = KheResourceGroupId(KheEventResourceGroupResourceGroup(erg));
      HnAssert(id != NULL, "KheArchiveWrite: Id missing"
	" from resource group referenced by event %s", e->id);
      KmlEltAttribute(kf, "ResourceGroup", "Reference", id);
    }
    KmlEnd(kf, "ResourceGroups");
  }

  /* optional event groups */
  if( HaArrayCount(e->user_event_groups) > (ceg != NULL ? 1 : 0) )
  {
    KmlBegin(kf, "EventGroups");
    HaArrayForEach(e->user_event_groups, eg, i)
      if( eg != ceg )
      {
	HnAssert(KheEventGroupId(eg) != NULL, "KheArchiveWrite: Id missing"
	  " from EventGroup referenced by event %s", e->id);
	KmlEltAttribute(kf, "EventGroup", "Reference", KheEventGroupId(eg));
      }
    KmlEnd(kf, "EventGroups");
  }

  /* footer */
  KmlEnd(kf, "Event");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventDebug(KHE_EVENT e, int verbosity, int indent, FILE *fp)     */
/*                                                                           */
/*  Debug print of e onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheEventDebug(KHE_EVENT e, int verbosity, int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "%s", e->id != NULL ? e->id : "-");
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}

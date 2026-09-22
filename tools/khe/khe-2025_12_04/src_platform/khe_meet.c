
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
/*  FILE:         khe_meet.c                                                 */
/*  DESCRIPTION:  A meet                                                     */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"
#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG8 0
#define DEBUG11 0
#define DEBUG13 0
#define DEBUG14 0
#define DEBUG15 0
#define DEBUG16 0
#define DEBUG18 0
#define DEBUG19 0
#define DEBUG20 0
#define DEBUG21 0
#define DEBUG22 0
#define DEBUG23 0
#define DEBUG24 0
#define DEBUG25 0

#define DEBUG26_MEET_ID1 "LS_103300"
#define DEBUG26_MEET_ID2 "LS_103300_student_1_min_requirement"
#define DEBUG26(m) (false && (strcmp(KheMeetId(m), DEBUG26_MEET_ID1) == 0  \
  || strcmp(KheMeetId(m), DEBUG26_MEET_ID2) == 0) )

#define DEBUG27_MEET_ID "LS_100_2_0"
/* #define DEBUG27(m) (true && strcmp(KheMeetId(m), DEBUG27_MEET_ID) == 0) */
#define DEBUG27(m, t) (false && KheMeetIsAssignedPreassigned(m, &t))

#define DEBUG28 0

#define CHECK 1
#define CHECK_MAGIC 229843

#define bool_show(x) ((x) ? "true" : "false")


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET - a meet                                                        */
/*                                                                           */
/*****************************************************************************/

struct khe_meet_rec {
  void				*back;			/* back pointer      */
  char				*id;			/* Id                */
  int				visit_num;		/* visit number      */
  int				reference_count;	/* reference count   */
  KHE_SOLN			soln;			/* enclosing soln    */
  int				soln_index;		/* in soln list      */
  int				duration;		/* duration          */
  int				all_demand;		/* demand            */
#if CHECK
  int				magic;			/* checking meet     */
#endif
  ARRAY_KHE_MEET		assigned_meets;		/* assigned to here  */
  KHE_MEET			target_meet;		/* assigned to       */
  int				target_index;		/* index in target   */
  int				target_offset;		/* offset in target  */
  int				assigned_time_index;	/* if time assigned  */
  bool				target_fixed;		/* target fixed      */
  ARRAY_KHE_MEET_BOUND		meet_bounds;		/* meet bounds       */
  KHE_TIME_GROUP		time_domain;		/* time domain       */
  ARRAY_KHE_TASK		tasks;			/* tasks             */
  KHE_NODE			node;			/* optional node     */
  int				node_index;		/* index in node     */
  ARRAY_KHE_ZONE		zones;			/* zones             */
  ARRAY_KHE_MATCHING_SUPPLY_CHUNK supply_chunks;	/* when matching     */
  ARRAY_KHE_MATCHING_DEMAND_CHUNK demand_chunks;	/* when matching     */
  KHE_EVENT_IN_SOLN		event_in_soln;		/* encl em           */
  KHE_MEET			copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "bug finder"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMeetCheck(KHE_MEET meet)                                         */
/*                                                                           */
/*  Check that meet really is a meet, possibly NULL.                         */
/*                                                                           */
/*****************************************************************************/

void KheMeetCheck(KHE_MEET meet)
{
#if CHECK
  HnAssert(meet == NULL || meet->magic == CHECK_MAGIC,
    "KheMeetCheck internal error");
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetTestTasks(KHE_MEET meet)                                     */
/*                                                                           */
/*  Test whether meet still has the right tasks.                             */
/*                                                                           */
/*****************************************************************************/

static void KheMeetTestTasks(KHE_MEET meet)
{
  KHE_TASK task;  int i;
  if( DEBUG20 )
  {
    HaArrayForEach(meet->tasks, task, i)
      if( KheTaskMeet(task) != meet )
      {
	fprintf(stderr, "[ KheMeetTestTasks(%p", (void *) meet);
	KheMeetDebug(meet, 2, -1, stderr);
	fprintf(stderr, ") failing:\n");
	HaArrayForEach(meet->tasks, task, i)
	{
	  fprintf(stderr, "  %p: ", (void *) task);
	  KheTaskDebug(task, 2, 0, stderr);
	}
	HnAbort("] KheMeetTestTasks failing\n");
      }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "back pointers"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelSetBack(KHE_MEET meet, void *back)                     */
/*                                                                           */
/*  Set the back pointer of meet to back, assuming all is well.              */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelSetBack(KHE_MEET meet, void *back)
{
  KheSolnOpMeetSetBack(meet->soln, meet, meet->back, back);
  meet->back = back;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelSetBackUndo(KHE_MEET meet, void *old_back,             */
/*    void *new_back)                                                        */
/*                                                                           */
/*  Undo KheMeetKernelSetBack.                                               */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelSetBackUndo(KHE_MEET meet, void *old_back, void *new_back)
{
  meet->back = old_back;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetBack(KHE_MEET meet, void *back)                           */
/*                                                                           */
/*  Set the back pointer of meet.                                            */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetBack(KHE_MEET meet, void *back)
{
  KheMeetKernelSetBack(meet, back);
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheMeetBack(KHE_MEET meet)                                         */
/*                                                                           */
/*  Return the back pointer of meet.                                         */
/*                                                                           */
/*****************************************************************************/

void *KheMeetBack(KHE_MEET meet)
{
  return meet->back;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "visit numbers"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetVisitNum(KHE_MEET meet, int num)                          */
/*                                                                           */
/*  Set the visit number of meet.                                            */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetVisitNum(KHE_MEET meet, int num)
{
  meet->visit_num = num;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetVisitNum(KHE_MEET meet)                                       */
/*                                                                           */
/*  Return the visit number of meet.                                         */
/*                                                                           */
/*****************************************************************************/

int KheMeetVisitNum(KHE_MEET meet)
{
  return meet->visit_num;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetVisited(KHE_MEET meet, int slack)                            */
/*                                                                           */
/*  Return true if meet has been visited recently.                           */
/*                                                                           */
/*****************************************************************************/

bool KheMeetVisited(KHE_MEET meet, int slack)
{
  return KheSolnGlobalVisitNum(meet->soln) - meet->visit_num <= slack;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetVisit(KHE_MEET meet)                                         */
/*                                                                           */
/*  Visit meet.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheMeetVisit(KHE_MEET meet)
{
  meet->visit_num = KheSolnGlobalVisitNum(meet->soln);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetUnVisit(KHE_MEET meet)                                       */
/*                                                                           */
/*  Unvisit meet.                                                            */
/*                                                                           */
/*****************************************************************************/

void KheMeetUnVisit(KHE_MEET meet)
{
  meet->visit_num = KheSolnGlobalVisitNum(meet->soln) - 1;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "other simple attributes"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheMeetSoln(KHE_MEET meet)                                      */
/*                                                                           */
/*  Return the enclosing solution of meet.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheMeetSoln(KHE_MEET meet)
{
  return meet->soln;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetSoln(KHE_MEET meet, KHE_SOLN soln)                        */
/*                                                                           */
/*  Set the soln attribute of meet.                                          */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetSoln(KHE_MEET meet, KHE_SOLN soln)
{
  meet->soln = soln;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetSolnIndex(KHE_MEET meet)                                      */
/*                                                                           */
/*  Return the soln_index attribute of meet.                                 */
/*                                                                           */
/*****************************************************************************/

int KheMeetSolnIndex(KHE_MEET meet)
{
  return meet->soln_index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetSolnIndex(KHE_MEET meet, int soln_index)                  */
/*                                                                           */
/*  Set the soln_index attribute of meet.                                    */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetSolnIndex(KHE_MEET meet, int soln_index)
{
  meet->soln_index = soln_index;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetDuration(KHE_MEET meet)                                       */
/*                                                                           */
/*  Return the duration of meet.                                             */
/*                                                                           */
/*****************************************************************************/

int KheMeetDuration(KHE_MEET meet)
{
  return meet->duration;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheMeetEvent(KHE_MEET meet)                                    */
/*                                                                           */
/*  Return the event that meet is derived from, or NULL if none.             */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KheMeetEvent(KHE_MEET meet)
{
  if( meet->event_in_soln == NULL )
    return NULL;
  else
    return KheEventInSolnEvent(meet->event_in_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetIsPreassigned(KHE_MEET meet, KHE_TIME *time)                 */
/*                                                                           */
/*  Return true and set *time if meet is derived from a preassigned event.   */
/*                                                                           */
/*****************************************************************************/

bool KheMeetIsPreassigned(KHE_MEET meet, KHE_TIME *time)
{
  KHE_EVENT e;  KHE_TIME res;
  e = KheMeetEvent(meet);
  if( e == NULL )
  {
    if( time != NULL )
      *time = NULL;
    return false;
  }
  res = KheEventPreassignedTime(e);
  if( res == NULL )
  {
    if( time != NULL )
      *time = NULL;
    return false;
  }
  if( time != NULL )
    *time = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetDoIsAssignedPreassigned(KHE_MEET meet, KHE_TIME *time)       */
/*                                                                           */
/*  Carry out the main part of KheMeetIsAssignedPreassigned.                 */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetDoIsAssignedPreassigned(KHE_MEET meet, KHE_TIME *time)
{
  KHE_MEET child_meet;  int i;

  /* check meet itself */
  if( KheMeetIsPreassigned(meet, time) )
    return true;

  /* check meets assigned to it */
  HaArrayForEach(meet->assigned_meets, child_meet, i)
    if( KheMeetIsAssignedPreassigned(child_meet, time) )
      return true;

  /* no luck */
  if( time != NULL )
    *time = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetIsAssignedPreassigned(KHE_MEET meet, KHE_TIME *time)         */
/*                                                                           */
/*  Return true if meet is, or is assigned directly or indirectly, a         */
/*  preassigned meet.                                                        */
/*                                                                           */
/*****************************************************************************/

bool KheMeetIsAssignedPreassigned(KHE_MEET meet, KHE_TIME *time)
{
  /* return early if domain is not a singleton (just to save time) */
  if( KheTimeGroupTimeCount(meet->time_domain) > 1 )
  {
    if( time != NULL )
      *time = NULL;
    return false;
  }

  /* main call, which searches the tree of meet for a preassigned meet */
  return KheMeetDoIsAssignedPreassigned(meet, time);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_IN_SOLN KheMeetEventInSoln(KHE_MEET meet)                      */
/*                                                                           */
/*  Return meet's event in soln.                                             */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_IN_SOLN KheMeetEventInSoln(KHE_MEET meet)
{
  return meet->event_in_soln;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetAssignedDuration(KHE_MEET meet)                               */
/*                                                                           */
/*  Return the assigned duration of meet:  its duration if assigned, and     */
/*  0 otherwise.                                                             */
/*                                                                           */
/*****************************************************************************/

int KheMeetAssignedDuration(KHE_MEET meet)
{
  return meet->target_meet != NULL ? meet->duration : 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAssignedDurationDebug(KHE_MEET meet, int verbosity,          */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of meet's assigned duration.                                 */
/*                                                                           */
/*****************************************************************************/

void KheMeetAssignedDurationDebug(KHE_MEET meet, int verbosity,
  int indent, FILE *fp)
{
  if( meet->target_meet != NULL )
    KheMeetDebug(meet, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetDemand(KHE_MEET meet)                                         */
/*                                                                           */
/*  Return the demand of meet.                                               */
/*                                                                           */
/*****************************************************************************/

int KheMeetDemand(KHE_MEET meet)
{
  return meet->all_demand;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "matchings"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMeetMatchingReset(KHE_MEET meet)                                 */
/*                                                                           */
/*  Reset the matching within meet (but not within its tasks).               */
/*                                                                           */
/*  This function makes no attempt to move gracefully from one state to      */
/*  another; rather, it starts again from scratch, querying the current      */
/*  solution to see what is required.  This is useful for initializing,      */
/*  and also when the matching type changes, since in those cases a reset    */
/*  is the sensible way forward.                                             */
/*                                                                           */
/*  This function will be called separately for every meet, and the similar  */
/*  function KheTaskMatchingReset will be called for every task, so there    */
/*  is no need to make recursive calls, or calls on tasks.                   */
/*                                                                           */
/*****************************************************************************/

void KheMeetMatchingReset(KHE_MEET meet)
{
  int i, resource_count;
  KHE_MATCHING_DEMAND_CHUNK dc; KHE_MATCHING_SUPPLY_CHUNK sc;
  KHE_MEET ancestor_meet;  int ancestor_offset;
  if( meet->time_domain != NULL )
  {
    resource_count = KheInstanceResourceCount(KheSolnInstance(meet->soln));
    switch( KheSolnMatchingType(meet->soln) )
    {
      case KHE_MATCHING_TYPE_EVAL_INITIAL:
      case KHE_MATCHING_TYPE_EVAL_RESOURCES:

	/* set the base and domain to reflect the current time domain */
	HaArrayForEach(meet->demand_chunks, dc, i)
	{
	  KheMatchingDemandChunkSetBase(dc, i * resource_count);
	  KheMatchingDemandChunkSetIncrement(dc, resource_count);
	  KheMatchingDemandChunkSetDomain(dc,
	    KheTimeGroupTimeSet(meet->time_domain),
	    KHE_MATCHING_DOMAIN_CHANGE_TO_OTHER);
	}
	break;

      case KHE_MATCHING_TYPE_EVAL_TIMES:
      case KHE_MATCHING_TYPE_SOLVE:

	/* set the base and domain to the corresponding supply set of meet */
	ancestor_meet = meet;
	ancestor_offset = 0;
	while( ancestor_meet->target_meet != NULL )
	{
	  ancestor_offset += ancestor_meet->target_offset;
	  ancestor_meet = ancestor_meet->target_meet;
	}
	HaArrayForEach(meet->demand_chunks, dc, i)
	{
	  HnAssert(ancestor_offset + i <
	    HaArrayCount(ancestor_meet->supply_chunks),
	    "KheMeetMatchingReset internal error");
	  sc = HaArray(ancestor_meet->supply_chunks, ancestor_offset + i);
	  KheMatchingDemandChunkSetBase(dc, KheMatchingSupplyChunkBase(sc));
	  KheMatchingDemandChunkSetIncrement(dc, resource_count);
	  KheMatchingDemandChunkSetDomain(dc,
	    KheSolnMatchingZeroTimeDomain(meet->soln),
	    KHE_MATCHING_DOMAIN_CHANGE_TO_OTHER);
	}
	break;

      default:

	HnAbort("KheMeetMatchingReset internal error");
	break;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_DEMAND_CHUNK KheMeetDemandChunk(KHE_MEET meet, int offset)  */
/*                                                                           */
/*  Return meet's demand chunk at this offset.                               */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_DEMAND_CHUNK KheMeetDemandChunk(KHE_MEET meet, int offset)
{
  return HaArray(meet->demand_chunks, offset);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetSupplyNodeOffset(KHE_MEET meet, KHE_MATCHING_SUPPLY_NODE sn)  */
/*                                                                           */
/*  Return the offset in meet that sn is for.                                */
/*                                                                           */
/*****************************************************************************/

int KheMeetSupplyNodeOffset(KHE_MEET meet, KHE_MATCHING_SUPPLY_NODE sn)
{
  int index, i;  KHE_MATCHING_SUPPLY_CHUNK sc;
  index = KheMatchingSupplyNodeIndex(sn);
  HaArrayForEachReverse(meet->supply_chunks, sc, i)
    if( KheMatchingSupplyChunkBase(sc) <= index )
    {
      HnAssert(index < KheMatchingSupplyChunkBase(sc) +
	KheMatchingSupplyChunkSupplyNodeCount(sc),
	"KheMeetSupplyNodeOffset internal error 1 (failed %d < %d + %d)",
	  index, KheMatchingSupplyChunkBase(sc),
	  KheMatchingSupplyChunkSupplyNodeCount(sc));
      return i;
    }
  HnAbort("KheMeetSupplyNodeOffset internal error 2");
  return 0; /* keep compiler happy */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetMatchingAttachAllOrdinaryDemandMonitors(KHE_MEET meet)       */
/*                                                                           */
/*  Ensure that all the ordinary demand monitors of meet are attached.       */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheMeetMatchingAttachAllOrdinaryDemandMonitors(KHE_MEET meet)
{
  KHE_TASK task;  int i;
  HaArrayForEach(meet->tasks, task, i)
    KheTaskMatchingAttachAllOrdinaryDemandMonitors(task);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetMatchingDetachAllOrdinaryDemandMonitors(KHE_MEET meet)       */
/*                                                                           */
/*  Ensure that all the ordinary demand monitors of meet are detached.       */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheMeetMatchingDetachAllOrdinaryDemandMonitors(KHE_MEET meet)
{
  KHE_TASK task;  int i;
  HaArrayForEach(meet->tasks, task, i)
    KheTaskMatchingDetachAllOrdinaryDemandMonitors(task);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetMatchingSetWeight(KHE_MEET meet, KHE_COST new_weight)        */
/*                                                                           */
/*  Change the weight of the ordinary demand monitors of meet.               */
/*                                                                           */
/*****************************************************************************/

void KheMeetMatchingSetWeight(KHE_MEET meet, KHE_COST new_weight)
{
  KHE_TASK task;  int i;
  HaArrayForEach(meet->tasks, task, i)
    KheTaskMatchingSetWeight(task, new_weight);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetMatchingBegin(KHE_MEET meet)                                 */
/*                                                                           */
/*  Begin the matching in meet.                                              */
/*                                                                           */
/*****************************************************************************/

void KheMeetMatchingBegin(KHE_MEET meet)
{
  int i, j;  KHE_MATCHING m;  /* ARRAY_SHORT domain; */  SSET *domain;
  KHE_MATCHING_SUPPLY_CHUNK sc;  KHE_INSTANCE ins;

  /* must be no chunks present when this is called */
  HnAssert(HaArrayCount(meet->supply_chunks) == 0,
    "KheMeetMatchingBegin internal error 1");
  HnAssert(HaArrayCount(meet->demand_chunks) == 0,
    "KheMeetMatchingBegin internal error 2");

  /* create and add the supply chunks */
  m = KheSolnMatching(meet->soln);
  for( i = 0;  i < meet->duration;  i++ )
  {
    sc = KheMatchingSupplyChunkMake(KheSolnMatching(meet->soln), (void *) meet);
    ins = KheSolnInstance(meet->soln);
    for( j = 0;  j < KheInstanceResourceCount(ins);  j++ )
      KheMatchingSupplyNodeMake(sc, (void *) KheInstanceResource(ins, j));
    HaArrayAddLast(meet->supply_chunks, sc);
  }

  /* create and add the demand chunks (at this point, with no demand nodes) */
  domain = KheSolnMatchingZeroTimeDomain(meet->soln);
  for( i = 0;  i < meet->duration;  i++ )
    HaArrayAddLast(meet->demand_chunks, KheMatchingDemandChunkMake(
      m, NULL, 0, 0, domain));
  KheMeetMatchingReset(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetMatchingEnd(KHE_MEET meet)                                   */
/*                                                                           */
/*  End the matching in meet.                                                */
/*                                                                           */
/*****************************************************************************/

void KheMeetMatchingEnd(KHE_MEET meet)
{
  /* the demand chunks should be empty by now */
  while( HaArrayCount(meet->demand_chunks) > 0 )
    KheMatchingDemandChunkDelete(HaArrayLastAndDelete(meet->demand_chunks));

  /* delete the supply chunks (includes deleting their supply nodes) */
  while( HaArrayCount(meet->supply_chunks) > 0 )
    KheMatchingSupplyChunkDelete(HaArrayLastAndDelete(meet->supply_chunks));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "creation and deletion"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheMeetDoMake(HA_ARENA a)                                       */
/*                                                                           */
/*  Obtain a new meet from a; initialize its arrays.                         */
/*                                                                           */
/*****************************************************************************/

static KHE_MEET KheMeetDoMake(HA_ARENA a)
{
  KHE_MEET res;
  HaMake(res, a);
  HaArrayInit(res->assigned_meets, a);
  HaArrayInit(res->meet_bounds, a);
  HaArrayInit(res->tasks, a);
  HaArrayInit(res->zones, a);
  HaArrayInit(res->supply_chunks, a);
  HaArrayInit(res->demand_chunks, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetUnMake(KHE_MEET meet)                                        */
/*                                                                           */
/*  Undo KheMeetDoMake, returning meet's memory to the memory allocator.     */
/*                                                                           */
/*****************************************************************************/

/* *** arena delete will do this now
void KheMeetUnMake(KHE_MEET meet)
{
  MArrayFree(meet->assigned_meets);
  MArrayFree(meet->meet_bounds);
  MArrayFree(meet->tasks);
  MArrayFree(meet->zones);
  MArrayFree(meet->supply_chunks);
  MArrayFree(meet->demand_chunks);
  MFree(meet);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheMeetDoGet(KHE_SOLN soln)                                     */
/*                                                                           */
/*  Get a meet object, either from soln's free list or allocated.            */
/*                                                                           */
/*****************************************************************************/

static KHE_MEET KheMeetDoGet(KHE_SOLN soln)
{
  KHE_MEET res;
  res = KheSolnGetMeetFromFreeList(soln);
  if( res == NULL )
    res = KheMeetDoMake(KheSolnArena(soln));
  res->reference_count = 0;
  if( DEBUG19 )
    fprintf(stderr, "KheMeetDoGet %p\n", (void *) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetUnGet(KHE_MEET meet)                                         */
/*                                                                           */
/*  Undo KheMeetDoGet, adding meet to its soln's free list.                  */
/*                                                                           */
/*****************************************************************************/

static void KheMeetUnGet(KHE_MEET meet)
{
  if( DEBUG19 )
    fprintf(stderr, "KheMeetUnGet %p\n", (void *) meet);
  KheSolnAddMeetToFreeList(meet->soln, meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetReferenceCountIncrement(KHE_MEET meet)                       */
/*                                                                           */
/*  Increment meet's reference count.                                        */
/*                                                                           */
/*****************************************************************************/

void KheMeetReferenceCountIncrement(KHE_MEET meet)
{
  meet->reference_count++;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetReferenceCountDecrement(KHE_MEET meet)                       */
/*                                                                           */
/*  Decrement meet's reference count, and possibly add it to the free list.  */
/*                                                                           */
/*****************************************************************************/

void KheMeetReferenceCountDecrement(KHE_MEET meet)
{
  HnAssert(meet->reference_count >= 1,
    "KheMeetReferenceCountDecrement internal error");
  if( --meet->reference_count == 0 )
    KheMeetUnGet(meet);
}


/*****************************************************************************/
/*                                                                           */
/* void KheMeetDoAdd(KHE_MEET meet, KHE_SOLN soln, int duration, KHE_EVENT e)*/
/*                                                                           */
/*  Initialize meet and add it to soln.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheMeetDoAdd(KHE_MEET meet, KHE_SOLN soln, int duration,
  KHE_EVENT e)
{
  meet->back = NULL;
  meet->id = NULL;
  meet->visit_num = 0;
  KheMeetReferenceCountIncrement(meet);
  meet->duration = duration;
  meet->all_demand = 0;
#if CHECK
  meet->magic = CHECK_MAGIC;
#endif
  KheSolnAddMeet(soln, meet);  /* will set soln and soln_index */
  HaArrayClear(meet->assigned_meets);
  meet->target_meet = NULL;
  meet->target_index = NO_TIME_INDEX;
  meet->target_offset = -1;
  meet->assigned_time_index = NO_TIME_INDEX;
  meet->target_fixed = false;
  HaArrayClear(meet->meet_bounds);
  meet->time_domain = KheInstanceFullTimeGroup(KheSolnInstance(soln));
  HaArrayClear(meet->tasks);
  meet->node = NULL;
  meet->node_index = -1;
  HaArrayClear(meet->zones);
  /* HaArrayFill(meet->zones, meet->duration, NULL); omitting NULL suffixes */
  HaArrayClear(meet->supply_chunks);
  HaArrayClear(meet->demand_chunks);
  if( KheSolnMatching(soln) != NULL )
    KheMeetMatchingBegin(meet);
  if( e != NULL )
  {
    meet->event_in_soln = KheSolnEventInSoln(soln, e);
    KheEventInSolnAddMeet(meet->event_in_soln, meet);
  }
  else
    meet->event_in_soln = NULL;
  meet->copy = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetUnAdd(KHE_MEET meet)                                         */
/*                                                                           */
/*  Undo KheMeetDoAdd, leaving meet unlinked from the solution.              */
/*                                                                           */
/*****************************************************************************/

static void KheMeetUnAdd(KHE_MEET meet)
{
  /* delete supply and demand nodes */
  if( KheSolnMatching(meet->soln) != NULL )
    KheMeetMatchingEnd(meet);

  /* inform meet's event in soln */
  if( meet->event_in_soln != NULL )
    KheEventInSolnDeleteMeet(meet->event_in_soln, meet);

  /* delete from soln */
  KheSolnDeleteMeet(meet->soln, meet);

  /* meet is now not referenced from solution (this call may free meet) */
#if CHECK
  meet->magic = 0;
#endif
  KheMeetReferenceCountDecrement(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelAdd(KHE_MEET meet, KHE_SOLN soln, int duration,        */
/*    KHE_EVENT e)                                                           */
/*                                                                           */
/*  Kernel operation which adds meet to soln (but does not make it).         */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelAdd(KHE_MEET meet, KHE_SOLN soln, int duration, KHE_EVENT e)
{
  KheMeetDoAdd(meet, soln, duration, e);
  KheSolnOpMeetAdd(soln, meet, duration, e);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelAddUndo(KHE_MEET meet)                                 */
/*                                                                           */
/*  Undo KheMeetKernelAdd.                                                   */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelAddUndo(KHE_MEET meet)
{
  KheMeetUnAdd(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelDelete(KHE_MEET meet)                                  */
/*                                                                           */
/*  Kernel operation which deletes meet (but does not free it).              */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelDelete(KHE_MEET meet)
{
  KheSolnOpMeetDelete(meet->soln, meet, KheMeetDuration(meet),
    KheMeetEvent(meet));
  KheMeetUnAdd(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelDeleteUndo(KHE_MEET meet, KHE_SOLN soln, int duration, */
/*    KHE_EVENT e)                                                           */
/*                                                                           */
/*  Undo KheMeetKernelDelete.                                                */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelDeleteUndo(KHE_MEET meet, KHE_SOLN soln, int duration,
  KHE_EVENT e)
{
  KheMeetDoAdd(meet, soln, duration, e);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheMeetMake(KHE_SOLN soln, int duration, KHE_EVENT e)           */
/*                                                                           */
/*  Make a meet and add it to soln.  No tasks are added.                     */
/*                                                                           */
/*****************************************************************************/

KHE_MEET KheMeetMake(KHE_SOLN soln, int duration, KHE_EVENT e)
{
  KHE_MEET res;  /* KHE_TIME_GROUP tg; */  KHE_MEET_BOUND mb;

  /* ensure parameters are legal */
  /* ***
  if( DEBUG4 )
    fprintf(stderr, "[ KheMeetMake(soln, %d, \"%s\")\n",
      duration, e == NULL || KheEventId(e) == NULL ? "-" : KheEventId(e));
  *** */
  HnAssert(duration > 0, "KheMeetMake: invalid duration (%d)", duration);
  HnAssert(soln != NULL, "KheMeetMake: soln parameter is NULL");

  /* make and initialize a new meet object from scratch */
  res = KheMeetDoGet(soln);

  /* add it to the soln */
  KheMeetKernelAdd(res, soln, duration, e);

  /* if its event is preassigned, add a singleton meet bound */
  if( e != NULL && KheEventPreassignedTime(e) != NULL )
  {
    mb = KheMeetBoundMake(soln, false,
      KheTimeSingletonTimeGroup(KheEventPreassignedTime(e)));
    KheMeetAddMeetBound(res, mb);
  }

  /* return it */
  if( DEBUG4 )
    fprintf(stderr, "KheMeetMake(soln, %d, \"%s\") = %p\n", duration,
      e == NULL || KheEventId(e) == NULL ? "-" : KheEventId(e), (void *) res);
  KheMeetTestTasks(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDelete(KHE_MEET meet)                                        */
/*                                                                           */
/*  Delete meet from soln.                                                   */
/*                                                                           */
/*****************************************************************************/

void KheMeetDelete(KHE_MEET meet)
{
  KHE_MEET m2;

  /* clear the back pointer of meet (since it will be recreated NULL) */
  if( DEBUG4 )
    fprintf(stderr, "KheMeetDelete(%p)\n", (void *) meet);
  KheMeetTestTasks(meet);
  if( meet->back != NULL )
    KheMeetSetBack(meet, NULL);

  /* delete the tasks of meet, last to first (recreated first to last) */
  while( HaArrayCount(meet->tasks) > 0 )
    KheTaskDelete(HaArrayLast(meet->tasks));

  /* unassign meet, if assigned */
  KheMeetAssignUnFix(meet);
  if( meet->target_meet != NULL )
    KheMeetUnAssign(meet);

  /* unassign everything assigned to meet */
  while( HaArrayCount(meet->assigned_meets) > 0 )
  {
    m2 = HaArrayLast(meet->assigned_meets);
    KheMeetAssignUnFix(m2);
    KheMeetUnAssign(m2);
  }

  /* delete the meet bounds of meet */
  while( HaArrayCount(meet->meet_bounds) > 0 )
    KheMeetDeleteMeetBound(meet, HaArrayFirst(meet->meet_bounds));

  /* remove meet from its node, if any (will also clear zones) */
  if( meet->node != NULL )
    KheNodeDeleteMeet(meet->node, meet);

  /* carry out the kernel delete operation (may free meet) */
  KheMeetKernelDelete(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "copy"                                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheMeetCopyPhase1(KHE_MEET meet, HA_ARENA a)                    */
/*                                                                           */
/*  Carry out Phase 1 of copying meet.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_MEET KheMeetCopyPhase1(KHE_MEET meet, HA_ARENA a)
{
  KHE_MEET copy, ameet;  KHE_TASK task;  int i;  KHE_ZONE zone;
  KHE_MATCHING_SUPPLY_CHUNK sc;  KHE_MATCHING_DEMAND_CHUNK dc;
  KHE_MEET_BOUND mb;
  if( meet->copy == NULL )
  {
    HaMake(copy, a);
    meet->copy = copy;
    copy->back = meet->back;
    copy->id = NULL;  /* will be regenerated later if required */
    copy->visit_num = meet->visit_num;
    copy->reference_count = 1;  /* no paths, and meet is linked in */
    copy->soln = KheSolnCopyPhase1(meet->soln);
    copy->soln_index = meet->soln_index;
    copy->duration = meet->duration;
    copy->all_demand = meet->all_demand;
    HaArrayInit(copy->assigned_meets, a);
    HaArrayForEach(meet->assigned_meets, ameet, i)
      HaArrayAddLast(copy->assigned_meets, KheMeetCopyPhase1(ameet, a));
    copy->target_meet = (meet->target_meet == NULL ? NULL :
      KheMeetCopyPhase1(meet->target_meet, a));
    copy->target_index = meet->target_index;
    copy->target_offset = meet->target_offset;
    copy->assigned_time_index = meet->assigned_time_index;
    copy->target_fixed = meet->target_fixed;
    HaArrayInit(copy->meet_bounds, a);
    HaArrayForEach(meet->meet_bounds, mb, i)
      HaArrayAddLast(copy->meet_bounds, KheMeetBoundCopyPhase1(mb, a));
    copy->time_domain = (meet->time_domain == NULL ? NULL :
      KheTimeGroupCopy(meet->time_domain, a));
    HaArrayInit(copy->tasks, a);
    HaArrayForEach(meet->tasks, task, i)
      HaArrayAddLast(copy->tasks, KheTaskCopyPhase1(task, a));
    copy->node = (meet->node == NULL ? NULL : KheNodeCopyPhase1(meet->node, a));
    copy->node_index = meet->node_index;
    HaArrayInit(copy->zones, a);
    HaArrayForEach(meet->zones, zone, i)
      HaArrayAddLast(copy->zones,
	zone == NULL ? NULL : KheZoneCopyPhase1(zone, a));
    HaArrayInit(copy->supply_chunks, a);
    HaArrayForEach(meet->supply_chunks, sc, i)
      HaArrayAddLast(copy->supply_chunks,
	KheMatchingSupplyChunkCopyPhase1(sc, a));
    HaArrayInit(copy->demand_chunks, a);
    HaArrayForEach(meet->demand_chunks, dc, i)
      HaArrayAddLast(copy->demand_chunks,
	KheMatchingDemandChunkCopyPhase1(dc, a));
    copy->event_in_soln = (meet->event_in_soln == NULL ? NULL :
      KheEventInSolnCopyPhase1(meet->event_in_soln, a));
    copy->copy = NULL;
  }
  return meet->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetCopyPhase2(KHE_MEET meet)                                    */
/*                                                                           */
/*  Carry out Phase 2 of copying meet.                                       */
/*                                                                           */
/*****************************************************************************/

void KheMeetCopyPhase2(KHE_MEET meet)
{
  int i;  KHE_TASK task;  KHE_ZONE zone;  KHE_MEET_BOUND mb;
  KHE_MATCHING_SUPPLY_CHUNK sc;  KHE_MATCHING_DEMAND_CHUNK dc;
  if( meet->copy != NULL )
  {
    meet->copy = NULL;
    HaArrayForEach(meet->meet_bounds, mb, i)
      KheMeetBoundCopyPhase2(mb);
    HaArrayForEach(meet->tasks, task, i)
      KheTaskCopyPhase2(task);
    HaArrayForEach(meet->zones, zone, i)
      if( zone != NULL )
	KheZoneCopyPhase2(zone);
    HaArrayForEach(meet->supply_chunks, sc, i)
      KheMatchingSupplyChunkCopyPhase2(sc);
    HaArrayForEach(meet->demand_chunks, dc, i)
      KheMatchingDemandChunkCopyPhase2(dc);
    /* *** omit these, since directly reachable from the soln object itself
    if( meet->event_in_soln != NULL )
      KheEventInSolnCopyPhase2(meet->event_in_soln);
    if( meet->target_meet != NULL )
      KheMeetCopyPhase2(meet->target_meet);
    HaArrayForEach(meet->assigned_meets, ameet, i)
      KheMeetCopyPhase2(ameet);
    *** */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "domain calculations"                                          */
/*                                                                           */
/*  This private submodule collects together all the helper functions for    */
/*  calculating domains required at various places throughout this file.     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheMeetAddBoundDomain(KHE_MEET meet, KHE_MEET_BOUND mb)   */
/*                                                                           */
/*  Return the domain of meet after adding mb, ignoring the NULL case.       */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP KheMeetAddBoundDomain(KHE_MEET meet, KHE_MEET_BOUND mb)
{
  KHE_TIME_GROUP res;  KHE_TIME_GROUP tg;
  if( HaArrayCount(meet->meet_bounds) == 0 )
    res = KheMeetBoundTimeGroup(mb, KheMeetDuration(meet));
  else if( HaArrayCount(meet->meet_bounds) == 1 &&
	   HaArrayFirst(meet->meet_bounds) == mb )
    res = KheMeetBoundTimeGroup(mb, KheMeetDuration(meet));
  else
  {
    /* NB tg must be calculated separately, before starting on res */
    tg = KheMeetBoundTimeGroup(mb, KheMeetDuration(meet));
    KheSolnTimeGroupBegin(meet->soln);
    KheSolnTimeGroupUnion(meet->soln, meet->time_domain);
    KheSolnTimeGroupIntersect(meet->soln, tg);
    res = KheSolnTimeGroupEnd(meet->soln);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheMeetRevisedDomain(KHE_MEET meet, int durn,             */
/*    KHE_MEET_BOUND del_mb)                                                 */
/*                                                                           */
/*  Ignoring the NULL domain case, return the value that meet's domain       */
/*  would have if meet's duration was changed to durn, and, if del_mb        */
/*  is non-NULL, if del_mb was deleted.                                      */
/*                                                                           */
/*  This code was altered by JeffK on 30/03/2024 to ensure that if del_mb    */
/*  appears in meet's list of meet bounds more than once, only its first     */
/*  occurrence is ignored.                                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP KheMeetRevisedDomain(KHE_MEET meet, int durn,
  KHE_MEET_BOUND del_mb)
{
  enum { KHE_STATE_NONE, KHE_STATE_ONE, KHE_STATE_MANY } state;
  KHE_MEET_BOUND mb;  int i;  KHE_TIME_GROUP res;  KHE_SOLN soln;
  KHE_TIME_GROUP tg;
  soln = meet->soln;
  state = KHE_STATE_NONE;
  res = NULL;  /* keep compiler happy */
  HaArrayForEach(meet->meet_bounds, mb, i)
  {
    if( mb == del_mb )
    {
      /* ignore del_mb, but only ignore it once */
      del_mb = NULL;
    }
    else
    {
      switch( state )
      {
	case KHE_STATE_NONE:

	  res = KheMeetBoundTimeGroup(mb, durn);
	  state = KHE_STATE_ONE;
	  break;

	case KHE_STATE_ONE:

	  /* NB tg must be calculated separately, before starting on res */
	  tg = KheMeetBoundTimeGroup(mb, durn);
	  KheSolnTimeGroupBegin(soln);
	  KheSolnTimeGroupUnion(soln, res);
	  KheSolnTimeGroupIntersect(soln, tg);
	  state = KHE_STATE_MANY;
	  break;

	case KHE_STATE_MANY:

	  KheSolnTimeGroupIntersect(soln, KheMeetBoundTimeGroup(mb, durn));
	  break;
      }
    }
  }
  if( state == KHE_STATE_NONE )
    res = KheInstanceFullTimeGroup(KheSolnInstance(soln));
  else if( state == KHE_STATE_MANY )
    res = KheSolnTimeGroupEnd(soln);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheMeetBoundDomain(KHE_MEET meet)                         */
/*                                                                           */
/*  Return the domain of meet, ignoring the NULL case.                       */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP KheMeetBoundDomain(KHE_MEET meet)
{
  return KheMeetRevisedDomain(meet, KheMeetDuration(meet), NULL);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheMeetDeleteBoundDomain(KHE_MEET meet, KHE_MEET_BOUND mb)*/
/*                                                                           */
/*  Return the domain of meet after deleting mb, ignoring the NULL case.     */
/*  This works whether or not mb is already deleted from meet->meet_bounds.  */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP KheMeetDeleteBoundDomain(KHE_MEET meet, KHE_MEET_BOUND mb)
{
  return KheMeetRevisedDomain(meet, KheMeetDuration(meet), mb);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheMeetChangeDurationDomain(KHE_MEET meet, int duration)  */
/*                                                                           */
/*  Return the domain of meet after changing its duration to the value       */
/*  given, ignoring the NULL case.                                           */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP KheMeetChangeDurationDomain(KHE_MEET meet, int duration)
{
  return KheMeetRevisedDomain(meet, duration, NULL);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetHasAncestorWithDomain(KHE_MEET meet, KHE_MEET *anc,int *offs)*/
/*                                                                           */
/*  If meet has an ancestor (possibly meet itself) with a non-NULL time      */
/*  domain, set *anc to that meet and *offs to the offset of meet in *anc,   */
/*  and return true.  Otherwise set *anc to NULL, *offs to -1, and return    */
/*  false.  Parameter meet may be NULL, and then the result is false.        */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetHasAncestorWithDomain(KHE_MEET meet, KHE_MEET *anc,int *offs)
{
  int offset;
  if( meet != NULL )
  {
    offset = 0;
    while( meet->time_domain == NULL && meet->target_meet != NULL )
    {
      offset += meet->target_offset;
      meet = meet->target_meet;
    }
    if( meet->time_domain != NULL )
      return *anc = meet, *offs = offset, true;
  }
  return *anc = NULL, *offs = -1, false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetDomainAllowsAssignment(KHE_MEET meet,                        */
/*    KHE_TIME_GROUP target_domain, int offset)                              */
/*                                                                           */
/*  Return true if the domain of meet allows its assignment to a meet        */
/*  with domain target_domain at offset.                                     */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetDomainAllowsAssignment(KHE_MEET meet,
  KHE_TIME_GROUP target_domain, int offset)
{
  KHE_MEET child_meet;  int i;
  if( meet->time_domain != NULL )
    return KheTimeGroupDomainsAllowAssignment(meet->time_domain,
      target_domain, offset);
  else
  {
    /* automatic domain, have to consult meet's children */
    HaArrayForEach(meet->assigned_meets, child_meet, i)
      if( !KheMeetDomainAllowsAssignment(child_meet, target_domain,
	    offset + child_meet->target_offset) )
	return false;
    return true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAndDomainDebug(KHE_MEET meet, FILE *fp)                      */
/*                                                                           */
/*  Debug print of meet and its domain onto fp.                              */
/*                                                                           */
/*****************************************************************************/

static void KheMeetAndDomainDebug(KHE_MEET meet, FILE *fp)
{
  KHE_TIME_GROUP tg;
  KheMeetDebug(meet, 1, -1, fp);
  tg = KheMeetDomain(meet);
  fprintf(fp, " : ");
  if( tg == NULL )
    fprintf(fp, "NULL");
  else
    KheTimeGroupDebug(tg, 2, -1, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAsstChainDebug(KHE_MEET meet, int indent, FILE *fp)          */
/*                                                                           */
/*  Debug the chain of assignments leading out of meet.                      */
/*                                                                           */
/*****************************************************************************/

static void KheMeetAsstChainDebug(KHE_MEET meet, int indent, FILE *fp)
{
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  KheMeetAndDomainDebug(meet, fp);
  while( KheMeetAsst(meet) != NULL )
  {
    fprintf(fp, " --%d-> ", KheMeetAsstOffset(meet));
    meet = KheMeetAsst(meet);
    KheMeetAndDomainDebug(meet, fp);
  }
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDomainDoCheck(KHE_MEET meet, int depth)                      */
/*                                                                           */
/*  Carry out the work of KheMeetDomainCheck below.                          */
/*                                                                           */
/*****************************************************************************/

static void KheMeetDomainDoCheck(KHE_MEET meet, int depth)
{
  KHE_MEET child_meet, anc;  int offs, i;

  /* check meet against its nearest proper ancestor with a domain */
  if( KheMeetHasAncestorWithDomain(meet->target_meet, &anc, &offs) &&
      !KheMeetDomainAllowsAssignment(meet, anc->time_domain,
      meet->target_offset + offs) )
  {
    KheMeetAsstChainDebug(meet, 2, stderr);
    HnAbort("KheMeetDomainCheck failed (depth %d)\n", depth);
  }

  /* check meet's children, if not already done */
  if( meet->time_domain != NULL )
    HaArrayForEach(meet->assigned_meets, child_meet, i)
      KheMeetDomainDoCheck(child_meet, depth + 1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDomainCheck(KHE_MEET meet)                                   */
/*                                                                           */
/*  Check that there are no domain inconsistencies in the vicinity of meet.  */
/*                                                                           */
/*****************************************************************************/

static void KheMeetDomainCheck(KHE_MEET meet)
{
  if( DEBUG15 )
    KheMeetDomainDoCheck(meet, 0);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "split and merge"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMeetMoveChunks(KHE_MEET meet1, int start1, KHE_MEET meet2)       */
/*                                                                           */
/*  Move the supply and demand chunks of meet1 between start1 and the end    */
/*  to the end of meet2.                                                     */
/*                                                                           */
/*****************************************************************************/

static void KheMeetMoveChunks(KHE_MEET meet1, int start1, KHE_MEET meet2)
{
  KHE_MATCHING_SUPPLY_CHUNK sc;  int i;
  HnAssert(HaArrayCount(meet1->supply_chunks) == HaArrayCount(meet1->demand_chunks),
    "KheMeetMoveChunks internal error");
  for( i = start1;  i < HaArrayCount(meet1->supply_chunks);  i++ )
  {
    sc = HaArray(meet1->supply_chunks, i);
    KheMatchingSupplyChunkSetImpl(sc, (void *) meet2);
    HaArrayAddLast(meet2->supply_chunks, sc);
    HaArrayAddLast(meet2->demand_chunks, HaArray(meet1->demand_chunks, i));
  }
  HaArrayDeleteLastSlice(meet1->supply_chunks,
    HaArrayCount(meet1->supply_chunks) - start1);
  HaArrayDeleteLastSlice(meet1->demand_chunks,
    HaArrayCount(meet1->demand_chunks) - start1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetMoveZones(KHE_MEET meet1, int start1, KHE_MEET meet2,        */
/*    int start2)                                                            */
/*                                                                           */
/*  Move the zones of meet1 between start1 and the end to meet2 starting     */
/*  at start2, which will be at or after the end of where it is now.         */
/*                                                                           */
/*  These are logical moves and this code has to cope with the absence of    */
/*  NULL suffixes in either or both arrays of zones.                         */
/*                                                                           */
/*****************************************************************************/

static void KheMeetMoveZones(KHE_MEET meet1, int start1, KHE_MEET meet2,
  int start2)
{
  KHE_ZONE zone;  int i;
  if( start1 < HaArrayCount(meet1->zones) )
  {
    /* one or more non-NULL zones to move; fill meet2 to start2 and move them */
    HaArrayFill(meet2->zones, start2, NULL);
    for( i = start1;  i < HaArrayCount(meet1->zones);  i++ )
    {
      zone = HaArray(meet1->zones, i);
      if( zone != NULL )
	KheZoneUpdateMeetOffset(zone, meet1,i,meet2,HaArrayCount(meet2->zones));
      HaArrayAddLast(meet2->zones, zone);
    }
    HaArrayDeleteLastSlice(meet1->zones, HaArrayCount(meet1->zones) - start1);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDoSplit(KHE_MEET meet1, KHE_MEET meet2, int durn1)           */
/*                                                                           */
/*  Split meet1 into meet2, assuming all is in order.  This function does    */
/*  not do recursive splits; those are separate kernel operations that will  */
/*  precede this one.  Nor does it split meet1's tasks; those task splits    */
/*  are separate kernel operations that will follow this one.  Nor does it   */
/*  make meet bounds or auto domains for the meet2; those too are separate   */
/*  kernel operation which follow this one.                                  */
/*                                                                           */
/*****************************************************************************/
static void KheMeetDoSetDomain(KHE_MEET meet, KHE_TIME_GROUP tg);

static void KheMeetDoSplit(KHE_MEET meet1, KHE_MEET meet2, int durn1)
{
  KHE_MEET child_meet, tmp;  int i;  /* KHE_MEET_BOUND mb, mb2; */

  if( DEBUG18 )
  {
    fprintf(stderr, "[ KheMeetDoSplit(%p ", (void *) meet1);
    KheMeetDebug(meet1, 1, -1, stderr);
    fprintf(stderr, ", %p, %d)\n", (void *) meet2, durn1);
  }

  /* simple attributes and array clearing */
  meet2->id = NULL;
  meet2->back = meet1->back;
  meet2->visit_num = meet1->visit_num;
  meet2->event_in_soln = NULL;  /* helps when debugging */
  KheMeetReferenceCountIncrement(meet2);
#if CHECK
  meet2->magic = CHECK_MAGIC;
#endif
  KheSolnAddMeet(meet1->soln, meet2); /* will set soln and soln_index */
  if( DEBUG18 )
    fprintf(stderr, "  meet1 #%d#, meet2 #%d#\n",
      meet1->soln_index, meet2->soln_index);
  HaArrayClear(meet2->assigned_meets);
  HaArrayClear(meet2->meet_bounds);
  HaArrayClear(meet2->tasks);
  HaArrayClear(meet2->zones);
  HaArrayClear(meet2->supply_chunks);
  HaArrayClear(meet2->demand_chunks);

  /* duration, all_demand, and assigned_meets */
  /* this code recalculates all_demand from scratch (important!) */
  meet2->duration = meet1->duration - durn1;
  meet1->duration = durn1;
  meet1->all_demand = meet1->duration * HaArrayCount(meet1->tasks);
  meet2->all_demand = meet2->duration * HaArrayCount(meet1->tasks);
  HaArrayForEach(meet1->assigned_meets, child_meet, i)
  {
    if( child_meet->target_offset + child_meet->duration <= durn1 )
    {
      /* child_meet is targeted entirely at the first fragment of meet */
      meet1->all_demand += child_meet->all_demand;
    }
    else if( child_meet->target_offset >= durn1 )
    {
      /* child_meet is targeted entirely at the second fragment of meet */
      /* retarget child_meet to meet2 */
      child_meet->target_meet = meet2;
      child_meet->target_offset -= durn1;
      child_meet->target_index = HaArrayCount(meet2->assigned_meets);
      HaArrayAddLast(meet2->assigned_meets, child_meet);
      meet2->all_demand += child_meet->all_demand;
      tmp = HaArrayLastAndDelete(meet1->assigned_meets);
      if( tmp != child_meet )
      {
	/* fill the hole left behind by child_meet */
	tmp->target_index = i;
	HaArrayPut(meet1->assigned_meets, i, tmp);
      }
      i--;
    }
    else
    {
      /* child_meet spands the split point; this cannot occur, because of */
      /* earlier checks and recursive splits */
      HnAbort("KheMeetDoSplit internal error 2");
    }
  }

  /* assignment */
  if( meet1->target_meet != NULL )
  {
    meet2->target_meet = meet1->target_meet;
    meet2->target_index = HaArrayCount(meet2->target_meet->assigned_meets);
    meet2->target_offset = meet1->target_offset + durn1;
    HaArrayAddLast(meet2->target_meet->assigned_meets, meet2);
  }
  else
  {
    meet2->target_meet = NULL;
    meet2->target_index = NO_TIME_INDEX;
    meet2->target_offset = -1;
  }

  /* assigned time index */
  if( meet1->assigned_time_index != NO_TIME_INDEX )
    meet2->assigned_time_index = meet1->assigned_time_index + durn1;
  else
    meet2->assigned_time_index = NO_TIME_INDEX;
  if( DEBUG18 )
  {
    char *str;
    fprintf(stderr, "  KheMeetDoSplit (a) meet2 = ");
    str = KheMeetId(meet2);
    fprintf(stderr, "%s\n", str);
  }

  /* fixed flags */
  meet2->target_fixed = meet1->target_fixed;

  /* meet bounds, domains, and cycle meets */
  if( KheMeetIsCycleMeet(meet1) )
  {
    meet2->time_domain = KheTimeGroupNeighbour(meet1->time_domain, durn1);
    KheSolnCycleMeetSplit(meet1->soln, meet1, meet2);
  }
  else
  {
    if( meet1->time_domain != NULL )
      KheMeetDoSetDomain(meet1, KheMeetBoundDomain(meet1));
    meet2->time_domain = KheInstanceFullTimeGroup(KheSolnInstance(meet2->soln));
  }

  /* *** task splitting is a separate kernel operation now */

  /* node */
  meet2->node = meet1->node;
  if( meet2->node != NULL )
    KheNodeAddSplitMeet(meet2->node, meet2);

  /* zones */
  KheMeetMoveZones(meet1, durn1, meet2, 0);

  /* supply and demand chunks */
  if( HaArrayCount(meet1->supply_chunks) > 0 )
    KheMeetMoveChunks(meet1, durn1, meet2);

  /* event_in_soln */
  meet2->event_in_soln = meet1->event_in_soln;
  if( meet1->event_in_soln != NULL )
    KheEventInSolnSplitMeet(meet1->event_in_soln, meet1, meet2);
  meet2->copy = NULL;

  /* all done */
  KheMeetDomainCheck(meet1);
  KheMeetDomainCheck(meet2);
  if( DEBUG18 )
  {
    fprintf(stderr, "  returning (meet1 %s durn %d,",
      KheMeetId(meet1), KheMeetDuration(meet1));
    fprintf(stderr, ", meet2 %s durn %d)\n",
      KheMeetId(meet2), KheMeetDuration(meet2));
    fprintf(stderr, "] KheMeetDoSplit returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetTasksAreMergeable(KHE_MEET meet1, KHE_MEET meet2)            */
/*                                                                           */
/*  Return true if the tasks of meet1 and meet2 are mergeable, but without   */
/*  actually permuting the tasks of either meet.                             */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetTasksAreMergeable(KHE_MEET meet1, KHE_MEET meet2)
{
  int i, j;  KHE_TASK task1, task2, tmp;  ARRAY_KHE_TASK meet2_tasks;
  HA_ARENA a;
  if( HaArrayCount(meet1->tasks) != HaArrayCount(meet2->tasks) )
    return false;

  /* as an optimization, find tasks that are mergeable without permuting */
  for( i = 0;  i < HaArrayCount(meet1->tasks);  i++ )
  {
    task1 = HaArray(meet1->tasks, i);
    task2 = HaArray(meet2->tasks, i);
    if( !KheTaskMergeCheck(task1, task2) )
      break;
  }

  /* see whether any permutation of the the tasks from i onwards is mergeable */
  if( i < HaArrayCount(meet1->tasks) )
  {
    a = HaArrayArena(meet1->tasks);
    HaArrayInit(meet2_tasks, a);
    HaArrayAppend(meet2_tasks, meet2->tasks, j);
    for( ; i < HaArrayCount(meet1->tasks);  i++ )
    {
      task1 = HaArray(meet1->tasks, i);

      /* find the first available task of meet2 compatible with task1 */
      for( j = i;  j < HaArrayCount(meet2_tasks);  j++ )
	if( KheTaskMergeCheck(task1, HaArray(meet2_tasks, j)) )
	  break;

      /* if not found, fail */
      if( j == HaArrayCount(meet2_tasks) )
      {
	HaArrayFree(meet2_tasks);
	return false;
      }

      /* swap the compatible task into position i, unless it's there already */
      if( i != j )
	HaArraySwap(meet2_tasks, i, j, tmp);
    }
    HaArrayFree(meet2_tasks);
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDoMerge(KHE_MEET meet1, KHE_MEET meet2, int durn1)           */
/*                                                                           */
/*  Merge meet1 and meet2, assuming all is in order.  Do not merge tasks.    */
/*                                                                           */
/*****************************************************************************/

static void KheMeetDoMerge(KHE_MEET meet1, KHE_MEET meet2, int durn1)
{
  KHE_MEET tmp, child_meet;  int i;

  /* *** task merging is a separate kernel operation now */

  if( DEBUG18 )
  {
    KHE_TASK task;
    fprintf(stderr, "[ KheMeetDoMerge(%p ", (void *) meet1);
    KheMeetDebug(meet1, 1, -1, stderr);
    fprintf(stderr, ", %p ", (void *) meet2);
    KheMeetDebug(meet2, 1, -1, stderr);
    fprintf(stderr, ", %d)\n", durn1);
    HaArrayForEach(meet1->tasks, task, i)
    {
      fprintf(stderr, "  initial meet1 task %d: %p ", i, (void *) task);
      KheTaskDebug(task, 1, 0, stderr);
    }
    HaArrayForEach(meet2->tasks, task, i)
    {
      fprintf(stderr, "  initial meet2 task %d: %p ", i, (void *) task);
      KheTaskDebug(task, 1, 0, stderr);
    }
  }

  /* inform the event_in_soln object about the merge */
  if( meet2->event_in_soln != NULL )
    KheEventInSolnMergeMeet(meet1->event_in_soln, meet1, meet2);

  /* let soln know if merging cycle meets */
  if( KheMeetIsCycleMeet(meet1) )
    KheSolnCycleMeetMerge(meet1->soln, meet1, meet2);

  /* fix meet1's duration and domain */
  meet1->duration += meet2->duration;
  if( meet1->time_domain != NULL )
    KheMeetDoSetDomain(meet1, KheMeetBoundDomain(meet1));

  /* retarget everything assigned to meet2 to be assigned to meet1 */
  HaArrayForEach(meet2->assigned_meets, child_meet, i)
  {
    child_meet->target_meet = meet1;
    child_meet->target_offset += durn1;
    child_meet->target_index = HaArrayCount(meet1->assigned_meets);
    HaArrayAddLast(meet1->assigned_meets, child_meet);
  }

  /* demand */
  meet1->all_demand += meet2->all_demand;

  /* move meet2's zones to meet1 */
  KheMeetMoveZones(meet2, 0, meet1, meet1->duration - meet2->duration);

  /* move meet2's supply and demand chunks to meet1 */
  if( HaArrayCount(meet2->supply_chunks) > 0 )
    KheMeetMoveChunks(meet2, 0, meet1);

  /* remove meet2 from its node, if any */
  if( meet2->node != NULL )
    KheNodeDeleteSplitMeet(meet2->node, meet2);

  /* remove meet2 from its parent and fill any hole left behind */
  if( meet2->target_meet != NULL )
  {
    tmp = HaArrayLastAndDelete(meet2->target_meet->assigned_meets);
    if( meet2 != tmp )
    {
      tmp->target_index = meet2->target_index;
      HaArrayPut(meet2->target_meet->assigned_meets, meet2->target_index, tmp);
    }
  }

  /* remove meet2 from soln and optionally free it */
  KheSolnDeleteMeet(meet1->soln, meet2);
#if CHECK
  meet2->magic = 0;
#endif
  KheMeetReferenceCountDecrement(meet2);  /* may free meet2 */
  KheMeetDomainCheck(meet1);
  if( DEBUG18 )
  {
    KHE_TASK task;
    fprintf(stderr, "] KheMeetDoMerge returning\n");
    HaArrayForEach(meet1->tasks, task, i)
    {
      fprintf(stderr, "  final meet1 task %d: %p ", i, (void *) task);
      KheTaskDebug(task, 1, 0, stderr);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelSplit(KHE_MEET meet1, KHE_MEET meet2, int durn1)       */
/*                                                                           */
/*  The kernel operation for splitting, assuming all is in order.            */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelSplit(KHE_MEET meet1, KHE_MEET meet2, int durn1)
{
  KheMeetDoSplit(meet1, meet2, durn1);
  KheSolnOpMeetSplit(meet1->soln, meet1, meet2, durn1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelSplitUndo(KHE_MEET meet1, KHE_MEET2, int durn1)        */
/*                                                                           */
/*  Undo KheMeetKernelSplit.                                                 */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelSplitUndo(KHE_MEET meet1, KHE_MEET meet2, int durn1)
{
  KheMeetDoMerge(meet1, meet2, durn1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelMerge(KHE_MEET meet1, KHE_MEET meet2, int durn1)       */
/*                                                                           */
/*  The kernel operation for merging, assuming all is in order, including    */
/*  the meets being adjacent in time and in the right order.                 */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelMerge(KHE_MEET meet1, KHE_MEET meet2, int durn1)
{
  KheSolnOpMeetMerge(meet1->soln, meet1, meet2, durn1);
  KheMeetTestTasks(meet1);
  KheMeetDoMerge(meet1, meet2, durn1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelMergeUndo(KHE_MEET meet1, KHE_MEET meet2, int durn1)   */
/*                                                                           */
/*  Undo KheMeetKernelMerge.                                                 */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelMergeUndo(KHE_MEET meet1, KHE_MEET meet2, int durn1)
{
  KheMeetDoSplit(meet1, meet2, durn1);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetOpReturn(bool res, bool debug, char *op, char *message)      */
/*                                                                           */
/*  Return op, but only after an optional debug print of op and message.     */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetOpReturn(bool res, bool debug, char *op, char *message)
{
  if( debug )
    fprintf(stderr, "] %s returning %s (%s)\n", op,
      res ? "true" : "false", message);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetDoSplitCheck(KHE_MEET meet, bool recursive,                  */
/*    int durn1, KHE_TIME_GROUP anc_domain1, int offset1,                    */
/*    int durn2, KHE_TIME_GROUP anc_domain2, int offset2)                    */
/*                                                                           */
/*  Return true if meet can split into two fragments.  If recursive is       */
/*  true, meet's child meets that span the split point may be split too,     */
/*  otherwise such child meets cause the check to return false.              */
/*                                                                           */
/*  After the split, the first fragment of meet has duration durn1.  If      */
/*  that fragment has a proper ancestor with a non-NULL domain, then         */
/*  anc_domain1 is the domain of the nearest such ancestor, and offset1      */
/*  is the offset of that first fragment into that ancestor.  Otherwise      */
/*  anc_domain1 is NULL and offset1 is undefined.                            */
/*                                                                           */
/*  Similarly, after the split, the second fragment of meet has duration     */
/*  durn2.  If that fragment has a proper ancestor with a non-NULL domain,   */
/*  then anc_domain2 is the domain of the nearest such ancestor, and         */
/*  offset2 is the offset of that second fragment into that ancestor.        */
/*  Otherwise anc_domain2 is NULL and offset2 is undefined.                  */
/*                                                                           */
/*  Either anc_domain1 and anc_domain2 are both non-NULL, or both are NULL.  */
/*  Note that domains are not themselves affected by offsets; only the       */
/*  result of KheTimeGroupDomainsAllowAssignment is affected by them.        */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetDoSplitCheck(KHE_MEET meet, bool recursive,
  int durn1, KHE_TIME_GROUP anc_domain1, int offset1,
  int durn2, KHE_TIME_GROUP anc_domain2, int offset2)
{
  KHE_TIME_GROUP meet_domain1, meet_domain2;  KHE_MEET cm;
  int i, child_durn1, child_durn2;

  if( DEBUG26(meet) )
  {
    fprintf(stderr, "[ KheMeetDoSplitCheck(%s, %s, %d,%s,%d, %d,%s,%d)\n",
      KheMeetId(meet), bool_show(recursive),
      durn1, anc_domain1 != NULL ? "anc_domain1" : "NULL", offset1,
      durn2, anc_domain2 != NULL ? "anc_domain2" : "NULL", offset2);
    KheMeetDebug(meet, 2, 2, stderr);
  }

  if( meet->time_domain != NULL )
  {
    /* find the domains of the split fragments of meet */
    meet_domain1 = KheMeetChangeDurationDomain(meet, durn1);
    meet_domain2 = (durn2 == durn1 ? meet_domain1 :
      KheMeetChangeDurationDomain(meet, durn2));

    if( DEBUG26(meet) )
    {
      if( anc_domain1 != NULL )
      {
	fprintf(stderr, "  allows(meet_domain1, anc_domain1, %d):\n", offset1);
	fprintf(stderr, "    meet_domain1:\n");
	KheTimeGroupDebug(meet_domain1, 2, 6, stderr);
	fprintf(stderr, "    anc_domain1:\n");
	KheTimeGroupDebug(anc_domain1, 2, 6, stderr);
      }
      if( anc_domain2 != NULL )
      {
	fprintf(stderr, "  allows(meet_domain2, anc_domain2, %d):\n", offset2);
	fprintf(stderr, "    meet_domain2:\n");
	KheTimeGroupDebug(meet_domain2, 2, 6, stderr);
	fprintf(stderr, "    anc_domain2:\n");
	KheTimeGroupDebug(anc_domain2, 2, 6, stderr);
      }
    }

    /* check meet's fragments against anc_domain1 and anc_domain2 if non-NULL */
    if( anc_domain1 != NULL &&
        !KheTimeGroupDomainsAllowAssignment(meet_domain1,anc_domain1,offset1) )
      return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck", "m1");
    if( anc_domain2 != NULL &&
        !KheTimeGroupDomainsAllowAssignment(meet_domain2,anc_domain2,offset2) )
      return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck", "m2");

    /* check meet's descendants against meet_domain1 and meet_domain2 */
    HaArrayForEach(meet->assigned_meets, cm, i)
      if( cm->target_offset >= durn1 )
      {
	/* cm lies entirely in the second fragment and does not split */
        if( !KheMeetDomainAllowsAssignment(cm, meet_domain2,
	      cm->target_offset - durn1) )
	  return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	    "unsplit child a2");
      }
      else if( cm->target_offset + cm->duration <= durn1 )
      {
	/* cm lies entirely in the first fragment and does not split */
        if(!KheMeetDomainAllowsAssignment(cm, meet_domain1, cm->target_offset))
	  return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	    "unsplit child a1");
      }
      else if( recursive )
      {
	/* cm spans the split point and must itself be split */
	child_durn1 = durn1 - cm->target_offset;
	child_durn2 = cm->duration - child_durn1;
	if( !KheMeetDoSplitCheck(cm, recursive,
	    child_durn1, meet_domain1, cm->target_offset,
	    child_durn2, meet_domain2, 0) )
	  return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	    "split child a");
      }
      else
      {
	/* cm spans the split point but cannot be split (!recursive) */
	return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	  "spanning child a");
      }
  }
  else if( anc_domain1 != NULL || anc_domain2 != NULL )
  {
    /* check meet's descendants against anc_domain1 and anc_domain2 */
    HnAssert(anc_domain1 != NULL && anc_domain2 != NULL,
      "KheMeetDoSplitCheck internal error");
    HaArrayForEach(meet->assigned_meets, cm, i)
      if( cm->target_offset >= durn1 )
      {
	/* cm lies entirely in the second fragment and does not split */
        if( !KheMeetDomainAllowsAssignment(cm, anc_domain2,
	      cm->target_offset - durn1 + offset2) )
	  return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	    "unsplit child b2");
      }
      else if( cm->target_offset + cm->duration <= durn1 )
      {
	/* cm lies entirely in the first fragment and does not split */
        if( !KheMeetDomainAllowsAssignment(cm, anc_domain1,
	    cm->target_offset + offset1))
	  return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	    "unsplit child b1");
      }
      else if( recursive )
      {
	/* cm spans the split point and must itself be split */
	child_durn1 = durn1 - cm->target_offset;
	child_durn2 = cm->duration - child_durn1;
	if( !KheMeetDoSplitCheck(cm, recursive,
	    child_durn1, anc_domain1, cm->target_offset + offset1,
	    child_durn2, anc_domain2, offset2) )
	  return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	    "split child b");
      }
      else
      {
	/* cm spans the split point but cannot be split (!recursive) */
	return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	  "spanning child b");
      }
  }
  else
  {
    /* check meet's descendants against NULL domains */
    HaArrayForEach(meet->assigned_meets, cm, i)
      if( cm->target_offset >= durn1 )
      {
	/* cm lies entirely in the second fragment and does not split */
	/* no domain above, so nothing can fail here */
      }
      else if( cm->target_offset + cm->duration <= durn1 )
      {
	/* cm lies entirely in the first fragment and does not split */
	/* no domain above, so nothing can fail here */
      }
      else if( recursive )
      {
	/* cm spans the split point and must itself be split */
	child_durn1 = durn1 - cm->target_offset;
	child_durn2 = cm->duration - child_durn1;
	if( !KheMeetDoSplitCheck(cm, recursive, child_durn1, NULL, -1,
	    child_durn2, NULL, -1) )
	  return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	    "split child c");
      }
      else
      {
	/* cm spans the split point but cannot be split (!recursive) */
	return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	  "spanning child c");
      }
  }

  /* all OK */
  return KheMeetOpReturn(true, DEBUG26(meet), "KheMeetDoSplitCheck", "ok");
}


/* *** old version with dodgy offsets, replaced by JeffK 1/4/2024
static bool KheMeetDoSplitCheck(KHE_MEET meet, bool recursive,
  int durn1, KHE_TIME_GROUP anc_domain1, int offset1,
  int durn2, KHE_TIME_GROUP anc_domain2, int offset2)
{
  KHE_TIME_GROUP meet_domain1, meet_domain2;  KHE_MEET cm;
  int i, child_durn1, child_durn2;

  if( DEBUG26(meet) )
  {
    fprintf(stderr, "[ KheMeetDoSplitCheck(%s, %s, %d,%s,%d, %d,%s,%d)\n",
      KheMeetId(meet), bool_show(recursive),
      durn1, anc_domain1 != NULL ? "anc_domain1" : "NULL", offset1,
      durn2, anc_domain2 != NULL ? "anc_domain2" : "NULL", offset2);
    KheMeetDebug(meet, 2, 2, stderr);
  }

  if( meet->time_domain != NULL )
  {
    ** find the domains of the split fragments of meet **
    meet_domain1 = KheMeetChangeDurationDomain(meet, durn1);
    meet_domain2 = (durn2 == durn1 ? meet_domain1 :
      KheMeetChangeDurationDomain(meet, durn2));

    if( DEBUG26(meet) )
    {
      if( anc_domain1 != NULL )
      {
	fprintf(stderr, "  allows(meet_domain1, anc_domain1, %d):\n", offset1);
	fprintf(stderr, "    meet_domain1:\n");
	KheTimeGroupDebug(meet_domain1, 2, 6, stderr);
	fprintf(stderr, "    anc_domain1:\n");
	KheTimeGroupDebug(anc_domain1, 2, 6, stderr);
      }
      if( anc_domain2 != NULL )
      {
	fprintf(stderr, "  allows(meet_domain2, anc_domain2, %d):\n", offset2);
	fprintf(stderr, "    meet_domain2:\n");
	KheTimeGroupDebug(meet_domain2, 2, 6, stderr);
	fprintf(stderr, "    anc_domain2:\n");
	KheTimeGroupDebug(anc_domain2, 2, 6, stderr);
      }
    }

    ** check meet's fragments against anc_domain1 and anc_domain2 if non-NULL **
    if( anc_domain1 != NULL &&
        !KheTimeGroupDomainsAllowAssignment(meet_domain1,anc_domain1,offset1) )
      return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck", "m1");
    if( anc_domain2 != NULL &&
        !KheTimeGroupDomainsAllowAssignment(meet_domain2,anc_domain2,offset2) )
      return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck", "m2");

    ** check meet's descendants against meet_domain1 and meet_domain2 **
    HaArrayForEach(meet->assigned_meets, cm, i)
      if( cm->target_offset >= durn1 )
      {
	** cm lies entirely in the second fragment and does not split **
        if( !KheMeetDomainAllowsAssignment(cm, meet_domain2,
	      cm->target_offset - durn1) )
	  return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	    "unsplit child a2");
      }
      else if( cm->target_offset + cm->duration <= durn1 )
      {
	** cm lies entirely in the first fragment and does not split **
        if(!KheMeetDomainAllowsAssignment(cm, meet_domain1, cm->target_offset))
	  return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	    "unsplit child a1");
      }
      else if( recursive )
      {
	** cm spans the split point and must itself be split **
	child_durn1 = durn1 - cm->target_offset;
	child_durn2 = cm->duration - child_durn1;
	if( !KheMeetDoSplitCheck(cm, recursive,
	    child_durn1, meet_domain1, cm->target_offset,
	    child_durn2, meet_domain2, 0) )
	  return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	    "split child a");
      }
      else
      {
	** cm spans the split point but cannot be split (!recursive) **
	return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	  "spanning child a");
      }
  }
  else if( anc_domain1 != NULL || anc_domain2 != NULL )
  {
    ** check meet's descendants against anc_domain1 and anc_domain2 **
    HnAssert(anc_domain1 != NULL && anc_domain2 != NULL,
      "KheMeetDoSplitCheck internal error");
    HaArrayForEach(meet->assigned_meets, cm, i)
      if( cm->target_offset >= durn1 )
      {
	** cm lies entirely in the second fragment and does not split **
        if( !KheMeetDomainAllowsAssignment(cm, anc_domain2,
	      cm->target_offset - durn1 + offset2) )
	  return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	    "unsplit child b2");
      }
      else if( cm->target_offset + cm->duration <= durn1 )
      {
	** cm lies entirely in the first fragment and does not split **
        if( !KheMeetDomainAllowsAssignment(cm, anc_domain1,
	    cm->target_offset + offset1))
	  return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	    "unsplit child b1");
      }
      else if( recursive )
      {
	** cm spans the split point and must itself be split **
	child_durn1 = durn1 - cm->target_offset;
	child_durn2 = cm->duration - child_durn1;
	if( !KheMeetDoSplitCheck(cm, recursive,
	    child_durn1, anc_domain1, cm->target_offset + offset1,
	    child_durn2, anc_domain2, offset2) )
	  return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	    "split child b");
      }
      else
      {
	** cm spans the split point but cannot be split (!recursive) **
	return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	  "spanning child b");
      }
  }
  else
  {
    ** check meet's descendants against NULL domains **
    HaArrayForEach(meet->assigned_meets, cm, i)
      if( cm->target_offset >= durn1 )
      {
	** cm lies entirely in the second fragment and does not split **
	** no domain above, so nothing can fail here **
      }
      else if( cm->target_offset + cm->duration <= durn1 )
      {
	** cm lies entirely in the first fragment and does not split **
	** no domain above, so nothing can fail here **
      }
      else if( recursive )
      {
	** cm spans the split point and must itself be split **
	child_durn1 = durn1 - cm->target_offset;
	child_durn2 = cm->duration - child_durn1;
	if( !KheMeetDoSplitCheck(cm, recursive, child_durn1, NULL, -1,
	    child_durn2, NULL, -1) )
	  return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	    "split child c");
      }
      else
      {
	** cm spans the split point but cannot be split (!recursive) **
	return KheMeetOpReturn(false, DEBUG26(meet), "KheMeetDoSplitCheck",
	  "spanning child c");
      }
  }

  ** all OK **
  return KheMeetOpReturn(true, DEBUG26(meet), "KheMeetDoSplitCheck", "ok");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetSplitCheck(KHE_MEET meet, int durn1, bool recursive)         */
/*                                                                           */
/*  Check whether meet can be split at durn1, possibly recursively.          */
/*                                                                           */
/*****************************************************************************/

bool KheMeetSplitCheck(KHE_MEET meet, int durn1, bool recursive)
{
  int offs;  KHE_MEET anc;  bool res;
  if( DEBUG26(meet) )
    fprintf(stderr, "[ KheMeetSplitCheck(%s, %d|%d, %s)\n", KheMeetId(meet),
      durn1, KheMeetDuration(meet) - durn1, bool_show(recursive));
  HnAssert(durn1 > 0 && durn1 < meet->duration,
    "KheMeetSplitCheck: invalid durn1 value %d (meet duration is %d)",
    durn1, meet->duration);
  KheMeetTestTasks(meet);
  if( KheMeetHasAncestorWithDomain(meet->target_meet, &anc, &offs) )
    res = KheMeetDoSplitCheck(meet, recursive, durn1, anc->time_domain,
      meet->target_offset + offs, meet->duration - durn1,
      anc->time_domain, meet->target_offset + offs + durn1);
  else
    res = KheMeetDoSplitCheck(meet, recursive, durn1, NULL, -1,
      meet->duration - durn1, NULL, -1);
  if( DEBUG26(meet) )
    fprintf(stderr, "] KheMeetSplitCheck returning %s\n", bool_show(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSplitUnchecked(KHE_MEET meet, int durn1, bool recursive,     */
/*    KHE_MEET *meet1, KHE_MEET *meet2)                                      */
/*                                                                           */
/*  Split meet into *meet1 and *meet2 at durn1, assuming no problems.        */
/*                                                                           */
/*****************************************************************************/

void KheMeetSplitUnchecked(KHE_MEET meet, int durn1, bool recursive,
  KHE_MEET *meet1, KHE_MEET *meet2)
{
  KHE_MEET child_meet, res, junk1, junk2;  int i, j;  KHE_TASK task1, task2;
  KHE_MEET_BOUND mb;  KHE_TASK_BOUND tb;  HA_ARENA a;

  /* if recursive, carry out recursive splits as required */
  KheMeetTestTasks(meet);
  a = KheSolnArena(meet->soln);
  if( recursive )
    HaArrayForEach(meet->assigned_meets, child_meet, i)
      if( child_meet->target_offset < durn1 &&
	  child_meet->target_offset + child_meet->duration > durn1 )
	KheMeetSplitUnchecked(child_meet, durn1 - child_meet->target_offset,
	  recursive, &junk1, &junk2);

  /* initialize a new meet object and its arrays (just enough to go on with) */
  res = KheMeetDoMake(a);
  res->reference_count = 0;

  /* split meet into res (unchecked kernel operation) */
  KheMeetKernelSplit(meet, res, durn1);

  /* add meet's meet bounds to res (unchecked kernel operations) */
  HaArrayForEach(meet->meet_bounds, mb, i)
    KheMeetKernelAddMeetBound(res, mb);
    /* *** bug fix JeffK 2/4/24; a check here may fail owing to unsplit parent
    if( !KheMeetAddMeetBound(res, mb) )
    {
      if( DEBUG24 )
      {
	fprintf(stderr, "  [ KheMeetSplitUnchecked failing:\n");
	KheMeetDebug(meet, 2, 4, stderr);
	KheMeetDebug(res, 2, 4, stderr);
	KheMeetBoundDebug(mb, 2, 4, stderr);
	fprintf(stderr, "  ]\n");
      }
      HnAbort("KheMeetSplitUnchecked internal error 1");
    }
    *** */
  if( meet->time_domain == NULL )
    KheMeetSetAutoDomain(res, true);

  /* split meet's tasks and add task bounds to the new task */
  HaArrayForEach(meet->tasks, task1, i)
  {
    task2 = KheTaskDoGet(meet->soln);
    KheTaskKernelSplit(task1, task2, durn1, res);
    for( j = 0;  j < KheTaskTaskBoundCount(task1);  j++ )
    {
      tb = KheTaskTaskBound(task1, j);
      if( !KheTaskAddTaskBound(task2, tb) )
        HnAbort("KheMeetSplitUnchecked internal error 2");
    }
  }

  /* set *meet1 and *meet2 and return */
  *meet1 = meet;
  *meet2 = res;
  KheMeetTestTasks(*meet1);
  KheMeetTestTasks(*meet2);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetSplit(KHE_MEET meet, int durn1, bool recursive,              */
/*    KHE_MEET *meet1, KHE_MEET *meet2)                                      */
/*                                                                           */
/*  Split meet into *meet1 and *meet2 at durn1.                              */
/*                                                                           */
/*****************************************************************************/

bool KheMeetSplit(KHE_MEET meet, int durn1, bool recursive,
  KHE_MEET *meet1, KHE_MEET *meet2)
{
  /* ***
  if( DEBUG21 )
  {
    fprintf(stderr, "[ KheMeetSplit(");
    KheMeetDebug(meet, 2, -1, stderr);
    fprintf(stderr, ", %d, %s, -, -)\n", durn1, recursive ? "true" : "false");
  }
  *** */
  KheMeetTestTasks(meet);
  if( !KheMeetSplitCheck(meet, durn1, recursive) )
  {
    /* ***
    if( DEBUG21 )
      fprintf(stderr, "] KheMeetSplit returning false\n");
    *** */
    return false;
  }
  KheMeetSplitUnchecked(meet, durn1, recursive, meet1, meet2);
  KheMeetTestTasks(*meet1);
  KheMeetTestTasks(*meet2);
  if( DEBUG21 )
    fprintf(stderr, "KheMeetSplit(%p, %d, %s) = %p, %p\n", (void *) meet,
      durn1, recursive ? "true" : "false", (void *) *meet1, (void *) *meet2);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetMergeCheck(KHE_MEET meet1, KHE_MEET meet2)                   */
/*                                                                           */
/*  Check whether meet1 and meet2 can be merged.                             */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetMergeReturn(bool res, bool debug, KHE_MEET meet1,
  KHE_MEET meet2, char *mess)
{
  if( debug )
  {
    fprintf(stderr, "  KheMeetMerge(");
    KheMeetDebug(meet1, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheMeetDebug(meet2, 1, -1, stderr);
    fprintf(stderr, ") returning %s (%s)\n", res ? "true" : "false", mess);
  }
  return res;
}

bool KheMeetMergeCheck(KHE_MEET meet1, KHE_MEET meet2)
{
  KHE_MEET tmp, anc, child_meet;  KHE_TIME_GROUP new_tg;  int offs, i;

  /* meets must be distinct */
  KheMeetTestTasks(meet1);
  KheMeetTestTasks(meet2);
  if( meet1 == meet2 )
    return KheMeetMergeReturn(false, DEBUG11, meet1, meet2, "equal");

  /* meets must both be cycle meets, or both not be cycle meets */
  if( KheMeetIsCycleMeet(meet1) != KheMeetIsCycleMeet(meet2) )
    return KheMeetMergeReturn(false, DEBUG11, meet1, meet2, "cycle");

  /* meets must be derived from the same event, or both from none */
  if( meet1->event_in_soln != meet2->event_in_soln )
    return KheMeetMergeReturn(false, DEBUG11, meet1, meet2, "event");

  /* meets must both lie in the same node, or in no node */
  if( meet1->node != meet2->node )
    return KheMeetMergeReturn(false, DEBUG11, meet1, meet2, "node");

  /* assignments must be compatible */
  if( KheMeetIsCycleMeet(meet1) )
  {
    /* cycle meets must have compatible assigned time indexes */
    if( meet2->assigned_time_index + meet2->duration ==
	meet1->assigned_time_index )
    {
      /* OK, but swap */
      tmp = meet1, meet1 = meet2, meet2 = tmp;
    }
    else if( meet1->assigned_time_index + meet1->duration !=
	meet2->assigned_time_index )
      return KheMeetMergeReturn(false, DEBUG11, meet1, meet2, "cycle offset");
  }
  else
  {
    /* non-cycle meets must have compatible target meets and offsets */
    if( meet1->target_meet != meet2->target_meet )
      return KheMeetMergeReturn(false, DEBUG11, meet1, meet2, "target");
    if( meet1->target_meet != NULL )
    {
      if( meet2->target_offset + meet2->duration == meet1->target_offset )
      {
	/* OK, but swap */
	tmp = meet1, meet1 = meet2, meet2 = tmp;
      }
      else if( meet1->target_offset + meet1->duration != meet2->target_offset )
	return KheMeetMergeReturn(false, DEBUG11, meet1, meet2, "offset");
    }
  }

  /* tasks must be mergeable (this no longer permutes any tasks) */
  if( !KheMeetTasksAreMergeable(meet1, meet2) )
    return KheMeetMergeReturn(false, DEBUG11, meet1, meet2, "tasks");

  /* check domains (only needed if domain is non-NULL) */
  if( meet1->time_domain != NULL && (HaArrayCount(meet1->assigned_meets) > 0
      || HaArrayCount(meet2->assigned_meets) > 0 || meet1->target_meet != NULL) )
  {
    new_tg = KheMeetIsCycleMeet(meet1) ? meet1->time_domain :
      KheMeetChangeDurationDomain(meet1, meet1->duration + meet2->duration);

    /* meet1's children must be assignable in the new domain */
    HaArrayForEach(meet1->assigned_meets, child_meet, i)
      if( !KheMeetDomainAllowsAssignment(child_meet, new_tg,
	    child_meet->target_offset) )
	return KheMeetMergeReturn(false, DEBUG11, meet1, meet2,"child1 domain");

    /* meet2's children must be assignable in the new domain */
    HaArrayForEach(meet2->assigned_meets, child_meet, i)
      if( !KheMeetDomainAllowsAssignment(child_meet, new_tg,
	  meet1->duration + child_meet->target_offset) )
	return KheMeetMergeReturn(false, DEBUG11, meet1, meet2,"child2 domain");

    /* meet must be assignable to its first ancestor with a non-NULL domain */
    if( KheMeetHasAncestorWithDomain(meet1->target_meet, &anc, &offs)
	&& !KheTimeGroupDomainsAllowAssignment(new_tg, anc->time_domain,
	  meet1->target_offset + offs) )
      return KheMeetMergeReturn(false, DEBUG11, meet1, meet2, "anc domain");
  }

  /* no problems, so they are mergeable */
  KheMeetTestTasks(meet1);
  KheMeetTestTasks(meet2);
  return KheMeetMergeReturn(true, DEBUG11, meet1, meet2, "ok");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetMerge(KHE_MEET meet1, KHE_MEET meet2, bool recursive,        */
/*    KHE_MEET *meet)                                                        */
/*                                                                           */
/*  Merge meet1 and meet2 into *meet.                                        */
/*                                                                           */
/*****************************************************************************/

bool KheMeetMerge(KHE_MEET meet1, KHE_MEET meet2, bool recursive,
  KHE_MEET *meet)
{
  KHE_MEET tmp, child_meet1, child_meet2;  int durn1, i, j;
  KHE_TASK task1, task2;

  /* check mergable */
  KheMeetTestTasks(meet1);
  KheMeetTestTasks(meet2);
  if( !KheMeetMergeCheck(meet1, meet2) )
    return false;

  /* make sure cycle meets and assigned meets are in the right order */
  if( KheMeetIsCycleMeet(meet1) )
  {
    if( meet1->assigned_time_index > meet2->assigned_time_index )
      tmp = meet1, meet1 = meet2, meet2 = tmp;
  }
  else if( meet1->target_meet != NULL )
  {
    if( meet1->target_offset > meet2->target_offset )
      tmp = meet1, meet1 = meet2, meet2 = tmp;
  }

  /* carry out the kernel task merge operations */
  durn1 = KheMeetDuration(meet1);
  HaArrayForEach(meet1->tasks, task1, i)
  {
    /* find task2, the first available task of meet2 compatible with task1 */
    task2 = NULL;  /* keep compiler happy */
    for( j = 0;  j < HaArrayCount(meet2->tasks);  j++ )
    {
      task2 = HaArray(meet2->tasks, j);
      if( KheTaskMergeCheck(task1, task2) )
	break;
    }
    HnAssert(j < HaArrayCount(meet2->tasks), "KheMeetMerge internal error 1");

    /* delete task2's task bounds */
    while( KheTaskTaskBoundCount(task2) > 0 )
      KheTaskBoundDelete(KheTaskTaskBound(task2, 0));

    /* merge the tasks */
    KheTaskKernelMerge(task1, task2, durn1, meet2);
  }

  /* kernel meet bound deletion operations */
  while( HaArrayCount(meet2->meet_bounds) > 0 )
    KheMeetKernelDeleteMeetBound(meet2, HaArrayLast(meet2->meet_bounds));

  /* carry out the kernel meet merge operation */
  KheMeetKernelMerge(meet1, meet2, KheMeetDuration(meet1));

  /* carry out recursive merges, if required */
  if( recursive )
  {
    /* for each meet child_meet1 at the end of the former meet1 */
    RESTART_RECURSIVE:
    HaArrayForEach(meet1->assigned_meets, child_meet1, i)
      if( child_meet1->target_offset + child_meet1->duration == durn1 )
      {
	/* child_meet1 lies at the end of the former meet1 */
	HaArrayForEach(meet1->assigned_meets, child_meet2, j)
	  if( child_meet2->target_offset == durn1 )
	  {
	    /* child_meet2 lies at the start of the former meet2 */
	    if( KheMeetMerge(child_meet1, child_meet2, true, &child_meet1) )
	      goto RESTART_RECURSIVE;
	  }
      }
  }

  /* set *meet and and return success */
  *meet = meet1;
  KheMeetTestTasks(*meet);
  if( DEBUG21 )
    fprintf(stderr, "KheMeetMerge(%p, %p, %s) = %p\n", (void *) meet1,
      (void *) meet2, recursive ? "true" : "false", (void *) *meet);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "assignment (basic functions)"                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAsstOpDebug(char *op, KHE_MEET meet, KHE_MEET target_meet,   */
/*    int target_offset)                                                     */
/*                                                                           */
/*  Generate a debug print of op(meet, target_meet, target_offset).          */
/*                                                                           */
/*****************************************************************************/

static void KheMeetAsstOpDebug(char *op, KHE_MEET meet, KHE_MEET target_meet,
  int offset)
{
  fprintf(stderr, "%s(", op);
  KheMeetAndDomainDebug(meet, stderr);
  fprintf(stderr, ", ");
  if( target_meet != NULL )
  {
    KheMeetAsstChainDebug(target_meet, -1, stderr);
    fprintf(stderr, ", %d)\n", offset);
  }
  else
    fprintf(stderr, "NULL, -)\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDoAssignTime(KHE_MEET meet)                                  */
/*                                                                           */
/*  Inform meet and all its descendants and their constraints that meet's    */
/*  parent has been assigned a time.                                         */
/*                                                                           */
/*****************************************************************************/

static void KheMeetDoAssignTime(KHE_MEET meet)
{
  KHE_MEET child_meet;  int i;  KHE_TASK task;
  meet->assigned_time_index =
    meet->target_meet->assigned_time_index + meet->target_offset;
  if( DEBUG6 )
  {
    KHE_TIME t;
    t = KheInstanceTime(KheSolnInstance(meet->soln), meet->assigned_time_index);
    fprintf(stderr, "     [ KheMeetDoAssignTime(");
    KheMeetDebug(meet, 1, -1, stderr);
    fprintf(stderr, ", %s); domain ", KheTimeId(t)==NULL ? "-" : KheTimeId(t));
    KheTimeGroupDebug(meet->time_domain, 2, -1, stderr);
    fprintf(stderr, "\n");
  }
  if( meet->event_in_soln != NULL )
    KheEventInSolnAssignTime(meet->event_in_soln, meet,
      meet->assigned_time_index);
  HaArrayForEach(meet->tasks, task, i)
    KheTaskAssignTime(task, meet->assigned_time_index);
  HaArrayForEach(meet->assigned_meets, child_meet, i)
  {
    if( DEBUG6 )
      fprintf(stderr, "  KheMeetDoAssignTime(%s) calling "
	"KheMeetDoAssignTime(%s)\n", KheMeetId(meet),
	KheMeetId(child_meet));
    KheMeetDoAssignTime(child_meet);
  }
  if( DEBUG6 )
    fprintf(stderr, "     ] KheMeetDoAssignTime returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDoUnAssignTime(KHE_MEET meet)                                */
/*                                                                           */
/*  Inform meet and all its descendants and their constraints that meet's    */
/*  time assignment is being removed.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheMeetDoUnAssignTime(KHE_MEET meet)
{
  KHE_MEET child_meet;  int i;  KHE_TASK task;
  if( DEBUG16 )
  {
    fprintf(stderr, "[ KheMeetDoUnAssignTime(%p ", (void *) meet);
    KheMeetDebug(meet, 1, -1, stderr);
    fprintf(stderr, ")\n");
    KheMeetTestTasks(meet);
  }
  if( meet->event_in_soln != NULL )
    KheEventInSolnUnAssignTime(meet->event_in_soln, meet,
      meet->assigned_time_index);
  HaArrayForEach(meet->tasks, task, i)
  {
    if( DEBUG16 )
      fprintf(stderr, "%d: %p", i, (void *) task);
    KheTaskUnAssignTime(task, meet->assigned_time_index);
  }
  HaArrayForEach(meet->assigned_meets, child_meet, i)
    KheMeetDoUnAssignTime(child_meet);
  meet->assigned_time_index = NO_TIME_INDEX;
  if( DEBUG16 )
    fprintf(stderr, "] KheMeetDoUnAssignTime returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetChangeDemandChunkBases(KHE_MEET meet,                        */
/*    KHE_MEET ancestor_meet, int ancestor_offset)                           */
/*                                                                           */
/*  Change the bases of the demand chunks of meet so that they are linked    */
/*  to the supply chunks of ancestor_meet, starting at ancestor_offset.  Do  */
/*  this for all meets assigned to meet, recursively, as well.               */
/*                                                                           */
/*****************************************************************************/

static void KheMeetChangeDemandChunkBases(KHE_MEET meet,
  KHE_MEET ancestor_meet, int ancestor_offset)
{
  KHE_MATCHING_DEMAND_CHUNK dc;  int i;
  KHE_MATCHING_SUPPLY_CHUNK sc;  KHE_MEET child_meet;
  HaArrayForEach(meet->demand_chunks, dc, i)
  {
    sc = HaArray(ancestor_meet->supply_chunks, ancestor_offset + i);
    KheMatchingDemandChunkSetBase(dc, KheMatchingSupplyChunkBase(sc));
  }
  HaArrayForEach(meet->assigned_meets, child_meet, i)
    KheMeetChangeDemandChunkBases(child_meet,
      ancestor_meet, ancestor_offset + child_meet->target_offset);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDoAssign(KHE_MEET meet, KHE_MEET target_meet, int offset)    */
/*                                                                           */
/*  Assign meet to target_meet at offset.   This is called only from         */
/*  KheMeetMove, at a time when we are sure that all is well.                */
/*                                                                           */
/*****************************************************************************/

static void KheMeetDoAssign(KHE_MEET meet, KHE_MEET target_meet, int offset)
{
  KHE_MEET ancestor_meet;  KHE_MATCHING_TYPE mt;  int ancestor_offset;

  /* add meet's demand to all its new ancestors, and find ancestor_offset */
  ancestor_offset = offset;
  ancestor_meet = target_meet;
  ancestor_meet->all_demand += meet->all_demand;
  while( ancestor_meet->target_meet != NULL )
  {
    ancestor_offset += ancestor_meet->target_offset;
    ancestor_meet = ancestor_meet->target_meet;
    ancestor_meet->all_demand += meet->all_demand;
  }

  /* record the assignment */
  meet->target_meet = target_meet;
  meet->target_offset = offset;
  meet->target_index = HaArrayCount(target_meet->assigned_meets);
  HaArrayAddLast(target_meet->assigned_meets, meet);

  /* if required, change the bases of all descendants' demand chunks */
  if( HaArrayCount(meet->demand_chunks) > 0 )
  {
    mt = KheSolnMatchingType(meet->soln);
    if( mt == KHE_MATCHING_TYPE_SOLVE || mt == KHE_MATCHING_TYPE_EVAL_TIMES )
      KheMeetChangeDemandChunkBases(meet, ancestor_meet, ancestor_offset);
  }

  /* if assigning a time, inform descendants of meet */
  if( target_meet->assigned_time_index != NO_TIME_INDEX )
  {
    if( DEBUG6 )
      fprintf(stderr, "  KheMeetDoAssign(%s, %s) calling "
	"KheMeetDoAssignTime(%s)\n", KheMeetId(meet),
	KheMeetId(target_meet), KheMeetId(meet));
    KheMeetDoAssignTime(meet);
  }
  KheMeetDomainCheck(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDoUnAssign(KHE_MEET meet)                                    */
/*                                                                           */
/*  Unassign meet.  This is called only from when we know that all is well.  */
/*                                                                           */
/*****************************************************************************/

static void KheMeetDoUnAssign(KHE_MEET meet)
{
  KHE_MEET tmp, ancestor_meet;  KHE_MATCHING_TYPE mt;

  /* if unassigning a time, inform descendants of meet */
  if( meet->target_meet->assigned_time_index != NO_TIME_INDEX )
    KheMeetDoUnAssignTime(meet);

  /* if required, change the bases of all descendants' demand chunks */
  if( HaArrayCount(meet->demand_chunks) > 0 )
  {
    mt = KheSolnMatchingType(meet->soln);
    if( mt == KHE_MATCHING_TYPE_SOLVE || mt == KHE_MATCHING_TYPE_EVAL_TIMES )
      KheMeetChangeDemandChunkBases(meet, meet, 0);
  }

  /* remove meet's demand from all its ancestors */
  ancestor_meet = meet->target_meet;
  ancestor_meet->all_demand -= meet->all_demand;
  while( ancestor_meet->target_meet != NULL )
  {
    ancestor_meet = ancestor_meet->target_meet;
    ancestor_meet->all_demand -= meet->all_demand;
  }

  /* remove meet from its parent and fill any hole left behind */
  HnAssert(HaArray(meet->target_meet->assigned_meets,meet->target_index)==meet,
    "KheMeetDoUnAssign internal error");
  tmp = HaArrayLastAndDelete(meet->target_meet->assigned_meets);
  if( meet != tmp )
  {
    tmp->target_index = meet->target_index;
    HaArrayPut(meet->target_meet->assigned_meets, meet->target_index, tmp);
  }
  meet->target_meet = NULL;
  meet->target_offset = -1;
  meet->target_index = NO_TIME_INDEX;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDoMove(KHE_MEET meet, KHE_MEET target_meet, int offset)      */
/*                                                                           */
/*  Carry out a move, without reporting it to the solution path.             */
/*                                                                           */
/*****************************************************************************/

static void KheMeetDoMove(KHE_MEET meet, KHE_MEET target_meet, int offset)
{
  KHE_TIME t;
  if( DEBUG28 && KheMeetIsAssignedPreassigned(meet, &t) && target_meet == NULL )
  {
    fprintf(stderr, "  KheMeetDoMove(%s, NULL, -)\n", KheMeetId(meet));
    /* HnAbort("  KheMeetDoMove aborting now"); */
  }
  if( meet->target_meet != NULL )
    KheMeetDoUnAssign(meet);
  if( target_meet != NULL )
  {
    if( DEBUG6 )
      fprintf(stderr, "KheMeetDoMove(%s, %s, %d) calling KheMeetDoAssign\n",
	KheMeetId(meet), KheMeetId(target_meet), offset);
    KheMeetDoAssign(meet, target_meet, offset);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelMove(KHE_MEET meet, KHE_MEET target_meet, int offset)  */
/*                                                                           */
/*  Move meet to target_meet at offset, assuming all is well.                */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelMove(KHE_MEET meet, KHE_MEET target_meet, int offset)
{
  KheSolnOpMeetMove(meet->soln, meet, meet->target_meet,
    meet->target_offset, target_meet, offset);
  if( DEBUG6 )
    fprintf(stderr, "  KheMeetKernelMove(%s, %s, %d) calling KheMeetDoMove\n",
      KheMeetId(meet), target_meet == NULL ? "-" : KheMeetId(target_meet),
      offset);
  KheMeetDoMove(meet, target_meet, offset);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelMoveUndo(KHE_MEET meet,                                */
/*    KHE_MEET old_target_meet, int old_offset,                              */
/*    KHE_MEET new_target_meet, int new_offset)                              */
/*                                                                           */
/*  Carry out an undo of KheMeetKernelMove (except not the solution path).   */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelMoveUndo(KHE_MEET meet,
  KHE_MEET old_target_meet, int old_offset,
  KHE_MEET new_target_meet, int new_offset)
{
  KheMeetDoMove(meet, old_target_meet, old_offset);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetMoveCheck(KHE_MEET meet, KHE_MEET target_meet, int offset)   */
/*                                                                           */
/*  Check whether moving meet to target_meet at offset is possible.          */
/*                                                                           */
/*****************************************************************************/

bool KheMeetMoveCheck(KHE_MEET meet, KHE_MEET target_meet, int offset)
{
  KHE_MEET anc;  int offs;
  if( DEBUG1 )
    KheMeetAsstOpDebug("[ KheMeetMoveCheck", meet, target_meet, offset);

  /* return false if meet's assignment is fixed */
  if( meet->target_fixed )
    return KheMeetOpReturn(false, DEBUG1, "KheMeetMoveCheck", "fixed");

  /* return false if meet is a cycle meet */
  if( KheMeetIsCycleMeet(meet) )
    return KheMeetOpReturn(false, DEBUG1, "KheMeetMoveCheck", "cycle meet");

  if( target_meet == NULL )
  {
    /* return false if the move changes nothing */
    if( meet->target_meet == NULL )
      return KheMeetOpReturn(false, DEBUG1, "KheMeetMoveCheck", "null change");
  }
  else
  {
    /* return false if the move changes nothing */
    if( meet->target_meet == target_meet && meet->target_offset == offset )
      return KheMeetOpReturn(false, DEBUG1, "KheMeetMoveCheck", "no change");

    /* return false if offset is out of range */
    if( offset < 0 || offset > target_meet->duration - meet->duration )
      return KheMeetOpReturn(false, DEBUG1, "KheMeetMoveCheck", "offset");

    /* return false if the time domain of target_meet is not a subset of  */
    /* the time domain of meet                                            */
    if( KheMeetHasAncestorWithDomain(target_meet, &anc, &offs)
	&& !KheMeetDomainAllowsAssignment(meet, anc->time_domain, offset+offs) )
    {
      if( DEBUG1 )
      {
	fprintf(stderr, "  meet->time_domain: ");
	KheTimeGroupDebug(meet->time_domain, 3, 0, stderr);
	fprintf(stderr, "  anc->time_domain: ");
	KheTimeGroupDebug(anc->time_domain, 3, 0, stderr);
	fprintf(stderr, "  offs %d\n", offs);
      }
      return KheMeetOpReturn(false, DEBUG1, "KheMeetMoveCheck", "domains");
    }

    /* return false if the node rule would be violated */
    if( meet->node != NULL && (KheNodeParent(meet->node) == NULL ||
	KheNodeParent(meet->node) != target_meet->node) )
      return KheMeetOpReturn(false, DEBUG1, "KheMeetMoveCheck", "node rule");
  }
  return KheMeetOpReturn(true, DEBUG1, "KheMeetMoveCheck", "ok");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetMove(KHE_MEET meet, KHE_MEET target_meet, int offset)        */
/*                                                                           */
/*  If possible, move meet to target_meet at offset.                         */
/*                                                                           */
/*****************************************************************************/

bool KheMeetMove(KHE_MEET meet, KHE_MEET target_meet, int offset)
{
  KHE_TIME t;
  if( DEBUG8 )
    KheMeetAsstOpDebug("[ KheMeetMove", meet, target_meet, offset);
  if( DEBUG23 && KheMeetIsPreassigned(meet, &t) )
    HnAssert(target_meet != NULL, "KheMeetMove clearing preassignment");
  KheMeetTestTasks(meet);
  if( !KheMeetMoveCheck(meet, target_meet, offset) )
    return KheMeetOpReturn(false, DEBUG8, "KheMeetMove", "failed check");
  if( target_meet == NULL ) offset = -1;
  if( DEBUG6 && target_meet != NULL )
    fprintf(stderr, "  KheMeetMove(%s, %s, %d) calling KheMeetKernelMove\n",
      KheMeetId(meet), KheMeetId(target_meet), offset);
  KheMeetKernelMove(meet, target_meet, offset);
  return KheMeetOpReturn(true, DEBUG8, "KheMeetMove", "ok");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "assignment (helper functions)"                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetAssignCheck(KHE_MEET meet, KHE_MEET target_meet, int offset) */
/*                                                                           */
/*  Check whether meet can be assigned to target_meet at offset.             */
/*                                                                           */
/*****************************************************************************/

bool KheMeetAssignCheck(KHE_MEET meet, KHE_MEET target_meet, int offset)
{
  return meet->target_meet==NULL && KheMeetMoveCheck(meet, target_meet, offset);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetAssign(KHE_MEET meet, KHE_MEET target_meet, int offset)      */
/*                                                                           */
/*  Assign meet to target_meet at offset, if possible.                       */
/*                                                                           */
/*****************************************************************************/

bool KheMeetAssign(KHE_MEET meet, KHE_MEET target_meet, int offset)
{
  if( DEBUG6 && target_meet != NULL && meet->target_meet == NULL )
    fprintf(stderr, "  KheMeetAssign(%s, %s, %d) calling KheMeetMove\n",
      KheMeetId(meet), KheMeetId(target_meet), offset);
  KheMeetCheck(meet);
  return meet->target_meet == NULL && KheMeetMove(meet, target_meet, offset);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetUnAssignCheck(KHE_MEET meet)                                 */
/*                                                                           */
/*  Return true if KheMeetUnAssign would succeed.                            */
/*                                                                           */
/*****************************************************************************/

bool KheMeetUnAssignCheck(KHE_MEET meet)
{
  return KheMeetMoveCheck(meet, NULL, -1);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetUnAssign(KHE_MEET meet)                                      */
/*                                                                           */
/*  Unassign meet from whatever it is assigned to, if anything.              */
/*                                                                           */
/*****************************************************************************/

bool KheMeetUnAssign(KHE_MEET meet)
{
  bool res;  KHE_TIME time;
  if( DEBUG27(meet, time) )
  {
    fprintf(stderr, "[ KheMeetUnAssign(%s)\n", KheMeetId(meet));
    HnAbort("  temporary meet unassign abort");
  }
  KheMeetCheck(meet);
  res = KheMeetMove(meet, NULL, -1);
  if( DEBUG27(meet, time) )
    fprintf(stderr, "] KheMeetUnAssign returning %s\n", bool_show(res));

  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSwapDebug(char *op, KHE_MEET meet1, KHE_MEET meet2)          */
/*                                                                           */
/*  Generate a debug print of op(meet1, meet2).                              */
/*                                                                           */
/*****************************************************************************/

static void KheMeetSwapDebug(char *op, KHE_MEET meet1, KHE_MEET meet2)
{
  fprintf(stderr, "%s(", op);
  KheMeetDebug(meet1, 1, -1, stderr);
  fprintf(stderr, ", ");
  KheMeetDebug(meet2, 1, -1, stderr);
  fprintf(stderr, ")\n");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetSwapCheck(KHE_MEET meet1, KHE_MEET meet2)                    */
/*                                                                           */
/*  Return true if a swap of the assignments of meet1 and meet2 is possible. */
/*                                                                           */
/*****************************************************************************/

bool KheMeetSwapCheck(KHE_MEET meet1, KHE_MEET meet2)
{
  return KheMeetMoveCheck(meet1, meet2->target_meet, meet2->target_offset)
    && KheMeetMoveCheck(meet2, meet1->target_meet, meet1->target_offset);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetSwap(KHE_MEET meet1, KHE_MEET meet2)                         */
/*                                                                           */
/*  Either swap the assignments of meet1 and meet2 and return true, or, if   */
/*  that is not possible, change nothing and return false.                   */
/*                                                                           */
/*****************************************************************************/

bool KheMeetSwap(KHE_MEET meet1, KHE_MEET meet2)
{
  KHE_MEET meet1_target_meet;  int meet1_target_offset;
  if( DEBUG8 )
    KheMeetSwapDebug("[ KheMeetSwap", meet1, meet2);
  KheMeetCheck(meet1);
  KheMeetCheck(meet2);
  if( KheMeetSwapCheck(meet1, meet2) )
  {
    meet1_target_meet = meet1->target_meet;
    meet1_target_offset = meet1->target_offset;
    KheMeetKernelMove(meet1, meet2->target_meet, meet2->target_offset);
    KheMeetKernelMove(meet2, meet1_target_meet, meet1_target_offset);
    return KheMeetOpReturn(true, DEBUG8, "KheMeetSwap", "ok");
  }
  else
    return KheMeetOpReturn(false, DEBUG8, "KheMeetSwap", "failed check");
}


/*****************************************************************************/
/*                                                                           */
/*  void GetBlockSwapOffsets(KHE_MEET meet1, KHE_MEET meet2,                 */
/*    int *meet1_block_offset, int *meet2_block_offset)                      */
/*                                                                           */
/*  Helper function for use when block swapping.  Here meet1 and meet2 are   */
/*  in their initial state and we are interested in block swapping them.     */
/*  With an ordinary swap, meet1's target offset is used for meet2, and      */
/*  meet2's target offset is used for meet1.  With a block swap, one of      */
/*  these could be different.  This function works them both out.            */
/*                                                                           */
/*  It is a precondition that both meet1 and meet2 are assigned, but         */
/*  possibly to different meets.                                             */
/*                                                                           */
/*****************************************************************************/

static void GetBlockSwapOffsets(KHE_MEET meet1, KHE_MEET meet2,
  int *meet1_block_offset, int *meet2_block_offset)
{
  if( meet1->target_meet == NULL || meet2->target_meet == NULL )
  {
    /* not a full swap, so not a block swap */
    *meet1_block_offset = meet1->target_offset;
    *meet2_block_offset = meet2->target_offset;
  }
  else if( meet1->target_meet != meet2->target_meet )
  {
    /* assigned to different meets, so not a block swap */
    *meet1_block_offset = meet1->target_offset;
    *meet2_block_offset = meet2->target_offset;
  }
  else if( meet1->target_offset + meet1->duration == meet2->target_offset )
  {
    /* block swap with meet2 immediately following meet1 */
    *meet1_block_offset = meet1->target_offset;             /* used by meet2 */
    *meet2_block_offset = meet1->target_offset + meet2->duration; /* by meet1*/
  }
  else if( meet2->target_offset + meet2->duration == meet1->target_offset )
  {
    /* block swap with meet1 immediately following meet2 */
    *meet2_block_offset = meet2->target_offset;             /* used by meet1 */
    *meet1_block_offset = meet2->target_offset + meet1->duration; /* by meet2*/
  }
  else
  {
    /* assigned to same meet but not adjacent, so not a block swap */
    *meet1_block_offset = meet1->target_offset;
    *meet2_block_offset = meet2->target_offset;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetBlockSwapCheck(KHE_MEET meet1, KHE_MEET meet2)               */
/*                                                                           */
/*  Return true if a block swap of the assignments of meet1 and meet2 is     */
/*  possible.                                                                */
/*                                                                           */
/*****************************************************************************/

bool KheMeetBlockSwapCheck(KHE_MEET meet1, KHE_MEET meet2)
{
  int meet1_block_offset, meet2_block_offset;
  GetBlockSwapOffsets(meet1, meet2, &meet1_block_offset, &meet2_block_offset);
  return KheMeetMoveCheck(meet1, meet2->target_meet, meet2_block_offset)
    && KheMeetMoveCheck(meet2, meet1->target_meet, meet1_block_offset);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetBlockSwap(KHE_MEET meet1, KHE_MEET meet2)                    */
/*                                                                           */
/*  Either block swap the assignments of meet1 and meet2 and return true,    */
/*  or, if that is not possible, change nothing and return false.            */
/*                                                                           */
/*****************************************************************************/

bool KheMeetBlockSwap(KHE_MEET meet1, KHE_MEET meet2)
{
  int meet1_block_offset, meet2_block_offset;  KHE_MEET meet1_target_meet;
  if( DEBUG8 )
    KheMeetSwapDebug("[ KheMeetBlockSwap", meet1, meet2);
  GetBlockSwapOffsets(meet1, meet2, &meet1_block_offset, &meet2_block_offset);
  if( KheMeetMoveCheck(meet1, meet2->target_meet, meet2_block_offset)
    && KheMeetMoveCheck(meet2, meet1->target_meet, meet1_block_offset) )
  {
    meet1_target_meet = meet1->target_meet;
    KheMeetKernelMove(meet1, meet2->target_meet, meet2_block_offset);
    KheMeetKernelMove(meet2, meet1_target_meet, meet1_block_offset);
    return KheMeetOpReturn(true, DEBUG8, "KheMeetBlockSwap", "ok");
  }
  else
    return KheMeetOpReturn(false, DEBUG8, "KheMeetBlockSwap", "failed check");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "assignment (queries)"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheMeetAsst(KHE_MEET meet)                                      */
/*                                                                           */
/*  Return the assignment of meet, or NULL if none.                          */
/*                                                                           */
/*****************************************************************************/

KHE_MEET KheMeetAsst(KHE_MEET meet)
{
  return meet->target_meet;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetAsstOffset(KHE_MEET meet)                                     */
/*                                                                           */
/*  Return the assignment offset of meet, or -1 if none.                     */
/*                                                                           */
/*****************************************************************************/

int KheMeetAsstOffset(KHE_MEET meet)
{
  return meet->target_offset;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetAssignedToCount(KHE_MEET target_meet)                         */
/*                                                                           */
/*  Return the number of meets assigned to target_meet.                      */
/*                                                                           */
/*****************************************************************************/

int KheMeetAssignedToCount(KHE_MEET target_meet)
{
  return HaArrayCount(target_meet->assigned_meets);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheMeetAssignedTo(KHE_MEET target_meet, int i)                  */
/*                                                                           */
/*  Return the i'th meet assigned to target_meet.                            */
/*                                                                           */
/*****************************************************************************/

KHE_MEET KheMeetAssignedTo(KHE_MEET target_meet, int i)
{
  return HaArray(target_meet->assigned_meets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheMeetRoot(KHE_MEET meet, int *offset_in_root)                 */
/*                                                                           */
/*  Return the root of meet, setting *offset_in_root to its offset.          */
/*                                                                           */
/*****************************************************************************/

KHE_MEET KheMeetRoot(KHE_MEET meet, int *offset_in_root)
{
  *offset_in_root = 0;
  while( meet->target_meet != NULL )
  {
    *offset_in_root += meet->target_offset;
    meet = meet->target_meet;
  }
  return meet;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetOverlap(KHE_MEET meet1, KHE_MEET meet2)                      */
/*                                                                           */
/*  Return true if meet1 and meet2 overlap in time.                          */
/*                                                                           */
/*****************************************************************************/

bool KheMeetOverlap(KHE_MEET meet1, KHE_MEET meet2)
{
  KHE_MEET root1, root2;  int offset1, offset2;
  root1 = KheMeetRoot(meet1, &offset1);
  root2 = KheMeetRoot(meet2, &offset2);
  if( root1 != root2 )
    return false;
  if( offset1 + meet1->duration <= offset2 )
    return false;
  if( offset2 + meet2->duration <= offset1 )
    return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetAdjacent(KHE_MEET meet1, KHE_MEET meet2, bool *swap)         */
/*                                                                           */
/*  Return true if meet1 and meet2 are adjacent in time, and in that case    */
/*  also set *swap, to true if they need to be swapped to be in time order.  */
/*                                                                           */
/*****************************************************************************/

bool KheMeetAdjacent(KHE_MEET meet1, KHE_MEET meet2, bool *swap)
{
  KHE_MEET root1, root2;  int offset1, offset2;
  root1 = KheMeetRoot(meet1, &offset1);
  root2 = KheMeetRoot(meet2, &offset2);
  if( root1 != root2 )
    return false;
  if( offset1 + meet1->duration == offset2 )
  {
    *swap = false;
    return true;
  }
  if( offset2 + meet2->duration <= offset1 )
  {
    *swap = true;
    return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "assignment (fixing and unfixing)"                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDoAssignFix(KHE_MEET meet)                                   */
/*                                                                           */
/*  Change the state of the solution to fix meet.                            */
/*                                                                           */
/*****************************************************************************/

static void KheMeetAssignFixRecursive(KHE_MEET meet)
{
  KHE_MEET sub_meet;  int i;
  if( meet->event_in_soln != NULL )
    KheEventInSolnMeetAssignFix(meet->event_in_soln);
  HaArrayForEach(meet->assigned_meets, sub_meet, i)
    if( sub_meet->target_fixed )
      KheMeetAssignFixRecursive(sub_meet);
}

static void KheMeetDoAssignFix(KHE_MEET meet)
{
  meet->target_fixed = true;
  KheMeetAssignFixRecursive(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDoAssignUnFix(KHE_MEET meet)                                 */
/*                                                                           */
/*  Change the state of the solution to unfix meet.                          */
/*                                                                           */
/*****************************************************************************/

static void KheMeetAssignUnFixRecursive(KHE_MEET meet)
{
  KHE_MEET sub_meet;  int i;
  if( meet->event_in_soln != NULL )
    KheEventInSolnMeetAssignUnFix(meet->event_in_soln);
  HaArrayForEach(meet->assigned_meets, sub_meet, i)
    if( sub_meet->target_fixed )
      KheMeetAssignUnFixRecursive(sub_meet);
}

void KheMeetDoAssignUnFix(KHE_MEET meet)
{
  meet->target_fixed = false;
  KheMeetAssignUnFixRecursive(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelAssignFix(KHE_MEET meet)                               */
/*                                                                           */
/*  The kernel operation which fixes meet.                                   */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelAssignFix(KHE_MEET meet)
{
  KheMeetDoAssignFix(meet);
  KheSolnOpMeetAssignFix(meet->soln, meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelAssignFixUndo(KHE_MEET meet)                           */
/*                                                                           */
/*  Undo KheMeetKernelAssignFix.                                             */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelAssignFixUndo(KHE_MEET meet)
{
  KheMeetDoAssignUnFix(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelAssignUnFix(KHE_MEET meet)                             */
/*                                                                           */
/*  The kernel operation which unfixes meet.                                 */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelAssignUnFix(KHE_MEET meet)
{
  KheMeetDoAssignUnFix(meet);
  KheSolnOpMeetAssignUnFix(meet->soln, meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelAssignUnFixUndo(KHE_MEET meet)                         */
/*                                                                           */
/*  Undo KheMeetKernelAssignUnFix.                                           */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelAssignUnFixUndo(KHE_MEET meet)
{
  KheMeetDoAssignFix(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAssignFix(KHE_MEET meet)                                     */
/*                                                                           */
/*  Prevent meet from assigning and unassigning.                             */
/*                                                                           */
/*****************************************************************************/

void KheMeetAssignFix(KHE_MEET meet)
{
  if( !meet->target_fixed )
  {
    if( DEBUG5 )
    {
      fprintf(stderr, "[ KheMeetAssignFix(");
      KheMeetDebug(meet, 1, -1, stderr);
      fprintf(stderr, ")\n");
    }
    KheMeetKernelAssignFix(meet);
    if( DEBUG5 )
      fprintf(stderr, "]\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAssignUnFix(KHE_MEET meet)                                   */
/*                                                                           */
/*  Allow meet to assign and unassign.                                       */
/*                                                                           */
/*****************************************************************************/

void KheMeetAssignUnFix(KHE_MEET meet)
{
  if( meet->target_fixed )
  {
    if( DEBUG5 )
    {
      fprintf(stderr, "[ KheMeetAssignUnFix(");
      KheMeetDebug(meet, 1, -1, stderr);
      fprintf(stderr, ")\n");
    }
    KheMeetKernelAssignUnFix(meet);
    if( DEBUG5 )
      fprintf(stderr, "]\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetAssignIsFixed(KHE_MEET meet)                                 */
/*                                                                           */
/*  Return true if meet assignments are fixed.                               */
/*                                                                           */
/*****************************************************************************/

bool KheMeetAssignIsFixed(KHE_MEET meet)
{
  return meet->target_fixed;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetIsMovable(KHE_MEET meet)                                     */
/*                                                                           */
/*  Return true if meet is movable.                                          */
/*                                                                           */
/*****************************************************************************/

bool KheMeetIsMovable(KHE_MEET meet)
{
  return !meet->target_fixed && !KheMeetIsCycleMeet(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheMeetFirstMovable(KHE_MEET meet, int *offset_in_result)       */
/*                                                                           */
/*  Return the first meet in the chain of assignments leading out of meet    */
/*  whose assignment could be changed, because it is not fixed and the meet  */
/*  is not a cycle meet, or NULL if there is no such meet.  If the result    */
/*  is non-NULL, set *offset_in_result to the offset of meet in the result   */
/*  meet, otherwise leave *offset_in_result undefined.                       */
/*                                                                           */
/*****************************************************************************/

KHE_MEET KheMeetFirstMovable(KHE_MEET meet, int *offset_in_result)
{
  HnAssert(meet != NULL, "KheMeetFirstUnFixed:  meet == NULL");
  *offset_in_result = 0;
  while( !KheMeetIsMovable(meet) )
  {
    *offset_in_result += meet->target_offset;
    meet = meet->target_meet;
    if( meet == NULL )
      return NULL;
  }
  return meet;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheMeetLastFixed(KHE_MEET meet, int *offset_in_result)          */
/*                                                                           */
/*  Return the last meet in the chain of fixed assignments leading out of    */
/*  of meet, and set *offset_in_result to the offset of meet in that meet.   */
/*  The result will be meet itself, with *offset_in_result set to 0, if      */
/*  meet's own assignment is unfixed or NULL.                                */
/*                                                                           */
/*****************************************************************************/

KHE_MEET KheMeetLastFixed(KHE_MEET meet, int *offset_in_result)
{
  HnAssert(meet != NULL, "KheMeetLastFixed:  meet == NULL");
  *offset_in_result = 0;
  while( meet->target_meet != NULL && meet->target_fixed )
  {
    *offset_in_result += meet->target_offset;
    meet = meet->target_meet;
  }
  return meet;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "cycle meets and time assignment"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetAssignedTimeIndexAndDomain(KHE_MEET meet, KHE_TIME time)  */
/*                                                                           */
/*  Set the assigned time index and domain attributes of meet, to make this  */
/*  a cycle meet.  This is called only when initializing the cycle meet.     */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetAssignedTimeIndexAndDomain(KHE_MEET meet, KHE_TIME time)
{
  meet->assigned_time_index = KheTimeIndex(time);
  KheMeetDoSetDomain(meet, KheTimeSingletonTimeGroup(time));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetAssignedTimeIndex(KHE_MEET meet)                              */
/*                                                                           */
/*  Return the assigned time index of meet, or NO_TIME_INDEX if meet is      */
/*  not assigned a time.                                                     */
/*                                                                           */
/*****************************************************************************/

int KheMeetAssignedTimeIndex(KHE_MEET meet)
{
  return meet->assigned_time_index;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetIsCycleMeet(KHE_MEET meet)                                   */
/*                                                                           */
/*  Return true if meet is a cycle meet.                                     */
/*                                                                           */
/*  Implementation note.  To save a boolean field, we don't store this       */
/*  condition explicitly in meet.  Instead, we use the fact that the only    */
/*  meets that can have an assigned time index without being                 */
/*  assigned to something else are cycle meets.                              */
/*                                                                           */
/*****************************************************************************/

bool KheMeetIsCycleMeet(KHE_MEET meet)
{
  return meet->target_meet == NULL &&
    meet->assigned_time_index != NO_TIME_INDEX;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetMoveTimeCheck(KHE_MEET meet, KHE_TIME t)                     */
/*                                                                           */
/*  Check whether meet can be moved to t.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheMeetMoveTimeCheck(KHE_MEET meet, KHE_TIME t)
{
  KHE_MEET target_meet;  int offset;
  target_meet = KheSolnTimeCycleMeet(meet->soln, t);
  offset = KheSolnTimeCycleMeetOffset(meet->soln, t);
  return KheMeetMoveCheck(meet, target_meet, offset);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetMoveTime(KHE_MEET meet, KHE_TIME t)                          */
/*                                                                           */
/*  Move meet to t.                                                          */
/*                                                                           */
/*****************************************************************************/

bool KheMeetMoveTime(KHE_MEET meet, KHE_TIME t)
{
  KHE_MEET target_meet;  int offset;
  target_meet = KheSolnTimeCycleMeet(meet->soln, t);
  offset = KheSolnTimeCycleMeetOffset(meet->soln, t);
  if( DEBUG6 )
    fprintf(stderr, "  KheMeetMoveTime(%s, %s) calling KheMeetMove"
      "(%s, %s, %d)\n", KheMeetId(meet), KheTimeId(t), KheMeetId(meet),
      KheMeetId(target_meet), offset);
  return KheMeetMove(meet, target_meet, offset);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetAssignTimeCheck(KHE_MEET meet, KHE_TIME time)                */
/*                                                                           */
/*  Check whether meet can be assigned to time.                              */
/*                                                                           */
/*****************************************************************************/

bool KheMeetAssignTimeCheck(KHE_MEET meet, KHE_TIME time)
{
  KHE_MEET target_meet;  int offset;
  target_meet = KheSolnTimeCycleMeet(meet->soln, time);
  offset = KheSolnTimeCycleMeetOffset(meet->soln, time);
  return KheMeetAssignCheck(meet, target_meet, offset);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetAssignTime(KHE_MEET meet, KHE_TIME time)                     */
/*                                                                           */
/*  Assign meet to time.                                                     */
/*                                                                           */
/*****************************************************************************/

bool KheMeetAssignTime(KHE_MEET meet, KHE_TIME time)
{
  KHE_MEET target_meet;  int offset;
  target_meet = KheSolnTimeCycleMeet(meet->soln, time);
  offset = KheSolnTimeCycleMeetOffset(meet->soln, time);
  return KheMeetAssign(meet, target_meet, offset);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetUnAssignTimeCheck(KHE_MEET meet)                             */
/*                                                                           */
/*  Check whether meet can be unassigned from the time it is assigned to.    */
/*                                                                           */
/*****************************************************************************/

bool KheMeetUnAssignTimeCheck(KHE_MEET meet)
{
  return KheMeetUnAssignCheck(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetUnAssignTime(KHE_MEET meet)                                  */
/*                                                                           */
/*  Unassign meet from whatever time it is assigned to.                      */
/*                                                                           */
/*****************************************************************************/

bool KheMeetUnAssignTime(KHE_MEET meet)
{
  return KheMeetUnAssign(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheMeetAsstTime(KHE_MEET meet)                                  */
/*                                                                           */
/*  Return the time that meet is assigned to, or NULL if none.               */
/*                                                                           */
/*****************************************************************************/

KHE_TIME KheMeetAsstTime(KHE_MEET meet)
{
  KHE_INSTANCE ins;
  if( meet->assigned_time_index == NO_TIME_INDEX )
    return NULL;
  else
  {
    ins = KheSolnInstance(meet->soln);
    return KheInstanceTime(ins, meet->assigned_time_index);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "meet domains and bounds"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheMeetDomain(KHE_MEET meet)                              */
/*                                                                           */
/*  Return the domain of meet.  This is NULL if the domain is automatic.     */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheMeetDomain(KHE_MEET meet)
{
  return meet->time_domain;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDoSetDomain(KHE_MEET meet, KHE_TIME_GROUP tg)                */
/*                                                                           */
/*  Set the domain of meet to tg, assuming that this is safe to do.          */
/*                                                                           */
/*****************************************************************************/

static void KheMeetDoSetDomain(KHE_MEET meet, KHE_TIME_GROUP tg)
{
  KHE_MATCHING_DEMAND_CHUNK dc;  int i;  KHE_MATCHING_TYPE t;
  meet->time_domain = tg;
  if( tg != NULL && KheSolnMatching(meet->soln) != NULL )
  {
    t = KheSolnMatchingType(meet->soln);
    if(t==KHE_MATCHING_TYPE_EVAL_INITIAL || t==KHE_MATCHING_TYPE_EVAL_RESOURCES)
      HaArrayForEach(meet->demand_chunks, dc, i)
	KheMatchingDemandChunkSetDomain(dc, KheTimeGroupTimeSet(tg),
	  KHE_MATCHING_DOMAIN_CHANGE_TO_OTHER);
  }
  KheMeetDomainCheck(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDoAddMeetBound(KHE_MEET meet, KHE_MEET_BOUND mb)             */
/*                                                                           */
/*  Add mb to meet.                                                          */
/*                                                                           */
/*****************************************************************************/

static void KheMeetDoAddMeetBound(KHE_MEET meet, KHE_MEET_BOUND mb)
{
  /* update meet and mb */
  if( DEBUG22 )
    fprintf(stderr, "KheMeetDoAddMeetBound(meet %p, mb %p) div %d\n",
      (void *) meet, (void *) mb, KheSolnDiversifier(meet->soln));
  HaArrayAddLast(meet->meet_bounds, mb);
  KheMeetBoundAddMeet(mb, meet);

  /* change meet's domain, if required */
  if( meet->time_domain != NULL )
    KheMeetDoSetDomain(meet, KheMeetAddBoundDomain(meet, mb));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDoDeleteMeetBound(KHE_MEET meet, KHE_MEET_BOUND mb,          */
/*    int pos, KHE_TIME_GROUP new_domain)                                    */
/*                                                                           */
/*  Delete mb from meet.  Here pos is mb's position in meet, and new_domain  */
/*  is the value of meet's domain after the deletion.                        */
/*                                                                           */
/*****************************************************************************/

static void KheMeetDoDeleteMeetBound(KHE_MEET meet, KHE_MEET_BOUND mb,
  int pos, KHE_TIME_GROUP new_domain)
{
  /* update meet and mb */
  HnAssert(HaArray(meet->meet_bounds, pos) == mb,
    "KheMeetDoDeleteMeetBound internal error");
  if( DEBUG22 )
    fprintf(stderr, "KheMeetDoDeleteMeetBound(meet %p, mb %p) div %d\n",
      (void *) meet, (void *) mb, KheSolnDiversifier(meet->soln));
  HaArrayDeleteAndPlug(meet->meet_bounds, pos);
  KheMeetBoundDeleteMeet(mb, meet);

  /* change meet's domain, if required */
  if( meet->time_domain != NULL )
    KheMeetDoSetDomain(meet, new_domain);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelAddMeetBound(KHE_MEET meet, KHE_MEET_BOUND mb)         */
/*                                                                           */
/*  Kernel operation which adds mb to meet.                                  */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelAddMeetBound(KHE_MEET meet, KHE_MEET_BOUND mb)
{
  if( DEBUG22 )
    fprintf(stderr, "KheMeetKernelAddMeetBound(meet %p, mb %p) div %d\n",
      (void *) meet, (void *) mb, KheSolnDiversifier(meet->soln));
  KheSolnOpMeetAddMeetBound(meet->soln, meet, mb);
  KheMeetDoAddMeetBound(meet, mb);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelAddMeetBoundUndo(KHE_MEET meet, KHE_MEET_BOUND mb)     */
/*                                                                           */
/*  Undo KheMeetKernelAddMeetBound.                                          */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelAddMeetBoundUndo(KHE_MEET meet, KHE_MEET_BOUND mb)
{
  int pos;  KHE_TIME_GROUP new_domain;
  if( DEBUG22 )
    fprintf(stderr, "KheMeetKernelAddMeetBoundUndo(meet %p, mb %p) div %d\n",
      (void *) meet, (void *) mb, KheSolnDiversifier(meet->soln));
  if( !HaArrayContains(meet->meet_bounds, mb, &pos) )
  {
    if( DEBUG22 )
    {
      fprintf(stderr,
	"KheMeetKernelAddMeetBoundUndo(meet %p, mb %p) div %d failing\n",
	(void *) meet, (void *) mb, KheSolnDiversifier(meet->soln));
    }
    HnAbort("KheMeetKernelAddMeetBoundUndo internal error");
  }
  if( meet->time_domain != NULL )
    new_domain = KheMeetDeleteBoundDomain(meet, mb);
  else
    new_domain = NULL;
  KheMeetDoDeleteMeetBound(meet, mb, pos, new_domain);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelDeleteMeetBound(KHE_MEET meet, KHE_MEET_BOUND mb)      */
/*                                                                           */
/*  Delete mb from meet.                                                     */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelDeleteMeetBound(KHE_MEET meet, KHE_MEET_BOUND mb)
{
  int pos;  KHE_TIME_GROUP new_domain;
  if( DEBUG22 )
    fprintf(stderr, "KheMeetKernelDeleteMeetBound(meet %p, mb %p) div %d\n",
      (void *) meet, (void *) mb, KheSolnDiversifier(meet->soln));
  if( !HaArrayContains(meet->meet_bounds, mb, &pos) )
    HnAbort("KheMeetKernelAddMeetBoundUndo internal error");
  if( meet->time_domain != NULL )
    new_domain = KheMeetDeleteBoundDomain(meet, mb);
  else
    new_domain = NULL;
  KheMeetDoDeleteMeetBound(meet, mb, pos, new_domain);
  KheSolnOpMeetDeleteMeetBound(meet->soln, meet, mb);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelDeleteMeetBoundUndo(KHE_MEET meet, KHE_MEET_BOUND mb)  */
/*                                                                           */
/*  Undo KheMeetKernelDeleteMeetBound.                                       */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelDeleteMeetBoundUndo(KHE_MEET meet, KHE_MEET_BOUND mb)
{
  if( DEBUG22 )
    fprintf(stderr, "KheMeetKernelDeleteMeetBoundUndo(meet %p, mb %p) div %d\n",
      (void *) meet, (void *) mb, KheSolnDiversifier(meet->soln));
  KheMeetDoAddMeetBound(meet, mb);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetAddMeetBoundCheck(KHE_MEET meet, KHE_MEET_BOUND mb)          */
/*                                                                           */
/*  Return true if it is safe to add mb to meet.                             */
/*                                                                           */
/*****************************************************************************/

bool KheMeetAddMeetBoundCheck(KHE_MEET meet, KHE_MEET_BOUND mb)
{
  KHE_MEET anc, a;  int offs /* , pos */, durn, i;  KHE_TIME_GROUP tg;
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheMeetAddMeetBoundCheck(");
    KheMeetDebug(meet, 1, -1, stderr);
    fprintf(stderr, ", mb)\n");
  }

  /* meet bound must not be already present */
  /* *** allowing this now (JeffK 30/03/2024)
  if( HaArrayContains(meet->meet_bounds, mb, &pos) )
    HnAbort("KheMeetAddMeetBoundCheck: mb already present in meet");
  *** */

  /* meet must not be a cycle meet */
  if( KheMeetIsCycleMeet(meet) )
    return KheMeetOpReturn(false, DEBUG2, "KheMeetAddMeetBoundCheck", "cycle");

  /* check compatibility of tg with the closest ancestor domain, if any */
  durn = KheMeetDuration(meet);
  tg = KheMeetBoundTimeGroup(mb, durn);
  if( KheMeetHasAncestorWithDomain(meet->target_meet, &anc, &offs) &&
      !KheTimeGroupDomainsAllowAssignment(tg, anc->time_domain,
	meet->target_offset + offs) )
  {
    if( DEBUG25 )
    {
      fprintf(stderr, "[ KheMeetAddMeetBoundCheck returning false:\n");
      fprintf(stderr, "    meet:\n");
      KheMeetDebug(meet, 2, 6, stderr);
      for( i = 1, a = meet->target_meet;  a != NULL;  i++, a = a->target_meet )
      {
	fprintf(stderr, "    ancestor %d%s:\n", i, a == anc ? " (== anc)" : "");
	KheMeetDebug(a, 2, 6, stderr);
      }
      fprintf(stderr, "  KheMeetHasAncestorWithDomain:\n");
      fprintf(stderr, "    meet->target_meet:\n");
      KheMeetDebug(meet->target_meet, 2, 6, stderr);
      fprintf(stderr, "    ancestor (offset %d):\n", offs);
      KheMeetDebug(anc, 2, 6, stderr);
      fprintf(stderr, "  && !KheTimeGroupDomainsAllowAssignment:\n");
      fprintf(stderr, "    KheMeetBoundTimeGroup(mb, %d):\n", durn);
      KheTimeGroupDebug(tg, 2, 6, stderr);
      fprintf(stderr, "    anc->time_domain (offset %d + %d = %d):\n",
        meet->target_offset, offs, meet->target_offset + offs);
      KheTimeGroupDebug(anc->time_domain, 2, 6, stderr);
      fprintf(stderr, "] KheMeetAddMeetBoundCheck\n");
    }
    return KheMeetOpReturn(false, DEBUG2, "KheMeetAddMeetBoundCheck", "anc");
  }

  /* all in order */
  return KheMeetOpReturn(true, DEBUG2, "KheMeetAddMeetBoundCheck", "ok");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAddMeetBound(KHE_MEET meet, KHE_MEET_BOUND mb)               */
/*                                                                           */
/*  Add mb to meet.                                                          */
/*                                                                           */
/*****************************************************************************/

bool KheMeetAddMeetBound(KHE_MEET meet, KHE_MEET_BOUND mb)
{
  if( !KheMeetAddMeetBoundCheck(meet, mb) )
    return false;

  if( DEBUG13 )
  {
    fprintf(stderr, "[ KheMeetAddMeetBound(");
    KheMeetDebug(meet, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheTimeGroupDebug(KheMeetBoundTimeGroup(mb, meet->duration), 1, -1, stderr);
    fprintf(stderr, ")\n");
    fprintf(stderr, "  initial domain: ");
    KheTimeGroupDebug(meet->time_domain, 1, 0, stderr);
  }

  /* carry out the kernel add operation */
  KheMeetKernelAddMeetBound(meet, mb);

  if( DEBUG13 )
  {
    fprintf(stderr, "  final domain: ");
    KheTimeGroupDebug(meet->time_domain, 1, 0, stderr);
    fprintf(stderr, "] KheMeetAddMeetBound returning true\n");
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetDoDeleteMeetBoundCheck(KHE_MEET meet, KHE_MEET_BOUND mb,     */
/*    int *pos, KHE_TIME_GROUP *new_domain)                                  */
/*                                                                           */
/*  Check that it is safe to delete mb from meet.  If so, set *pos to mb's   */
/*  position in meet->meet_bounds, and set *new_domain to the new domain.    */
/*                                                                           */
/*****************************************************************************/

bool KheMeetDoDeleteMeetBoundCheck(KHE_MEET meet, KHE_MEET_BOUND mb,
  int *pos, KHE_TIME_GROUP *new_domain)
{
  int i;  KHE_MEET child_meet;

  /* abort if mb is not present in meet */
  if( !HaArrayContains(meet->meet_bounds, mb, pos) )
    HnAbort("KheMeetDeleteMeetBoundCheck:  meet does not contain mb");

  /* meet must not be a cycle meet */
  if( KheMeetIsCycleMeet(meet) )
    return KheMeetOpReturn(false, DEBUG2,
      "KheMeetDoDeleteMeetBoundCheck", "cycle");

  /* check compatibility with descendant domains */
  if( meet->time_domain != NULL )
  {
    *new_domain = KheMeetDeleteBoundDomain(meet, mb);
    HaArrayForEach(meet->assigned_meets, child_meet, i)
      if( !KheMeetDomainAllowsAssignment(child_meet, *new_domain,
	    child_meet->target_offset) )
	return KheMeetOpReturn(false, DEBUG2, "KheMeetDoDeleteMeetBoundCheck",
	  "child");
  }
  else
    *new_domain = NULL;

  /* all in order */
  return KheMeetOpReturn(true, DEBUG2, "KheMeetDoDeleteMeetBoundCheck", "ok");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetDeleteMeetBoundCheck(KHE_MEET meet, KHE_MEET_BOUND mb)       */
/*                                                                           */
/*  Check that it is safe to delete mb from meet.                            */
/*                                                                           */
/*****************************************************************************/

bool KheMeetDeleteMeetBoundCheck(KHE_MEET meet, KHE_MEET_BOUND mb)
{
  int pos;  KHE_TIME_GROUP new_domain;
  return KheMeetDoDeleteMeetBoundCheck(meet, mb, &pos, &new_domain);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDeleteMeetBound(KHE_MEET meet, KHE_MEET_BOUND mb)            */
/*                                                                           */
/*  Delete mb from meet.                                                     */
/*                                                                           */
/*****************************************************************************/

bool KheMeetDeleteMeetBound(KHE_MEET meet, KHE_MEET_BOUND mb)
{
  int pos;  KHE_TIME_GROUP new_domain;

  /* make sure that the operation can proceed */
  if( !KheMeetDoDeleteMeetBoundCheck(meet, mb, &pos, &new_domain) )
    return false;

  if( DEBUG13 )
  {
    fprintf(stderr, "[ KheMeetDeleteMeetBound(");
    KheMeetDebug(meet, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheTimeGroupDebug(KheMeetBoundTimeGroup(mb, meet->duration), 1, -1, stderr);
    fprintf(stderr, ")\n");
    fprintf(stderr, "  initial domain: ");
    KheTimeGroupDebug(meet->time_domain, 1, 0, stderr);
  }

  /* carry out the kernel delete operation */
  /* the body of KheMeetKernelDeleteMeetBound, minus things already done */
  KheMeetDoDeleteMeetBound(meet, mb, pos, new_domain);
  KheSolnOpMeetDeleteMeetBound(meet->soln, meet, mb);

  if( DEBUG13 )
  {
    fprintf(stderr, "  final domain: ");
    KheTimeGroupDebug(meet->time_domain, 1, 0, stderr);
    fprintf(stderr, "] KheMeetDeleteMeetBound returning true\n");
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetMeetBoundCount(KHE_MEET meet)                                 */
/*                                                                           */
/*  Return the number of meet bounds of meet.                                */
/*                                                                           */
/*****************************************************************************/

int KheMeetMeetBoundCount(KHE_MEET meet)
{
  return HaArrayCount(meet->meet_bounds);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET_BOUND KheMeetMeetBound(KHE_MEET meet, int i)                    */
/*                                                                           */
/*  Return the i'th meet bound of meet.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_MEET_BOUND KheMeetMeetBound(KHE_MEET meet, int i)
{
  return HaArray(meet->meet_bounds, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "meet domains and bounds (automatic domains)"                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDoSetAutoDomain(KHE_MEET meet, bool automatic)               */
/*                                                                           */
/*  Carry out a set auto domain operation, without reporting it to the       */
/*  soln path.                                                               */
/*                                                                           */
/*****************************************************************************/

static void KheMeetDoSetAutoDomain(KHE_MEET meet, bool automatic)
{
  meet->time_domain = (automatic ? NULL : KheMeetBoundDomain(meet));
  KheMeetDomainCheck(meet);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelSetAutoDomain(KHE_MEET meet, bool automatic)           */
/*                                                                           */
/*  Set meet's domain to automatic, assuming all is well.                    */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelSetAutoDomain(KHE_MEET meet, bool automatic)
{
  KheSolnOpMeetSetAutoDomain(meet->soln, meet, automatic);
  KheMeetDoSetAutoDomain(meet, automatic);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetKernelSetAutoDomainUndo(KHE_MEET meet, bool automatic)       */
/*                                                                           */
/*  Undo KheMeetKernelSetAutoDomain.                                         */
/*                                                                           */
/*****************************************************************************/

void KheMeetKernelSetAutoDomainUndo(KHE_MEET meet, bool automatic)
{
  KheMeetDoSetAutoDomain(meet, !automatic);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetSetAutoDomainCheck(KHE_MEET meet, bool automatic)            */
/*                                                                           */
/*  Check whether it is safe to set meet's domain to automatic, or away      */
/*  from automatic, depending on the value of automatic.                     */
/*                                                                           */
/*****************************************************************************/

bool KheMeetSetAutoDomainCheck(KHE_MEET meet, bool automatic)
{
  KHE_TIME_GROUP tg;  KHE_MEET anc, cm;  int offs, i;
  if( meet->time_domain == NULL && !automatic ) /* only case that might fail */
  {
    tg = KheMeetBoundDomain(meet);
    if( KheMeetHasAncestorWithDomain(meet->target_meet, &anc, &offs)
	  && !KheTimeGroupDomainsAllowAssignment(tg, anc->time_domain,
	    meet->target_offset + offs) )
      return false;
    HaArrayForEach(meet->assigned_meets, cm, i)
      if( !KheMeetDomainAllowsAssignment(cm, tg, cm->target_offset) )
	return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetSetAutoDomain(KHE_MEET meet, bool automatic)                 */
/*                                                                           */
/*  Set meet to automatic, or away from it, depending on automatic.          */
/*                                                                           */
/*****************************************************************************/

bool KheMeetSetAutoDomain(KHE_MEET meet, bool automatic)
{
  if( (meet->time_domain == NULL) != automatic )
  {
    if( !KheMeetSetAutoDomainCheck(meet, automatic) )
      return false;
    KheMeetKernelSetAutoDomain(meet, automatic);
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntesectMeetDescendants(KHE_MEET meet, int offset,               */
/*    KHE_DOMAIN_STATE *state, KHE_TIME_GROUP *domain)                       */
/*                                                                           */
/*  Accumulate the intersection of the domains of the descendants of meet,   */
/*  adjusted by offset, in *domain, depending on *state as follows:          */
/*                                                                           */
/*    KHE_DOMAIN_INIT: no domains have been encountered yet.                 */
/*                                                                           */
/*    KHE_DOMAIN_SINGLE: at least one domain has been encounted, and one     */
/*    of those domains is a subset of all the others; *domain is that one.   */
/*                                                                           */
/*    KHE_DOMAIN_MULTI: neither of the above cases applies.  A time group    */
/*    is under construction by KheSolnTimeGroupBegin etc.                    */
/*                                                                           */
/*  Parameter *domain is really undefined if *state != KHE_DOMAIN_SINGLE;    */
/*  but for sanity its value is set to NULL in that case.                    */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_DOMAIN_INIT,
  KHE_DOMAIN_SINGLE,
  KHE_DOMAIN_MULTI
} KHE_DOMAIN_STATE;

static void KheIntesectMeetDescendants(KHE_MEET meet, int offset,
  KHE_DOMAIN_STATE *state, KHE_TIME_GROUP *domain)
{
  int i;  KHE_MEET child_meet;  KHE_TIME_GROUP tg;
  if( meet->time_domain == NULL )
  {
    HaArrayForEach(meet->assigned_meets, child_meet, i)
      KheIntesectMeetDescendants(child_meet,
	offset - child_meet->target_offset, state, domain);
  }
  else
  {
    tg = KheTimeGroupNeighbour(meet->time_domain, offset);
    switch( *state )
    {
      case KHE_DOMAIN_INIT:

	*domain = tg;
	*state = KHE_DOMAIN_SINGLE;
	break;

      case KHE_DOMAIN_SINGLE:

	if( KheTimeGroupSubset(tg, *domain) )
	  *domain = tg;
	else if( !KheTimeGroupSubset(*domain, tg) )
	{
	  KheSolnTimeGroupBegin(meet->soln);
	  KheSolnTimeGroupUnion(meet->soln, *domain);
	  KheSolnTimeGroupIntersect(meet->soln, tg);
	  *state = KHE_DOMAIN_MULTI;
	  *domain = NULL;
	}
	break;

      case KHE_DOMAIN_MULTI:

	KheSolnTimeGroupIntersect(meet->soln, tg);
	break;

      default:
	HnAbort("KheIntesectMeetDescendants internal error");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheMeetDescendantsDomain(KHE_MEET meet)                   */
/*                                                                           */
/*  Return the time group which is the intersection of the domains of the    */
/*  meets assigned to meet, or the full group if none.                       */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheMeetDescendantsDomain(KHE_MEET meet)
{
  KHE_DOMAIN_STATE state;  KHE_TIME_GROUP domain;
  state = KHE_DOMAIN_INIT;
  domain = NULL;
  KheIntesectMeetDescendants(meet, 0, &state, &domain);
  switch( state )
  {
    case KHE_DOMAIN_INIT:

      return KheInstanceFullTimeGroup(KheSolnInstance(meet->soln));

    case KHE_DOMAIN_SINGLE:

      return domain;

    case KHE_DOMAIN_MULTI:

      return KheSolnTimeGroupEnd(meet->soln);

    default:

      HnAbort("KheMeetDescendantsDomain internal error");
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "tasks"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAssignPreassignedResources(KHE_MEET meet,                    */
/*    KHE_RESOURCE_TYPE rt)                                                  */
/*                                                                           */
/*  Assign preassigned resources to those tasks of meet that have them and   */
/*  are not already assigned.  If rt != NULL, do this only for tasks of      */
/*  type rt.                                                                 */
/*                                                                           */
/*  A task is considered to have a preassigned resource if it is derived     */
/*  from an event resource with a preassigned resource.                      */
/*                                                                           */
/*****************************************************************************/

void KheMeetAssignPreassignedResources(KHE_MEET meet, KHE_RESOURCE_TYPE rt)
{
  KHE_TASK task;  KHE_RESOURCE r;  int i;  KHE_EVENT_RESOURCE er;
  HaArrayForEach(meet->tasks, task, i)
    if( (rt==NULL || KheTaskResourceType(task)==rt) && KheTaskAsst(task)==NULL )
    {
      er = KheTaskEventResource(task);
      if( er != NULL )
      {
	r = KheEventResourcePreassignedResource(er);
	if( r != NULL && !KheTaskAssignResource(task, r) )
	  HnAbort("KheMeetAssignPreassignedResources internal error");
      }
    }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAddTask(KHE_MEET meet, KHE_TASK task, bool add_demand)       */
/*                                                                           */
/*  Add task to meet, including setting its meet and meet_index fields.  If  */
/*  add_demand is true, this is a genuinely new task (not a fragment of a    */
/*  split), so update the demand in meet and its ancestors.                  */
/*                                                                           */
/*****************************************************************************/

void KheMeetAddTask(KHE_MEET meet, KHE_TASK task, bool add_demand)
{
  KHE_MEET prnt;  int pos;
  HnAssert(meet->time_domain != NULL,
    "KheMeetAddTask: meet has automatic domain");
  KheTaskSetMeet(task, meet);
  KheTaskSetMeetIndex(task, HaArrayCount(meet->tasks));
  if( DEBUG16 && HaArrayContains(meet->tasks, task, &pos) )
    HnAbort("KheMeetAddTask: task already present");
  HaArrayAddLast(meet->tasks, task);
  if( DEBUG20 )
  {
    fprintf(stderr, "KheMeetAddTask(%p ", (void *) meet);
    KheMeetDebug(meet, 1, -1, stderr);
    fprintf(stderr, ", %p ", (void *) task);
    KheTaskDebug(task, 1, -1, stderr);
    fprintf(stderr, ", %s) %d tasks after\n", add_demand ? "true" : "false",
      HaArrayCount(meet->tasks));
    KheMeetTestTasks(meet);
  }
  if( add_demand )
    for( prnt = meet;  prnt != NULL;  prnt = prnt->target_meet )
      prnt->all_demand += meet->duration;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDeleteTask(KHE_MEET meet, int task_index)                    */
/*                                                                           */
/*  Delete from meet the soln resource at task_index.                        */
/*                                                                           */
/*  This function is not called when splitting or merging; it represents     */
/*  a genuine deletion, so update the demand in meet and its ancestors.      */
/*                                                                           */
/*****************************************************************************/

void KheMeetDeleteTask(KHE_MEET meet, int task_index)
{
  KHE_MEET prnt;  KHE_TASK task;  int i;
  for( i = task_index;  i < HaArrayCount(meet->tasks) - 1;  i++ )
  {
    task = HaArray(meet->tasks, i + 1);
    KheTaskSetMeetIndex(task, i);
    HaArrayPut(meet->tasks, i, task);
  }
  HaArrayDeleteLast(meet->tasks);
  for( prnt = meet;  prnt != NULL;  prnt = prnt->target_meet )
    prnt->all_demand -= meet->duration;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetTaskCount(KHE_MEET meet)                                      */
/*                                                                           */
/*  Return the number of solution resources in meet.                         */
/*                                                                           */
/*****************************************************************************/

int KheMeetTaskCount(KHE_MEET meet)
{
  return HaArrayCount(meet->tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheMeetTask(KHE_MEET meet, int i)                               */
/*                                                                           */
/*  Return the i'th solution resource of meet.                               */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheMeetTask(KHE_MEET meet, int i)
{
  return HaArray(meet->tasks, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetRetrieveTask(KHE_MEET meet, char *role, KHE_TASK *task)      */
/*                                                                           */
/*  Retrieve the first task of meet whose event resource exists and has the  */
/*  given role.                                                              */
/*                                                                           */
/*****************************************************************************/

bool KheMeetRetrieveTask(KHE_MEET meet, char *role, KHE_TASK *task)
{
  KHE_TASK task1;  KHE_EVENT_RESOURCE er;  int i;
  HaArrayForEach(meet->tasks, task1, i)
  {
    er = KheTaskEventResource(task1);
    if( er != NULL && KheEventResourceRole(er) != NULL &&
	strcmp(KheEventResourceRole(er), role) == 0 )
    {
      *task = task1;
      return true;
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/* bool KheMeetFindTask(KHE_MEET meet, KHE_EVENT_RESOURCE er, KHE_TASK *task)*/
/*                                                                           */
/*  Similar to KheMeetRetrieveTask, but it searches for a task with a given  */
/*  event resource, rather than with an event resource with a given role.    */
/*                                                                           */
/*****************************************************************************/

bool KheMeetFindTask(KHE_MEET meet, KHE_EVENT_RESOURCE er, KHE_TASK *task)
{
  KHE_TASK task1;  int i;
  HaArrayForEach(meet->tasks, task1, i)
    if( KheTaskEventResource(task1) == er )
    {
      *task = task1;
      return true;
    }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetContainsResourcePreassignment(KHE_MEET meet,                 */
/*    KHE_RESOURCE r, KHE_TASK *task)                                        */
/*                                                                           */
/*  If meet contains a task preassigned r, return true and set *task to      */
/*  that task.                                                               */
/*                                                                           */
/*****************************************************************************/

bool KheMeetContainsResourcePreassignment(KHE_MEET meet,
  KHE_RESOURCE r, KHE_TASK *task)
{
  KHE_TASK task1;  KHE_RESOURCE r1;  int i;
  HaArrayForEach(meet->tasks, task1, i)
    if( KheTaskIsPreassigned(task1, &r1) && r1 == r )
    {
      *task = task1;
      return true;
    }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetContainsResourceAssignment(KHE_MEET meet, KHE_RESOURCE r,    */
/*    KHE_TASK *task)                                                        */
/*                                                                           */
/*  If meet contains a task assigned r, return true and set *task to         */
/*  that task.                                                               */
/*                                                                           */
/*****************************************************************************/

bool KheMeetContainsResourceAssignment(KHE_MEET meet, KHE_RESOURCE r,
  KHE_TASK *task)
{
  KHE_TASK task1;  int i;
  HaArrayForEach(meet->tasks, task1, i)
    if( KheTaskAsstResource(task1) == r )
    {
      *task = task1;
      return true;
    }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetPartitionTaskCount(KHE_MEET meet, int offset,                */
/*    KHE_RESOURCE_GROUP partition, int *count)                              */
/*                                                                           */
/*  For each task running at the given offset in meet and in the meets       */
/*  assigned to it, directly or indirectly, whose domains lie in partition,  */
/*  add 1 to *count.                                                         */
/*                                                                           */
/*****************************************************************************/

void KheMeetPartitionTaskCount(KHE_MEET meet, int offset,
  KHE_RESOURCE_GROUP partition, int *count)
{
  KHE_TASK task;  int i, child_offset;  KHE_MEET child_meet;

  /* do the job for the tasks of meet itself */
  HaArrayForEach(meet->tasks, task, i)
    if( KheResourceGroupPartition(KheTaskDomain(task)) == partition )
      (*count)++;

  /* do the job for the tasks of the overlapping meets assigned to meet */
  HaArrayForEach(meet->assigned_meets, child_meet, i)
  {
    child_offset = KheMeetAsstOffset(child_meet);
    if( child_offset <= offset &&
        child_offset + KheMeetDuration(child_meet) > offset )
      KheMeetPartitionTaskCount(child_meet, offset - child_offset,
	partition, count);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetPartitionTaskCount(KHE_MEET meet, int offset,                */
/*    KHE_RESOURCE_GROUP partition, int *count)                              */
/*                                                                           */
/*  For each task running at the given offset in meet and in the meets       */
/*  assigned to it, directly or indirectly, whose domains lie in partition,  */
/*  add 1 to *count.                                                         */
/*                                                                           */
/*****************************************************************************/

void KheMeetPartitionTaskDebug(KHE_MEET meet, int offset,
  KHE_RESOURCE_GROUP partition, int verbosity, int indent, FILE *fp)
{
  KHE_TASK task;  int i, child_offset;  KHE_MEET child_meet;

  /* do the job for the tasks of meet itself */
  HaArrayForEach(meet->tasks, task, i)
    if( KheResourceGroupPartition(KheTaskDomain(task)) == partition )
      KheMeetDebug(meet, 1, indent, fp);

  /* do the job for the tasks of the overlapping meets assigned to meet */
  HaArrayForEach(meet->assigned_meets, child_meet, i)
  {
    child_offset = KheMeetAsstOffset(child_meet);
    if( child_offset <= offset &&
        child_offset + KheMeetDuration(child_meet) > offset )
      KheMeetPartitionTaskDebug(child_meet, offset - child_offset,
	partition, verbosity, indent, fp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "nodes"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE KheMeetNode(KHE_MEET meet)                                      */
/*                                                                           */
/*  Return the node containing meet, or NULL if none.                        */
/*                                                                           */
/*****************************************************************************/

KHE_NODE KheMeetNode(KHE_MEET meet)
{
  return meet->node;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetNode(KHE_MEET meet, KHE_NODE node)                        */
/*                                                                           */
/*  Set the node attribute of meet.  This is used internally, unchecked.     */
/*  NB changing the node means that all zones are lost.                      */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetNode(KHE_MEET meet, KHE_NODE node)
{
  int offset;  KHE_ZONE zone;
  if( node != meet->node )
  {
    HaArrayForEachReverse(meet->zones, zone, offset)
      if( zone != NULL )
        KheZoneDeleteMeetOffset(zone, meet, offset);
    meet->node = node;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetNodeIndex(KHE_MEET meet)                                      */
/*                                                                           */
/*  Set the node_index attribute of meet.                                    */
/*                                                                           */
/*****************************************************************************/

int KheMeetNodeIndex(KHE_MEET meet)
{
  return meet->node_index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetSetNodeIndex(KHE_MEET meet, int node_index)                  */
/*                                                                           */
/*  Set the index of meet in its node.                                       */
/*                                                                           */
/*****************************************************************************/

void KheMeetSetNodeIndex(KHE_MEET meet, int node_index)
{
  meet->node_index = node_index;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetAddNodeCheck(KHE_MEET meet, KHE_NODE node)                   */
/*                                                                           */
/*  Check whether adding node to meet is safe.                               */
/*                                                                           */
/*****************************************************************************/

bool KheMeetAddNodeCheck(KHE_MEET meet, KHE_NODE node)
{
  /* meet must not be already assigned to a node */
  if( DEBUG14 && meet->node != NULL )
    fprintf(stderr, "  KheMeetAddNodeCheck(%p, %p) failing: meet->node = %p\n",
      (void *) meet, (void *) node, (void *) meet->node);
  HnAssert(meet->node == NULL,
    "KheNodeAddMeetCheck: meet already lies in a node");

  /* check the node rule at meet */
  if( meet->target_meet != NULL )
  {
    /* it will become true that "meet meet lies in node and is  */
    /* assigned to target_meet", therefore it must be true */
    /* that "node has a parent and target_meet lies in that parent." */
    if( KheNodeParent(node) == NULL ||
	KheMeetNode(meet->target_meet) != KheNodeParent(node) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetDeleteNodeCheck(KHE_MEET meet, KHE_NODE node)                */
/*                                                                           */
/*  Check whether it is safe to delete meet from node.                       */
/*                                                                           */
/*****************************************************************************/

bool KheMeetDeleteNodeCheck(KHE_MEET meet, KHE_NODE node)
{
  KHE_MEET child_meet;  int i;

  /* meet must be already assigned to node */
  if( meet->node != node )
    HnAbort("KheNodeDeleteMeetCheck: meet does not lie in node");

  /* check the node rule at each child of meet */
  HaArrayForEach(meet->assigned_meets, child_meet, i)
    if( child_meet->node != NULL )
    {
      /* at this point we have "meet child_meet lies in node and  */
      /* is assigned to meet meet", therefore it must be true     */
      /* that "node has a parent" - but we are trying to delete that node */
      return false;
    }

  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "zones"                                                        */
/*                                                                           */
/*  Implementation note.  Logically there is one entry in the zones array    */
/*  for each offset of meet, but this implementation omits NULL entries at   */
/*  the end of the array.  This saves a lot of space in cycle meets when     */
/*  there are no zones.                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMeetOffsetAddZone(KHE_MEET meet, int offset, KHE_ZONE zone)      */
/*                                                                           */
/*  Add zone to meet at offset.                                              */
/*                                                                           */
/*****************************************************************************/

void KheMeetOffsetAddZone(KHE_MEET meet, int offset, KHE_ZONE zone)
{
  if( zone != NULL )
  {
    HaArrayFill(meet->zones, offset + 1, NULL);
    HaArrayPut(meet->zones, offset, zone);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetOffsetDeleteZone(KHE_MEET meet, int offset)                  */
/*                                                                           */
/*  Delete the zone of meet at offset.                                       */
/*                                                                           */
/*****************************************************************************/

void KheMeetOffsetDeleteZone(KHE_MEET meet, int offset)
{
  HnAssert(offset < HaArrayCount(meet->zones),
    "KheMeetOffsetDeleteZone internal error");
  HaArrayPut(meet->zones, offset, NULL);
  while( HaArrayCount(meet->zones) > 0 && HaArrayLast(meet->zones) == NULL )
    HaArrayDeleteLast(meet->zones);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ZONE KheMeetOffsetZone(KHE_MEET meet, int offset)                    */
/*                                                                           */
/*  Return the zone of meet at offset (possibly NULL).                       */
/*                                                                           */
/*****************************************************************************/

KHE_ZONE KheMeetOffsetZone(KHE_MEET meet, int offset)
{
  HnAssert(offset >= 0 && offset < KheMeetDuration(meet),
    "KheMeetOffsetZone: offset (%d) out of range (0 .. %d)",
    offset, KheMeetDuration(meet) - 1);
  return (offset < HaArrayCount(meet->zones) ? HaArray(meet->zones, offset)
    : NULL);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "file reading and writing"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetMakeFromKml(KML_ELT meet_elt, KHE_SOLN soln, KML_ERROR *ke)  */
/*                                                                           */
/*  Make a meet based on meet_elt and add it to soln.                        */
/*                                                                           */
/*****************************************************************************/

bool KheMeetMakeFromKml(KML_ELT meet_elt, KHE_SOLN soln, KML_ERROR *ke)
{
  KML_ELT duration_elt, time_elt, resources_elt, resource_elt;
  KHE_EVENT event;  KHE_TIME time, preassigned_time;
  int duration, j;  char *ref;  KHE_MEET meet;  HA_ARENA a;
  a = KheSolnArena(soln);
  if( !KmlCheck(meet_elt, "Reference : +#Duration +Time +Resources", ke) )
    return false;

  /* reference (must be present in instance) */
  ref = KmlAttributeValue(meet_elt, 0);
  if( DEBUG3 )
    fprintf(stderr, "[ KheMeetMakeFromKml(%s, soln, -)\n", ref);
  if( !KheInstanceRetrieveEvent(KheSolnInstance(soln), ref, &event) )
    return KmlError(ke, a, KmlLineNum(meet_elt),
      KmlColNum(meet_elt), "<Event> Reference \"%s\" unknown", ref);

  /* Duration and event duration */
  if( KmlContainsChild(meet_elt, "Duration", &duration_elt) )
    sscanf(KmlText(duration_elt), "%d", &duration);
  else
    duration = KheEventDuration(event);

  /* make and add meet */
  meet = KheMeetMake(soln, duration, event);

  /* Time */
  if( KmlContainsChild(meet_elt, "Time", &time_elt) )
  {
    if( !KmlCheck(time_elt, "Reference", ke) )
      return false;
    ref = KmlAttributeValue(time_elt, 0);
    if( !KheInstanceRetrieveTime(KheSolnInstance(soln), ref, &time) )
      return KmlError(ke, a, KmlLineNum(time_elt), KmlColNum(time_elt),
	"<Time> Reference \"%s\" unknown", ref);
    preassigned_time = KheEventPreassignedTime(event);
    if( preassigned_time != NULL && preassigned_time != time )
    {
      return KmlError(ke, a, KmlLineNum(time_elt), KmlColNum(time_elt),
	"<Time> \"%s\" conflicts with preassigned time \"%s\"",
	ref, KheTimeId(preassigned_time));
    }
    if( !KheMeetAssignTime(meet, time) )
      return KmlError(ke, a, KmlLineNum(time_elt), KmlColNum(time_elt),
	"<Time> \"%s\" not assignable to <Event> \"%s\"",
	KheTimeId(time), KheEventId(event));
    if( DEBUG3 )
      fprintf(stderr, "  assigned time %s to %s\n", KheTimeId(time), ref);
  }

  /* Resources */
  if( KmlContainsChild(meet_elt, "Resources", &resources_elt) )
  {
    if( !KmlCheck(resources_elt, ": *Resource", ke) )
      return false;
    for( j = 0;  j < KmlChildCount(resources_elt);  j++ )
    {
      resource_elt = KmlChild(resources_elt, j);
      if( !KheTaskMakeFromKml(resource_elt, meet, ke) )
	return false;
    }
  }
  if( DEBUG3 )
    fprintf(stderr, "] KheMeetMakeFromKml returning\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMeetAssignedTimeCmp(const void *t1, const void *t2)               */
/*                                                                           */
/*  Comparison function for sorting an array of meets by                     */
/*  increasing assigned time.  Unassigned meets go at the end.               */
/*                                                                           */
/*****************************************************************************/

int KheMeetAssignedTimeCmp(const void *t1, const void *t2)
{
  KHE_MEET meet1 = * (KHE_MEET *) t1;
  KHE_MEET meet2 = * (KHE_MEET *) t2;
  KHE_TIME time1 = KheMeetAsstTime(meet1);
  KHE_TIME time2 = KheMeetAsstTime(meet2);
  if( time1 == time2 )
    return KheMeetSolnIndex(meet1) - KheMeetSolnIndex(meet2);
  else
    return time1 == NULL ? 1 : time2 == NULL ? -1 :
      KheTimeIndex(time1) - KheTimeIndex(time2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetCheckForWriting(KHE_MEET meet)                               */
/*                                                                           */
/*  Check that meet can be written safely; abort with error message if not.  */
/*                                                                           */
/*  This function does not check that meet's tasks can be written safely.    */
/*  It is better to check that just before writing them, so that if we       */
/*  do generate an incomplete file it stops as close as possible to the      */
/*  point of error.                                                          */
/*                                                                           */
/*****************************************************************************/

void KheMeetCheckForWriting(KHE_MEET meet)
{
  KHE_TIME ass_t, pre_t;  KHE_EVENT e;  KHE_INSTANCE ins;

  /* check that meet is derived from an event with an Id */
  HnAssert(meet->event_in_soln != NULL, "KheMeetWrite: internal error");
  e = KheMeetEvent(meet);
  ins = KheEventInstance(e);
  HnAssert(KheEventId(e) != NULL,
    "KheArchiveWrite: in instance %s, event without Id", KheInstanceId(ins));

  /* get assigned and preassigned times and check that they have Ids */
  ass_t = KheMeetAsstTime(meet);
  pre_t = KheEventPreassignedTime(e);
  HnAssert(ass_t == NULL || KheTimeId(ass_t) != NULL,
    "KheArchiveWrite: in instance %s, time without Id assigned to event %s",
    KheInstanceId(ins), KheEventId(e));
  HnAssert(pre_t == NULL || KheTimeId(pre_t) != NULL,
    "KheArchiveWrite: in instance %s, time without Id preassigned to event %s",
    KheInstanceId(ins), KheEventId(e));

  /* if preassigned time, check that the assigned time is equal to it */
  if( pre_t != NULL )
  {
    HnAssert(ass_t != NULL,
      "KheArchiveWrite: in instance %s, event %s with preassigned time %s"
      " has meet with missing time assignment",
      KheInstanceId(ins), KheEventId(e), KheTimeId(pre_t));
    HnAssert(ass_t == pre_t,
      "KheArchiveWrite: in instance %s, event %s with preassigned time %s"
      " has meet with inconsistent time assignment %s",
      KheInstanceId(ins), KheEventId(e), KheTimeId(pre_t), KheTimeId(ass_t));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetMustWrite(KHE_MEET meet)                                     */
/*                                                                           */
/*  Return true if it is necessary to write meet, either because its         */
/*  duration differs from the duration of the corresponding instance         */
/*  event, or it is assigned a time which is not a preassigned time, or      */
/*  it is necessary to write at least one of its solution resources.         */
/*                                                                           */
/*  This function assumes that KheMeetCheckForWriting returned successfully. */
/*                                                                           */
/*****************************************************************************/

bool KheMeetMustWrite(KHE_MEET meet)
{
  KHE_TIME ass_t, pre_t;  KHE_TASK task;  int i;  KHE_EVENT e;

  /* check duration */
  e = KheMeetEvent(meet);
  if( meet->duration != KheEventDuration(e) )
    return true;

  /* check assigned and preassigned times */
  ass_t = KheMeetAsstTime(meet);
  pre_t = KheEventPreassignedTime(e);
  if( ass_t != NULL && ass_t != pre_t )
    return true;

  /* check tasks */
  HaArrayForEach(meet->tasks, task, i)
    if( KheTaskMustWrite(task) )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetWrite(KHE_MEET meet, KML_FILE kf)                            */
/*                                                                           */
/*  Write meet to kf.                                                        */
/*                                                                           */
/*  This function assumes that KheMeetCheckForWriting and KheMeetMustWrite   */
/*  returned successfully.                                                   */
/*                                                                           */
/*****************************************************************************/

void KheMeetWrite(KHE_MEET meet, KML_FILE kf)
{
  KHE_TIME ass_t, pre_t;  KHE_TASK task;  int i;  bool started_resources;
  KHE_EVENT e;

  /* print event header, and duration if required */
  e = KheMeetEvent(meet);
  KmlBegin(kf, "Event");
  KmlAttribute(kf, "Reference", KheEventId(e));
  if( meet->duration != KheEventDuration(e) )
    KmlEltFmtText(kf, "Duration", "%d", meet->duration);

  /* print assigned time if present and different from preassigned */
  ass_t = KheMeetAsstTime(meet);
  pre_t = KheEventPreassignedTime(e);
  if( ass_t != NULL && ass_t != pre_t )
    KmlEltAttribute(kf, "Time", "Reference", KheTimeId(ass_t));

  /* print resources */
  started_resources = false;
  HaArrayForEach(meet->tasks, task, i)
  {
    KheTaskCheckForWriting(task);
    if( KheTaskMustWrite(task) )
    {
      if( !started_resources )
      {
	KmlBegin(kf, "Resources");
	started_resources = true;
      }
      KheTaskWrite(task, kf);
    }
  }
  if( started_resources )
    KmlEnd(kf, "Resources");
  KmlEnd(kf, "Event");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *KheMeetId(KHE_MEET meet)                                           */
/*                                                                           */
/*  Return the Id of meet, first creating it if it does not exist yet.       */
/*                                                                           */
/*****************************************************************************/

char *KheMeetId(KHE_MEET meet)
{
  KHE_EVENT e;  KHE_TIME t;  int i;  HA_ARENA a;  char *e_id;
  if( meet->id == NULL )
  {
    a = KheSolnArena(meet->soln);
    if( KheMeetIsCycleMeet(meet) )
    {
      t = KheMeetAsstTime(meet);
      if( KheTimeId(t) != NULL )
	meet->id = HnStringMake(a, "/%s/", KheTimeId(t));
      else
        meet->id = HnStringMake(a, "/%d/", meet->assigned_time_index);
    }
    else if( meet->event_in_soln != NULL )
    {
      e = KheEventInSolnEvent(meet->event_in_soln);
      e_id = KheEventId(e);
      if( e_id == NULL )
        e_id = "-";
      if( KheEventMeetCount(meet->soln, e) == 1 )
        meet->id = HnStringMake(a, "%s", e_id);
      else
      {
	for( i = 0;  i < KheEventMeetCount(meet->soln, e);  i++ )
	  if( KheEventMeet(meet->soln, e, i) == meet )
	    break;
	meet->id = HnStringMake(a, "%s:%d", e_id, i);
      }
    }
    else
      meet->id = HnStringMake(a, "/s%d/", meet->soln_index);
  }
  return meet->id;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDebugName(KHE_MEET meet, FILE *fp)                           */
/*                                                                           */
/*  Debug print of the name of meet onto fp.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMeetDebugName(KHE_MEET meet, FILE *fp)
{
  KHE_EVENT e;  KHE_TIME t;  int j;
  if( KheMeetIsCycleMeet(meet) )
  {
    t = KheMeetAsstTime(meet);
    if( KheTimeId(t) != NULL )
      fprintf(fp, "/%s/", KheTimeId(t));
    else
      fprintf(fp, "/%d/", meet->assigned_time_index);
  }
  else if( meet->event_in_soln != NULL )
  {
    e = KheEventInSolnEvent(meet->event_in_soln);
    for( j = 0;  j < KheEventMeetCount(meet->soln, e);  j++ )
      if( KheEventMeet(meet->soln, e, j) == meet )
	break;
    fprintf(fp, "%s", KheEventId(e) != NULL ? KheEventId(e) : "-");
    if( KheEventMeetCount(meet->soln, e) > 1 )
      fprintf(fp, ":%d", j < KheEventMeetCount(meet->soln, e) ? j : -1);
  }
  else
    fprintf(fp, "#%d#", meet->soln_index);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetHasZones(KHE_MEET meet)                                      */
/*                                                                           */
/*  Return true if meet has any non-NULL zones.                              */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetHasZones(KHE_MEET meet)
{
  return HaArrayCount(meet->zones) > 0;
  /* ***
  int i;  KHE_ZONE zone;
  HaArrayForEach(meet->zones, zone, i)
    if( zone != NULL )
      return true;
  return false;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDebug(KHE_MEET meet, int verbosity, int indent, FILE *fp)    */
/*                                                                           */
/*  Debug print of meet onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

void KheMeetDebug(KHE_MEET meet, int verbosity, int indent, FILE *fp)
{
  KHE_ZONE zone;  int i;  KHE_TASK task;  KHE_MEET meet2;
  HnAssert(meet->soln != NULL, "KheMeetDebug internal error");
  KheMeetCheck(meet);
  if( indent < 0 )
    fprintf(fp, "%s", KheMeetId(meet));
  else if( verbosity == 1 )
    fprintf(fp, "%*s%s\n", indent, "", KheMeetId(meet));
  else
  {
    /* header */
    fprintf(fp, "%*s[ Meet %s(durn %d, children %d, tasks %d)\n", indent, "",
      KheMeetId(meet), meet->duration, HaArrayCount(meet->assigned_meets),
      HaArrayCount(meet->tasks));

    /* time domain */
    if( meet->time_domain == NULL )
    {
      fprintf(fp, "%*s  auto domain: ", indent, "");
      KheTimeGroupDebug(KheMeetDescendantsDomain(meet), 2, 0, fp);
    }
    else
    {
      fprintf(fp, "%*s  domain: ", indent, "");
      KheTimeGroupDebug(meet->time_domain, 2, 0, fp);
    }

    /* zones (optional) */
    if( verbosity >= 3 && KheMeetHasZones(meet) )
    {
      fprintf(fp, "%*s  zones: ", indent, "");
      HaArrayForEach(meet->zones, zone, i)
      {
	if( i > 0 )
	  fprintf(fp, ":");
	if( zone == NULL )
	  fprintf(fp, "_");
	else
	  fprintf(fp, "%d", KheZoneNodeIndex(zone));
      }
      fprintf(fp, "\n");
    }

    /* print arrows and ancestors (names only) */
    if( meet->target_meet != NULL )
    {
      fprintf(fp, "%*s  -%d-> %s\n", indent, "", meet->target_offset,
	KheMeetId(meet->target_meet));
      /* KheMeetDebug(meet->target_meet, verbosity, indent + 2, fp); */
    }

    /* print tasks */
    if( verbosity >= 3 )
      HaArrayForEach(meet->tasks, task, i)
	KheTaskDebug(task, verbosity, indent + 2, fp);

    if( verbosity >= 3 )
      HaArrayForEach(meet->assigned_meets, meet2, i)
        KheMeetDebug(meet2, verbosity, indent + 2, fp);

    /* all done */
    fprintf(fp, "%*s]\n", indent, "");
  }
}

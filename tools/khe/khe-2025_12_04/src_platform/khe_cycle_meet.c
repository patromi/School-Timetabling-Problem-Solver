
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
/*  FILE:         khe_cycle_meet.c                                           */
/*  DESCRIPTION:  KheSplitCycleMeet()                                        */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_CLASS - an equivalence class of events                         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_event_class_rec *KHE_EVENT_CLASS;
typedef HA_ARRAY(KHE_EVENT_CLASS) ARRAY_KHE_EVENT_CLASS;

typedef HA_ARRAY(KHE_PREFER_TIMES_CONSTRAINT) ARRAY_KHE_PREFER_TIMES_CONSTRAINT;

struct khe_event_class_rec {
  KHE_EVENT_CLASS			parent_class;	/* parent class      */
  ARRAY_KHE_EVENT			events;		/* events in class   */
  ARRAY_KHE_PREFER_TIMES_CONSTRAINT	constraints;	/* class constraints */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "event classes"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_CLASS KheEventClassMake(KHE_EVENT e, HA_ARENA a)               */
/*                                                                           */
/*  Make an event class containing just e.                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_EVENT_CLASS KheEventClassMake(KHE_EVENT e, HA_ARENA a)
{
  KHE_EVENT_CLASS res;
  HaMake(res, a);
  res->parent_class = NULL;
  HaArrayInit(res->events, a);
  HaArrayAddLast(res->events, e);
  HaArrayInit(res->constraints, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventClassDelete(KHE_EVENT_CLASS ec)                             */
/*                                                                           */
/*  Delete ec.                                                               */
/*                                                                           */
/*****************************************************************************/

/* *** stil l to do
static void KheEventClassDelete(KHE_EVENT_CLASS ec)
{
  MArrayFree(ec->events);
  MArrayFree(ec->constraints);
  MFree(ec);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_CLASS KheEventClassRoot(KHE_EVENT_CLASS ec)                    */
/*                                                                           */
/*  Return the root class of ec.                                             */
/*                                                                           */
/*****************************************************************************/

static KHE_EVENT_CLASS KheEventClassRoot(KHE_EVENT_CLASS ec)
{
  while( ec->parent_class != NULL )
    ec = ec->parent_class;
  return ec;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventClassMerge(KHE_EVENT_CLASS ec1, KHE_EVENT_CLASS ec2)        */
/*                                                                           */
/*  Merge ec2 into ec1, unless they already have the same root class.        */
/*                                                                           */
/*****************************************************************************/

static void KheEventClassMerge(KHE_EVENT_CLASS ec1, KHE_EVENT_CLASS ec2)
{
  int i;
  ec1 = KheEventClassRoot(ec1);
  ec2 = KheEventClassRoot(ec2);
  if( ec1 != ec2 )
  {
    ec2->parent_class = ec1;
    HaArrayAppend(ec1->events, ec2->events, i);
    HnAssert(HaArrayCount(ec2->constraints) == 0,
      "KheEventClassMerge internal error");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventClassSubEventDurationRange(KHE_EVENT_CLASS ec,              */
/*    int *min_duration, int *max_duration)                                  */
/*                                                                           */
/*  Return the range of durations permitted to the events of ec by their     */
/*  durations and split events constraints.                                  */
/*                                                                           */
/*****************************************************************************/

static void KheEventClassSubEventDurationRange(KHE_EVENT_CLASS ec,
  int *min_duration, int *max_duration)
{
  KHE_CONSTRAINT c;  KHE_SPLIT_EVENTS_CONSTRAINT sec;  int i, j;
  KHE_EVENT e;

  /* initialize *min_duration and *max_duration based on event durations */
  *min_duration = 1;
  *max_duration = 0;
  HaArrayForEach(ec->events, e, i)
    if( KheEventDuration(e) > *max_duration )
      *max_duration = KheEventDuration(e);

  /* modify *min_duration and *max_duration based on split events constraints */
  HaArrayForEach(ec->events, e, i)
    for( j = 0;  j < KheEventConstraintCount(e);  j++ )
    {
      c = KheEventConstraint(e, j);
      if( KheConstraintTag(c) == KHE_SPLIT_EVENTS_CONSTRAINT_TAG &&
	  KheConstraintRequired(c) && KheConstraintWeight(c) > 0 )
      {
	sec = KheToSplitEventsConstraint(c);
	if( KheSplitEventsConstraintMinDuration(sec) > *min_duration )
	  *min_duration = KheSplitEventsConstraintMinDuration(sec);
	if( KheSplitEventsConstraintMaxDuration(sec) < *max_duration )
	  *max_duration = KheSplitEventsConstraintMaxDuration(sec);
      }
    }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventClassHasPreassignedTime(KHE_EVENT_CLASS ec, KHE_TIME *t)    */
/*                                                                           */
/*  Return true if ec's events contain at least one preassigned time among   */
/*  them, setting *t to that time.                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheEventClassHasPreassignedTime(KHE_EVENT_CLASS ec, KHE_TIME *t)
{
  KHE_EVENT e;  int i;
  HaArrayForEach(ec->events, e, i)
  {
    *t = KheEventPreassignedTime(e);
    if( *t != NULL )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePreferTimesConstraintTimeIsLegal(                                */
/*    KHE_PREFER_TIMES_CONSTRAINT ptc, KHE_TIME t)                           */
/*                                                                           */
/*  Return true if t is a starting time for ptc.                             */
/*                                                                           */
/*****************************************************************************/

static bool KhePreferTimesConstraintTimeIsLegal(
  KHE_PREFER_TIMES_CONSTRAINT ptc, KHE_TIME t)
{
  int i, pos;  KHE_TIME time;  KHE_TIME_GROUP tg;
  for( i = 0;  i < KhePreferTimesConstraintTimeGroupCount(ptc);  i++ )
  {
    tg = KhePreferTimesConstraintTimeGroup(ptc, i);
    if( KheTimeGroupContains(tg, t, &pos) )
      return true;
  }
  for( i = 0;  i < KhePreferTimesConstraintTimeCount(ptc);  i++ )
  {
    time = KhePreferTimesConstraintTime(ptc, i);
    if( time == t )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventClassStartingTimeIsLegal(KHE_EVENT_CLASS ec,                */
/*    KHE_TIME t, int durn)                                                  */
/*                                                                           */
/*  Return true if ec allows a solution event of the given durn to start     */
/*  at time t.                                                               */
/*                                                                           */
/*****************************************************************************/

static bool KheEventClassStartingTimeIsLegal(KHE_EVENT_CLASS ec,
  KHE_TIME t, int durn)
{
  KHE_PREFER_TIMES_CONSTRAINT ptc;  int i, d;
  HaArrayForEach(ec->constraints, ptc, i)
  {
    d = KhePreferTimesConstraintDuration(ptc);
    if( d == durn || d == KHE_ANY_DURATION )
    {
      /* ptc is applicable; if it doesn't contain t, then return false */
      if( !KhePreferTimesConstraintTimeIsLegal(ptc, t) )
	return false;
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventClassInferNonBreaks(KHE_EVENT_CLASS ec,                     */
/*    HA_ARRAY_BOOL *break_after)                                            */
/*                                                                           */
/*  Infer from ec those times which cannot be followed by breaks, and set    */
/*  the elements of break_after corresponding to those times to false.       */
/*                                                                           */
/*****************************************************************************/

static void KheEventClassInferNonBreaks(KHE_EVENT_CLASS ec,
  HA_ARRAY_BOOL *break_after)
{
  KHE_TIME t, t2;  int min_duration, max_duration;
  KHE_INSTANCE ins;  int i, j, durn;
  KheEventClassSubEventDurationRange(ec, &min_duration, &max_duration);
  if( KheEventClassHasPreassignedTime(ec, &t) )
  {
    /* every starting time from t up to max_duration has no break */
    for( i = 0;  i < max_duration - 1;  i++ )
      if( KheTimeHasNeighbour(t, i) )
      {
        t2 = KheTimeNeighbour(t, i);
	if( DEBUG1 )
	  fprintf(stderr, "    no break at %s (preassigned)\n",
	    KheTimeId(t2) == NULL ? "-" : KheTimeId(t2));
	HaArrayPut(*break_after, KheTimeIndex(t2), false);
      }
  }
  else
  {
    /* every legal starting time up to max_duration has no break */
    if( min_duration <= max_duration && max_duration >= 2 )
    {
      ins = KheEventInstance(HaArrayFirst(ec->events));
      for( i = 0;  i < KheInstanceTimeCount(ins);  i++ )
      {
	t = KheInstanceTime(ins, i);
	for( durn = min_duration;  durn <= max_duration;  durn++ )
	  if( KheEventClassStartingTimeIsLegal(ec, t, durn) )
	    for( j = 0;  j < durn - 1;  j++ )
	      if( KheTimeHasNeighbour(t, j) )
	      {
		t2 = KheTimeNeighbour(t, j);
		if( DEBUG1 )
		  fprintf(stderr, "    no break at %s\n",
		    KheTimeId(t2) == NULL ? "-" : KheTimeId(t2));
		HaArrayPut(*break_after, KheTimeIndex(t2), false);
	      }
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventClassDebug(KHE_EVENT_CLASS ec, int verbosity, int indent,   */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of ec with the given verbosity and indent.                   */
/*                                                                           */
/*****************************************************************************/

static void KheEventClassDebug(KHE_EVENT_CLASS ec, int verbosity, int indent,
  FILE *fp)
{
  int i;  KHE_EVENT e;  KHE_PREFER_TIMES_CONSTRAINT ptc;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ Event Class\n", indent, "");
    HaArrayForEach(ec->events, e, i)
      KheEventDebug(e, 1, indent + 2, fp);
    HaArrayForEach(ec->constraints, ptc, i)
      KheConstraintDebug((KHE_CONSTRAINT) ptc, 1, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main function"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnSplitCycleMeet(KHE_SOLN soln)                                */
/*                                                                           */
/*  Split the cycle meet of soln at points allowed by soln's instance;       */
/*  or do nothing if there is no cycle meet.                                 */
/*                                                                           */
/*  This functions uses its own arena and deletes it at the end.             */
/*                                                                           */
/*****************************************************************************/

void KheSolnSplitCycleMeet(KHE_SOLN soln)
{
  ARRAY_KHE_EVENT_CLASS event_classes;  KHE_EVENT_CLASS ec1, ec2;
  KHE_EVENT_GROUP eg; KHE_EVENT e;  int i, j, k, pos, durn;
  KHE_CONSTRAINT c;  KHE_LINK_EVENTS_CONSTRAINT lec;  KHE_TIME t;
  KHE_PREFER_TIMES_CONSTRAINT ptc;  KHE_INSTANCE ins;  HA_ARENA a;
  HA_ARRAY_BOOL break_after;  bool break_here;  KHE_MEET cycle_meet, junk;

  ins = KheSolnInstance(soln);
  a = KheSolnArenaBegin(soln);
  if( DEBUG1 )
    fprintf(stderr, "[ KheSolnSplitCycleMeet(soln of %s)\n",
      KheInstanceId(ins));

  /* find the initial cycle meet and ensure that it is the only cycle meet */
  for( i = 0;  i < KheSolnMeetCount(soln);  i++ )
  {
    cycle_meet = KheSolnMeet(soln, i);
    if( KheMeetIsCycleMeet(KheSolnMeet(soln, i)) )
      break;
  }
  if( i >= KheSolnMeetCount(soln) )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheSolnSplitCycleMeet returning (no cycle meet)\n");
    return;
  }
  cycle_meet = KheSolnMeet(soln, i);
  HnAssert(KheMeetDuration(cycle_meet) == KheInstanceTimeCount(ins),
    "KheSolnSplitCycleMeet: no single initial cycle meet");

  /* initialize the break_after array to be true at every time */
  HaArrayInit(break_after, a);
  HaArrayFill(break_after, KheInstanceTimeCount(ins), true);

  /* build one event class for each event of ins */
  HaArrayInit(event_classes, a);
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    HaArrayAddLast(event_classes, KheEventClassMake(e, a));
  }

  /* merge classes whose events share required link events constraints */
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintTag(c) == KHE_LINK_EVENTS_CONSTRAINT_TAG &&
	KheConstraintRequired(c) && KheConstraintWeight(c) > 0 )
    {
      lec = (KHE_LINK_EVENTS_CONSTRAINT) c;
      for( j = 0;  j < KheLinkEventsConstraintEventGroupCount(lec);  j++ )
      {
	eg = KheLinkEventsConstraintEventGroup(lec, j);
	if( KheEventGroupEventCount(eg) >= 2 )
	{
	  e = KheEventGroupEvent(eg, 0);
	  ec1 = HaArray(event_classes, KheEventIndex(e));
	  for( k = 1;  k < KheEventGroupEventCount(eg);  k++ )
	  {
	    e = KheEventGroupEvent(eg, k);
	    ec2 = HaArray(event_classes, KheEventIndex(e));
            KheEventClassMerge(ec1, ec2);
	  }
	}
      }
    }
  }

  /* add required prefer times constraints to the root classes */
  /* and deduce which times don't have breaks                  */
  HaArrayForEach(event_classes, ec1, i)
    if( ec1->parent_class == NULL )
    {
      HaArrayForEach(ec1->events, e, j)
	for( k = 0;  k < KheEventConstraintCount(e);  k++ )
	{
	  c = KheEventConstraint(e, k);
	  if( KheConstraintTag(c) == KHE_PREFER_TIMES_CONSTRAINT_TAG &&
	      /* KheConstraintRequired(c) && */ KheConstraintWeight(c) > 0 )
	  {
	    ptc = (KHE_PREFER_TIMES_CONSTRAINT) c;
	    if( !HaArrayContains(ec1->constraints, ptc, &pos) )
	      HaArrayAddLast(ec1->constraints, ptc);
	  }
	}
      if( DEBUG2 )
        KheEventClassDebug(ec1, 1, 2, stderr);
      KheEventClassInferNonBreaks(ec1, &break_after);
    }

  /* split the initial cycle meet at the times indicated by break_after */
  HaArrayPut(break_after, HaArrayCount(break_after) - 1, false);
  durn = 0;
  HaArrayForEach(break_after, break_here, i)
  {
    durn++;
    if( break_here )
    {
      t = KheInstanceTime(ins, i);
      if( DEBUG2 )
      {
	fprintf(stderr, "  at %d (%s): calling KheMeetSplit(", i,
	  KheTimeId(t) == NULL ? "-" : KheTimeId(t));
	KheMeetDebug(cycle_meet, 1, -1, stderr);
	fprintf(stderr, ", %d, false, -, -)\n", durn);
      }
      if( !KheMeetSplit(cycle_meet, durn, false, &junk, &cycle_meet) )
	HnAbort("KheSolnSplitCycleMeet internal error");
      durn = 0;
    }
  }

  /* reclaim memory and quit */
  KheSolnArenaEnd(soln, a);
  /* ***
  while( HaArrayCount(event_classes) > 0 )
    KheEventClassDelete(MArrayRemoveLast(event_classes));
  HaArrayFree(event_classes);
  HaArrayFree(break_after);
  *** */

  if( DEBUG1 )
    fprintf(stderr, "] KheSolnSplitCycleMeet returning\n");
}

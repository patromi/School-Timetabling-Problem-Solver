
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
/*  FILE:         khe_sl_distribute_split_job.c                              */
/*  DESCRIPTION:  One distribute split job, used by KheSplitAndLink()        */
/*                                                                           */
/*****************************************************************************/
#include "khe_sl_layer_tree.h"

#define DEBUG1 0
#define DEBUG2 0

/*****************************************************************************/
/*                                                                           */
/*   KHE_DISTRIBUTE_SPLIT_JOB - one distribute job for KheSplitAndLink       */
/*                                                                           */
/*****************************************************************************/

struct khe_distribute_split_job_rec {
  KHE_SPLIT_JOB_TAG		tag;		/* job type tag              */
  KHE_COST			priority;	/* for sorting               */
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT constraint;  /* the constraint      */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_DISTRIBUTE_SPLIT_JOB KheDistributeSplitJobMake(                      */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)                              */
/*                                                                           */
/*  Make a new distribute split job with these attributes.                   */
/*                                                                           */
/*****************************************************************************/

KHE_DISTRIBUTE_SPLIT_JOB KheDistributeSplitJobMake(
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c, HA_ARENA a)
{
  KHE_DISTRIBUTE_SPLIT_JOB res;
  HaMake(res, a);
  res->tag = KHE_DISTRIBUTE_SPLIT_JOB_TAG;
  res->priority = KheConstraintCombinedWeight((KHE_CONSTRAINT) c);
  res->constraint = c;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeSplitJobFree(KHE_DISTRIBUTE_SPLIT_JOB st)              */
/*                                                                           */
/*  Free st.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheDistributeSplitJobFree(KHE_DISTRIBUTE_SPLIT_JOB st)
{
  MFree(st);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeSplitJobTryEvent(KHE_DISTRIBUTE_SPLIT_JOB st,          */
/*    KHE_EVENT e, KHE_SPLIT_FOREST sf)                                      */
/*                                                                           */
/*  Try to apply st to e in sf.                                              */
/*                                                                           */
/*****************************************************************************/

static void KheDistributeSplitJobTryEvent(KHE_DISTRIBUTE_SPLIT_JOB st,
  KHE_EVENT e, KHE_SPLIT_FOREST sf, HA_ARENA a)
{
  int durn, minimum, maximum;  /* KHE_SPLIT_STRONG ss; */
  durn = KheDistributeSplitEventsConstraintDuration(st->constraint);
  minimum = KheDistributeSplitEventsConstraintMinimum(st->constraint);
  maximum = KheDistributeSplitEventsConstraintMaximum(st->constraint);
  if( DEBUG1 )
  {
    fprintf(stderr, "  [ KheDistributeSplitJobTryEvent(st, %s)\n",
      KheEventId(e) != NULL ? KheEventId(e) : "-");
    fprintf(stderr, "    durn %d, minimum %d, maximum %d\n",
      durn, minimum, maximum);
  }
  KheSplitForestTryDurnAmount(sf, e, durn, minimum, maximum, a);

  /* ***
  ss = KheSplitInfoSplitStrong(st->split_info, e);
  KheSplitStrongDurnAmount(ss, durn, minimum, maximum);
  *** */
  if( DEBUG1 )
    fprintf(stderr, "  ] KheDistributeSplitJobTryEvent returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventDecreasingDurnCmp(const void *p1, const void *p2)            */
/*                                                                           */
/*  Comparison function for sorting events by decreasing duration and        */
/*  also uniqueifying them.                                                  */
/*                                                                           */
/*****************************************************************************/

typedef struct event_job_rec {
  KHE_EVENT			event;
  KHE_DISTRIBUTE_SPLIT_JOB	job;
} KHE_EVENT_JOB;

typedef HA_ARRAY(KHE_EVENT_JOB) ARRAY_KHE_EVENT_JOB;

static int KheEventJobDecreasingDurnCmp(const void *p1, const void *p2)
{
  KHE_EVENT_JOB ej1 = * (KHE_EVENT_JOB *) p1;
  KHE_EVENT_JOB ej2 = * (KHE_EVENT_JOB *) p2;
  int durn1 = KheEventDuration(ej1.event);
  int durn2 = KheEventDuration(ej2.event);
  if( durn1 != durn2 )
    return durn2 - durn1;
  else
    return KheEventIndex(ej2.event) - KheEventIndex(ej1.event);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,           */
/*    int start, int stop, KHE_SPLIT_FOREST sf)                              */
/*                                                                           */
/*  Try distribute split jobs *split_jobs[start .. stop-1] on sf.            */
/*                                                                           */
/*****************************************************************************/

void KheDistributeSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,
  int start, int stop, KHE_SPLIT_FOREST sf, HA_ARENA a)
{
  KHE_EVENT_GROUP eg;  KHE_EVENT e;  int i, j, k;
  ARRAY_KHE_EVENT_JOB event_jobs;  KHE_EVENT_JOB ej;
  KHE_DISTRIBUTE_SPLIT_JOB st;  /* KHE_SOLN soln;  bool doit; */
  if( DEBUG2 )
    fprintf(stderr, "[ KheDistributeSplitJobTry(-, %d, %d, sf)\n", start, stop);

  /* put all the event-jobs into a single array and sort */
  HaArrayInit(event_jobs, a);
  for( k = start;  k < stop;  k++ )
  {
    st = (KHE_DISTRIBUTE_SPLIT_JOB) HaArray(*split_jobs, k);
    for( i = 0;
	 i < KheDistributeSplitEventsConstraintEventCount(st->constraint); i++ )
    {
      e = KheDistributeSplitEventsConstraintEvent(st->constraint, i);
      ej.event = e;
      ej.job = st;
      HaArrayAddLast(event_jobs, ej);
    }
    for( i = 0;
	 i < KheDistributeSplitEventsConstraintEventGroupCount(st->constraint);
	 i++ )
    {
      eg = KheDistributeSplitEventsConstraintEventGroup(st->constraint, i);
      for( j = 0;  j < KheEventGroupEventCount(eg);  j++ )
      {
	e = KheEventGroupEvent(eg, j);
	ej.event = e;
	ej.job = st;
	HaArrayAddLast(event_jobs, ej);
      }
    }
  }
  HaArraySortUnique(event_jobs, &KheEventJobDecreasingDurnCmp);

  /* try each event-job */
  /* soln = KheSplitForestSoln(sf); */
  HaArrayForEach(event_jobs, ej, i)
  {
    if( DEBUG2 )
      fprintf(stderr, "  KheDistributeSplitJobTryEvent(%s) (durn %d)\n",
	KheEventId(ej.event), KheEventDuration(ej.event));
    /* ***
    doit = KheConstraintCombinedWeight((KHE_CONSTRAINT) ej.job->constraint) >=
      KheCost(1, 0);
    if( doit || (KheSolnDiversifier(soln) + i) % 6 != 0 )
    *** */
    KheDistributeSplitJobTryEvent(ej.job, ej.event, sf, a);
  }
  HaArrayFree(event_jobs);
  if( DEBUG2 )
    fprintf(stderr, "] KheDistributeSplitJobTry\n");

  /* ***
  for( i = 0;
       i < KheDistributeSplitEventsConstraintEventCount(st->constraint);  i++ )
  {
    e = KheDistributeSplitEventsConstraintEvent(st->constraint, i);
    KheDistributeSplitJobTryEvent(st, e, sf);
  }
  for( i = 0;
       i < KheDistributeSplitEventsConstraintEventGroupCount(st->constraint);
       i++ )
  {
    eg = KheDistributeSplitEventsConstraintEventGroup(st->constraint, i);
    for( j = 0;  j < KheEventGroupEventCount(eg);  j++ )
    {
      e = KheEventGroupEvent(eg, j);
      KheDistributeSplitJobTryEvent(st, e, sf);
    }
  }
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeSplitJobDebug(KHE_DISTRIBUTE_SPLIT_JOB st,             */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of job st onto fp with the given indent.                     */
/*                                                                           */
/*****************************************************************************/

void KheDistributeSplitJobDebug(KHE_DISTRIBUTE_SPLIT_JOB st,
  int indent, FILE *fp, HA_ARENA a)
{
  fprintf(fp, "%*s[ %.5f Distribute Split Job %s ]\n", indent, "",
    KheCostShow(st->priority), KheConstraintId((KHE_CONSTRAINT)st->constraint));
}

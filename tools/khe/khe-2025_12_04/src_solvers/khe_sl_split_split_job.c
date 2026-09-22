
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
/*  FILE:         khe_sl_split_split_job.c                                   */
/*  DESCRIPTION:  One split split job, used by KheSplitAndLink()             */
/*                                                                           */
/*****************************************************************************/
#include "khe_sl_layer_tree.h"

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*   KHE_SPLIT_SPLIT_JOB - one split job for KheSplitAndLink                 */
/*                                                                           */
/*****************************************************************************/

struct khe_split_split_job_rec {
  KHE_SPLIT_JOB_TAG		tag;		/* job type tag              */
  KHE_COST			priority;	/* for sorting               */
  KHE_SPLIT_EVENTS_CONSTRAINT	constraint;	/* the constraint to apply   */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_SPLIT_JOB KheSplitSplitJobMake(KHE_SPLIT_EVENTS_CONSTRAINT c)  */
/*                                                                           */
/*  Make a split split job with these attributes.                            */
/*                                                                           */
/*****************************************************************************/

KHE_SPLIT_SPLIT_JOB KheSplitSplitJobMake(KHE_SPLIT_EVENTS_CONSTRAINT c,
  HA_ARENA a)
{
  KHE_SPLIT_SPLIT_JOB res;
  HaMake(res, a);
  res->tag = KHE_SPLIT_SPLIT_JOB_TAG;
  res->priority = KheConstraintCombinedWeight((KHE_CONSTRAINT) c);
  res->constraint = c;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitSplitJobFree(KHE_SPLIT_SPLIT_JOB st)                        */
/*                                                                           */
/*  Free st.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSplitSplitJobFree(KHE_SPLIT_SPLIT_JOB st)
{
  MFree(st);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitSplitJobTryEvent(KHE_SPLIT_SPLIT_JOB st, KHE_EVENT e,       */
/*    KHE_SPLIT_FOREST sf)                                                   */
/*                                                                           */
/*  Try st on e in sf.                                                       */
/*                                                                           */
/*****************************************************************************/

static void KheSplitSplitJobTryEvent(KHE_SPLIT_SPLIT_JOB st, KHE_EVENT e,
  KHE_SPLIT_FOREST sf, HA_ARENA a)
{
  int d, durn, min_durn, max_durn, min_amount, max_amount;
  /* KHE_SPLIT_STRONG ss; */  /* bool success; */
  durn = KheEventDuration(e);
  min_durn = KheSplitEventsConstraintMinDuration(st->constraint);
  max_durn = KheSplitEventsConstraintMaxDuration(st->constraint);
  min_amount = KheSplitEventsConstraintMinAmount(st->constraint);
  max_amount = KheSplitEventsConstraintMaxAmount(st->constraint);
  if( DEBUG1 )
  {
    fprintf(stderr, "  [ KheSplitSplitJobTryEvent(st, %s)\n",
      KheEventId(e) != NULL ? KheEventId(e) : "-");
    fprintf(stderr,
      "    min_durn %d, max_durn %d, min_amount %d, max_amount %d\n",
      min_durn, max_durn, min_amount, max_amount);
  }

  /* try everything */
  KheSplitForestTryTotalAmount(sf, e, min_amount, max_amount, a);
  for( d = 1;  d < min_durn;  d++ )
    KheSplitForestTryDurnAmount(sf, e, d, 0, 0, a);
  for( d = max_durn + 1;  d <= durn;  d++ )
    KheSplitForestTryDurnAmount(sf, e, d, 0, 0, a);

  /* ***
  ss = KheSplitInfoSplitStrong(st->split_info, e);
  KheSplitStrongTotalAmount(ss, min_amount, max_amount);
  for( d = 1;  d < min_durn;  d++ )
    KheSplitStrongDurnAmount(ss, d, 0, 0);
  for( d = max_durn + 1;  d <= durn;  d++ )
    KheSplitStrongDurnAmount(ss, d, 0, 0);
  *** */

  if( DEBUG1 )
    fprintf(stderr, "  ] KheSplitSplitJobTryEvent returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,                */
/*    int start, int stop, KHE_SPLIT_FOREST sf)                              */
/*                                                                           */
/*  Try split split jobs *split_jobs[start .. stop-1] on sf.                 */
/*                                                                           */
/*****************************************************************************/

void KheSplitSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,
  int start, int stop, KHE_SPLIT_FOREST sf, HA_ARENA a)
{
  KHE_EVENT_GROUP eg;  KHE_EVENT e;  int i, j, k;  KHE_SPLIT_SPLIT_JOB st;
  for( k = start;  k < stop;  k++ )
  {
    st = (KHE_SPLIT_SPLIT_JOB) HaArray(*split_jobs, k);
    for( i = 0;  i < KheSplitEventsConstraintEventCount(st->constraint);  i++ )
    {
      e = KheSplitEventsConstraintEvent(st->constraint, i);
      KheSplitSplitJobTryEvent(st, e, sf, a);
    }
    for( i=0; i < KheSplitEventsConstraintEventGroupCount(st->constraint); i++ )
    {
      eg = KheSplitEventsConstraintEventGroup(st->constraint, i);
      for( j = 0;  j < KheEventGroupEventCount(eg);  j++ )
      {
	e = KheEventGroupEvent(eg, j);
	KheSplitSplitJobTryEvent(st, e, sf, a);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/* void KheSplitSplitJobDebug(KHE_SPLIT_SPLIT_JOB st, int indent, FILE *fp)  */
/*                                                                           */
/*  Debug print of job st onto fp with the given indent.                     */
/*                                                                           */
/*****************************************************************************/

void KheSplitSplitJobDebug(KHE_SPLIT_SPLIT_JOB st, int indent, FILE *fp,
  HA_ARENA a)
{
  fprintf(fp, "%*s[ %.5f Split Split Job %s ]\n", indent, "",
    KheCostShow(st->priority), KheConstraintId((KHE_CONSTRAINT)st->constraint));
}

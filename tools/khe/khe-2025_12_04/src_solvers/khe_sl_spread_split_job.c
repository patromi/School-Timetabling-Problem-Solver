
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
/*  FILE:         khe_sl_spread_split_job.c                                  */
/*  DESCRIPTION:  One spread split job, used by KheSplitAndLink()            */
/*                                                                           */
/*****************************************************************************/
#include "khe_sl_layer_tree.h"

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*   KHE_SPREAD_SPLIT_JOB - one spread job for KheSplitAndLink               */
/*                                                                           */
/*****************************************************************************/

struct khe_spread_split_job_rec {
  KHE_SPLIT_JOB_TAG		tag;		/* job type tag              */
  KHE_COST			priority;	/* for sorting               */
  KHE_SPREAD_EVENTS_CONSTRAINT	constraint;	/* the constraint            */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_SPREAD_SPLIT_JOB KheSpreadSplitJobMake(                              */
/*    KHE_SPREAD_EVENTS_CONSTRAINT c)                                        */
/*                                                                           */
/*  Make a spread split job with these attributes.                           */
/*                                                                           */
/*****************************************************************************/

KHE_SPREAD_SPLIT_JOB KheSpreadSplitJobMake(
  KHE_SPREAD_EVENTS_CONSTRAINT c, HA_ARENA a)
{
  KHE_SPREAD_SPLIT_JOB res;
  HaMake(res, a);
  res->tag = KHE_SPREAD_SPLIT_JOB_TAG;
  res->priority = KheConstraintCombinedWeight((KHE_CONSTRAINT) c);
  res->constraint = c;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSpreadSplitJobFree(KHE_SPREAD_SPLIT_JOB st)                      */
/*                                                                           */
/*  Free st.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSpreadSplitJobFree(KHE_SPREAD_SPLIT_JOB st)
{
  MFree(st);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSpreadSplitJobTryEventGroup(KHE_SPREAD_SPLIT_JOB st,             */
/*    int min_amount, int max_amount, KHE_EVENT_GROUP eg)                    */
/*                                                                           */
/*  Try st on eg.                                                            */
/*                                                                           */
/*****************************************************************************/

static void KheSpreadSplitJobTryEventGroup(KHE_SPREAD_SPLIT_JOB st,
  int min_amount, int max_amount, KHE_EVENT_GROUP eg, KHE_SPLIT_FOREST sf,
  HA_ARENA a)
{
  if( DEBUG1 )
    fprintf(stderr, "[ KheSpreadSplitJobTryEventGroup(st, %d-%d, %s)\n",
      min_amount, max_amount,
      KheEventGroupId(eg) != NULL ? KheEventGroupId(eg) : "-");
  KheSplitForestTrySpread(sf, eg, min_amount, max_amount, a);
  if( DEBUG1 )
    fprintf(stderr, "] KheSpreadSplitJobTryEventGroup\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSpreadSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,               */
/*    int start, int stop, KHE_SPLIT_FOREST sf)                              */
/*                                                                           */
/*  Try spread split jobs *split_jobs[start .. stop-1] on sf.                */
/*                                                                           */
/*****************************************************************************/

void KheSpreadSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,
  int start, int stop, KHE_SPLIT_FOREST sf, HA_ARENA a)
{
  KHE_TIME_SPREAD ts;  KHE_LIMITED_TIME_GROUP ltg;  int i, k;
  int min_amount, max_amount;  KHE_EVENT_GROUP eg;  KHE_SPREAD_SPLIT_JOB st;

  for( k = start;  k < stop;  k++ )
  {
    st = (KHE_SPREAD_SPLIT_JOB) HaArray(*split_jobs, k);

    /* work out the minimum and maximum number of parts */
    min_amount = max_amount = 0;
    ts = KheSpreadEventsConstraintTimeSpread(st->constraint);
    for( i = 0;  i < KheTimeSpreadLimitedTimeGroupCount(ts);  i++ )
    {
      ltg = KheTimeSpreadLimitedTimeGroup(ts, i);
      min_amount += KheLimitedTimeGroupMinimum(ltg);
      max_amount += KheLimitedTimeGroupMaximum(ltg);
    }

    /* but a minimum only makes sense if the time groups are disjoint */
    if( !KheTimeSpreadTimeGroupsDisjoint(ts) )
      min_amount = 0;

    /* and a maximum only makes sense if they cover the whole cycle */
    if( !KheTimeSpreadCoversWholeCycle(ts) )
      max_amount = INT_MAX;

    /* apply job to each event group, but only if it might change something */
    if( min_amount > 0 || max_amount < INT_MAX )
      for(i=0; i<KheSpreadEventsConstraintEventGroupCount(st->constraint); i++)
      {
	eg = KheSpreadEventsConstraintEventGroup(st->constraint, i);
	KheSpreadSplitJobTryEventGroup(st, min_amount, max_amount, eg, sf, a);
      }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSpreadSplitJobDebug(KHE_SPREAD_SPLIT_JOB st,                     */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of job st onto fp with the given indent.                     */
/*                                                                           */
/*****************************************************************************/

void KheSpreadSplitJobDebug(KHE_SPREAD_SPLIT_JOB st, int indent, FILE *fp,
  HA_ARENA a)
{
  fprintf(fp, "%*s[ %.5f Spread Split Job %s ]\n", indent, "",
    KheCostShow(st->priority), KheConstraintId((KHE_CONSTRAINT)st->constraint));
}

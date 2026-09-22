
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
/*  FILE:         khe_sl_domain_split_job.c                                  */
/*  DESCRIPTION:  One domain split job, used by KheSplitAndLink()            */
/*                                                                           */
/*****************************************************************************/
#include "khe_sl_layer_tree.h"

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*   KHE_DOMAIN_SPLIT_JOB - one domain job for KheSplitAndLink               */
/*                                                                           */
/*****************************************************************************/

struct khe_domain_split_job_rec {
  KHE_SPLIT_JOB_TAG		tag;		/* job type tag              */
  KHE_COST			priority;	/* for sorting               */
  KHE_PREFER_TIMES_CONSTRAINT	constraint;	/* the constraint            */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_DOMAIN_SPLIT_JOB KheDomainSplitJobMake(                              */
/*    KHE_PREFER_TIMES_CONSTRAINT c)                                         */
/*                                                                           */
/*  Make a domain split job with these attributes.                           */
/*                                                                           */
/*****************************************************************************/

KHE_DOMAIN_SPLIT_JOB KheDomainSplitJobMake(KHE_PREFER_TIMES_CONSTRAINT c,
  HA_ARENA a)
{
  KHE_DOMAIN_SPLIT_JOB res;
  HaMake(res, a);
  res->tag = KHE_DOMAIN_SPLIT_JOB_TAG;
  res->priority = KheConstraintCombinedWeight((KHE_CONSTRAINT) c);
  res->constraint = c;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDomainSplitJobFree(KHE_DOMAIN_SPLIT_JOB st)                      */
/*                                                                           */
/*  Free st.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheDomainSplitJobFree(KHE_DOMAIN_SPLIT_JOB st)
{
  MFree(st);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheDomainSplitJobTryEvent(KHE_DOMAIN_SPLIT_JOB st, KHE_EVENT e)     */
/*                                                                           */
/*  Try st on e.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheDomainSplitJobTryEvent(KHE_DOMAIN_SPLIT_JOB st, KHE_EVENT e,
  KHE_SPLIT_FOREST sf, HA_ARENA a)
{
  KHE_TIME_GROUP tg;  int durn, d;  /* bool success; */  /* KHE_SPLIT_WEAK sw; */
  if( DEBUG1 )
    fprintf(stderr, "[ KheDomainSplitJobTryEvent(st, %s)\n",
      KheEventId(e) != NULL ? KheEventId(e) : "-");
  tg = KhePreferTimesConstraintDomain(st->constraint);
  durn = KhePreferTimesConstraintDuration(st->constraint);
  if( durn == KHE_ANY_DURATION )
    for( d = 1;  d <= KheEventDuration(e);  d++ )
      KheSplitForestTryEventDomain(sf, e, tg, d, a);
  else
    KheSplitForestTryEventDomain(sf, e, tg, durn, a);

  /* ***
  sw = KheSplitInfoSplitWeak(st->split_info, e);
  if( durn == KHE_ANY_DURATION )
    for( d = 1;  d <= KheEventDuration(e);  d++ )
      KheSplitWeakDomain(sw, tg, d);
  else
    KheSplitWeakDomain(sw, tg, durn);
  *** */
  if( DEBUG1 )
    fprintf(stderr, "] KheDomainSplitJobTryEvent returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDomainSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,               */
/*    int start, int stop, KHE_SPLIT_FOREST sf)                              */
/*                                                                           */
/*  Try domain split jobs *split_jobs[start .. stop-1] on sf.                */
/*                                                                           */
/*****************************************************************************/

void KheDomainSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,
  int start, int stop, KHE_SPLIT_FOREST sf, HA_ARENA a)
{
  KHE_EVENT_GROUP eg;  KHE_EVENT e;  int i, j, k;  KHE_DOMAIN_SPLIT_JOB st;
  for( k = start;  k < stop;  k++ )
  {
    st = (KHE_DOMAIN_SPLIT_JOB) HaArray(*split_jobs, k);
    for( i = 0;  i < KhePreferTimesConstraintEventCount(st->constraint);  i++ )
    {
      e = KhePreferTimesConstraintEvent(st->constraint, i);
      KheDomainSplitJobTryEvent(st, e, sf, a);
    }
    for( i=0; i < KhePreferTimesConstraintEventGroupCount(st->constraint); i++ )
    {
      eg = KhePreferTimesConstraintEventGroup(st->constraint, i);
      for( j = 0;  j < KheEventGroupEventCount(eg);  j++ )
      {
	e = KheEventGroupEvent(eg, j);
	KheDomainSplitJobTryEvent(st, e, sf, a);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDomainSplitJobDebug(KHE_DOMAIN_SPLIT_JOB st,                     */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of job st onto fp with the given indent.                     */
/*                                                                           */
/*****************************************************************************/

void KheDomainSplitJobDebug(KHE_DOMAIN_SPLIT_JOB st, int indent, FILE *fp,
  HA_ARENA a)
{
  fprintf(fp, "%*s[ %.5f Domain Split Job %s ]\n", indent, "",
    KheCostShow(st->priority), KheConstraintId((KHE_CONSTRAINT)st->constraint));
}

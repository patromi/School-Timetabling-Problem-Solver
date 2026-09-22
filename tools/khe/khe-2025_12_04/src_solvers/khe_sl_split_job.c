
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
/*  FILE:         khe_sl_split_job.c                                         */
/*  DESCRIPTION:  One split job, used by KheSplitAndLink()                   */
/*                                                                           */
/*****************************************************************************/
#include "khe_sl_layer_tree.h"

/*****************************************************************************/
/*                                                                           */
/*   KHE_SPLIT_JOB - one job to be carried out by KheSplitAndLink            */
/*                                                                           */
/*****************************************************************************/

struct khe_split_job_rec {
  KHE_SPLIT_JOB_TAG		tag;		/* job type tag              */
  KHE_COST			priority;	/* for sorting               */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_JOB_TAG KheSplitJobTag(KHE_SPLIT_JOB st)                       */
/*                                                                           */
/*  Return the type tag of st.                                               */
/*                                                                           */
/*****************************************************************************/

KHE_SPLIT_JOB_TAG KheSplitJobTag(KHE_SPLIT_JOB st)
{
  return st->tag;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSplitJobPriority(KHE_SPLIT_JOB st)                                */
/*                                                                           */
/*  Return the priority of st.                                               */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheSplitJobPriority(KHE_SPLIT_JOB st)
{
  return st->priority;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSplitJobDecreasingPriorityCmp(const void *t1, const void *t2)     */
/*                                                                           */
/*  Comparison function for sorting split jobs by decreasing priority,       */
/*  and if priority is equal, by decreasing tag.                             */
/*                                                                           */
/*****************************************************************************/

int KheSplitJobDecreasingPriorityCmp(const void *t1, const void *t2)
{
  KHE_SPLIT_JOB st1 = * (KHE_SPLIT_JOB *) t1;
  KHE_SPLIT_JOB st2 = * (KHE_SPLIT_JOB *) t2;
  if( st2->priority != st1->priority )
    return KheCostCmp(st2->priority, st1->priority);
  else
    return st2->tag - st1->tag;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs, int start,          */
/*    int stop, KHE_SPLIT_FOREST sf)                                         */
/*                                                                           */
/*  Try *split_jobs[start .. stop-1].  There is at least one job in this     */
/*  range, and they all have the same tag and the same priority.             */
/*                                                                           */
/*****************************************************************************/

void KheSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs, int start,
  int stop, KHE_SPLIT_FOREST sf, HA_ARENA a)
{
  KHE_SPLIT_JOB sj;
  sj = HaArray(*split_jobs, start);
  switch( sj->tag )
  {
    case KHE_AVOID_CLASHES_SPLIT_JOB_TAG:

      KheAvoidClashesSplitJobTry(split_jobs, start, stop, sf, a);
      break;

    case KHE_PACK_SPLIT_JOB_TAG:

      KhePackSplitJobTry(split_jobs, start, stop, sf, a);
      break;

    case KHE_PREASSIGNED_SPLIT_JOB_TAG:

      KhePreassignedSplitJobTry(split_jobs, start, stop, sf, a);
      break;

    case KHE_ASSIGNED_SPLIT_JOB_TAG:

      KheAssignedSplitJobTry(split_jobs, start, stop, sf, a);
      break;

    case KHE_LINK_SPLIT_JOB_TAG:

      KheLinkSplitJobTry(split_jobs, start, stop, sf, a);
      break;

    case KHE_SPLIT_SPLIT_JOB_TAG:

      KheSplitSplitJobTry(split_jobs, start, stop, sf, a);
      break;

    case KHE_DISTRIBUTE_SPLIT_JOB_TAG:

      KheDistributeSplitJobTry(split_jobs, start, stop, sf, a);
      break;

    case KHE_SPREAD_SPLIT_JOB_TAG:

      KheSpreadSplitJobTry(split_jobs, start, stop, sf, a);
      break;

    case KHE_DOMAIN_SPLIT_JOB_TAG:

      KheDomainSplitJobTry(split_jobs, start, stop, sf, a);
      break;

    default:

      HnAbort("KheSplitJobTry given job of unknown type");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitJobFree(KHE_SPLIT_JOB st)                                   */
/*                                                                           */
/*  Free st.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSplitJobFree(KHE_SPLIT_JOB st)
{
  switch( st->tag )
  {
    ** ***
    case KHE_LAYER_SPLIT_JOB_TAG:

      KheLayerSplitJobFree((KHE_LAYER_SPLIT_JOB) st);
      break;
    *** **

    case KHE_AVOID_CLASHES_SPLIT_JOB_TAG:

      KheAvoidClashesSplitJobFree((KHE_AVOID_CLASHES_SPLIT_JOB) st);
      break;

    case KHE_PACK_SPLIT_JOB_TAG:

      KhePackSplitJobFree((KHE_PACK_SPLIT_JOB) st);
      break;

    case KHE_PREASSIGNED_SPLIT_JOB_TAG:

      KhePreassignedSplitJobFree((KHE_PREASSIGNED_SPLIT_JOB) st);
      break;

    case KHE_ASSIGNED_SPLIT_JOB_TAG:

      KheAssignedSplitJobFree((KHE_ASSIGNED_SPLIT_JOB) st);
      break;

    case KHE_LINK_SPLIT_JOB_TAG:

      KheLinkSplitJobFree((KHE_LINK_SPLIT_JOB) st);
      break;

    case KHE_SPLIT_SPLIT_JOB_TAG:

      KheSplitSplitJobFree((KHE_SPLIT_SPLIT_JOB) st);
      break;

    case KHE_DISTRIBUTE_SPLIT_JOB_TAG:

      KheDistributeSplitJobFree((KHE_DISTRIBUTE_SPLIT_JOB) st);
      break;

    case KHE_SPREAD_SPLIT_JOB_TAG:

      KheSpreadSplitJobFree((KHE_SPREAD_SPLIT_JOB) st);
      break;

    case KHE_DOMAIN_SPLIT_JOB_TAG:

      KheDomainSplitJobFree((KHE_DOMAIN_SPLIT_JOB) st);
      break;

    default:

      HnAbort("KheSplitJobFree given job of unknown type");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitJobDebug(KHE_SPLIT_JOB st, int indent, FILE *fp)            */
/*                                                                           */
/*  Debug print of job st onto fp with the given indent.                     */
/*                                                                           */
/*****************************************************************************/

void KheSplitJobDebug(KHE_SPLIT_JOB st, int indent, FILE *fp, HA_ARENA a)
{
  switch( st->tag )
  {
    /* ***
    case KHE_LAYER_SPLIT_JOB_TAG:

      KheLayerSplitJobDebug((KHE_LAYER_SPLIT_JOB) st, indent, fp);
      break;
    *** */

    case KHE_AVOID_CLASHES_SPLIT_JOB_TAG:

      KheAvoidClashesSplitJobDebug((KHE_AVOID_CLASHES_SPLIT_JOB) st,
	indent, fp, a);
      break;

    case KHE_PACK_SPLIT_JOB_TAG:

      KhePackSplitJobDebug((KHE_PACK_SPLIT_JOB) st, indent, fp, a);
      break;

    case KHE_PREASSIGNED_SPLIT_JOB_TAG:

      KhePreassignedSplitJobDebug((KHE_PREASSIGNED_SPLIT_JOB) st, indent, fp, a);
      break;

    case KHE_ASSIGNED_SPLIT_JOB_TAG:

      KheAssignedSplitJobDebug((KHE_ASSIGNED_SPLIT_JOB) st, indent, fp, a);
      break;

    case KHE_LINK_SPLIT_JOB_TAG:

      KheLinkSplitJobDebug((KHE_LINK_SPLIT_JOB) st, indent, fp, a);
      break;

    case KHE_SPLIT_SPLIT_JOB_TAG:

      KheSplitSplitJobDebug((KHE_SPLIT_SPLIT_JOB) st, indent, fp, a);
      break;

    case KHE_DISTRIBUTE_SPLIT_JOB_TAG:

      KheDistributeSplitJobDebug((KHE_DISTRIBUTE_SPLIT_JOB) st, indent, fp, a);
      break;

    case KHE_SPREAD_SPLIT_JOB_TAG:

      KheSpreadSplitJobDebug((KHE_SPREAD_SPLIT_JOB) st, indent, fp, a);
      break;

    case KHE_DOMAIN_SPLIT_JOB_TAG:

      KheDomainSplitJobDebug((KHE_DOMAIN_SPLIT_JOB) st, indent, fp, a);
      break;

    default:

      HnAbort("KheSplitJobDebug given job of unknown type");
  }
}

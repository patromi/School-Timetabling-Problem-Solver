
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
/*  FILE:         khe_sl_preassigned_split_job.c                             */
/*  DESCRIPTION:  One preassigned split job, used by KheSplitAndLink()       */
/*                                                                           */
/*****************************************************************************/
#include "khe_sl_layer_tree.h"

/*****************************************************************************/
/*                                                                           */
/*   KHE_PREASSIGNED_SPLIT_JOB - one preassigned job for KheSplitAndLink     */
/*                                                                           */
/*****************************************************************************/

struct khe_preassigned_split_job_rec {
  KHE_SPLIT_JOB_TAG		tag;		/* job type tag              */
  KHE_COST			priority;	/* for sorting               */
  KHE_EVENT			event;		/* the preassigned event     */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_PREASSIGNED_SPLIT_JOB KhePreassignedSplitJobMake(KHE_EVENT e)        */
/*                                                                           */
/*  Make a preassigned split job with these attributes.                      */
/*                                                                           */
/*****************************************************************************/

KHE_PREASSIGNED_SPLIT_JOB KhePreassignedSplitJobMake(KHE_EVENT e, HA_ARENA a)
{
  KHE_PREASSIGNED_SPLIT_JOB res;
  HaMake(res, a);
  res->tag = KHE_PREASSIGNED_SPLIT_JOB_TAG;
  res->priority = KheCostMax - 1;
  res->event = e;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePreassignedSplitJobFree(KHE_PREASSIGNED_SPLIT_JOB st)            */
/*                                                                           */
/*  Free st.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
void KhePreassignedSplitJobFree(KHE_PREASSIGNED_SPLIT_JOB st)
{
  MFree(st);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePreassignedSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,          */
/*    int start, int stop, KHE_SPLIT_FOREST sf)                              */
/*                                                                           */
/*  Try preassigned split jobs *split_jobs[start .. stop-1] on sf.           */
/*                                                                           */
/*****************************************************************************/

void KhePreassignedSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,
  int start, int stop, KHE_SPLIT_FOREST sf, HA_ARENA a)
{
  int k;  KHE_PREASSIGNED_SPLIT_JOB st;
  /* ***
  KHE_SPLIT_WEAK sw;  bool success;
  sw = KheSplitInfoSplitWeak(st->split_info, st->event);
  KheSplitWeakPreassigned(sw, KheEventPreassignedTime(st->event));
  *** */
  for( k = start;  k < stop;  k++ )
  {
    st = (KHE_PREASSIGNED_SPLIT_JOB) HaArray(*split_jobs, k);
    KheSplitForestTryPreassignedTime(sf, st->event,
      KheEventPreassignedTime(st->event), a);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePreassignedSplitJobDebug(KHE_PREASSIGNED_SPLIT_JOB st,           */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of job st onto fp with the given indent.                     */
/*                                                                           */
/*****************************************************************************/

void KhePreassignedSplitJobDebug(KHE_PREASSIGNED_SPLIT_JOB st,
  int indent, FILE *fp, HA_ARENA a)
{
  fprintf(fp, "%*s[ %.5f Preassigned Split Job %s ]\n", indent, "",
    KheCostShow(st->priority),
    KheEventId(st->event) != NULL ? KheEventId(st->event) : "-");
}

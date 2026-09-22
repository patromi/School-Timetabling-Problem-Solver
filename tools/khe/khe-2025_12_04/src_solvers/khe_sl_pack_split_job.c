
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
/*  FILE:         khe_sl_pack_split_job.c                                    */
/*  DESCRIPTION:  One pack split job, used by KheSplitAndLink()              */
/*                                                                           */
/*****************************************************************************/
#include "khe_sl_layer_tree.h"

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*   KHE_PACK_SPLIT_JOB - one pack split job for KheSplitAndLink             */
/*                                                                           */
/*****************************************************************************/

struct khe_pack_split_job_rec {
  KHE_SPLIT_JOB_TAG		tag;		/* job type tag              */
  KHE_COST			priority;	/* for sorting               */
  KHE_EVENT			event;		/* the already-split event   */
  KHE_PARTITION			partition;	/* partition of sub-events   */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_PACK_SPLIT_JOB KhePackSplitJobMake(KHE_EVENT e, KHE_PARTITION p)     */
/*                                                                           */
/*  Make a pack split job with these attributes.  Parameter e may be NULL,   */
/*  in which case the job is to be applied to every event of the instance,   */
/*  because p describes the cycle layer, that all events eventually enter.   */
/*                                                                           */
/*****************************************************************************/

KHE_PACK_SPLIT_JOB KhePackSplitJobMake(KHE_EVENT e, KHE_PARTITION p,
  HA_ARENA a)
{
  KHE_PACK_SPLIT_JOB res;
  HaMake(res, a);
  res->tag = KHE_PACK_SPLIT_JOB_TAG;
  res->priority = KheCostMax;
  res->event = e;
  res->partition = p;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePackSplitJobFree(KHE_PACK_SPLIT_JOB st)                          */
/*                                                                           */
/*  Free st.  Don't free its partition though.                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void KhePackSplitJobFree(KHE_PACK_SPLIT_JOB st)
{
  MFree(st);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePackSplitJobTryEvent(KHE_PACK_SPLIT_JOB st, KHE_EVENT e,         */
/*    KHE_SPLIT_FOREST sf)                                                   */
/*                                                                           */
/*  Try st on e in sf.                                                       */
/*                                                                           */
/*****************************************************************************/

static void KhePackSplitJobTryEvent(KHE_PACK_SPLIT_JOB st, KHE_EVENT e,
  KHE_SPLIT_FOREST sf, HA_ARENA a)
{
  if( DEBUG1 )
    fprintf(stderr, "[ KhePackSplitJobTryEvent(st, %s) %s\n",
      KheEventId(e) != NULL ? KheEventId(e) : "-",
      KhePartitionShow(st->partition, a));
  KheSplitForestTryPackableInto(sf, e, st->partition, a);
  if( DEBUG1 )
    fprintf(stderr, "] KhePackSplitJobTryEvent returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePackSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,                 */
/*    int start, int stop, KHE_SPLIT_FOREST sf)                              */
/*                                                                           */
/*  Try pack jobs *split_jobs[start .. stop-1] on sf.                        */
/*                                                                           */
/*****************************************************************************/

void KhePackSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,
  int start, int stop, KHE_SPLIT_FOREST sf, HA_ARENA a)
{
  KHE_INSTANCE ins;  KHE_EVENT e;  int i, k;  KHE_PACK_SPLIT_JOB st;
  for( k = start;  k < stop;  k++ )
  {
    st = (KHE_PACK_SPLIT_JOB) HaArray(*split_jobs, k);
    if( st->event != NULL )
      KhePackSplitJobTryEvent(st, st->event, sf, a);
    else
    {
      ins = KheSolnInstance(KheSplitForestSoln(sf));
      for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
      {
	e = KheInstanceEvent(ins, i);
	KhePackSplitJobTryEvent(st, e, sf, a);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePackSplitJobDebug(KHE_PACK_SPLIT_JOB st, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of job st onto fp with the given indent.                     */
/*                                                                           */
/*****************************************************************************/

void KhePackSplitJobDebug(KHE_PACK_SPLIT_JOB st, int indent, FILE *fp,
  HA_ARENA a)
{
  fprintf(fp, "%*s[ %.5f Pack Split Job %s %s ]\n", indent, "",
    KheCostShow(st->priority), st->event == NULL ? "(all)" :
    KheEventId(st->event) != NULL ? KheEventId(st->event) : "-",
    KhePartitionShow(st->partition, a));
}

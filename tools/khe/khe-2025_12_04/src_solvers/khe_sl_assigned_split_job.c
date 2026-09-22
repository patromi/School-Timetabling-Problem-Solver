
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
/*  FILE:         khe_assigned_split_job.c                                   */
/*  DESCRIPTION:  One assigned split job, used by KheSplitAndLink()          */
/*                                                                           */
/*****************************************************************************/
#include "khe_sl_layer_tree.h"

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*   KHE_ASSIGNED_SPLIT_JOB - one assigned job for KheSplitAndLink           */
/*                                                                           */
/*****************************************************************************/

struct khe_assigned_split_job_rec {
  KHE_SPLIT_JOB_TAG		tag;		/* job type tag              */
  KHE_COST			priority;	/* for sorting               */
  KHE_EVENT			child_event;	/* the assigned event        */
  KHE_EVENT			prnt_event;	/* the event assigned to     */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_ASSIGNED_SPLIT_JOB KheAssignedSplitJobMake(KHE_EVENT e)              */
/*                                                                           */
/*  Return an assigned split job with these attributes.                      */
/*                                                                           */
/*****************************************************************************/

KHE_ASSIGNED_SPLIT_JOB KheAssignedSplitJobMake(KHE_EVENT child_event,
  KHE_EVENT prnt_event, HA_ARENA a)
{
  KHE_ASSIGNED_SPLIT_JOB res;
  HaMake(res, a);
  res->tag = KHE_ASSIGNED_SPLIT_JOB_TAG;
  res->priority = KheCostMax - 2;
  res->child_event = child_event;
  res->prnt_event = prnt_event;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignedSplitJobFree(KHE_ASSIGNED_SPLIT_JOB st)                  */
/*                                                                           */
/*  Free st.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheAssignedSplitJobFree(KHE_ASSIGNED_SPLIT_JOB st)
{
  MFree(st);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignedSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,             */
/*    int start, int stop, KHE_SPLIT_FOREST sf)                              */
/*                                                                           */
/*  Try assigned split jobs *split_jobs[start .. stop-1] on sf.              */
/*                                                                           */
/*****************************************************************************/

void KheAssignedSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,
  int start, int stop, KHE_SPLIT_FOREST sf, HA_ARENA a)
{
  /* int i;  KHE_SOLN_EVENT se;  KHE_SOLN soln; */
  int k;  KHE_ASSIGNED_SPLIT_JOB st;
  for( k = start;  k < stop;  k++ )
  {
    st = (KHE_ASSIGNED_SPLIT_JOB) HaArray(*split_jobs, k);
    if( DEBUG1 )
      fprintf(stderr, "[ KheAssignedSplitJobTry(st, %s -> %s)\n",
	KheEventId(st->child_event) != NULL ? KheEventId(st->child_event) : "-",
	KheEventId(st->prnt_event) != NULL ? KheEventId(st->prnt_event) : "-");
    KheSplitForestTryEventAssign(sf, st->child_event, st->prnt_event, a);
    if( DEBUG1 )
      fprintf(stderr, "] KheAssignedSplitJobTry returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignedSplitJobDebug(KHE_ASSIGNED_SPLIT_JOB st,                 */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of job st onto fp with the given indent.                     */
/*                                                                           */
/*****************************************************************************/

void KheAssignedSplitJobDebug(KHE_ASSIGNED_SPLIT_JOB st,
  int indent, FILE *fp, HA_ARENA a)
{
  fprintf(fp, "%*s[ %.5f Assigned Split Job %s -> %s ]\n", indent, "",
    KheCostShow(st->priority),
    KheEventId(st->child_event) != NULL ? KheEventId(st->child_event) : "-",
    KheEventId(st->prnt_event) != NULL ? KheEventId(st->prnt_event) : "-");
}

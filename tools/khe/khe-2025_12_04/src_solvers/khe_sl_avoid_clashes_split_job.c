
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
/*  FILE:         khe_sl_avoid_clashes_split_job.c                           */
/*  DESCRIPTION:  One avoid clashes split job, used by KheSplitAndLink()     */
/*                                                                           */
/*****************************************************************************/
#include "khe_sl_layer_tree.h"

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*   KHE_AVOID_CLASHES_SPLIT_JOB - one avoid clashes job KheSplitAndLink     */
/*                                                                           */
/*****************************************************************************/

struct khe_avoid_clashes_split_job_rec {
  KHE_SPLIT_JOB_TAG		tag;		/* job type tag              */
  KHE_COST			priority;	/* for sorting               */
  KHE_AVOID_CLASHES_CONSTRAINT	constraint;	/* the constraint            */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVOID_CLASHES_SPLIT_JOB KheAvoidClashesSplitJobMake(                 */
/*    KHE_AVOID_CLASHES_CONSTRAINT c, HA_ARENA a)                            */
/*                                                                           */
/*  Make an avoid_clashes split job with these attributes.                   */
/*                                                                           */
/*****************************************************************************/

KHE_AVOID_CLASHES_SPLIT_JOB KheAvoidClashesSplitJobMake(
  KHE_AVOID_CLASHES_CONSTRAINT c, HA_ARENA a)
{
  KHE_AVOID_CLASHES_SPLIT_JOB res;
  HaMake(res, a);
  res->tag = KHE_AVOID_CLASHES_SPLIT_JOB_TAG;
  res->priority = KheConstraintCombinedWeight((KHE_CONSTRAINT) c);
  res->constraint = c;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesSplitJobFree(KHE_AVOID_CLASHES_SPLIT_JOB st)         */
/*                                                                           */
/*  Free st.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheAvoidClashesSplitJobFree(KHE_AVOID_CLASHES_SPLIT_JOB st)
{
  MFree(st);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerAddEvent(KHE_LAYER layer, KHE_EVENT e)                      */
/*                                                                           */
/*  Either add every solution event to layer and return true, or else        */
/*  add none of them and return false.                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheLayerAddEvent(KHE_LAYER layer, KHE_EVENT e)
{
  KHE_SOLN soln;  int i, j;  KHE_SOLN_EVENT se;
  soln = KheLayerSoln(layer);
  for( i = 0;  i < KheEventSolnEventCount(soln, e);  i++ )
  {
    se = KheEventSolnEvent(soln, e, i);
    if( !KheLayerAddSolnEvent(layer, se) )
    {
      ** failure, undo everything and return false **
      for( j = 0;  j < i;  j++ )
      {
	se = KheEventSolnEvent(soln, e, j);
	KheLayerDeleteSolnEvent(layer, se);
      }
      return false;
    }
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerDeleteEvent(KHE_LAYER layer, KHE_EVENT e)                   */
/*                                                                           */
/*  Delete every solution event of e from layer.                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheLayerDeleteEvent(KHE_LAYER layer, KHE_EVENT e)
{
  KHE_SOLN soln;  int i;  KHE_SOLN_EVENT se;
  soln = KheLayerSoln(layer);
  for( i = 0;  i < KheEventSolnEventCount(soln, e);  i++ )
  {
    se = KheEventSolnEvent(soln, e, i);
    KheLayerDeleteSolnEvent(layer, se);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesSplitJobTryResource(KHE_RESOURCE r,                  */
/*    KHE_SPLIT_FOREST sf)                                                   */
/*                                                                           */
/*  Try to carry out st on r.                                                */
/*                                                                           */
/*  If some events won't add, at present they just get left out.             */
/*                                                                           */
/*****************************************************************************/

static void KheAvoidClashesSplitJobTryResource(KHE_RESOURCE r,
  KHE_SPLIT_FOREST sf, HA_ARENA a)
{
  KHE_EVENT e;  int i;  bool success;
  if( DEBUG1 )
    fprintf(stderr, "  [ KheAvoidClashesSplitJobTryResource(%s, sf)\n",
      KheResourceId(r) != NULL ? KheResourceId(r) : "-");
  for( i = 0;  i < KheResourcePreassignedEventResourceCount(r);  i++ )
  {
    e = KheEventResourceEvent(KheResourcePreassignedEventResource(r, i));
    if( DEBUG1 )
      fprintf(stderr, "    trying event %s (duration %d):\n",
	KheEventId(e) != NULL ? KheEventId(e) : "-", KheEventDuration(e));

    /* add e's layer membership to the forest, if it will accept it */
    success = KheSplitForestTryAddToResource(sf, e, r, a);
    if( DEBUG1 )
      fprintf(stderr, "    %s\n", success ? "success" : "failure");
  }
  if( DEBUG1 )
    fprintf(stderr, "  ] KheAvoidClashesSplitJobTryResource returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesSplitJobTryResource(KHE_RESOURCE r,                  */
/*    KHE_SPLIT_FOREST sf, ARRAY_KHE_LAYER *layers)                          */
/*                                                                           */
/*  Try to carry out st on r.  Parameter *layers is a scratch temp.          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheAvoidClashesSplitJobTryResource(KHE_RESOURCE r,
  KHE_SPLIT_FOREST sf, ARRAY_KHE_LAYER *layers)
{
  KHE_LAYER layer;  KHE_EVENT e;  int i, j;  bool done;
  if( DEBUG1 )
    fprintf(stderr, "  [ KheAvoidClashesSplitJobTryResource(st, %s, sf)\n",
      KheResourceId(r) != NULL ? KheResourceId(r) : "-");
  MArrayClear(*layers);
  for( i = 0;  i < KheResourcePreassignedEventResourceCount(r);  i++ )
  {
    e = KheEventResourceEvent(KheResourcePreassignedEventResource(r, i));
    if( DEBUG1 )
      fprintf(stderr, "    trying event %s (duration %d):\n",
	KheEventId(e) != NULL ? KheEventId(e) : "-", KheEventDuration(e));

    ** add e to the first existing layer that will accept it **
    done = false;
    MArrayForEach(*layers, layer, j)
    {
      ** try to add e to layer, in soln and forest **
      if( DEBUG1 )
	fprintf(stderr, "      layer %d (duration %d) ", j,
	  KheLayerDuration(layer));
      if( KheLayerAddEvent(layer, e) )
      {
	if( !KheSplitForestTryAddToLayer(sf, e, layer) )
	{
	  if( DEBUG1 )
	    fprintf(stderr, "  KheSplitForestTryAddToLayer failed\n");
          KheLayerDeleteEvent(layer, e);
	}
	else
	{
	  if( DEBUG1 )
	    fprintf(stderr, "  success\n");
	  done = true;
	  break;
	}
      }
      else
	if( DEBUG1 )
	  fprintf(stderr, "  KheLayerAddEvent failed\n");
    }

    ** not done, so make a new layer and try to add e to that **
    if( !done )
    {
      layer = KheLayerMake(KheSplitForestSoln(sf), r);
      if( DEBUG1 )
	fprintf(stderr, "      new layer (duration %d) ",
	  KheLayerDuration(layer));
      if( KheLayerAddEvent(layer, e) )
      {
	if( !KheSplitForestTryAddToLayer(sf, e, layer) )
	{
          KheLayerDeleteEvent(layer, e);
	  KheLayerDelete(layer);
	  if( DEBUG1 )
	    fprintf(stderr, "  KheSplitForestTryAddToLayer failed\n");
	}
	else
	{
	  if( DEBUG1 )
	    fprintf(stderr, "  success\n");
	  MArrayAddLast(*layers, layer);
	}
      }
      else
      {
	if( DEBUG1 )
	  fprintf(stderr, "  KheLayerAddEvent failed\n");
	KheLayerDelete(layer);
      }
    }
  }
  if( DEBUG1 )
    fprintf(stderr, "  ] KheAvoidClashesSplitJobTryResource returning\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,         */
/*    int start, int stop, KHE_SPLIT_FOREST sf)                              */
/*                                                                           */
/*  Try avoid clashes jobs *split_jobs[start .. stop-1] on sf.               */
/*                                                                           */
/*****************************************************************************/

void KheAvoidClashesSplitJobTry(ARRAY_KHE_SPLIT_JOB *split_jobs,
  int start, int stop, KHE_SPLIT_FOREST sf, HA_ARENA a)
{
  int i, j, k;  KHE_RESOURCE r;  KHE_RESOURCE_GROUP rg;
  KHE_AVOID_CLASHES_SPLIT_JOB st;
  for( k = start;  k < stop;  k++ )
  {
    st = (KHE_AVOID_CLASHES_SPLIT_JOB) HaArray(*split_jobs, k);
    for(i=0; i<KheAvoidClashesConstraintResourceGroupCount(st->constraint); i++)
    {
      rg = KheAvoidClashesConstraintResourceGroup(st->constraint, i);
      if( DEBUG1 )
	fprintf(stderr, "[ KheAvoidClashesSplitJobTry at rg %s\n",
	  KheResourceGroupId(rg) == NULL ? "-" : KheResourceGroupId(rg));
      for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
      {
	r = KheResourceGroupResource(rg, j);
	KheAvoidClashesSplitJobTryResource(r, sf /* , &layers */, a);
      }
      if( DEBUG1 )
	fprintf(stderr, "]\n");
    }
    for( i=0; i < KheAvoidClashesConstraintResourceCount(st->constraint);  i++ )
    {
      r = KheAvoidClashesConstraintResource(st->constraint, i);
      if( DEBUG1 )
	fprintf(stderr, "[ KheAvoidClashesSplitJobTry at r %s\n",
	  KheResourceId(r) == NULL ? "-" : KheResourceId(r));
      KheAvoidClashesSplitJobTryResource(r, sf /* , &layers */, a);
      if( DEBUG1 )
	fprintf(stderr, "]\n");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidClashesSplitJobDebug(KHE_AVOID_CLASHES_SPLIT_JOB st,        */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of job st onto fp with the given indent.                     */
/*                                                                           */
/*****************************************************************************/

void KheAvoidClashesSplitJobDebug(KHE_AVOID_CLASHES_SPLIT_JOB st,
  int indent, FILE *fp, HA_ARENA a)
{
  fprintf(fp, "%*s[ %.5f Avoid Clashes Split Job %s ]\n", indent, "",
    KheCostShow(st->priority), KheConstraintId((KHE_CONSTRAINT)st->constraint));
}

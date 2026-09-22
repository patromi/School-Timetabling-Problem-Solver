
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
/*  FILE:         khe_sr_classify.c                                          */
/*  DESCRIPTION:  Classify resources by workload                             */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>
#include <float.h>

#define DEBUG1 1

/*****************************************************************************/
/*                                                                           */
/*  KHE_CLASSIFY - information for classifying one resource                  */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_classify_rec {
  KHE_RESOURCE		resource;			/* the resource      */
  int			busy_times;			/* MaxBusyTimes      */
  float			workload;			/* MaxWorkload       */
  bool			is_maximal;			/* true if maximal   */
} *KHE_CLASSIFY;

typedef HA_ARRAY(KHE_CLASSIFY) ARRAY_KHE_CLASSIFY;


/*****************************************************************************/
/*                                                                           */
/*  KHE_CLASSIFY KheClassifyMake(KHE_RESOURCE r, int busy_times,             */
/*    float workload, HA_ARENA a)                                            */
/*                                                                           */
/*  Make a new KHE_CLASSIFY object with these attributes.                    */
/*                                                                           */
/*****************************************************************************/

static KHE_CLASSIFY KheClassifyMake(KHE_RESOURCE r, int busy_times,
  float workload, HA_ARENA a)
{
  KHE_CLASSIFY res;
  HaMake(res, a);
  res->resource = r;
  res->busy_times = busy_times;
  res->workload = workload;
  res->is_maximal = false;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClassifyDebug(KHE_CLASSIFY cl, int indent, FILE *fp)             */
/*                                                                           */
/*  Debug print of cl onto fp with the given indent.                         */
/*                                                                           */
/*****************************************************************************/

static void KheClassifyDebug(KHE_CLASSIFY cl, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "[%s, ", KheResourceId(cl->resource));
  if( cl->busy_times == INT_MAX )
    fprintf(fp, "INT_MAX");
  else
    fprintf(fp, "%d", cl->busy_times);
  fprintf(fp, ", ");
  if( cl->workload == FLT_MAX )
    fprintf(fp, "FLT_MAX");
  else
    fprintf(fp, "%.1f", cl->workload);
  fprintf(fp, "]");
  if( indent > 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClassifyResourcesByWorkload(KHE_SOLN soln, KHE_RESOURCE_GROUP rg,*/
/*    KHE_RESOURCE_GROUP *rg1, KHE_RESOURCE_GROUP *rg2)                      */
/*                                                                           */
/*  Classify rg's resources into high-workload (*rg1) and the rest (*rg2).   */
/*                                                                           */
/*****************************************************************************/

bool KheClassifyResourcesByWorkload(KHE_SOLN soln, KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE_GROUP *rg1, KHE_RESOURCE_GROUP *rg2)
{
  HA_ARENA a;  int i, busy_times, max_busy_times, max_count;  KHE_RESOURCE r;
  float workload, max_workload;
  ARRAY_KHE_CLASSIFY classifies;  KHE_CLASSIFY cl;  KHE_RESOURCE_TYPE rt;
  if( DEBUG1 )
    fprintf(stderr, "[ KheClassifyResourcesByWorkload(soln, %s, -, -)\n",
      KheResourceGroupId(rg));

  /* sort out each resource of rg; set max_busy_times and max_workload */
  a = KheSolnArenaBegin(soln);
  HaArrayInit(classifies, a);
  max_busy_times = 0;
  max_workload = 0.0;
  for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
  {
    r = KheResourceGroupResource(rg, i);
    KheResourceMaxBusyTimes(soln, r, &busy_times);
    if( busy_times > max_busy_times )
      max_busy_times = busy_times;
    KheResourceMaxWorkload(soln, r, &workload);
    if( workload > max_workload )
      max_workload = workload;
    HaArrayAddLast(classifies, KheClassifyMake(r, busy_times, workload, a));
  }

  /* set is_maximal in classifies of maximal busy times; set max_count */
  max_count = 0;
  HaArrayForEach(classifies, cl, i)
    if( cl->busy_times == max_busy_times && cl->workload == max_workload )
      cl->is_maximal = true, max_count++;

  /* succeed if neither all nor none of rg are maximal */
  if( max_count > 0 && max_count < HaArrayCount(classifies) )
  {
    /* set *rg1 to the maximal elements */
    if( DEBUG1 )
      fprintf(stderr, "  maximal resources:\n");
    rt = KheResourceGroupResourceType(rg);
    KheSolnResourceGroupBegin(soln, rt);
    HaArrayForEach(classifies, cl, i)
      if( cl->is_maximal )
      {
	KheSolnResourceGroupAddResource(soln, cl->resource);
	if( DEBUG1 )
	  KheClassifyDebug(cl, 4, stderr);
      }
    *rg1 = KheSolnResourceGroupEnd(soln);

    /* set *rg2 to the non-maximal elements */
    if( DEBUG1 )
      fprintf(stderr, "  non-maximal resources:\n");
    KheSolnResourceGroupBegin(soln, rt);
    HaArrayForEach(classifies, cl, i)
      if( ! cl->is_maximal )
      {
	KheSolnResourceGroupAddResource(soln, cl->resource);
	if( DEBUG1 )
	  KheClassifyDebug(cl, 4, stderr);
      }
    *rg2 = KheSolnResourceGroupEnd(soln);

    if( DEBUG1 )
      fprintf(stderr, "] KheClassifyResourcesByWorkload returning true\n");
  }
  else
  {
    *rg1 = *rg2 = NULL;
    if( DEBUG1 )
      fprintf(stderr, "] KheClassifyResourcesByWorkload returning false\n");
  }

  KheSolnArenaEnd(soln, a);
  return *rg1 != NULL;
}

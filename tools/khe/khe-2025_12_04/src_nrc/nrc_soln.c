
/*****************************************************************************/
/*                                                                           */
/*  THE NRCONV NURSE ROSTERING TO XESTT CONVERTER                            */
/*  COPYRIGHT (C) 2016, Jeffrey H. Kingston                                  */
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
/*  FILE:         nrc_soln.c                                                 */
/*  DESCRIPTION:  A solution                                                 */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG4 0


/*****************************************************************************/
/*                                                                           */
/*  NRC_SOLN_SHIFT - one shift's worth of solutions                          */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_WORKER) ARRAY_NRC_WORKER;

typedef struct nrc_soln_shift_rec {
  NRC_SHIFT		shift;
  ARRAY_NRC_WORKER	workers;
} *NRC_SOLN_SHIFT;


/*****************************************************************************/
/*                                                                           */
/*  NRC_SOLN_DAY - one day's worth of solutions                              */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_SOLN_SHIFT) ARRAY_NRC_SOLN_SHIFT;

typedef struct nrc_soln_day_rec {
  NRC_DAY		day;
  ARRAY_NRC_SOLN_SHIFT	soln_shifts;
} *NRC_SOLN_DAY;

typedef HA_ARRAY(NRC_SOLN_DAY) ARRAY_NRC_SOLN_DAY;


/*****************************************************************************/
/*                                                                           */
/*  NRC_SOLN - a solution                                                    */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_SOLN_GROUP) ARRAY_NRC_SOLN_GROUP;

struct nrc_soln_rec {
  HA_ARENA			arena;			/* arena             */
  NRC_INSTANCE			instance;		/* instance solved   */
  ARRAY_NRC_SOLN_GROUP		soln_groups;		/* soln groups       */
  char				*description;		/* description       */
  float				running_time;		/* running time      */
  ARRAY_NRC_SOLN_DAY		soln_days;		/* soln days         */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcSolnAddSolnGroup(NRC_SOLN soln, NRC_SOLN_GROUP soln_group)       */
/*                                                                           */
/*  Add soln_group to soln; soln's instance is in soln_group's archive.      */
/*                                                                           */
/*****************************************************************************/

void NrcSolnAddSolnGroup(NRC_SOLN soln, NRC_SOLN_GROUP soln_group)
{
  HaArrayAddLast(soln->soln_groups, soln_group);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcSolnDeleteSolnGroup(NRC_SOLN soln, NRC_SOLN_GROUP soln_group)    */
/*                                                                           */
/*  Delete soln_group from soln's list of soln_groups.  It must be present.  */
/*                                                                           */
/*****************************************************************************/

void NrcSolnDeleteSolnGroup(NRC_SOLN soln, NRC_SOLN_GROUP soln_group)
{
  int pos;
  if( !HaArrayContains(soln->soln_groups, soln_group, &pos) )
    HnAssert(false, "NrcSolnDeleteSolnGroup internal error");
  HaArrayDeleteAndShift(soln->soln_groups, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SOLN_SHIFT NrcSolnShiftMake(NRC_SHIFT s, HA_ARENA a)                 */
/*                                                                           */
/*  Make a soln_shift object with these attributes.                          */
/*                                                                           */
/*****************************************************************************/

static NRC_SOLN_SHIFT NrcSolnShiftMake(NRC_SHIFT s, HA_ARENA a)
{
  NRC_SOLN_SHIFT res;
  HaMake(res, a);
  res->shift = s;
  HaArrayInit(res->workers, a);
  HaArrayFill(res->workers, NrcShiftDemandCount(s), NULL);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SOLN_DAY NrcSolnDayMake(NRC_INSTANCE ins, NRC_DAY d, HA_ARENA a)     */
/*                                                                           */
/*  Make a soln_day object with these attributes.                            */
/*                                                                           */
/*****************************************************************************/

static NRC_SOLN_DAY NrcSolnDayMake(NRC_INSTANCE ins, NRC_DAY d, HA_ARENA a)
{
  NRC_SOLN_DAY res;  int i;  NRC_SHIFT_TYPE st;  NRC_SOLN_SHIFT ss;
  HaMake(res, a);
  res->day = d;
  HaArrayInit(res->soln_shifts, a);
  for( i = 0;  i < NrcInstanceShiftTypeCount(ins);  i++ )
  {
    st = NrcInstanceShiftType(ins, i);
    ss = NrcSolnShiftMake(NrcDayShiftFromShiftType(d, st), a);
    HaArrayAddLast(res->soln_shifts, ss);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SOLN NrcSolnMake(NRC_INSTANCE ins)                                   */
/*                                                                           */
/*  Make and return a new soln of instance, with no meets, no nodes, and     */
/*  no layers.                                                               */
/*                                                                           */
/*****************************************************************************/

NRC_SOLN NrcSolnMake(NRC_INSTANCE ins, HA_ARENA_SET as)
{
  NRC_SOLN res;  NRC_SOLN_DAY sd;  NRC_DAY d;  int i;  HA_ARENA a;
  HnAssert(NrcInstanceComplete(ins),
    "NrcSolnMake called on instance lacking NrcIinstanceMakeEnd");
  a = HaArenaMake(as);
  HaMake(res, a);
  res->arena = a;
  res->instance = ins;
  HaArrayInit(res->soln_groups, a);
  res->description = NULL;
  res->running_time = -1.0;
  if( DEBUG1 )
    fprintf(stderr, "] NrcSolnMake returning\n");
  HnAssert(res->instance != NULL, "NrcSolnMake internal error");
  HaArrayInit(res->soln_days, a);
  for( i = 0;  i < NrcInstanceCycleDayCount(ins);  i++ )
  {
    d = NrcInstanceCycleDay(ins, i);
    sd = NrcSolnDayMake(ins, d, a);
    HaArrayAddLast(res->soln_days, sd);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE NrcSolnInstance(NRC_SOLN soln)                              */
/*                                                                           */
/*  Return the instance that soln is for.                                    */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE NrcSolnInstance(NRC_SOLN soln)
{
  return soln->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcSolnSetDescription(NRC_SOLN soln, char *description)             */
/*                                                                           */
/*  Set or reset the Description attribute of soln.                          */
/*                                                                           */
/*****************************************************************************/

void NrcSolnSetDescription(NRC_SOLN soln, char *description)
{
  soln->description = HnStringCopy(description, soln->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcSolnDescription(NRC_SOLN soln)                                  */
/*                                                                           */
/*  Return the Description attribute of soln.                                */
/*                                                                           */
/*****************************************************************************/

char *NrcSolnDescription(NRC_SOLN soln)
{
  return soln->description;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcSolnSetRunningTime(NRC_SOLN soln, float running_time)            */
/*                                                                           */
/*  Set or reset the RunningTime attribute of soln.                          */
/*                                                                           */
/*****************************************************************************/

void NrcSolnSetRunningTime(NRC_SOLN soln, float running_time)
{
  soln->running_time = running_time;
}


/*****************************************************************************/
/*                                                                           */
/*  float NrcSolnRunningTime(NRC_SOLN soln)                                  */
/*                                                                           */
/*  Return the RunningTime attribute of soln.                                */
/*                                                                           */
/*****************************************************************************/

float NrcSolnRunningTime(NRC_SOLN soln)
{
  return soln->running_time;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "assignments"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_SOLN_SHIFT NrcSolnShiftGet(NRC_SOLN soln, NRC_SHIFT s, int i)        */
/*                                                                           */
/*  Get the soln_shift of soln corresponding to s.  Also check that i        */
/*  is in range.                                                             */
/*                                                                           */
/*****************************************************************************/

static NRC_SOLN_SHIFT NrcSolnShiftGet(NRC_SOLN soln, NRC_SHIFT s, int i)
{
  int j, k;  NRC_SOLN_SHIFT res;  NRC_SOLN_DAY sd;
  HnAssert(NrcShiftInstance(s) == soln->instance,
    "NrcSolnShiftGet:  soln and shift come from different instances");

  /* get the soln_day */
  j = NrcDayIndexInCycle(NrcShiftDay(s));
  HnAssert(j >= 0 && j < HaArrayCount(soln->soln_days),
    "NrcSolnShiftGet internal error 1");
  sd = HaArray(soln->soln_days, j);

  /* get the soln_shift */
  k = NrcShiftTypeIndex(NrcShiftType(s));
  HnAssert(k >= 0 && k < HaArrayCount(sd->soln_shifts),
    "NrcSolnShiftGet internal error 2");
  res = HaArray(sd->soln_shifts, k);
  HnAssert(res->shift == s, "NrcSolnShiftGet internal error 3");

  /* check i and return the soln_shift */
  HnAssert(i >= 0 && i < HaArrayCount(res->workers),
    "NrcSolnShiftGet: i (%d) out of range (0 .. %d)", i,
    HaArrayCount(res->workers));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcSolnAddAssignment(NRC_SOLN soln, NRC_SHIFT s, int i,NRC_WORKER w)*/
/*                                                                           */
/*  In soln, assign w to the demand of shift s whose index is i.             */
/*                                                                           */
/*****************************************************************************/

void NrcSolnAddAssignment(NRC_SOLN soln, NRC_SHIFT s, int i, NRC_WORKER w)
{
  NRC_SOLN_SHIFT ss;
  ss = NrcSolnShiftGet(soln, s, i);
  HnAssert(HaArray(ss->workers, i) == NULL,
    "NrcSolnAddAssignment:  worker already assigned");
  HaArrayPut(ss->workers, i, w);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER NrcSolnAssignment(NRC_SOLN soln, NRC_SHIFT s,                 */
/*    int demand_index)                                                      */
/*                                                                           */
/*  Return the worker assigned in soln to the demand of shift s whose index  */
/*  is i, or NULL if that demand is unassigned.                              */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER NrcSolnAssignment(NRC_SOLN soln, NRC_SHIFT s, int i)
{
  NRC_SOLN_SHIFT ss;
  ss = NrcSolnShiftGet(soln, s, i);
  return HaArray(ss->workers, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "conversion"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN NrcSolnConvert(NRC_SOLN soln)                                   */
/*                                                                           */
/*  Convert NRC solution soln into a KHE solution.                           */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN NrcSolnConvert(NRC_SOLN soln)
{
  KHE_SOLN res;  NRC_SOLN_DAY sd;  NRC_SOLN_SHIFT ss;  int i, j, k;
  NRC_WORKER w;  KHE_EVENT e;  KHE_MEET m;  KHE_TASK task;
  KHE_RESOURCE_TYPE rt;  KHE_INSTANCE ins;  KHE_EVENT_RESOURCE er;
  if( DEBUG2 )
    fprintf(stderr, "[ NrcSolnConvert(soln)\n");
  ins = NrcInstanceKheInstance(soln->instance);
  res = KheSolnMake(ins, NULL);
  if( DEBUG2 )
    fprintf(stderr, "  after KheSolnMake\n");
  rt = KheInstanceResourceType(ins, 0);
  HaArrayForEach(soln->soln_days, sd, i)
  {
    if( DEBUG2 )
      fprintf(stderr, "  converting day %d (%d shifts)\n", i,
	HaArrayCount(sd->soln_shifts));
    HaArrayForEach(sd->soln_shifts, ss, j)
    {
      e = NrcShiftEvent(ss->shift);
      HnAssert(HaArrayCount(ss->workers) == KheEventResourceCount(e),
	"NrcSolnConvert internal error");
      m = KheMeetMake(res, 1, e);
      KheMeetAssignTime(m, NrcShiftTime(ss->shift));
      HaArrayForEach(ss->workers, w, k)
      {
	er = KheEventResource(e, k);
	task = KheTaskMake(res, rt, m, er);
	if( w != NULL )
	  KheTaskAssignResource(task, NrcWorkerResource(w));
      }
    }
  }
  if( soln->description != NULL )
    KheSolnSetDescription(res, soln->description);
  if( soln->running_time >= 0.0 )
    KheSolnSetRunningTime(res, soln->running_time);
  if( DEBUG2 )
    fprintf(stderr, "] NrcSolnConvert returning\n");
  return res;
}

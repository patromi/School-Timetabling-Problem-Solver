
/*****************************************************************************/
/*                                                                           */
/*  THE NRCONV NURSE ROSTERING TO XHSTT CONVERTER                            */
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
/*  FILE:         nrc_shift.c                                                */
/*  MODULE:       A shift                                                    */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

#define DEBUG1 0
#define DEBUG2 0


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT                                                                */
/*                                                                           */
/*  A shift.                                                                 */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_DEMAND) ARRAY_NRC_DEMAND;
typedef HA_ARRAY(NRC_DEMAND_SET) ARRAY_NRC_DEMAND_SET;
typedef HA_ARRAY(NRC_WORKER) ARRAY_NRC_WORKER;
typedef HA_ARRAY(NRC_DEMAND_CONSTRAINT) ARRAY_NRC_DEMAND_CONSTRAINT;

struct nrc_shift_rec {
  NRC_INSTANCE			instance;
  NRC_DAY			day;
  NRC_SHIFT_TYPE		shift_type;
  ARRAY_NRC_DEMAND		demands;
  ARRAY_NRC_WORKER		preassignments;
  NRC_SHIFT_SET			singleton_shift_set;
  int				index;
  ARRAY_NRC_DEMAND_CONSTRAINT	demand_constraints;
  KHE_TIME			time;
  KHE_EVENT			event;
};


/*****************************************************************************/
/*                                                                           */
/*  int NrcShiftTypedCmp(NRC_SHIFT s1, NRC_SHIFT s2)                         */
/*                                                                           */
/*  Typed comparison function for sorting shifts into increasing canonical   */
/*  order:  day first, then shift type.                                      */
/*                                                                           */
/*****************************************************************************/

int NrcShiftTypedCmp(NRC_SHIFT s1, NRC_SHIFT s2)
{
  return s1->index - s2->index;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcShiftCmp(const void *p1, const void *p2)                          */
/*                                                                           */
/*  Untyped comparison function for sorting an array of shifts.              */
/*                                                                           */
/*****************************************************************************/

int NrcShiftCmp(const void *p1, const void *p2)
{
  NRC_SHIFT s1 = * (NRC_SHIFT *) p1;
  NRC_SHIFT s2 = * (NRC_SHIFT *) p2;
  return NrcShiftTypedCmp(s1, s2);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT NrcShiftMake(NRC_INSTANCE ins, NRC_DAY day, NRC_SHIFT_TYPE st) */
/*                                                                           */
/*  Make a shift and add it to ins, day, and st.                             */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT NrcShiftMake(NRC_INSTANCE ins, NRC_DAY day, NRC_SHIFT_TYPE st)
{
  NRC_SHIFT res;  HA_ARENA a;
  if( DEBUG1 )
    fprintf(stderr, "[ NrcShiftMake(ins, %s, %s)\n", NrcDayShortName(day),
      NrcShiftTypeName(st));
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->instance = ins;
  res->day = day;
  res->shift_type = st;
  HaArrayInit(res->demands, a);
  HaArrayInit(res->preassignments, a);
  res->singleton_shift_set = NrcShiftSetMake(ins);
  NrcShiftSetAddShift(res->singleton_shift_set, res);
  HaArrayInit(res->demand_constraints, a);
  res->time = NULL;
  res->event = NULL;
  res->index = NrcInstanceAddShift(ins, res);
  NrcDayAddShift(day, res);
  NrcShiftTypeAddShift(st, res);
  if( DEBUG1 )
    fprintf(stderr, "] NrcShiftMake returning\n");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE NrcShiftInstance(NRC_SHIFT s)                               */
/*                                                                           */
/*  Return the instance containing s.                                        */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE NrcShiftInstance(NRC_SHIFT s)
{
  return s->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY NrcShiftDay(NRC_SHIFT s)                                         */
/*                                                                           */
/*  Return the day attribute of s.                                           */
/*                                                                           */
/*****************************************************************************/

NRC_DAY NrcShiftDay(NRC_SHIFT s)
{
  return s->day;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcShiftName(NRC_SHIFT s)                                          */
/*                                                                           */
/*  Return the name attribute of s.                                          */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_TYPE NrcShiftType(NRC_SHIFT s)
{
  return s->shift_type;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcShiftSingletonShiftSet(NRC_SHIFT s)                     */
/*                                                                           */
/*  Return a shift-set containing just s.                                    */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcShiftSingletonShiftSet(NRC_SHIFT s)
{
  return s->singleton_shift_set;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcShiftIndex(NRC_SHIFT s)                                           */
/*                                                                           */
/*  Return the index of s.                                                   */
/*                                                                           */
/*****************************************************************************/

int NrcShiftIndex(NRC_SHIFT s)
{
  return s->index;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "demand constraint count"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftAddDemandConstraint(NRC_SHIFT s, NRC_DEMAND_CONSTRAINT dc)  */
/*                                                                           */
/*  Increment the demand constraint count of s.                              */
/*                                                                           */
/*****************************************************************************/

void NrcShiftAddDemandConstraint(NRC_SHIFT s, NRC_DEMAND_CONSTRAINT dc)
{
  HaArrayAddLast(s->demand_constraints, dc);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcShiftDemandConstraintCount(NRC_SHIFT s)                           */
/*                                                                           */
/*  Return the demand constraint count of s.                                 */
/*                                                                           */
/*****************************************************************************/

int NrcShiftDemandConstraintCount(NRC_SHIFT s)
{
  return HaArrayCount(s->demand_constraints);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcDemandConstraintBoundIsNonTrivial(NRC_DEMAND_CONSTRAINT dc)      */
/*                                                                           */
/*  Return true if dc's bound is non-trivial.                                */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool NrcBoundIsNonTrivial(NRC_BOUND b)
{
  int min_value;  bool allow_zero;  NRC_PENALTY below_min_penalty;
  int max_value;  NRC_PENALTY above_max_penalty;
  return NrcBoundMin(b, &min_value, &allow_zero, &below_min_penalty) ||
    NrcBoundMax(b, &max_value, &above_max_penalty);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool NrcShiftBoundsMergeable(NRC_SHIFT s, NRC_BOUND *b)                  */
/*                                                                           */
/*  If the bounds of the demand constraints of s are mergeable, return       */
/*  true and set *b to the merged bound.  Otherwise return false.            */
/*                                                                           */
/*  This function ignores demand constraints whose bounds are empty.         */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete
static bool NrcShiftBoundsMergeable(NRC_SHIFT s, NRC_BOUND *b)
{
  NRC_BOUND bounds[4], b2;  int bound_count, i, j;  NRC_DEMAND_CONSTRAINT dc;

  ** get the bounds; return if there are too many **
  bound_count = HaArrayCount(s->demand_constraints);
  if( bound_count > 4 )
    return *b = NULL, false;
  j = 0;
  HaArrayForEach(s->demand_constraints, dc, i)
  {
    b2 = NrcDemandConstraintBound(dc);
    if( NrcBoundIsNonTrivial(b2) )
      bounds[j++] = b2;
  }

  ** try the merge **
  return NrcBoundSpecialMerge(bounds, j, s->instance, b);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool NrcDemandConstraintCoversOneShift(NRC_DEMAND_CONSTRAINT dc)         */
/*                                                                           */
/*  Return true if dc applies to exactly one shift.                          */
/*                                                                           */
/*****************************************************************************/

static bool NrcDemandConstraintCoversOneShift(NRC_DEMAND_CONSTRAINT dc)
{
  return NrcShiftSetShiftCount(NrcDemandConstraintShiftSet(dc)) == 1;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftReplaceDemandConstraintsByDemands(NRC_SHIFT s,              */
/*    NRC_DC_CONVERTER dcc)                                                  */
/*                                                                           */
/*  If s can use demands instead of demand constraints, make the replacement */
/*  using dcc.  Otherwise change nothing.                                    */
/*                                                                           */
/*****************************************************************************/

void NrcShiftReplaceDemandConstraintsByDemands(NRC_SHIFT s,
  NRC_DC_CONVERTER dcc)
{
  NRC_DEMAND d;  int i;  NRC_DEMAND_SET ds;  NRC_DEMAND_CONSTRAINT dc;
  if( DEBUG2 )
  {
    fprintf(stderr, "[ NrcShiftReplaceDemandConstraintsByDemands(");
    NrcShiftDebug(s, -1, stderr);
    fprintf(stderr, ")\n");
    HaArrayForEach(s->demands, d, i)
      NrcDemandDebug(d, 0, 2, stderr);
    HaArrayForEach(s->demand_constraints, dc, i)
      NrcDemandConstraintDebug(dc, 2, stderr);
  }

  /* there must be at least one demand */
  if( HaArrayCount(s->demands) == 0 )
  {
    if( DEBUG2 )
      fprintf(stderr, "] NrcShiftReplaceDemandConstraintsByDemands returning "
	"(no demands)\n");
    return;
  }

  /* all demands must have no penalizers */
  HaArrayForEach(s->demands, d, i)
    if( NrcDemandPenalizerCount(d) > 0 )
    {
      if( DEBUG2 )
	fprintf(stderr, "] NrcShiftReplaceDemandConstraintsByDemands returning "
	  "(preferences)\n");
      return;
    }

  /* all demands must currently have weight 0 for not assigned and assigned */
  /* *** obsolete
  HaArrayForEach(s->demands, d, i)
  {
    p1 = NrcDemandNotAssignedPenalty(d);
    p2 = NrcDemandAssignedPenalty(d);
    if( NrcPenaltyWeight(p1) != 0 || NrcPenaltyWeight(p2) != 0 )
    {
      if( DEBUG2 )
      {
	HA_ARENA a = NrcInstanceArena(s->instance);
	fprintf(stderr, "] NrcShiftReplaceDemandConstraintsByDemands "
	  "returning (%d: not_assigned %s, assigned %s)\n",
	  i, NrcPenaltyShow(p1, a), NrcPenaltyShow(p2, a));
      }
      return;
    }
  }

  ** all the preferred_ws and not_preferred_penalty values must be equal **
  d = HaArrayFirst(s->demands);
  first_ws = NrcDemandPreferredWorkerSet(d);
  first_p = NrcDemandNotPreferredPenalty(d);
  for( i = 1;  i < HaArrayCount(s->demands);  i++ )
  {
    d = HaArray(s->demands, i);
    if( !NrcWorkerSetEqual(first_ws, NrcDemandPreferredWorkerSet(d)) )
    {
      if( DEBUG2 )
	fprintf(stderr,
	  "] NrcShiftReplaceDemandConstraintsByDemands returning "
	  "(worker set %d)\n", i);
      return;
    }
    if( !NrcPenaltyEqual(first_p, NrcDemandNotPreferredPenalty(d)) )
    {
      if( DEBUG2 )
	fprintf(stderr,
	  "] NrcShiftReplaceDemandConstraintsByDemands returning "
	  "(not_preferred %d)\n", i);
      return;
    }
  }
  *** */

  /* the demand constraints for this shift must be suitable */
  HaArrayForEach(s->demand_constraints, dc, i)
    if( !NrcDemandConstraintCoversOneShift(dc) )
    {
      if( DEBUG2 )
      {
	fprintf(stderr, "] NrcShiftReplaceDemandConstraintsByDemands returning "
	  "(bad ");
	NrcDemandConstraintDebug(dc, -1, stderr);
	fprintf(stderr, ")\n");
      }
      return;
    }

  /* load the demand constraint converter and see what we get */
  NrcDCConverterClear(dcc);
  HaArrayForEach(s->demand_constraints, dc, i)
    NrcDCConverterAddDemandConstraint(dcc, dc);
  if( NrcDCConverterSolve(dcc, HaArrayCount(s->demands), &ds) )
  {
    /* mark the demand constraints defunct */
    HaArrayForEach(s->demand_constraints, dc, i)
      NrcDemandConstraintSetDefunct(dc);
    HaArrayClear(s->demand_constraints);

    /* replace the current demands by the demand set returned by the solve */
    HaArrayClear(s->demands);
    NrcShiftAddDemandSet(s, ds);
    if( DEBUG2 )
      fprintf(stderr, "] NrcShiftReplaceDemandConstraintsByDemands returning "
	"(done)\n");
  }
  else
  {
    if( DEBUG2 )
      fprintf(stderr, "] NrcShiftReplaceDemandConstraintsByDemands returning "
	"(fail)\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcShiftTryDemands(NRC_SHIFT s, NRC_BOUND b)                        */
/*                                                                           */
/*  If s can use demands, do so and return true.                             */
/*                                                                           */
/*  This will be the case if s is subject to a single demand constraint,     */
/*  and s's demands all have weight 0 apart from preferred worker-sets.      */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete (now using NrcShiftReplaceDemandConstraintsByDemands above)
bool NrcShiftTryDemands(NRC_SHIFT s)
{
  NRC_DEMAND d;  int i;  NRC_PENALTY first_p, p1, p2;  NRC_WORKER_SET first_ws;
  NRC_DEMAND_SET ds;  NRC_DEMAND_CONSTRAINT dc;
  if( DEBUG2 )
  {
    fprintf(stderr, "NrcShiftTryDemands(");
    NrcShiftDebug(s, -1, stderr);
    fprintf(stderr, ", %s) %d demands:\n", NrcBoundShow(b),
      HaArrayCount(s->demands));
    HaArrayForEach(s->demands, d, i)
      NrcDemandDebug(d, 2, stderr);
    HaArrayForEach(s->demand_constraints, dc, i)
      NrcDemandConstraintDebug(dc, 2, stderr);
  }

  ** there must be at least one demand **
  if( HaArrayCount(s->demands) == 0 )
  {
    if( DEBUG2 )
      fprintf(stderr, "NrcShiftTryDemands returning false (no demands)\n");
    return false;
  }

  ** there can be at most one demand constraint referencing this shift **
  if( HaArrayCount(s->demand_constraints) != 1 )
  {
    if( DEBUG2 )
      fprintf(stderr, "NrcShiftTryDemands returning false (count %d)\n",
	HaArrayCount(s->demand_constraints));
    return false;
  }

  ** all demands must currently have weight 0 for not assigned and assigned **
  HaArrayForEach(s->demands, d, i)
  {
    p1 = NrcDemandNotAssignedPenalty(d);
    p2 = NrcDemandAssignedPenalty(d);
    if( NrcPenaltyWeight(p1) != 0 || NrcPenaltyWeight(p2) != 0 )
    {
      if( DEBUG2 )
      {
	HA_ARENA a = NrcInstanceArena(s->instance);
	fprintf(stderr, "NrcShiftTryDemands returning false (not_assigned %s,"
	  " assigned %s)\n", NrcPenaltyShow(p1, a), NrcPenaltyShow(p2, a));
      }
      return false;
    }
  }

  ** all the preferred_ws and not_preferred_penalty values must be equal **
  d = HaArrayFirst(s->demands);
  first_ws = NrcDemandPreferredWorkerSet(d);
  first_p = NrcDemandNotPreferredPenalty(d);
  for( i = 1;  i < HaArrayCount(s->demands);  i++ )
  {
    d = HaArray(s->demands, i);
    if( !NrcWorkerSetEqual(first_ws, NrcDemandPreferredWorkerSet(d)) )
    {
      if( DEBUG2 )
	fprintf(stderr, "NrcShiftTryDemands returning false (worker set %d)\n",
	  i);
      return false;
    }
    if( !NrcPenaltyEqual(first_p, NrcDemandNotPreferredPenalty(d)) )
    {
      if( DEBUG2 )
	fprintf(stderr,
	  "NrcShiftTryDemands returning false (not_preferred %d)\n", i);
      return false;
    }
  }

  ** OK, do it **
  ds = NrcDemandSetMakeFromBound(s->instance, b, HaArrayCount(s->demands),
    first_ws, first_p);
  HaArrayClear(s->demands);
  NrcShiftAddDemandSet(s, ds);
  if( DEBUG2 )
    fprintf(stderr, "NrcShiftTryDemands returning true\n");
  return true;
}
*** */



/*****************************************************************************/
/*                                                                           */
/*  bool NrcShiftTryDemands(NRC_SHIFT s, NRC_BOUND b)                        */
/*                                                                           */
/*  If s can use demands, do so and return true.                             */
/*                                                                           */
/*  This will be the case if s is subject to a single demand constraint,     */
/*  and s's demands all have weight 0 apart from preferred worker-sets.      */
/*                                                                           */
/*****************************************************************************/

/* ***
bool OldNrcShiftTryDemands(NRC_SHIFT s, NRC_BOUND b)
{
  NRC_DEMAND d;  int i;  NRC_PENALTY first_p, p1, p2;  NRC_WORKER_SET first_ws;
  NRC_DEMAND_SET ds;  NRC_DEMAND_CONSTRAINT dc;
  if( DEBUG2 )
  {
    fprintf(stderr, "NrcShiftTryDemands(");
    NrcShiftDebug(s, -1, stderr);
    fprintf(stderr, ", %s) %d demands:\n", NrcBoundShow(b),
      HaArrayCount(s->demands));
    HaArrayForEach(s->demands, d, i)
      NrcDemandDebug(d, 2, stderr);
    HaArrayForEach(s->demand_constraints, dc, i)
      NrcDemandConstraintDebug(dc, 2, stderr);
  }

  ** there must be at least one demand **
  if( HaArrayCount(s->demands) == 0 )
  {
    if( DEBUG2 )
      fprintf(stderr, "NrcShiftTryDemands returning false (no demands)\n");
    return false;
  }

  ** there can be at most one demand constraint referencing this shift **
  if( HaArrayCount(s->demand_constraints) != 1 )
  {
    if( DEBUG2 )
      fprintf(stderr, "NrcShiftTryDemands returning false (count %d)\n",
	HaArrayCount(s->demand_constraints));
    return false;
  }

  ** all demands must currently have weight 0 for not assigned and assigned **
  HaArrayForEach(s->demands, d, i)
  {
    p1 = NrcDemandNotAssignedPenalty(d);
    p2 = NrcDemandAssignedPenalty(d);
    if( NrcPenaltyWeight(p1) != 0 || NrcPenaltyWeight(p2) != 0 )
    {
      if( DEBUG2 )
      {
	HA_ARENA a = NrcInstanceArena(s->instance);
	fprintf(stderr, "NrcShiftTryDemands returning false (not_assigned %s,"
	  " assigned %s)\n", NrcPenaltyShow(p1, a), NrcPenaltyShow(p2, a));
      }
      return false;
    }
  }

  ** all the preferred_ws and not_preferred_penalty values must be equal **
  d = HaArrayFirst(s->demands);
  first_ws = NrcDemandPreferredWorkerSet(d);
  first_p = NrcDemandNotPreferredPenalty(d);
  for( i = 1;  i < HaArrayCount(s->demands);  i++ )
  {
    d = HaArray(s->demands, i);
    if( !NrcWorkerSetEqual(first_ws, NrcDemandPreferredWorkerSet(d)) )
    {
      if( DEBUG2 )
	fprintf(stderr, "NrcShiftTryDemands returning false (worker set %d)\n",
	  i);
      return false;
    }
    if( !NrcPenaltyEqual(first_p, NrcDemandNotPreferredPenalty(d)) )
    {
      if( DEBUG2 )
	fprintf(stderr,
	  "NrcShiftTryDemands returning false (not_preferred %d)\n", i);
      return false;
    }
  }

  ** OK, do it **
  ds = NrcDemandSetMakeFromBound(s->instance, b, HaArrayCount(s->demands),
    first_ws, first_p);
  HaArrayClear(s->demands);
  NrcShiftAddDemandSet(s, ds);
  if( DEBUG2 )
    fprintf(stderr, "NrcShiftTryDemands returning true\n");
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "workload"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int NrcShiftWorkload(NRC_SHIFT s)                                        */
/*                                                                           */
/*  Return the workload of s.  This is just the workload of its shift type.  */
/*                                                                           */
/*****************************************************************************/

int NrcShiftWorkload(NRC_SHIFT s)
{
  return NrcShiftTypeWorkload(s->shift_type);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "demands and demand-sets"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftAddDemand(NRC_SHIFT s, NRC_DEMAND d)                        */
/*                                                                           */
/*  Add d to s.                                                              */
/*                                                                           */
/*****************************************************************************/

void NrcShiftAddDemand(NRC_SHIFT s, NRC_DEMAND d)
{
  HaArrayAddLast(s->demands, d);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftAddDemandMulti(NRC_SHIFT s, NRC_DEMAND d, int multiplicity) */
/*                                                                           */
/*  Add demand d to s multiplicity times.                                    */
/*                                                                           */
/*****************************************************************************/

void NrcShiftAddDemandMulti(NRC_SHIFT s, NRC_DEMAND d, int multiplicity)
{
  int i;
  for( i = 0;  i < multiplicity;  i++ )
    NrcShiftAddDemand(s, d);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftAddDemandSet(NRC_SHIFT s, NRC_DEMAND_SET ds)                */
/*                                                                           */
/*  Add ds to s.                                                             */
/*                                                                           */
/*****************************************************************************/

void NrcShiftAddDemandSet(NRC_SHIFT s, NRC_DEMAND_SET ds)
{
  int i;
  for( i = 0;  i < NrcDemandSetDemandCount(ds);  i++ )
    NrcShiftAddDemand(s, NrcDemandSetDemand(ds, i));
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcShiftDemandCount(NRC_SHIFT s)                                     */
/*                                                                           */
/*  Return the number of demands added to s.                                 */
/*                                                                           */
/*****************************************************************************/

int NrcShiftDemandCount(NRC_SHIFT s)
{
  return HaArrayCount(s->demands);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DEMAND NrcShiftDemand(NRC_SHIFT s, int i)                            */
/*                                                                           */
/*  Return the i'th demand of s.                                             */
/*                                                                           */
/*****************************************************************************/

NRC_DEMAND NrcShiftDemand(NRC_SHIFT s, int i)
{
  return HaArray(s->demands, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "preassignments"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftAddPreassignment(NRC_SHIFT s, NRC_WORKER w)                 */
/*                                                                           */
/*  Add w to s.                                                              */
/*                                                                           */
/*****************************************************************************/

void NrcShiftAddPreassignment(NRC_SHIFT s, NRC_WORKER w)
{
  HaArrayAddLast(s->preassignments, w);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcShiftPreassignmentCount(NRC_SHIFT s)                              */
/*                                                                           */
/*  Return the number of preassignments added to s.                          */
/*                                                                           */
/*****************************************************************************/

int NrcShiftPreassignmentCount(NRC_SHIFT s)
{
  return HaArrayCount(s->preassignments);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER NrcShiftPreassignment(NRC_SHIFT s, int i)                     */
/*                                                                           */
/*  Return the i'th preassignment of s.                                      */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER NrcShiftPreassignment(NRC_SHIFT s, int i)
{
  return HaArray(s->preassignments, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "conversion"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftConvertToTime(NRC_SHIFT s, KHE_INSTANCE ins)                */
/*                                                                           */
/*  Convert s into a time, and store the time in s.                          */
/*                                                                           */
/*****************************************************************************/

void NrcShiftConvertToTime(NRC_SHIFT s, KHE_INSTANCE ins)
{
  char *id;  HA_ARENA a;
  HnAssert(s->time == NULL, "NrcShiftConvertToTime internal error 1");
  a = NrcInstanceArena(s->instance);
  id = HnStringMake(a, "%s%d", NrcDayShortName(s->day),
    NrcShiftTypeIndex(s->shift_type) + 1);
  if( !KheTimeMake(ins, id, id, false, &s->time) )
    HnAbort("NrcShiftConvertToTime: internal error 2 (%s)", id);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME NrcShiftTime(NRC_SHIFT s)                                       */
/*                                                                           */
/*  Return the KHE time that s has previously been converted to.             */
/*                                                                           */
/*****************************************************************************/

KHE_TIME NrcShiftTime(NRC_SHIFT s)
{
  HnAssert(s->time != NULL, "NrcShiftTime internal error");
  return s->time;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcShiftName(NRC_SHIFT s)                                          */
/*                                                                           */
/*  Return the name that s will have in the KHE instance.                    */
/*                                                                           */
/*****************************************************************************/

char *NrcShiftName(NRC_SHIFT s)
{
  HA_ARENA a;  char *label;
  a = NrcInstanceArena(s->instance);
  label = NrcShiftTypeLabel(s->shift_type);
  return HnStringMake(a, "%s:%s", NrcDayShortName(s->day),
    label != NULL ? label : NrcShiftTypeName(s->shift_type));
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftConvertToEvent(NRC_SHIFT s, KHE_INSTANCE ins)               */
/*                                                                           */
/*  Convert s into an event, and store the event in s.                       */
/*                                                                           */
/*****************************************************************************/

void NrcShiftConvertToEvent(NRC_SHIFT s, KHE_INSTANCE ins)
{
  char *id;  NRC_DEMAND d;  int i, pi, workload;  NRC_WORKER w;
  HnAssert(s->event == NULL, "NrcShiftConvertToEvent internal error 1");
  /* ***
  label = NrcShiftTypeLabel(s->shift_type);
  id = HnStringMake(a, "%s:%s", NrcDayShortName(s->day),
    label != NULL ? label : NrcShiftTypeName(s->shift_type));
  *** */
  id = NrcShiftName(s);
  workload =
    (NrcInstanceHasWorkloadConstraint(s->instance) ? NrcShiftWorkload(s) : 1);
  if( !KheEventMake(ins, id, id, NULL, 1, workload, s->time, &s->event) )
    HnAbort("NrcShiftConvertToEvent internal error (%s)", id);
  pi = 0;
  HaArrayForEach(s->demands, d, i)
  {
    /* find a resource that could be preassigned to d, or NULL if none */
    /* this is scrappy code, but I'm not going to build a bipartite match! */
    /* ***
    w = NULL;
    if( pi < HaArrayCount(s->preassignments) )
    {
      w2 = HaArray(s->preassignments, pi);
      ws = NrcDemandPreferredWorkerSet(d);
      if( ws == NULL || NrcWorkerSetContainsWorker(ws, w2) )
	w = w2, pi++;
    }
    *** */

    /* and this code is even more scrappy */
    if( pi < HaArrayCount(s->preassignments) )
      w = HaArray(s->preassignments, pi++);
    else
      w = NULL;

    /* convert d */
    NrcDemandConvertToEventResource(d, s, w, s->event);
  }
  HnAssert(pi >= HaArrayCount(s->preassignments), "NrcShiftConvertToEvent: "
    "converted only %d of the %d preassignments of shift %s:%s",
    pi, HaArrayCount(s->preassignments), NrcDayShortName(s->day),
    NrcShiftTypeName(s->shift_type));
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT NrcShiftEvent(NRC_SHIFT s)                                     */
/*                                                                           */
/*  Return the KHE event that s has previously been converted to.            */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT NrcShiftEvent(NRC_SHIFT s)
{
  HnAssert(s->event != NULL, "NrcShiftEvent internal error");
  return s->event;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftDebug(NRC_SHIFT s, int indent, FILE *fp)                    */
/*                                                                           */
/*  Debug print of s onto fp with the given indent.                          */
/*                                                                           */
/*****************************************************************************/

void NrcShiftDebug(NRC_SHIFT s, int indent, FILE *fp)
{
  int i;  NRC_DEMAND d;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ Shift(", indent, "");
    NrcDayDebug(s->day, -1, fp);
    fprintf(fp, ", %s):\n", NrcShiftTypeName(s->shift_type));
    HaArrayForEach(s->demands, d, i)
      NrcDemandDebug(d, 0, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "%s:%s", NrcDayShortName(s->day),
      NrcShiftTypeName(s->shift_type));
    /* ***
    fprintf(fp, "%s:%s/%d", NrcDayShortName(s->day),
      NrcShiftTypeName(s->shift_type), s->index);
    *** */
}

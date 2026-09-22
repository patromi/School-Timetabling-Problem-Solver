
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
/*  FILE:         coi_cover.c                                                */
/*  MODULE:       Cover for the Curtois original instances                   */
/*                                                                           */
/*****************************************************************************/
#include <setjmp.h>
#include "externs.h"
#define str_eq(a, b) (strcmp((a), (b)) == 0)
#define str_eq2(a, b1, b2) (str_eq((a), (b1)) || str_eq((a), (b2)))

#define DEBUG1 0



/*****************************************************************************/
/*                                                                           */
/*  Submodule "type declarations"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  COI_COVER - one <Cover> element, interpreted                             */
/*                                                                           */
/*****************************************************************************/

struct coi_cover_rec {
  KML_ELT		cover_elt;
  NRC_SHIFT_TYPE	shift_type;
  NRC_TIME_INTERVAL	time_interval;
  NRC_WORKER_SET	skill_ws;
  COI_BOUND		min_bound;
  COI_BOUND		pref_bound;
  COI_BOUND		max_bound;
};

typedef HA_ARRAY(COI_COVER) ARRAY_COI_COVER;


/*****************************************************************************/
/*                                                                           */
/*  COI_DAY_COVER - a <DayOfWeekCover> or <DateSpecificCover>, interpreted   */
/*                                                                           */
/*****************************************************************************/

struct coi_day_cover_rec
{
  KML_ELT		day_cover_elt;
  NRC_DAY_SET		day_set;
  ARRAY_COI_COVER	covers;
};

typedef HA_ARRAY(COI_DAY_COVER) ARRAY_COI_DAY_COVER;


/*****************************************************************************/
/*                                                                           */
/*  COI_COVER_REQTS - a <CoverRequirements>, interpreted                     */
/*                                                                           */
/*****************************************************************************/

struct coi_cover_reqts_rec
{
  KML_ELT		cover_reqts_elt;
  int			dft_min_below_weight;
  int			dft_max_above_weight;
  int			dft_pref_above_weight;
  int			dft_pref_below_weight;
  ARRAY_COI_DAY_COVER	day_covers;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "cover"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  COI_COVER CoiCoverMake(KML_ELT cover_elt, NRC_SHIFT_TYPE st,             */
/*    NRC_TIME_INTERVAL ti, NRC_WORKER_SET skill_ws, COI_BOUND min_b,        */
/*    COI_BOUND pref_b, COI_BOUND max_b)                                     */
/*                                                                           */
/*  Make a new COI cover object with these attributes.                       */
/*                                                                           */
/*****************************************************************************/

COI_COVER CoiCoverMake(KML_ELT cover_elt, NRC_SHIFT_TYPE st,
  NRC_TIME_INTERVAL ti, NRC_WORKER_SET skill_ws, COI_BOUND min_b,
  COI_BOUND pref_b, COI_BOUND max_b, HA_ARENA a)
{
  COI_COVER res;
  HnAssert(st == NULL || ti == NULL, "CoiCoverMake: st and ti both non-NULL");
  HnAssert(st != NULL || ti != NULL, "CoiCoverMake: st and ti both NULL");
  HaMake(res, a);
  res->cover_elt = cover_elt;
  res->shift_type = st;
  res->time_interval = ti;
  res->skill_ws = skill_ws;
  res->min_bound = min_b;
  res->pref_bound = pref_b;
  res->max_bound = max_b;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET CoiCoverShiftSetOnDay(COI_COVER cv, NRC_DAY d)             */
/*                                                                           */
/*  Return the set of shifts constrained by cv on d.                         */
/*                                                                           */
/*****************************************************************************/

static NRC_SHIFT_SET CoiCoverShiftSetOnDay(COI_COVER cv, NRC_DAY d)
{
  if( cv->shift_type != NULL )
    return NrcShiftSingletonShiftSet(
      NrcDayShiftFromShiftType(d, cv->shift_type));
  else
    return NrcTimeIntervalShiftSet(cv->time_interval, d);
}


/*****************************************************************************/
/*                                                                           */
/*  char *CoverDescr(COI_COVER c)                                            */
/*                                                                           */
/*  Return a description of what c covers:  either a shift type or a         */
/*  time interval.                                                           */
/*                                                                           */
/*****************************************************************************/

static char *CoverDescr(COI_COVER c, HA_ARENA a)
{
  if( c->shift_type != NULL )
    return NrcShiftTypeName(c->shift_type);
  else
    return NrcTimeIntervalShow(c->time_interval, a);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "day cover"                                                    */
/*                                                                           */
/*****************************************************************************/

/****************************************************************************/
/*                                                                           */
/*  COI_DAY_COVER CoiDayCoverMake(KML_ELT day_cover_elt,                     */
/*    NRC_DAY_SET day_set, HA_ARENA a)                                       */
/*                                                                           */
/*  Make a new day cover object, initially unconstrained.                    */
/*                                                                           */
/*****************************************************************************/

COI_DAY_COVER CoiDayCoverMake(KML_ELT day_cover_elt,
  NRC_DAY_SET day_set, HA_ARENA a)
{
  COI_DAY_COVER res;
  HaMake(res, a);
  res->day_cover_elt = day_cover_elt;
  res->day_set = day_set;
  HaArrayInit(res->covers, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void CoiDayCoverAddCover(COI_DAY_COVER dc, COI_COVER cover)              */
/*                                                                           */
/*  Add cover to dc.                                                         */
/*                                                                           */
/*****************************************************************************/

void CoiDayCoverAddCover(COI_DAY_COVER dc, COI_COVER cover)
{
  HaArrayAddLast(dc->covers, cover);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "cover requirements"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  COI_COVER_REQTS CoiCoverReqtsMake(KML_ELT cover_reqts_elt,               */
/*    int dft_min_below_weight, int dft_max_above_weight,                    */
/*    int dft_pref_above_weight, int dft_pref_below_weight)                  */
/*                                                                           */
/*  Make a new cover requirements object, initially unconstrained.           */
/*                                                                           */
/*****************************************************************************/

COI_COVER_REQTS CoiCoverReqtsMake(KML_ELT cover_reqts_elt,
  int dft_min_below_weight, int dft_max_above_weight,
  int dft_pref_above_weight, int dft_pref_below_weight, HA_ARENA a)
{
  COI_COVER_REQTS res;
  HaMake(res, a);
  res->cover_reqts_elt = cover_reqts_elt;
  res->dft_min_below_weight = dft_min_below_weight;
  res->dft_max_above_weight = dft_max_above_weight;
  res->dft_pref_above_weight = dft_pref_above_weight;
  res->dft_pref_below_weight = dft_pref_below_weight;
  HaArrayInit(res->day_covers, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KML_ELT CoiCoverReqtsElt(COI_COVER_REQTS cr)                             */
/*                                                                           */
/*  Return the cover_reqts_elt attribute of cr.                              */
/*                                                                           */
/*****************************************************************************/

KML_ELT CoiCoverReqtsElt(COI_COVER_REQTS cr)
{
  return cr->cover_reqts_elt;
}


/*****************************************************************************/
/*                                                                           */
/*  int CoiCoverReqtsDftMinBelowWeight(COI_COVER_REQTS cr)                   */
/*                                                                           */
/*  Return the dft_min_below_weight attribute of cr.                         */
/*                                                                           */
/*****************************************************************************/

int CoiCoverReqtsDftMinBelowWeight(COI_COVER_REQTS cr)
{
  return cr->dft_min_below_weight;
}


/*****************************************************************************/
/*                                                                           */
/*  int CoiCoverReqtsDftMaxAboveWeight(COI_COVER_REQTS cr)                   */
/*                                                                           */
/*  Return the dft_max_above_weight attribute of cr.                         */
/*                                                                           */
/*****************************************************************************/

int CoiCoverReqtsDftMaxAboveWeight(COI_COVER_REQTS cr)
{
  return cr->dft_max_above_weight;
}


/*****************************************************************************/
/*                                                                           */
/*  int CoiCoverReqtsDftPrefAboveWeight(COI_COVER_REQTS cr)                  */
/*                                                                           */
/*  Return the dft_pref_above_weight attribute of cr.                        */
/*                                                                           */
/*****************************************************************************/

int CoiCoverReqtsDftPrefAboveWeight(COI_COVER_REQTS cr)
{
  return cr->dft_pref_above_weight;
}


/*****************************************************************************/
/*                                                                           */
/*  int CoiCoverReqtsDftPrefBelowWeight(COI_COVER_REQTS cr)                  */
/*                                                                           */
/*  Return the dft_pref_below_weight attribute of cr.                        */
/*                                                                           */
/*****************************************************************************/

int CoiCoverReqtsDftPrefBelowWeight(COI_COVER_REQTS cr)
{
  return cr->dft_pref_below_weight;
}


/*****************************************************************************/
/*                                                                           */
/*  void CoiCoverReqtsAddDayCover(COI_COVER_REQTS cr, COI_DAY_COVER dc)      */
/*                                                                           */
/*  Add dc to cr.  Don't make any consistency checks yet.                    */
/*                                                                           */
/*****************************************************************************/

void CoiCoverReqtsAddDayCover(COI_COVER_REQTS cr, COI_DAY_COVER dc)
{
  HaArrayAddLast(cr->day_covers, dc);
}


/*****************************************************************************/
/*                                                                           */
/*  int CoiDayCoverCmp(const void *t1, const void *t2)                       */
/*                                                                           */
/*  Comparison function for sorting an array of day covers by decreasing     */
/*  number of days covered.                                                  */
/*                                                                           */
/*****************************************************************************/

static int CoiDayCoverCmp(const void *t1, const void *t2)
{
  COI_DAY_COVER dc1 = * (COI_DAY_COVER *) t1;
  COI_DAY_COVER dc2 = * (COI_DAY_COVER *) t2;
  return NrcDaySetDayCount(dc2->day_set) - NrcDaySetDayCount(dc1->day_set);
}


/*****************************************************************************/
/*                                                                           */
/*  bool CoiDayCoverContainsCover(COI_DAY_COVER dc, COI_COVER c)             */
/*                                                                           */
/*  Return true if dc contains a cover like c.                               */
/*                                                                           */
/*****************************************************************************/

static bool CoiDayCoverContainsCover(COI_DAY_COVER dc, COI_COVER c,
  COI_COVER *res)
{
  COI_COVER c2;  int i;
  if( c->shift_type != NULL )
  {
    /* comparing shift types */
    HaArrayForEach(dc->covers, c2, i)
      if( c->skill_ws == c2->skill_ws && c->shift_type == c2->shift_type )
	return *res = c2, true;
  }
  else
  {
    /* comparing time intervals */
    HaArrayForEach(dc->covers, c2, i)
      if( c->skill_ws == c2->skill_ws && c2->time_interval != NULL &&
	  NrcTimeIntervalEqual(c->time_interval, c2->time_interval))
	return *res = c2, true;
  }
  return *res = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void CoiDayCoverMeld(COI_DAY_COVER dest_dc, COI_DAY_COVER source_dc)     */
/*                                                                           */
/*  Add anything that dest_dc does not already have from source_dc.          */
/*                                                                           */
/*****************************************************************************/

static void CoiDayCoverMeld(COI_DAY_COVER dest_dc, COI_DAY_COVER source_dc)
{
  COI_COVER source_c, dest_c;  int i;
  HaArrayForEach(source_dc->covers, source_c, i)
    if( CoiDayCoverContainsCover(dest_dc, source_c, &dest_c) )
    {
      /* dest_dc contains something like source_c, but does it have its bits? */
      if( dest_c->min_bound == NULL )
	dest_c->min_bound = source_c->min_bound;
      if( dest_c->pref_bound == NULL )
	dest_c->pref_bound = source_c->pref_bound;
      if( dest_c->max_bound == NULL )
	dest_c->max_bound = source_c->max_bound;
    }
    else
    {
      /* dest_dc contains nothing like source_c, so add all of it */
      HaArrayAddLast(dest_dc->covers, source_c);
    }
}


/*****************************************************************************/
/*                                                                           */
/*  void CoiCoverReqtsMakeDaysCoversDisjoint(COI_COVER_REQTS cr)             */
/*                                                                           */
/*  Ensure that the day covers of cr are disjoint.                           */
/*                                                                           */
/*****************************************************************************/

void CoiCoverReqtsMakeDayCoversDisjoint(COI_COVER_REQTS cr)
{
  int i, j;  COI_DAY_COVER dci, dcj;
  HaArraySort(cr->day_covers, &CoiDayCoverCmp);
  HaArrayForEach(cr->day_covers, dci, i)
  {
    /* invariant:  there are no overlaps in cr->day_covers[0 .. i-1] */
    for( j = 0;  j < i;  j++ )
    {
      dcj = HaArray(cr->day_covers, j);
      if( NrcDaySetsOverlap(dci->day_set, dcj->day_set) )
      {
	/* dci replaces dcj for the dci day-set */
	dcj->day_set = NrcDaySetDifference(dcj->day_set, dci->day_set);

	/* but dci inherits anything it doesn't already have from dcj */
	CoiDayCoverMeld(dci, dcj);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void CoiCoverReqtsDemandCountForShiftOnDay(COI_COVER_REQTS cr,           */
/*    NRC_SHIFT s, NRC_DAY d, int total_workers, int *res)                   */
/*                                                                           */
/*  Carry out the work required by CoiCoverReqtsDemandCountForShift below    */
/*  for day d.                                                               */
/*                                                                           */
/*****************************************************************************/

static void CoiCoverReqtsDemandCountForShiftOnDay(COI_COVER_REQTS cr,
  NRC_SHIFT s, NRC_DAY d, int total_workers, int *res)
{
  int i, j, lim, v;  COI_DAY_COVER dc;  COI_COVER cv;  NRC_SHIFT_SET ss;
  NRC_BOUND b;  NRC_PENALTY junk1, junk2;
  HaArrayForEach(cr->day_covers, dc, i)
  {
    if( NrcDaySetContainsDay(dc->day_set, d) )
    {
      /* dc contains d, so it could constrain s */
      HaArrayForEach(dc->covers, cv, j)
      {
	if( cv->max_bound != NULL || cv->pref_bound != NULL )
	{
	  ss = CoiCoverShiftSetOnDay(cv, d);
	  if( NrcShiftSetContainsShift(ss, s) )
	  {
	    /* cv constrains s; find the limit it imposes */
	    if( cv->max_bound != NULL )
	    {
	      b = CoiBoundBound(cv->max_bound);
	      if( !NrcBoundMax(b, &lim, &junk1) )
	        HnAbort("CoiCoverReqtsDemandCountForShiftOnDay internal err 1");
	      lim += 1;
	    }
	    else
	    {
	      b = CoiBoundBound(cv->pref_bound);
	      if( !NrcBoundPreferred(b, &lim, &junk1, &junk2) )
	        HnAbort("CoiCoverReqtsDemandCountForShiftOnDay internal err 2");
	      lim = 2 * lim + 1;
	    }

	    /* find the bound, either the limit or as described above */
	    if( cv->skill_ws == NULL )
	      v = lim;
	    else
	      v = lim + (total_workers - NrcWorkerSetWorkerCount(cv->skill_ws));

	    /* *res is the smaller of its current value and the new bound */
	    *res = min(*res, v);
	  }
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int CoiCoverReqtsDemandCountForShift(COI_COVER_REQTS cr, NRC_SHIFT s)    */
/*                                                                           */
/*  Return a good estimate of the number of workers needed by shift s.       */
/*                                                                           */
/*  At worst, we need one demand for every worker in the instance.  But if   */
/*  some constraint within cr imposes a maximum limit of Max(S) for workers  */
/*  with a particular skill S in s, and if N is the number of workers in     */
/*  the instance and N(S) is the number with skill S, then we need at most   */
/*  Max(S) + (N - N(S)) demands, so we take the minimum of all these values. */
/*                                                                           */
/*  If there is a preferred value Pref(S) but no Max(S), then we take        */
/*  Max(S) to be 2 * Pref(S).  We ignore minimum limits.                     */
/*                                                                           */
/*  A complicating factor is that shift s may be constrained by a cover      */
/*  on the day before it begins (if the cover is for a time interval that    */
/*  spans midnight) or the day after it begins (if s itself spans midnight). */
/*  So this code traverses all the day covers three times, first looking     */
/*  for cases where a cover for s's day constrains s, then looking for       */
/*  cases where a cover for the day before s's day constrains s, then for    */
/*  cases where a cover for the day after s's day constrains s.              */
/*                                                                           */
/*  When this was run, Ikegami-3Shift-DATA1.Solution.2.roster was the        */
/*  only unreadable solution, and its maximum overload was only 1, so        */
/*  the code actually uses Max(S) + 1 rather than Max(S).                    */
/*                                                                           */
/*****************************************************************************/

int CoiCoverReqtsDemandCountForShift(COI_COVER_REQTS cr, NRC_SHIFT s)
{
  int res, total_workers;  NRC_DAY d, prev_d, next_d;
  total_workers = NrcInstanceStaffingWorkerCount(NrcShiftInstance(s));
  d = NrcShiftDay(s);
  prev_d = NrcDayPrev(d);
  next_d = NrcDayNext(d);
  res = total_workers;
  CoiCoverReqtsDemandCountForShiftOnDay(cr, s, d, total_workers, &res);
  if( prev_d != NULL )
    CoiCoverReqtsDemandCountForShiftOnDay(cr, s, prev_d, total_workers, &res);
  if( next_d != NULL )
    CoiCoverReqtsDemandCountForShiftOnDay(cr, s, next_d, total_workers, &res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool CoiCoverReqtsGenerateDemandConstraints(COI_COVER_REQTS cr,          */
/*    NRC_WORKER_SET dft_ws, char **err_mess, KML_ELT *problem_elt)          */
/*                                                                           */
/*  Add demand constraints corresponding to the covers of cr.  If some       */
/*  cover has no worker-set, use dft_ws (which will contain all workers).    */
/*  Return true if successful, else return false with *err_mess and          */
/*  *problem_elt set.                                                        */
/*                                                                           */
/*****************************************************************************/

bool CoiCoverReqtsGenerateDemandConstraints(COI_COVER_REQTS cr,
  NRC_INSTANCE ins, char *soln_file_name, NRC_WORKER_SET dft_ws,
  char **err_mess, KML_ELT *problem_elt, HA_ARENA a)
{
  COI_DAY_COVER dc;  COI_COVER cv;  int i, j, k, v;  NRC_SHIFT_SET ss;
  NRC_WORKER_SET ws;  /* NRC_DEMAND_CONSTRAINT c; */  NRC_DAY d;  char *name;
  NRC_BOUND b, b2;  bool allow_zero;  NRC_PENALTY p1, p2;
  if( DEBUG1 )
    fprintf(stderr, "[ CoiCoverReqtsGenerateDemandConstraints(cr, %s, ...)\n",
      NrcInstanceId(ins));
  HaArrayForEach(cr->day_covers, dc, i)
  {
    if( DEBUG1 )
      fprintf(stderr, "  [ day_cover for %s\n",NrcDaySetShortName(dc->day_set));
    HaArrayForEach(dc->covers, cv, j)
    {
      /* find bound for cv */
      if( DEBUG1 )
	fprintf(stderr, "    [ cover for %s\n", CoverDescr(cv, a));
      b = NrcBoundMake(ins);
      if( cv->pref_bound != NULL )
      {
	b2 = CoiBoundBound(cv->pref_bound);
	if( !NrcBoundPreferred(b2, &v, &p1, &p2) )
	  HnAbort("CoiCoverReqtsGenerateDemandConstraints internal error 1");
	if( !NrcBoundAddPreferred(b, v, p1, p2) )
	  HnAbort("CoiCoverReqtsGenerateDemandConstraints internal error 2");
      }
      if( cv->min_bound != NULL )
      {
	b2 = CoiBoundBound(cv->min_bound);
	if( !NrcBoundMin(b2, &v, &allow_zero, &p1) )
	  HnAbort("CoiCoverReqtsGenerateDemandConstraints internal error 3");
	if( v > 0 && !NrcBoundAddMin(b, v, allow_zero, p1) )
	  KmlEltWarning(cv->cover_elt, soln_file_name,
	    "ignoring <Min> (inconsistent with <Preferred>)");
	  /* ***
	  return *err_mess = "<Min> inconsistent with <Preferred> or <Max>",
	    *problem_elt = cv->cover_elt, false;
	  *** */
      }
      if( cv->max_bound != NULL )
      {
	b2 = CoiBoundBound(cv->max_bound);
	if( !NrcBoundMax(b2, &v, &p1) )
	  HnAbort("CoiCoverReqtsGenerateDemandConstraints internal error 4");
	if( !NrcBoundAddMax(b, v, p1) )
	  KmlEltWarning(cv->cover_elt, soln_file_name,
	    "ignoring <Max> (inconsistent with <Preferred> or <Min>)");
	  /* ***
	  return *err_mess = "<Max> inconsistent with <Min> or <Preferred>",
	    *problem_elt = cv->cover_elt, false;
	  *** */
      }
      /* ***
      if( cv->min_lim != NULL )
	if( !CoiLimitAddToDemandConstraint(cv->min_lim, c) )
	  return *err_mess = "<Min> inconsistent with <Preferred> or <Max>",
	    *problem_elt = cv->cover_elt, false;
      if( cv->pref_lim != NULL )
	if( !CoiLimitAddToDemandConstraint(cv->pref_lim, c) )
	  return *err_mess = "<Preferred> inconsistent with <Min> or <Max>",
	    *problem_elt = cv->cover_elt, false;
      if( cv->max_lim != NULL )
	if( !CoiLimitAddToDemandConstraint(cv->max_lim, c) )
	  return *err_mess = "<Max> inconsistent with <Min> or <Preferred>",
	    *problem_elt = cv->cover_elt, false;
      */

      /* do it for each day */
      for( k = 0;  k < NrcDaySetDayCount(dc->day_set);  k++ )
      {
	d = NrcDaySetDay(dc->day_set, k);
	if( DEBUG1 )
	  fprintf(stderr, "      day %s\n", NrcDayShortName(d));
        ss = CoiCoverShiftSetOnDay(cv, d);
	if( NrcShiftSetShiftCount(ss) > 0 )
	{
	  ws = (cv->skill_ws != NULL ? cv->skill_ws : dft_ws);
	  name = HnStringMake(a, "%s:%s", CoverDescr(cv, a),
	    cv->skill_ws != NULL ? NrcWorkerSetName(cv->skill_ws) : "all");
	  if( DEBUG1 )
	  {
	    fprintf(stderr, "      NrcDemandConstraintMake(%s, ",NrcBoundShow(b));
	    NrcShiftSetDebug(ss, -1, stderr);
	    fprintf(stderr, ", ws, \"%s\")\n", name);
	  }
	  NrcDemandConstraintMake(b, ss, ws, name);
	}
      }
      if( DEBUG1 )
	fprintf(stderr, "    ] cover for %s\n", CoverDescr(cv, a));
    }
    if( DEBUG1 )
      fprintf(stderr, "  ]\n");
  }
  if( DEBUG1 )
    fprintf(stderr, "] CoiCoverReqtsGenerateDemandConstraints ret. true\n");
  return *err_mess = NULL, *problem_elt = NULL, true;
}

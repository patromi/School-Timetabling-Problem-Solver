
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
/*  FILE:         nrc_demand_set.c                                           */
/*  MODULE:       A demand-set                                               */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

/*****************************************************************************/
/*                                                                           */
/*  NRC_DEMAND_SET                                                           */
/*                                                                           */
/*  A demand-set                                                             */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_DEMAND) ARRAY_NRC_DEMAND;

struct nrc_demand_set_rec {
  NRC_INSTANCE		instance;
  ARRAY_NRC_DEMAND	demands;
};


/*****************************************************************************/
/*                                                                           */
/*  NRC_DEMAND_SET NrcDemandSetMake(NRC_INSTANCE ins)                        */
/*                                                                           */
/*  Return a new, empty demand-set.                                          */
/*                                                                           */
/*****************************************************************************/

NRC_DEMAND_SET NrcDemandSetMake(NRC_INSTANCE ins)
{
  NRC_DEMAND_SET res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->instance = ins;
  HaArrayInit(res->demands, a);
  NrcInstanceAddDemandSet(ins, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE NrcDemandSetInstance(NRC_DEMAND_SET ds)                     */
/*                                                                           */
/*  Return the enclosing instance.                                           */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE NrcDemandSetInstance(NRC_DEMAND_SET ds)
{
  return ds->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandSetAddDemand(NRC_DEMAND_SET cs, NRC_DEMAND c)              */
/*                                                                           */
/*  Add d to ds.                                                             */
/*                                                                           */
/*****************************************************************************/

void NrcDemandSetAddDemand(NRC_DEMAND_SET ds, NRC_DEMAND d)
{
  HaArrayAddLast(ds->demands, d);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandSetAddDemandMulti(NRC_DEMAND_SET ds, NRC_DEMAND d,         */
/*    int multiplicity)                                                      */
/*                                                                           */
/*  Add d to ds multiplicity times.                                          */
/*                                                                           */
/*****************************************************************************/

void NrcDemandSetAddDemandMulti(NRC_DEMAND_SET ds, NRC_DEMAND d,
  int multiplicity)
{
  int i;
  for( i = 0;  i < multiplicity;  i++ )
    NrcDemandSetAddDemand(ds, d);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandSetAddWanted(NRC_DEMAND_SET ds, NRC_PENALTY absent_p,      */
/*    NRC_WORKER_SET preferred_ws, NRC_PENALTY not_preferred_p, int num)     */
/*                                                                           */
/*  Add num wanted demands with these attributes to ds.                      */
/*                                                                           */
/*****************************************************************************/

static void NrcDemandSetAddWanted(NRC_DEMAND_SET ds, NRC_PENALTY absent_p,
  NRC_WORKER_SET preferred_ws, NRC_PENALTY not_preferred_p, int num)
{
  NRC_INSTANCE ins;  NRC_DEMAND d;
  if( num > 0 )
  {
    ins = NrcDemandSetInstance(ds);
    d = NrcDemandMakeBegin(ins);
    NrcDemandPenalizeNonAssignment(d, NRC_PENALTY_UNIQUE, absent_p);
    if( preferred_ws != NULL )
      NrcDemandPenalizeNotWorkerSet(d, preferred_ws, NRC_PENALTY_UNIQUE,
	not_preferred_p);
    NrcDemandMakeEnd(d);
    NrcDemandSetAddDemandMulti(ds, d, num);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandSetAddUnWanted(NRC_DEMAND_SET ds, NRC_PENALTY present_p,   */
/*    NRC_WORKER_SET preferred_ws, NRC_PENALTY not_preferred_p, int num)     */
/*                                                                           */
/*  Add num unwanted demands with these attributes to ds.                    */
/*                                                                           */
/*****************************************************************************/

static void NrcDemandSetAddUnWanted(NRC_DEMAND_SET ds, NRC_PENALTY present_p,
  NRC_WORKER_SET preferred_ws, NRC_PENALTY not_preferred_p, int num)
{
  NRC_INSTANCE ins;  NRC_DEMAND d;
  if( num > 0 )
  {
    ins = NrcDemandSetInstance(ds);
    d = NrcDemandMakeBegin(ins);
    NrcDemandPenalizeWorkerSet(d, NrcInstanceStaffing(ins),
      NRC_PENALTY_UNIQUE, present_p);
    if( preferred_ws != NULL )
      NrcDemandPenalizeNotWorkerSet(d, preferred_ws, NRC_PENALTY_ADD,
	not_preferred_p);
    NrcDemandMakeEnd(d);
    NrcDemandSetAddDemandMulti(ds, d, num);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandSetAddNeutral(NRC_DEMAND_SET ds,                           */
/*    NRC_WORKER_SET preferred_ws, NRC_PENALTY not_preferred_p, int num)     */
/*                                                                           */
/*  Add num unwanted demands with these attributes to ds.                    */
/*                                                                           */
/*****************************************************************************/

static void NrcDemandSetAddNeutral(NRC_DEMAND_SET ds,
  NRC_WORKER_SET preferred_ws, NRC_PENALTY not_preferred_p, int num)
{
  NRC_INSTANCE ins;  NRC_DEMAND d;
  if( num > 0 )
  {
    ins = NrcDemandSetInstance(ds);
    d = NrcDemandMakeBegin(ins);
    if( preferred_ws != NULL )
      NrcDemandPenalizeNotWorkerSet(d, preferred_ws,
	NRC_PENALTY_UNIQUE, not_preferred_p);
    NrcDemandMakeEnd(d);
    NrcDemandSetAddDemandMulti(ds, d, num);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DEMAND_SET NrcDemandSetMakeFromBound(NRC_INSTANCE ins,               */
/*    NRC_BOUND b, int count, NRC_WORKER_SET preferred_ws,                   */
/*    NRC_PENALTY not_preferred_penalty)                                     */
/*                                                                           */
/*  Make a demand-set based on bound b.                                      */
/*                                                                           */
/*****************************************************************************/

NRC_DEMAND_SET NrcDemandSetMakeFromBound(NRC_INSTANCE ins,
  NRC_BOUND b, int count, NRC_WORKER_SET preferred_ws,
  NRC_PENALTY not_preferred_penalty)
{
  int min_value, pref_value, max_value;  bool allow_zero;
  NRC_PENALTY below_min_p, below_pref_p, above_pref_p, above_max_p;
  NRC_DEMAND_SET res;
  res = NrcDemandSetMake(ins);
  if( NrcBoundMin(b, &min_value, &allow_zero, &below_min_p) )
  {
    HnAssert(!allow_zero, "NrcDemandSetMakeFromBound: allow_zero in bound");
    HnAssert(NrcPenaltyCostFn(below_min_p) == NRC_COST_FUNCTION_LINEAR,
      "NrcDemandSetMakeFromBound: cost_fn in min penalty not linear");
    if( NrcBoundPreferred(b, &pref_value, &below_pref_p, &above_pref_p) )
    {
      HnAssert(NrcPenaltyCostFn(below_pref_p) == NRC_COST_FUNCTION_LINEAR,
        "NrcDemandSetMakeFromBound: cost_fn in below_pref penalty not linear");
      HnAssert(NrcPenaltyCostFn(above_pref_p) == NRC_COST_FUNCTION_LINEAR,
        "NrcDemandSetMakeFromBound: cost_fn in above_pref penalty not linear");
      if( NrcBoundMax(b, &max_value, &above_max_p) )
      {
	HnAssert(NrcPenaltyCostFn(above_max_p) == NRC_COST_FUNCTION_LINEAR,
	  "NrcDemandSetMakeFromBound: cost_fn in max penalty not linear");

	/* min; pref; max */
	NrcDemandSetAddWanted(res, below_min_p, preferred_ws,
	  not_preferred_penalty, min_value);
	NrcDemandSetAddWanted(res, below_pref_p, preferred_ws,
	  not_preferred_penalty, pref_value - min_value);
	NrcDemandSetAddUnWanted(res, above_pref_p, preferred_ws,
	  not_preferred_penalty, max_value - pref_value);
	NrcDemandSetAddUnWanted(res, above_max_p, preferred_ws,
	  not_preferred_penalty, count - max_value);
      }
      else
      {
	/* min; pref; no max */
	NrcDemandSetAddWanted(res, below_min_p, preferred_ws,
	  not_preferred_penalty, min_value);
        NrcDemandSetAddWanted(res, below_pref_p, preferred_ws,
	  not_preferred_penalty, pref_value - min_value);
	NrcDemandSetAddUnWanted(res, above_pref_p, preferred_ws,
	  not_preferred_penalty, count - pref_value);
      }
    }
    else
    {
      if( NrcBoundMax(b, &max_value, &above_max_p) )
      {
	HnAssert(NrcPenaltyCostFn(above_max_p) == NRC_COST_FUNCTION_LINEAR,
	  "NrcDemandSetMakeFromBound: cost_fn in max penalty not linear");

	/* min; no pref; max */
	NrcDemandSetAddWanted(res, below_min_p, preferred_ws,
	  not_preferred_penalty, min_value);
	NrcDemandSetAddNeutral(res, preferred_ws,
	  not_preferred_penalty, max_value - min_value);
	NrcDemandSetAddUnWanted(res, above_max_p, preferred_ws,
	  not_preferred_penalty, count - max_value);
      }
      else
      {
	/* min; no pref; no max */
	NrcDemandSetAddWanted(res, below_min_p, preferred_ws,
	  not_preferred_penalty, min_value);
	NrcDemandSetAddNeutral(res, preferred_ws,
	  not_preferred_penalty, count - min_value);
      }
    }
  }
  else
  {
    if( NrcBoundPreferred(b, &pref_value, &below_pref_p, &above_pref_p) )
    {
      HnAssert(NrcPenaltyCostFn(below_pref_p) == NRC_COST_FUNCTION_LINEAR,
        "NrcDemandSetMakeFromBound: cost_fn in below_pref penalty not linear");
      HnAssert(NrcPenaltyCostFn(above_pref_p) == NRC_COST_FUNCTION_LINEAR,
        "NrcDemandSetMakeFromBound: cost_fn in above_pref penalty not linear");
      if( NrcBoundMax(b, &max_value, &above_max_p) )
      {
	HnAssert(NrcPenaltyCostFn(above_max_p) == NRC_COST_FUNCTION_LINEAR,
	  "NrcDemandSetMakeFromBound: cost_fn in max penalty not linear");

	/* no min; pref; max */
	NrcDemandSetAddWanted(res, below_pref_p, preferred_ws,
	  not_preferred_penalty, pref_value);
	NrcDemandSetAddUnWanted(res, above_pref_p, preferred_ws,
	  not_preferred_penalty, max_value - pref_value);
	NrcDemandSetAddUnWanted(res, above_max_p, preferred_ws,
	  not_preferred_penalty, count - max_value);
      }
      else
      {
	/* no min; pref; no max */
	NrcDemandSetAddWanted(res, below_pref_p, preferred_ws,
	  not_preferred_penalty, pref_value);
	NrcDemandSetAddUnWanted(res, above_pref_p, preferred_ws,
	  not_preferred_penalty, count - pref_value);
      }
    }
    else
    {
      if( NrcBoundMax(b, &max_value, &above_max_p) )
      {
	HnAssert(NrcPenaltyCostFn(above_max_p) == NRC_COST_FUNCTION_LINEAR,
	  "NrcDemandSetMakeFromBound: cost_fn in max penalty not linear");

	/* no min; no pref; max */
	NrcDemandSetAddNeutral(res, preferred_ws,
	  not_preferred_penalty, max_value);
	NrcDemandSetAddUnWanted(res, above_max_p, preferred_ws,
	  not_preferred_penalty, count - max_value);
      }
      else
      {
	/* no min; no pref; no max */
	NrcDemandSetAddNeutral(res, preferred_ws,
	  not_preferred_penalty, count);
      }
    }
  }
  return res;
}


/* *** old version that uses old-style demands
NRC_DEMAND_SET NrcDemandSetMakeFromBound(NRC_INSTANCE ins,
  NRC_BOUND b, int count, NRC_WORKER_SET preferred_ws,
  NRC_PENALTY not_preferred_penalty)
{
  int min_value, pref_value, max_value;  bool allow_zero;
  NRC_PENALTY below_min_p, below_pref_p, above_pref_p, above_max_p, zero_p;
  NRC_DEMAND_SET res;  NRC_DEMAND d;
  res = NrcDemandSetMake(ins);
  zero_p = NrcInstanceZeroPenalty(ins);
  ** zero_p = NrcPenalty(false, 0, NRC_COST_FUNCTION_LINEAR); **
  if( NrcBoundMin(b, &min_value, &allow_zero, &below_min_p) )
  {
    HnAssert(!allow_zero, "NrcDemandSetMakeFromBound: allow_zero in bound");
    HnAssert(NrcPenaltyCostFn(below_min_p) == NRC_COST_FUNCTION_LINEAR,
      "NrcDemandSetMakeFromBound: cost_fn in min penalty not linear");
    if( NrcBoundPreferred(b, &pref_value, &below_pref_p, &above_pref_p) )
    {
      HnAssert(NrcPenaltyCostFn(below_pref_p) == NRC_COST_FUNCTION_LINEAR,
        "NrcDemandSetMakeFromBound: cost_fn in below_pref penalty not linear");
      HnAssert(NrcPenaltyCostFn(above_pref_p) == NRC_COST_FUNCTION_LINEAR,
        "NrcDemandSetMakeFromBound: cost_fn in above_pref penalty not linear");
      if( NrcBoundMax(b, &max_value, &above_max_p) )
      {
	HnAssert(NrcPenaltyCostFn(above_max_p) == NRC_COST_FUNCTION_LINEAR,
	  "NrcDemandSetMakeFromBound: cost_fn in max penalty not linear");

	** min, pref, max **
	if( min_value > 0 )
	{
	  d = N rcDemandMake(ins, below_min_p, zero_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, min_value);
	}
	if( pref_value - min_value > 0 )
	{
	  d = N rcDemandMake(ins, below_pref_p, zero_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, pref_value - min_value);
	}
	if( max_value - pref_value > 0 )
	{
	  d = N rcDemandMake(ins, zero_p, above_pref_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, max_value - pref_value);
	}
	if( count - max_value > 0 )
	{
	  d = N rcDemandMake(ins, zero_p, above_max_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, count - max_value);
	}
      }
      else
      {
	** min, pref, no max **
	if( min_value > 0 )
	{
	  d = N rcDemandMake(ins, below_min_p, zero_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, min_value);
	}
	if( pref_value - min_value > 0 )
	{
	  d = N rcDemandMake(ins, below_pref_p, zero_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, pref_value - min_value);
	}
	if( count - pref_value > 0 )
	{
	  d = N rcDemandMake(ins, zero_p, above_pref_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, count - pref_value);
	}
      }
    }
    else
    {
      if( NrcBoundMax(b, &max_value, &above_max_p) )
      {
	HnAssert(NrcPenaltyCostFn(above_max_p) == NRC_COST_FUNCTION_LINEAR,
	  "NrcDemandSetMakeFromBound: cost_fn in max penalty not linear");

	** min, no pref, max **
	if( min_value > 0 )
	{
	  d = N rcDemandMake(ins, below_min_p, zero_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, min_value);
	}
	if( max_value - min_value > 0 )
	{
	  d = N rcDemandMake(ins, zero_p, zero_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, max_value - min_value);
	}
	if( count - max_value > 0 )
	{
	  d = N rcDemandMake(ins, zero_p, above_max_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, count - max_value);
	}
      }
      else
      {
	** min, no pref, no max **
	if( min_value > 0 )
	{
	  d = N rcDemandMake(ins, below_min_p, zero_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, min_value);
	}
	if( count - min_value > 0 )
	{
	  d = N rcDemandMake(ins, zero_p, zero_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, count - min_value);
	}
      }
    }
  }
  else
  {
    if( NrcBoundPreferred(b, &pref_value, &below_pref_p, &above_pref_p) )
    {
      HnAssert(NrcPenaltyCostFn(below_pref_p) == NRC_COST_FUNCTION_LINEAR,
        "NrcDemandSetMakeFromBound: cost_fn in below_pref penalty not linear");
      HnAssert(NrcPenaltyCostFn(above_pref_p) == NRC_COST_FUNCTION_LINEAR,
        "NrcDemandSetMakeFromBound: cost_fn in above_pref penalty not linear");
      if( NrcBoundMax(b, &max_value, &above_max_p) )
      {
	HnAssert(NrcPenaltyCostFn(above_max_p) == NRC_COST_FUNCTION_LINEAR,
	  "NrcDemandSetMakeFromBound: cost_fn in max penalty not linear");

	** no min, pref, max **
	if( pref_value > 0 )
	{
	  d = N rcDemandMake(ins, below_pref_p, zero_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, pref_value);
	}
	if( max_value - pref_value > 0 )
	{
	  d = N rcDemandMake(ins, zero_p, above_pref_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, max_value - pref_value);
	}
	if( count - max_value > 0 )
	{
	  d = N rcDemandMake(ins, zero_p, above_max_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, count - max_value);
	}
      }
      else
      {
	** no min, pref, no max **
	if( pref_value > 0 )
	{
	  d = N rcDemandMake(ins, below_pref_p, zero_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, pref_value);
	}
	if( count - pref_value > 0 )
	{
	  d = N rcDemandMake(ins, zero_p, above_pref_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, count - pref_value);
	}
      }
    }
    else
    {
      if( NrcBoundMax(b, &max_value, &above_max_p) )
      {
	HnAssert(NrcPenaltyCostFn(above_max_p) == NRC_COST_FUNCTION_LINEAR,
	  "NrcDemandSetMakeFromBound: cost_fn in max penalty not linear");

	** no min, no pref, max **
	if( max_value > 0 )
	{
	  d = N rcDemandMake(ins, zero_p, zero_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, max_value);
	}
	if( count - max_value > 0 )
	{
	  d = N rcDemandMake(ins, zero_p, above_max_p, preferred_ws,
	    not_preferred_penalty);
	  NrcDemandSetAddDemandMulti(res, d, count - max_value);
	}
      }
      else
      {
	** no min, no pref, no max **
	d = N rcDemandMake(ins, zero_p, zero_p, preferred_ws,
	  not_preferred_penalty);
	NrcDemandSetAddDemandMulti(res, d, count);
      }
    }
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int NrcDemandSetDemandCount(NRC_DEMAND_SET ds)                           */
/*                                                                           */
/*  Return the number of demands of ds.                                      */
/*                                                                           */
/*****************************************************************************/

int NrcDemandSetDemandCount(NRC_DEMAND_SET ds)
{
  return HaArrayCount(ds->demands);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DEMAND NrcDemandSetDemand(NRC_DEMAND_SET ds, int i)                  */
/*                                                                           */
/*  Return the i'th demand of ds.                                            */
/*                                                                           */
/*****************************************************************************/

NRC_DEMAND NrcDemandSetDemand(NRC_DEMAND_SET ds, int i)
{
  return HaArray(ds->demands, i);
}



/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandSetDebug(NRC_DEMAND_SET ds, int indent, FILE *fp)          */
/*                                                                           */
/*  Debug printf of ds onto fp with the given indent.                        */
/*                                                                           */
/*****************************************************************************/

void NrcDemandSetDebug(NRC_DEMAND_SET ds, int indent, FILE *fp)
{
  NRC_DEMAND d;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ DemandSet:\n", indent, "");
    HaArrayForEach(ds->demands, d, i)
      NrcDemandDebug(d, 0, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "{%d demands}", HaArrayCount(ds->demands));
}

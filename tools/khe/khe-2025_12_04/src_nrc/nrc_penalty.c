
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
/*  FILE:         nrc_penalty.c                                              */
/*  MODULE:       A penalty (hard, weight, cost_fn)                          */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY                                                              */
/*                                                                           */
/*  A penalty.                                                               */
/*                                                                           */
/*****************************************************************************/

struct nrc_penalty_rec {
  bool			hard;
  int			weight;
  NRC_COST_FUNCTION	cost_fn;
  char			*id;
};


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY NrcPenalty(bool hard, int weight, NRC_COST_FUNCTION cost_fn, */
/*    NRC_INSTANCE ins)                                                      */
/*                                                                           */
/*  Make a new penalty object with these attributes.                         */
/*                                                                           */
/*****************************************************************************/

NRC_PENALTY NrcPenalty(bool hard, int weight, NRC_COST_FUNCTION cost_fn,
  NRC_INSTANCE ins)
{
  NRC_PENALTY res;  HA_ARENA a;

  /* make the object */
  HnAssert(weight >= 0, "NrcPenalty: negative weight");
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->hard = (weight == 0 ? false : hard);
  res->weight = min(weight, 1000);
  res->cost_fn = (weight == 0 ? NRC_COST_FUNCTION_LINEAR : cost_fn);

  /* check legal cost_fn and also build the id */
  switch( cost_fn )
  {
    case NRC_COST_FUNCTION_STEP:

      res->id = HnStringMake(a, "%c%ds", hard ? 'h' : 's', res->weight);
      break;

    case NRC_COST_FUNCTION_LINEAR:

      res->id = HnStringMake(a, "%c%d", hard ? 'h' : 's', res->weight);
      break;

    case NRC_COST_FUNCTION_QUADRATIC:

      res->id = HnStringMake(a, "%c%dq", hard ? 'h' : 's', res->weight);
      break;

    default:

      HnAbort("NrcPenalty: invalid cost_fn (%d)", cost_fn);
      break;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcPenaltyHard(NRC_PENALTY p)                                       */
/*                                                                           */
/*  Return the hard atribute of p.                                           */
/*                                                                           */
/*****************************************************************************/

bool NrcPenaltyHard(NRC_PENALTY p)
{
  return p->hard;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcPenaltyWeight(NRC_PENALTY p)                                      */
/*                                                                           */
/*  Return the weight attribute of p.                                        */
/*                                                                           */
/*****************************************************************************/

int NrcPenaltyWeight(NRC_PENALTY p)
{
  return p->weight;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_COST_FUNCTION NrcPenaltyCostFn(NRC_PENALTY p)                        */
/*                                                                           */
/*  Return the cost function attribute of p.                                 */
/*                                                                           */
/*****************************************************************************/

NRC_COST_FUNCTION NrcPenaltyCostFn(NRC_PENALTY p)
{
  return p->cost_fn;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcPenaltyShow(NRC_PENALTY p)                                      */
/*                                                                           */
/*  Show the Id of p (a brief string which identifies its value).            */
/*                                                                           */
/*****************************************************************************/

char *NrcPenaltyShow(NRC_PENALTY p)
{
  return p->id;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcPenaltyEqual(NRC_PENALTY p1, NRC_PENALTY p2)                     */
/*                                                                           */
/*  Return true if p1 and p2 have equal attributes.                          */
/*                                                                           */
/*****************************************************************************/

bool NrcPenaltyEqual(NRC_PENALTY p1, NRC_PENALTY p2)
{
  return p1->hard == p2->hard && p1->weight == p2->weight &&
    p1->cost_fn == p2->cost_fn;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcPenaltyLessThan(NRC_PENALTY p1, NRC_PENALTY p2)                  */
/*                                                                           */
/*  Return true if p1 is less than p2.  Cost functions must be equal.        */
/*                                                                           */
/*  Implementation note.  This function assumes that if the weight is 0      */
/*  then the penalty is soft, as enforced by NrcPenalty().                   */
/*                                                                           */
/*****************************************************************************/

bool NrcPenaltyLessThan(NRC_PENALTY p1, NRC_PENALTY p2)
{
  HnAssert(p1->cost_fn == p2->cost_fn,
    "NrcPenaltyLessThan: cost functions differ");
  if( p1->hard )
    return (p2->hard ? p1->weight < p2->weight : false);
  else
    return (p2->hard ? true : p1->weight < p2->weight);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY NrcPenaltyAdd(NRC_PENALTY p1, NRC_PENALTY p2,                */
/*    NRC_INSTANCE ins)                                                      */
/*                                                                           */
/*  Return the sum of p1 and p2.                                             */
/*                                                                           */
/*****************************************************************************/

NRC_PENALTY NrcPenaltyAdd(NRC_PENALTY p1, NRC_PENALTY p2, NRC_INSTANCE ins)
{
  if( p1->weight == 0 )
    return p2;
  else if( p2->weight == 0 )
    return p1;
  else if( p1->hard != p2->hard )
    return p1->hard ? p1 : p2;
  else
  {
    HnAssert(p1->cost_fn == p2->cost_fn, "NrcPenaltyAdd: cost fns differ");
    return NrcPenalty(p1->hard, p1->weight + p2->weight, p1->cost_fn, ins);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY NrcPenaltyAdjust(NRC_PENALTY p, int subtract_weight)         */
/*                                                                           */
/*  Return a new penalty which is the same as p only with its weight         */
/*  reduced by subtract_weight.                                              */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
NRC_PENALTY NrcPenaltyAdjust(NRC_PENALTY p, int subtract_weight,
  NRC_INSTANCE ins)
{
  HnAssert(p->weight - subtract_weight >= 0, "NrcPenaltyAdjust: bad weight");
  return NrcPenalty(p->hard, p->weight - subtract_weight, p->cost_fn, ins);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int NrcPenaltyTypedCmp(NRC_PENALTY p1, NRC_PENALTY p2)                   */
/*                                                                           */
/*  Comparison function for comparing two penalties.                         */
/*                                                                           */
/*  Either or both may be NULL.  Two NULL penalties compare equal,           */
/*  a NULL and a non-NULL penalty compare unequal.                           */
/*                                                                           */
/*****************************************************************************/

int NrcPenaltyTypedCmp(NRC_PENALTY p1, NRC_PENALTY p2)
{
  int cmp;
  if( p1 == NULL )
    return p2 == NULL ? 0 : -1;
  else if( p2 == NULL )
    return 1;
  else
  {
    cmp = (int) p1->hard - (int) p2->hard;
    if( cmp != 0 )
      return cmp;
    cmp = p1->weight - p2->weight;
    if( cmp != 0 )
      return cmp;
    cmp = (int) p1->cost_fn - (int) p2->cost_fn;
    return cmp;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcPenaltyCmp(const void *p1, const void *p2)                        */
/*                                                                           */
/*  Comparison function for sorting an array of penalties                    */
/*                                                                           */
/*****************************************************************************/

int NrcPenaltyCmp(const void *p1, const void *p2)
{
  NRC_PENALTY pen1 = * (NRC_PENALTY *) p1;
  NRC_PENALTY pen2 = * (NRC_PENALTY *) p2;
  return NrcPenaltyTypedCmp(pen1, pen2);
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcCostFnShow(NRC_COST_FUNCTION cost_fn)                           */
/*                                                                           */
/*  Show cost_fn.                                                            */
/*                                                                           */
/*****************************************************************************/

char *NrcCostFnShow(NRC_COST_FUNCTION cost_fn)
{
  switch( cost_fn )
  {
    case NRC_COST_FUNCTION_STEP:

      return "step";

    case NRC_COST_FUNCTION_LINEAR:

      return "linear";

    case NRC_COST_FUNCTION_QUADRATIC:

      return "quadratic";

    default:

      HnAbort("NrcCostFnShow: invalid cost function (%d)", cost_fn);
      return NULL;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcPenaltyShow(NRC_PENALTY p)                                      */
/*                                                                           */
/*  Return a string display of p.                                            */
/*                                                                           */
/*****************************************************************************/

/* ***
static char *show_hard(bool hard) { return hard ? "hard" : "soft"; }

char *NrcPenaltyShow(NRC_PENALTY p, HA_ARENA a)
{
  if( p == NULL )
    return HnStringMake(a, "-");
  else switch( p->cost_fn )
  {
    case NRC_COST_FUNCTION_STEP:

      return HnStringMake(a, "%s:%d:step", show_hard(p->hard), p->weight);

    case NRC_COST_FUNCTION_LINEAR:

      return HnStringMake(a, "%s:%d", show_hard(p->hard), p->weight);

    case NRC_COST_FUNCTION_QUADRATIC:

      return HnStringMake(a, "%s:%d:quadratic", show_hard(p->hard), p->weight);

    default:

      HnAbort("NrcPenaltyShow: invalid cost function (%d)", p->cost_fn);
      return NULL;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST_FUNCTION NrcPenaltyKheCostFn(NRC_PENALTY p)                     */
/*                                                                           */
/*  Return p's cost function attribute, converted to a KHE cost function.    */
/*                                                                           */
/*****************************************************************************/

KHE_COST_FUNCTION NrcPenaltyKheCostFn(NRC_PENALTY p)
{
  switch( p->cost_fn )
  {
    case NRC_COST_FUNCTION_STEP:

      return KHE_STEP_COST_FUNCTION;

    case NRC_COST_FUNCTION_LINEAR:

      return KHE_LINEAR_COST_FUNCTION;

    case NRC_COST_FUNCTION_QUADRATIC:

      return KHE_QUADRATIC_COST_FUNCTION;

    default:

      HnAbort("NrcPenaltyKheCostFn: invalid cost function (%d)", p->cost_fn);
      return KHE_LINEAR_COST_FUNCTION;  /* keep compiler happy */
  }
}


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
/*  FILE:         nrc_bound.c                                                */
/*  MODULE:       A bound (a set of limits, with penalties)                  */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"
#define NO_VALUE -1

#define DEBUG1 0
#define DEBUG2 0

/*****************************************************************************/
/*                                                                           */
/*  NRC_BOUND_ITEM                                                           */
/*                                                                           */
/*  One item in the list of constraints that implement a bound.              */
/*                                                                           */
/*  Value -1 for min_value or max_value signifies not present.               */
/*                                                                           */
/*****************************************************************************/

typedef struct nrc_bound_item_rec {
  int		min_value;
  bool		allow_zero;
  int		max_value;
  NRC_PENALTY	penalty;
} *NRC_BOUND_ITEM;

typedef HA_ARRAY(NRC_BOUND_ITEM) ARRAY_NRC_BOUND_ITEM;


/*****************************************************************************/
/*                                                                           */
/*  NRC_BOUND                                                                */
/*                                                                           */
/*  A bound.                                                                 */
/*                                                                           */
/*  Value -1 for min_value, max_value, or pref_value signifies not present.  */
/*                                                                           */
/*****************************************************************************/

struct nrc_bound_rec {
  NRC_INSTANCE		instance;
  int			min_value;
  bool			allow_zero;
  NRC_PENALTY		below_min_p;
  int			max_value;
  NRC_PENALTY		above_max_p;
  int			pref_value;
  NRC_PENALTY		below_pref_p;
  NRC_PENALTY		above_pref_p;
  bool			items_done;
  ARRAY_NRC_BOUND_ITEM	items;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "creation and query"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_BOUND NrcBoundMake(void)                                             */
/*                                                                           */
/*  Make a new bound object that initially does not impose any bound.        */
/*                                                                           */
/*****************************************************************************/

NRC_BOUND NrcBoundMake(NRC_INSTANCE ins)
{
  NRC_BOUND res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->instance = ins;
  res->min_value = NO_VALUE;
  res->allow_zero = false;
  res->below_min_p = NULL;
  res->max_value = NO_VALUE;
  res->above_max_p = NULL;
  res->pref_value = NO_VALUE;
  res->below_pref_p = NULL;
  res->above_pref_p = NULL;
  res->items_done = false;
  HaArrayInit(res->items, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcBoundMinAndPrefStepWeight(NRC_BOUND b)                            */
/*                                                                           */
/*  Return the weight of the penalty of the step constraint needed to        */
/*  glue a min and pref together, assuming both are present and the min      */
/*  value is less than the pref value (if these conditions do not hold,      */
/*  there can be no reason to calculate this weight).  If the return         */
/*  value is negative, they are not compatible.  If it is 0, they are        */
/*  compatible but no actual step constraint is needed.                      */
/*                                                                           */
/*  See the User's Guide for the algebra underlying this function.           */
/*                                                                           */
/*****************************************************************************/

int NrcBoundMinAndPrefStepWeight(NRC_BOUND b)
{
  int x1, x2, w1, w2;
  HnAssert(b->min_value != NO_VALUE && b->pref_value != NO_VALUE &&
    b->min_value < b->pref_value,
    "NrcBoundMinAndPrefStepWeight internal error");
  x1 = b->pref_value;
  w1 = NrcPenaltyWeight(b->below_pref_p);
  x2 = b->min_value;
  w2 = NrcPenaltyWeight(b->below_min_p);
  return w2 - w1 * (x1 - x2 + 1);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcMinAndPrefConsistent(NRC_BOUND b)                                */
/*                                                                           */
/*  Return true if the min and pref parts of b are consistent.               */
/*                                                                           */
/*****************************************************************************/

static bool NrcMinAndPrefConsistent(NRC_BOUND b)
{
  if( b->min_value == NO_VALUE || b->pref_value == NO_VALUE )
    return true;
  else if( b->min_value == b->pref_value )
    return true;
  else if( b->min_value > b->pref_value || b->allow_zero )
    return false;
  else if( !NrcPenaltyHard(b->below_pref_p) && NrcPenaltyHard(b->below_min_p) )
  {
    /* new; we allow a hard min constraint to override a soft preferred */
    return true;
  }
  else
    return NrcPenaltyHard(b->below_pref_p) == NrcPenaltyHard(b->below_min_p) &&
      NrcPenaltyCostFn(b->below_min_p) == NRC_COST_FUNCTION_LINEAR &&
      NrcPenaltyCostFn(b->below_pref_p) == NRC_COST_FUNCTION_LINEAR &&
      NrcBoundMinAndPrefStepWeight(b) >= 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcBoundMaxAndPrefStepWeight(NRC_BOUND b)                            */
/*                                                                           */
/*  Return the weight of the penalty of the step constraint needed to        */
/*  glue a max and pref together, assuming both are present and the max      */
/*  value is less than the pref value (if these conditions do not hold,      */
/*  there can be no reason to calculate this weight).  If the return         */
/*  value is negative, they are not compatible.  If it is 0, they are        */
/*  compatible but no actual step constraint is needed.                      */
/*                                                                           */
/*  See the User's Guide for the algebra underlying this function.           */
/*                                                                           */
/*****************************************************************************/

int NrcBoundMaxAndPrefStepWeight(NRC_BOUND b)
{
  int x1, x2, w1, w2;
  HnAssert(b->max_value != NO_VALUE && b->pref_value != NO_VALUE &&
    b->max_value > b->pref_value,
    "NrcBoundMaxAndPrefStepWeight internal error");
  x1 = b->pref_value;
  w1 = NrcPenaltyWeight(b->above_pref_p);
  x2 = b->max_value;
  w2 = NrcPenaltyWeight(b->above_max_p);
  return w2 - w1 * (x2 - x1 + 1);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcMaxAndPrefConsistent(NRC_BOUND b)                                */
/*                                                                           */
/*  Return true if the max and pref parts of b are consistent.               */
/*                                                                           */
/*****************************************************************************/

static bool NrcMaxAndPrefConsistent(NRC_BOUND b)
{
  if( b->max_value == NO_VALUE || b->pref_value == NO_VALUE )
    return true;
  else if( b->max_value == b->pref_value )
    return true;
  else if( b->max_value < b->pref_value )
    return false;
  else if( !NrcPenaltyHard(b->above_pref_p) && NrcPenaltyHard(b->above_max_p) )
  {
    /* new; we allow a hard max constraint to override a soft preferred */
    return true;
  }
  else
    return NrcPenaltyHard(b->above_pref_p) == NrcPenaltyHard(b->above_max_p) &&
      NrcPenaltyCostFn(b->above_max_p) == NRC_COST_FUNCTION_LINEAR &&
      NrcPenaltyCostFn(b->above_pref_p) == NRC_COST_FUNCTION_LINEAR &&
      NrcBoundMaxAndPrefStepWeight(b) >= 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcMinAndMaxConsistent(NRC_BOUND b)                                 */
/*                                                                           */
/*  Return true if the min and max parts of b are consistent.                */
/*                                                                           */
/*****************************************************************************/

static bool NrcMinAndMaxConsistent(NRC_BOUND b)
{
  if( b->min_value == NO_VALUE || b->max_value == NO_VALUE )
    return true;
  else
    return b->min_value <= b->max_value;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcBoundAddMin(NRC_BOUND b, int min_value, bool allow_zero,         */
/*    NRC_PENALTY below_min_penalty)                                         */
/*                                                                           */
/*  Add a min value with penalty and optional allow_zero.                    */
/*                                                                           */
/*****************************************************************************/

bool NrcBoundAddMin(NRC_BOUND b, int min_value,
  bool allow_zero, NRC_PENALTY below_min_penalty)
{
  bool res;
  if( b->min_value != NO_VALUE )
    return false;
  HnAssert(min_value >= 0, "NrcBoundAddMin: negative min_value");
  HnAssert(below_min_penalty != NULL, "NrcBoundAddMin: NULL below_min_penalty");
  b->min_value = min_value;
  b->allow_zero = allow_zero;
  b->below_min_p = below_min_penalty;
  res = NrcMinAndPrefConsistent(b) && NrcMinAndMaxConsistent(b);
  if( !res )
    b->min_value = NO_VALUE, b->allow_zero = false, b->below_min_p = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcBoundAddMax(NRC_BOUND b, int max_value,                          */
/*    NRC_PENALTY above_max_penalty)                                         */
/*                                                                           */
/*  Add a max value with penalty.                                            */
/*                                                                           */
/*****************************************************************************/

bool NrcBoundAddMax(NRC_BOUND b, int max_value, NRC_PENALTY above_max_penalty)
{
  bool res;
  if( b->max_value != NO_VALUE )
    return false;
  HnAssert(max_value >= 0, "NrcBoundAddMax: negative max_value");
  HnAssert(above_max_penalty != NULL, "NrcBoundAddMax: NULL above_max_penalty");
  b->max_value = max_value;
  b->above_max_p = above_max_penalty;
  res = NrcMaxAndPrefConsistent(b) && NrcMinAndMaxConsistent(b);
  if( !res )
    b->max_value = NO_VALUE, b->above_max_p = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcBoundAddPreferred(NRC_BOUND b, int preferred_value,              */
/*    NRC_PENALTY below_preferred_penalty,                                   */
/*    NRC_PENALTY above_preferred_penalty)                                   */
/*                                                                           */
/*  Add a preferred value with penalties.                                    */
/*                                                                           */
/*****************************************************************************/

bool NrcBoundAddPreferred(NRC_BOUND b, int preferred_value,
  NRC_PENALTY below_preferred_penalty, NRC_PENALTY above_preferred_penalty)
{
  bool res;
  if( b->pref_value != NO_VALUE )
    return false;
  HnAssert(preferred_value >= 0,
    "NrcBoundAddPreferred: negative preferred_value");
  HnAssert(below_preferred_penalty != NULL,
    "NrcBoundAddPreferred: NULL below_preferred_penalty");
  HnAssert(above_preferred_penalty != NULL,
    "NrcBoundAddPreferred: NULL above_preferred_penalty");
  b->pref_value = preferred_value;
  b->below_pref_p = below_preferred_penalty;
  b->above_pref_p = above_preferred_penalty;
  res = NrcMinAndPrefConsistent(b) && NrcMaxAndPrefConsistent(b);
  if( !res )
    b->pref_value = NO_VALUE, b->below_pref_p = b->above_pref_p = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_BOUND NrcBoundMakeMin(int min_value, bool allow_zero,                */
/*    NRC_PENALTY below_min_penalty)                                         */
/*                                                                           */
/*  Make a new bound object and add a minimum limit to it.                   */
/*                                                                           */
/*****************************************************************************/

NRC_BOUND NrcBoundMakeMin(int min_value, bool allow_zero,
  NRC_PENALTY below_min_penalty, NRC_INSTANCE ins)
{
  NRC_BOUND res;
  res = NrcBoundMake(ins);
  NrcBoundAddMin(res, min_value, allow_zero, below_min_penalty);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_BOUND NrcBoundMakeMax(int max_value, NRC_PENALTY above_max_penalty)  */
/*                                                                           */
/*  Make a new bound object and add a maximum limit to it.                   */
/*                                                                           */
/*****************************************************************************/

NRC_BOUND NrcBoundMakeMax(int max_value, NRC_PENALTY above_max_penalty,
  NRC_INSTANCE ins)
{
  NRC_BOUND res;
  res = NrcBoundMake(ins);
  NrcBoundAddMax(res, max_value, above_max_penalty);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_BOUND NrcBoundMakePreferred(int preferred_value,                     */
/*    NRC_PENALTY below_preferred_penalty,                                   */
/*    NRC_PENALTY above_preferred_penalty)                                   */
/*                                                                           */
/*  Make a new bound object and add a preferred limit to it.                 */
/*                                                                           */
/*****************************************************************************/

NRC_BOUND NrcBoundMakePreferred(int preferred_value,
  NRC_PENALTY below_preferred_penalty, NRC_PENALTY above_preferred_penalty,
  NRC_INSTANCE ins)
{
  NRC_BOUND res;
  res = NrcBoundMake(ins);
  NrcBoundAddPreferred(res, preferred_value, below_preferred_penalty,
    above_preferred_penalty);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcBoundMin(NRC_BOUND b, int *min_value, bool *allow_zero,          */
/*    NRC_PENALTY *below_min_penalty)                                        */
/*                                                                           */
/*  If b has a minimum limit, return true with *min_value, *allow_zero,      */
/*  and *below_min_penalty set to its attributes.  Otherwise return false.   */
/*                                                                           */
/*****************************************************************************/

bool NrcBoundMin(NRC_BOUND b, int *min_value, bool *allow_zero,
  NRC_PENALTY *below_min_penalty)
{
  *min_value = b->min_value;
  *allow_zero = b->allow_zero;
  *below_min_penalty = b->below_min_p;
  return b->min_value != NO_VALUE;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcBoundMax(NRC_BOUND b, int *max_value,                            */
/*    NRC_PENALTY *above_max_penalty)                                        */
/*                                                                           */
/*  If b has a maximum limit, return true with *max_value and                */
/*  *above_max_penalty set to its attributes.  Otherwise return false.       */
/*                                                                           */
/*****************************************************************************/

bool NrcBoundMax(NRC_BOUND b, int *max_value,
  NRC_PENALTY *above_max_penalty)
{
  *max_value = b->max_value;
  *above_max_penalty = b->above_max_p;
  return b->max_value != NO_VALUE;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcBoundPreferred(NRC_BOUND b, int *preferred_value,                */
/*    NRC_PENALTY *below_preferred_penalty,                                  */
/*    NRC_PENALTY *above_preferred_penalty)                                  */
/*                                                                           */
/*  If b has a preferred limit, return true with *preferred_value,           */
/*  *below_preferred_penalty, and *above_preferred_penalty set to its        */
/*  attributes.  Otherwise return false.                                     */
/*                                                                           */
/*****************************************************************************/

bool NrcBoundPreferred(NRC_BOUND b, int *preferred_value,
  NRC_PENALTY *below_preferred_penalty, NRC_PENALTY *above_preferred_penalty)
{
  *preferred_value = b->pref_value;
  *below_preferred_penalty = b->below_pref_p;
  *above_preferred_penalty =  b->above_pref_p;
  return b->pref_value != NO_VALUE;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "bound comparison and merging"                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int NrcBoundTypedCmp(NRC_BOUND b1, NRC_BOUND b2)                         */
/*                                                                           */
/*  Typed comparison function for bounds.                                    */
/*                                                                           */
/*  Constraints with min bounds come before constraints without.  If both    */
/*  have min bounds, smaller value precedes larger value.                    */
/*                                                                           */
/*  Otherwise, constraints with pref bounds come before constraints          */
/*  without.  If both have pref bounds, smaller value precedes larger.       */
/*                                                                           */
/*  Otherwise, constraints with max bounds come before constraints           */
/*  without.  If both have max bounds, smaller value precedes larger.        */
/*                                                                           */
/*****************************************************************************/

int NrcBoundTypedCmp(NRC_BOUND b1, NRC_BOUND b2)
{
  int cmp;

  /* if one has a min bound but the other doesn't */
  if( b1->min_value != NO_VALUE && b2->min_value == NO_VALUE )
    return -1;
  if( b1->min_value == NO_VALUE && b2->min_value != NO_VALUE )
    return 1;

  /* if both have a min bound, look for a decision there */
  if( b1->min_value != NO_VALUE )
  {
    cmp = (int) b1->allow_zero - (int) b2->allow_zero;
    if( cmp != 0 )  return cmp;
    cmp = b1->min_value - b2->min_value;
    if( cmp != 0 )  return cmp;
    cmp = NrcPenaltyTypedCmp(b1->below_min_p, b2->below_min_p);
    if( cmp != 0 )  return cmp;
  }

  /* if one has a pref bound but the other doesn't */
  if( b1->pref_value != NO_VALUE && b2->pref_value == NO_VALUE )
    return -1;
  if( b1->pref_value == NO_VALUE && b2->pref_value != NO_VALUE )
    return 1;

  /* if both have a pref bound, look for a decision there */
  if( b1->pref_value != NO_VALUE )
  {
    cmp = b1->pref_value - b2->pref_value;
    if( cmp != 0 )  return cmp;
    cmp = NrcPenaltyTypedCmp(b1->below_pref_p, b2->below_pref_p);
    if( cmp != 0 )  return cmp;
    cmp = NrcPenaltyTypedCmp(b1->above_pref_p, b2->above_pref_p);
    if( cmp != 0 )  return cmp;
  }

  /* if one has a max bound but the other doesn't */
  if( b1->max_value != NO_VALUE && b2->max_value == NO_VALUE )
    return -1;
  if( b1->max_value == NO_VALUE && b2->max_value != NO_VALUE )
    return 1;

  /* if both have a max bound, look for a decision there */
  if( b1->max_value != NO_VALUE )
  {
    cmp = b1->max_value - b2->max_value;
    if( cmp != 0 )  return cmp;
    cmp = NrcPenaltyTypedCmp(b1->above_max_p, b2->above_max_p);
    if( cmp != 0 )  return cmp;
  }

  /* all equal */
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcBoundCmp(const void *p1, const void *p2)                          */
/*                                                                           */
/*  Untyped comparison function for bounds.                                  */
/*                                                                           */
/*****************************************************************************/

int NrcBoundCmp(const void *p1, const void *p2)
{
  NRC_BOUND b1 = * (NRC_BOUND *) p1;
  NRC_BOUND b2 = * (NRC_BOUND *) p2;
  return NrcBoundTypedCmp(b1, b2);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcBoundFree(NRC_BOUND b)                                           */
/*                                                                           */
/*  Free b.                                                                  */
/*                                                                           */
/*****************************************************************************/

/* *** now unused
static void NrcBoundFree(NRC_BOUND b)
{
  MFree(b);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool NrcBoundMerge(NRC_BOUND b1, NRC_BOUND b2, NRC_BOUND *res)           */
/*                                                                           */
/*  If b1 and b2 can be merged into a single bound, return true and set      */
/*  *res to that bound.  Otherwise return false.                             */
/*                                                                           */
/*  This function is for merging worker constraints.  It will not merge      */
/*  constraints with preferred limits; it will only merge one min limit      */
/*  with one max limit.                                                      */
/*                                                                           */
/*****************************************************************************/

bool NrcBoundMerge(NRC_BOUND b1, NRC_BOUND b2, NRC_BOUND *res)
{
  NRC_BOUND tmp;

  /* if there are preferred limits, no good */
  if( b1->pref_value != NO_VALUE || b2->pref_value != NO_VALUE )
    return false;

  /* if both have a min limit, no good */
  if( b1->min_value != NO_VALUE && b2->min_value != NO_VALUE )
    return false;

  /* if both have a max limit, no good */
  if( b1->max_value != NO_VALUE && b2->max_value != NO_VALUE )
    return false;

  /* make b1 the bound with the min limit, b2 the bound with the max limit */
  if( b1->max_value != NO_VALUE )
    tmp = b1, b1 = b2, b2 = tmp;

  /* if the penalties differ, no good */
  if( NrcPenaltyTypedCmp(b1->below_min_p, b2->above_max_p) != 0 )
    return false;

  /* make a new bound with b1's min limit and b2's max limit */
  *res = NrcBoundMake(b1->instance);
  NrcBoundAddMin(*res, b1->min_value, b1->allow_zero, b1->below_min_p);
  NrcBoundAddMax(*res, b2->max_value, b2->above_max_p);
  return true;

  /* *** obsolete below here
  ** make the new bound and add b1's parts to it; this must succeed **
  if( b1->min_value != NO_VALUE )
  if( b1->max_value != NO_VALUE )
    NrcBoundAddMax(*res, b1->max_value, b1->above_max_p);
  if( b1->pref_value != NO_VALUE )
    NrcBoundAddPreferred(*res, b1->pref_value, b1->below_pref_p,
      b1->above_pref_p);

  ** add b2's parts; these additions can fail **
  if( b2->min_value != NO_VALUE )
    if( !NrcBoundAddMin(*res, b2->min_value, b2->allow_zero, b2->below_min_p) )
      return NrcBoundFree(*res), false;
  if( b2->max_value != NO_VALUE )
      return NrcBoundFree(*res), false;
  if( b2->pref_value != NO_VALUE )
    if( !NrcBoundAddPreferred(*res, b2->pref_value, b2->below_pref_p,
	  b2->above_pref_p) )
      return NrcBoundFree(*res), false;

  ** all good **
  return true;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcBoundBCDTMerge(NRC_BOUND b[], int bound_count, NRC_BOUND *res)   */
/*                                                                           */
/*  If b[0 ... bound_count - 1] (which are sorted) fit the pattern of the    */
/*  BCDT bounds, merge them, set *res to the merged bound, and return true.  */
/*  Otherwise return false.                                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool NrcBoundBCDTMerge(NRC_BOUND b[], int bound_count, NRC_BOUND *res)
{
  if(
    ** there must be exactly 4 bounds **
    bound_count == 4 &&

    ** the first must be a hard min constraint only **
    b[0]->min_value != NO_VALUE && b[0]->pref_value == NO_VALUE &&
    b[0]->max_value == NO_VALUE && NrcPenaltyHard(b[0]->below_min_p) &&

    ** the second must be a soft min constraint only **
    b[1]->min_value != NO_VALUE && b[1]->pref_value == NO_VALUE &&
    b[1]->max_value == NO_VALUE && !NrcPenaltyHard(b[1]->below_min_p) &&

    ** the third must be a soft max constraint only **
    b[2]->min_value == NO_VALUE && b[2]->pref_value == NO_VALUE &&
    b[2]->max_value != NO_VALUE && !NrcPenaltyHard(b[2]->above_max_p) &&

    ** the fourth must be a hard max constraint only **
    b[3]->min_value == NO_VALUE && b[3]->pref_value == NO_VALUE &&
    b[3]->max_value != NO_VALUE && NrcPenaltyHard(b[3]->above_max_p) &&

    ** the limits must be suitable **
    b[0]->min_value < b[1]->min_value &&
    b[1]->min_value == b[2]->max_value &&
    b[2]->max_value < b[3]->max_value
  )
  {
    ** the bounds fit the BCDT pattern, so make *res and return true **
    *res = NrcBoundMake(b[0]->instance);
    NrcBoundAddMin(*res, b[0]->min_value, false, b[0]->below_min_p);
    if( !NrcBoundAddPreferred(*res, b[1]->min_value, b[1]->below_min_p,
	b[2]->above_max_p) )
      HnAbort("NrcBoundBCDTMerge internal error 1");
    if( !NrcBoundAddMax(*res, b[3]->max_value, b[3]->above_max_p) )
      HnAbort("NrcBoundBCDTMerge internal error 2");
    return true;
  }
  else
  {
    ** the bounds do not fit the BCDT pattern **
    return *res = NULL, true;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool NrcBoundSpecialMerge(NRC_BOUND bounds[], int bound_count,           */
/*    NRC_INSTANCE ins, NRC_BOUND *res)                                      */
/*                                                                           */
/*  If bounds[0 .. bound_count - 1] can be merged, do that, set *res to      */
/*  the merged bound, and return true.  Otherwise return false.              */
/*                                                                           */
/*  This function makes a special case of the BCDT-Sep merging problem.      */
/*                                                                           */
/*****************************************************************************/

/* ***
bool NrcBoundSpecialMerge(NRC_BOUND bounds[], int bound_count,
  NRC_INSTANCE ins, NRC_BOUND *res)
{
  int i;

  ** sort the bounds **
  qsort(bounds, bound_count, sizeof(NRC_BOUND), &NrcBoundCmp);
  if( DEBUG2 )
  {
    fprintf(stderr, "[ NrcBoundSpecialMerge(");
    for( i = 0;  i < bound_count;  i++ )
      fprintf(stderr, "%s%s", i == 0 ? "" : ", ", NrcBoundShow(bounds[i]));
    fprintf(stderr, ")\n");
  }

  if( bound_count == 0 )
  {
    ** no bounds, means unconstrained **
    if( DEBUG2 )
      fprintf(stderr, "] NrcBoundSpecialMerge returning true (no bounds)\n");
    return *res = NrcBoundMake(ins), true;
  }
  else if( bound_count == 1 )
  {
    ** just one bound, it alone will do the job **
    if( DEBUG2 )
      fprintf(stderr, "] NrcBoundSpecialMerge returning true (one bound)\n");
    return *res = bounds[0], true;
  }
  else if( NrcBoundBCDTMerge(bounds, bound_count, res) )
  {
    ** COI-BCDT-Sep case: hard min, soft min = soft max, hard max **
    if( DEBUG2 )
      fprintf(stderr, "] NrcBoundSpecialMerge returning true (BCDT case %s)\n",
	NrcBoundShow(*res));
    return true;
  }
  else
  {
    ** try a regular merge, may work for one min and one max **
    *res = bounds[0];
    for( i = 1;  i < bound_count;  i++ )
    {
      if( !NrcBoundMerge(*res, bounds[i], res) )
      {
	if( DEBUG2 )
	  fprintf(stderr, "] NrcBoundSpecialMerge returning false (%d)\n", i);
	return *res = NULL, false;
      }
    }
    if( DEBUG2 )
      fprintf(stderr, "] NrcBoundSpecialMerge returning true (%s)\n",
	NrcBoundShow(*res));
    return true;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "bound items and analysis"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_BOUND_ITEM NrcBoundItemMake(int min_value, bool allow_zero,          */
/*    int max_value, NRC_PENALTY penalty)                                    */
/*                                                                           */
/*  Make a new bound item with these attributes.                             */
/*                                                                           */
/*****************************************************************************/

static NRC_BOUND_ITEM NrcBoundItemMake(int min_value, bool allow_zero,
  int max_value, NRC_PENALTY penalty, HA_ARENA a)
{
  NRC_BOUND_ITEM res;
  HaMake(res, a);
  res->min_value = min_value;
  res->allow_zero = allow_zero;
  res->max_value = max_value;
  res->penalty = penalty;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcBoundItemMerge(NRC_BOUND_ITEM bi1, NRC_BOUND_ITEM bi2)           */
/*                                                                           */
/*  If bi1 and bi2 can be merged, merge bi2 into bi1 and return true.        */
/*  Otherwise change nothing and return false.                               */
/*                                                                           */
/*****************************************************************************/

static bool NrcBoundItemMerge(NRC_BOUND_ITEM bi1, NRC_BOUND_ITEM bi2)
{
  /* unmergeable if penalties differ */
  if( NrcPenaltyTypedCmp(bi1->penalty, bi2->penalty) != 0 )
    return false;

  /* unmergeable if both have min or both have max */
  if( bi1->min_value != NO_VALUE && bi2->min_value != NO_VALUE )
    return false;
  if( bi1->max_value != NO_VALUE && bi2->max_value != NO_VALUE )
    return false;

  /* mergeable; merge bi2 into bi1 */
  if( bi2->min_value != NO_VALUE )
    bi1->min_value = bi2->min_value, bi1->allow_zero = bi2->allow_zero;
  if( bi2->max_value != NO_VALUE )
    bi1->max_value = bi2->max_value;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcBoundItemShow(NRC_BOUND_ITEM bi)                                */
/*                                                                           */
/*  Display bi.  The result lies in static memory and will be overwritten    */
/*  by the next call to NrcBoundItemShow.                                    */
/*                                                                           */
/*****************************************************************************/

static char *NrcBoundItemShow(NRC_BOUND_ITEM bi)
{
  static char buff[200];
  sprintf(buff, "[min %d, allow_zero %s, max %d, penalty %s]", bi->min_value,
    bi->allow_zero ? "true" : "false", bi->max_value,
    NrcPenaltyShow(bi->penalty));
  return buff;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcBoundAddMinAndMaxItems(NRC_BOUND b,                              */
/*    int min_value, bool allow_zero, NRC_PENALTY below_min_p,               */
/*    int max_value, NRC_PENALTY above_max_p)                                */
/*                                                                           */
/*  Add one or more items representing these min and max constraints.        */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static void NrcBoundAddMinAndMaxItems(NRC_BOUND b,
  int min_value, bool allow_zero, NRC_PENALTY below_min_p,
  int max_value, NRC_PENALTY above_max_p)
{
  NRC_BOUND_ITEM bi;
  if( NrcPenaltyEqual(below_min_p, above_max_p) )
  {
    ** one item holding both the minimum and maximum limits **
    bi = NrcBoundItemMake(min_value, allow_zero, max_value, below_min_p);
    MArrayAddLast(b->items, bi);
  }
  else
  {
    ** one item holding the minimum limit **
    bi = NrcBoundItemMake(min_value, allow_zero, NO_VALUE, below_min_p);
    MArrayAddLast(b->items, bi);

    ** one item holding the maximum limit **
    bi = NrcBoundItemMake(NO_VALUE, false, max_value, above_max_p);
    MArrayAddLast(b->items, bi);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcBoundAddMinItem(NRC_BOUND b,                                     */
/*    int min_value, NRC_PENALTY below_min_p)                                */
/*                                                                           */
/*  Add an item representing this min constraint.                            */
/*                                                                           */
/*****************************************************************************/

static void NrcBoundAddMinItem(NRC_BOUND b,
  int min_value, bool allow_zero, NRC_PENALTY below_min_p)
{
  NRC_BOUND_ITEM bi;  HA_ARENA a;
  a = NrcInstanceArena(b->instance);
  bi = NrcBoundItemMake(min_value, allow_zero, NO_VALUE, below_min_p, a);
  HaArrayAddLast(b->items, bi);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcBoundAddMaxItem(NRC_BOUND b,                                     */
/*    int max_value, NRC_PENALTY above_max_p)                                */
/*                                                                           */
/*  Add an item representing this max constraint.                            */
/*                                                                           */
/*****************************************************************************/

static void NrcBoundAddMaxItem(NRC_BOUND b,
  int max_value, NRC_PENALTY above_max_p)
{
  NRC_BOUND_ITEM bi;  HA_ARENA a;
  a = NrcInstanceArena(b->instance);
  bi = NrcBoundItemMake(NO_VALUE, false, max_value, above_max_p, a);
  HaArrayAddLast(b->items, bi);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcBoundAddDoubleMinItems(NRC_BOUND b)                              */
/*                                                                           */
/*  Add items representing a double min constraint.                          */
/*                                                                           */
/*****************************************************************************/

static void NrcBoundAddDoubleMinItems(NRC_BOUND b)
{
  int x1, x2, w, w1, w2;  NRC_PENALTY p;  bool hard, allow_zero;

  /* define x1, w1, x2, and w2 as in the algebra, plus hard and allow_zero */
  x1 = b->pref_value;
  w1 = NrcPenaltyWeight(b->below_pref_p);
  x2 = b->min_value;
  w2 = NrcPenaltyWeight(b->below_min_p);
  hard = NrcPenaltyHard(b->below_pref_p);
  allow_zero = b->allow_zero;

  /* a linear constraint with min limit x1 and weight w1 */
  NrcBoundAddMinItem(b, x1, allow_zero, b->below_pref_p);

  /* a step constraint with min limit x2 and weight w2 - w1(x1 - x2 + 1) */
  w = NrcBoundMinAndPrefStepWeight(b);
  if( w > 0 )
  {
    p = NrcPenalty(hard, w, NRC_COST_FUNCTION_STEP, b->instance);
    NrcBoundAddMinItem(b, x2, allow_zero, p);
  }

  /* a linear constraint with min limit x2 - 1 and weight w2 - w1 */
  if( x2 - 1 > 0 )
  {
    p = NrcPenalty(hard, w2 - w1, NRC_COST_FUNCTION_LINEAR, b->instance);
    NrcBoundAddMinItem(b, x2 - 1, allow_zero, p);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcBoundAddDoubleMaxItems(NRC_BOUND b)                              */
/*                                                                           */
/*  Add items representing a double max constraint.                          */
/*                                                                           */
/*****************************************************************************/

static void NrcBoundAddDoubleMaxItems(NRC_BOUND b)
{
  int x1, x2, w, w1, w2;  NRC_PENALTY p;  bool hard;

  /* define x1, w1, x2, and w2 as in the algebra, plus hard */
  x1 = b->pref_value;
  w1 = NrcPenaltyWeight(b->above_pref_p);
  x2 = b->max_value;
  w2 = NrcPenaltyWeight(b->above_max_p);
  hard = NrcPenaltyHard(b->below_pref_p);

  /* a linear constraint with max limit x1 and weight w1 */
  NrcBoundAddMaxItem(b, x1, b->above_pref_p);

  /* a step constraint with max limit x2 and weight w2 - w1(x2 - x1 + 1) */
  w = NrcBoundMaxAndPrefStepWeight(b);
  if( w > 0 )
  {
    p = NrcPenalty(hard, w, NRC_COST_FUNCTION_STEP, b->instance);
    NrcBoundAddMaxItem(b, x2, p);
  }

  /* a linear constraint with min limit x2 - 1 and weight w2 - w1 */
  p = NrcPenalty(hard, w2 - w1, NRC_COST_FUNCTION_LINEAR, b->instance);
  NrcBoundAddMaxItem(b, x2 + 1, p);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcBoundAnalyse(NRC_BOUND b)                                        */
/*                                                                           */
/*  Analyse bound b to work out which XESTT constraints are needed for it.   */
/*                                                                           */
/*****************************************************************************/

static void NrcBoundAnalyse(NRC_BOUND b)
{
  int i, j;  NRC_BOUND_ITEM bii, bij;

  /* handle minimum limits */
  if( DEBUG1 )
    fprintf(stderr, "[ NrcBoundAnalyse(%s)\n", NrcBoundShow(b));
  if( b->min_value != NO_VALUE && b->pref_value != NO_VALUE &&
      b->pref_value != b->min_value )
  {
    /* double min (from a min limit and the min part of pref) */
    NrcBoundAddDoubleMinItems(b);
  }
  else if( b->min_value != NO_VALUE )
  {
    /* single min (from a min limit) */
    NrcBoundAddMinItem(b, b->min_value, b->allow_zero, b->below_min_p);
  }
  else if( b->pref_value != NO_VALUE )
  {
    /* single min (from the min part of pref) */
    NrcBoundAddMinItem(b, b->pref_value, false, b->below_pref_p);
  }

  /* handle maximum limits */
  if( b->max_value != NO_VALUE && b->pref_value != NO_VALUE &&
      b->pref_value != b->max_value )
  {
    /* double max (from a max limit and the max part of pref) */
    NrcBoundAddDoubleMaxItems(b);
  }
  else if( b->max_value != NO_VALUE )
  {
    /* single max (from a max limit) */
    NrcBoundAddMaxItem(b, b->max_value, b->above_max_p);
  }
  else if( b->pref_value != NO_VALUE )
  {
    /* single max (from the max part of pref) */
    NrcBoundAddMaxItem(b, b->pref_value, b->above_pref_p);
  }

  /* merge compatible items */
  for( i = 0;  i < HaArrayCount(b->items);  i++ )
  {
    bii = HaArray(b->items, i);
    for( j = i + 1;  j < HaArrayCount(b->items);  j++ )
    {
      bij = HaArray(b->items, j);
      if( NrcBoundItemMerge(bii, bij) )
      {
	HaArrayDeleteAndPlug(b->items, j);
	j--;
      }
    }
  }

  /* all done */
  b->items_done = true;
  if( DEBUG1 )
  {
    HaArrayForEach(b->items, bii, i)
      fprintf(stderr, "  item %s\n", NrcBoundItemShow(bii));
    fprintf(stderr, "]\n");
  }
}


/* *** old version from before step functions
static void NrcBoundAnalyseOld(NRC_BOUND b)
{
  int wt1, wt2;

  ** ignore min if pref covers it **
  if( b->min_value != NO_VALUE && b->pref_value != NO_VALUE &&
      NrcPenaltyEqual(b->below_min_p, b->below_pref_p) &&
      NrcPenaltyCostFn(b->below_min_p) == NRC_COST_FUNCTION_LINEAR )
    b->min_value = NO_VALUE;

  ** ignore max if pref covers it **
  if( b->max_value != NO_VALUE && b->pref_value != NO_VALUE &&
      NrcPenaltyEqual(b->above_max_p, b->above_max_p) &&
      NrcPenaltyCostFn(b->above_max_p) == NRC_COST_FUNCTION_LINEAR )
    b->max_value = NO_VALUE;

  if( b->min_value != NO_VALUE )
  {
    if( b->pref_value != NO_VALUE )
    {
      if( b->max_value != NO_VALUE )
      {
	** min, pref, and max **
	** do min and max (both adjusted) **
	wt1 = NrcPenaltyWeight(b->below_pref_p);
	wt2 = NrcPenaltyWeight(b->above_pref_p);
	NrcBoundAddMinAndMaxItems(b,
	  b->min_value, b->allow_zero, NrcPenalt yAdjust(b->below_min_p, wt1),
	  b->max_value, NrcPenalty Adjust(b->above_max_p, wt2));

	** do pref **
	NrcBoundAddMinAndMaxItems(b, b->pref_value, false, b->below_pref_p,
	  b->pref_value, b->above_pref_p);
      }
      else
      {
	** min, pref, no max **
	** do min (adjusted) **
	wt1 = NrcPenaltyWeight(b->below_pref_p);
	NrcBoundAddMinItems(b,
	  b->min_value, b->allow_zero, NrcPenalty Adjust(b->below_min_p, wt1));

	** do pref **
	NrcBoundAddMinAndMaxItems(b, b->pref_value, false, b->below_pref_p,
	  b->pref_value, b->above_pref_p);
      }
    }
    else
    {
      if( b->max_value != NO_VALUE )
      {
	** min, no pref, max **
        NrcBoundAddMinAndMaxItems(b, b->min_value, b->allow_zero,
	  b->below_min_p, b->max_value, b->above_max_p);
      }
      else
      {
	** min, no pref, no max **
        NrcBoundAddMinItems(b, b->min_value, b->allow_zero, b->below_min_p);
      }
    }
  }
  else
  {
    if( b->pref_value != NO_VALUE )
    {
      if( b->max_value != NO_VALUE )
      {
	** no min, pref, max **
	** do max (adjusted) **
	wt2 = NrcPenaltyWeight(b->above_pref_p);
	NrcBoundAddMaxItems(b,
	  b->max_value, NrcPenalty Adjust(b->above_max_p, wt2));

	** do pref **
	NrcBoundAddMinAndMaxItems(b, b->pref_value, false, b->below_pref_p,
	  b->pref_value, b->above_pref_p);
      }
      else
      {
	** no min, pref, no max **
        NrcBoundAddMinAndMaxItems(b, b->pref_value, false, b->below_pref_p,
	  b->pref_value, b->above_pref_p);
      }
    }
    else
    {
      if( b->max_value != NO_VALUE )
      {
	** no min, no pref, max **
	NrcBoundAddMaxItems(b, b->max_value, b->above_max_p);
      }
      else
      {
	** no min, no pref, no max **
	** unconstrained, nothing to do **
      }
    }
  }

  ** items done now **
  b->items_done = true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int NrcBoundItemCount(NRC_BOUND b)                                       */
/*                                                                           */
/*  Return the number of items in b.                                         */
/*                                                                           */
/*****************************************************************************/

int NrcBoundItemCount(NRC_BOUND b)
{
  if( !b->items_done )
    NrcBoundAnalyse(b);
  return HaArrayCount(b->items);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcBoundItem(NRC_BOUND b, int i, int *min_value, bool *allow_zero,  */
/*    int *max_value, NRC_PENALTY *penalty)                                  */
/*                                                                           */
/*  Return the i'th item of b.                                               */
/*                                                                           */
/*****************************************************************************/

void NrcBoundItem(NRC_BOUND b, int i, int *min_value, bool *allow_zero,
  int *max_value, NRC_PENALTY *penalty)
{
  NRC_BOUND_ITEM bi;
  if( !b->items_done )
    NrcBoundAnalyse(b);
  bi = HaArray(b->items, i);
  *min_value = (bi->min_value == NO_VALUE ? 0 : bi->min_value);
  *allow_zero = bi->allow_zero;
  *max_value = (bi->max_value == NO_VALUE ? INT_MAX : bi->max_value);
  *penalty = bi->penalty;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *NrcBoundShow(NRC_BOUND b)                                          */
/*                                                                           */
/*  Display b.                                                               */
/*                                                                           */
/*****************************************************************************/

char *NrcBoundShow(NRC_BOUND b)
{
  char buff[3][60];  static char res[200];  int bc;

  /* min */
  bc = 0;
  if( b->min_value != NO_VALUE )
    sprintf(buff[bc++], "min %d %s(%s)", b->min_value,
      b->allow_zero ? "allow_zero" : "", NrcPenaltyShow(b->below_min_p));

  /* pref */
  if( b->pref_value != NO_VALUE )
    sprintf(buff[bc++], "pref %d (%s, %s)", b->pref_value,
      NrcPenaltyShow(b->below_pref_p), NrcPenaltyShow(b->above_pref_p));

  /* max */
  if( b->max_value != NO_VALUE )
    sprintf(buff[bc++], "max %d (%s)", b->max_value,
      NrcPenaltyShow(b->above_max_p));

  /* combine the bits into res */
  if( bc == 0 )
    sprintf(res, "[]");
  else if( bc == 1 )
    sprintf(res, "[%s]", buff[0]);
  else if( bc == 2 )
    sprintf(res, "[%s, %s]", buff[0], buff[1]);
  else
    sprintf(res, "[%s, %s, %s]", buff[0], buff[1], buff[2]);
  return res;
}

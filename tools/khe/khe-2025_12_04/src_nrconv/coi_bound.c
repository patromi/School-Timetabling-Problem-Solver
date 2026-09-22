
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
/*  FILE:         coi_limit.c                                                */
/*  MODULE:       Limits (Min, Max, Preferred) as in the COI source model    */
/*                                                                           */
/*****************************************************************************/
#include "externs.h"
#define str_eq(a, b) (strcmp((a), (b)) == 0)
#define str_eq2(a, b1, b2) (str_eq((a), (b1)) || str_eq((a), (b2)))



/*****************************************************************************/
/*                                                                           */
/*  Submodule "type declarations"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  COI_BOUND - one bound with labels etc.                                   */
/*                                                                           */
/*****************************************************************************/

struct coi_bound_rec {
  char			*label;
  char			*var;
  NRC_BOUND		bound;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  COI_BOUND CoiBoundMake(char *label, char *var, NRC_BOUND b)              */
/*                                                                           */
/*  Make a COI bound object with these attributes.                           */
/*                                                                           */
/*****************************************************************************/

static COI_BOUND CoiBoundMake(char *label, char *var, NRC_BOUND b,
  HA_ARENA a)
{
  COI_BOUND res;
  HaMake(res, a);
  res->label = label;
  res->var = var;
  res->bound = b;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool ReadInt(char *str, int *res, char **err_mess)                       */
/*                                                                           */
/*  Attempt to read an integer from str.  If successful, set *res to         */
/*  its value and return true.  If unsuccessful, set *err_mess to an         */
/*  error message and return false.                                          */
/*                                                                           */
/*****************************************************************************/

static bool ReadInt(char *str, int *res, char **err_mess)
{
  if( strstr(str, ".") != NULL )
    return *err_mess = "non-integral value not implemented", false;
  else if( sscanf(str, "%d", res) != 1 )
    return *err_mess = "format error where integer expected", false;
  else
    return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool ReadLim(KML_ELT lim_elt, bool dft_hard, int dft_weight,             */
/*    char **var, char **label, int *limit, NRC_PENALTY *penalty,            */
/*    char **err_mess, HA_ARENA a)                                           */
/*                                                                           */
/*  Convert lim_elt into a COI_LIMIT object, returning true and setting      */
/*  *var, *label, *limit, and *penalty if successful, and returning false    */
/*  and setting *err_mess if unsuccessful.                                   */
/*                                                                           */
/*  If there is no indication of a weight within lim_elt, ReadLim uses       */
/*  dft_hard and dft_weight for the weight.                                  */
/*                                                                           */
/*  There are two kinds of <Min> and <Max> object, one with the limit in     */
/*  the body like this:                                                      */
/*                                                                           */
/*    <Min weight="100">5</Min>                                              */
/*                                                                           */
/*  and one with the limit in a <Count> child attribute like this:           */
/*                                                                           */
/*    <Min><Count>5</Count><Weight>100</Weight></Min>                        */
/*                                                                           */
/*  ReadLim accepts both kinds.                                              */
/*                                                                           */
/*****************************************************************************/

static bool ReadLim(KML_ELT lim_elt, bool dft_hard, int dft_weight,
  char **var, char **label, int *limit, NRC_PENALTY *penalty,
  char **err_mess, HA_ARENA a, NRC_INSTANCE ins)
{
  char *str;  KML_ELT weight_elt, count_elt, label_elt, var_elt;  KML_ERROR ke;
  bool hard;  int weight;  NRC_COST_FUNCTION cost_fn;

  /* default values for return */
  *label = NULL;
  *var = NULL;
  *penalty = NULL;
  hard = dft_hard;
  weight = dft_weight;
  cost_fn = NRC_COST_FUNCTION_LINEAR;

  if( KmlCheck(lim_elt, ": #Count +#Weight +$Var +$Label", &ke) )
  {
    /* limit */
    count_elt = KmlChild(lim_elt, 0);
    if( !ReadInt(KmlText(count_elt), limit, err_mess) )
      return false;

    /* optional weight with optional function */
    if( KmlContainsChild(lim_elt, "Weight", &weight_elt) )
    {
      if( !KmlContainsAttribute(weight_elt, "function", &str) )
      {
	if( !ReadInt(KmlText(weight_elt), &weight, err_mess) )
	  return false;
	hard = false;
	cost_fn = NRC_COST_FUNCTION_LINEAR;
      }
      else if( str_eq2(str, "Constraint", "constraint") )
      {
	hard = true;
	weight = 1;
	cost_fn = NRC_COST_FUNCTION_LINEAR;
      }
      else if( str_eq2(str, "Linear", "linear") )
      {
	if( !ReadInt(KmlText(weight_elt), &weight, err_mess) )
	  return false;
	hard = false;
	cost_fn = NRC_COST_FUNCTION_LINEAR;
      }
      else if( str_eq2(str, "Quadratic", "quadratic") )
      {
	if( !ReadInt(KmlText(weight_elt), &weight, err_mess) )
	  return false;
	hard = false;
	cost_fn = NRC_COST_FUNCTION_QUADRATIC;
      }
      else if( str_eq2(str, "Constant", "constant") )
      {
	if( !ReadInt(KmlText(weight_elt), &weight, err_mess) )
	  return false;
	hard = false;
	cost_fn = NRC_COST_FUNCTION_STEP;
      }
      else
	return *err_mess = HnStringMake(a, "unknown function %s", str), false;
    }

    /* var */
    if( KmlContainsChild(lim_elt, "Var", &var_elt) )
      *var = HnStringCopy(KmlText(var_elt), a);

    /* label */
    if( KmlContainsChild(lim_elt, "Label", &label_elt) )
      *label = HnStringCopy(KmlText(label_elt), a);
  }
  else if( KmlCheck(lim_elt, "+function +weight", &ke) )
  {
    /* weight */
    if( KmlContainsAttribute(lim_elt, "weight", &str) )
    {
      if( !ReadInt(str, &weight, err_mess) )
	return false;
      hard = false;
      cost_fn = NRC_COST_FUNCTION_LINEAR;
    }

    /* function */
    if( KmlContainsAttribute(lim_elt, "function", &str) )
    {
      if( str_eq2(str, "Constraint", "constraint") )
      {
	if( KmlContainsAttribute(lim_elt, "weight", &str) )
	  return *err_mess = "weight present with Constraint", false;
	hard = true;
	weight = 1;
	cost_fn = NRC_COST_FUNCTION_LINEAR;
      }
      else if( str_eq2(str, "Linear", "linear") )
      {
	hard = false;
	cost_fn = NRC_COST_FUNCTION_LINEAR;
      }
      else if( str_eq2(str, "Quadratic", "quadratic") )
      {
	hard = false;
	cost_fn = NRC_COST_FUNCTION_QUADRATIC;
      }
      else if( str_eq2(str, "Constant", "constant") )
      {
	hard = false;
	cost_fn = NRC_COST_FUNCTION_STEP;
      }
      else
	return *err_mess = HnStringMake(a, "unknown function %s", str), false;
    }

    /* limit */
    if( !ReadInt(KmlText(lim_elt), limit, err_mess) )
      return false;
  }
  else
    return *err_mess = HnStringMake(a, "unknown function %s",
      KmlLabel(lim_elt)), false;

  if( weight == -1 )
    return *err_mess = "weight missing (no default available)", false;
  *penalty = NrcPenalty(hard, weight, cost_fn, ins);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool CoiMinBoundRead(KML_ELT lim_elt, bool dft_below_hard,               */
/*    int dft_below_weight, COI_LIMIT *res, char **err_mess)                 */
/*                                                                           */
/*  Read a minimum bound, using dft_below_hard and dft_below_weight          */
/*  if there is no weight.                                                   */
/*                                                                           */
/*****************************************************************************/

bool CoiMinBoundRead(KML_ELT lim_elt, bool dft_below_hard,
  int dft_below_weight, COI_BOUND *res, char **err_mess, HA_ARENA a,
  NRC_INSTANCE ins)
{
  char *var, *label;  int limit;  NRC_PENALTY penalty;  NRC_BOUND b;
  if( !ReadLim(lim_elt, dft_below_hard, dft_below_weight, &var, &label, &limit,
	&penalty, err_mess, a, ins) )
    return false;
  else
  {
    b = NrcBoundMakeMin(limit, false, penalty, ins);
    return *res = CoiBoundMake(label, var, b, a), true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool CoiPreferredLimitRead(KML_ELT lim_elt, bool dft_below_hard,         */
/*    int dft_below_weight, bool dft_above_hard, int dft_above_weight,       */
/*    COI_BOUND *res, char **err_mess)                                       */
/*                                                                           */
/*  Read a preferred bound, using dft_below_hard, dft_below_weight,          */
/*  dft_above_hard and dft_above_weight if there is no weight.               */
/*                                                                           */
/*****************************************************************************/

bool CoiPreferredBoundRead(KML_ELT lim_elt, bool dft_below_hard,
  int dft_below_weight, bool dft_above_hard, int dft_above_weight,
  COI_BOUND *res, char **err_mess, HA_ARENA a, NRC_INSTANCE ins)
{
  char *var, *label;  int limit;  NRC_PENALTY penalty_below, penalty_above;
  NRC_BOUND b;
  if( dft_below_weight == -1 )
    return *err_mess = "no default weight for below in <Preferred>", false;
  else if( dft_above_weight == -1 )
    return *err_mess = "no default weight for above in <Preferred>", false;
  else if( !ReadLim(lim_elt, dft_below_hard, dft_below_weight, &var, &label,
      &limit, &penalty_below, err_mess, a, ins) )
    return false;
  else if( !ReadLim(lim_elt, dft_above_hard, dft_above_weight, &var, &label,
      &limit, &penalty_above, err_mess, a, ins) )
    return false;
  else
  {
    b = NrcBoundMakePreferred(limit, penalty_below, penalty_above, ins);
    return *res = CoiBoundMake(label, var, b, a), true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool CoiMaxBoundRead(KML_ELT lim_elt, bool dft_above_hard,               */
/*    int dft_above_weight, COI_BOUND *res, char **err_mess)                 */
/*                                                                           */
/*  Read a maximum bound, using dft_above_hard and dft_above_weight          */
/*  if there is no weight.                                                   */
/*                                                                           */
/*****************************************************************************/

bool CoiMaxBoundRead(KML_ELT lim_elt, bool dft_above_hard,
  int dft_above_weight, COI_BOUND *res, char **err_mess,
  HA_ARENA a, NRC_INSTANCE ins)
{
  char *var, *label;  int limit;  NRC_PENALTY penalty;  NRC_BOUND b;
  if( !ReadLim(lim_elt, dft_above_hard, dft_above_weight, &var, &label, &limit,
	&penalty, err_mess, a, ins) )
    return false;
  else
  {
    b = NrcBoundMakeMax(limit, penalty, ins);
    return *res = CoiBoundMake(label, var, b, a), true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *CoiBoundLabel(COI_BOUND b)                                         */
/*                                                                           */
/*  Return the label attribute of b.                                         */
/*                                                                           */
/*****************************************************************************/

char *CoiBoundLabel(COI_BOUND b)
{
  return b->label;
}


/*****************************************************************************/
/*                                                                           */
/*  char *CoiBoundVar(COI_BOUND b)                                           */
/*                                                                           */
/*  Return the var attribute of b.                                           */
/*                                                                           */
/*****************************************************************************/

char *CoiBoundVar(COI_BOUND b)
{
  return b->var;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_BOUND CoiBoundBound(COI_BOUND b)                                     */
/*                                                                           */
/*  Return the bound attribute of b.                                         */
/*                                                                           */
/*****************************************************************************/

NRC_BOUND CoiBoundBound(COI_BOUND b)
{
  return b->bound;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY CoiMinLimitPenaltyBelow(COI_LIMIT lim)                       */
/*                                                                           */
/*  Return the penalty_below attribute of lim, which must be a min limit.    */
/*                                                                           */
/*****************************************************************************/

/* ***
NRC_PENALTY CoiMinLimitPenaltyBelow(COI_LIMIT lim)
{
  COI_MIN_LIMIT min_limit;
  MAssert(lim->type == COI_LIMIT_TYPE_MIN,
    "CoiMinLimitPenaltyBelow: lim is not a minimum limit");
  min_limit = (COI_MIN_LIMIT) lim;
  return min_limit->below_penalty;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY CoiMaxLimitPenaltyAbove(COI_LIMIT lim)                       */
/*                                                                           */
/*  Return the penalty_above attribute of lim, which must be a max limit.    */
/*                                                                           */
/*****************************************************************************/

/* ***
NRC_PENALTY CoiMaxLimitPenaltyAbove(COI_LIMIT lim)
{
  COI_MAX_LIMIT max_limit;
  MAssert(lim->type == COI_LIMIT_TYPE_MAX,
    "CoiMinLimitPenaltyAbove: lim is not a maximum limit");
  max_limit = (COI_MAX_LIMIT) lim;
  return max_limit->above_penalty;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void CoiLimitAddToDemandConstraint(COI_LIMIT lim,                        */
/*    NRC_DEMAND_CONSTRAINT dc)                                              */
/*                                                                           */
/*  Add lim to dc.                                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
bool CoiLimitAddToDemandConstraint(COI_LIMIT lim, NRC_DEMAND_CONSTRAINT dc)
{
  COI_MIN_LIMIT min_lim;  COI_PREFERRED_LIMIT pref_lim;  COI_MAX_LIMIT max_lim;
  NRC_BOUND b;
  b = NrcDemandConstraintBound(dc);
  switch( lim->type )
  {
    case COI_LIMIT_TYPE_MIN:

      min_lim = (COI_MIN_LIMIT) lim;
      return NrcBoundAddMin(b, min_lim->limit, false, min_lim->below_penalty);
      ** ***
      return NrcDemandConstraintAddMin(dc, min_lim->limit,
	min_lim->below_penalty);
      *** **

    case COI_LIMIT_TYPE_PREFERRED:

      pref_lim = (COI_PREFERRED_LIMIT) lim;
      return NrcBoundAddPreferred(b, pref_lim->limit,
	pref_lim->below_penalty, pref_lim->above_penalty);
      ** ***
      return NrcDemandConstraintAddPreferred(dc, pref_lim->limit,
	pref_lim->below_penalty, pref_lim->above_penalty);
      *** **

    case COI_LIMIT_TYPE_MAX:

      max_lim = (COI_MAX_LIMIT) lim;
      return NrcBoundAddMax(b, max_lim->limit, max_lim->above_penalty);
      ** ***
      return NrcDemandConstraintAddMax(dc, max_lim->limit,
	max_lim->above_penalty);
      *** **

    default:

      MAbort("CoiLimitAddToDemandConstraint internal error (type %d)",
	lim->type);
      return false;  ** keep compiler happy **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void CoiBoundWorkloadLimitConstraintMake(COI_BOUND b, NRC_INSTANCE ins,  */
/*    NRC_WORKER_SET ws, NRC_SHIFT_SET ss, NRC_CONSTRAINT *c)                */
/*                                                                           */
/*  Make a workload limit constraint out of b.                               */
/*                                                                           */
/*****************************************************************************/

/* *** should be implementable outside coi_bound.c noew
void CoiBoundWorkloadLimitConstraintMake(COI_BOUND b, NRC_INSTANCE ins,
  NRC_WORKER_SET ws, NRC_SHIFT_SET ss, NRC_CONSTRAINT *c)
{
  COI_MIN_LIMIT min_lim;  COI_MAX_LIMIT max_lim;  char *name;
  MAbort("CoiBoundWorkloadLimitConstraintMake still to do");
  switch( lim->type )
  {
    case COI_LIMIT_TYPE_MIN:

      min_lim = (COI_MIN_LIMIT) lim;
      name = "Workload limit";
      *c = NrcConstraintMake(ins, name, ws, NRC_CONSTRAINT_WORKLOAD,
	NrcBoundMakeMin(min_lim->limit, false, min_lim->below_penalty), NULL);
      NrcConstraintAddShiftSet(*c, ss, NRC_POSITIVE);
      ** ***
      name = HnStringMake("At least %d workload", min_lim->limit);
      *c = NrcConstraintMake(ins, name, ws, min_lim->below_penalty,
	NrcLimit(NRC_LIMIT_MIN_WORKLOAD, min_lim->limit), NULL);
      NrcConstraintAddShiftSet(*c, ss, NRC_POSITIVE);
      *** **
      break;

    case COI_LIMIT_TYPE_PREFERRED:

      MAbort("CoiLimitWorkloadLimitConstraintMake: preferred not implemented");
      break;

    case COI_LIMIT_TYPE_MAX:

      max_lim = (COI_MAX_LIMIT) lim;
      name = "Workload limit";
      *c = NrcConstraintMake(ins, name, ws, NRC_CONSTRAINT_WORKLOAD,
	NrcBoundMakeMin(max_lim->limit, false, max_lim->above_penalty), NULL);
      NrcConstraintAddShiftSet(*c, ss, NRC_POSITIVE);
      ** ***
      name = HnStringMake("At most %d workload", max_lim->limit);
      *c = NrcConstraintMake(ins, name, ws, max_lim->above_penalty,
	NrcLimit(NRC_LIMIT_MAX_WORKLOAD, max_lim->limit), NULL);
      NrcConstraintAddShiftSet(*c, ss, NRC_POSITIVE);
      *** **
      break;

    default:

      MAbort("CoiLimitWorkloadLimitConstraintMake internal error (type %d)",
	lim->type);
      break;
  }
}
*** */

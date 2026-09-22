
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
/*  FILE:         nrc_condensed.c                                            */
/*  MODULE:       Condensed worker constraints                               */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG_FAILURES 0
#define DEBUG_SUCCESSES 0

/*****************************************************************************/
/*                                                                           */
/*  NRC_CONDENSED_CONSTRAINT                                                 */
/*                                                                           */
/*  A condensed constraint.  This contains all the information in the        */
/*  original constraint, only in a condensed form that anticipates its       */
/*  conversion from time window style to consecutive style.                  */
/*                                                                           */
/*****************************************************************************/

struct nrc_condensed_constraint_rec {
  NRC_INSTANCE		instance;
  char			*name;
  NRC_WORKER_SET	worker_set;
  NRC_BOUND		bound;
  NRC_SHIFT_SET		first_ss;
  int			length;
  int			offset;
  NRC_POLARITY		polarity;
  bool			limits_initial;
  bool			limits_max;
  NRC_CONSTRAINT	orig_constraint;
};

typedef HA_ARRAY(NRC_CONDENSED_CONSTRAINT) ARRAY_NRC_CONDENSED_CONSTRAINT;


/*****************************************************************************/
/*                                                                           */
/*  NRC_CONDENSER_PAIR                                                       */
/*                                                                           */
/*  A pair of condensed constraints, one for the initial region and one      */
/*  for the general region.  The initial one is optional.                    */
/*                                                                           */
/*****************************************************************************/

typedef struct nrc_condenser_pair_rec {
  int				penalized_len;
  NRC_PENALTY			penalty;
  int				remaining_weight;
  NRC_COST_FUNCTION		cost_fn;
  NRC_CONDENSED_CONSTRAINT	init_cc;	/* optional, may be NULL */
  NRC_CONDENSED_CONSTRAINT	full_cc;
} *NRC_CONDENSER_PAIR;

typedef HA_ARRAY(NRC_CONDENSER_PAIR) ARRAY_NRC_CONDENSER_PAIR;


/*****************************************************************************/
/*                                                                           */
/*  NRC_CONDENSER_BAG                                                        */
/*                                                                           */
/*  A bag of compatible condensed constraints, where compatibility is        */
/*  defined by NrcCondensedConstraintCompatible below.                       */
/*                                                                           */
/*****************************************************************************/

typedef struct nrc_condenser_bag_rec {
  ARRAY_NRC_CONDENSED_CONSTRAINT	constraints;
  ARRAY_NRC_CONDENSER_PAIR		pairs;
} *NRC_CONDENSER_BAG;

typedef HA_ARRAY(NRC_CONDENSER_BAG) ARRAY_NRC_CONDENSER_BAG;


/*****************************************************************************/
/*                                                                           */
/*  NRC_CONSTRAINT_CONDENSER                                                 */
/*                                                                           */
/*****************************************************************************/

struct nrc_constraint_condenser_rec {
  HA_ARENA				arena;
  NRC_INSTANCE				instance;
  ARRAY_NRC_CONDENSED_CONSTRAINT	constraints;
  ARRAY_NRC_CONDENSER_BAG		bags;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "constraint pairs"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_CONDENSER_PAIR NrcCondenserPairMake(NRC_CONDENSED_CONSTRAINT init_cc,*/
/*    NRC_CONDENSED_CONSTRAINT full_cc)                                      */
/*                                                                           */
/*  Make a condenser pair with these attributes.                             */
/*                                                                           */
/*****************************************************************************/

static NRC_CONDENSER_PAIR NrcCondenserPairMake(NRC_CONDENSED_CONSTRAINT init_cc,
  NRC_CONDENSED_CONSTRAINT full_cc, HA_ARENA a)
{
  NRC_CONDENSER_PAIR res;
  HnAssert(full_cc != NULL, "NrcCondenserPairMake internal error");
  HaMake(res, a);
  NrcBoundMax(full_cc->bound, &res->penalized_len, &res->penalty);
  res->penalized_len--;
  res->remaining_weight = NrcPenaltyWeight(res->penalty);
  res->cost_fn = NRC_COST_FUNCTION_LINEAR;
  res->init_cc = init_cc;
  res->full_cc = full_cc;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcCondenserPairDebug(NRC_CONDENSER_PAIR pair, int indent, FILE *fp)*/
/*                                                                           */
/*  Debug print of pair onto fp.                                             */
/*                                                                           */
/*****************************************************************************/
void NrcCondensedConstraintDebug(NRC_CONDENSED_CONSTRAINT cc,
  int indent, FILE *fp);

static void NrcCondenserPairDebug(NRC_CONDENSER_PAIR pair, int indent, FILE *fp)
{
  fprintf(fp,
    "%*s[ CondenserPair (penalized_len %d, penalty %s, remaining %d:%s)\n",
    indent, "", pair->penalized_len, NrcPenaltyShow(pair->penalty),
    pair->remaining_weight, NrcCostFnShow(pair->cost_fn));
  NrcCondensedConstraintDebug(pair->init_cc, indent + 2, fp);
  NrcCondensedConstraintDebug(pair->full_cc, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcCondenserPairDecreasingExactLengthTypedCmp(                       */
/*    NRC_CONDENSER_PAIR pair1, NRC_CONDENSER_PAIR pair2)                    */
/*                                                                           */
/*  Comparison functions for sorting an array of pairs into increasing       */
/*  exact length order.                                                      */
/*                                                                           */
/*****************************************************************************/

static int NrcCondenserPairDecreasingExactLengthTypedCmp(
  NRC_CONDENSER_PAIR pair1, NRC_CONDENSER_PAIR pair2)
{
  return pair2->penalized_len - pair1->penalized_len;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcCondenserPairDecreasingExactLengthCmp(const void *t1,             */
/*    const void *t2)                                                        */
/*                                                                           */
/*  Like NrcCondenserPairDecreasingExactLengthTypedCmp only untyped.         */
/*                                                                           */
/*****************************************************************************/

static int NrcCondenserPairDecreasingExactLengthCmp(const void *t1,
  const void *t2)
{
  NRC_CONDENSER_PAIR pair1 = * (NRC_CONDENSER_PAIR *) t1;
  NRC_CONDENSER_PAIR pair2 = * (NRC_CONDENSER_PAIR *) t2;
  return NrcCondenserPairDecreasingExactLengthTypedCmp(pair1, pair2);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "constraint regularity"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_CONDENSED_CONSTRAINT NrcCondensedConstraintMake(NRC_INSTANCE ins,    */
/*    char *name, NRC_WORKER_SET worker_set, NRC_BOUND bound,                */
/*    NRC_SHIFT_SET first_ss, int length, int offset, NRC_POLARITY po,       */
/*    bool limits_initial, bool limits_max, NRC_CONSTRAINT orig_constraint)  */
/*                                                                           */
/*  Make a condensed constraint with these attributes.                       */
/*                                                                           */
/*****************************************************************************/

NRC_CONDENSED_CONSTRAINT NrcCondensedConstraintMake(NRC_INSTANCE ins,
  char *name, NRC_WORKER_SET worker_set, NRC_BOUND bound,
  NRC_SHIFT_SET first_ss, int length, int offset, NRC_POLARITY po,
  bool limits_initial, bool limits_max, NRC_CONSTRAINT orig_constraint)
{
  NRC_CONDENSED_CONSTRAINT res;
  HaMake(res, NrcInstanceArena(ins));
  res->instance = ins;
  res->name = name;
  res->worker_set = worker_set;
  res->bound = bound;
  res->first_ss = first_ss;
  res->length = length;
  res->offset = offset;
  res->polarity = po;
  res->limits_initial = limits_initial;
  res->limits_max = limits_max;
  res->orig_constraint = orig_constraint;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcCondensedConstraintCompatible(NRC_CONDENSED_CONSTRAINT cc1,      */
/*    NRC_CONDENSED_CONSTRAINT cc2)                                          */
/*                                                                           */
/*  Return true if cc1 and cc2 are compatible, that is, if they could        */
/*  possibly be merged.                                                      */
/*                                                                           */
/*  NB limits_initial does not affect this function.                         */
/*                                                                           */
/*****************************************************************************/

static bool NrcCondensedConstraintCompatible(NRC_CONDENSED_CONSTRAINT cc1,
  NRC_CONDENSED_CONSTRAINT cc2)
{
  int junk;  NRC_PENALTY p1, p2;
  NrcBoundMax(cc1->bound, &junk, &p1);
  NrcBoundMax(cc2->bound, &junk, &p2);
  return NrcPenaltyHard(p1) == NrcPenaltyHard(p2) &&
    NrcWorkerSetEqual(cc1->worker_set, cc2->worker_set) &&
    cc1->polarity == cc2->polarity &&
    cc1->limits_max == cc2->limits_max &&
    cc1->offset == cc2->offset &&
    NrcShiftSetsEqual(cc1->first_ss, cc2->first_ss);
    /* NrcShiftSetsUniform(cc1->first_ss, cc2->first_ss, &junk); */
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcCondensedConstraintMakePair(NRC_CONDENSED_CONSTRAINT cc1,        */
/*    NRC_CONDENSED_CONSTRAINT cc2, NRC_CONSTRAINT_CONDENSER cc,             */
/*    NRC_CONDENSER_PAIR *res)                                               */
/*                                                                           */
/*  Assuming that cc1 and cc2 are compatible (as defined just above),        */
/*  return true if they make an init-full pair, that is, one is init and     */
/*  the other is full, the full one has one more time group than the init    */
/*  one, and their cost functions are equal.                                 */
/*                                                                           */
/*****************************************************************************/

static bool NrcCondensedConstraintMakePair(NRC_CONDENSED_CONSTRAINT cc1,
  NRC_CONDENSED_CONSTRAINT cc2, NRC_CONSTRAINT_CONDENSER cc,
  NRC_CONDENSER_PAIR *res)
{
  int max_value1, max_value2;  NRC_PENALTY p1, p2;
  NrcBoundMax(cc1->bound, &max_value1, &p1);
  NrcBoundMax(cc2->bound, &max_value2, &p2);
  *res = NULL;
  if( max_value1 == max_value2 - 1 )
  {
    /* cc1 must be the init and cc2 the full */
    if( cc1->limits_initial && !cc2->limits_initial && NrcPenaltyEqual(p1, p2) )
      *res = NrcCondenserPairMake(cc1, cc2, cc->arena);
  }
  else if( max_value2 == max_value1 - 1 )
  {
    /* cc2 must be the init and cc1 the full */
    if( cc2->limits_initial && !cc1->limits_initial && NrcPenaltyEqual(p1, p2) )
      *res = NrcCondenserPairMake(cc2, cc1, cc->arena);
  }
  else
  {
    /* doesn't work */
  }
  return *res != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcCondensedConstraintMakeSingle(NRC_CONDENSED_CONSTRAINT cc1,      */
/*    NRC_CONSTRAINT_CONDENSER cc, NRC_CONDENSER_PAIR *res)                  */
/*                                                                           */
/*  Return true if cc1 is full, and store it in *res.                        */
/*                                                                           */
/*****************************************************************************/

static bool NrcCondensedConstraintMakeSingle(NRC_CONDENSED_CONSTRAINT cc1,
  NRC_CONSTRAINT_CONDENSER cc, NRC_CONDENSER_PAIR *res)
{
  if( !cc1->limits_initial )
  {
    /* cc1 is full */
    *res = NrcCondenserPairMake(NULL, cc1, cc->arena);
  }
  else
  {
    /* doesn't work */
    *res = NULL;
  }
  return *res != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcCondensedConstraintDebug(NRC_CONDENSED_CONSTRAINT cc,            */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of cc onto fp with the given indent.                         */
/*                                                                           */
/*****************************************************************************/

void NrcCondensedConstraintDebug(NRC_CONDENSED_CONSTRAINT cc,
  int indent, FILE *fp)
{
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  if( cc == NULL )
    fprintf(fp, "%s%s", "NULL constraint", indent >= 0 ? "\n" : "");
  else
  {
    fprintf(fp, "%s%s%d:%d|%s|%s|%s", NrcWorkerSetName(cc->worker_set),
      NrcBoundShow(cc->bound), cc->length, cc->offset,
      cc->polarity == NRC_POSITIVE ? "+ve" : "-ve",
      cc->limits_initial ? "init" : "full", cc->limits_max ? "max" : "exact");
    NrcShiftSetDebug(cc->first_ss, indent >= 0 ? 0 : -1, fp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "condenser bags"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_CONDENSER_BAG NrcCondenserBagMake(NRC_CONDENSED_CONSTRAINT cc,       */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Make a new condenser bag containing just cc.                             */
/*                                                                           */
/*****************************************************************************/

static NRC_CONDENSER_BAG NrcCondenserBagMake(NRC_CONDENSED_CONSTRAINT cc,
  HA_ARENA a)
{
  NRC_CONDENSER_BAG res;
  HaMake(res, a);
  HaArrayInit(res->constraints, a);
  HaArrayAddLast(res->constraints, cc);
  HaArrayInit(res->pairs, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcCondenserBagBuildPairs(NRC_CONDENSER_BAG cb,                      */
/*    NRC_CONSTRAINT_CONDENSER cc)                                           */
/*                                                                           */
/*  Build pairs of related constraints in cb.  Return the number built.      */
/*                                                                           */
/*****************************************************************************/

static int NrcCondenserBagBuildPairs(NRC_CONDENSER_BAG cb,
  NRC_CONSTRAINT_CONDENSER cc)
{
  int i, j, res;  NRC_CONDENSED_CONSTRAINT cci, ccj;  NRC_CONDENSER_PAIR pair;
  res = 0;
  for( i = 0;  i < HaArrayCount(cb->constraints);  i++ )
  {
    cci = HaArray(cb->constraints, i);
    for( j = i + 1;  j < HaArrayCount(cb->constraints);  j++ )
    {
      ccj = HaArray(cb->constraints, j);
      if( NrcCondensedConstraintMakePair(cci, ccj, cc, &pair) )
      {
	HaArrayAddLast(cb->pairs, pair);
	res++;
	HaArrayDeleteAndPlug(cb->constraints, j);  /* NB order is important */
	HaArrayDeleteAndPlug(cb->constraints, i);  /* NB order is important */
	i--;
	break;
      }
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcCondenserBagBuildSingles(NRC_CONDENSER_BAG cb,                    */
/*    NRC_CONSTRAINT_CONDENSER cc)                                           */
/*                                                                           */
/*  Build singles of non-initial constraints in cb.  Return the number built.*/
/*                                                                           */
/*****************************************************************************/

static int NrcCondenserBagBuildSingles(NRC_CONDENSER_BAG cb,
  NRC_CONSTRAINT_CONDENSER cc)
{
  int i, res;  NRC_CONDENSED_CONSTRAINT cci;  NRC_CONDENSER_PAIR pair;
  res = 0;
  for( i = 0;  i < HaArrayCount(cb->constraints);  i++ )
  {
    cci = HaArray(cb->constraints, i);
    if( NrcCondensedConstraintMakeSingle(cci, cc, &pair) )
    {
      HaArrayAddLast(cb->pairs, pair);
      res++;
      HaArrayDeleteAndPlug(cb->constraints, i);
      i--;
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcCondenserBagDebug(NRC_CONDENSER_BAG cb, int indent, FILE *fp)    */
/*                                                                           */
/*  Debug print of cb onto fp with the given indent.                         */
/*                                                                           */
/*****************************************************************************/

void NrcCondenserBagDebug(NRC_CONDENSER_BAG cb, int indent, FILE *fp)
{
  NRC_CONDENSED_CONSTRAINT cc;  int i;  NRC_CONDENSER_PAIR pair;
  fprintf(fp, "%*s[ CondenserBag\n", indent, "");
  HaArrayForEach(cb->constraints, cc, i)
    NrcCondensedConstraintDebug(cc, indent + 2, fp);
  HaArrayForEach(cb->pairs, pair, i)
    NrcCondenserPairDebug(pair, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "constraint merger"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_CONSTRAINT_CONDENSER NrcConstraintCondenserBegin(NRC_INSTANCE ins)   */
/*                                                                           */
/*  Make and return a constraint merger object.                              */
/*                                                                           */
/*****************************************************************************/

NRC_CONSTRAINT_CONDENSER NrcConstraintCondenserBegin(NRC_INSTANCE ins)
{
  NRC_CONSTRAINT_CONDENSER res;  HA_ARENA a;
  a = NrcInstanceArenaBegin(ins);
  HaMake(res, a);
  res->arena = a;
  res->instance = ins;
  HaArrayInit(res->constraints, a);
  HaArrayInit(res->bags, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintCondenserEnd(NRC_CONSTRAINT_CONDENSER cc)              */
/*                                                                           */
/*  End the use of cc; reclaim its memory.                                   */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintCondenserEnd(NRC_CONSTRAINT_CONDENSER cc)
{
  NrcInstanceArenaEnd(cc->instance, cc->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintCondenserAddCondensedConstraint(                       */
/*    NRC_CONSTRAINT_CONDENSER cm, NRC_CONDENSED_CONSTRAINT cc)              */
/*                                                                           */
/*  Add cc to cm.                                                            */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintCondenserAddCondensedConstraint(
  NRC_CONSTRAINT_CONDENSER cm, NRC_CONDENSED_CONSTRAINT cc)
{
  NRC_CONDENSER_BAG cb;  int i;

  /* boilerplate, and add to constraints list */
  HnAssert(cc != NULL,
    "NrcConstraintCondenserAddCondensedConstraint internal error");
  if( DEBUG1 )
  {
    fprintf(stderr,"[ NrcConstraintCondenserAddCondensedConstraint(cm, cc):\n");
    NrcCondensedConstraintDebug(cc, 2, stderr);
  }
  HaArrayAddLast(cm->constraints, cc);

  /* add to a suitable bag, or if none, make a new bag and add it to that */
  HaArrayForEach(cm->bags, cb, i)
    if( NrcCondensedConstraintCompatible(HaArrayFirst(cb->constraints), cc) )
    {
      HaArrayAddLast(cb->constraints, cc);
      break;
    }
  if( i >= HaArrayCount(cm->bags) )
  {
    cb = NrcCondenserBagMake(cc, cm->arena);
    HaArrayAddLast(cm->bags, cb);
  }
  if( DEBUG1 )
   fprintf(stderr,"] NrcConstraintCondenserAddCondensedConstraint returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcCondensedConstraintMaxMakeConsec(NRC_CONDENSED_CONSTRAINT c)     */
/*                                                                           */
/*  Convert c, which places a limit on the maximum number of active time     */
/*  groups, to one which places limits on consecutive time groups.           */
/*                                                                           */
/*****************************************************************************/

static void NrcCondensedConstraintMaxMakeConsec(NRC_CONDENSED_CONSTRAINT c)
{
  NRC_CONSTRAINT res;  int i;
  char *name;  NRC_SHIFT_SET ss, shifted_ss;

  if( DEBUG2 )
  {
    fprintf(stderr, "[ NrcCondensedConstraintMaxMakeConsec(c), c =\n");
    NrcCondensedConstraintDebug(c, 2, stderr);
  }

  /* make and add a new consecutive constraint to replace c */
  name = HnStringMake(NrcInstanceArena(c->instance), "%s (condensed)",
    c->name);
  res = NrcConstraintMake(c->instance, name, c->worker_set,
    NRC_CONSTRAINT_CONSECUTIVE, c->bound, NULL);

  /* add shift sets */
  ss = c->first_ss;
  NrcConstraintAddShiftSet(res, ss, c->polarity);
  for( i = 1;  i < c->length;  i++ )
  {
    if( NrcShiftSetMakeShifted(ss, c->offset, &shifted_ss) )
      NrcConstraintAddShiftSet(res, shifted_ss, c->polarity);
    ss = shifted_ss;
  }

  if( DEBUG2 )
  {
    fprintf(stderr, "  old constraint:\n");
    NrcConstraintDebug(c->orig_constraint, 2, stderr);
    fprintf(stderr, "  new constraint:\n");
    NrcConstraintDebug(res, 2, stderr);
  }

  /* kill the original constraint by emptying its worker set */
  NrcConstraintKill(c->orig_constraint);

  if( DEBUG2 )
    fprintf(stderr, "] NrcCondensedConstraintMaxMakeConsec returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcCondenserBagExactLengthsWork(NRC_CONDENSER_BAG cb)               */
/*                                                                           */
/*  Return true if the exact lengths of cb's pairs work.  They need          */
/*  to count downwards to 1.                                                 */
/*                                                                           */
/*****************************************************************************/

static bool NrcCondenserBagExactLengthsWork(NRC_CONDENSER_BAG cb)
{
  int wanted, i;  NRC_CONDENSER_PAIR pair;
  wanted = 1;
  HaArrayForEachReverse(cb->pairs, pair, i)
  {
    if( pair->penalized_len != wanted )
      return false;
    wanted++;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcCondenserBagWeightsWork(NRC_CONDENSER_BAG cb, int start_index)   */
/*                                                                           */
/*  Return true if cb's pairs' weights are non-negative and non-decreasing   */
/*  from start_index (which must exist) onwards.                             */
/*                                                                           */
/*****************************************************************************/

static bool NrcCondenserBagWeightsWork(NRC_CONDENSER_BAG cb, int start_index)
{
  NRC_CONDENSER_PAIR pair;  int prev_weight, weight, i;
  pair = HaArray(cb->pairs, start_index);
  prev_weight = pair->remaining_weight;
  if( prev_weight < 0 )
    return false;
  for( i = start_index + 1;  i < HaArrayCount(cb->pairs);  i++ )
  {
    pair = HaArray(cb->pairs, i);
    weight = pair->remaining_weight;
    if( weight < prev_weight )
      return false;
    prev_weight = weight;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcCondenserBagAllPairsHaveInit(NRC_CONDENSER_BAG cb)               */
/*                                                                           */
/*  Return true if all the pairs of cb have an init constraint.              */
/*                                                                           */
/*****************************************************************************/

static bool NrcCondenserBagAllPairsHaveInit(NRC_CONDENSER_BAG cb)
{
  NRC_CONDENSER_PAIR pair;  int i;
  HaArrayForEach(cb->pairs, pair, i)
    if( pair->init_cc == NULL )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcCondenserBagNoPairsHaveInit(NRC_CONDENSER_BAG cb)                */
/*                                                                           */
/*  Return true if none of the pairs of cb have an init constraint.          */
/*                                                                           */
/*****************************************************************************/

static bool NrcCondenserBagNoPairsHaveInit(NRC_CONDENSER_BAG cb)
{
  NRC_CONDENSER_PAIR pair;  int i;
  HaArrayForEach(cb->pairs, pair, i)
    if( pair->init_cc != NULL )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcCondenserBagInitsWork(NRC_CONDENSER_BAG cb)                      */
/*                                                                           */
/*  Return true if all the pairs of cb either have an initial constraint,    */
/*  or all do not.                                                           */
/*                                                                           */
/*****************************************************************************/

static bool NrcCondenserBagInitsWork(NRC_CONDENSER_BAG cb)
{
  return NrcCondenserBagAllPairsHaveInit(cb) ||
    NrcCondenserBagNoPairsHaveInit(cb);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcCondenserBagTryFn(NRC_CONDENSER_BAG cb,                          */
/*    int start_index, NRC_COST_FUNCTION cost_fn, int *weight)               */
/*  void NrcCondenserBagUnTryFn(NRC_CONDENSER_BAG cb,                        */
/*    int start_index, NRC_COST_FUNCTION cost_fn, int weight)                */
/*                                                                           */
/*  Try whether cost_fn would work, starting from start_index; and undo it.  */
/*                                                                           */
/*****************************************************************************/

static int fn(int weight, NRC_COST_FUNCTION cost_fn, int j)
{
  switch( cost_fn )
  {
    case NRC_COST_FUNCTION_STEP:	return weight;
    case NRC_COST_FUNCTION_LINEAR:	return weight * j;
    case NRC_COST_FUNCTION_QUADRATIC:	return weight * j * j;

    default:

      HnAbort("NrcCondenserBagTryFn internal error");
      return 0;  /* keep compiler happy */
  }
}

static void NrcCondenserBagTryFn(NRC_CONDENSER_BAG cb,
  int start_index, NRC_COST_FUNCTION cost_fn, int *weight)
{
  NRC_CONDENSER_PAIR pair;  int i, j;
  pair = HaArray(cb->pairs, start_index);
  *weight = pair->remaining_weight;
  for( i = start_index, j = 1;  i < HaArrayCount(cb->pairs);  i++, j++ )
  {
    pair = HaArray(cb->pairs, i);
    pair->remaining_weight -= fn(*weight, cost_fn, j);
  }
}

static void NrcCondenserBagUnTryFn(NRC_CONDENSER_BAG cb,
  int start_index, NRC_COST_FUNCTION cost_fn, int weight)
{
  NRC_CONDENSER_PAIR pair;  int i, j;
  for( i = start_index, j = 1;  i < HaArrayCount(cb->pairs);  i++, j++ )
  {
    pair = HaArray(cb->pairs, i);
    pair->remaining_weight += fn(weight, cost_fn, j);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcFindCostFnAndWeight(NRC_CONDENSER_BAG cb, int start_index,       */
/*    NRC_COST_FUNCTION *cost_fn, int *weight)                               */
/*                                                                           */
/*  Find a suitable cost function and weight for covering the cb pair        */
/*  at position start_index.                                                 */
/*                                                                           */
/*****************************************************************************/
#define COST_FN_COUNT 3

static void NrcFindCostFnAndWeight(NRC_CONDENSER_BAG cb, int start_index,
  NRC_COST_FUNCTION *cost_fn, int *weight)
{
  static NRC_COST_FUNCTION fns[COST_FN_COUNT] = {NRC_COST_FUNCTION_QUADRATIC,
    NRC_COST_FUNCTION_LINEAR, NRC_COST_FUNCTION_STEP};
  int init_j, j;
  init_j = (start_index == HaArrayCount(cb->pairs) - 1 ? 1 : 0);
  for( j = init_j;  j < COST_FN_COUNT;  j++ )
  {
    *cost_fn = fns[j];
    NrcCondenserBagTryFn(cb, start_index, *cost_fn, weight);
    if( NrcCondenserBagWeightsWork(cb, start_index) )
      return;
    NrcCondenserBagUnTryFn(cb, start_index, *cost_fn, *weight);
  }
  HnAbort("NrcFindCostFnAndWeight internal error");
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcCondenserBagExactCondense(NRC_CONDENSER_BAG cb,                  */
/*    NRC_INSTANCE ins, HA_ARENA a)                                          */
/*                                                                           */
/*  Condense cb.                                                             */
/*                                                                           */
/*****************************************************************************/

static void NrcCondenserBagExactCondense(NRC_CONDENSER_BAG cb,
  NRC_CONSTRAINT_CONDENSER cc)
{
  bool lengths_work, weights_work, inits_work;  NRC_WORKER w;
  int i, j, weight, pairs, singles;  NRC_BOUND b;
  NRC_CONDENSER_PAIR pair;  NRC_PENALTY p;  NRC_COST_FUNCTION cost_fn;
  NRC_CONDENSED_CONSTRAINT c;  char *name;  NRC_SHIFT_SET ss, shifted_ss;
  NRC_CONSTRAINT res;
  if( DEBUG2 )
    fprintf(stderr, "[ NrcCondenserBagExactCondense(cb, %s)\n",
      NrcInstanceId(cc->instance));
  pairs = NrcCondenserBagBuildPairs(cb, cc);
  if( DEBUG2 )
  {
    fprintf(stderr, "  bag with pairs only (pairs %d):\n", pairs);
    NrcCondenserBagDebug(cb, 2, stderr);
  }
  singles = NrcCondenserBagBuildSingles(cb, cc);
  if( DEBUG2 )
  {
    fprintf(stderr, "  bag with pairs and singles (pairs %d, singles %d):\n",
      pairs, singles);
    NrcCondenserBagDebug(cb, 2, stderr);
  }
  if( pairs + singles > 0 && (pairs == 0 || singles == 0) )
  {
    HaArraySort(cb->pairs, &NrcCondenserPairDecreasingExactLengthCmp);
    lengths_work = NrcCondenserBagExactLengthsWork(cb);
    weights_work = NrcCondenserBagWeightsWork(cb, 0);
    inits_work = NrcCondenserBagInitsWork(cb);
    if( DEBUG2 )
    {
      fprintf(stderr, "  lengths_work %s, weights_work %s, inits_work %s:\n",
	lengths_work ? "true" : "false", weights_work ? "true" : "false",
	inits_work ? "true" : "false");
      NrcCondenserBagDebug(cb, 2, stderr);
    }

    if( lengths_work && weights_work && inits_work )
    {
      /* the paired constraints in cb can be and now will be condensed */
      if( DEBUG_SUCCESSES )
      {
	fprintf(stderr, "[ NrcCondenserBagExactCondense(%s) condensing bag:\n",
	  NrcInstanceId(cc->instance));
	NrcCondenserBagDebug(cb, 2, stderr);
      }
      HaArrayForEach(cb->pairs, pair, i)
      {
	/* if pair has any remaining weight, generate a constraint for it */
	if( pair->remaining_weight > 0 )
	{
	  /* find cost_fn and weight and make a suitable bound */
          NrcFindCostFnAndWeight(cb, i, &cost_fn, &weight);
	  p = NrcPenalty(NrcPenaltyHard(pair->penalty), weight,
	    cost_fn, cc->instance);
	  b = NrcBoundMakeMin(pair->penalized_len + 1, false,
	    p, cc->instance);
	  if( DEBUG2 )
	    fprintf(stderr, "  building constraint with bound %s\n",
	      NrcBoundShow(b));

	  /* make and add a new consecutive constraint to replace pair */
	  c = pair->full_cc;
	  name = HnStringMake(NrcInstanceArena(c->instance),
	    "%s (condensed)", c->name);
	  res = NrcConstraintMake(c->instance, name, c->worker_set,
	    NRC_CONSTRAINT_CONSECUTIVE, b, NULL);

	  /* add shift sets */
	  ss = c->first_ss;
	  NrcConstraintAddShiftSet(res, ss, c->polarity);
	  for( j = 1;  j < c->length &&
	      NrcShiftSetMakeShifted(ss, c->offset, &shifted_ss);  j++ )
	  {
	    NrcConstraintAddShiftSet(res, shifted_ss, c->polarity);
	    ss = shifted_ss;
	  }

	  /* add history */
	  if( pair->init_cc == NULL )
	  {
	    /* history before and after */
	    NrcConstraintAddHistory(res, pair->penalized_len + 1,
	      pair->penalized_len + 1);
	    for( j = 0;  j < NrcWorkerSetWorkerCount(c->worker_set);  j++ )
	    {
	      w = NrcWorkerSetWorker(c->worker_set, j);
	      NrcConstraintAddHistoryWorker(res, w, pair->penalized_len + 1);
	    }
	  }
	  else
	  {
	    /* history after only */
	    NrcConstraintAddHistory(res, 0, pair->penalized_len + 1);
	  }

	  if( DEBUG_SUCCESSES )
	  {
	    fprintf(stderr, "  adding condensed constraint:\n");
	    NrcConstraintDebug(res, 2, stderr);
	  }

	  if( DEBUG2 )
	  {
	    fprintf(stderr, "  adding new constraint:\n");
	    NrcConstraintDebug(res, 2, stderr);
	    fprintf(stderr, "  bag afterwards:\n");
	    NrcCondenserBagDebug(cb, 2, stderr);
	  }
	}

        /* kill off the original constraints of the pair */
	if( DEBUG_SUCCESSES )
	{
	  fprintf(stderr, "  removing old constraints:\n");
	  if( pair->init_cc != NULL )
	    NrcConstraintDebug(pair->init_cc->orig_constraint, 2, stderr);
	  NrcConstraintDebug(pair->full_cc->orig_constraint, 2, stderr);
	}
	if( pair->init_cc != NULL )
	  NrcConstraintKill(pair->init_cc->orig_constraint);
	NrcConstraintKill(pair->full_cc->orig_constraint);
      }
      if( DEBUG_SUCCESSES )
	fprintf(stderr, "]\n");
    }

    if( DEBUG_FAILURES )
    {
      if( !(lengths_work && weights_work && inits_work) ||
	  HaArrayCount(cb->constraints) > 0 )
      {
	fprintf(stderr, "NrcCondenserBagExactCondense(%s)%s%s:\n",
	  NrcInstanceId(cc->instance),
	  lengths_work && weights_work ? "" : ", failed pairs",
          HaArrayCount(cb->constraints) == 0 ? "" : ", failed unpaired");
	NrcCondenserBagDebug(cb, 2, stderr);
      }
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "] NrcCondenserBagExactCondense\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintCondenserMerge(NRC_CONSTRAINT_CONDENSER cc)            */
/*                                                                           */
/*  Do the merging promised by cm.                                           */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintCondenserMerge(NRC_CONSTRAINT_CONDENSER cc)
{
  NRC_CONDENSER_BAG cb;  NRC_CONDENSED_CONSTRAINT c;  int i, j;
  if( DEBUG2 )
    fprintf(stderr, "[ NrcConstraintCondenserMerge(cc)\n");
  HaArrayForEach(cc->bags, cb, i)
  {
    if( DEBUG2 )
    {
      fprintf(stderr, "  NrcConstraintCondenserMerge examining bag:\n");
      NrcCondenserBagDebug(cb, 2, stderr);
    }
    c = HaArrayFirst(cb->constraints);
    if( c->limits_max )
    {
      /* max limit */
      HaArrayForEach(cb->constraints, c, j)
	NrcCondensedConstraintMaxMakeConsec(c);
    }
    else
    {
      /* exact limits;  build and sort pairs */
      NrcCondenserBagExactCondense(cb, cc);
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "] NrcConstraintCondenserMerge returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintCondenserDebug(NRC_CONSTRAINT_CONDENSER cm,            */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of cm onto fp with the given indent.                         */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintCondenserDebug(NRC_CONSTRAINT_CONDENSER cm,
  int indent, FILE *fp)
{
  NRC_CONDENSER_BAG cb;  int i;
  fprintf(fp, "%*s[ ConstraintCondenser(%s, %d constraints, %d bags):\n",
    indent, "", NrcInstanceId(cm->instance), HaArrayCount(cm->constraints),
    HaArrayCount(cm->bags));
  HaArrayForEach(cm->bags, cb, i)
    NrcCondenserBagDebug(cb, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}

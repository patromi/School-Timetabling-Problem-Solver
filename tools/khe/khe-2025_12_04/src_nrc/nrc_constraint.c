
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
/*  FILE:         nrc_constraint.c                                           */
/*  MODULE:       A worker constraint                                        */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

#define DEBUG1 0
/* #define DEBUG2 0 */


/*****************************************************************************/
/*                                                                           */
/*  NRC_CONSTRAINT - a worker constraint                                     */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_SHIFT_SET) ARRAY_NRC_SHIFT_SET;
typedef HA_ARRAY(NRC_POLARITY) ARRAY_NRC_POLARITY;

struct nrc_constraint_rec {
  NRC_INSTANCE			instance;
  char				*name;
  NRC_WORKER_SET		worker_set;
  NRC_CONSTRAINT_TYPE		type;
  NRC_BOUND			bound;
  NRC_SHIFT_SET			starting_ss;
  ARRAY_NRC_SHIFT_SET		shift_sets;
  ARRAY_NRC_POLARITY		polarities;
  int				history_before;		/* history: ai       */
  int				history_after;		/* history: ci       */
  HA_ARRAY_INT			history;		/* history: xi       */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "creation and query"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_CONSTRAINT NrcConstraintMake(NRC_INSTANCE ins, char *name,           */
/*    NRC_WORKER_SET ws, NRC_CONSTRAINT_TYPE type, NRC_BOUND bound,          */
/*    NRC_SHIFT_SET starting_ss)                                             */
/*                                                                           */
/*  Add a constraint with these attributes to ins.                           */
/*                                                                           */
/*****************************************************************************/

NRC_CONSTRAINT NrcConstraintMake(NRC_INSTANCE ins, char *name,
  NRC_WORKER_SET ws, NRC_CONSTRAINT_TYPE type, NRC_BOUND bound,
  NRC_SHIFT_SET starting_ss)
{
  NRC_CONSTRAINT res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->instance = ins;
  res->name = HnStringCopy(name, a);
  res->worker_set = ws;
  res->type = type;
  /* res->uniformity = NRC_UNIFORMITY_UNKNOWN; */
  /* res->offset = -1; */
  res->bound = bound;
  /* res->penalty = penalty; */
  /* res->limit = limit; */
  res->starting_ss = starting_ss;
  HaArrayInit(res->shift_sets, a);
  HaArrayInit(res->polarities, a);
  res->history_before = 0;	/* signifies not present */
  res->history_after = 0;	/* signifies not present */
  HaArrayInit(res->history, a);
  /* res->regularity = NULL; */
  NrcInstanceAddConstraint(ins, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE NrcConstraintInstance(NRC_CONSTRAINT c)                     */
/*                                                                           */
/*  Return the instance attribute of c.                                      */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE NrcConstraintInstance(NRC_CONSTRAINT c)
{
  return c->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcConstraintName(NRC_CONSTRAINT c)                                */
/*                                                                           */
/*  Return the name attribute of c.                                          */
/*                                                                           */
/*****************************************************************************/

char *NrcConstraintName(NRC_CONSTRAINT c)
{
  return c->name;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_SET NrcConstraintWorkerSet(NRC_CONSTRAINT c)                  */
/*                                                                           */
/*  Return the worker_set attribute of c.                                    */
/*                                                                           */
/*****************************************************************************/

NRC_WORKER_SET NrcConstraintWorkerSet(NRC_CONSTRAINT c)
{
  return c->worker_set;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY NrcConstraintPenalty(NRC_CONSTRAINT c)                       */
/*                                                                           */
/*  Return the penalty attribute of c.                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
NRC_PENALTY NrcConstraintPenalty(NRC_CONSTRAINT c)
{
  return c->penalty;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  NRC_LIMIT NrcConstraintLimit(NRC_CONSTRAINT c)                           */
/*                                                                           */
/*  Return the limit attribute of c.                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
NRC_LIMIT NrcConstraintLimit(NRC_CONSTRAINT c)
{
  return c->limit;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  NRC_CONSTRAINT_TYPE NrcConstraintType(NRC_CONSTRAINT c)                  */
/*                                                                           */
/*  Return the type attribute of c.                                          */
/*                                                                           */
/*****************************************************************************/

NRC_CONSTRAINT_TYPE NrcConstraintType(NRC_CONSTRAINT c)
{
  return c->type;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_BOUND NrcConstraintBound(NRC_CONSTRAINT c)                           */
/*                                                                           */
/*  Return the bound attribute of c.                                         */
/*                                                                           */
/*****************************************************************************/

NRC_BOUND NrcConstraintBound(NRC_CONSTRAINT c)
{
  return c->bound;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcConstraintStartingShiftSet(NRC_CONSTRAINT c)            */
/*                                                                           */
/*  Return the starting_ss attribute of c.                                   */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcConstraintStartingShiftSet(NRC_CONSTRAINT c)
{
  return c->starting_ss;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "shift-sets"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool NrcConstraintLimitsWorkload(NRC_CONSTRAINT c)                       */
/*                                                                           */
/*  Return true if c needs to be implemented by a limit workload constraint. */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool NrcConstraintLimitsWorkload(NRC_CONSTRAINT c)
{
  NRC_LIMIT_TYPE limit_type;
  limit_type = NrcLimitType(c->limit);
  return limit_type == NRC_LIMIT_MIN_WORKLOAD ||
    limit_type == NRC_LIMIT_MAX_WORKLOAD ||
    limit_type == NRC_LIMIT_MIN_AND_MAX_WORKLOAD;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool NrcConstraintIsConsecutive(NRC_CONSTRAINT c)                        */
/*                                                                           */
/*  Return true if c needs to be implemented by a limit active intervals     */
/*  constraint.                                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool NrcConstraintIsConsecutive(NRC_CONSTRAINT c)
{
  NRC_LIMIT_TYPE limit_type;
  limit_type = NrcLimitType(c->limit);
  return limit_type == NRC_LIMIT_MIN_CONSECUTIVE ||
    limit_type == NRC_LIMIT_MAX_CONSECUTIVE ||
    limit_type == NRC_LIMIT_MIN_AND_MAX_CONSECUTIVE;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintAddShiftSet(NRC_CONSTRAINT c, NRC_SHIFT_SET ss,        */
/*    NRC_POLARITY po)                                                       */
/*                                                                           */
/*  Add ss to c with the given polarity.                                     */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintAddShiftSet(NRC_CONSTRAINT c, NRC_SHIFT_SET ss,
  NRC_POLARITY po)
{
  HnAssert(c->type != NRC_CONSTRAINT_WORKLOAD || po == NRC_POSITIVE,
   "NrcConstraintAddShiftSet:  negative polarity incompatible with limit_type");
  HaArrayAddLast(c->shift_sets, ss);
  HaArrayAddLast(c->polarities, po);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintAddShiftSetSet(NRC_CONSTRAINT c,                       */
/*    NRC_SHIFT_SET_SET sss, NRC_POLARITY po)                                */
/*                                                                           */
/*  Add the shift-sets of sss to c, with the given polarity.                 */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintAddShiftSetSet(NRC_CONSTRAINT c,
  NRC_SHIFT_SET_SET sss, NRC_POLARITY po)
{
  int i;  NRC_SHIFT_SET ss;
  for( i = 0;  i < NrcShiftSetSetShiftSetCount(sss);  i++ )
  {
    ss = NrcShiftSetSetShiftSet(sss, i);
    NrcConstraintAddShiftSet(c, ss, po);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintAddPattern(NRC_CONSTRAINT c, NRC_PATTERN p, NRC_DAY d) */
/*                                                                           */
/*  Add p, starting on day d, to c.                                          */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintAddPattern(NRC_CONSTRAINT c, NRC_PATTERN p, NRC_DAY d)
{
  int i;  NRC_SHIFT_TYPE_SET sts;
  NRC_POLARITY po;  NRC_SHIFT_SET ss;  NRC_DAY d2;
  for( i = 0;  i < NrcPatternTermCount(p);  i++ )
  {
    NrcPatternTerm(p, i, &sts, &po);
    HnAssert(NrcDayIndexInCycle(d) + i < NrcInstanceCycleDayCount(c->instance),
      "NrcConstraintAddPattern: pattern goes off end of cycle");
    d2 = NrcInstanceCycleDay(c->instance, NrcDayIndexInCycle(d) + i);
    ss = NrcDayShiftSetFromShiftTypeSet(d2, sts);
    NrcConstraintAddShiftSet(c, ss, po);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcConstraintShiftSetCount(NRC_CONSTRAINT c)                         */
/*                                                                           */
/*  Return the number of shift-sets in c.                                    */
/*                                                                           */
/*****************************************************************************/

int NrcConstraintShiftSetCount(NRC_CONSTRAINT c)
{
  return HaArrayCount(c->shift_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintShiftSet(NRC_CONSTRAINT c, int i,                      */
/*    NRC_SHIFT_SET *ss, NRC_POLARITY *po)                                   */
/*                                                                           */
/*  Return the i'th shift-set of c, with its polarity.                       */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintShiftSet(NRC_CONSTRAINT c, int i,
  NRC_SHIFT_SET *ss, NRC_POLARITY *po)
{
  *ss = HaArray(c->shift_sets, i);
  *po = HaArray(c->polarities, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintConvertActiveToConsecutiveMax(NRC_CONSTRAINT c)        */
/*                                                                           */
/*  Convert c, which places a limit on the maximum number of active time     */
/*  groups, to one which places limits on consecutive time groups.           */
/*                                                                           */
/*****************************************************************************/

/* *** moved to NrcCondensedConstraintMaxMakeConsecutive in nrc_condensed.c
void NrcConstraintConvertActiveToConsecutiveMax(NRC_CONSTRAINT c)
{
  NRC_CONSTRAINT res;  NRC_POLARITY po;  int starting_ss_count, i, offset;
  char *name;  NRC_SHIFT_SET ss, shifted_ss;

  if( DEBUG2 )
  {
    fprintf(stderr, "[ NrcConstraintConvertActiveToConsecutiveMax(c), c =\n");
    NrcConstraintDebug(c, 2, stderr);
  }

  ** make and add a new consecutive constraint to replace c **
  name = HnStringMake(NrcInstanceArena(c->instance), "%s (redone)", c->name);
  res = NrcConstraintMake(c->instance, name, c->worker_set,
    NRC_CONSTRAINT_CONSECUTIVE, c->bound, NULL);

  ** add current shift sets **
  po = HaArrayFirst(c->polarities);
  HaArrayForEach(c->shift_sets, ss, i)
    NrcConstraintAddShiftSet(res, ss, po);

  ** add shifted shift sets **
  starting_ss_count = NrcShiftSetShiftCount(NrcConstraintStartingShiftSet(c));
  HnAssert(starting_ss_count >= 2,
    "NrcConstraintConvertActiveToConsecutiveMax internal error");
  ss = HaArrayLast(c->shift_sets);
  cr = NrcConstraintRegularity(c);
  offset = NrcConstraintRegularityOffset(cr);
  for( i = 1;  i < starting_ss_count;  i++ )
  {
    if( NrcShiftSetMakeShifted(ss, offset, &shifted_ss) )
      NrcConstraintAddShiftSet(res, shifted_ss, po);
    ss = shifted_ss;
  }

  ** kill c by emptying its worker set **
  c->worker_set = NrcInstanceEmptyWorkerSet(c->instance);

  if( DEBUG2 )
  {
    fprintf(stderr, "  new constraint =\n");
    NrcConstraintDebug(res, 2, stderr);
    fprintf(stderr, "] NrcConstraintConvertActiveToConsecutiveMax returning\n");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintKill(NRC_CONSTRAINT c)                                 */
/*                                                                           */
/*  Kill c by giving it an empty worker-set.                                 */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintKill(NRC_CONSTRAINT c)
{
  c->worker_set = NrcInstanceEmptyWorkerSet(c->instance);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_CONSTRAINT NrcUnwantedPatternConstraintMake(NRC_INSTANCE ins,        */
/*    char *name, NRC_WORKER_SET ws, NRC_PENALTY penalty, NRC_PATTERN p,     */
/*    NRC_DAY_SET starting_ds)                                               */
/*                                                                           */
/*  Create an unwanted pattern constraint with these attributes.             */
/*                                                                           */
/*****************************************************************************/

NRC_CONSTRAINT NrcUnwantedPatternConstraintMake(NRC_INSTANCE ins,
  char *name, NRC_WORKER_SET ws, NRC_PENALTY penalty, NRC_PATTERN p,
  NRC_DAY_SET starting_ds)
{
  NRC_CONSTRAINT res;  NRC_SHIFT_TYPE_SET sts;  NRC_POLARITY po;  int i;
  NRC_DAY d;  NRC_SHIFT_SET ss;  NRC_BOUND b;

  HnAssert(NrcPatternTermCount(p) > 0,
    "NrcUnwantedPatternConstraintMake:  empty pattern");
  HnAssert(NrcDaySetDayCount(starting_ds) > 0,
    "NrcUnwantedPatternConstraintMake:  empty starting_ds");
  b = NrcBoundMakeMax(NrcPatternTermCount(p) - 1, penalty, ins);
  if( false && NrcDaySetDayCount(starting_ds) + NrcPatternTermCount(p) >
        NrcInstanceCycleDayCount(ins) && NrcPatternIsUniform(p) )
  {
    /* uniform pattern, single limit active intervals constraint */
    res = NrcConstraintMake(ins, name, ws, NRC_CONSTRAINT_CONSECUTIVE, b, NULL);
    NrcPatternTerm(p, 0, &sts, &po);
    for( i = 0;  i < NrcInstanceCycleDayCount(ins);  i++ )
    {
      d = NrcInstanceCycleDay(ins, i);
      ss = NrcDayShiftSetFromShiftTypeSet(d, sts);
      NrcConstraintAddShiftSet(res, ss, po);
    }
  }
  else
  {
    /* non-uniform pattern, repeating cluster busy times constraint */
    res = NrcConstraintMake(ins, name, ws, NRC_CONSTRAINT_ACTIVE, b,
      NrcDaySetStartingShiftSet(starting_ds));
    NrcConstraintAddPattern(res, p, NrcDaySetDay(starting_ds, 0));
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "history"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintAddHistory(NRC_CONSTRAINT c, int history_before,       */
/*    int history_after)                                                     */
/*                                                                           */
/*  Add these history_before (ai) and history_after (ci) values to c.        */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintAddHistory(NRC_CONSTRAINT c, int history_before,
  int history_after)
{
  HnAssert(c->history_before == 0, "NrcConstraintAddHistory called twice on c");
  HnAssert(history_before >= 0, "NrcConstraintAddHistory: history_before < 0");
  HnAssert(history_after >= 0,  "NrcConstraintAddHistory: history_after < 0");
  c->history_before = history_before;
  c->history_after = history_after;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintAddHistoryWorker(NRC_CONSTRAINT c, NRC_WORKER w,       */
/*    int value)                                                             */
/*                                                                           */
/*  Add this history xi value to c.                                          */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintAddHistoryWorker(NRC_CONSTRAINT c, NRC_WORKER w, int value)
{
  int pos;
  HnAssert(value >= 0, "NrcConstraintAddHistory:  value (%d) < 0", value);
  HnAssert(value <= c->history_before,
    "NrcConstraintAddHistory:  value (%d) > history_before (%d)", value,
    c->history_before);
  HnAssert(c->starting_ss == NULL,
    "NrcConstraintAddHistory:  non-NULL starting shift-set");
  if( !NrcWorkerSetContains(c->worker_set, w, &pos) )
    HnAbort("NrcConstraintAddHistory:  w not in c's worker set");
  HaArrayFill(c->history, pos + 1, 0);
  if( HaArray(c->history, pos) != 0 )
    HnAbort("NrcConstraintAddHistory:  w's history added twice");
  HaArrayPut(c->history, pos, value);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintAddHistoryAllWorkers(NRC_CONSTRAINT c,                 */
/*    int history_before, int history_after, char *name)                     */
/*                                                                           */
/*  Add a history value for each worker in c's worker set, defined by name.  */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintAddHistoryAllWorkers(NRC_CONSTRAINT c,
  int history_before, int history_after, char *name)
{
  NRC_WORKER w;  int i, v;
  NrcConstraintAddHistory(c, history_before, history_after);
  for( i = 0;  i < NrcWorkerSetWorkerCount(c->worker_set);  i++ )
  {
    w = NrcWorkerSetWorker(c->worker_set, i);
    if( NrcWorkerRetrieveHistory(w, name, &v) && v > 0 )
      NrcConstraintAddHistoryWorker(c, w, v);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "semi-uniform constraints"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool NrcPolaritiesHaveFullUniformity(NRC_CONSTRAINT c)                   */
/*                                                                           */
/*  Return true if the polarities of c have full uniformity, that is,        */
/*  they are all equal.                                                      */
/*                                                                           */
/*****************************************************************************/

static bool NrcPolaritiesHaveFullUniformity(NRC_CONSTRAINT c)
{
  NRC_POLARITY po_0;  int i;
  po_0 = HaArrayFirst(c->polarities);
  for( i = 1;  i < HaArrayCount(c->polarities);  i++ )
    if( HaArray(c->polarities, i) != po_0 )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcPolaritiesHaveNotLRUniformity(NRC_CONSTRAINT c)                  */
/*                                                                           */
/*  Return true if the polarities of c have not-lr uniformity, that is,      */
/*  they are all equal except that the first and last are the opposite.      */
/*                                                                           */
/*****************************************************************************/

static bool NrcPolaritiesHaveNotLRUniformity(NRC_CONSTRAINT c)
{
  NRC_POLARITY po_0;  int i;
  if( HaArrayCount(c->polarities) <= 2 )
    return false;
  po_0 = HaArrayFirst(c->polarities);
  for( i = 1;  i < HaArrayCount(c->polarities) - 1;  i++ )
    if( HaArray(c->polarities, i) == po_0 )
      return false;
  if( HaArrayLast(c->polarities) != po_0 )
    return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcPolaritiesHaveNotRUniformity(NRC_CONSTRAINT c)                   */
/*                                                                           */
/*  Return true if the polarities of c have not-r uniformity, that is,       */
/*  they are all equal except that the last is the opposite.                 */
/*                                                                           */
/*****************************************************************************/

static bool NrcPolaritiesHaveNotRUniformity(NRC_CONSTRAINT c)
{
  NRC_POLARITY po_0;  int i;
  po_0 = HaArrayFirst(c->polarities);
  for( i = 1;  i < HaArrayCount(c->polarities) - 1;  i++ )
    if( HaArray(c->polarities, i) != po_0 )
      return false;
  if( HaArrayLast(c->polarities) == po_0 )
    return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_CONDENSED_CONSTRAINT NrcConstraintCondensedConstraint(               */
/*    NRC_CONSTRAINT c)                                                      */
/*                                                                           */
/*  Return the condensed version of c, or NULL if c cannot be condensed.     */
/*                                                                           */
/*****************************************************************************/

NRC_CONDENSED_CONSTRAINT NrcConstraintCondensedConstraint(NRC_CONSTRAINT c)
{
  int value, max_value, i, offset, offs, length;  bool allow_zero;
  NRC_PENALTY p1, p2;  NRC_SHIFT_SET ss1, ss2;  NRC_POLARITY po;
  bool limits_initial, limits_max, multi_starts;

  /* type must be active */
  if( c->type != NRC_CONSTRAINT_ACTIVE )
    return NULL;

  /* must have a max limit only, one less than the number of shift sets */
  if( NrcBoundMin(c->bound, &value, &allow_zero, &p1) ||
      NrcBoundPreferred(c->bound, &value, &p2, &p2) )
    return NULL;
  if( !NrcBoundMax(c->bound, &max_value, &p1) )
    return NULL;
  if( max_value != HaArrayCount(c->shift_sets) - 1 )
    return NULL;

  /* must be at least two shift sets */
  if( HaArrayCount(c->shift_sets) < 2 )
    return NULL;

  /* shift sets must be uniform, with equal offsets */
  ss1 = HaArray(c->shift_sets, 0);
  ss2 = HaArray(c->shift_sets, 1);
  if( !NrcShiftSetsUniform(ss1, ss2, &offset) )
    return NULL;
  for( i = 2;  i < HaArrayCount(c->shift_sets);  i++ )
  {
    ss1 = HaArray(c->shift_sets, i-1);
    ss2 = HaArray(c->shift_sets, i);
    if( !NrcShiftSetsUniform(ss1, ss2, &offs) || offs != offset )
      return NULL;
  }

  /* if the starting shift-set has an offset, it must equal offset */
  multi_starts = (c->starting_ss != NULL &&
    NrcShiftSetShiftCount(c->starting_ss) >= 2);
  if( multi_starts )
  {
    if( !NrcShiftSetUniform(c->starting_ss, &offs) || offs != offset )
      return NULL;
  }

  /* must be no history (it's too hard to analyse) */
  if( c->history_before != 0 || c->history_after != 0 ||
      HaArrayCount(c->history) > 0 )
    return NULL;

  /* regularity depends on the polarities now */
  if( NrcPolaritiesHaveFullUniformity(c) )
  {
    /* all the shift-sets have the same polarity */
    if( !multi_starts )
      return NULL;
    po = HaArrayFirst(c->polarities);
    limits_initial = false;
    limits_max = true;
  }
  else if( NrcPolaritiesHaveNotRUniformity(c) )
  {
    /* all have the same polarity except for the last one */
    if( multi_starts )
      return NULL;
    po = HaArrayFirst(c->polarities);
    limits_initial = true;
    limits_max = false;
  }
  else if( NrcPolaritiesHaveNotLRUniformity(c) )
  {
    /* all have the same polarity except for the first and last ones */
    if( !multi_starts )
      return NULL;
    po = HaArray(c->polarities, 1);
    limits_initial = false;
    limits_max = false;
  }
  else
    return NULL;

  /* all good, make and return the condensed version */
  length = (c->starting_ss == NULL ? 1 : NrcShiftSetShiftCount(c->starting_ss))
    + HaArrayCount(c->shift_sets) - 1;
  return NrcCondensedConstraintMake(c->instance, c->name, c->worker_set,
    c->bound, HaArrayFirst(c->shift_sets), length, offset, po,
    limits_initial, limits_max, c);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_CONSTRAINT_REGULARITY NrcConstraintRegularity(NRC_CONSTRAINT c)      */
/*                                                                           */
/*  Return the regularity attribute of c.  This will be NULL if it has       */
/*  not been set yet, and also if c is not regular.                          */
/*                                                                           */
/*****************************************************************************/

/* ***
NRC_CONSTRAINT_REGULARITY NrcConstraintRegularity(NRC_CONSTRAINT c)
{
  return c->regularity;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "conversion"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int NrcConstraintTypedCmpIgnoreNamesAndBoundsAndWorkerSets(              */
/*    NRC_CONSTRAINT c1, NRC_CONSTRAINT c2)                                  */
/*                                                                           */
/*  Typed comparison function for bringing together constraints which are    */
/*  equal when their names, bounds, and worker-sets are ignored.             */
/*                                                                           */
/*  Formerly I wrote:  "actually their history_before and history_after      */
/*  values may differ, as long as one or both of the constraints has no      */
/*  minimum bound."  This is wrong.                                          */
/*                                                                           */
/*****************************************************************************/

int NrcConstraintTypedCmpIgnoreNamesAndBoundsAndWorkerSets(NRC_CONSTRAINT c1,
  NRC_CONSTRAINT c2)
{
  int cmp, i;  NRC_SHIFT_SET ss1, ss2;  NRC_POLARITY po1, po2;
  int min_value;  bool allow_zero;  NRC_PENALTY below_min_penalty;

  /* compare types */
  cmp = (int) c1->type - (int) c2->type;
  if( cmp != 0 ) return cmp;

  /* compare starting shift-sets (which may be NULL) */
  cmp = NrcShiftSetTypedCmp(c1->starting_ss, c2->starting_ss);
  if( cmp != 0 )  return cmp;

  /* compare shift-sets and polarities */
  cmp = HaArrayCount(c1->shift_sets) - HaArrayCount(c2->shift_sets);
  if( cmp != 0 )  return cmp;
  for( i = 0;  i < HaArrayCount(c1->shift_sets);  i++ )
  {
    ss1 = HaArray(c1->shift_sets, i);
    ss2 = HaArray(c2->shift_sets, i);
    cmp = NrcShiftSetTypedCmp(ss1, ss2);
    if( cmp != 0 )  return cmp;
    po1 = HaArray(c1->polarities, i);
    po2 = HaArray(c2->polarities, i);
    cmp = (int) po1 - (int) po2;
    if( cmp != 0 )  return cmp;
  }

  /* compare history_before and history */
  cmp = c1->history_before - c2->history_before;
  if( cmp != 0 )  return cmp;
  cmp = HaArrayCount(c1->history) - HaArrayCount(c2->history);
  if( cmp != 0 )  return cmp;
  for( i = 0;  i < HaArrayCount(c1->history);  i++ )
  {
    cmp = HaArray(c1->history, i) - HaArray(c2->history, i);
    if( cmp != 0 )  return cmp;
  }

  /* compare history_after only if both have a min bound */
  if( NrcBoundMin(c1->bound, &min_value, &allow_zero, &below_min_penalty) &&
      NrcBoundMin(c2->bound, &min_value, &allow_zero, &below_min_penalty) )
  {
    cmp = c1->history_after - c2->history_after;
    if( cmp != 0 )  return cmp;
  }

  /* all equal */
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcConstraintTypedCmpIgnoreNamesAndWorkerSets(NRC_CONSTRAINT c1,     */
/*    NRC_CONSTRAINT c2)                                                     */
/*                                                                           */
/*  Typed comparison function for bringing together constraints which are    */
/*  equal when their names and worker-sets are ignored.                      */
/*                                                                           */
/*****************************************************************************/

int NrcConstraintTypedCmpIgnoreNamesAndWorkerSets(NRC_CONSTRAINT c1,
  NRC_CONSTRAINT c2)
{
  int cmp;
  cmp = NrcConstraintTypedCmpIgnoreNamesAndBoundsAndWorkerSets(c1, c2);
  if( cmp != 0 )  return cmp;
  cmp = NrcBoundTypedCmp(c1->bound, c2->bound);
  if( cmp != 0 )  return cmp;
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcConstraintCmpIgnoreNamesAndWorkerSets(const void *p1,             */
/*    const void *p2)                                                        */
/*                                                                           */
/*  Untyped comparison function for bringing together constraints which are  */
/*  equal when their names and worker-sets are ignored.                      */
/*                                                                           */
/*****************************************************************************/

int NrcConstraintCmpIgnoreNamesAndWorkerSets(const void *p1, const void *p2)
{
  NRC_CONSTRAINT c1 = * (NRC_CONSTRAINT *) p1;
  NRC_CONSTRAINT c2 = * (NRC_CONSTRAINT *) p2;
  return NrcConstraintTypedCmpIgnoreNamesAndWorkerSets(c1, c2);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcConstraintTypedCmpIgnoreNamesAndBounds(NRC_CONSTRAINT c1,         */
/*    NRC_CONSTRAINT c2)                                                     */
/*                                                                           */
/*  Typed comparison function for bringing together constraints which are    */
/*  equal when their names and bounds are ignored.                           */
/*                                                                           */
/*****************************************************************************/

int NrcConstraintTypedCmpIgnoreNamesAndBounds(NRC_CONSTRAINT c1,
  NRC_CONSTRAINT c2)
{
  int cmp;
  cmp = NrcConstraintTypedCmpIgnoreNamesAndBoundsAndWorkerSets(c1, c2);
  if( cmp != 0 )  return cmp;
  cmp = NrcWorkerSetTypedCmp(c1->worker_set, c2->worker_set);
  if( cmp != 0 )  return cmp;
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcConstraintCmpIgnoreNamesAndBounds(const void *p1, const void *p2) */
/*                                                                           */
/*  Untyped comparison function for bringing together constraints which are  */
/*  equal when their names and bounds are ignored.                           */
/*                                                                           */
/*****************************************************************************/

int NrcConstraintCmpIgnoreNamesAndBounds(const void *p1, const void *p2)
{
  NRC_CONSTRAINT c1 = * (NRC_CONSTRAINT *) p1;
  NRC_CONSTRAINT c2 = * (NRC_CONSTRAINT *) p2;
  return NrcConstraintTypedCmpIgnoreNamesAndBounds(c1, c2);
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcConstraintGetId(NRC_CONSTRAINT c, int name_index, int i)        */
/*                                                                           */
/*  Generate a suitable name for c with name_index.                          */
/*                                                                           */
/*****************************************************************************/

char *NrcConstraintGetId(NRC_CONSTRAINT c, int name_index, int i)
{
  HA_ARENA a = NrcInstanceArena(c->instance);
  if( i == 0 )
    return HnStringMake(a, "Constraint:%d", name_index);
  else
    return HnStringMake(a, "Constraint:%d:%d", name_index, i);
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcMergedName(char *name1, char *name2)                            */
/*                                                                           */
/*  Merge name1 and name2.  The result is always a fresh string in malloced  */
/*  memory.  If name1 and name2 are equal, the result is equal to them;      */
/*  otherwise it is a form of merge.  For example, merging                   */
/*                                                                           */
/*     "common prefix summer solstice common suffix"                         */
/*     "common prefix autumn equinox common suffix"                          */
/*                                                                           */
/*  produces                                                                 */
/*                                                                           */
/*     "common prefix (summer solstice / autumn equinox) common suffix"      */
/*                                                                           */
/*****************************************************************************/

static char *NrcMergedName(char *name1, char *name2, HA_ARENA a)
{
  char *p1, *p2, *q1, *q2, *p;  int len1, len2;  HA_ARRAY_NCHAR ac;
  char *last_p1_space, *last_p2_space, *first_q1_space, *first_q2_space;

  /* if equal, just the common value */
  if( strcmp(name1, name2) == 0 )
    return HnStringCopy(name1, a);

  /* find the first and last places where the two strings differ */
  last_p1_space = name1 - 1;
  last_p2_space = name2 - 1;
  for( p1 = name1, p2 = name2;  *p1 == *p2;  p1++, p2++ )
    if( *p1 == ' ' )
      last_p1_space = p1, last_p2_space = p2;

  len1 = strlen(name1);
  len2 = strlen(name2);
  first_q1_space = &name1[len1];
  first_q2_space = &name2[len2];
  for( q1 = &name1[len1 - 1], q2 = &name2[len2 - 1];  *q1 == *q2;  q1--, q2-- )
    if( *q1 == ' ' )
      first_q1_space = q1, first_q2_space = q2;

  /* build the result string */
  HnStringBegin(ac, a);
  if( last_p1_space == name1 - 1 && last_p2_space == name2 - 1 &&
      first_q1_space == &name1[len1] && first_q2_space == &name2[len2] )
  {
    for( p = last_p1_space + 1;  p != first_q1_space;  p++ )
      HaArrayAddLast(ac, *p);
    HnStringAdd(&ac, " / ");
    for( p = last_p2_space + 1;  p != first_q2_space;  p++ )
      HaArrayAddLast(ac, *p);
  }
  else
  {
    for( p = name1;  p != last_p1_space + 1;  p++ )
      HaArrayAddLast(ac, *p);
    HnStringAdd(&ac, "(");
    for( p = last_p1_space + 1;  p != first_q1_space;  p++ )
      HaArrayAddLast(ac, *p);
    HnStringAdd(&ac, " / ");
    for( p = last_p2_space + 1;  p != first_q2_space;  p++ )
      HaArrayAddLast(ac, *p);
    HnStringAdd(&ac, ")");
    for( p = first_q2_space;  *p != '\0';  p++ )
      HaArrayAddLast(ac, *p);
  }
  return HnStringEnd(ac);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintMergeBounds(NRC_CONSTRAINT c, int first_index,         */
/*    int last_index)                                                        */
/*                                                                           */
/*  The constraints in range first_index .. last_index are all equal,        */
/*  including their worker sets, when their names and bounds are ignored.    */
/*  Merge pairs of them into double bounds where possible.                   */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintMergeBounds(NRC_CONSTRAINT c, int first_index,
  int last_index)
{
  int i, j;  NRC_CONSTRAINT ci, cj;  NRC_BOUND bound;  HA_ARENA a;
  int min_value;  bool allow_zero;  NRC_PENALTY below_min_penalty;

  if( DEBUG1 && last_index - first_index + 1 >= 2 )
  {
    fprintf(stderr, "[ NrcConstraintMergeBounds(%d constraints):\n",
      last_index - first_index + 1);
    for( i = first_index;  i <= last_index;  i++ )
    {
      ci = NrcInstanceConstraint(c->instance, i);
      NrcConstraintDebug(ci, 2, stderr);
    }
  }
  a = NrcInstanceArena(c->instance);
  for( i = first_index;  i <= last_index;  i++ )
  {
    ci = NrcInstanceConstraint(c->instance, i);
    for( j = i + 1;  j <= last_index;  j++ )
    {
      cj = NrcInstanceConstraint(c->instance, j);
      if( NrcBoundMerge(ci->bound, cj->bound, &bound) )
      {
	if( DEBUG1 )
	{
	  fprintf(stderr, "merging bounds of constraint\n");
	  NrcConstraintDebug(ci, 2, stderr);
	  fprintf(stderr, "with constraint\n");
	  NrcConstraintDebug(cj, 2, stderr);
	}

	/* change cj to represent the merged constraint */
	cj->bound = bound;
	cj->name = NrcMergedName(ci->name, cj->name, a);
	if( NrcBoundMin(ci->bound, &min_value, &allow_zero,&below_min_penalty) )
	{
	  /* we need ci's history_after */
	  cj->history_after = ci->history_after;
	}
	if( DEBUG1 )
	{
	  fprintf(stderr, "to produce constraint\n");
	  NrcConstraintDebug(cj, 2, stderr);
	}

	/* kill ci by emptying its worker set */
	ci->worker_set = NrcInstanceEmptyWorkerSet(c->instance);
	break; /* no point continuing to try to merge ci */
      }
    }
  }
  if( DEBUG1 && last_index - first_index + 1 >= 2 )
    fprintf(stderr, "] NrcConstraintMergeBounds returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcConstraintUseLimitBusy(NRC_CONSTRAINT c)                         */
/*                                                                           */
/*  Return true if c can use a limit busy times constraint.                  */
/*                                                                           */
/*****************************************************************************/

static bool NrcConstraintUseLimitBusy(NRC_CONSTRAINT c)
{
  NRC_SHIFT_SET ss;  int i;
  if( c->history_before > 0 || c->history_after > 0 ||
      HaArrayCount(c->history) > 0 )
    return false;
  HaArrayForEach(c->shift_sets, ss, i)
    if( NrcShiftSetShiftCount(ss) != 1 ||
	HaArray(c->polarities, i) == NRC_NEGATIVE)
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_POLARITY NrcPolarityToKhePolarity(NRC_POLARITY po)                   */
/*                                                                           */
/*  Convert an NRC polarity into a KHE polarity.                             */
/*                                                                           */
/*****************************************************************************/

static KHE_POLARITY NrcPolarityToKhePolarity(NRC_POLARITY po)
{
  return (po == NRC_NEGATIVE ? KHE_NEGATIVE : KHE_POSITIVE);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcBoundAdjust(NRC_CONSTRAINT c, int min_limit,                     */
/*    bool allow_zero, int *max_limit)                                       */
/*                                                                           */
/*  According to the bound, a constraint is needed with these attributes.    */
/*  Return true if it really is needed, and reset *max_limit if required.    */
/*                                                                           */
/*****************************************************************************/

static bool NrcBoundAdjust(NRC_CONSTRAINT c, int min_limit,
  bool allow_zero, int *max_limit)
{
  int m;
  switch( c->type )
  {
    case NRC_CONSTRAINT_ACTIVE:

      m = HaArrayCount(c->shift_sets) + c->history_before + c->history_after;
      if( *max_limit == INT_MAX )
	*max_limit = m;
      return (allow_zero && min_limit > 1) || (!allow_zero && min_limit > 0)
	|| *max_limit < m;

    case NRC_CONSTRAINT_CONSECUTIVE:

      m = HaArrayCount(c->shift_sets) + c->history_before + c->history_after;
      if( *max_limit == INT_MAX )
	*max_limit = m;
      return min_limit > 1 || *max_limit < m;

    case NRC_CONSTRAINT_WORKLOAD:

      if( *max_limit == INT_MAX )
	*max_limit = NrcInstanceTotalWorkload(c->instance);
      return min_limit>0 || *max_limit < NrcInstanceTotalWorkload(c->instance);

    default:

      HnAbort("NrcBoundAdjust internal error (type %d)", c->type);
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcConstraintAllResources(NRC_INSTANCE ins,                         */
/*    int first_index, int last_index, KHE_RESOURCE_GROUP *krg)              */
/*                                                                           */
/*  If the constraints between first_index and last_index inclusive          */
/*  cover all resources exactly once, return true with *krg set to the       */
/*  resource group holding all resources.                                    */
/*                                                                           */
/*****************************************************************************/

static bool NrcConstraintAllResources(NRC_INSTANCE ins,
  int first_index, int last_index, KHE_RESOURCE_GROUP *krg)
{
  NRC_WORKER_SET ws;  NRC_CONSTRAINT c;  int i;  char *name;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  name = HnStringMake(a, "CS:%d-%d", first_index, last_index);
  ws = NrcWorkerSetMake(ins, name);
  for( i = first_index;  i <= last_index;  i++ )
  {
    c = NrcInstanceConstraint(ins, i);
    NrcWorkerSetAddWorkerSet(ws, c->worker_set);
  }
  if( NrcWorkerSetWorkerCount(ws) == NrcInstanceStaffingWorkerCount(ins) &&
      NrcWorkerSetHasNoDuplicates(ws) )
  {
    *krg = NrcWorkerSetResourceGroup(NrcInstanceStaffing(ins),
      NrcInstanceKheInstance(ins));
    return true;
  }
  else
  {
    *krg = NULL;
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintGenerateLimitWorkload(NRC_CONSTRAINT c,                */
/*    int name_index, int first_index, int last_index, KHE_INSTANCE ins)     */
/*                                                                           */
/*  Generate one limit workload constraint, for c etc.                       */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintGenerateLimitWorkload(NRC_CONSTRAINT c,
  int name_index, int first_index, int last_index, KHE_INSTANCE ins)
{
  char *id;  int i, j, min_limit, max_limit;  bool allow_zero;
  KHE_LIMIT_WORKLOAD_CONSTRAINT kc;  NRC_WORKER w;  NRC_SHIFT_SET ss;
  KHE_RESOURCE_GROUP rg;  NRC_PENALTY p;
  /* if( NrcConstraintLimits(c, &min_limit, &max_limit, &allow_zero) ) */
  for( i = 0;  i < NrcBoundItemCount(c->bound);  i++ )
  {
    NrcBoundItem(c->bound, i, &min_limit, &allow_zero, &max_limit, &p);
    if( NrcBoundAdjust(c, min_limit, allow_zero, &max_limit) )
    {
      /* make the constraint and add its time groups */
      id = NrcConstraintGetId(c, name_index, i);
      if( !KheLimitWorkloadConstraintMake(ins, id, c->name,
	  NrcPenaltyConvert(p), NULL, min_limit, max_limit, false, &kc) )
        HnAbort("NrcConstraintGenerateLimitWorkload internal error %s",id);
      for( j = 0;  j < HaArrayCount(c->shift_sets);  j++ )
      {
	ss = HaArray(c->shift_sets, j);
	KheLimitWorkloadConstraintAddTimeGroup(kc,NrcShiftSetTimeGroup(ss,ins));
      }

      /* add the resources and resource groups */
      if( NrcConstraintAllResources(c->instance, first_index, last_index, &rg) )
	KheLimitWorkloadConstraintAddResourceGroup(kc, rg);
      else
      {
	for( j = first_index;  j <= last_index;  j++ )
	{
	  c = NrcInstanceConstraint(c->instance, j);
	  if( NrcWorkerSetWorkerCount(c->worker_set) == 1 )
	  {
	    w = NrcWorkerSetWorker(c->worker_set, 0);
	    KheLimitWorkloadConstraintAddResource(kc, NrcWorkerResource(w));
	  }
	  else
	    KheLimitWorkloadConstraintAddResourceGroup(kc,
	      NrcWorkerSetResourceGroup(c->worker_set, ins));
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintGenerateLimitActive(NRC_CONSTRAINT c,                  */
/*    int name_index, int first_index, int last_index, KHE_INSTANCE ins)     */
/*                                                                           */
/*  Generate a limit active intervals constraint for c.                      */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintGenerateLimitActive(NRC_CONSTRAINT c,
  int name_index, int first_index, int last_index, KHE_INSTANCE ins)
{
  char *id;  int min_limit, max_limit;  bool allow_zero;  NRC_CONSTRAINT c2;
  int i, j, k, v;  NRC_POLARITY po;  KHE_TIME_GROUP tg;  NRC_WORKER w;
  NRC_SHIFT_SET ss;  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT kc;
  KHE_RESOURCE_GROUP rg;  NRC_PENALTY p;
  /* if( NrcConstraintLimits(c, &min_limit, &max_limit, &allow_zero) ) */
  for( i = 0;  i < NrcBoundItemCount(c->bound);  i++ )
  {
    NrcBoundItem(c->bound, i, &min_limit, &allow_zero, &max_limit, &p);
    if( NrcBoundAdjust(c, min_limit, allow_zero, &max_limit) )
    {
      /* make the constraint and add its time groups */
      id = NrcConstraintGetId(c, name_index, i);
      tg = (c->starting_ss == NULL || NrcShiftSetShiftCount(c->starting_ss)==1 ?
	    NULL : NrcShiftSetTimeGroup(c->starting_ss, ins));
      if( !KheLimitActiveIntervalsConstraintMake(ins, id, c->name,
	    NrcPenaltyConvert(p), tg, min_limit, max_limit, &kc) )
        HnAbort("NrcConstraintGenerateLimitActive internal error (%s)",id);
      for( j = 0;  j < HaArrayCount(c->shift_sets);  j++ )
      {
	ss = HaArray(c->shift_sets, j);
	po = HaArray(c->polarities, j);
	KheLimitActiveIntervalsConstraintAddTimeGroup(kc,
	  NrcShiftSetTimeGroup(ss, ins), NrcPolarityToKhePolarity(po));
      }

      /* add the resources and resource groups */
      if( NrcConstraintAllResources(c->instance, first_index, last_index, &rg) )
	KheLimitActiveIntervalsConstraintAddResourceGroup(kc, rg);
      else
      {
	for( j = first_index;  j <= last_index;  j++ )
	{
	  c = NrcInstanceConstraint(c->instance, j);
	  if( NrcWorkerSetWorkerCount(c->worker_set) == 1 )
	  {
	    w = NrcWorkerSetWorker(c->worker_set, 0);
	    KheLimitActiveIntervalsConstraintAddResource(kc,NrcWorkerResource(w));
	  }
	  else
	    KheLimitActiveIntervalsConstraintAddResourceGroup(kc,
	      NrcWorkerSetResourceGroup(c->worker_set, ins));
	}
      }

      /* add history */
      KheLimitActiveIntervalsConstraintAddHistoryBefore(kc, c->history_before);
      KheLimitActiveIntervalsConstraintAddHistoryAfter(kc, c->history_after);
      for( j = first_index;  j <= last_index;  j++ )
      {
	c2 = NrcInstanceConstraint(c->instance, j);
	HaArrayForEach(c2->history, v, k) if( v > 0 )
	{
	  w = NrcWorkerSetWorker(c2->worker_set, k);
	  KheLimitActiveIntervalsConstraintAddHistory(kc,
	    NrcWorkerResource(w), v);
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcConstraintTimeGroupsAllNegative(NRC_CONSTRAINT c,                */
/*    int first_ss_index, int last_ss_index)                                 */
/*                                                                           */
/*  Return true if c's time groups all have negative polarity.               */
/*                                                                           */
/*****************************************************************************/

/* *** only used when inverting
static bool NrcConstraintTimeGroupsAllNegative(NRC_CONSTRAINT c,
  int first_ss_index, int last_ss_index)
{
  int i;  NRC_POLARITY po;
  for( i = first_ss_index;  i <= last_ss_index;  i++ )
  {
    po = HaArray(c->polarities, i);
    if( po == NRC_POSITIVE )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcGenerateClusterConstraint(NRC_CONSTRAINT c,                      */
/*    int first_index, int last_index, int weight,                           */
/*    char *id, int min_limit, int max_limit, bool allow_zero,               */
/*    int first_ss_index, int last_ss_index, bool negate_ends,               */
/*    KHE_INSTANCE ins)                                                      */
/*                                                                           */
/*  Generate one cluster busy times constraint.  The resources come from     */
/*  constraints first_index ... last_index, the id, min_limit, max_limit,    */
/*  and allow_zero attributes are as given, and the shift sets are from      */
/*  c, in the range first_ss_index ... last_ss_index.                        */
/*                                                                           */
/*****************************************************************************/

static void NrcGenerateClusterConstraint(NRC_CONSTRAINT c,
  int first_index, int last_index, /* int weight, */ NRC_PENALTY p,
  char *id, int min_limit, int max_limit, bool allow_zero,
  int first_ss_index, int last_ss_index, /* bool negate_ends, */
  KHE_INSTANCE ins)
{
  int i, j, v;  NRC_POLARITY po;  KHE_TIME_GROUP tg;  NRC_WORKER w;
  NRC_SHIFT_SET ss;  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT kc;  NRC_CONSTRAINT c2;
  /* NRC_PENALTY p; */  KHE_RESOURCE_GROUP rg;  bool inverting;

  /* sort out whether we are inverting or not; inverting means to */
  /* convert a constraint with a minimum limit only whose time groups */
  /* are all negative into a constraint with a maximum limit whose time */
  /* groups are all positive */
  inverting = false;  /* not currently doing this */
  /* ***
  inverting = (c->history_before == 0 && c->history_after == 0 &&
    max_limit >= (last_ss_index - first_ss_index + 1) &&
    NrcConstraintTimeGroupsAllNegative(c, first_ss_index, last_ss_index));
  if( inverting )
  {
    max_limit = (last_ss_index - first_ss_index + 1) - min_limit;
    min_limit = 0;
    c->name = HnStringMake(NrcInstanceArena(c->instance),
      "%s (inverted)", c->name);
  }
  *** */

  /* make the constraint and add its time groups (with polarities) */
  tg = (c->starting_ss == NULL || NrcShiftSetShiftCount(c->starting_ss) == 1 ?
	NULL : NrcShiftSetTimeGroup(c->starting_ss, ins));
  /* *** still to do here
  p = NrcPenalty(NrcPenaltyHard(c->penalty), weight,
        NrcPenaltyCostFn(c->penalty));
  p = NULL;
  *** */
  if( !KheClusterBusyTimesConstraintMake(ins, id, c->name,
	NrcPenaltyConvert(p), tg, min_limit, max_limit, allow_zero, &kc) )
    HnAbort("NrcConstraintGenerateCluster internal error 2 (%s)", id);
  for( i = first_ss_index;  i <= last_ss_index;  i++ )
  {
    ss = HaArray(c->shift_sets, i);
    tg = NrcShiftSetTimeGroup(ss, ins);
    po = (inverting ? NRC_POSITIVE : HaArray(c->polarities, i));
    /* ***
    if( negate_ends && (i == first_ss_index || i == last_ss_index) )
      po = NrcPolarityNegate(po);
    *** */
    KheClusterBusyTimesConstraintAddTimeGroup(kc, tg,
      NrcPolarityToKhePolarity(po));
  }

  /* add the resources and resource groups */
  if( NrcConstraintAllResources(c->instance, first_index, last_index, &rg) )
    KheClusterBusyTimesConstraintAddResourceGroup(kc, rg);
  else
  {
    for( i = first_index;  i <= last_index;  i++ )
    {
      c = NrcInstanceConstraint(c->instance, i);
      if( NrcWorkerSetWorkerCount(c->worker_set) == 1 )
      {
	w = NrcWorkerSetWorker(c->worker_set, 0);
	KheClusterBusyTimesConstraintAddResource(kc, NrcWorkerResource(w));
      }
      else
	KheClusterBusyTimesConstraintAddResourceGroup(kc,
	  NrcWorkerSetResourceGroup(c->worker_set, ins));
    }
  }

  /* add history */
  KheClusterBusyTimesConstraintAddHistoryBefore(kc, c->history_before);
  KheClusterBusyTimesConstraintAddHistoryAfter(kc, c->history_after);
  for( i = first_index;  i <= last_index;  i++ )
  {
    c2 = NrcInstanceConstraint(c->instance, i);
    HaArrayForEach(c2->history, v, j) if( v > 0 )
    {
      w = NrcWorkerSetWorker(c2->worker_set, j);
      KheClusterBusyTimesConstraintAddHistory(kc, NrcWorkerResource(w), v);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintGenerateConsecMax(NRC_CONSTRAINT c,                    */
/*    int first_index, int last_index, KHE_INSTANCE ins)                     */
/*                                                                           */
/*  Generate cluster busy times constraints implementing the time windows    */
/*  approach to limiting consecutive busy times, when there is a max limit.  */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
void NrcConstraintGenerateConsecMax(NRC_CONSTRAINT c,
  int name_index, int first_index, int last_index, KHE_INSTANCE ins)
{
  char *id, *name;  int i, max_limit;
  name = NrcConstraintGetId(c, name_index, 0);
  max_limit = NrcLimitMaxLimit(c->limit);
  for( i = 0;  i < HaArrayCount(c->shift_sets) - max_limit;  i++ )
  {
    id = HnStringMake("%s:%d", name, i);
    NrcGenerateCluster Constraint(c, first_index, last_index,
      NrcPenaltyWeight(c->penalty), id, 0, max_limit, false,
      i, i + max_limit, false, ins);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintGenerateConsecMin(NRC_CONSTRAINT c,                    */
/*    int name_index, int first_index, int last_index, KHE_INSTANCE ins)     */
/*                                                                           */
/*  Generate cluster busy times constraints implementing the time windows    */
/*  approach to limiting consecutive busy times, when there is a min limit.  */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
void NrcConstraintGenerateConsecMin(NRC_CONSTRAINT c,
  int name_index, int first_index, int last_index, KHE_INSTANCE ins)
{
  int non_zero_len, start, min_limit;  char *id, *name;
  name = NrcConstraintGetId(c, name_index);
  min_limit = NrcLimitMinLimit(c->limit);
  for( non_zero_len = 1;  non_zero_len < min_limit;  non_zero_len++ )
    for(start=0; start < HaArrayCount(c->shift_sets) - non_zero_len - 1; start++)
    {
      id = HnStringMake("%s:%d:%d", name, non_zero_len, start);
      NrcGenerateCluster Constraint(c, first_index, last_index,
	NrcPenaltyWeight(c->penalty) * (min_limit - non_zero_len), id, 0,
	non_zero_len + 1, false, start, start + non_zero_len + 1, true, ins);
    }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintGenerateCluster(NRC_CONSTRAINT c,                      */
/*    int first_index, int last_index, KHE_INSTANCE ins)                     */
/*                                                                           */
/*  Generate one cluster busy times constraint, for c etc.                   */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintGenerateCluster(NRC_CONSTRAINT c, int name_index,
  int first_index, int last_index, KHE_INSTANCE ins)
{
  char *id;  int i, min_limit, max_limit;  bool allow_zero;  NRC_PENALTY p;
  /* if( NrcConstraintLimits(c, &min_limit, &max_limit, &allow_zero) ) */
  for( i = 0;  i < NrcBoundItemCount(c->bound);  i++ )
  {
    NrcBoundItem(c->bound, i, &min_limit, &allow_zero, &max_limit, &p);
    if( NrcBoundAdjust(c, min_limit, allow_zero, &max_limit) )
    {
      id = NrcConstraintGetId(c, name_index, i);
      NrcGenerateClusterConstraint(c, first_index, last_index,
	p, /* NrcPenaltyWeight(p), */ id, min_limit, max_limit,
	allow_zero, 0, HaArrayCount(c->shift_sets) - 1, /* false, */ ins);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintGenerateLimitBusy(NRC_CONSTRAINT c,                    */
/*    int name_index, int first_index, int last_index, KHE_INSTANCE ins)     */
/*                                                                           */
/*  Generate one limit busy times constraint, for c etc.                     */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintGenerateLimitBusy(NRC_CONSTRAINT c,
  int name_index, int first_index, int last_index, KHE_INSTANCE ins)
{
  char *id;  int i, min_limit, max_limit;  bool allow_zero;  NRC_POLARITY po;
  NRC_SHIFT_SET ss, ss2;  KHE_LIMIT_BUSY_TIMES_CONSTRAINT kc;
  KHE_TIME_GROUP tg;  NRC_WORKER w;  KHE_RESOURCE_GROUP rg;  NRC_PENALTY p;

  /* id and limits */
  /* if( NrcConstraintLimits(c, &min_limit, &max_limit, &allow_zero) ) */
  for( i = 0;  i < NrcBoundItemCount(c->bound);  i++ )
  {
    NrcBoundItem(c->bound, i, &min_limit, &allow_zero, &max_limit, &p);
    if( NrcBoundAdjust(c, min_limit, allow_zero, &max_limit) )
    {
      /* make the constraint and add one time group */
      id = NrcConstraintGetId(c, name_index, i);
      tg = (c->starting_ss==NULL || NrcShiftSetShiftCount(c->starting_ss) == 1 ?
	    NULL : NrcShiftSetTimeGroup(c->starting_ss, ins));
      if( !KheLimitBusyTimesConstraintMake(ins, id, c->name,
	    NrcPenaltyConvert(p), tg, min_limit, max_limit, allow_zero, &kc) )
	HnAbort("NrcConstraintGenerateLimitBusy internal error 2 (%s)", id);
      ss2 = NrcShiftSetMake(c->instance);
      HaArrayForEach(c->shift_sets, ss, i)
      {
	HnAssert(NrcShiftSetShiftCount(ss) == 1,
	  "NrcConstraintGenerateLimitBusy internal error 3");
	po = HaArray(c->polarities, i);
	HnAssert(po == NRC_POSITIVE,
	  "NrcConstraintGenerateLimitBusy internal error 4");
	NrcShiftSetAddShift(ss2, NrcShiftSetShift(ss, 0));
      }
      tg = NrcShiftSetTimeGroup(ss2, ins);
      KheLimitBusyTimesConstraintAddTimeGroup(kc, tg);

      /* add the resources and resource groups */
      if( NrcConstraintAllResources(c->instance, first_index, last_index, &rg) )
	KheLimitBusyTimesConstraintAddResourceGroup(kc, rg);
      else
      {
	for( i = first_index;  i <= last_index;  i++ )
	{
	  c = NrcInstanceConstraint(c->instance, i);
	  if( NrcWorkerSetWorkerCount(c->worker_set) == 1 )
	  {
	    w = NrcWorkerSetWorker(c->worker_set, 0);
	    KheLimitBusyTimesConstraintAddResource(kc, NrcWorkerResource(w));
	  }
	  else
	    KheLimitBusyTimesConstraintAddResourceGroup(kc,
	      NrcWorkerSetResourceGroup(c->worker_set, ins));
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintConvert(NRC_CONSTRAINT c, int name_index,              */
/*    int first_index, int last_index, KHE_INSTANCE ins)                     */
/*                                                                           */
/*  The worker constraints of the instance from index first_index to         */
/*  index last_index inclusive compare equal using NrcConstraintTypedCmp     */
/*  so they can be generated together.  Do this generation.  Constraint c    */
/*  is the first of these constraints, the one at index first_index.         */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintConvert(NRC_CONSTRAINT c, int name_index,
  int first_index, int last_index, KHE_INSTANCE ins)
{
  int i, min_value;  bool allow_zero;  NRC_PENALTY below_min_penalty;
  NRC_CONSTRAINT ci;
  HnAssert(first_index <= last_index, "NrcConstraintConvert internal error");
  if( DEBUG1 && first_index < last_index )
  {
    NRC_CONSTRAINT ci;  int i;
    fprintf(stderr, "generating %s, the merge of %d constraints:\n",
      NrcConstraintGetId(c, name_index, 0), last_index - first_index + 1);
    for( i = first_index;  i <= last_index;  i++ )
    {
      ci = NrcInstanceConstraint(c->instance, i);
      NrcConstraintDebug(ci, 2, stderr);
    }
  }

  /* make sure c->history_after comes from a min constraint, if any */
  for( i = first_index;  i <= last_index;  i++ )
  {
    ci = NrcInstanceConstraint(c->instance, i);
    if( NrcBoundMin(ci->bound, &min_value, &allow_zero, &below_min_penalty) )
    {
      c->history_after = ci->history_after;
      break;
    }
  }

  switch( c->type )
  {
    case NRC_CONSTRAINT_ACTIVE:

      if( NrcConstraintUseLimitBusy(c) )
	NrcConstraintGenerateLimitBusy(c, name_index, first_index,
	  last_index, ins);
      else
	NrcConstraintGenerateCluster(c, name_index, first_index,
	  last_index, ins);
      break;

    case NRC_CONSTRAINT_CONSECUTIVE:

      NrcConstraintGenerateLimitActive(c, name_index, first_index,
	last_index, ins);
      break;

    case NRC_CONSTRAINT_WORKLOAD:

      NrcConstraintGenerateLimitWorkload(c, name_index, first_index,
	last_index, ins);
      break;

    default:

      HnAbort("NrcConstraintConvert: invalid type (%d)", c->type);
  }
  /* ***
  if( NrcConstraintLimitsWorkload(c) )
  else if( c->type == NRC_CONSTRAINT_CONSECUTIVE )
  else if( NrcConstraintIsConsecutive(c) )
  {
    if( use_limit_active )
    else if( NrcLimitType(c->limit) == NRC_LIMIT_MAX_CONSECUTIVE )
      NrcConstraintGenerateConsecMax(c,name_index,first_index,last_index,ins);
    else if( NrcLimitType(c->limit) == NRC_LIMIT_MIN_CONSECUTIVE )
      NrcConstraintGenerateConsecMin(c,name_index,first_index,last_index,ins);
    else
      HnAbort("NrcConstraintConvert: NRC_LIMIT_MIN_AND_MAX_CONSECUTIVE without"
	" use_limit_active");
  }
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *NrcConstraintTypeShow(NRC_CONSTRAINT_TYPE type)                    */
/*                                                                           */
/*  Show type.                                                               */
/*                                                                           */
/*****************************************************************************/

static char *NrcConstraintTypeShow(NRC_CONSTRAINT_TYPE type)
{
  switch( type )
  {
    case NRC_CONSTRAINT_ACTIVE:		return "active";
    case NRC_CONSTRAINT_CONSECUTIVE:	return "consec";
    case NRC_CONSTRAINT_WORKLOAD:	return "workload";

    default:
      HnAbort("NrcConstraintTypeShow: unknown type (%d)", type);
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcUniformShow(NRC_UNIFORMITY u)                                   */
/*                                                                           */
/*  Show u.                                                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
char *NrcUniformityShow(NRC_UNIFORMITY u)
{
  switch( u )
  {
    case NRC_UNIFORMITY_UNKNOWN:	return "unknown";
    case NRC_UNIFORMITY_NONE:		return "none";
    case NRC_UNIFORMITY_NOT_LR:		return "not_lr";
    case NRC_UNIFORMITY_NOT_R:		return "not_r";
    case NRC_UNIFORMITY_FULL:		return "full";

    default:
      HnAbort("NrcUniformityShow internal error");
      return NULL;  ** keep compiler happy **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void NrcConstraintDebug(NRC_CONSTRAINT c, int indent, FILE *fp)          */
/*                                                                           */
/*  Debug print of constraint c onto fp with the given indent.               */
/*                                                                           */
/*****************************************************************************/

void NrcConstraintDebug(NRC_CONSTRAINT c, int indent, FILE *fp)
{
  NRC_SHIFT_SET	ss;  NRC_POLARITY po;  int i, v;  NRC_WORKER w;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ Constraint(%s, %s, %s, %s)\n", indent, "", c->name,
      NrcWorkerSetName(c->worker_set), NrcConstraintTypeShow(c->type),
      NrcBoundShow(c->bound));
    /* ***
    if( c->regularity != NULL )
      NrcConstraintRegularityDebug(c->regularity, indent + 2, fp);
    *** */
    if( c->starting_ss != NULL )
      NrcShiftSetDebug(c->starting_ss, indent + 2, fp);
    fprintf(fp, "%*s  ----------------------------------\n", indent, "");
    for( i = 0;  i < HaArrayCount(c->shift_sets);  i++ )
    {
      ss = HaArray(c->shift_sets, i);
      po = HaArray(c->polarities, i);
      NrcShiftSetAndPolarityDebug(ss, po, indent + 2, fp);
    }
    fprintf(fp, "%*s  ----------------------------------\n", indent, "");
    if( c->history_before != 0 || c->history_after != 0 )
    {
      fprintf(fp, "%*s  history (before %d, after %d):\n", indent, "",
        c->history_before, c->history_after);
      HaArrayForEach(c->history, v, i) if( v > 0 )
      {
	w = NrcWorkerSetWorker(c->worker_set, i);
	fprintf(fp, "%*s    %s: %d\n", indent, "", NrcWorkerConvertedName(w),v);
      }
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "%s", c->name);
}

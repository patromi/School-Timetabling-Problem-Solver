
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
/*  FILE:         nrc_demand.c                                               */
/*  MODULE:       Demands                                                    */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0


/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_PENALTY                                                       */
/*                                                                           */
/*  The penalty for assigning a given worker (or non-assignment)             */
/*                                                                           */
/*****************************************************************************/

typedef struct nrc_worker_penalty_rec {
  NRC_WORKER			worker;			/* the worker        */
  NRC_PENALTY			penalty;		/* the penalty       */
} *NRC_WORKER_PENALTY;

typedef HA_ARRAY(NRC_WORKER_PENALTY) ARRAY_NRC_WORKER_PENALTY;


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALIZER                                                             */
/*                                                                           */
/*  A record of one call saying how assignments to a demand should be        */
/*  penalized.                                                               */
/*                                                                           */
/*****************************************************************************/

typedef struct nrc_penalizer_rec {
  NRC_PENALIZER_TYPE		type;			/* penalizer type    */
  NRC_WORKER_SET		worker_set;		/* worker set        */
  NRC_PENALTY_TYPE		penalty_type;		/* penalty type      */
  NRC_PENALTY			penalty;		/* penalty           */
} *NRC_PENALIZER;

typedef HA_ARRAY(NRC_PENALIZER) ARRAY_NRC_PENALIZER;


/*****************************************************************************/
/*                                                                           */
/*  NRC_DEMAND - a demand for one worker                                     */
/*                                                                           */
/*****************************************************************************/

struct nrc_demand_rec {
  NRC_INSTANCE			instance;
  char				*name;
  ARRAY_NRC_PENALIZER		penalizers;
  NRC_DEMAND			forward;
  NRC_SHIFT			prev_shift;
  int				prev_shift_index;
  int				max_shift_index;
  NRC_SHIFT_SET			shift_set;
  ARRAY_NRC_WORKER_PENALTY	worker_penalties;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "worker penalties (private)"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_WORKER_PENALTY NrcWorkerPenaltyMake(NRC_WORKER w)                    */
/*                                                                           */
/*  Make a new worker penalty object, with initial penalty zero.             */
/*                                                                           */
/*****************************************************************************/

static NRC_WORKER_PENALTY NrcWorkerPenaltyMake(NRC_WORKER w, NRC_INSTANCE ins)
{
  NRC_WORKER_PENALTY res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->worker = w;
  res->penalty = NrcInstanceZeroPenalty(ins);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY NrcPenaltyCombine(NRC_PENALTY old_p, NRC_PENALTY new_p,      */
/*    NRC_PENALTY_TYPE new_pt, NRC_INSTANCE ins)                             */
/*                                                                           */
/*  Combine old_p and new_p according to new_pt.                             */
/*                                                                           */
/*****************************************************************************/

static NRC_PENALTY NrcPenaltyCombine(NRC_PENALTY old_p, NRC_PENALTY new_p,
  NRC_PENALTY_TYPE new_pt, NRC_INSTANCE ins)
{
  switch( new_pt )
  {
    case NRC_PENALTY_REPLACE:

      return new_p;

    case NRC_PENALTY_ADD:

      return NrcPenaltyAdd(old_p, new_p, ins);

    case NRC_PENALTY_UNIQUE:

      HnAssert(NrcPenaltyWeight(old_p) == 0,
	"NrcPenaltyCombine: unique penalty is replacement");
      return new_p;

    default:

      HnAbort("NrcPenaltyCombine internal error");
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void WorkerPenaltyAddPenalizer(NRC_WORKER_PENALTY wp, NRC_PENALIZER pr)  */
/*                                                                           */
/*  Add pr to wp.                                                            */
/*                                                                           */
/*****************************************************************************/

static void WorkerPenaltyAddPenalizer(NRC_WORKER_PENALTY wp, NRC_PENALIZER pr,
  NRC_INSTANCE ins)
{
  wp->penalty = NrcPenaltyCombine(wp->penalty, pr->penalty,
    pr->penalty_type, ins);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcWorkerPenaltyTypedCmp(NRC_WORKER_PENALTY wp1,                     */
/*    NRC_WORKER_PENALTY wp2)                                                */
/*                                                                           */
/*  Comparison function for bringing worker penalties with equal             */
/*  penalties together, except that non-assignment is always separate.       */
/*                                                                           */
/*****************************************************************************/

static int NrcWorkerPenaltyTypedCmp(NRC_WORKER_PENALTY wp1,
  NRC_WORKER_PENALTY wp2)
{
  int cmp;
  cmp = (int) (wp1->worker != NULL) - (int) (wp2->worker != NULL);
  if( cmp != 0 )  return cmp;
  return NrcPenaltyTypedCmp(wp1->penalty, wp2->penalty);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcWorkerPenaltyCmp(const void *p1, const void *p2)                  */
/*                                                                           */
/*  Untyped version of NrcWorkerPenaltyTypedCmp.                             */
/*                                                                           */
/*****************************************************************************/

static int NrcWorkerPenaltyCmp(const void *p1, const void *p2)
{
  NRC_WORKER_PENALTY wp1 = * (NRC_WORKER_PENALTY *) p1;
  NRC_WORKER_PENALTY wp2 = * (NRC_WORKER_PENALTY *) p2;
  return NrcWorkerPenaltyTypedCmp(wp1, wp2);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcWorkerPenaltyDebug(NRC_WORKER_PENALTY wp, int indent, FILE *fp)  */
/*                                                                           */
/*  Debug print of wp onto fp with the given indent.                         */
/*                                                                           */
/*****************************************************************************/

static void NrcWorkerPenaltyDebug(NRC_WORKER_PENALTY wp, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "(%s, %c%d)",
    wp->worker == NULL ? "-" : NrcWorkerName(wp->worker),
    NrcPenaltyHard(wp->penalty) ? 'h' : 's', NrcPenaltyWeight(wp->penalty));
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "penalizers"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALIZER NrcPenalizerMake(NRC_DEMAND d, NRC_PENALIZER_TYPE pt,      */
/*    NRC_WORKER_SET ws, NRC_PENALTY_TYPE ptype, NRC_PENALTY p)              */
/*                                                                           */
/*  Make a new penalizer object with these attributes.                       */
/*                                                                           */
/*****************************************************************************/

static NRC_PENALIZER NrcPenalizerMake(NRC_DEMAND d, NRC_PENALIZER_TYPE pt,
  NRC_WORKER_SET ws, NRC_PENALTY_TYPE ptype, NRC_PENALTY p)
{
  NRC_PENALIZER res;
  HaMake(res, NrcInstanceArena(d->instance));
  res->type = pt;
  res->worker_set = ws;
  res->penalty_type = ptype;
  res->penalty = p;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandAddPenalizer(NRC_DEMAND d, NRC_PENALIZER_TYPE pt,          */
/*    NRC_WORKER_SET ws, NRC_PENALTY_TYPE ptype, NRC_PENALTY p)              */
/*                                                                           */
/*  Add a penalizer with these attributes to d.                              */
/*                                                                           */
/*****************************************************************************/

static void NrcDemandAddPenalizer(NRC_DEMAND d, NRC_PENALIZER_TYPE pt,
  NRC_WORKER_SET ws, NRC_PENALTY_TYPE ptype, NRC_PENALTY p)
{
  NRC_PENALIZER pr;
  pr = NrcPenalizerMake(d, pt, ws, ptype, p);
  HaArrayAddLast(d->penalizers, pr);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcPenalizerDebug(NRC_PENALIZER pr, int indent, FILE *fp)           */
/*                                                                           */
/*  Debug print of pr onto fp with the given indent.                         */
/*                                                                           */
/*****************************************************************************/

static void NrcPenalizerDebug(NRC_PENALIZER pr, int indent, FILE *fp)
{
  char *name;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  switch( pr->type )
  {
    case NRC_PENALIZER_NON_ASSIGNMENT:

      fprintf(fp, "Penalize(non-assignment, %s)", NrcPenaltyShow(pr->penalty));
      break;

    case NRC_PENALIZER_WORKER_SET:

      name = NrcWorkerSetName(pr->worker_set);
      fprintf(fp, "Penalize(workers from %s, %s)", name,
	NrcPenaltyShow(pr->penalty));
      break;

    case NRC_PENALIZER_NOT_WORKER_SET:

      name = NrcWorkerSetName(pr->worker_set);
      fprintf(fp, "Penalize(workers not from %s, %s)", name,
	NrcPenaltyShow(pr->penalty));
      break;

    default:

      HnAbort("NrcPenalizerDebug: unknown penalizer type %d", pr->type);
  }
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_DEMAND NrcDemandMakeBegin(NRC_INSTANCE ins)                          */
/*                                                                           */
/*  Begin the construction of a demand object.                               */
/*                                                                           */
/*****************************************************************************/

NRC_DEMAND NrcDemandMakeBegin(NRC_INSTANCE ins)
{
  NRC_DEMAND res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->instance = ins;
  res->name = NULL;
  HaArrayInit(res->penalizers, a);
  res->forward = NULL;
  res->prev_shift = NULL;
  res->prev_shift_index = 0;
  res->max_shift_index = 0;
  res->shift_set = NULL;
  HaArrayInit(res->worker_penalties, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcDemandMakeName(NRC_DEMAND d)                                    */
/*                                                                           */
/*  Make and return a name for d based on d's penalizers.  The labels are:   */
/*                                                                           */
/*    X    Demand has no penalizers at all                                   */
/*    NA   Penalizer for non-assignment                                      */
/*    A    Penalizer for assignment of any worker at all                     */
/*    W    Penalizer for assignment of worker from a given worker set        */
/*    NW   Penalizer for assignment of worker not from a given worker set    */
/*                                                                           */
/*  Implementation note.  Previously we were making the name depend on       */
/*  the contents of the worker sets passed.  But that is erroneous if the    */
/*  caller chooses to pass the worker sets before adding their workers to    */
/*  them.  So now the name depends only on the names of the worker sets.     */
/*                                                                           */
/*****************************************************************************/

static char *NrcDemandMakeName(NRC_DEMAND d)
{
  HA_ARRAY_NCHAR anc;  HA_ARENA a;  NRC_PENALIZER pr;  int i, items;
  char *name;  NRC_PENALTY p;
  if( HaArrayCount(d->penalizers) == 0 )
    return "X";
  else
  {
    /* get things started */
    a = NrcInstanceArena(d->instance);
    HnStringBegin(anc, a);
    items = 0;

    /* do non-assignment (it's independent of the res); combine penalizers */
    p = NrcInstanceZeroPenalty(d->instance);
    HaArrayForEach(d->penalizers, pr, i)
      if( pr->type == NRC_PENALIZER_NON_ASSIGNMENT )
	p = NrcPenaltyCombine(p, pr->penalty, pr->penalty_type, d->instance);
    if( NrcPenaltyWeight(p) > 0 )
    {
      HnStringAdd(&anc, "NA=%s", NrcPenaltyShow(p));
      items++;
    }

    /* do the rest */
    HaArrayForEach(d->penalizers, pr, i)
    {
      switch( pr->type )
      {
	case NRC_PENALIZER_NON_ASSIGNMENT:

	  /* already done */
	  break;

	case NRC_PENALIZER_WORKER_SET:

	  if( items > 0 )
	    HnStringAdd(&anc, pr->penalty_type == NRC_PENALTY_ADD ? "+" : "|");
	  if( pr->worker_set == NrcInstanceStaffing(d->instance) )
	    HnStringAdd(&anc, "A=%s", NrcPenaltyShow(pr->penalty));
	  else
	  {
	    name = NrcWorkerSetName(pr->worker_set);
	    if( strstr(name, "Skill-") == name )
	      name = &name[strlen("Skill-")];
	    HnStringAdd(&anc, "W%s=%s", name, NrcPenaltyShow(pr->penalty));
	  }
	  items++;
	  break;

	case NRC_PENALIZER_NOT_WORKER_SET:

	  if( items > 0 )
	    HnStringAdd(&anc, pr->penalty_type == NRC_PENALTY_ADD ? "+" : "|");
	  if( pr->worker_set == NrcInstanceEmptyWorkerSet(d->instance) )
	    HnStringAdd(&anc, "A=%s", NrcPenaltyShow(pr->penalty));
	  else
	  {
	    name = NrcWorkerSetName(pr->worker_set);
	    if( strstr(name, "Skill-") == name )
	      name = &name[strlen("Skill-")];
	    HnStringAdd(&anc, "NW%s=%s", name, NrcPenaltyShow(pr->penalty));
	  }
	  items++;
	  break;

	default:

	  HnAbort("NrcDemandMakeName: unknown penalizer type %d", pr->type);
      }
    }
    return HnStringEnd(anc);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandBuildWorkerPenalties(NRC_DEMAND d)                         */
/*                                                                           */
/*  Build (but do not sort) d's worker penalties array.                      */
/*                                                                           */
/*****************************************************************************/

static void NrcDemandBuildWorkerPenalties(NRC_DEMAND d)
{
  NRC_WORKER w;  NRC_PENALIZER pr;  NRC_WORKER_PENALTY wp;  int i, j;

  /* build the array of worker penalties, including non-assignment */
  if( DEBUG5 )
    fprintf(stderr, "[ NrcDemandBuildWorkerPenalties(%s)\n", d->name);
  for( i = 0;  i < NrcInstanceStaffingWorkerCount(d->instance);  i++ )
  {
    w = NrcInstanceStaffingWorker(d->instance, i);
    HaArrayAddLast(d->worker_penalties, NrcWorkerPenaltyMake(w, d->instance));
  }
  HaArrayAddLast(d->worker_penalties, NrcWorkerPenaltyMake(NULL, d->instance));

  /* add in all the penalties from all the penalizers */
  HaArrayForEach(d->penalizers, pr, i)
  {
    if( DEBUG5 )
    {
      fprintf(stderr, "  adding penalizer ");
      NrcPenalizerDebug(pr, 0, stderr);
    }
    switch( pr->type )
    {
      case NRC_PENALIZER_NON_ASSIGNMENT:

	wp = HaArrayLast(d->worker_penalties);
	HnAssert(wp->worker == NULL,
	  "NrcDemandBuildWorkerPenalties internal error 1");
	WorkerPenaltyAddPenalizer(wp, pr, d->instance);
	break;

      case NRC_PENALIZER_WORKER_SET:

	for( j = 0;  j < NrcWorkerSetWorkerCount(pr->worker_set);  j++ )
	{
	  w = NrcWorkerSetWorker(pr->worker_set, j);
	  wp = HaArray(d->worker_penalties, NrcWorkerIndex(w));
	  WorkerPenaltyAddPenalizer(wp, pr, d->instance);
	}
	break;

      case NRC_PENALIZER_NOT_WORKER_SET:

	for( j = 0;  j < NrcInstanceStaffingWorkerCount(d->instance);  j++ )
	{
	  w = NrcInstanceStaffingWorker(d->instance, j);
	  if( !NrcWorkerSetContainsWorker(pr->worker_set, w) )
	  {
	    wp = HaArray(d->worker_penalties, NrcWorkerIndex(w));
	    WorkerPenaltyAddPenalizer(wp, pr, d->instance);
	  }
	}
	break;

      default:

	HnAbort("NrcDemandBuildWorkerPenalties internal error 2");
	break;
    }
  }
  if( DEBUG5 )
    fprintf(stderr, "] NrcDemandBuildWorkerPenalties returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandMakeEnd(NRC_DEMAND d)                                      */
/*                                                                           */
/*  End making demand d, including building its worker penalties array,      */
/*  making a name for it, and adding it to ins if it's new.                  */
/*                                                                           */
/*****************************************************************************/

void NrcDemandMakeEnd(NRC_DEMAND d)
{
  NRC_DEMAND res;
  HnAssert(d->forward == NULL, "NrcDemandMakeEnd called out of order");
  HnAssert(d->name == NULL, "NrcDemandMakeEnd internal error");
  d->name = NrcDemandMakeName(d);
  if( NrcInstanceRetrieveDemand(d->instance, d->name, &res) )
    d->forward = res;
  else
  {
    d->forward = d;
    NrcDemandBuildWorkerPenalties(d);
    NrcInstanceAddDemand(d->instance, d->name, d);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandPenalizeNonAssignment(NRC_DEMAND d,                        */
/*    NRC_PENALTY_TYPE ptype, NRC_PENALTY p)                                 */
/*                                                                           */
/*  Add a penalty for non-assignment to d.                                   */
/*                                                                           */
/*****************************************************************************/

void NrcDemandPenalizeNonAssignment(NRC_DEMAND d,
  NRC_PENALTY_TYPE ptype, NRC_PENALTY p)
{
  HnAssert(d->forward == NULL,
    "NrcDemandPenalizeNonAssignment called out of order");
  HnAssert(NrcPenaltyCostFn(p) == NRC_COST_FUNCTION_LINEAR,
    "NrcDemandPenalizeNonAssignment: penalty cost function not linear");
  NrcDemandAddPenalizer(d, NRC_PENALIZER_NON_ASSIGNMENT, NULL, ptype, p);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandPenalizeWorkerSet(NRC_DEMAND d, NRC_WORKER_SET ws,         */
/*    NRC_PENALTY_TYPE ptype, NRC_PENALTY p)                                 */
/*                                                                           */
/*  Add a penalty for assigning a worker from ws to d.  Ignore the request   */
/*  when ws is the fixed empty worker set, since it changes nothing.         */
/*                                                                           */
/*****************************************************************************/

void NrcDemandPenalizeWorkerSet(NRC_DEMAND d, NRC_WORKER_SET ws,
  NRC_PENALTY_TYPE ptype, NRC_PENALTY p)
{
  HnAssert(d->forward == NULL,
    "NrcDemandPenalizeWorkerSet called out of order");
  HnAssert(NrcPenaltyCostFn(p) == NRC_COST_FUNCTION_LINEAR,
    "NrcDemandPenalizeWorkerSet: penalty cost function not linear");
  if( ws != NrcInstanceEmptyWorkerSet(d->instance) )
    NrcDemandAddPenalizer(d, NRC_PENALIZER_WORKER_SET, ws, ptype, p);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandPenalizeNotWorkerSet(NRC_DEMAND d, NRC_WORKER_SET ws,      */
/*    NRC_PENALTY_TYPE ptype, NRC_PENALTY p)                                 */
/*                                                                           */
/*  Add a penalty for not assigning a worker from ws to d.  Ignore the       */
/*  request when ws contains all workers, since it changes nothing.          */
/*                                                                           */
/*  If the name of ws indicates that it is the complement of some other      */
/*  worker set, change the call to penalizing the complement, to remove      */
/*  the double negative.                                                     */
/*                                                                           */
/*****************************************************************************/

void NrcDemandPenalizeNotWorkerSet(NRC_DEMAND d, NRC_WORKER_SET ws,
  NRC_PENALTY_TYPE ptype, NRC_PENALTY p)
{
  char *name;
  HnAssert(d->forward == NULL,
    "NrcDemandPenalizeNotWorkerSet called out of order");
  HnAssert(NrcPenaltyCostFn(p) == NRC_COST_FUNCTION_LINEAR,
    "NrcDemandPenalizeNotWorkerSet: penalty cost function not linear");
  if( ws != NrcInstanceStaffing(d->instance) )
  {
    name = NrcWorkerSetName(ws);
    if( name[0] == '!' )
    {
      if( !NrcInstanceRetrieveWorkerSet(d->instance, &name[1], &ws) )
	HnAbort("NrcDemandPenalizeNotWorkerSet internal error");
      NrcDemandAddPenalizer(d, NRC_PENALIZER_WORKER_SET, ws, ptype, p);
    }
    else
      NrcDemandAddPenalizer(d, NRC_PENALIZER_NOT_WORKER_SET, ws, ptype, p);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "query"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *NrcDemandName(NRC_DEMAND d)                                        */
/*                                                                           */
/*  Return the name of d, but only after NrcDemandMakeEnd(d).                */
/*                                                                           */
/*****************************************************************************/

char *NrcDemandName(NRC_DEMAND d)
{
  HnAssert(d->name != NULL, "NrcDemandName called before NrcDemandMakeEnd");
  return d->name;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE NrcDemandInstance(NRC_DEMAND d)                             */
/*                                                                           */
/*  Return the instance attribute of d.                                      */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE NrcDemandInstance(NRC_DEMAND d)
{
  return d->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcDemandPenalizerCount(NRC_DEMAND d)                                */
/*                                                                           */
/*  Return the number of preferences of d.                                    */
/*                                                                           */
/*****************************************************************************/

int NrcDemandPenalizerCount(NRC_DEMAND d)
{
  return HaArrayCount(d->penalizers);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandPenalizer(NRC_DEMAND d, int i, NRC_PENALIZER_TYPE *pt,     */
/*    NRC_WORKER_SET *ws, NRC_PENALTY_TYPE *ptype, NRC_PENALTY *p)           */
/*                                                                           */
/*  Return the fields of the i'th penalizer of d.                            */
/*                                                                           */
/*****************************************************************************/

void NrcDemandPenalizer(NRC_DEMAND d, int i, NRC_PENALIZER_TYPE *pt,
  NRC_WORKER_SET *ws, NRC_PENALTY_TYPE *ptype, NRC_PENALTY *p)
{
  NRC_PENALIZER pr;
  pr = HaArray(d->penalizers, i);
  *pt = pr->type;
  *ws = pr->worker_set;
  *ptype = pr->penalty_type;
  *p = pr->penalty;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_PENALTY NrcDemandWorkerPenalty(NRC_DEMAND d, NRC_WORKER w)           */
/*                                                                           */
/*  Return the penalty for worker w, or for non-assignment if w is NULL.     */
/*  This can only be called after NrcDemandMakeEnd and before conversion.    */
/*                                                                           */
/*****************************************************************************/

NRC_PENALTY NrcDemandWorkerPenalty(NRC_DEMAND d, NRC_WORKER w)
{
  int i;  NRC_WORKER_PENALTY wp;
  HnAssert(d->forward != NULL, "NrcDemandWorkerPenalty: called out of order");
  d = d->forward;
  i = (w == NULL ? HaArrayCount(d->worker_penalties) - 1 : NrcWorkerIndex(w));
  wp = HaArray(d->worker_penalties, i);
  HnAssert(wp->worker == w, "NrcDemandWorkerPenalty: called out of order (2)");
  return wp->penalty;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "conversion to XESTT"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *NrcDemandRoleMake(NRC_DEMAND d, int index)                         */
/*                                                                           */
/*  Make a role name for demand d with index.                                */
/*                                                                           */
/*****************************************************************************/

static char *NrcDemandRoleMake(NRC_DEMAND d, int index)
{
  HA_ARENA a = NrcInstanceArena(d->instance);
  return HnStringMake(a, "%s:%d", d->name, index);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandConvertToEventResource(NRC_DEMAND d, NRC_SHIFT s,          */
/*    NRC_WORKER preasst, KHE_EVENT e)                                       */
/*                                                                           */
/*  Convert d into an event resource for s, whose KHE event is e.            */
/*                                                                           */
/*  Implementation note.  This function assumes that all calls for a         */
/*  given shift s occur together, not separated by calls for other           */
/*  shifts, as is indeed the case.                                           */
/*                                                                           */
/*****************************************************************************/

void NrcDemandConvertToEventResource(NRC_DEMAND d, NRC_SHIFT s,
  NRC_WORKER preasst, KHE_EVENT e)
{
  KHE_RESOURCE_TYPE rt;  KHE_EVENT_RESOURCE er;  char *role;
  KHE_RESOURCE r;

  /* delegate if required (possibly to self) */
  d = d->forward;

  /* sort out prev_shift, prev_shift_index, and max_shift_index */
  if( d->prev_shift == s )
    d->prev_shift_index++;
  else
  {
    d->prev_shift = s;
    d->prev_shift_index = 1;
    if( d->shift_set == NULL )
      d->shift_set = NrcShiftSetMake(d->instance);
    NrcShiftSetAddShift(d->shift_set, s);
  }
  if( d->max_shift_index < d->prev_shift_index )
    d->max_shift_index = d->prev_shift_index;

  /* generate the event resource */
  rt = KheInstanceResourceType(KheEventInstance(e), 0);
  r = (preasst == NULL ? NULL : NrcWorkerResource(preasst));
  role = NrcDemandRoleMake(d, d->prev_shift_index);
  if( !KheEventResourceMake(e, rt, r, role, KheEventWorkload(e), &er) )
    HnAbort("NrcDemandConvertToEventResource internal error: %s", role);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcSinglePreferredWorkerSet(NRC_DEMAND d, NRC_PENALIZER *pr)        */
/*                                                                           */
/*  If d contains a single penalize not worker set and no penalize worker    */
/*  set penalizers, return true and set *pr to the penalize not worker set   */
/*  penalizer.  Otherwise return false with *pr set, but to an unspecified   */
/*  value.  A single preferred workser set can be the basis of a concise     */
/*  prefer resources constraint.                                             */
/*                                                                           */
/*****************************************************************************/

static bool NrcSinglePreferredWorkerSet(NRC_DEMAND d, NRC_PENALIZER *pr)
{
  int i, counts[3] = {0, 0, 0};  NRC_PENALIZER pr2;  bool res;
  *pr = NULL;
  HaArrayForEach(d->penalizers, pr2, i)
  {
    counts[(int) pr2->type] += 1;
    if( pr2->type == NRC_PENALIZER_NOT_WORKER_SET )
      *pr = pr2;
  }
  res = counts[(int) NRC_PENALIZER_WORKER_SET] == 0 &&
    counts[(int) NRC_PENALIZER_NOT_WORKER_SET] == 1;
  if( DEBUG4 )
    fprintf(stderr, "  %s counts = {%d, %d, %d} returning %s\n",
      d->name, counts[0], counts[1], counts[2], res ? "true" : "false");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandConvertToEventResourceConstraints(NRC_DEMAND d,            */
/*    KHE_INSTANCE ins)                                                      */
/*                                                                           */
/*  Convert d into all corresponding event resources constraints.  This      */
/*  is called (by NrcInstanceConvertDemandsToEventResourceConstraints)       */
/*  once for each unforwarded demand object.                                 */
/*                                                                           */
/*****************************************************************************/

void NrcDemandConvertToEventResourceConstraints(NRC_DEMAND d, KHE_INSTANCE ins)
{
  NRC_PENALIZER pr;  KHE_RESOURCE r, r2;  KHE_RESOURCE_GROUP rg;
  NRC_WORKER_PENALTY wp, wp2;  char *role, *id, *name;  HA_ARENA a;
  KHE_ASSIGN_RESOURCE_CONSTRAINT karc;  KHE_PREFER_RESOURCES_CONSTRAINT kprc;
  KHE_EVENT e;  KHE_EVENT_GROUP eg;  int i, j, k, index;

  if( DEBUG4 )
    fprintf(stderr, "[ NrcDemandConvertTo...(%s, %s)\n",
      d->name, KheInstanceId(ins));

  /* no need to delegate, given where this function is called from */
  HnAssert(d == d->forward,
    "NrcDemandConvertToEventResourceConstraints internal error");

  /* if d applies to no shifts, do nothing */
  if( d->shift_set == NULL )
    return;

  /* get the event group containing the shifts that contain d */
  a = NrcInstanceArena(d->instance);
  id = HnStringMake(a, "EG:%s", d->name);
  eg = NrcShiftSetEventGroup(d->shift_set, id, ins);

  /* sort the worker penalties to bring equal penalties together */
  HaArraySort(d->worker_penalties, &NrcWorkerPenaltyCmp);

  /* handle the worker penalties, starting by grouping them by equal penalty */
  for( i = 0;  i < HaArrayCount(d->worker_penalties);  i = j )
  {
    /* build a group of worker penalties with the same penalty as wp */
    wp = HaArray(d->worker_penalties, i);
    for( j = i + 1;  j < HaArrayCount(d->worker_penalties);  j++ )
    {
      wp2 = HaArray(d->worker_penalties, j);
      if( NrcWorkerPenaltyTypedCmp(wp, wp2) != 0 )
	break;
    }

    /* here d->worker_penalties[i .. j-1] form one non-zero group */
    if( DEBUG4 )
    {
      fprintf(stderr, "  [ group %d .. %d (penalty %s):\n", i, j - 1,
	NrcPenaltyShow(wp->penalty));
      for( k = i;  k < j;  k++ )
      {
	wp2 = HaArray(d->worker_penalties, k);
	NrcWorkerPenaltyDebug(wp2, 4, stderr);
      }
      fprintf(stderr, "  ]\n");
    }

    /* ignore this group if its penalty weight is 0 */
    if( NrcPenaltyWeight(wp->penalty) == 0 )
      continue;

    /* one constraint for each role that d could take in any shift */
    for( index = 1;  index <= d->max_shift_index;  index++ )
    {
      role = NrcDemandRoleMake(d, index);
      if( wp->worker == NULL )
      {
	/* penalize non-assignment using an XESTT assign resource constraint */
	id = HnStringMake(a, "ARC:%s", role);
	name = "Assign resource";
	if( !KheAssignResourceConstraintMake(ins, id, name,
	    NrcPenaltyConvert(wp->penalty), role, &karc) )
	  HnAbort("cannot add %s constraint", id);
	KheAssignResourceConstraintAddEventGroup(karc, eg);
      }
      /* else if( i > 0 || j < HaArrayCount(d->worker_penalties) - 1 ) */
      else if( NrcSinglePreferredWorkerSet(d, &pr) )
      {
	/* single preferred worker set, so single prefer resources constraint */
	rg = NrcWorkerSetResourceGroup(pr->worker_set, ins);
	id = HnStringMake(a, "PRC:%s:%s", role, KheResourceGroupId(rg));
	name = HnStringMake(a, "Prefer %s", KheResourceGroupId(rg));
	if( !KhePreferResourcesConstraintMake(ins, id, name,
	    NrcPenaltyConvert(pr->penalty), role, &kprc) )
	  HnAbort("cannot add %s constraint", id);
	if( !KhePreferResourcesConstraintAddResourceGroup(kprc, rg) )
	  HnAbort("cannot add resource group %s to %s constraint",
            KheResourceGroupId(rg), id);
	if( !KhePreferResourcesConstraintAddEventGroup(kprc, eg, &e) )
	  HnAbort("cannot add eg to %s constraint (problem is %s)",
	    id, KheEventId(e));
      }
      else
      {
	/* penalize unpreferred using an XESTT prefer resources constraint */
	r = NrcWorkerResource(wp->worker);
	id = HnStringMake(a, "PRC:%s:Not%s%s", role, KheResourceId(r),
	  i+1 == j ? "" : "Etc");
	name = HnStringMake(a, "Prefer not %s%s", KheResourceId(r),
	  i+1 == j ? "" : " (etc.)");
	if( !KhePreferResourcesConstraintMake(ins, id, name,
	    NrcPenaltyConvert(wp->penalty), role, &kprc) )
	  HnAbort("cannot add %s constraint", id);
	for( k = 0;  k < i;  k++ )
	{
	  wp2 = HaArray(d->worker_penalties, k);
	  if( wp2->worker != NULL )
	  {
	    r2 = NrcWorkerResource(wp2->worker);
	    if( !KhePreferResourcesConstraintAddResource(kprc, r2) )
	      HnAbort("cannot add resource %s to %s constraint",
		KheResourceId(r2), id);
	  }
	}
	for( k = j;  k < HaArrayCount(d->worker_penalties) - 1;  k++ )
	{
	  wp2 = HaArray(d->worker_penalties, k);
	  if( wp2->worker != NULL )
	  {
	    r2 = NrcWorkerResource(wp2->worker);
	    if( !KhePreferResourcesConstraintAddResource(kprc, r2) )
	      HnAbort("cannot add resource %s to %s constraint",
		KheResourceId(r2), id);
	  }
	}
	if( !KhePreferResourcesConstraintAddEventGroup(kprc, eg, &e) )
	  HnAbort("cannot add eg to %s constraint (problem is %s)",
	    id, KheEventId(e));
      }
    }
  }
  if( DEBUG4 )
    fprintf(stderr, "] NrcDemandConvertTo... returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcDemandDebug(NRC_DEMAND d, int indent, FILE *fp)                  */
/*                                                                           */
/*  Debug print of d onto fp.                                                */
/*                                                                           */
/*****************************************************************************/

void NrcDemandDebug(NRC_DEMAND d, int multiplicity, int indent, FILE *fp)
{
  NRC_PENALIZER pr;  int i;  char buff[20];
  if( indent >= 0 )
  {
    /* full details */
    if( multiplicity > 1 )
      sprintf(buff, "%d * ", multiplicity);
    else
      strcpy(buff, "");
    fprintf(fp, "%*s[ %sDemand %s", indent, "", buff,
      d->name==NULL ? "?" : d->name);
    if( HaArrayCount(d->penalizers) == 0 )
      fprintf(fp, " ]\n");
    else
    {
      fprintf(fp, "\n");
      HaArrayForEach(d->penalizers, pr, i)
	NrcPenalizerDebug(pr, indent + 2, fp);
      fprintf(fp, "%*s]\n", indent, "");
    }
  }
  else
  {
    /* name only */
    fprintf(fp, "%s", d->name == NULL ? "?" : d->name);
  }
}

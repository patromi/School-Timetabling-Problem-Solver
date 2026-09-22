
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
/*  FILE:         nrc_dc_converter.c                                         */
/*  MODULE:       A converter of demand constraints into demands             */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*  NRC_MIN_LIMIT - min limits representing the analysed constraints         */
/*                                                                           */
/*****************************************************************************/

typedef struct nrc_min_limit_rec *NRC_MIN_LIMIT;
typedef HA_ARRAY(NRC_MIN_LIMIT) ARRAY_NRC_MIN_LIMIT;

struct nrc_min_limit_rec {
  int			value;			/* value of limit            */
  NRC_WORKER_SET	worker_set;		/* the worker set            */
  bool			non_assignment;		/* true if non-asst is in    */
  NRC_PENALTY		penalty;		/* cost per violation        */
  NRC_MIN_LIMIT		parent;			/* parent node of this one   */
  ARRAY_NRC_MIN_LIMIT	children;		/* child nodes of this one   */
};


/*****************************************************************************/
/*                                                                           */
/*  NRC_DC_CONVERTER - a converter of demand constraints into demands        */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_DEMAND_CONSTRAINT) ARRAY_NRC_DEMAND_CONSTRAINT;
typedef HA_ARRAY(NRC_DEMAND) ARRAY_NRC_DEMAND;

struct nrc_dc_converter_rec {
  NRC_INSTANCE			instance;
  ARRAY_NRC_DEMAND_CONSTRAINT	constraints;
  int				wanted;
  ARRAY_NRC_MIN_LIMIT		min_limits;
  ARRAY_NRC_MIN_LIMIT		free_min_limits;
  ARRAY_NRC_DEMAND		demands;
  HA_ARRAY_INT			demand_multiplicities;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "min limit objects"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_MIN_LIMIT NrcMinLimitMake(NRC_DC_CONVERTER dcc, int value,           */
/*    NRC_WORKER_SET ws, bool non_assignment, NRC_PENALTY p)                 */
/*                                                                           */
/*  Make a new min limit object with these attributes.                       */
/*                                                                           */
/*****************************************************************************/

static NRC_MIN_LIMIT NrcMinLimitMake(NRC_DC_CONVERTER dcc, int value,
  NRC_WORKER_SET ws, bool non_assignment, NRC_PENALTY p)
{
  HA_ARENA a;  NRC_MIN_LIMIT res;

  /* get a min limit object, either from the free list or from the arena */
  if( HaArrayCount(dcc->free_min_limits) > 0 )
  {
    res = HaArrayLastAndDelete(dcc->free_min_limits);
    HaArrayClear(res->children);
  }
  else
  {
    a = NrcInstanceArena(dcc->instance);
    HaMake(res, a);
    HaArrayInit(res->children, a);
  }

  /* fill in the fields and return */
  res->value = value;
  res->worker_set = ws;
  res->non_assignment = non_assignment;
  res->penalty = p;
  res->parent = NULL;
  /* res->children initialized to empty above */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcMinLimitWorkerSetSubset(NRC_MIN_LIMIT lim1, NRC_MIN_LIMIT lim2)  */
/*                                                                           */
/*  Return true if the worker set of lim1 is a subset of the worker set      */
/*  of lim2, including non-assignment as an element of the worker sets.      */
/*                                                                           */
/*****************************************************************************/

static bool NrcMinLimitWorkerSetSubset(NRC_MIN_LIMIT lim1, NRC_MIN_LIMIT lim2)
{
  if( lim1->non_assignment && !lim2->non_assignment )
    return false;
  return NrcWorkerSetSubset(lim1->worker_set, lim2->worker_set);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcMinLimitWorkerSetDisjoint(NRC_MIN_LIMIT lim1, NRC_MIN_LIMIT lim2)*/
/*                                                                           */
/*  Return true if the worker sets of lim1 and lim2 are disjoint, including  */
/*  non-assignment as an element of the worker sets.                         */
/*                                                                           */
/*****************************************************************************/

static bool NrcMinLimitWorkerSetDisjoint(NRC_MIN_LIMIT lim1, NRC_MIN_LIMIT lim2)
{
  if( lim1->non_assignment && lim2->non_assignment )
    return false;
  return NrcWorkerSetDisjoint(lim1->worker_set, lim2->worker_set);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcMinLimitTypedCmp(NRC_MIN_LIMIT lim1, NRC_MIN_LIMIT lim2)          */
/*                                                                           */
/*  Typed comparison function for sorting an array of min limits into        */
/*  decreasing order, first by worker set size (including non-assignment),   */
/*  then by value.                                                           */
/*                                                                           */
/*****************************************************************************/

static int NrcMinLimitTypedCmp(NRC_MIN_LIMIT lim1, NRC_MIN_LIMIT lim2)
{
  int cmp, s1, s2;
  s1 = NrcWorkerSetWorkerCount(lim1->worker_set) + (int) lim1->non_assignment;
  s2 = NrcWorkerSetWorkerCount(lim2->worker_set) + (int) lim2->non_assignment;
  cmp = s2 - s1;
  if( cmp != 0 )  return cmp;
  return lim2->value - lim1->value;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcMinLimitCmp(const void *t1, const void *t2)                       */
/*                                                                           */
/*  Untyped comparison function for sorting an array of min limits into      */
/*  decreasing order, by the same definition as for NrcMinLimitTypedCmp.     */
/*                                                                           */
/*****************************************************************************/

static int NrcMinLimitCmp(const void *t1, const void *t2)
{
  NRC_MIN_LIMIT lim1 = * (NRC_MIN_LIMIT *) t1;
  NRC_MIN_LIMIT lim2 = * (NRC_MIN_LIMIT *) t2;
  return NrcMinLimitTypedCmp(lim1, lim2);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcMinLimitDebug(NRC_MIN_LIMIT lim, int indent, FILE *fp)           */
/*                                                                           */
/*  Debug print of lim onto fp with the given indent.                        */
/*                                                                           */
/*****************************************************************************/

static void NrcMinLimitDebug(NRC_MIN_LIMIT lim, int indent, FILE *fp)
{
  NRC_MIN_LIMIT child_lim;  int i;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "min(%d, %s%s, %s)", lim->value,
    NrcWorkerSetName(lim->worker_set), lim->non_assignment ? " + $" : "",
    NrcPenaltyShow(lim->penalty));
  if( indent >= 0 )
  {
    HaArrayForEach(lim->children, child_lim, i)
      NrcMinLimitDebug(child_lim, indent + 2, fp);
    fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "creation and query"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_DC_CONVERTER NrcDCConverterMake(NRC_INSTANCE ins)                    */
/*                                                                           */
/*  Make a new demand constraint converter object for ins.                   */
/*                                                                           */
/*****************************************************************************/

NRC_DC_CONVERTER NrcDCConverterMake(NRC_INSTANCE ins)
{
  NRC_DC_CONVERTER res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->instance = ins;
  HaArrayInit(res->constraints, a);
  res->wanted = -1;
  HaArrayInit(res->min_limits, a);
  HaArrayInit(res->free_min_limits, a);
  HaArrayInit(res->demands, a);
  HaArrayInit(res->demand_multiplicities, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDCConverterClear(NRC_DC_CONVERTER dcc)                           */
/*                                                                           */
/*  Clear dcc, ready for a new conversion.                                   */
/*                                                                           */
/*****************************************************************************/

void NrcDCConverterClear(NRC_DC_CONVERTER dcc)
{
  int i;
  HaArrayClear(dcc->constraints);
  dcc->wanted = -1;
  HaArrayAppend(dcc->free_min_limits, dcc->min_limits, i);
  HaArrayClear(dcc->min_limits);
  HaArrayClear(dcc->demands);
  HaArrayClear(dcc->demand_multiplicities);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDCConverterAddDemandConstraint(NRC_DC_CONVERTER dcc,             */
/*    NRC_DEMAND_CONSTRAINT dc)                                              */
/*                                                                           */
/*  Add dc to dcc.                                                           */
/*                                                                           */
/*****************************************************************************/

void NrcDCConverterAddDemandConstraint(NRC_DC_CONVERTER dcc,
  NRC_DEMAND_CONSTRAINT dc)
{
  HaArrayAddLast(dcc->constraints, dc);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "solving"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcHandleMin(NRC_DC_CONVERTER dcc, int min_value,                   */
/*    NRC_WORKER_SET ws, NRC_PENALTY p)                                      */
/*                                                                           */
/*  Handle a min limit case.                                                 */
/*                                                                           */
/*****************************************************************************/

static void NrcHandleMin(NRC_DC_CONVERTER dcc, int min_value,
  NRC_WORKER_SET ws, NRC_PENALTY p)
{
  NRC_MIN_LIMIT lim;
  lim = NrcMinLimitMake(dcc, min_value, ws, false, p);
  HaArrayAddLast(dcc->min_limits, lim);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcHandleMax(NRC_DC_CONVERTER dcc, int max_value,                   */
/*    NRC_WORKER_SET ws, NRC_PENALTY p)                                      */
/*                                                                           */
/*  Handle a max limit case.                                                 */
/*                                                                           */
/*****************************************************************************/

static void NrcHandleMax(NRC_DC_CONVERTER dcc, int max_value,
  NRC_WORKER_SET ws, NRC_PENALTY p)
{
  NRC_MIN_LIMIT lim;
  lim = NrcMinLimitMake(dcc, dcc->wanted - max_value,
    NrcWorkerSetComplement(ws), true, p);
  HaArrayAddLast(dcc->min_limits, lim);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcBuildMinLimits(NRC_DC_CONVERTER dcc)                             */
/*                                                                           */
/*  Build min limits for the constraints of dcc.                             */
/*                                                                           */
/*****************************************************************************/

static void NrcBuildMinLimits(NRC_DC_CONVERTER dcc)
{
  NRC_DEMAND_CONSTRAINT dc;  int i;  NRC_BOUND b;  NRC_WORKER_SET ws;
  int min_value, max_value, preferred_value;  bool allow_zero;
  NRC_PENALTY below_min_penalty, above_max_penalty;
  NRC_PENALTY below_preferred_penalty, above_preferred_penalty;
  HaArrayForEach(dcc->constraints, dc, i)
  {
    b = NrcDemandConstraintBound(dc);
    ws = NrcDemandConstraintWorkerSet(dc);
    if( NrcBoundMin(b, &min_value, &allow_zero, &below_min_penalty) &&
	NrcPenaltyWeight(below_min_penalty) > 0 )
    {
      /* handle min penalty */
      NrcHandleMin(dcc, min_value, ws, below_min_penalty);
    }
    if( NrcBoundMax(b, &max_value, &above_max_penalty) &&
	NrcPenaltyWeight(above_max_penalty) > 0 )
    {
      /* handle max penalty */
      NrcHandleMax(dcc, max_value, ws, above_max_penalty);
    }
    if( NrcBoundPreferred(b, &preferred_value, &below_preferred_penalty,
	  &above_preferred_penalty) )
    {
      /* handle preferred penalty */
      if( NrcPenaltyWeight(below_preferred_penalty) > 0 )
	NrcHandleMin(dcc, preferred_value, ws, below_preferred_penalty);
      if( NrcPenaltyWeight(above_preferred_penalty) > 0 )
	NrcHandleMax(dcc, preferred_value, ws, above_preferred_penalty);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcMinLimitInsertBelow(NRC_MIN_LIMIT root, NRC_MIN_LIMIT node)      */
/*                                                                           */
/*  Insert node somewhere below root, returning true if successful.          */
/*                                                                           */
/*****************************************************************************/

static bool NrcMinLimitInsertBelow(NRC_MIN_LIMIT root, NRC_MIN_LIMIT node)
{
  NRC_MIN_LIMIT child;  int i;

  /* insert below a child of root if feasible */
  HaArrayForEach(root->children, child, i)
    if( node->value <= child->value && NrcMinLimitWorkerSetSubset(node, child) )
      return NrcMinLimitInsertBelow(child, node);

  /* otherwise, if not disjoint from all children, fail */
  HaArrayForEach(root->children, child, i)
    if( !NrcMinLimitWorkerSetDisjoint(node, child) )
      return false;

  /* OK to insert node as a child of root */
  HaArrayAddLast(root->children, node);
  node->parent = root;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcMinLimitPenalize(NRC_MIN_LIMIT node, NRC_PENALTY penalty_below,  */
/*    NRC_INSTANCE ins)                                                      */
/*                                                                           */
/*  Generate the penalizers at and above node.  Here penalty_below is the    */
/*  penalty below node.  Parameter node is represented in the documentation  */
/*  by node n_j, where 2 <= j < i.  Node n_i is handled separately.          */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete (probably correct, but never tested)
static void NrcMinLimitPenalize(NRC_DEMAND d, NRC_MIN_LIMIT node,
  NRC_PENALTY penalty_below, bool non_assignment_below, NRC_INSTANCE ins)
{
  NRC_PENALTY penalty_here;
  if( node->parent != NULL )
  {
    penalty_here = NrcPenaltyAdd(penalty_below, node->penalty, ins);
    NrcMinLimitPenalize(d, node->parent, penalty_here, node->non_assignment,
      ins);
  }
  NrcDemandPenalizeWorkerSet(d, node->worker_set, NRC_PENALTY_REPLACE,
    penalty_below);
  if( node->non_assignment && !non_assignment_below )
    NrcDemandPenalizeNonAssignment(d, NRC_PENALTY_UNIQUE, penalty_below);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool NrcGenerateDemands(NRC_DC_CONVERTER dcc, NRC_MIN_LIMIT node,        */
/*    NRC_DEMAND_SET res)                                                    */
/*                                                                           */
/*  Generate demands corresponding to the min limits at and below node,      */
/*  add them to res, and return true.  Return false if there is a problem.   */
/*                                                                           */
/*****************************************************************************/

static bool NrcGenerateDemands(NRC_DC_CONVERTER dcc, NRC_MIN_LIMIT node,
  NRC_DEMAND_SET res)
{
  int i, demands_below_node, demands_at_node, done;
  NRC_MIN_LIMIT child;  NRC_DEMAND d;  NRC_INSTANCE ins;

  /* generate demands below node, failing if that fails */
  demands_below_node = 0;
  HaArrayForEach(node->children, child, i)
  {
    if( !NrcGenerateDemands(dcc, child, res) )
      return false;
    demands_below_node += child->value;
  }

  /* generate demands at node, unless too many generated already */
  demands_at_node = node->value - demands_below_node;
  if( demands_at_node < 0 )
    return false;
  if( demands_at_node > 0 )
  {
    /* make a new demand at node, record it, and add it multiply to res */
    ins = dcc->instance;
    d = NrcDemandMakeBegin(ins);
    HaArrayAddLast(dcc->demands, d);
    HaArrayAddLast(dcc->demand_multiplicities, demands_at_node);
    NrcDemandSetAddDemandMulti(res, d, demands_at_node);

    /* add suitable penalties to all demands made at or below node */
    done = 0;
    for( i = HaArrayCount(dcc->demands) - 1; i >= 0 && done < node->value; i-- )
    {
      d = HaArray(dcc->demands, i);
      NrcDemandPenalizeNotWorkerSet(d, node->worker_set, NRC_PENALTY_ADD,
	node->penalty);
      if( !node->non_assignment )
        NrcDemandPenalizeNonAssignment(d, NRC_PENALTY_ADD, node->penalty);
      done += HaArray(dcc->demand_multiplicities, i);
    }
    HnAssert(done == node->value, "NrcGenerateDemands internal error");
    /* *** obsolete
    NrcMinLimitPenalize(d, node->parent, node->penalty, node->non_assignment,
      ins);
    NrcDemandPenalizeWorkerSet(d, node->worker_set,
      NRC_PENALTY_REPLACE, NrcInstanceZeroPenalty(ins));
    NrcDemandMakeEnd(d);
    *** */
  }

  /* all done, no problems */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcDCConverterSolve(NRC_DC_CONVERTER dcc, int wanted,               */
/*    NRC_DEMAND_SET *res)                                                   */
/*                                                                           */
/*  Solve the demand constraint conversion problem for the demand            */
/*  constraints currently loaded into dcc.  Here wanted is the number of     */
/*  demand objects wanted.  If successful, set *res to an equivalent demand  */
/*  set and return true.  Otherwise set *res to NULL and return false.       */
/*                                                                           */
/*****************************************************************************/

bool NrcDCConverterSolve(NRC_DC_CONVERTER dcc, int wanted, NRC_DEMAND_SET *res)
{
  NRC_MIN_LIMIT root, lim;  NRC_INSTANCE ins;  int i;  NRC_DEMAND d;

  if( DEBUG1 )
    fprintf(stderr, "[ NrcDCConverterSolve(dcc, %d, -)\n", wanted);

  /* build min limits out of the demand constraints of dcc */
  dcc->wanted = wanted;
  NrcBuildMinLimits(dcc);

  if( DEBUG1 )
    NrcDCConverterDebug(dcc, 2, stderr);

  /* make the artificial min limit which is the root node */
  ins = dcc->instance;
  root = NrcMinLimitMake(dcc, dcc->wanted, NrcInstanceStaffing(ins),
    true, NrcInstanceZeroPenalty(ins));

  /* sort the limits and insert them into the tree, or fail if can't */
  HaArraySort(dcc->min_limits, &NrcMinLimitCmp);
  HaArrayForEach(dcc->min_limits, lim, i)
    if( !NrcMinLimitInsertBelow(root, lim) )
    {
      if( DEBUG1 )
	fprintf(stderr, "] NrcDCConverterSolve returning false (lim %d)\n", i);
      return false;
    }

  /* traverse the tree generating demands */
  *res = NrcDemandSetMake(ins);
  if( !NrcGenerateDemands(dcc, root, *res) )
  {
    if( DEBUG1 )
      fprintf(stderr, "] NrcDCConverterSolve returning false (generate)\n");
    return false;
  }

  /* all good; end demands and return */
  HaArrayForEach(dcc->demands, d, i)
    NrcDemandMakeEnd(d);
  if( DEBUG1 )
  {
    fprintf(stderr, "  NrcDCConverterSolve at end:\n");
    HaArrayForEach(dcc->demands, d, i)
      NrcDemandDebug(d, HaArray(dcc->demand_multiplicities, i), 4, stderr);
    fprintf(stderr, "] NrcDCConverterSolve returning true\n");
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDCConverterDebug(NRC_DC_CONVERTER dcc, int indent, FILE *fp)     */
/*                                                                           */
/*  Debug print of dcc onto fp with the given indent.                        */
/*                                                                           */
/*****************************************************************************/

void NrcDCConverterDebug(NRC_DC_CONVERTER dcc, int indent, FILE *fp)
{
  NRC_DEMAND_CONSTRAINT dc;  int i;  NRC_MIN_LIMIT lim;
  fprintf(stderr, "%*s[ Demand constraint converter for %s\n", indent, "",
    NrcInstanceId(dcc->instance));
  HaArrayForEach(dcc->constraints, dc, i)
    NrcDemandConstraintDebug(dc, indent + 2, fp);
  HaArrayForEach(dcc->min_limits, lim, i)
    NrcMinLimitDebug(lim, indent + 2, fp);
  fprintf(stderr, "%*s]\n", indent, "");
}

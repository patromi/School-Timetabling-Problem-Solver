
/*****************************************************************************/
/*                                                                           */
/*  THE KHE HIGH SCHOOL TIMETABLING ENGINE                                   */
/*  COPYRIGHT (C) 2010 Jeffrey H. Kingston                                   */
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
/*  FILE:         khe_sr_assign_by_history.c                                 */
/*  DESCRIPTION:  Assignment by history                                      */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>
#include "khe_mmatch.h"
#define bool_show(x) ((x) ? "true" : "false")

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

#define DEBUG1	0
#define DEBUG2	0
#define DEBUG3	0
#define DEBUG4	0
#define DEBUG5	0	/* assignments (only) */
#define DEBUG6	0	/* edges (in detail) */
#define DEBUG7	0	/* busy days limit */

#define KHE_TASK_NODE_MAGIC 3028777

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_NODE - one task with derived information                        */
/*                                                                           */
/*  This type implements the "admissible task" of the documentation.         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_task_node_rec {
  /* int			index; */ 	/* index in mmatch list      */
  int				magic;		/* checking */
  KHE_TASK			task;		/* the proper root task      */
  KHE_INTERVAL			interval;	/* the interval it covers    */
  KHE_COST			non_asst_cost;	/* cost of non-assignment    */
  KHE_COST			asst_cost;	/* cost of assignment        */
  KHE_MMATCH_NODE		match_node;
} *KHE_TASK_NODE;

typedef HA_ARRAY(KHE_TASK_NODE) ARRAY_KHE_TASK_NODE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_NODE - information about one resource                       */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_constraint_plus_offset_rec *KHE_CONSTRAINT_PLUS_OFFSET;
typedef HA_ARRAY(KHE_CONSTRAINT_PLUS_OFFSET) ARRAY_KHE_CONSTRAINT_PLUS_OFFSET;

typedef struct khe_constraint_class_rec *KHE_CONSTRAINT_CLASS;
typedef HA_ARRAY(KHE_CONSTRAINT_CLASS) ARRAY_KHE_CONSTRAINT_CLASS;

typedef struct khe_resource_node_rec {
  KHE_RESOURCE			resource;	/* the resource              */
  /* KHE_CONSTRAINT_CLASS	class; */	/* constraint class          */
  ARRAY_KHE_CONSTRAINT_PLUS_OFFSET constraints;	/* constraints, from class   */
  bool				ignore_me;	/* ignore this resource      */
  int				history;	/* equal in all constraints  */
  int				max_min_length;	/* over all constraints      */
  int				min_max_length;	/* over all constraints      */
  int				busy_days_limit;  /* over linked constraints */
  KHE_INTERVAL			curr_interval;	/* during solving            */
  KHE_COST			curr_non_asst_cost;  /* during solving       */
  KHE_COST			curr_asst_cost;	/* during solving            */
  KHE_MMATCH_NODE		match_node;
} *KHE_RESOURCE_NODE;

typedef HA_ARRAY(KHE_RESOURCE_NODE) ARRAY_KHE_RESOURCE_NODE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT_PLUS_OFFSET                                               */
/*                                                                           */
/*  A limit active intervals constraint, plus one legal offset of it.        */
/*                                                                           */
/*****************************************************************************/

struct khe_constraint_plus_offset_rec {
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT	laic;
  int					offset;
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_CONSTRAINT_CLASS                                                */
/*                                                                           */
/*  A set of limit active intervals constraints (plus offsets) with equal    */
/*  time groups.                                                             */
/*                                                                           */
/*****************************************************************************/

struct khe_constraint_class_rec {
  ARRAY_KHE_CONSTRAINT_PLUS_OFFSET	constraints;
  ARRAY_KHE_RESOURCE_NODE		resource_nodes;
  ARRAY_KHE_CONSTRAINT_CLASS		linked_classes;
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_FIX_TYPE - whether to fix the assignments made                  */
/*                                                                           */
/*****************************************************************************/

/* *** we're now fixing when sa != NULL
typedef enum {
  KHE_FIX_NONE,
  KHE_FIX_FIX,
  KHE_FIX_FIX_NO_UNDO
} KHE_FIX_TYPE;
*** */


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_HISTORY_SOLVER                                                  */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_history_solver_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_RESOURCE_TYPE		resource_type;
  KHE_OPTIONS			options;
  KHE_FRAME			days_frame;
  KHE_EVENT_TIMETABLE_MONITOR	etm;
  /* KHE_FIX_TYPE		fix_type; */
  KHE_SOLN_ADJUSTER		soln_adjuster;
  ARRAY_KHE_CONSTRAINT_CLASS	admissible_classes;
  ARRAY_KHE_CONSTRAINT_CLASS	busy_days_classes;
  ARRAY_KHE_RESOURCE_NODE	curr_resource_nodes;	/* during solving */
  ARRAY_KHE_TASK_NODE		curr_task_nodes;	/* during solving */
  KHE_MMATCH			match;
  HA_ARRAY_INT			tmp_indexes;		/* scratch array  */
} *KHE_HISTORY_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TASK_NODE"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_NODE KheTaskNodeMake(KHE_TASK task, KHE_INTERVAL in,            */
/*    int index, HA_ARENA a)                                                 */
/*                                                                           */
/*  Return a new task node containing task, whose interval is in.  This      */
/*  node's index in the list of nodes it lies in is index.                   */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_NODE KheTaskNodeMake(KHE_TASK task, KHE_INTERVAL in,
  /* int index, */ HA_ARENA a)
{
  KHE_TASK_NODE res;
  HaMake(res, a);
  /* res->index = index; */
  res->magic = KHE_TASK_NODE_MAGIC;
  res->task = task;
  res->interval = in;
  KheTaskNonAsstAndAsstCost(task, &res->non_asst_cost, &res->asst_cost);
  res->match_node = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskNodeInitForSolving(KHE_TASK_NODE tn, KHE_HISTORY_SOLVER hs)  */
/*                                                                           */
/*  Initialize tn for solving.                                               */
/*                                                                           */
/*****************************************************************************/

static void KheTaskNodeInitForSolving(KHE_TASK_NODE tn, KHE_HISTORY_SOLVER hs)
{
  tn->match_node = KheMMatchSupplyNodeMake(hs->match, 1, (void *) tn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskNodeAssign(KHE_TASK_NODE tn, KHE_RESOURCE r,                 */
/*    KHE_HISTORY_SOLVER hs)                                                 */
/*                                                                           */
/*  Assign r to tn.  This must succeed.  Record the assignment in            */
/*  hs->task_set, if hs->task_set is non-NULL.                               */
/*                                                                           */
/*****************************************************************************/

static void KheTaskNodeAssign(KHE_TASK_NODE tn, KHE_RESOURCE r,
  KHE_HISTORY_SOLVER hs)
{
  if( !KheTaskAssignResource(tn->task, r) )
    HnAbort("KheTaskNodeAssign internal error");
  if( DEBUG5 )
  {
    fprintf(stderr, "  KheAssignByHistory assigning %s to ", KheResourceId(r));
    KheTaskDebug(tn->task, 2, 0, stderr);
  }
  if( hs->soln_adjuster != NULL )
    KheSolnAdjusterTaskEnsureFixed(hs->soln_adjuster, tn->task);
  /* *** no, the doc says now that if sa == NULL the assts are not fixed
  else
    KheTaskAssignFix(tn->task);
  *** */
}

/* *** old version that makes a fuss over fixing
static void KheTaskNodeAssign(KHE_TASK_NODE tn, KHE_RESOURCE r,
  KHE_HISTORY_SOLVER hs)
{
  if( !KheTaskAssignResource(tn->task, r) )
    HnAbort("KheTaskNodeAssign internal error");
  if( DEBUG5 )
  {
    fprintf(stderr, "  KheAssignByHistory assigning %s to ", KheResourceId(r));
    KheTaskDebug(tn->task, 2, -1, stderr);
    fprintf(stderr, " (%s)\n", hs->fix_type == KHE_FIX_NONE ? "no fix" :
      hs->fix_type == KHE_FIX_FIX ? "fix" : "fix_no_undo");
  }
  switch( hs->fix_type )
  {
    case KHE_FIX_NONE:

      ** no fixing to do **
      break;

    case KHE_FIX_FIX:

      ** fix and add to sa so that it can be undone later **
      KheSolnAdjusterTaskEnsureFixed(hs->soln_adjuster, tn->task);
      break;

    case KHE_FIX_FIX_NO_UNDO:

      ** fix without adding to sa so that it will not be undone later **
      KheTaskAssignFix(tn->task);
      break;

    default:

      HnAbort("KheTaskNodeAssign internal error");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskNodeDebugHeader(KHE_TASK_NODE tn, KHE_FRAME frame, FILE *fp) */
/*                                                                           */
/*  Debug print of the header of tn in frame onto fp.                        */
/*                                                                           */
/*****************************************************************************/

static void KheTaskNodeDebugHeader(KHE_TASK_NODE tn, KHE_FRAME frame, FILE *fp)
{
  fprintf(fp, "TaskNode(");
  /* fprintf(fp, "TaskNode %d(", tn->index); */
  KheTaskDebug(tn->task, 1, -1, fp);
  fprintf(fp, ", in %s, non_asst %.5f, asst %.5f)",
    KheIntervalShow(tn->interval, frame),
    KheCostShow(tn->non_asst_cost), KheCostShow(tn->asst_cost));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskNodeDebug(KHE_TASK_NODE tn, KHE_FRAME frame,                 */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of tn in frame onto fp with the given verbosity and indent.  */
/*                                                                           */
/*****************************************************************************/

static void KheTaskNodeDebug(KHE_TASK_NODE tn, KHE_FRAME frame,
  int verbosity, int indent, FILE *fp)
{
  if( indent < 0 )
    KheTaskNodeDebugHeader(tn, frame, fp);
  else if( verbosity <= 1 )
  {
    fprintf(fp, "%*s", indent, "");
    KheTaskNodeDebugHeader(tn, frame, fp);
    fprintf(fp, "\n");
  }
  else
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheTaskNodeDebugHeader(tn, frame, fp);
    fprintf(fp, " ]\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskNodeDebugFn(void *value, int verbosity,                      */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Task node debug function for passing to the mmatch module.               */
/*                                                                           */
/*****************************************************************************/

static void KheTaskNodeDebugFn(void *value, int verbosity,
  int indent, FILE *fp)
{
  KHE_TASK_NODE tn;
  tn = (KHE_TASK_NODE) value;
  KheTaskNodeDebug(tn, NULL, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_RESOURCE_NODE"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_NODE KheResourceNodeMake(KHE_RESOURCE r,                    */
/*    KHE_CONSTRAINT_CLASS first_class,                                      */
/*    KHE_CONSTRAINT_PLUS_OFFSET first_constraint, int history, HA_ARENA a)  */
/*                                                                           */
/*  Make a resource info object for r with these attributes.                 */
/*                                                                           */
/*****************************************************************************/
static void KheResourceNodeDebug(KHE_RESOURCE_NODE rn, int verbosity,
  int indent, FILE *fp);

static KHE_RESOURCE_NODE KheResourceNodeMake(KHE_RESOURCE r,
  /* KHE_CONSTRAINT_CLASS class, */ HA_ARENA a)
{
  KHE_RESOURCE_NODE res;
  HaMake(res, a);
  res->resource = r;
  /* res->class = class; */
  HaArrayInit(res->constraints, a);
  res->ignore_me = false;
  res->history = -1;			/* means initially undefined */
  res->max_min_length = -1;		/* means initially undefined */
  res->min_max_length = -1;		/* means initially undefined */
  res->busy_days_limit = -1;		/* means initially undefined */
  res->curr_interval = KheIntervalMake(1, 0);
  res->curr_non_asst_cost = 0;		/* actually undefined */
  res->curr_asst_cost = 0;		/* actually undefined */
  res->match_node = NULL;
  if( DEBUG3 )
  {
    fprintf(stderr, "  KheResourceNodeMake returning ");
    KheResourceNodeDebug(res, 2, 0, stderr);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceNodeAddAdmissibleConstraint(KHE_RESOURCE_NODE rn,        */
/*    KHE_CONSTRAINT_PLUS_OFFSET co)                                         */
/*                                                                           */
/*  Add admissible constraint co to rn.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheResourceNodeAddAdmissibleConstraint(KHE_RESOURCE_NODE rn,
  KHE_CONSTRAINT_PLUS_OFFSET co, int history /* , KHE_COST non_asst_cost */)
{
  int min_length, max_length;

  /* add co to rn */
  HaArrayAddLast(rn->constraints, co);

  /* update history, ignoring rn if history values differ */
  if( rn->history == -1 )
    rn->history = history;
  else if( history != rn->history )
    rn->ignore_me = true;

  /* update max_min_length */
  min_length = KheLimitActiveIntervalsConstraintMinimum(co->laic);
  if( rn->max_min_length == -1 )
    rn->max_min_length = min_length;
  else if( min_length > rn->max_min_length )
    rn->max_min_length = min_length;

  /* update min_max_length */
  max_length = KheLimitActiveIntervalsConstraintMaximum(co->laic);
  if( rn->min_max_length == -1 )
    rn->min_max_length = max_length;
  else if( max_length < rn->min_max_length )
    rn->min_max_length = max_length;

  /* ignore rn if there is no suitable length */
  if( rn->max_min_length > rn->min_max_length )
    rn->ignore_me = true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceNodeAddBusyDaysConstraint(KHE_RESOURCE_NODE rn,          */
/*    KHE_CONSTRAINT_PLUS_OFFSET co, int history)                            */
/*                                                                           */
/*  Add busy days constraint co to rn.                                       */
/*                                                                           */
/*****************************************************************************/

static void KheResourceNodeAddBusyDaysConstraint(KHE_RESOURCE_NODE rn,
  KHE_CONSTRAINT_PLUS_OFFSET co, int history)
{
  int max_length, busy_days_limit;
  max_length = KheLimitActiveIntervalsConstraintMaximum(co->laic);
  busy_days_limit = max(0, max_length - history);
  if( rn->busy_days_limit == -1 )
    rn->busy_days_limit = busy_days_limit;
  else if( busy_days_limit < rn->busy_days_limit )
    rn->busy_days_limit = busy_days_limit;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceNodeInitForSolving(KHE_RESOURCE_NODE rn,                 */
/*    int tgi, KHE_HISTORY_SOLVER hs)                                        */
/*                                                                           */
/*  Initialize rn for solving, after checking that it is wanted.             */
/*                                                                           */
/*****************************************************************************/
static KHE_COST KheConstraintPlusOffsetNonAsstCost(
  KHE_CONSTRAINT_PLUS_OFFSET co,  int det);

static void KheResourceNodeInitForSolving(KHE_RESOURCE_NODE rn,
  int tgi, KHE_HISTORY_SOLVER hs)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  int i, seq_len;
  seq_len = rn->history + tgi + 1;
  if( !rn->ignore_me && !KheIntervalContains(rn->curr_interval, tgi) &&
      seq_len <= rn->max_min_length )
  {
    if( rn->busy_days_limit == -1 || tgi < rn->busy_days_limit )
    {
      /* this resource needs to be included in the solve */
      rn->match_node = KheMMatchDemandNodeMake(hs->match, 1, (void *) rn);
      rn->curr_asst_cost = 0;
      rn->curr_non_asst_cost = 0;
      HaArrayForEach(rn->constraints, co, i)
	rn->curr_non_asst_cost += KheConstraintPlusOffsetNonAsstCost(co,
	  rn->history + tgi);
      HaArrayAddLast(hs->curr_resource_nodes, rn);
    }
    else
    {
      /* this resource is being omitted owing to a busy days limit */
      if( DEBUG7 )
	fprintf(stderr, "  omitting %s owing to busy_days_limit %d\n",
	  KheResourceId(rn->resource), rn->busy_days_limit);
      rn->match_node = NULL;
    }
  }
  else
  {
    /* this resource is not wanted; it might be wanted for a later solve */
    rn->match_node = NULL;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceNodeDebug(KHE_RESOURCE_NODE rn, int verbosity,           */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of rn onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/
static void KheConstraintPlusOffsetDebug(KHE_CONSTRAINT_PLUS_OFFSET co,
  int verbosity, int indent, FILE *fp);

static void KheResourceNodeDebug(KHE_RESOURCE_NODE rn, int verbosity,
  int indent, FILE *fp)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  int i;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "[ %s: ", KheResourceId(rn->resource));
  HaArrayForEach(rn->constraints, co, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    KheConstraintPlusOffsetDebug(co, verbosity, -1, fp);
  }
  fprintf(fp, " h%d l%d-%d", rn->history, rn->max_min_length,
    rn->min_max_length);
  if( rn->busy_days_limit != -1 )
    fprintf(fp, " busy_days_limit %d", rn->busy_days_limit);
  fprintf(fp, " ]");
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceNodeDebugFn(void *value, int verbosity,                  */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Resource node debug function for passing to the mmatch module.           */
/*                                                                           */
/*****************************************************************************/

static void KheResourceNodeDebugFn(void *value, int verbosity,
  int indent, FILE *fp)
{
  KHE_RESOURCE_NODE rn;
  rn = (KHE_RESOURCE_NODE) value;
  KheResourceNodeDebug(rn, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_CONSTRAINT_PLUS_OFFSET"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT_PLUS_OFFSET KheConstraintPlusOffsetMake(                  */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic, int offset, HA_ARENA a)    */
/*                                                                           */
/*  Make a new constraint plus offset object with these attributes.          */
/*                                                                           */
/*****************************************************************************/

static KHE_CONSTRAINT_PLUS_OFFSET KheConstraintPlusOffsetMake(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic, int offset, HA_ARENA a)
{
  KHE_CONSTRAINT_PLUS_OFFSET res;
  HaMake(res, a);
  res->laic = laic;
  res->offset = offset;
  /* res->used_count = 0; */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintPlusOffsetCompatible(KHE_CONSTRAINT_PLUS_OFFSET co1,   */
/*    KHE_CONSTRAINT_PLUS_OFFSET co2)                                        */
/*                                                                           */
/*  Return true if co1 and co2 are compatible:  if they have the same time   */
/*  groups in the same order.                                                */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintPlusOffsetCompatible(KHE_CONSTRAINT_PLUS_OFFSET co1,
  KHE_CONSTRAINT_PLUS_OFFSET co2)
{
  int count1, count2, i;  KHE_TIME_GROUP tg1, tg2;  KHE_POLARITY po1, po2;
  count1 = KheLimitActiveIntervalsConstraintTimeGroupCount(co1->laic);
  count2 = KheLimitActiveIntervalsConstraintTimeGroupCount(co1->laic);
  if( count1 != count2 )
    return false;
  for( i = 0;  i < count1;  i++ )
  {
    tg1 = KheLimitActiveIntervalsConstraintTimeGroup(co1->laic, i,
      co1->offset, &po1);
    tg2 = KheLimitActiveIntervalsConstraintTimeGroup(co2->laic, i,
      co2->offset, &po2);
    if( !KheTimeGroupEqual(tg1, tg2) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintPlusOffsetLinkable(KHE_CONSTRAINT_PLUS_OFFSET co1,     */
/*    KHE_CONSTRAINT_PLUS_OFFSET co2)                                        */
/*                                                                           */
/*  Return true if co1 is linkable to co2:  if co1's time groups are         */
/*  subsets of the corresponding time groups of co2, and co2's weight        */
/*  is at least as great as co1's.                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintPlusOffsetLinkable(KHE_CONSTRAINT_PLUS_OFFSET co1,
  KHE_CONSTRAINT_PLUS_OFFSET co2)
{
  int count1, count2, i;  KHE_TIME_GROUP tg1, tg2;  KHE_POLARITY po1, po2;
  KHE_COST weight1, weight2;

  /* check constraint weights */
  weight1 = KheConstraintCombinedWeight((KHE_CONSTRAINT) co1->laic);
  weight2 = KheConstraintCombinedWeight((KHE_CONSTRAINT) co2->laic);
  if( weight2 <= weight1 )
    return false;

  /* check time groups */
  count1 = KheLimitActiveIntervalsConstraintTimeGroupCount(co1->laic);
  count2 = KheLimitActiveIntervalsConstraintTimeGroupCount(co2->laic);
  if( count1 != count2 )
    return false;
  for( i = 0;  i < count1;  i++ )
  {
    tg1 = KheLimitActiveIntervalsConstraintTimeGroup(co1->laic, i,
      co1->offset, &po1);
    tg2 = KheLimitActiveIntervalsConstraintTimeGroup(co2->laic, i,
      co2->offset, &po2);
    if( !KheTimeGroupSubset(tg1, tg2) )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheConstraintPlusOffsetNonAsstCost(                             */
/*    KHE_CONSTRAINT_PLUS_OFFSET co, int det)                                */
/*                                                                           */
/*  Return the cost of not assigning a resource with this determinant.       */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheConstraintPlusOffsetNonAsstCost(
  KHE_CONSTRAINT_PLUS_OFFSET co,  int det)
{
  int dev, min_limit, max_limit;;  KHE_COST combined_weight;

  /* record the fact that co has been used */
  /* co->used_count++; */

  /* find the deviation */
  min_limit = KheLimitActiveIntervalsConstraintMinimum(co->laic);
  max_limit = KheLimitActiveIntervalsConstraintMaximum(co->laic);
  if( det < min_limit )
    dev = min_limit - det;
  else if( det > max_limit )
    dev = det - max_limit;
  else
    dev = 0;

  /* find and return the cost */
  combined_weight = KheConstraintCombinedWeight((KHE_CONSTRAINT) co->laic);
  switch( KheConstraintCostFunction((KHE_CONSTRAINT) co->laic) )
  {
    case KHE_STEP_COST_FUNCTION:

      return combined_weight;

    case KHE_LINEAR_COST_FUNCTION:

      return combined_weight * dev;

    case KHE_QUADRATIC_COST_FUNCTION:

      return combined_weight * dev * dev;

    default:

      HnAbort("KheConstraintPlusOffsetNonAsstCost internal error 2");
      return 0;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintPlusOffsetDebug(KHE_CONSTRAINT_PLUS_OFFSET co,         */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of co onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintPlusOffsetDebug(KHE_CONSTRAINT_PLUS_OFFSET co,
  int verbosity, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "%s", KheConstraintId((KHE_CONSTRAINT) co->laic));
  if( co->offset > 0 )
    fprintf(fp, "|%d", co->offset);
  /* fprintf(fp, " (%d)", co->used_count); */
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_CONSTRAINT_CLASS"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT_CLASS KheConstraintClassMake(                             */
/*    KHE_CONSTRAINT_PLUS_OFFSET co, HA_ARENA a)                             */
/*                                                                           */
/*  Make a new constraint class containing just co.                          */
/*                                                                           */
/*****************************************************************************/

static KHE_CONSTRAINT_CLASS KheConstraintClassMake(
  KHE_CONSTRAINT_PLUS_OFFSET co, HA_ARENA a)
{
  KHE_CONSTRAINT_CLASS res;
  HaMake(res, a);
  HaArrayInit(res->constraints, a);
  HaArrayAddLast(res->constraints, co);
  HaArrayInit(res->resource_nodes, a);
  HaArrayInit(res->linked_classes, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintClassAcceptsConstraintPlusOffset(                      */
/*    KHE_CONSTRAINT_CLASS cc, KHE_CONSTRAINT_PLUS_OFFSET co)                */
/*                                                                           */
/*  If cc accepts co, add co to cc and return true.  Otherwise change        */
/*  nothing and return false.                                                */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintClassAcceptsConstraintPlusOffset(
  KHE_CONSTRAINT_CLASS cc, KHE_CONSTRAINT_PLUS_OFFSET co)
{
  KHE_CONSTRAINT_PLUS_OFFSET co2;
  co2 = HaArrayFirst(cc->constraints);
  if( KheConstraintPlusOffsetCompatible(co2, co) )
  {
    HaArrayAddLast(cc->constraints, co);
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintClassTypedCmp(KHE_CONSTRAINT_CLASS cc1,                 */
/*    KHE_CONSTRAINT_CLASS cc2)                                              */
/*                                                                           */
/*  Typed comparison function for sorting an array of constraint classes     */
/*  so that classes with smaller time groups (actually, a smaller first      */
/*  time group) come first.                                                  */
/*                                                                           */
/*  Implementation note.  We are only dealing with limit active intervals    */
/*  constraints that have at least one time group.                           */
/*                                                                           */
/*****************************************************************************/

/* *** irrelevant now we are limiting to time groups with a single time each
static int KheConstraintClassTypedCmp(KHE_CONSTRAINT_CLASS cc1,
  KHE_CONSTRAINT_CLASS cc2)
{
  KHE_TIME_GROUP tg1, tg2;  KHE_POLARITY po1, po2;
  KHE_CONSTRAINT_PLUS_OFFSET co1, co2;

  co1 = HaArrayFirst(cc1->constraints);
  co2 = HaArrayFirst(cc2->constraints);
  tg1 = KheLimitActiveIntervalsConstraintTimeGroup(co1->laic, 0,
    co1->offset, &po1);
  tg2 = KheLimitActiveIntervalsConstraintTimeGroup(co2->laic, 0,
    co2->offset, &po2);
  return KheTimeGroupTimeCount(tg1) - KheTimeGroupTimeCount(tg2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintClassCmp(const void *t1, const void *t2)                */
/*                                                                           */
/*  Untyped comparison function for sorting an array of constraint classes   */
/*  so that classes with smaller time groups (actually, a smaller first      */
/*  time group) come first.                                                  */
/*                                                                           */
/*****************************************************************************/

/* *** irrelevant now we are limiting to time groups with a single time each
static int KheConstraintClassCmp(const void *t1, const void *t2)
{
  KHE_CONSTRAINT_CLASS cc1, cc2;
  cc1 = * (KHE_CONSTRAINT_CLASS *) t1;
  cc2 = * (KHE_CONSTRAINT_CLASS *) t2;
  return KheConstraintClassTypedCmp(cc1, cc2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintClassLinkable(KHE_CONSTRAINT_CLASS cc,                 */
/*    KHE_CONSTRAINT_CLASS busy_days_cc)                                     */
/*                                                                           */
/*  Return true if cc is linkable to busy_days_cc:  if cc's time groups      */
/*  are subsets of the corresponding time groups of busy_days_cc.            */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintClassLinkable(KHE_CONSTRAINT_CLASS cc,
  KHE_CONSTRAINT_CLASS busy_days_cc)
{
  KHE_CONSTRAINT_PLUS_OFFSET co, busy_days_co;
  HnAssert(HaArrayCount(cc->constraints) > 0,
    "KheConstraintClassLinkable internal error 1");
  HnAssert(HaArrayCount(busy_days_cc->constraints) > 0,
    "KheConstraintClassLinkable internal error 2");
  co = HaArrayFirst(cc->constraints);
  busy_days_co = HaArrayFirst(busy_days_cc->constraints);
  return KheConstraintPlusOffsetLinkable(co, busy_days_co);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassLink(KHE_CONSTRAINT_CLASS cc,                     */
/*    KHE_CONSTRAINT_CLASS busy_days_cc)                                     */
/*                                                                           */
/*  Link cc to busy_days_cc.                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassLink(KHE_CONSTRAINT_CLASS cc,
  KHE_CONSTRAINT_CLASS busy_days_cc)
{
  HaArrayAddLast(cc->linked_classes, busy_days_cc);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintClassHasResourceHistory(KHE_CONSTRAINT_CLASS cc,       */
/*    KHE_RESOURCE r, HA_ARENA a)                                            */
/*                                                                           */
/*  If cc includes history for r, return true, make a resource node object   */
/*  and add it to cc.  Otherwise return false.                               */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintClassHasResourceHistory(KHE_CONSTRAINT_CLASS cc,
  KHE_RESOURCE r, HA_ARENA a)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  int i, j, history;
  KHE_CONSTRAINT_CLASS linked_cc;  KHE_RESOURCE_NODE rn;

  /* build a resource node for r containing limits from cc's constraints */
  rn = NULL;
  HaArrayForEach(cc->constraints, co, i)
  {
    history = KheLimitActiveIntervalsConstraintHistory(co->laic, r);
    if( DEBUG3 )
      fprintf(stderr,
	"  KheLimitActiveIntervalsConstraintHistory(%s, %s) = %d\n",
	KheConstraintId((KHE_CONSTRAINT) co->laic), KheResourceId(r), history);
    if( history > 0 &&
	history < KheLimitActiveIntervalsConstraintMinimum(co->laic) )
    {
      /* if no rn yet, create one and add it to cc */
      if( rn == NULL )
      {
	rn = KheResourceNodeMake(r, /* cc, */ a);
	HaArrayAddLast(cc->resource_nodes, rn);
      }

      /* update rn to reflect the discovery of co */
      KheResourceNodeAddAdmissibleConstraint(rn, co, history);
    }
  }

  /* adjust rn's limits to take account of linked constraints */
  if( rn != NULL )
    HaArrayForEach(cc->linked_classes, linked_cc, i)
      HaArrayForEach(linked_cc->constraints, co, j)
      {
	history = KheLimitActiveIntervalsConstraintHistory(co->laic, r);
	/* *** do this whether there is history or not
	if( history > 0 &&
	    history < KheLimitActiveIntervalsConstraintMinimum(co->laic) )
	*** */
	KheResourceNodeAddBusyDaysConstraint(rn, co, history);
      }

  /* all done, return true if we made a resource node for r */
  return rn != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCheckTimesAndSetIndexes(KHE_TASK task,                           */
/*    KHE_CONSTRAINT_PLUS_OFFSET co, KHE_HISTORY_SOLVER hs)                  */
/*                                                                           */
/*  Helper function for KheTaskIsAdmissibleAndStarting below.  This          */
/*  checks that every time that task is running is monitored by co,          */
/*  and it builds hs->indexes, an array of indexes of the days that          */
/*  task is running.                                                         */
/*                                                                           */
/*  Implementation note.  Variable index is clearly set to an index          */
/*  into hs->days_frame.  But by the conditions applied to constraints,      */
/*  this is also an index into the sequence of time groups of co->laic.      */
/*                                                                           */
/*****************************************************************************/

static bool KheCheckTimesAndSetIndexes(KHE_TASK task,
  KHE_CONSTRAINT_PLUS_OFFSET co, KHE_HISTORY_SOLVER hs)
{
  KHE_TASK child_task;  int i, index, pos;  KHE_MEET meet;
  KHE_TIME meet_t, t;  KHE_TIME_GROUP tg;  KHE_POLARITY po;

  /* do the job for task itself */
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    meet_t = KheMeetAsstTime(meet);
    if( meet_t != NULL )
      for( i = 0;  i < KheMeetDuration(meet);  i++ )
      {
	t = KheTimeNeighbour(meet_t, i);
	index = KheFrameTimeIndex(hs->days_frame, t);
	tg = KheLimitActiveIntervalsConstraintTimeGroup(co->laic, index,
	  co->offset, &po);
	if( !KheTimeGroupContains(tg, t, &pos) )
	  return false;  /* (e) */
	HaArrayAddLast(hs->tmp_indexes, index);
      }
  }

  /* do the job for task's children */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    if( !KheCheckTimesAndSetIndexes(child_task, co, hs) )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIntCmp(const void *t1, const void *t2)                            */
/*                                                                           */
/*  Comparison function for sorting an array of integers into increasing     */
/*  order.                                                                   */
/*                                                                           */
/*****************************************************************************/

static int KheIntCmp(const void *t1, const void *t2)
{
  int i1, i2;
  i1 = * (int *) t1;
  i2 = * (int *) t2;
  return i1 - i2;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskIsAdmissibleAndStarting(KHE_TASK task,                       */
/*    KHE_CONSTRAINT_CLASS cc, KHE_TIME_GROUP tg, int tg_index,              */
/*    KHE_HISTORY_SOLVER hs, KHE_INTERVAL *in)                               */
/*                                                                           */
/*  Helper function for KheConstraintClassInitTaskNodesForSolving.  It       */
/*  checks some of its conditions, and builds an interval holding the        */
/*  indexes of the days that task is running.                                */
/*                                                                           */
/*  Implementation note.  Condition (c), that task must be running at at     */
/*  least one time, is guaranteed by the way task was discovered.            */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskIsAdmissibleAndStarting(KHE_TASK task,
  KHE_CONSTRAINT_CLASS cc, KHE_TIME_GROUP tg, int tg_index,
  KHE_HISTORY_SOLVER hs, KHE_INTERVAL *in)
{
  int init_count, count, first, last;
  HaArrayClear(hs->tmp_indexes);
  *in = KheIntervalMake(1, 0);
  if( !KheCheckTimesAndSetIndexes(task, HaArrayFirst(cc->constraints), hs) )
    return false;  /* (e) */
  init_count = HaArrayCount(hs->tmp_indexes);
  HaArraySortUnique(hs->tmp_indexes, &KheIntCmp);
  count = HaArrayCount(hs->tmp_indexes);
  if( count != init_count )
    return false;  /* (d) */
  HnAssert(count > 0, "KheTaskIsAdmissibleAndStarting internal error");
  first = HaArrayFirst(hs->tmp_indexes);
  if( first != tg_index )
    return false;  /* (g) */
  last = HaArrayLast(hs->tmp_indexes);
  if( last - first + 1 != count )
    return false;  /* (f) */
  *in = KheIntervalMake(first, last);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassInitTaskNodesForSolving(KHE_CONSTRAINT_CLASS cc,  */
/*    KHE_TIME_GROUP tg, int tg_index, KHE_HISTORY_SOLVER hs)                */
/*                                                                           */
/*  Add new task nodes to *nodes for the tasks of cc starting at tg,         */
/*  whose index in the constraints of cc is tg_index.  Each task must        */
/*  be admissible, that is                                                   */
/*                                                                           */
/*  (a) task has resource type hs->resource_type.                            */
/*                                                                           */
/*  (b) task is a proper root task.                                          */
/*                                                                           */
/*  (c) task is running at at least one time.                                */
/*                                                                           */
/*  (d) the times task is running include at most one time from each day.    */
/*                                                                           */
/*  (e) every time that task is running is monitored by cc's constraints.    */
/*                                                                           */
/*  (f) the days of the times that task is running are consecutive.          */
/*                                                                           */
/*  In addition, this function requires                                      */
/*                                                                           */
/*  (g) task's first time lies in tg, whose index in the frame is tg_index.  */
/*                                                                           */
/*  Condition (g) allows us to find the tasks we need by searching all       */
/*  tasks in all meets running at any time of tg.  It also guarantees (c).   */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassInitTaskNodesForSolving(KHE_CONSTRAINT_CLASS cc,
  KHE_TIME_GROUP tg, int tg_index, KHE_HISTORY_SOLVER hs)
{
  int i, j, k;  KHE_TIME t;  KHE_MEET meet;  KHE_TASK task;
  KHE_TASK_NODE tn;  KHE_INTERVAL in;
  HaArrayClear(hs->curr_task_nodes);
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);  /* (c) */
    for( j = 0;  j < KheEventTimetableMonitorTimeMeetCount(hs->etm, t);  j++ )
    {
      meet = KheEventTimetableMonitorTimeMeet(hs->etm, t, j);
      for( k = 0;  k < KheMeetTaskCount(meet);  k++ )
      {
	task = KheMeetTask(meet, k);
	task = KheTaskProperRoot(task);  /* (b) */
	if( KheTaskResourceType(task) == hs->resource_type && /* (a) */
            KheTaskIsAdmissibleAndStarting(task, cc, tg, tg_index, hs, &in) )
	{
	  /* add task to hs->curr_task_nodes */
	  tn = KheTaskNodeMake(task, in,
	    /* HaArrayCount(hs->curr_task_nodes), */ hs->arena);
	  HaArrayAddLast(hs->curr_task_nodes, tn);
	  KheTaskNodeInitForSolving(tn, hs);
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDropCurrTaskNode(KHE_HISTORY_SOLVER hs, KHE_TASK_NODE tn)       */
/*                                                                           */
/*  Drop tn from hs->curr_task_nodes.                                        */
/*                                                                           */
/*****************************************************************************/

/* *** no longer needed
static void KheDropCurrTaskNode(KHE_HISTORY_SOLVER hs, KHE_TASK_NODE tn)
{
  KHE_TASK_NODE tn2;
  HnAssert(HaArray(hs->curr_task_nodes, tn->index) == tn,
    "KheDropCurrTaskNode internal error");
  tn2 = HaArrayLast(hs->curr_task_nodes);
  if( tn2 != tn )
  {
    tn2->index = tn->index;
    HaArrayPut(hs->curr_task_nodes, tn2->index, tn2);
  }
  HaArrayDeleteLast(hs->curr_task_nodes);
}
*** */

/*****************************************************************************/
/*                                                                           */
/*  bool KheEdgeIsWanted(KHE_RESOURCE_NODE rn, KHE_TASK_NODE tn,             */
/*    int *c1, int *c2, int *c3)                                             */
/*                                                                           */
/*  If an edge is wanted between rn and tn, return true and set *c1, *c2,    */
/*  and *c3 to a suitable cost vector.  Otherwise return false.              */
/*                                                                           */
/*****************************************************************************/

static bool KheEdgeIsWanted(KHE_RESOURCE_NODE rn, KHE_TASK_NODE tn,
  int *c1, int *c2, int *c3)
{
  KHE_RESOURCE r2;  bool ok1, ok2;

  /* work out wheether the task will accept the resource */
  if( DEBUG6 )
    fprintf(stderr, "edge from %s to %s: ", KheResourceId(rn->resource),
      KheTaskId(tn->task));
  r2 = KheTaskAsstResource(tn->task);
  if( r2 != NULL )
    ok1 = (r2 == rn->resource);
  else
    ok1 = KheTaskAssignResourceCheck(tn->task, rn->resource);

  /* work out whether the resource will accept the task */
  ok2 = (rn->history + KheIntervalLength(tn->interval) <= rn->min_max_length );

  if( ok1 && ok2 )
  {
    /* OK, so set the costs and return true */
    *c1 = 1;  /* so no edges have negative cost */
    *c2 = KheHardCost(rn->curr_asst_cost) - KheHardCost(rn->curr_non_asst_cost)
      + KheHardCost(tn->asst_cost) - KheHardCost(tn->non_asst_cost);
    *c3 = KheSoftCost(rn->curr_asst_cost) - KheSoftCost(rn->curr_non_asst_cost)
      + KheSoftCost(tn->asst_cost) - KheSoftCost(tn->non_asst_cost);
    if( DEBUG6 )
      fprintf(stderr, "true (rn asst_cost %.5f, rn non_asst_cost %.5f, "
	"tn asst_cost %.5f, tn non_asst_cost %.5f)\n",
	KheCostShow(rn->curr_asst_cost), KheCostShow(rn->curr_non_asst_cost),
	KheCostShow(tn->asst_cost), KheCostShow(tn->non_asst_cost));
    return true;
  }
  else
  {
    /* not OK, so set the costs to arbitrary values and return false */
    *c1 = *c2 = *c3 = -1;
    if( DEBUG6 )
      fprintf(stderr, "false (assignable=%s && length=%s)\n",
	bool_show(ok1), bool_show(ok2));
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassMatch(KHE_CONSTRAINT_CLASS cc,                    */
/*    KHE_TIME_GROUP tg, int tgi, KHE_HISTORY_SOLVER hs, bool *more_to_do)   */
/*                                                                           */
/*  Carry out the Match operation from the documentation.  Set *more_to_do   */
/*  to true if there is more to do after this.                               */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassMatch(KHE_CONSTRAINT_CLASS cc,
  KHE_TIME_GROUP tg, int tgi, KHE_HISTORY_SOLVER hs, bool *more_to_do)
{
  KHE_RESOURCE_NODE rn;  KHE_TASK_NODE tn;  int i, j, mult, c1, c2, c3;
  KHE_MMATCH_NODE sn;

  /* clear match */
  KheMMatchClear(hs->match);

  /* add demand (resource) nodes; return if none left */
  *more_to_do = false;
  HaArrayClear(hs->curr_resource_nodes);
  HaArrayForEach(cc->resource_nodes, rn, i)
    KheResourceNodeInitForSolving(rn, tgi, hs);
  if( HaArrayCount(hs->curr_resource_nodes) == 0 )
    return;

  /* add supply nodes */
  KheConstraintClassInitTaskNodesForSolving(cc, tg, tgi, hs);
  /* *** this now done by KheConstraintClassInitTaskNodesForSolving
  HaArrayForEach(hs->curr_task_nodes, tn, j)
    KheTaskNodeInitForSolving(tn, hs->match);
  *** */

  /* add edges */
  HaArrayForEach(hs->curr_resource_nodes, rn, i)
    HaArrayForEach(hs->curr_task_nodes, tn, j)
      if( KheEdgeIsWanted(rn, tn, &c1, &c2, &c3) )
	KheMMatchAddEdge(rn->match_node, tn->match_node, 1, c1, c2, c3);

  /* solve */
  KheMMatchSolve(hs->match);

  /* handle the consequences */
  HaArrayForEach(hs->curr_resource_nodes, rn, j)
    if( KheMMatchResultEdge(rn->match_node, &sn, &mult, &c1, &c2, &c3) )
    {
      /* assign rn->resource to tn's tasks */
      tn = (KHE_TASK_NODE) KheMMatchSupplyNodeBack(sn);
      HnAssert(tn->magic == KHE_TASK_NODE_MAGIC,
	"KheConstraintClassMatch internal error");
      KheTaskNodeAssign(tn, rn->resource, hs);
      rn->curr_interval = KheIntervalUnion(rn->curr_interval, tn->interval);

      if( rn->history + KheIntervalLength(rn->curr_interval) <
	  rn->max_min_length )
	*more_to_do = true;
    }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintClassSolve(KHE_CONSTRAINT_CLASS cc,                     */
/*    KHE_HISTORY_SOLVER hs)                                                 */
/*                                                                           */
/*  Solve cc.                                                                */
/*                                                                           */
/*****************************************************************************/
static void KheConstraintClassDebug(KHE_CONSTRAINT_CLASS cc,
  int verbosity, int indent, FILE *fp);

static int KheConstraintClassSolve(KHE_CONSTRAINT_CLASS cc,
  KHE_HISTORY_SOLVER hs)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  int tg_count, tgi, res;
  KHE_TIME_GROUP tg;  KHE_POLARITY po;  bool more_to_do;
  if( DEBUG4 )
  {
    fprintf(stderr, "[ KheConstraintClassSolve(cc, hs); cc =\n");
    KheConstraintClassDebug(cc, 2, 2, stderr);
  }
  res = 0;
  co = HaArrayFirst(cc->constraints);
  tg_count = KheLimitActiveIntervalsConstraintTimeGroupCount(co->laic);
  more_to_do = true;
  for( tgi = 0;  tgi < tg_count && more_to_do;  tgi++ )
  {
    tg = KheLimitActiveIntervalsConstraintTimeGroup(co->laic, tgi,
      co->offset, &po);
    if( DEBUG4 )
      fprintf(stderr, "  %d resources, time group %d: %s\n",
	HaArrayCount(cc->resource_nodes), tgi, KheTimeGroupId(tg));
    KheConstraintClassMatch(cc, tg, tgi, hs, &more_to_do);
  }
  if( DEBUG4 )
    fprintf(stderr, "] KheConstraintClassSolve returning %d\n", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassDebug(KHE_CONSTRAINT_CLASS cc,                    */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of cc onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/
static void KheResourceNodeDebug(KHE_RESOURCE_NODE rn, int verbosity,
  int indent, FILE *fp);

static void KheConstraintClassDebug(KHE_CONSTRAINT_CLASS cc,
  int verbosity, int indent, FILE *fp)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  int i;  KHE_RESOURCE_NODE rn;
  KHE_CONSTRAINT_CLASS linked_cc;
  HnAssert(HaArrayCount(cc->constraints) > 0,
    "KheConstraintClassDebug internal error");
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ConstraintClass\n", indent, "");
    HaArrayForEach(cc->constraints, co, i)
      KheConstraintPlusOffsetDebug(co, verbosity, indent + 2, fp);
    HaArrayForEach(cc->resource_nodes, rn, i)
      KheResourceNodeDebug(rn, verbosity, indent + 2, fp);
    if( HaArrayCount(cc->linked_classes) > 0 )
    {
      fprintf(fp, "%*s  linked classes:\n", indent, "");
      HaArrayForEach(cc->linked_classes, linked_cc, i)
        KheConstraintClassDebug(linked_cc, 1, indent + 2, fp);
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
  {
    co = HaArrayFirst(cc->constraints);
    KheConstraintPlusOffsetDebug(co, verbosity, indent, fp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_HISTORY_SOLVER"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheCostDebugFn(int c1, int c2, int c3, FILE *fp)                    */
/*                                                                           */
/*  Debug function for printing an edge cost.                                */
/*                                                                           */
/*****************************************************************************/

static void KheCostDebugFn(int c1, int c2, int c3, FILE *fp)
{
  fprintf(fp, "%d:%d:%d", c1, c2, c3);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_FIX_TYPE KheGetFixType(KHE_OPTIONS options)                          */
/*                                                                           */
/*  Get the fix type from options.                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_FIX_TYPE KheGetFixType(KHE_OPTIONS options)
{
  char *str;
  str = KheOptionsGet(options, "rs_assign_by_history_fix", "none");
  if( strcmp(str, "none") == 0 )
    return KHE_FIX_NONE;
  else if( strcmp(str, "fix") == 0 )
    return KHE_FIX_FIX;
  else if( strcmp(str, "fix_no_undo") == 0 )
    return KHE_FIX_FIX_NO_UNDO;
  else
  {
    HnAbort("invalid value \"%s\" of rs_assign_by_history_fix option", str);
    return KHE_FIX_NONE;  ** keep compiler happy **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_HISTORY_SOLVER KheHistorySolverMake(KHE_SOLN soln,                   */
/*    KHE_RESOURCE_TYPE rt, KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa,       */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Make a new history grouper object with these attributes.                 */
/*                                                                           */
/*****************************************************************************/
static void KheHistorySolverDebugFn(void *value, int verbosity,
  int indent, FILE *fp);

static KHE_HISTORY_SOLVER KheHistorySolverMake(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa,
  HA_ARENA a)
{
  KHE_HISTORY_SOLVER res;
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->resource_type = rt;
  res->options = options;
  res->days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
  res->etm = (KHE_EVENT_TIMETABLE_MONITOR) KheOptionsGetObject(options,
    "gs_event_timetable_monitor", NULL);
  /* res->fix_type = KheGetFixType(options); */
  res->soln_adjuster = sa;
  HaArrayInit(res->admissible_classes, a);
  HaArrayInit(res->busy_days_classes, a);
  /* HaArrayInit(res->resource_nodes, a); */
  HaArrayInit(res->curr_resource_nodes, a);
  HaArrayInit(res->curr_task_nodes, a);
  res->match = KheMMatchMake((void *) res, &KheHistorySolverDebugFn,
    &KheResourceNodeDebugFn, &KheTaskNodeDebugFn, &KheCostDebugFn, a);
  HaArrayInit(res->tmp_indexes, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintAdmissible(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic, */
/*    int offset, KHE_FRAME days_frame)                                      */
/*                                                                           */
/*  Return true if laic at offset is admissible.                             */
/*                                                                           */
/*  This function checks conditions (3), (4), and (5) of the documentation.  */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintAdmissible(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic,
  int offset, KHE_FRAME days_frame)
{
  int i, tg_count;  KHE_TIME_GROUP laic_tg;  KHE_POLARITY po;

  /* constraint must have no more time groups than frame */
  tg_count = KheLimitActiveIntervalsConstraintTimeGroupCount(laic);
  if( KheFrameTimeGroupCount(days_frame) < tg_count )
    return false;

  /* each time group of laic must be contain a single time, from frame_tg */
  for( i = 0;  i < tg_count;  i++ )
  {
    laic_tg = KheLimitActiveIntervalsConstraintTimeGroup(laic, i, offset, &po);
    if( KheTimeGroupTimeCount(laic_tg) != 1 ||
	KheFrameTimeIndex(days_frame, KheTimeGroupTime(laic_tg, 0)) != i )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintBusyDays(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic,   */
/*    int offset, KHE_FRAME days_frame)                                      */
/*                                                                           */
/*  Return true if laic is a consecutive busy days constraint, as defined    */
/*  by the documentation.                                                    */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintBusyDays(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic,
  int offset, KHE_FRAME days_frame)
{
  int i, tg_count;  KHE_TIME_GROUP laic_tg, ftg;  KHE_POLARITY po;

  /* constraint must have no more time groups than frame */
  tg_count = KheLimitActiveIntervalsConstraintTimeGroupCount(laic);
  if( KheFrameTimeGroupCount(days_frame) < tg_count )
    return false;

  /* each time group of laic must be a subset of the current day */
  for( i = 0;  i < tg_count;  i++ )
  {
    laic_tg = KheLimitActiveIntervalsConstraintTimeGroup(laic, i, offset, &po);
    ftg = KheFrameTimeGroup(days_frame, i);
    if( KheTimeGroupTimeCount(laic_tg) < 2 || !KheTimeGroupSubset(laic_tg,ftg) )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAddAdmissibleConstraint(KHE_HISTORY_SOLVER hs,                   */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic, int offset)                */
/*                                                                           */
/*  Add admissible constraint (laic, offset) to hs's admissible classes.     */
/*                                                                           */
/*****************************************************************************/

static void KheAddAdmissibleConstraint(KHE_HISTORY_SOLVER hs,
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic, int offset)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  KHE_CONSTRAINT_CLASS cc;  int i;

  /* make co and add it to an existing class if possible */
  co = KheConstraintPlusOffsetMake(laic, offset, hs->arena);
  HaArrayForEach(hs->admissible_classes, cc, i)
    if( KheConstraintClassAcceptsConstraintPlusOffset(cc, co) )
      return;

  /* no match, so make a new class and add co to that */
  cc = KheConstraintClassMake(co, hs->arena);
  HaArrayAddLast(hs->admissible_classes, cc);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAddBusyDaysConstraint(KHE_HISTORY_SOLVER hs,                     */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic, int offset)                */
/*                                                                           */
/*  Add busy days constraint (laic, offset) to hs's busy days classes.       */
/*                                                                           */
/*****************************************************************************/

static void KheAddBusyDaysConstraint(KHE_HISTORY_SOLVER hs,
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic, int offset)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  KHE_CONSTRAINT_CLASS cc;  int i;

  /* make co and add it to an existing class if possible */
  co = KheConstraintPlusOffsetMake(laic, offset, hs->arena);
  HaArrayForEach(hs->busy_days_classes, cc, i)
    if( KheConstraintClassAcceptsConstraintPlusOffset(cc, co) )
      return;

  /* no match, so make a new class and add co to that */
  cc = KheConstraintClassMake(co, hs->arena);
  HaArrayAddLast(hs->busy_days_classes, cc);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheHistorySolverBuildConstraintClasses(KHE_HISTORY_SOLVER hs)       */
/*                                                                           */
/*  Build the constraint classes of hs.                                      */
/*                                                                           */
/*  This function checks conditions (1) and (2) of the documentation, and    */
/*  calls function KheConstraintAdmissible to check (3), (4), and (5).       */
/*                                                                           */
/*****************************************************************************/

static void KheHistorySolverBuildConstraintClasses(KHE_HISTORY_SOLVER hs)
{
  KHE_INSTANCE ins;  KHE_CONSTRAINT c;  int i, j, ocount, offset;
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic;
  ins = KheSolnInstance(hs->soln);
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintTag(c) == KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG )
    {
      laic = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c;
      if( KheLimitActiveIntervalsConstraintTimeGroupCount(laic) > 0 &&
	  KheLimitActiveIntervalsConstraintResourceOfTypeCount(laic,
	    hs->resource_type) > 0 &&
	  KheLimitActiveIntervalsConstraintAllPositive(laic) )
      {
	ocount = KheLimitActiveIntervalsConstraintAppliesToOffsetCount(laic);
	for( j = 0;  j < ocount;  j++ )
	{
	  offset = KheLimitActiveIntervalsConstraintAppliesToOffset(laic, j);
	  if( KheConstraintAdmissible(laic, offset, hs->days_frame) )
	    KheAddAdmissibleConstraint(hs, laic, offset);
	  else if( KheConstraintBusyDays(laic, offset, hs->days_frame) )
	    KheAddBusyDaysConstraint(hs, laic, offset);
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheHistorySolverDebug(KHE_HISTORY_SOLVER hs, int verbosity,         */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of hs onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheHistorySolverDebug(KHE_HISTORY_SOLVER hs, int verbosity,
  int indent, FILE *fp)
{
  KHE_CONSTRAINT_CLASS cc;  int i;
  fprintf(fp, "%*s[ HistorySolver(%s, %s)\n", indent, "",
    KheInstanceId(KheSolnInstance(hs->soln)),
    KheResourceTypeId(hs->resource_type));
  fprintf(fp, "%*s  admissible constraints:\n", indent, "");
  HaArrayForEach(hs->admissible_classes, cc, i)
    KheConstraintClassDebug(cc, verbosity, indent + 2, fp);
  fprintf(fp, "%*s  busy days constraints:\n", indent, "");
  HaArrayForEach(hs->busy_days_classes, cc, i)
    KheConstraintClassDebug(cc, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheHistorySolverDebugFn(void *value, int verbosity,                 */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug function for passing to the mmatch module.                         */
/*                                                                           */
/*****************************************************************************/

static void KheHistorySolverDebugFn(void *value, int verbosity,
  int indent, FILE *fp)
{
  KHE_HISTORY_SOLVER hs;
  hs = (KHE_HISTORY_SOLVER) value;
  KheHistorySolverDebug(hs, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheAssignByHistory"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheAssignByHistory(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,              */
/*    KHE_OPTIONS options, KHE_TASK_SET r_ts)                                */
/*                                                                           */
/*  Group tasks of type rt and assign them resources of type rt to handle    */
/*  history.  Return the number of groups made.                              */
/*                                                                           */
/*****************************************************************************/

int KheAssignByHistory(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa)
{
  KHE_HISTORY_SOLVER hs;  HA_ARENA a;  int res, i, j;
  KHE_RESOURCE r;  KHE_CONSTRAINT_CLASS cc, busy_days_cc;

  /* return immediately if solve is off */
  if( KheOptionsGetBool(options, "rs_assign_by_history_off", false) )
    return 0;

  /* make a history solver object and build its constraint classes */
  if( DEBUG1 )
    fprintf(stderr, "[ KheAssignByHistory(%s, %s, -, -)\n",
      KheInstanceId(KheSolnInstance(soln)), KheResourceTypeId(rt));
  HnAssert(sa != NULL, "KheAssignByHistory: sa == NULL");
  a = KheSolnArenaBegin(soln);
  hs = KheHistorySolverMake(soln, rt, options, sa, a);

  /* build constraint classes and link admissibles to busy days */
  KheHistorySolverBuildConstraintClasses(hs);
  HaArrayForEach(hs->admissible_classes, cc, i)
    HaArrayForEach(hs->busy_days_classes, busy_days_cc, j)
      if( KheConstraintClassLinkable(cc, busy_days_cc) )
	KheConstraintClassLink(cc, busy_days_cc);
  /* HaArraySort(hs->admissible_classes, &KheConstraintClassCmp); */

  /* add the resource node objects */
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    HaArrayForEach(hs->admissible_classes, cc, j)
      if( KheConstraintClassHasResourceHistory(cc, r, a) )
	break;
  }
  if( DEBUG2 )
    KheHistorySolverDebug(hs, 2, 2, stderr);

  /* solve each constraint class */
  res = 0;
  HaArrayForEach(hs->admissible_classes, cc, i)
    res += KheConstraintClassSolve(cc, hs);

  /* all done */
  KheSolnArenaEnd(soln, a);
  if( DEBUG1 )
    fprintf(stderr, "] KheAssignByHistory returning %d\n", res);
  return res;
}

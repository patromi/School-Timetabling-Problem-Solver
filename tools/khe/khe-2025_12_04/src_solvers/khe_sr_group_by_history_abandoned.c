
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
/*  FILE:         khe_sr_group_by_history.c                                  */
/*  DESCRIPTION:  Group by history                                           */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>
#include "khe_mmatch.h"
#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1	1
#define DEBUG2	1
#define DEBUG3	0
#define DEBUG4	1


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_NODE - a set of tasks with derived information                  */
/*                                                                           */
/*  This type implements the "admissible task-set" of the documentation.     */
/*  The interval of these tasks starts at the start of the cycle.            */
/*                                                                           */
/*  For convenience, this type is also used for the "admissible task" of     */
/*  the documentation.  The interval need not be at the start of the cycle.  */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK;

typedef struct khe_task_node_rec {
  int			index;		/* index in mmatch list */
  ARRAY_KHE_TASK	tasks;		/* the tasks */
  KHE_INTERVAL		interval;	/* the interval they cover */
  int			length;		/* the length of the interval */
  KHE_RESOURCE_SET	domain;		/* the intersection of their domains */
  KHE_COST		non_asst_cost;	/* cost of non-assignment            */
  KHE_COST		asst_cost;	/* cost of assignment                */
  KHE_MMATCH_NODE	match_node;
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

typedef struct khe_resource_node_rec {
  KHE_RESOURCE			resource;		/* the resource      */
  KHE_CONSTRAINT_CLASS		first_class;		/* its first class   */
  ARRAY_KHE_CONSTRAINT_PLUS_OFFSET constraints;
  int				history;		/* from first cons.  */
  int				min_length;		/* from cons.        */
  int				max_length;		/* from cons.        */
  KHE_COST			non_asst_cost;		/* from cons.        */
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
  int					used_count;	/* how often used */
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
};

typedef HA_ARRAY(KHE_CONSTRAINT_CLASS) ARRAY_KHE_CONSTRAINT_CLASS;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_HISTORY_GROUPER                                                 */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_history_grouper_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_RESOURCE_TYPE		resource_type;
  KHE_OPTIONS			options;
  KHE_FRAME			days_frame;
  KHE_EVENT_TIMETABLE_MONITOR	etm;
  KHE_TASK_SET			task_set;
  ARRAY_KHE_CONSTRAINT_CLASS	constraint_classes;
  ARRAY_KHE_TASK_NODE		curr_task_nodes;
  ARRAY_KHE_TASK_NODE		next_task_nodes;
  KHE_MMATCH			xmatch;		/* resources to task sets */
  KHE_MMATCH			ymatch;		/* task sets to task sets */
  HA_ARRAY_INT			tmp_indexes;	/* scratch array */
} *KHE_HISTORY_GROUPER;


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
  int index, HA_ARENA a)
{
  KHE_TASK_NODE res;  KHE_RESOURCE_TYPE rt;
  rt = KheTaskResourceType(task);
  HaMake(res, a);
  res->index = index;
  HaArrayInit(res->tasks, a);
  HaArrayAddLast(res->tasks, task);
  res->interval = in;
  res->length = KheIntervalLength(in);
  res->domain = KheResourceSetMake(rt, a);
  KheResourceSetUnionGroup(res->domain, KheTaskDomain(task));
  KheTaskNonAsstAndAsstCost(task, &res->non_asst_cost, &res->asst_cost);
  res->match_node = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskNodeMerge(KHE_TASK_NODE to_tn, KHE_TASK_NODE from_tn)        */
/*                                                                           */
/*  Merge from_tn into to_tn.                                                */
/*                                                                           */
/*****************************************************************************/

static void KheTaskNodeMerge(KHE_TASK_NODE to_tn, KHE_TASK_NODE from_tn)
{
  int i;
  HnAssert(to_tn->interval.last + 1 == from_tn->interval.first,
    "KheTaskNodeMerge internal error 1");
  HaArrayAppend(to_tn->tasks, from_tn->tasks, i);
  to_tn->interval = KheIntervalUnion(to_tn->interval, from_tn->interval);
  to_tn->length = KheIntervalLength(to_tn->interval);
  KheResourceSetIntersect(to_tn->domain, from_tn->domain);
  to_tn->non_asst_cost += from_tn->non_asst_cost;
  to_tn->asst_cost += from_tn->asst_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskNodeAddMMatchSupplyNode(KHE_TASK_NODE tn, KHE_MMATCH mm)     */
/*                                                                           */
/*  Make a supply node representing tn and add it to mm.                     */
/*                                                                           */
/*****************************************************************************/

static void KheTaskNodeAddMMatchSupplyNode(KHE_TASK_NODE tn, KHE_MMATCH mm)
{
  tn->match_node = KheMMatchSupplyNodeMake(mm, 1, (void *) tn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskNodeAddMMatchDemandNode(KHE_TASK_NODE tn, KHE_MMATCH mm)     */
/*                                                                           */
/*  Make a demand node representing tn and add it to mm.                     */
/*                                                                           */
/*****************************************************************************/

static void KheTaskNodeAddMMatchDemandNode(KHE_TASK_NODE tn, KHE_MMATCH mm)
{
  tn->match_node = KheMMatchDemandNodeMake(mm, 1, (void *) tn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskNodeAssign(KHE_TASK_NODE tn, KHE_RESOURCE r,                 */
/*    KHE_HISTORY_GROUPER hg)                                                */
/*                                                                           */
/*  Assign r to the tasks of tn.  This must succeed.  Record the assignment  */
/*  in hg->task_set, if non-NULL.                                            */
/*                                                                           */
/*****************************************************************************/

static void KheTaskNodeAssign(KHE_TASK_NODE tn, KHE_RESOURCE r,
  KHE_HISTORY_GROUPER hg)
{
  KHE_TASK task;  int i;
  HaArrayForEach(tn->tasks, task, i)
  {
    if( !KheTaskAssignResource(task, r) )
      HnAbort("KheTaskNodeAssign internal error");
    if( hg->task_set != NULL )
      KheTaskSetAddTask(hg->task_set, task);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskNodeDebugHeader(KHE_TASK_NODE tn, KHE_FRAME frame, FILE *fp) */
/*                                                                           */
/*  Debug print of the header of tn in frame onto fp.                        */
/*                                                                           */
/*****************************************************************************/

static void KheTaskNodeDebugHeader(KHE_TASK_NODE tn, KHE_FRAME frame, FILE *fp)
{
  fprintf(fp, "TaskNode %d(%d tasks, i %s, len %d, non_asst %.5f, asst %.5f)",
    tn->index, HaArrayCount(tn->tasks), KheIntervalShow(tn->interval, frame),
    tn->length, KheCostShow(tn->non_asst_cost), KheCostShow(tn->asst_cost));
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
  KHE_TASK task;  int i;
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
    fprintf(fp, "\n");
    HaArrayForEach(tn->tasks, task, i)
      KheTaskDebug(task, verbosity, indent + 2, fp);
    KheResourceSetDebug(tn->domain, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
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
  KHE_CONSTRAINT_CLASS first_class, HA_ARENA a)
{
  KHE_RESOURCE_NODE res;
  HaMake(res, a);
  res->resource = r;
  res->first_class = first_class;
  HaArrayInit(res->constraints, a);
  res->history = 0;
  res->min_length = INT_MAX;
  res->max_length = 0;
  res->non_asst_cost = 0;
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
/*  void KheResourceNodeAddConstraintPlusOffset(KHE_RESOURCE_NODE rn,        */
/*    KHE_CONSTRAINT_PLUS_OFFSET co)                                         */
/*                                                                           */
/*  Add co to rn.                                                            */
/*                                                                           */
/*****************************************************************************/

static void KheResourceNodeAddConstraintPlusOffset(KHE_RESOURCE_NODE rn,
  KHE_CONSTRAINT_PLUS_OFFSET co, int history, KHE_COST non_asst_cost)
{
  int min_length, max_length;

  /* add co to rn */
  HaArrayAddLast(rn->constraints, co);

  /* update history */
  if( history > rn->history )
    rn->history = history;

  /* update min_length */
  min_length = KheLimitActiveIntervalsConstraintMinimum(co->laic);
  if( min_length < rn->min_length )
    rn->min_length = min_length;

  /* update max_length */
  max_length = KheLimitActiveIntervalsConstraintMaximum(co->laic);
  if( max_length > rn->max_length )
    rn->max_length = max_length;

  /* update non_asst_cost */
  rn->non_asst_cost += non_asst_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceNodeAddMMatchDemandNode(KHE_RESOURCE_NODE rn,            */
/*    KHE_MMATCH mm)                                                         */
/*                                                                           */
/*  Make a demand node representing rn and add it to mm.                     */
/*                                                                           */
/*****************************************************************************/

static void KheResourceNodeAddMMatchDemandNode(KHE_RESOURCE_NODE rn,
  KHE_MMATCH mm)
{
  rn->match_node = KheMMatchDemandNodeMake(mm, 1, (void *) rn);
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
  fprintf(fp, " h%d l%d-%d %.5f ]", rn->history, rn->min_length,
    rn->max_length, KheCostShow(rn->non_asst_cost));
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
  res->used_count = 0;
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
/*  KHE_COST KheConstraintPlusOffsetNonAsstCost(                             */
/*    KHE_CONSTRAINT_PLUS_OFFSET co,  int history)                           */
/*                                                                           */
/*  Return the cost of not assigning a resource with this history,           */
/*  assuming history < KheLimitActiveIntervalsConstraintMinimum(co->laic).   */
/*                                                                           */
/*  Also record in co->used_count the fact that co has been used.            */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheConstraintPlusOffsetNonAsstCost(
  KHE_CONSTRAINT_PLUS_OFFSET co,  int history)
{
  int dev;  KHE_COST combined_weight;
  dev = KheLimitActiveIntervalsConstraintMinimum(co->laic) - history;
  HnAssert(dev > 0, "KheConstraintPlusOffsetNonAsstCost internal error 1");
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

  /* record the fact that co has been used */
  co->used_count++;
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
  fprintf(fp, " (%d)", co->used_count);
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


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintClassCmp(const void *t1, const void *t2)                */
/*                                                                           */
/*  Untyped comparison function for sorting an array of constraint classes   */
/*  so that classes with smaller time groups (actually, a smaller first      */
/*  time group) come first.                                                  */
/*                                                                           */
/*****************************************************************************/

static int KheConstraintClassCmp(const void *t1, const void *t2)
{
  KHE_CONSTRAINT_CLASS cc1, cc2;
  cc1 = * (KHE_CONSTRAINT_CLASS *) t1;
  cc2 = * (KHE_CONSTRAINT_CLASS *) t2;
  return KheConstraintClassTypedCmp(cc1, cc2);
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
  KHE_CONSTRAINT_PLUS_OFFSET co;  int i, history;  KHE_COST non_asst_cost;
  KHE_RESOURCE_NODE rn;
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
	rn = KheResourceNodeMake(r, cc, a);
	HaArrayAddLast(cc->resource_nodes, rn);
      }

      /* update rn to reflect the discovery of co */
      non_asst_cost = KheConstraintPlusOffsetNonAsstCost(co, history);
      KheResourceNodeAddConstraintPlusOffset(rn, co, history, non_asst_cost);
    }
  }
  return rn != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCheckTimesAndSetIndexes(KHE_TASK task,                           */
/*    KHE_CONSTRAINT_PLUS_OFFSET co, KHE_HISTORY_GROUPER hg)                 */
/*                                                                           */
/*  Helper function for KheTaskIsAdmissibleAndStarting below.  This          */
/*  checks that every time that task is running is monitored by co,          */
/*  and it builds hg->indexes, an array of indexes of the days that          */
/*  task is running.                                                         */
/*                                                                           */
/*  Implementation note.  Variable index is clearly set to an index          */
/*  into hg->days_frame.  But by the conditions applied to constraints,      */
/*  this is also an index into the sequence of time groups of co->laic.      */
/*                                                                           */
/*****************************************************************************/

static bool KheCheckTimesAndSetIndexes(KHE_TASK task,
  KHE_CONSTRAINT_PLUS_OFFSET co, KHE_HISTORY_GROUPER hg)
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
	index = KheFrameTimeIndex(hg->days_frame, t);
	tg = KheLimitActiveIntervalsConstraintTimeGroup(co->laic, index,
	  co->offset, &po);
	if( !KheTimeGroupContains(tg, t, &pos) )
	  return false;  /* (e) */
	HaArrayAddLast(hg->tmp_indexes, index);
      }
  }

  /* do the job for task's children */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    if( !KheCheckTimesAndSetIndexes(child_task, co, hg) )
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
/*    KHE_HISTORY_GROUPER hg, KHE_INTERVAL *in)                              */
/*                                                                           */
/*  Helper function for KheConstraintClassBuildTaskNodes.  It checks some    */
/*  of its conditions, and builds an interval holding the indexes of the     */
/*  days that task is running.                                               */
/*                                                                           */
/*  Implementation note.  Condition (c), that task must be running at at     */
/*  least one time, is guaranteed by the way task was discovered.            */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskIsAdmissibleAndStarting(KHE_TASK task,
  KHE_CONSTRAINT_CLASS cc, KHE_TIME_GROUP tg, int tg_index,
  KHE_HISTORY_GROUPER hg, KHE_INTERVAL *in)
{
  int init_count, count, first, last;
  HaArrayClear(hg->tmp_indexes);
  *in = KheIntervalMake(1, 0);
  if( !KheCheckTimesAndSetIndexes(task, HaArrayFirst(cc->constraints), hg) )
    return false;  /* (e) */
  init_count = HaArrayCount(hg->tmp_indexes);
  HaArraySortUnique(hg->tmp_indexes, &KheIntCmp);
  count = HaArrayCount(hg->tmp_indexes);
  if( count != init_count )
    return false;  /* (d) */
  HnAssert(count > 0, "KheTaskIsAdmissibleAndStarting internal error");
  first = HaArrayFirst(hg->tmp_indexes);
  if( first != tg_index )
    return false;  /* (g) */
  last = HaArrayLast(hg->tmp_indexes);
  if( last - first + 1 != count )
    return false;  /* (f) */
  *in = KheIntervalMake(first, last);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassBuildTaskNodes(KHE_CONSTRAINT_CLASS cc,           */
/*    KHE_TIME_GROUP tg, int tg_index, ARRAY_KHE_TASK_NODE *nodes,           */
/*    KHE_HISTORY_GROUPER hg)                                                */
/*                                                                           */
/*  Add new task nodes to *nodes for the tasks of cc starting at tg,         */
/*  whose index in the constraints of cc is tg_index.  Each task must        */
/*  be admissible, that is                                                   */
/*                                                                           */
/*  (a) task has resource type hg->resource_type.                            */
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
/*  (g) task's first time lies in tg.                                        */
/*                                                                           */
/*  Condition (g) allows us to find the tasks we need by searching all       */
/*  tasks in all meets running at any time of tg.  It also guarantees (c).   */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassBuildTaskNodes(KHE_CONSTRAINT_CLASS cc,
  KHE_TIME_GROUP tg, int tg_index, ARRAY_KHE_TASK_NODE *nodes,
  KHE_HISTORY_GROUPER hg)
{
  int i, j, k;  KHE_TIME t;  KHE_MEET meet;  KHE_TASK task;
  KHE_TASK_NODE tn;  KHE_INTERVAL in;
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    for( j = 0;  j < KheEventTimetableMonitorTimeMeetCount(hg->etm, t);  j++ )
    {
      meet = KheEventTimetableMonitorTimeMeet(hg->etm, t, j);
      for( k = 0;  k < KheMeetTaskCount(meet);  k++ )
      {
	task = KheMeetTask(meet, k);
	task = KheTaskProperRoot(task);  /* (b) */
	if( KheTaskResourceType(task) == hg->resource_type && /* (a) */
            KheTaskIsAdmissibleAndStarting(task, cc, tg, tg_index, hg, &in) )
	{
	  /* add task to *nodes */
	  tn = KheTaskNodeMake(task, in, HaArrayCount(*nodes), hg->arena);
	  HaArrayAddLast(*nodes, tn);
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDropCurrTaskNode(KHE_HISTORY_GROUPER hg, KHE_TASK_NODE tn)       */
/*                                                                           */
/*  Drop tn from hg->curr_task_nodes.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheDropCurrTaskNode(KHE_HISTORY_GROUPER hg, KHE_TASK_NODE tn)
{
  KHE_TASK_NODE tn2;
  HnAssert(HaArray(hg->curr_task_nodes, tn->index) == tn,
    "KheDropCurrTaskNode internal error");
  tn2 = HaArrayLast(hg->curr_task_nodes);
  if( tn2 != tn )
  {
    tn2->index = tn->index;
    HaArrayPut(hg->curr_task_nodes, tn2->index, tn2);
  }
  HaArrayDeleteLast(hg->curr_task_nodes);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassXMatch(KHE_CONSTRAINT_CLASS cc,                   */
/*    KHE_HISTORY_GROUPER hg)                                                */
/*                                                                           */
/*  Carry out the XMatch operation from the documentation.                   */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassXMatch(KHE_CONSTRAINT_CLASS cc,
  KHE_HISTORY_GROUPER hg)
{
  KHE_RESOURCE_NODE rn;  KHE_TASK_NODE tn;  int i, j, mult, c1, c2, c3;
  KHE_MMATCH_NODE sn;

  /* clear xmatch and add demand nodes */
  KheMMatchClear(hg->xmatch);
  HaArrayForEach(cc->resource_nodes, rn, j)
    KheResourceNodeAddMMatchDemandNode(rn, hg->xmatch);

  /* add supply nodes */
  HaArrayForEach(hg->curr_task_nodes, tn, j)
    KheTaskNodeAddMMatchSupplyNode(tn, hg->xmatch);

  /* add edges */
  HaArrayForEach(cc->resource_nodes, rn, i)
    HaArrayForEach(hg->curr_task_nodes, tn, j)
    {
      if( KheResourceSetContainsResource(tn->domain, rn->resource) &&
	  rn->history + tn->length <= rn->max_length )
      {
	/* rn and tn can match; now find the cost and make an edge */
	c2 = KheHardCost(rn->non_asst_cost);
	c3 = KheSoftCost(rn->non_asst_cost);
	/* ***
	c3 = rn->history - rn->min_length -
	  KheResourceSetResourceCount(tn->domain);
	*** */
	KheMMatchAddEdge(rn->match_node, tn->match_node, 1, 0, c2, c3);
      }
    }

  /* solve */
  KheMMatchSolve(hg->xmatch);

  /* handle the consequences */
  HaArrayForEach(cc->resource_nodes, rn, j)
    if( !KheMMatchResultEdge(rn->match_node, &sn, &mult, &c1, &c2, &c3) )
    {
      /* drop rn, it missed out */
      if( DEBUG4 )
	fprintf(stderr, "  dropping %s\n", KheResourceId(rn->resource));
      HaArrayDeleteAndPlug(cc->resource_nodes, j);
      j--;
    }
    else
    {
      tn = (KHE_TASK_NODE) sn;
      if( rn->history + tn->length >= rn->min_length )
      {
	/* assign rn->resource to tn's tasks */
	if( DEBUG4 )
	{
	  fprintf(stderr, "  assigning %s to:\n",
	    KheResourceId(rn->resource));
	  KheTaskNodeDebug(tn, hg->days_frame, 2, 2, stderr);
	}
	KheTaskNodeAssign(tn, rn->resource, hg);

	/* drop rn, it has everything it needs  */
	HaArrayDeleteAndPlug(cc->resource_nodes, j);
	j--;

	/* drop tn, it is assigned rn now */
	KheDropCurrTaskNode(hg, tn);
      }
    }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassYMatch(KHE_CONSTRAINT_CLASS cc,                   */
/*    int tgi, KHE_HISTORY_GROUPER hg)                                       */
/*                                                                           */
/*  Carry out the YMatch operation from the documentation.  Here S is        */
/*  hg->curr_task_nodes and T_i is hg->next_task_nodes, and i is tgi.        */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassYMatch(KHE_CONSTRAINT_CLASS cc,
  KHE_TIME_GROUP tg, int tgi, KHE_HISTORY_GROUPER hg)
{
  KHE_TASK_NODE dtn, stn;  int i, j, mult, c1, c2, c3;  KHE_MMATCH_NODE sn;

  /* clear ymatch and add demand nodes */
  KheMMatchClear(hg->ymatch);
  HaArrayForEach(hg->curr_task_nodes, dtn, i)
    KheTaskNodeAddMMatchDemandNode(dtn, hg->ymatch);

  /* build supply nodes and add them */
  KheConstraintClassBuildTaskNodes(cc, tg, tgi,
    &hg->next_task_nodes, hg);
  HaArrayForEach(hg->next_task_nodes, stn, j)
    KheTaskNodeAddMMatchSupplyNode(stn, hg->ymatch);

  /* add edges */
  HaArrayForEach(hg->curr_task_nodes, dtn, i)
    if( dtn->interval.last == tgi - 1 )
      HaArrayForEach(hg->next_task_nodes, stn, j)
      {
	c3 = - KheResourceSetIntersectCount(dtn->domain, stn->domain);
	KheMMatchAddEdge(dtn->match_node, stn->match_node, 1, 0, 0, c3);
      }

  /* solve */
  KheMMatchSolve(hg->ymatch);

  /* merge next task nodes that match */
  HaArrayForEach(hg->curr_task_nodes, dtn, i)
    if( dtn->interval.last >= tgi )
    {
      /* dtn cannot match, but it is retained as is */
    }
    else if( KheMMatchResultEdge(dtn->match_node, &sn, &mult, &c1,&c2,&c3) )
    {
      /* dtn is retained with stn added to it */
      stn = (KHE_TASK_NODE) sn;
      KheTaskNodeMerge(dtn, stn);
    }
    else
    {
      /* dtn is dropped */
      KheDropCurrTaskNode(hg, dtn);
      i--;
    }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintClassSolve(KHE_CONSTRAINT_CLASS cc,                     */
/*    KHE_HISTORY_GROUPER hg)                                                */
/*                                                                           */
/*  Solve cc.                                                                */
/*                                                                           */
/*****************************************************************************/
static void KheConstraintClassDebug(KHE_CONSTRAINT_CLASS cc,
  int verbosity, int indent, FILE *fp);

static int KheConstraintClassSolve(KHE_CONSTRAINT_CLASS cc,
  KHE_HISTORY_GROUPER hg)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  int tg_count, tgi, res;
  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  if( DEBUG4 )
  {
    fprintf(stderr, "[ KheConstraintClassSolve(cc, hg); cc =\n");
    KheConstraintClassDebug(cc, 2, 2, stderr);
  }
  res = 0;
  co = HaArrayFirst(cc->constraints);
  tg_count = KheLimitActiveIntervalsConstraintTimeGroupCount(co->laic);
  for( tgi = 0; tgi < tg_count && HaArrayCount(cc->resource_nodes) > 0; tgi++ )
  {
    /* boilerplate */
    tg = KheLimitActiveIntervalsConstraintTimeGroup(co->laic, tgi,
      co->offset, &po);
    if( DEBUG4 )
      fprintf(stderr, "  %d resources, time group %d: %s\n",
	HaArrayCount(cc->resource_nodes), tgi, KheTimeGroupId(tg));

    /* find the current admissible task-set S */
    if( tgi == 0 )
      KheConstraintClassBuildTaskNodes(cc, tg, tgi, &hg->curr_task_nodes, hg);
    else
      KheConstraintClassYMatch(cc, tg, tgi, hg);

    /* match the current resources of interest with S */
    KheConstraintClassXMatch(cc, hg);
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
  HnAssert(HaArrayCount(cc->constraints) > 0,
    "KheConstraintClassDebug internal error");
  if(HaArrayCount(cc->constraints) > 1 || HaArrayCount(cc->resource_nodes) > 0)
  {
    fprintf(fp, "%*s[ ConstraintClass\n", indent, "");
    HaArrayForEach(cc->constraints, co, i)
      KheConstraintPlusOffsetDebug(co, verbosity, indent + 2, fp);
    HaArrayForEach(cc->resource_nodes, rn, i)
      KheResourceNodeDebug(rn, verbosity, indent + 2, fp);
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
/*  Submodule "KHE_HISTORY_GROUPER"                                          */
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
/*  KHE_HISTORY_GROUPER KheHistoryGrouperMake(KHE_SOLN soln,                 */
/*    KHE_RESOURCE_TYPE rt, KHE_OPTIONS options, KHE_TASK_SET r_ts,          */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Make a new history grouper object with these attributes.                 */
/*                                                                           */
/*****************************************************************************/
static void KheHistoryGrouperDebugFn(void *value, int verbosity,
  int indent, FILE *fp);

static KHE_HISTORY_GROUPER KheHistoryGrouperMake(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options, KHE_TASK_SET r_ts, HA_ARENA a)
{
  KHE_HISTORY_GROUPER res;
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->resource_type = rt;
  res->options = options;
  res->days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
  res->etm = (KHE_EVENT_TIMETABLE_MONITOR) KheOptionsGetObject(options,
    "gs_event_timetable_monitor", NULL);
  res->task_set = r_ts;
  HaArrayInit(res->constraint_classes, a);
  /* HaArrayInit(res->resource_nodes, a); */
  HaArrayInit(res->curr_task_nodes, a);
  HaArrayInit(res->next_task_nodes, a);
  res->xmatch = KheMMatchMake((void *) res, &KheHistoryGrouperDebugFn,
    &KheResourceNodeDebugFn, &KheTaskNodeDebugFn, &KheCostDebugFn, a);
  res->ymatch = KheMMatchMake((void *) res, &KheHistoryGrouperDebugFn,
    &KheTaskNodeDebugFn, &KheTaskNodeDebugFn, &KheCostDebugFn, a);
  HaArrayInit(res->tmp_indexes, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitActiveIntervalsConstraintPlusOffsetMatchesDays(             */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic, int offset,                */
/*    KHE_FRAME days_frame)                                                  */
/*                                                                           */
/*  Return true if laic at offset matches days_frame.                        */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitActiveIntervalsConstraintPlusOffsetMatchesDays(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic, int offset,
  KHE_FRAME days_frame)
{
  int i, tg_count;  KHE_TIME_GROUP frame_tg, laic_tg;  KHE_POLARITY po;

  /* constraint must have no more time groups than frame */
  tg_count = KheLimitActiveIntervalsConstraintTimeGroupCount(laic);
  if( KheFrameTimeGroupCount(days_frame) < tg_count )
    return false;

  /* each time group of laic must be a subset of the corresponding frame tg */
  for( i = 0;  i < tg_count;  i++ )
  {
    laic_tg = KheLimitActiveIntervalsConstraintTimeGroup(laic, i, offset, &po);
    frame_tg = KheFrameTimeGroup(days_frame, i);
    if( !KheTimeGroupSubset(laic_tg, frame_tg) )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAddLimitActiveIntervalsConstraintPlusOffset(                     */
/*    KHE_HISTORY_GROUPER hg, KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic,    */
/*    int offset)                                                            */
/*                                                                           */
/*  Add (laic, offset) to the constraint classes of hg.                      */
/*                                                                           */
/*****************************************************************************/

static void KheAddLimitActiveIntervalsConstraintPlusOffset(
  KHE_HISTORY_GROUPER hg, KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic,
  int offset)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  KHE_CONSTRAINT_CLASS cc;  int j;

  /* make co and add it to an existing class if possible */
  co = KheConstraintPlusOffsetMake(laic, offset, hg->arena);
  HaArrayForEach(hg->constraint_classes, cc, j)
    if( KheConstraintClassAcceptsConstraintPlusOffset(cc, co) )
      return;

  /* no match, so make a new class and add co to that */
  cc = KheConstraintClassMake(co, hg->arena);
  HaArrayAddLast(hg->constraint_classes, cc);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheHistoryGrouperBuildConstraintClasses(KHE_HISTORY_GROUPER hg)     */
/*                                                                           */
/*  Build the constraint classes of hg.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheHistoryGrouperBuildConstraintClasses(KHE_HISTORY_GROUPER hg)
{
  KHE_INSTANCE ins;  KHE_CONSTRAINT c;  int i, j, ocount, offset;
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic;
  ins = KheSolnInstance(hg->soln);
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintTag(c) == KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG )
    {
      laic = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c;
      if( KheLimitActiveIntervalsConstraintTimeGroupCount(laic) > 0 &&
	  KheLimitActiveIntervalsConstraintResourceOfTypeCount(laic,
	    hg->resource_type) > 0 &&
	  KheLimitActiveIntervalsConstraintAllPositive(laic) )
      {
	ocount = KheLimitActiveIntervalsConstraintAppliesToOffsetCount(laic);
	for( j = 0;  j < ocount;  j++ )
	{
	  offset = KheLimitActiveIntervalsConstraintAppliesToOffset(laic, j);
	  if( KheLimitActiveIntervalsConstraintPlusOffsetMatchesDays(laic,
	      offset, hg->days_frame) )
	    KheAddLimitActiveIntervalsConstraintPlusOffset(hg, laic, offset);
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheHistoryGrouperDebug(KHE_HISTORY_GROUPER hg, int verbosity,       */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of hg onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheHistoryGrouperDebug(KHE_HISTORY_GROUPER hg, int verbosity,
  int indent, FILE *fp)
{
  KHE_CONSTRAINT_CLASS cc;  int i;
  fprintf(fp, "%*s[ HistoryGrouper(%s, %s)\n", indent, "",
    KheInstanceId(KheSolnInstance(hg->soln)),
    KheResourceTypeId(hg->resource_type));
  HaArrayForEach(hg->constraint_classes, cc, i)
    KheConstraintClassDebug(cc, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheHistoryGrouperDebugFn(void *value, int verbosity,                */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug function for passing to the mmatch module.                         */
/*                                                                           */
/*****************************************************************************/

static void KheHistoryGrouperDebugFn(void *value, int verbosity,
  int indent, FILE *fp)
{
  KHE_HISTORY_GROUPER hg;
  hg = (KHE_HISTORY_GROUPER) value;
  KheHistoryGrouperDebug(hg, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheGroupByHistory"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheGroupByHistory(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,               */
/*    KHE_OPTIONS options, KHE_TASK_SET r_ts)                                */
/*                                                                           */
/*  Group tasks of type rt and assign them resources of type rt to handle    */
/*  history.  Return the number of groups made.                              */
/*                                                                           */
/*****************************************************************************/

int KheGroupByHistory(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_TASK_SET r_ts)
{
  KHE_HISTORY_GROUPER hg;  HA_ARENA a;  int res, i, j;
  KHE_RESOURCE r;  KHE_CONSTRAINT_CLASS cc;

  /* make a history grouper object and build and sort its constraint classes */
  if( DEBUG1 )
    fprintf(stderr, "[ KheGroupByHistory(%s, %s, -, -)\n",
      KheInstanceId(KheSolnInstance(soln)), KheResourceTypeId(rt));
  a = KheSolnArenaBegin(soln);
  hg = KheHistoryGrouperMake(soln, rt, options, r_ts, a);
  KheHistoryGrouperBuildConstraintClasses(hg);
  HaArraySort(hg->constraint_classes, &KheConstraintClassCmp);

  /* add the resource node objects */
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    HaArrayForEach(hg->constraint_classes, cc, j)
      if( KheConstraintClassHasResourceHistory(cc, r, a) )
	break;
  }
  if( DEBUG2 )
    KheHistoryGrouperDebug(hg, 2, 2, stderr);

  /* solve for each constraint class */
  res = 0;
  HaArrayForEach(hg->constraint_classes, cc, i)
    res += KheConstraintClassSolve(cc, hg);

  /* all done */
  if( DEBUG1 )
    fprintf(stderr, "] KheGroupByHistory returning %d\n", res);
  return res;
}

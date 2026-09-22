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
/*  FILE:         khe_sr_interval_grouping.c                                 */
/*  DESCRIPTION:  Interval grouping using dynamic programming                */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
/* #include "khe_t rie.h" */
#include "khe_mmatch.h"
#include <limits.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define bool_show(x) ((x) ? "true" : "false")

#define TRY_OPTIONAL_TASKS_SECOND_RUN 0
#define MAX_KEEP 50000
#define KHE_COL_WIDTH 14

#define DEBUG_COMPATIBLE 1
#define DEBUG_EXPAND_PREV 0

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
/* #define DEBUG5 0 */
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 1		/* print final results as timetables */
#define DEBUG9 1
#define DEBUG10 0
#define DEBUG11 0
#define DEBUG12 0
#define DEBUG13 0
#define DEBUG14 0
#define DEBUG15 0
#define DEBUG16 0
#define DEBUG17 0
#define DEBUG18 0
#define DEBUG19 0
#define DEBUG20 0
#define DEBUG21 0
#define DEBUG22 0
#define DEBUG23 0
#define DEBUG24 0
#define DEBUG25 0		/* also consider DEBUG34 and DEBUG35 */
#define DEBUG26 1
#define DEBUG27 0
#define DEBUG28 0
#define DEBUG29 0

#define DEBUG30(ige, task)						     \
  (false && strcmp(KheIgTimeGroupId(ige->next_igtg), "1Fri4") == 0	     \
   && strcmp(KheTaskId(task), "1Fri:Night.18") == 0 )

#define DEBUG31(ige, igtg)						     \
  (false && strcmp(KheIgTimeGroupId(ige->next_igtg), "1Fri4") == 0	     \
   && strcmp(KheIgTaskGroupId(igtg), "1Thu:Night.12") == 0 )

#define DEBUG32(igtg)						    	     \
  (false && strcmp(KheIgTaskGroupId(igtg), "1Fri:Night.18") == 0)

#define DEBUG33(i)  (false && i == 0)

#define DEBUG34 0
#define DEBUG35(igtg)							     \
  (false && strcmp(KheIgTimeGroupId(igtg), "1Sat4") == 0)

#define DEBUG36 0
#define DEBUG37 0
#define DEBUG38 0
#define DEBUG39 0

#define DEBUG40(igtg)							     \
  (false && strcmp(KheIgTimeGroupId(igtg), "1Sun4") == 0)

#define DEBUG41(igtg, index)						     \
  (false && strcmp(KheIgTimeGroupId(igtg), "1Wed4") == 0 && index == 2)

#define DEBUG42	0
#define DEBUG43	0

#define DEBUG44(igtg)							     \
  (false && strcmp(KheIgTimeGroupId(igtg), "1Mon4") == 0)

#define DEBUG45	0
#define DEBUG47	0
#define DEBUG48	0
#define DEBUG49	0
#define DEBUG50(num) (false && igsv->soln_seq_num == (num))
#define DEBUG51	0
#define DEBUG52	0
#define DEBUG54	0
#define DEBUG56(igtg)							    \
  (false && strcmp(KheIgTimeGroupId(igtg), "1Mon4") == 0)
#define DEBUG57(ige)							    \
  (false && strcmp(KheIgTimeGroupId(ige->next_igtg), "1Mon4") == 0)


/*****************************************************************************/
/*                                                                           */
/*  Forward typedefs                                                         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_ig_time_group_rec *KHE_IG_TIME_GROUP;
typedef HA_ARRAY(KHE_IG_TIME_GROUP) ARRAY_KHE_IG_TIME_GROUP;

typedef struct khe_ig_task_group_rec *KHE_IG_TASK_GROUP;
typedef HA_ARRAY(KHE_IG_TASK_GROUP) ARRAY_KHE_IG_TASK_GROUP;

typedef struct khe_ig_task_group_class_rec *KHE_IG_TASK_GROUP_CLASS;
typedef HA_ARRAY(KHE_IG_TASK_GROUP_CLASS) ARRAY_KHE_IG_TASK_GROUP_CLASS;

typedef struct khe_ig_link_rec *KHE_IG_LINK;
typedef HA_ARRAY(KHE_IG_LINK) ARRAY_KHE_IG_LINK;

typedef struct khe_ig_soln_rec *KHE_IG_SOLN;
typedef HA_ARRAY(KHE_IG_SOLN) ARRAY_KHE_IG_SOLN;

/* ***
typedef struct khe_ig_soln_set_rec *KHE_IG_SOLN_SET;
typedef HA_ARRAY(KHE_IG_SOLN_SET) ARRAY_KHE_IG_SOLN_SET;

typedef struct khe_ig_soln_cache_rec *KHE_IG_SOLN_CACHE;
*** */

typedef struct khe_ig_soln_set_trie_rec *KHE_IG_SOLN_SET_TRIE;
typedef HA_ARRAY(KHE_IG_SOLN_SET_TRIE) ARRAY_KHE_IG_SOLN_SET_TRIE;

typedef struct khe_ig_solver_rec *KHE_IG_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_PLACEMENT - where a task can appear within an interval          */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_PLACEMENT_ANY,
  KHE_PLACEMENT_FIRST_ONLY,
  KHE_PLACEMENT_LAST_ONLY
} KHE_PLACEMENT;


/*****************************************************************************/
/*                                                                           */
/*  Types KHE_IG_MTASK and KHE_IG_TASK - one mtask and one task              */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_ig_mtask_rec  *KHE_IG_MTASK;
typedef HA_ARRAY(KHE_IG_MTASK) ARRAY_KHE_IG_MTASK;

typedef struct khe_ig_task_rec *KHE_IG_TASK;
typedef HA_ARRAY(KHE_IG_TASK) ARRAY_KHE_IG_TASK;

typedef struct khe_ig_mtask_rec {
  KHE_MTASK		mtask;			/* the mtask                 */
  /* int		seq_num; */		/* each mtask has its own    */
  ARRAY_KHE_IG_TASK	included_tasks;		/* included tasks            */
  int			full_durn;		/* full duration             */
  int			primary_durn;		/* p(s)                      */
  KHE_PLACEMENT		placement;		/* q(s)                      */
  bool			finished_cd;		/* these tasks end a group   */
  HA_ARRAY_INT		time_offsets;		/* T'(s)                     */
} *KHE_IG_MTASK;

typedef struct khe_ig_task_rec {
  KHE_TASK		task;			/* the task; d(s), a(s), f(s)*/
  KHE_IG_MTASK		ig_mtask;		/* its mtask                 */
  KHE_COST		non_must_assign_cost;	/* n(s)                      */
  /* bool		equivalent_to_prev; */	/* same mtask etc as prev    */
  bool			optional;		/* o(s)                      */
  KHE_IG_TASK_GROUP	initial_task_group;	/* holding just this igt     */
  bool			expand_used;		/* true if used by expand    */
#if DEBUG_EXPAND_PREV
  KHE_IG_TASK_GROUP	expand_prev;		/* debugging only            */
#endif
} *KHE_IG_TASK;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_IG_TASK_CLASS - a set of equivalent tasks                       */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_ig_task_class_rec {
  ARRAY_KHE_IG_TASK	tasks;			/* the tasks                 */
  int			expand_used_count;	/* number used by expand     */
  ARRAY_KHE_IG_LINK	links;			/* tasks can extend these    */
  /* ARRAY_KHE_IG_TASK_GROUP_CLASS task_group_classes; */
} *KHE_IG_TASK_CLASS;

typedef HA_ARRAY(KHE_IG_TASK_CLASS) ARRAY_KHE_IG_TASK_CLASS;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_IG_COST_TRIE - a trie data structure whose values are costs     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_ig_cost_trie_rec *KHE_IG_COST_TRIE;
typedef HA_ARRAY(KHE_IG_COST_TRIE) ARRAY_KHE_IG_COST_TRIE;

struct khe_ig_cost_trie_rec {
  KHE_COST		value;			/* or -1 if none             */
  ARRAY_KHE_IG_COST_TRIE children;
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_IG_TIME_GROUP - one time group from a constraint class          */
/*                                                                           */
/*  Fields concerned with tasks:                                             */
/*                                                                           */
/*    starting_mtasks                                                        */
/*      These are the mtasks starting during this time group from which      */
/*      the tasks starting during this time group will be drawn.  This       */
/*      value is established at the start of the solve and remains fixed,    */
/*      although the value of each mtask's included tasks may change.        */
/*                                                                           */
/*    starting_tasks (now replaced by task_classes)                          */
/*      These are the tasks starting during this time group.  Unlike         */
/*      starting_mtasks and running_tasks, this field's value is             */
/*      established by KheIgSolverDoSolve, which might be called twice       */
/*      during one solve with different values for starting_tasks:  first    */
/*      without lengthener tasks, then with lengthener tasks.                */
/*                                                                           */
/*    task_classes                                                           */
/*      The task classes whose tasks are starting during this time group.    */
/*      Unlike starting_mtasks and running_tasks, this field's value is      */
/*      established by KheIgSolverDoSolve, which might be called twice       */
/*      during one solve with different values for task_classes:  first      */
/*      without lengthener tasks, then with lengthener tasks.                */
/*                                                                           */
/*    running_tasks                                                          */
/*      These are the tasks (actually just the number of tasks) running      */
/*      during this time group, not including lengthener tasks.  As for      */
/*      starting_mtasks, this value is established at the start of the       */
/*      solve and remains fixed.                                             */
/*                                                                           */
/*  Fields concerned with solutions:                                         */
/*                                                                           */
/*    total_solns                                                            */
/*      The total number of solutions added to this time group               */
/*                                                                           */
/*    undominated_solutions                                                  */
/*      The number of undominated solutions added to this time group.        */
/*                                                                           */
/*    solns                                                                  */
/*      The solutions being kept.  These are all undominated solutions,      */
/*      but their number may be reduced below undominated_solutions, if      */
/*      MAX_KEEP indicates that we want to keep fewer.                       */
/*                                                                           */
/*****************************************************************************/

struct khe_ig_time_group_rec {

  /* general */
  KHE_IG_SOLVER		solver;
  int			index;			/* index in frame            */
  int			offset;			/* offset in frame           */
  KHE_TIME_GROUP	time_group;		/* the time group            */

  /* tasks */
  ARRAY_KHE_IG_MTASK	starting_mtasks;	/* included mtasks starting  */
  /* ARRAY_KHE_IG_TASK	starting_tasks; */	/* included tasks starting   */
  ARRAY_KHE_IG_TASK_CLASS task_classes;
  int			running_tasks;		/* included tasks running    */
  /* ARRAY_KHE_IG_SOLN	solns; */		/* solns (undominated only)  */
  /* KHE_IG_SOLN_CACHE	soln_cache; */		/* solns (undominated only)  */

  /* solutions */
  KHE_IG_SOLN_SET_TRIE  soln_set_trie;		/* trie of soln sets         */
  KHE_IG_COST_TRIE	cost_trie;
  int			total_solns;		/* solns (dominated + undom) */
  int			undominated_solns;	/* solns (undominated)       */
#if DEBUG_COMPATIBLE
  KHE_IG_SOLN		other_igs;		/* from LOR17 or whatever    */
  int			compatible_solns;
#endif
  ARRAY_KHE_IG_SOLN	final_solns;		/* after day ends            */
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_IG_TASK_GROUP - one task group lying in a solution              */
/*                                                                           */
/*  This type inherits type KHE_TASK_GROUPER_ENTRY.  It is the only type     */
/*  anywhere here that inherits KHE_TASK_GROUPER_ENTRY, and so it is quite   */
/*  safe to upcast and downcast (unchecked) between these two types.         */
/*                                                                           */
/*  Field "prev" of KHE_TASK_GROUPER_ENTRY has type KHE_TASK_GROUPER_ENTRY,  */
/*  but here it actually points to an object of type KHE_IG_TASK_GROUP.      */
/*  This is an instance of the "binary methods" issue from object-oriented   */
/*  programming.  It is hard to get right even in O-O, let alone in C.       */
/*                                                                           */
/*  As for type KHE_TASK_GROUPER_ENTRY, one task group is represented by     */
/*  a linked list of KHE_IG_TASK_GROUP objects, one for each day that the    */
/*  task group is running, with the head of the list on the last day and     */
/*  its first element on the first day.  Multi-day tasks are represented     */
/*  by one task group object for each day the task is running.  The task     */
/*  itself appears in the task group entry object for the first day that     */
/*  it is running; on its subsequent days the task group objects are         */
/*  dummy objects.  There may also be a history task group entry if the      */
/*  task group is assigned and thereby linked to a resource's history.       */
/*                                                                           */
/*  The fields defined here, not counting the inherited fields, are:         */
/*                                                                           */
/*    optional                                                               */
/*      True when this task group is optional, that is, when all of its      */
/*      tasks are optional.                                                  */
/*                                                                           */
/*    finished_bcd                                                           */
/*      True when the task in this record must be the last task in its       */
/*      group, for reasons (b), (c), or (d) of the documentation.  The       */
/*      only way a task group can be extended if this is true is when        */
/*      overhang > 0, and then only by continuing with the same task.        */
/*                                                                           */
/*    expand_used                                                            */
/*      Used during expansion of this task group's solution, to say          */
/*      whether this task group has been used (matched with a task).         */
/*                                                                           */
/*    last_of_best_group (overloaded with expand_used)                       */
/*      Used within KheIgSolnAddLengthenerTasks as a temporary flag, true    */
/*      when this object ends a task group in the current best solution.     */
/*                                                                           */
/*    primary_durn                                                           */
/*      The primary duration of the task group (the number of its times      */
/*      that are monitored by the constraint class we are currently solving  */
/*      for).  This includes any history value.  It also includes any        */
/*      primary times in the future that the task is running, i.e. it        */
/*      includes primary times in the overhang.                              */
/*                                                                           */
/*    overhang                                                               */
/*      The number of days in the future that the task of this task group    */
/*      object is running, not including the current day.  For each task     */
/*      (single-day or multiple-day) this number decreases by one on each    */
/*      subsequent day, until on the task's last day its value is 0.         */
/*                                                                           */
/*  If finished_bcd is true and overhang == 0, the group is said to be       */
/*  "self-finished", meaning that it must end here.  All other groups are    */
/*  said to be "non-self-finished".                                          */
/*                                                                           */
/*  When a solution's task groups are sorted, self-finished groups come      */
/*  last.  We make no assumptions about the order that non-self-finished     */
/*  groups appear, except that they precede self-finished groups.            */
/*                                                                           */
/*****************************************************************************/

struct khe_ig_task_group_rec {
  INHERIT_KHE_TASK_GROUPER_ENTRY
  bool			optional;		/* o(g)                      */
  bool			finished_bcd;
  bool			expand_used;		/* true if used by expand    */
  int			index;			/* one element of T'(g)      */
  int			primary_durn;		/* p(g)                      */
  KHE_COST		non_must_assign_cost;	/* n(g)                      */
  int			overhang;
};

#define last_of_best_group expand_used


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_IG_TASK_GROUP_CLASS - a set of equivalent task groups           */
/*                                                                           */
/*****************************************************************************/

struct khe_ig_task_group_class_rec {
  ARRAY_KHE_IG_TASK_GROUP	task_groups;	/* the task groups           */
  ARRAY_KHE_IG_LINK		links;		/* can extend task groups    */
  int				expand_used_count;
  int				class_seq;	/* sequence number           */
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_IG_LINK - a link between a task group class and a task class    */
/*                                                                           */
/*  task_group_class                                                         */
/*    The task group class participating in the link (possibly NULL).        */
/*                                                                           */
/*  task_class                                                               */
/*    The task class participating in the link (possibly NULL).              */
/*                                                                           */
/*  domain                                                                   */
/*    The intersection of the task_group_class and task_class domains.       */
/*                                                                           */
/*  used_count                                                               */
/*    The number of times this link is currently used (i.e. the number of    */
/*    task groups in the current solution of the expansion which consist     */
/*    of a task group from task_group_class plus a task from task_class).    */
/*                                                                           */
/*  blocked_count                                                            */
/*    The number of other links which are currently blocking this link.      */
/*                                                                           */
/*  block_list                                                               */
/*    When this link is in use, these other links must not be in uses; so    */
/*    used_count > 0, then for each link on block_list, blocked_count > 0.   */
/*                                                                           */
/*****************************************************************************/

struct khe_ig_link_rec {
  KHE_IG_TASK_GROUP_CLASS	task_group_class;
  KHE_IG_TASK_CLASS		task_class;
  KHE_RESOURCE_GROUP		domain;
  int				used_count;
  int				blocked_count;
  ARRAY_KHE_IG_LINK		block_list;
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_IG_SOLN - one solution (a set of tasks)                         */
/*                                                                           */
/*  When a KHE_IG_SOLN object is finalized, its task groups are sorted       */
/*  to make dominance testing straightforward.                               */
/*                                                                           */
/*****************************************************************************/

struct khe_ig_soln_rec {
  ARRAY_KHE_IG_TASK_GROUP task_groups;		/* task groups of this soln  */
  KHE_COST		cost;			/* cost of this soln         */
  /* int		varying_domain_groups;*/ /* groups with varying doms */
  int			randomizer;		/* random but deterministic  */
#if DEBUG_COMPATIBLE
  bool			compatible;
  int			cc_index;		/* index of cc               */
#endif
  KHE_IG_SOLN		prev_igs;		/* prev soln if any          */
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_IG_SOLN_SET - a set of solutions                                */
/*                                                                           */
/*****************************************************************************/

/* ***
struct khe_ig_soln_set_rec {
  ARRAY_KHE_IG_SOLN	solns;			** solutions                 **
};
*** */


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_IG_SOLN_CACHE - a set of sets of solutions                      */
/*                                                                           */
/*****************************************************************************/

/* ***
struct khe_ig_soln_cache_rec {
  int			total_solns;		** solns (dominated + undom) **
  int			undominated_solns;	** solns (undominated)       **
#if DEBUG_COMPATIBLE
  KHE_IG_SOLN		other_igs;		** from LOR17 or whatever    **
  int			compatible_solns;
#endif
  ARRAY_KHE_IG_SOLN_SET	soln_sets;		** solution sets             **
  ARRAY_KHE_IG_SOLN	final_solns;		** after day ends            **
};
*** */


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_IG_SOLN_SET_TRIE - a trie whose values are sets of solutions    */
/*                                                                           */
/*****************************************************************************/

struct khe_ig_soln_set_trie_rec {
  ARRAY_KHE_IG_SOLN		solns;		/* solutions at this node    */
  ARRAY_KHE_IG_SOLN_SET_TRIE	children;	/* child nodes               */
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_IG_EXPAND_INFO                                                  */
/*                                                                           */
/*  Used during expansion to hold temporary information.                     */
/*                                                                           */
/*****************************************************************************/

/* ***
typedef struct khe_ig_expand_info_rec {
  KHE_MULTISET_GENERATOR	multiset_generator;
  ARRAY_KHE_IG_TASK_CLASS	task_classes;
  ARRAY_KHE_IG_TASK_GROUP_CLASS	task_group_classes;
  ARRAY_KHE_IG_TASK		tasks;
  ARRAY_KHE_IG_TASK_GROUP	task_groups;
} *KHE_IG_EXPAND_INFO;

typedef HA_ARRAY(KHE_IG_EXPAND_INFO) ARRAY_KHE_IG_EXPAND_INFO;
*** */


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_IG_EXPAND_NODE - a node in the expand graph                     */
/*                                                                           */
/*****************************************************************************/

/* ***
typedef struct khe_ig_expand_node_rec {
  KHE_IG_TASK_GROUP_CLASS		task_group_class;
} *KHE_IG_EXPAND_NODE;

typedef HA_ARRAY(KHE_IG_EXPAND_NODE) ARRAY_KHE_IG_EXPAND_NODE;
*** */


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_IG_EXPANDER - info used by KheIgSolnExpand                      */
/*                                                                           */
/*  This object holds information used when expanding a solution by one day: */
/*                                                                           */
/*    solver                                                                 */
/*      The enclosing solver                                                 */
/*                                                                           */
/*    prev_igs                                                               */
/*      The solution we are currently expanding                              */
/*                                                                           */
/*    prev_task_group_classes                                                */
/*      The task groups of prev_igs, grouped into equivalence classes        */
/*                                                                           */
/*    next_igs                                                               */
/*      The solution we are currently building, which may eventually         */
/*      get copied and stored in next_igtg                                   */
/*                                                                           */
/*    next_igtg                                                              */
/*      The day after prev_igs's day; the day next_igs is for.               */
/*                                                                           */
/*    all_links                                                              */
/*      An array of all links built for use during the current expansion.    */
/*      This is used only to simplify freeing them afterwards.               */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_MMATCH_NODE) ARRAY_KHE_MMATCH_NODE;

typedef struct khe_ig_expander_rec {
  KHE_IG_SOLVER			solver;
  KHE_IG_SOLN			prev_igs;
  ARRAY_KHE_IG_TASK_GROUP_CLASS	prev_task_group_classes;
  KHE_IG_SOLN			next_igs;
  KHE_IG_TIME_GROUP		next_igtg;
  ARRAY_KHE_IG_LINK		all_links;
} *KHE_IG_EXPANDER;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_IG_SOLVER - interval grouping by dynamic programming solver     */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_CONSTRAINT_CLASS) ARRAY_KHE_CONSTRAINT_CLASS;
/* typedef KHE_TRIE(KHE_COST) KHE_TRIE_COST; */

struct khe_ig_solver_rec {

  /* free lists */
  ARRAY_KHE_IG_MTASK			ig_mtask_free_list;
  ARRAY_KHE_IG_TASK			ig_task_free_list;
  ARRAY_KHE_IG_TASK_CLASS		ig_task_class_free_list;
  ARRAY_KHE_IG_TIME_GROUP		ig_time_group_free_list;
  ARRAY_KHE_IG_COST_TRIE		ig_cost_trie_free_list;
  ARRAY_KHE_IG_TASK_GROUP		ig_task_group_free_list;
  ARRAY_KHE_IG_TASK_GROUP_CLASS		ig_task_group_class_free_list;
  ARRAY_KHE_IG_LINK			ig_link_free_list;
  ARRAY_KHE_IG_SOLN			ig_soln_free_list;
  ARRAY_KHE_IG_SOLN_SET_TRIE		ig_soln_set_trie_free_list;
  /* ARRAY_KHE_IG_SOLN_SET		ig_soln_set_free_list; */
  /* ARRAY_KHE_IG_EXPAND_INFO		ig_expand_info_free_list; */
  /* ARRAY_KHE_IG_EXPAND_NODE		ig_expand_node_free_list; */

  /* fields defined (and mostly constant) throughout the solve */
  HA_ARENA				arena;
  KHE_SOLN				soln;
  KHE_RESOURCE_TYPE			resource_type;
  KHE_OPTIONS				options;
  int					ig_min;
  int					ig_range;
  KHE_MTASK_FINDER			mtask_finder;
  KHE_FRAME				days_frame;
  KHE_TASK_GROUP_DOMAIN_FINDER		domain_finder;
  KHE_SOLN_ADJUSTER			soln_adjuster;
  KHE_COST				marginal_cost;
  KHE_CONSTRAINT_CLASS_FINDER		candidate_ccf;
  KHE_CONSTRAINT_CLASS_FINDER		busy_days_ccf;
  KHE_CONSTRAINT_CLASS_FINDER		complete_weekends_ccf;
  int					groups_count;
  /* KHE_TRIE_COST			cost_trie; */

  /* fields defined when solving one constraint class */
  KHE_CONSTRAINT_CLASS			curr_class;
  int					cc_index;
#if DEBUG_COMPATIBLE
  /* KHE_GROUPED_TASKS_DISPLAY		other_gtd; */
#endif
  int					min_limit;
  int					max_limit;
  int					soln_seq_num;
  /* int				mtask_seq_num; */
  ARRAY_KHE_CONSTRAINT_CLASS		busy_days_classes;
  ARRAY_KHE_IG_TIME_GROUP		time_groups;
  ARRAY_KHE_IG_TASK			included_tasks;
  /* KHE_INTERVAL_COST_TABLE		cost_table; */

  /* scratch fields */
  KHE_IG_EXPANDER			expander;
  /* HA_ARRAY_INT			tmp_indexes1; */
  /* HA_ARRAY_INT			tmp_indexes2; */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Constraint classes"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassFindCandidates(KHE_SOLN soln,                     */
/*    KHE_RESOURCE_TYPE rt, KHE_FRAME days_frame,                            */
/*    KHE_CONSTRAINT_CLASS_FINDER candidate_ccf,                             */
/*    KHE_CONSTRAINT_CLASS_FINDER busy_days_ccf)                             */
/*                                                                           */
/*  Find candidates for the constraint classes needed by the implementation, */
/*  including checking conditions (1), (2), and (3) of the documentation.    */
/*  Also build busy days classes.                                            */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassFindCandidates(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_FRAME days_frame,
  KHE_CONSTRAINT_CLASS_FINDER candidate_ccf,
  KHE_CONSTRAINT_CLASS_FINDER busy_days_ccf)
{
  int i, j, count, offset;  KHE_INSTANCE ins;  KHE_CONSTRAINT c;
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic;
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintTag(c) == KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG &&
	KheConstraintCombinedWeight(c) > 0 )
    {
      laic = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c;
      if( KheLimitActiveIntervalsConstraintResourceOfTypeCount(laic, rt) > 0 &&
	  KheLimitActiveIntervalsConstraintTimeGroupCount(laic) > 0 &&
	  KheLimitActiveIntervalsConstraintAllPositive(laic) )		/* 3 */
      {
	count = KheLimitActiveIntervalsConstraintAppliesToOffsetCount(laic);
	for( j = 0;  j < count;  j++ )
	{
	  offset = KheLimitActiveIntervalsConstraintAppliesToOffset(laic, j);
	  if( KheConstraintTimeGroupsSubsetFrame(c,offset,days_frame) ) /* 1 */
	  {
	    if( KheConstraintTimeGroupsAllSingletons(c) )		/* 2 */
	      KheConstraintClassFinderAddConstraint(candidate_ccf, c, offset);
	    else
	      KheConstraintClassFinderAddConstraint(busy_days_ccf, c, offset);
	  }
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintClassIsSelected(KHE_CONSTRAINT_CLASS candidate_cc,     */
/*    int ig_min, int ig_range)                                              */
/*                                                                           */
/*  Return true if candidate_cc should be selected for interval grouping.    */
/*  This code checks conditions (4), (5), and (6) of the documentation.      */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintClassIsSelected(KHE_CONSTRAINT_CLASS candidate_cc,
  int ig_min, int ig_range)
{
  int minimum, maximum;
  minimum = KheConstraintClassMinimum(candidate_cc);
  maximum = KheConstraintClassMaximum(candidate_cc);
  if( DEBUG15 )
  {
    fprintf(stderr,
      "[ KheConstraintClassIsSelected(candidate_cc min %d max %d, igsv)\n",
      minimum, maximum);
    KheConstraintClassDebug(candidate_cc, 2, 2, stderr);
    fprintf(stderr, "] KheConstraintClassIsSelected returning"
      " %s && %s && %s && %s\n", bool_show(minimum >= ig_min),
      bool_show(maximum - minimum <= ig_range),
      bool_show(KheConstraintClassHasUniformLimits(candidate_cc)),
      bool_show(KheConstraintClassCoversResourceType(candidate_cc)));
  }
  return minimum >= ig_min && maximum - minimum <= ig_range &&      /* 6 */
    KheConstraintClassHasUniformLimits(candidate_cc) &&             /* 5 */
    KheConstraintClassCoversResourceType(candidate_cc);             /* 4 */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgSelectedClassAcceptsBusyDaysClass(KHE_CONSTRAINT_CLASS cc,     */
/*    KHE_CONSTRAINT_CLASS busy_days_cc)                                     */
/*                                                                           */
/*  Return true if selected class cc is linkable to busy_days_cc:  if cc's   */
/*  time groups are subsets of the corresponding time groups of              */
/*  busy_days_cc, and busy_days_cc's weight is larger.                       */
/*                                                                           */
/*  This function follows KheHistoryInstanceAcceptsBusyDaysClass from        */
/*  khe_sr_assign_by_history.c.  It is the same function except that         */
/*  its first parameter is the class, not the solver holding the class.      */
/*                                                                           */
/*****************************************************************************/

static bool KheIgSelectedClassAcceptsBusyDaysClass(KHE_CONSTRAINT_CLASS cc,
  KHE_CONSTRAINT_CLASS busy_days_cc)
{
  int tg_count1, tg_count2, i;  KHE_TIME_GROUP tg1, tg2;
  KHE_POLARITY po1, po2;  KHE_COST weight1, weight2;

  /* check time groups */
  tg_count1 = KheConstraintClassTimeGroupCount(cc);
  tg_count2 = KheConstraintClassTimeGroupCount(busy_days_cc);
  HnAssert(tg_count1 == tg_count2,
    "KheIgSelectedClassAcceptsBusyDaysClass internal error");
  for( i = 0;  i < tg_count1;  i++ )
  {
    tg1 = KheConstraintClassTimeGroup(cc, i, &po1);
    tg2 = KheConstraintClassTimeGroup(busy_days_cc, i, &po2);
    if( !KheTimeGroupSubset(tg1, tg2) )
      return false;
  }

  /* check weight */
  weight1 = KheConstraintClassCombinedWeight(cc);
  weight2 = KheConstraintClassCombinedWeight(busy_days_cc);
  if( weight2 <= weight1 )
    return false;

  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_PLACEMENT"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalsDefinePlacement(KHE_INTERVAL full_in,                   */
/*    KHE_INTERVAL primary_in, KHE_PLACEMENT *placement)                     */
/*                                                                           */
/*  If full_in and primary_in together define a placement, set *placement    */
/*  to that placement and return true.  Otherwise return false.  Note that   */
/*  an empty primary_in is acceptable and has "any" placement.               */
/*                                                                           */
/*****************************************************************************/

static bool KheIntervalsDefinePlacement(KHE_INTERVAL full_in,
  KHE_INTERVAL primary_in, KHE_PLACEMENT *placement)
{
  if( KheIntervalEmpty(primary_in) )
  {
    return *placement = KHE_PLACEMENT_ANY, true;
  }
  else if( primary_in.first == full_in.first )
  {
    if( primary_in.last == full_in.last )
      return *placement = KHE_PLACEMENT_ANY, true;
    else
      return *placement = KHE_PLACEMENT_LAST_ONLY, true;
  }
  else
  {
    if( primary_in.last == full_in.last )
      return *placement = KHE_PLACEMENT_FIRST_ONLY, true;
    else
      return *placement = KHE_PLACEMENT_ANY, false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KhePlacementShow(KHE_PLACEMENT placement)                          */
/*                                                                           */
/*  Show placement.                                                          */
/*                                                                           */
/*****************************************************************************/

static char *KhePlacementShow(KHE_PLACEMENT placement)
{
  switch( placement )
  {
    case KHE_PLACEMENT_ANY:		return "any";
    case KHE_PLACEMENT_FIRST_ONLY:	return "first_only";
    case KHE_PLACEMENT_LAST_ONLY:	return "last_only";
    default:				return "??";
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_MTASK"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_MTASK KheIgMTaskMake(KHE_MTASK mt, int seq_num,                   */
/*    int primary_durn, KHE_PLACEMENT pl, KHE_IG_SOLVER igsv)                */
/*                                                                           */
/*  Make a new interval grouping mtask object with these attributes,         */
/*  and initially no tasks.  These objects are only made for mtasks          */
/*  whose tasks are admissible.                                              */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_MTASK KheIgMTaskMake(KHE_MTASK mt, /* int seq_num, */
  int primary_durn, KHE_PLACEMENT pl, KHE_IG_SOLVER igsv)
{
  KHE_IG_MTASK res;  KHE_TIME_SET ts;  int i;  KHE_TIME t;  KHE_INTERVAL in;
  if( HaArrayCount(igsv->ig_mtask_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(igsv->ig_mtask_free_list);
    HaArrayClear(res->included_tasks);
    HaArrayClear(res->time_offsets);
  }
  else
  {
    HaMake(res, igsv->arena);
    HaArrayInit(res->included_tasks, igsv->arena);
    HaArrayInit(res->time_offsets, igsv->arena);
  }
  res->mtask = mt;
  /* res->seq_num = seq_num; */
  res->full_durn = KheMTaskTotalDuration(mt);
  res->primary_durn = primary_durn;
  res->placement = pl;
  in = KheMTaskInterval(mt);
  res->finished_cd = (pl == KHE_PLACEMENT_LAST_ONLY ||
    in.last == KheFrameTimeGroupCount(igsv->days_frame) - 1);

  /* set time_offsets, in reverse order */
  ts = KheMTaskTimeSet(mt);
  for( i = KheTimeSetTimeCount(ts) - 1;  i >= 0;  i-- )
  {
    t = KheTimeSetTime(ts, i);
    HaArrayAddLast(res->time_offsets, KheFrameTimeOffset(igsv->days_frame, t));
  }

  if( DEBUG29 )
    fprintf(stderr, "  KheIgMTaskMake(%s, full_durn %d, primary_durn %d, pl %s"
      ", finished_cd %s)\n", KheMTaskId(mt), res->full_durn,
      primary_durn, KhePlacementShow(pl), bool_show(res->finished_cd));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgMTaskFree(KHE_IG_MTASK igmt, KHE_IG_SOLVER igsv)               */
/*                                                                           */
/*  Free igmt and its ig tasks.                                              */
/*                                                                           */
/*****************************************************************************/
static void KheIgTaskFree(KHE_IG_TASK igt, KHE_IG_SOLVER igsv);

static void KheIgMTaskFree(KHE_IG_MTASK igmt, KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK igt;  int i;
  HaArrayForEach(igmt->included_tasks, igt, i)
    KheIgTaskFree(igt, igsv);
  HaArrayAddLast(igsv->ig_mtask_free_list, igmt);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK KheIgMTaskAddTask(KHE_IG_MTASK igmt, KHE_TASK task,          */
/*    bool non_must_assign, KHE_IG_SOLVER igsv)                              */
/*                                                                           */
/*  Add task to igmt->included_tasks and igsv->included_tasks.  Return       */
/*  the new ig task.                                                         */
/*                                                                           */
/*****************************************************************************/
static KHE_IG_TASK KheIgTaskMake(KHE_TASK task, KHE_IG_MTASK igmt,
  bool non_must_assign, KHE_IG_SOLVER igsv);

static KHE_IG_TASK KheIgMTaskAddTask(KHE_IG_MTASK igmt, KHE_TASK task,
  bool non_must_assign, KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK res;  int index;

  /* make the new ig task */
  res = KheIgTaskMake(task, igmt, non_must_assign, igsv);

  /* add the new ig task to igsv */
  index = KheTaskSolnIndex(task);
  HaArrayFill(igsv->included_tasks, index + 1, NULL);
  HnAssert(HaArray(igsv->included_tasks, index) == NULL,
    "KheIgMTaskAddTask internal error");
  HaArrayPut(igsv->included_tasks, index, res);

  /* add the new ig task to igmt and return it */
  HaArrayAddLast(igmt->included_tasks, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskIsAdmissible(KHE_MTASK mt, KHE_IG_SOLVER igsv,              */
/*    KHE_PLACEMENT *pl, int *primary_durn)                                  */
/*                                                                           */
/*  If mt is admissible (if its tasks are admissible), then return true      */
/*  with *pl and *primary_durn set appropriately.  Else return false.        */
/*                                                                           */
/*  Admissibility conditions (1) and (2) from the documentation are already  */
/*  established when this function is called, because of where mt comes      */
/*  from.  This function checks conditions (3), (4), and (5).                */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskIsAdmissible(KHE_MTASK mt, KHE_IG_SOLVER igsv,
  KHE_PLACEMENT *pl, int *primary_durn)
{
  typedef enum {
    INTERVAL_BEFORE,
    INTERVAL_WITHIN,
    INTERVAL_AFTER,
  } INTERVAL_STATE;

  KHE_TIME_SET ts;  KHE_TIME first_time, time;
  int first_index, i, in_first, in_last, pos;
  KHE_TIME_GROUP frame_tg, cc_tg;  KHE_POLARITY po;  INTERVAL_STATE state;
  bool in_cc_tg;  KHE_INTERVAL full_in, primary_in;

  /* condition (3): mt must have no overlaps and no gaps */
  if( DEBUG14 )
    fprintf(stderr, "[ KheMTaskIsAdmissible(%s, igsv, cc, -)\n",
      KheMTaskId(mt));
  if( !KheMTaskNoOverlap(mt) )
  {
    if( DEBUG14 )
      fprintf(stderr, "] KheMTaskIsAdmissible returning false (overlap)\n");
    return false;
  }
  if( !KheMTaskNoGaps(mt) )
  {
    if( DEBUG14 )
      fprintf(stderr, "] KheMTaskIsAdmissible returning false (gaps)\n");
    return false;
  }

  /* condition (4): mt must contain at least one assigned time */
  ts = KheMTaskTimeSet(mt);
  if( KheTimeSetTimeCount(ts) == 0 )
  {
    if( DEBUG14 )
      fprintf(stderr, "] KheMTaskIsAdmissible returning false (no times)\n");
    return false;
  }
  if( DEBUG14 )
  {
    fprintf(stderr, "time set: ");
    KheTimeSetDebug(ts, 2, 0, stderr);
  }

  /* condition (5): primary times (times monitored by igsv->curr_class) must */
  /* be adjacent and at one end.  First, find the day holding the first time */
  first_time = KheTimeSetTime(ts, 0);
  first_index = KheFrameTimeIndex(igsv->days_frame, first_time);

  /* find the interval of days that ts intersects with igsv->curr_class */
  state = INTERVAL_BEFORE;
  in_first = in_last = -1;  /* keep compiler happy */
  for( i = 0;  i < KheTimeSetTimeCount(ts);  i++ )
  {
    time = KheTimeSetTime(ts, i);
    frame_tg = KheFrameTimeGroup(igsv->days_frame, first_index + i);
    HnAssert(KheTimeGroupContains(frame_tg, time, &pos),
      "KheMTaskIsAdmissible internal error (time %s, frame_tg %s)\n",
      KheTimeId(time), KheTimeGroupId(frame_tg));
    cc_tg = KheConstraintClassTimeGroup(igsv->curr_class, first_index + i, &po);
    in_cc_tg = KheTimeGroupContains(cc_tg, time, &pos);
    switch( state )
    {
      case INTERVAL_BEFORE:

	if( in_cc_tg )
	{
	  in_first = first_index + i;
	  state = INTERVAL_WITHIN;
	}
	break;

      case INTERVAL_WITHIN:

	if( !in_cc_tg )
	{
	  in_last = first_index + i - 1;
          state = INTERVAL_AFTER;
	}
	break;

      case INTERVAL_AFTER:

	if( in_cc_tg )
	{
	  if( DEBUG14 )
	    fprintf(stderr, "] KheMTaskIsAdmissible returning false (%s)\n",
	      KheTimeId(time));
	  return false;  /* monitored times not adjacent */
	}
	break;

      default:

	HnAbort("KheMTaskIsAdmissible internal error");
	break;
    }
  }

  /* set primary_in, the interval of days holding the primary times */
  switch( state )
  {
    case INTERVAL_BEFORE:

      primary_in = KheIntervalMake(1, 0);
      break;

    case INTERVAL_WITHIN:

      primary_in = KheIntervalMake(in_first, first_index + i - 1);
      break;

    case INTERVAL_AFTER:

      primary_in = KheIntervalMake(in_first, in_last);
      break;

    default:

      HnAbort("KheMTaskIsAdmissible internal error");
      break;
  }

  /* sort out mt's placement and return */
  full_in = KheMTaskInterval(mt);
  if( !KheIntervalsDefinePlacement(full_in, primary_in, pl) )
  {
    if( DEBUG14 )
      fprintf(stderr, "] KheMTaskIsAdmissible returning false (pl): "
	"full %s, class %s\n", KheIntervalShow(full_in, igsv->days_frame),
	KheIntervalShow(primary_in, igsv->days_frame));
    return false;    /* monitored times not at either end */
  }
  *primary_durn = KheIntervalLength(primary_in);
  if( DEBUG14 )
    fprintf(stderr, "] KheMTaskIsAdmissible returning true\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIgMTaskIndex(KHE_IG_MTASK igmt, int overhang)                     */
/*                                                                           */
/*  Return the correct index for a task group holding a task of igmt with    */
/*  this overhang.                                                           */
/*                                                                           */
/*****************************************************************************/

static int KheIgMTaskIndex(KHE_IG_MTASK igmt, int overhang)
{
  return HaArray(igmt->time_offsets, overhang);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgMTaskDebugHeader(KHE_IG_MTASK igmt, FILE *fp)                  */
/*                                                                           */
/*  Debug print of the header of igmt onto fp.                               */
/*                                                                           */
/*****************************************************************************/

static void KheIgMTaskDebugHeader(KHE_IG_MTASK igmt, FILE *fp)
{
  fprintf(fp, "IgMTask(%s, included %d, primary_durn %d, pl %s)",
    KheMTaskId(igmt->mtask), HaArrayCount(igmt->included_tasks),
    /* KheIntervalShow(igmt->full_in, frame), */
    /* KheIntervalShow(igmt->primary_in, frame), */
    igmt->primary_durn, KhePlacementShow(igmt->placement));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgMTaskDebug(KHE_IG_MTASK igmt, KHE_FRAME frame,                 */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of igmt onto fp.                                             */
/*                                                                           */
/*****************************************************************************/
static void KheIgTaskDebug(KHE_IG_TASK igt, int verbosity, int indent,
  FILE *fp);

static void KheIgMTaskDebug(KHE_IG_MTASK igmt,
  int verbosity, int indent, FILE *fp)
{
  KHE_IG_TASK igt;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheIgMTaskDebugHeader(igmt, fp);
    fprintf(fp, "\n");
    HaArrayForEach(igmt->included_tasks, igt, i)
      KheIgTaskDebug(igt, verbosity, indent + 2, fp);
    /* KheMTaskDebug(igmt->mtask, verbosity, indent + 2, fp); */
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheIgMTaskDebugHeader(igmt, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_TASK"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK KheIgTaskMake(KHE_TASK task, KHE_IG_MTASK igmt,              */
/*    bool non_must_assign, KHE_IG_SOLVER igsv)                              */
/*                                                                           */
/*  Return an interval grouping task object with these attributes.  Do       */
/*  not add it to anything (KheIgMTaskAddTask does that).                    */
/*                                                                           */
/*****************************************************************************/
static KHE_IG_TASK_GROUP KheIgTaskGroupMakeInitial(KHE_IG_TASK igt,
  KHE_IG_SOLVER igsv);

static KHE_IG_TASK KheIgTaskMake(KHE_TASK task, KHE_IG_MTASK igmt,
  bool non_must_assign, KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK res;  KHE_COST non_asst_cost, asst_cost;
  if( HaArrayCount(igsv->ig_task_free_list) > 0 )
    res = HaArrayLastAndDelete(igsv->ig_task_free_list);
  else
    HaMake(res, igsv->arena);
  res->task = task;
  res->ig_mtask = igmt;
  KheTaskNonAsstAndAsstCost(task, &non_asst_cost, &asst_cost);
  if( non_must_assign )
    res->non_must_assign_cost = (asst_cost - non_asst_cost) +
      igsv->marginal_cost * KheTaskTotalDuration(task);
  else
    res->non_must_assign_cost = 0;
  res->optional = igmt->primary_durn == 0 || (non_asst_cost <= asst_cost);
  res->initial_task_group = KheIgTaskGroupMakeInitial(res, igsv);
  res->expand_used = false;
#if DEBUG_EXPAND_PREV
  res->expand_prev = NULL;
#endif
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskFree(KHE_IG_TASK igt, KHE_IG_SOLVER igsv)                  */
/*                                                                           */
/*  Free igt.                                                                */
/*                                                                           */
/*****************************************************************************/
static void KheIgTaskGroupFree(KHE_IG_TASK_GROUP igtg, KHE_IG_SOLVER igsv);

static void KheIgTaskFree(KHE_IG_TASK igt, KHE_IG_SOLVER igsv)
{
  KheIgTaskGroupFree(igt->initial_task_group, igsv);
  HaArrayAddLast(igsv->ig_task_free_list, igt);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskHasFixedNonAsst(KHE_IG_TASK igt)                           */
/*                                                                           */
/*  Return true if igt has a fixed non-assignment.                           */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskHasFixedNonAsst(KHE_IG_TASK igt)
{
  return KheTaskAssignIsFixed(igt->task) &&
    KheTaskAsstResource(igt->task) == NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskSameClass(KHE_IG_TASK igt1, KHE_IG_TASK igt2,              */
/*    bool include_domain_in_test)                                           */
/*                                                                           */
/*  Return true if igt1 and igt2 belong to the same equivalence class.       */
/*  If include_domain_in_test is true, include their domains in the test,    */
/*  otherwise omit domains.                                                  */
/*                                                                           */
/*****************************************************************************/
static bool KheArrayIntEqual(HA_ARRAY_INT *array_int1,HA_ARRAY_INT *array_int2);

static bool KheIgTaskSameClass(KHE_IG_TASK igt1, KHE_IG_TASK igt2,
  bool include_domain_in_test)
{
  KHE_IG_MTASK igmt1, igmt2;  KHE_RESOURCE_GROUP rg1, rg2;
  igmt1 = igt1->ig_mtask;
  igmt2 = igt2->ig_mtask;
  if( igmt1 == igmt2 )
  {
    /* shortcut version for when the two tasks lie in the same mtask */
    return KheTaskAsstResource(igt1->task) == KheTaskAsstResource(igt2->task) &&
      KheIgTaskHasFixedNonAsst(igt1) == KheIgTaskHasFixedNonAsst(igt2) &&
      igt1->optional == igt2->optional &&
      igt1->non_must_assign_cost == igt2->non_must_assign_cost;
  }
  else
  {
    /* full version for when the two tasks lie in different mtasks */
    rg1 = KheTaskDomain(igt1->task);
    rg2 = KheTaskDomain(igt2->task);
    return KheArrayIntEqual(&igmt1->time_offsets, &igmt2->time_offsets) &&
      (include_domain_in_test ? KheResourceGroupEqual(rg1, rg2) : true) &&
      KheTaskAsstResource(igt1->task) == KheTaskAsstResource(igt2->task) &&
      KheIgTaskHasFixedNonAsst(igt1) == KheIgTaskHasFixedNonAsst(igt2) &&
      igt1->optional == igt2->optional &&
      igt1->non_must_assign_cost == igt2->non_must_assign_cost &&
      igmt1->placement == igmt2->placement;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskExpandReset(KHE_IG_TASK igt)                               */
/*                                                                           */
/*  Reset igt for a new expand.                                              */
/*                                                                           */
/*****************************************************************************/

static void KheIgTaskExpandReset(KHE_IG_TASK igt)
{
  igt->expand_used = false;
#if DEBUG_EXPAND_PREV
  igt->expand_prev = NULL;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheIgTaskId(KHE_IG_TASK igt)                                       */
/*                                                                           */
/*  Return the Id of igt.                                                    */
/*                                                                           */
/*****************************************************************************/

static char *KheIgTaskId(KHE_IG_TASK igt)
{
  return KheTaskId(igt->task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskDebug(KHE_IG_TASK igt, int verbosity, int indent, FILE *fp)*/
/*                                                                           */
/*  Debug printf of igt with the given verbosity and indent.                 */
/*                                                                           */
/*****************************************************************************/

static void KheIgTaskDebug(KHE_IG_TASK igt, int verbosity, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "IgTask(%s, %s, nma_cost %.5f%s%s%s)", KheIgTaskId(igt),
    KheResourceGroupId(KheTaskDomain(igt->task)),
    KheCostShow(igt->non_must_assign_cost),
    igt->optional ? ", optional" : "",
    igt->expand_used ? ", expand_used:" : "",
#if DEBUG_EXPAND_PREV
    igt->expand_prev == NULL ? "" :
      igt->expand_prev->task == NULL ? "notask" :
      KheTaskId(igt->expand_prev->task)
#else
    ""
#endif
  );
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_TASK_CLASS"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK_CLASS KheIgTaskClassMake(KHE_IG_SOLVER igsv)                 */
/*                                                                           */
/*  Make a new ig task class object.                                         */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_TASK_CLASS KheIgTaskClassMake(KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK_CLASS res;
  if( HaArrayCount(igsv->ig_task_class_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(igsv->ig_task_class_free_list);
    HaArrayClear(res->tasks);
    HaArrayClear(res->links);
  }
  else
  {
    HaMake(res, igsv->arena);
    HaArrayInit(res->tasks, igsv->arena);
    HaArrayInit(res->links, igsv->arena);
  }
  res->expand_used_count = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskClassFree(KHE_IG_TASK_CLASS igtc, KHE_IG_SOLVER igsv)      */
/*                                                                           */
/*  Free igtc.                                                               */
/*                                                                           */
/*****************************************************************************/

/* *** correct but currently unused
static void KheIgTaskClassFree(KHE_IG_TASK_CLASS igtc, KHE_IG_SOLVER igsv)
{
  HaArrayAddLast(igsv->ig_task_class_free_list, igtc);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskClassAcceptsTask(KHE_IG_TASK_CLASS igtc, KHE_IG_TASK igt)  */
/*                                                                           */
/*  Return true if igtc accepts igt.                                         */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskClassAcceptsTask(KHE_IG_TASK_CLASS igtc, KHE_IG_TASK igt)
{
  KHE_IG_TASK igt2;
  HnAssert(HaArrayCount(igtc->tasks) > 0,
    "KheIgTaskClassAcceptsTask internal error");
  igt2 = HaArrayFirst(igtc->tasks);
  return KheIgTaskSameClass(igt, igt2, true);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskClassAddTask(KHE_IG_TASK_CLASS igtc, KHE_IG_TASK igt)      */
/*                                                                           */
/*  Add igt to igtc.                                                         */
/*                                                                           */
/*****************************************************************************/

static void KheIgTaskClassAddTask(KHE_IG_TASK_CLASS igtc, KHE_IG_TASK igt)
{
  HaArrayAddLast(igtc->tasks, igt);
  /* *** actually igt->expand_used is undefined here
  HnAssert(!igt->expand_used, "KheIgTaskClassAddTask internal error");
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIgTaskClassExpandUnusedCount(KHE_IG_TASK_CLASS igtc)              */
/*                                                                           */
/*  Return the number of tasks of igtc whose expand_used field is false.     */
/*                                                                           */
/*****************************************************************************/

static int KheIgTaskClassExpandUnusedCount(KHE_IG_TASK_CLASS igtc)
{
  return HaArrayCount(igtc->tasks) - igtc->expand_used_count;
  /* ***
  KHE_IG_TASK igt;  int i, res;
  res = 0;
  HaArrayForEach(igtc->tasks, igt, i)
    if( !igt->expand_used )
      res++;
  return res;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskClassExpandReset(KHE_IG_TASK_CLASS igtc,                   */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Reset igtc, ready for a new expansion.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheIgTaskClassExpandReset(KHE_IG_TASK_CLASS igtc, KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK igt;  int i;
  HaArrayForEach(igtc->tasks, igt, i)
    KheIgTaskExpandReset(igt);
  igtc->expand_used_count = 0;
  /* done separately HaArrayAppend(igsv->ig_link_free_list, igtc->links, i); */
  HaArrayClear(igtc->links);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskClassAddLink(KHE_IG_TASK_CLASS igtc, KHE_IG_LINK link)     */
/*                                                                           */
/*  Add link to igtc.                                                        */
/*                                                                           */
/*****************************************************************************/

static void KheIgTaskClassAddLink(KHE_IG_TASK_CLASS igtc, KHE_IG_LINK link)
{
  HaArrayAddLast(igtc->links, link);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskClassDebug(KHE_IG_TASK_CLASS igtc, int verbosity,          */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of igtc onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

static void KheIgTaskClassDebug(KHE_IG_TASK_CLASS igtc, int verbosity,
  int indent, FILE *fp)
{
  KHE_IG_TASK igt;  int i;
  if( indent < 0 )
    fprintf(fp, "TaskClass");
  else
  {
    fprintf(fp, "%*s[ TaskClass (expand_used_count %d)\n", indent, "",
      igtc->expand_used_count);
    HaArrayForEach(igtc->tasks, igt, i)
      KheIgTaskDebug(igt, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_COST_TRIE"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheIgCostTrieClear(KHE_IG_COST_TRIE trie)                           */
/*                                                                           */
/*  Clear trie.                                                              */
/*                                                                           */
/*****************************************************************************/

static void KheIgCostTrieClear(KHE_IG_COST_TRIE trie, KHE_IG_SOLVER igsv)
{
  KHE_IG_COST_TRIE child;  int i;
  if( trie != NULL )
  {
    HaArrayForEach(trie->children, child, i)
      KheIgCostTrieClear(child, igsv);
    HaArrayAddLast(igsv->ig_cost_trie_free_list, trie);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgCostTrieAdd(KHE_IG_COST_TRIE *trie, KHE_IG_TASK_GROUP key,     */
/*    KHE_COST value, KHE_IG_SOLVER igsv)                                    */
/*                                                                           */
/*  Add a new (key, value) pair, where key holds the sequence of indexes,    */
/*  and value is a cost.                                                     */
/*                                                                           */
/*****************************************************************************/
static KHE_IG_TASK_GROUP KheIgTaskGroupPrev(KHE_IG_TASK_GROUP igtg);

static void KheIgCostTrieAdd(KHE_IG_COST_TRIE *trie, KHE_IG_TASK_GROUP key,
  KHE_COST value, KHE_IG_SOLVER igsv)
{
  /* you can't insert into an empty trie, so first, if empty, make a node */
  if( *trie == NULL )
  {
    if( HaArrayCount(igsv->ig_cost_trie_free_list) > 0 )
    {
      *trie = HaArrayLastAndDelete(igsv->ig_cost_trie_free_list);
      HaArrayClear((*trie)->children);
    }
    else
    {
      HaMake(*trie, igsv->arena);
      HaArrayInit((*trie)->children, igsv->arena);
    }
    (*trie)->value = -1;         /* i.e. no key ends at this node */
  }

  /* now we have a node, so insert into it */
  if( key == NULL )
  {
    /* the current node is the one we are looking for, to insert into */
    HnAssert((*trie)->value == -1,
      "KheIgCostTrieAdd internal error (key already inserted)");
    (*trie)->value = value;
  }
  else
  {
    /* have to recurse */
    HnAssert(key->index >= 0, "KheIgCostTrieAdd: index (%d) is negative\n",
      key->index);
    HaArrayFill((*trie)->children, key->index + 1, NULL);
    KheIgCostTrieAdd(&HaArray((*trie)->children, key->index),
      KheIgTaskGroupPrev(key), value, igsv);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgCostTrieRetrieve(KHE_IG_COST_TRIE trie, KHE_IG_TASK_GROUP key, */
/*    KHE_COST *value)                                                       */
/*                                                                           */
/*  If trie contains a (key, value) pair with this key, return true with     */
/*  value set to the corresponding value.  Otherwise return false.           */
/*                                                                           */
/*****************************************************************************/

static bool KheIgCostTrieRetrieve(KHE_IG_COST_TRIE trie, KHE_IG_TASK_GROUP key,
  KHE_COST *value)
{
  bool res;
  if( trie == NULL )
    *value = -1, res = false;
  else if( key == NULL )
    *value = trie->value, res = *value != -1;
  else
  {
    HnAssert(key->index >= 0, "KheTrieImplRetrieve: key item (%d) is neg\n",
      key->index);
    if( key->index >= HaArrayCount(trie->children) )
      *value = -1, res = false;
    else
      res = KheIgCostTrieRetrieve(HaArray(trie->children, key->index),
	KheIgTaskGroupPrev(key), value);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgCostTrieDebug(KHE_IG_COST_TRIE trie, int verbosity,            */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of trie with the given verbosity and indent.                 */
/*                                                                           */
/*****************************************************************************/

static void KheIgCostTrieDebug(KHE_IG_COST_TRIE trie, int verbosity,
  int indent, FILE *fp)
{
  KHE_IG_COST_TRIE child_trie;  int i;
  if( trie != NULL )
  {
    if( trie->value != -1 )
      fprintf(fp, "%*s%.5f\n", indent, "", KheCostShow(trie->value));
    fprintf(fp, "%*s[\n", indent, "");
    HaArrayForEach(trie->children, child_trie, i)
      if( child_trie != NULL )
      {
	fprintf(fp, "%*s  %d:\n", indent, "", i);
        KheIgCostTrieDebug(child_trie, verbosity, indent + 2, fp);
      }
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_TIME_GROUP"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TIME_GROUP KheIgTimeGroupMake(KHE_TIME_GROUP tg, int index,       */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Make a new ig time group object with these attributes.                   */
/*                                                                           */
/*****************************************************************************/
/* ***
static KHE_IG_SOLN_CACHE KheIgSolnCacheMake(KHE_IG_SOLVER igsv);
static void KheIgSolnCacheClear(KHE_IG_SOLN_CACHE igsc, KHE_IG_SOLVER igsv);
*** */

static KHE_IG_TIME_GROUP KheIgTimeGroupMake(KHE_TIME_GROUP tg, int index,
  KHE_IG_SOLVER igsv)
{
  KHE_IG_TIME_GROUP res;

  if( HaArrayCount(igsv->ig_time_group_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(igsv->ig_time_group_free_list);
    HaArrayClear(res->starting_mtasks);
    HaArrayClear(res->task_classes);
    HnAssert(res->soln_set_trie == NULL, "KheIgTimeGroupMake internal error");
    /* KheIgSolnCacheClear(res->soln_cache, igsv); */
    HaArrayClear(res->final_solns);
  }
  else
  {
    HaMake(res, igsv->arena);
    HaArrayInit(res->starting_mtasks, igsv->arena);
    HaArrayInit(res->task_classes, igsv->arena);
    res->soln_set_trie = NULL;
    /* res->soln_cache = KheIgSolnCacheMake(igsv); */
    HaArrayInit(res->final_solns, igsv->arena);
  }

  /* general */
  res->solver = igsv;
  res->index = index;
  res->offset = KheFrameTimeOffset(igsv->days_frame, KheTimeGroupTime(tg, 0));
  res->time_group = tg;

  /* tasks */
  res->running_tasks = 0;

  /* solutions */
  res->cost_trie = NULL;
  res->total_solns = 0;
  res->undominated_solns = 0;
#if DEBUG_COMPATIBLE
  res->other_igs = NULL;
  res->compatible_solns = 0;
#endif
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheIgTimeGroupId(KHE_IG_TIME_GROUP igtg)                           */
/*                                                                           */
/*  Return the id of igtg (the id of its time group).                        */
/*                                                                           */
/*****************************************************************************/

static char *KheIgTimeGroupId(KHE_IG_TIME_GROUP igtg)
{
  return KheTimeGroupId(igtg->time_group);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTimeGroupAddStartingMTask(KHE_IG_TIME_GROUP igtg,              */
/*    KHE_IG_MTASK igmt)                                                     */
/*                                                                           */
/*  Record the fact that igmt is starting during igtg.                       */
/*                                                                           */
/*****************************************************************************/

static void KheIgTimeGroupAddStartingMTask(KHE_IG_TIME_GROUP igtg,
  KHE_IG_MTASK igmt)
{
  HaArrayAddLast(igtg->starting_mtasks, igmt);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTimeGroupAddRunningMTask(KHE_IG_TIME_GROUP igtg,               */
/*    KHE_IG_MTASK igmt)                                                     */
/*                                                                           */
/*  Record the fact that igmt is running during igtg.                        */
/*                                                                           */
/*****************************************************************************/

static void KheIgTimeGroupAddRunningMTask(KHE_IG_TIME_GROUP igtg,
  KHE_IG_MTASK igmt)
{
  igtg->running_tasks += HaArrayCount(igmt->included_tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTimeGroupFindBestLengthenerMTask(KHE_IG_TIME_GROUP igtg,       */
/*    KHE_RESOURCE_GROUP domain, bool length_one_only,                       */
/*    KHE_IG_MTASK *best_igmt)                                               */
/*                                                                           */
/*  If there is a way to increase our options at igtg for tasks with this    */
/*  domain, return the mtask with the best options.  If length_one_only is   */
/*  true, we are only interested in mtasks whose duration is 1.              */
/*                                                                           */
/*  As a special case, igtg may be NULL, and then false is always returned.  */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTimeGroupFindBestLengthenerMTask(KHE_IG_TIME_GROUP igtg,
  KHE_RESOURCE_GROUP domain, bool length_one_only, KHE_IG_MTASK *best_igmt)
{
  KHE_MTASK mt;  int i, rg_count, best_rg_count;
  KHE_RESOURCE_GROUP rg;  KHE_IG_MTASK igmt;
  *best_igmt = NULL;
  if( igtg == NULL )
    return false;
  best_rg_count = INT_MAX;
  HaArrayForEach(igtg->starting_mtasks, igmt, i)
  {
    mt = igmt->mtask;
    rg = KheMTaskDomain(mt);
    if( (!length_one_only || KheMTaskTotalDuration(mt) == 1) &&
	KheResourceGroupSubset(domain, rg) &&
	HaArrayCount(igmt->included_tasks) < KheMTaskTaskCount(mt) )
    {
      rg_count = KheResourceGroupResourceCount(rg);
      if( rg_count < best_rg_count )
      {
	*best_igmt = igmt;
	best_rg_count = rg_count;
      }
    }
  }
  if( DEBUG17 && *best_igmt != NULL )
  {
    mt = igmt->mtask;
    fprintf(stderr, "  KheIgTimeGroupFindBestLengthenerMTask(%s, %s, %s, -) "
      "returning %s (%s)\n", KheIgTimeGroupId(igtg),
      KheResourceGroupId(domain), bool_show(length_one_only),
      KheMTaskId(mt), KheResourceGroupId(KheMTaskDomain(mt)));
  }
  return *best_igmt != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTimeGroupContainsAcceptingTaskClass(KHE_IG_TIME_GROUP igtg,    */
/*    KHE_IG_TASK igt, KHE_IG_TASK_CLASS *igtc)                              */
/*                                                                           */
/*  If igtg contains a task class that accepts igt, set *igtc to that        */
/*  class and return true.  Otherwise return false.                          */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTimeGroupContainsAcceptingTaskClass(KHE_IG_TIME_GROUP igtg,
  KHE_IG_TASK igt, KHE_IG_TASK_CLASS *igtc)
{
  int i;
  HaArrayForEach(igtg->task_classes, *igtc, i)
    if( KheIgTaskClassAcceptsTask(*igtc, igt) )
      return true;
  return *igtc = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTimeGroupSetTaskClasses(KHE_IG_TIME_GROUP igtg,                */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Set the task_classes array of igtg.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheIgTimeGroupSetTaskClasses(KHE_IG_TIME_GROUP igtg,
  KHE_IG_SOLVER igsv)
{
  KHE_IG_MTASK igmt;  int i, j;  KHE_IG_TASK igt;  KHE_IG_TASK_CLASS igtc;
  HaArrayClear(igtg->task_classes);
  HaArrayForEach(igtg->starting_mtasks, igmt, i)
    HaArrayForEach(igmt->included_tasks, igt, j)
    {
      if( !KheIgTimeGroupContainsAcceptingTaskClass(igtg, igt, &igtc) )
      {
	igtc = KheIgTaskClassMake(igsv);
	HaArrayAddLast(igtg->task_classes, igtc);
      }
      KheIgTaskClassAddTask(igtc, igt);
    }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTimeGroupDeleteSolns(KHE_IG_TIME_GROUP igtg,                   */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Delete igtg's solutions.                                                 */
/*                                                                           */
/*****************************************************************************/
static void KheIgSolnFree(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv);

static void KheIgTimeGroupDeleteSolns(KHE_IG_TIME_GROUP igtg,
  KHE_IG_SOLVER igsv)
{
  KHE_IG_SOLN igs;  int i;
  /* KheIgSolnCacheClear(igtg->soln_cache, igsv); */
  HnAssert(igtg->soln_set_trie == NULL,
    "KheIgTimeGroupDeleteSolns internal error 1");
  KheIgCostTrieClear(igtg->cost_trie, igsv);
  igtg->cost_trie = NULL;
  igtg->total_solns = 0;
  igtg->undominated_solns = 0;
  HaArrayForEach(igtg->final_solns, igs, i)
    KheIgSolnFree(igs, igsv);
  HaArrayClear(igtg->final_solns);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTimeGroupFree(KHE_IG_TIME_GROUP igtg, KHE_IG_SOLVER igsv)      */
/*                                                                           */
/*  Free igtg, including freeing its mtasks (which will free its tasks),     */
/*  freeing its task classes, and freeing any solutions it still contains.   */
/*                                                                           */
/*****************************************************************************/

static void KheIgTimeGroupFree(KHE_IG_TIME_GROUP igtg, KHE_IG_SOLVER igsv)
{
  KHE_IG_MTASK igmt;  int i;
  HaArrayForEach(igtg->starting_mtasks, igmt, i)
    KheIgMTaskFree(igmt, igsv);
  HaArrayClear(igtg->starting_mtasks);
  HaArrayAppend(igsv->ig_task_class_free_list, igtg->task_classes, i);
  KheIgTimeGroupDeleteSolns(igtg, igsv);
  HaArrayAddLast(igsv->ig_time_group_free_list, igtg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTimeGroupAddSoln(KHE_IG_TIME_GROUP igtg, KHE_IG_SOLN igs)      */
/*                                                                           */
/*  Add igs to igtg, doing dominance testing as we go.  Return true if       */
/*  igs was saved.                                                           */
/*                                                                           */
/*  The solution passed here, igs, is a copy.  So if it ends up not being    */
/*  used, because it is dominated, then free it.                             */
/*                                                                           */
/*****************************************************************************/
static void KheIgSolnDebugTimetable(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv,
  int verbosity, int indent, FILE *fp);
/* ***
static bool KheIgSolnDominates(KHE_IG_SOLN igs1, KHE_IG_SOLN igs2,
  KHE_IG_SOLVER igsv);
static void KheIgSolnCacheAddSoln(KHE_IG_SOLN_CACHE igsc, KHE_IG_SOLN igs,
  KHE_IG_SOLVER igsv);
*** */
static void KheIgSolnSetTrieAddSoln(KHE_IG_SOLN_SET_TRIE *trie,
  KHE_IG_SOLN igs, KHE_IG_SOLVER igsv);

static void KheIgTimeGroupAddSoln(KHE_IG_TIME_GROUP igtg, KHE_IG_SOLN igs)
{
  /* KHE_IG_SOLN igs2;  int i; */  KHE_IG_SOLVER igsv;

  /* if igs is dominated, free igs and return without changing anything else */
  if( DEBUG24 )
    fprintf(stderr, "[ KheIgTimeGroupAddSoln(%s with %d solns, igs)\n",
      KheIgTimeGroupId(igtg), igtg->undominated_solns);
  if( DEBUG44(igtg) )
  {
    fprintf(stderr, "  adding %s solution:\n", KheIgTimeGroupId(igtg));
    KheIgSolnDebugTimetable(igs, igtg->solver, 2, 2, stderr);
  }
  igsv = igtg->solver;
  igtg->total_solns++;
#if DEBUG_COMPATIBLE
  if( igs->compatible )
    igtg->compatible_solns++;
#endif
  KheIgSolnSetTrieAddSoln(&igtg->soln_set_trie, igs, igsv);
  /* KheIgSolnCacheAddSoln(igtg->soln_cache, igs, igtg->solver); */
  /* *** moved to KheIgSolnCacheAddSoln
  igsv = igtg->solver;
  igtg->total_solns++;
  #if DEBUG_COMPATIBLE
  if( igs->compatible )
    igtg->compatible_solns++;
  #endif
  HaArrayForEach(igtg->solns, igs2, i)
    if( KheIgSolnDominates(igs2, igs, igsv) )
    {
  #if DEBUG_COMPATIBLE
      if( DEBUG43 && igs->compatible )
      {
	fprintf(stderr, "  new compatible solution dominated by this %s "
	  "solution:\n", igs2->compatible ? "compatible" : "incompatible");
	KheIgSolnDebugTimetable(igs2, igsv, 2, 2, stderr);
      }
  #endif
      KheIgSolnFree(igs, igsv);
      if( DEBUG24 )
	fprintf(stderr, "] KheIgTimeGroupAddSoln returning "
	  "(dominated by soln %d)\n", i);
      return false;
    }

  ** igs is not dominated, so free anything dominated by it **
  HaArrayForEach(igtg->solns, igs2, i)
    if( KheIgSolnDominates(igs, igs2, igsv) )
    {
  #if DEBUG_COMPATIBLE
      if( DEBUG43 && igs2->compatible )
      {
	fprintf(stderr, "  this previously saved compatible solution:\n");
	KheIgSolnDebugTimetable(igs2, igsv, 2, 2, stderr);
	fprintf(stderr, "  is dominated by this new %s solution:\n",
	  igs->compatible ? "compatible" : "incompatible");
	KheIgSolnDebugTimetable(igs, igsv, 2, 2, stderr);
      }
  #endif
      KheIgSolnFree(igs2, igsv);
      HaArrayDeleteAndPlug(igtg->solns, i);
      if( DEBUG24 )
	fprintf(stderr, "  KheIgTimeGroupAddSoln deleted dominated soln "
	  "%d, %d solns left\n", i, HaArrayCount(igtg->solns));
      i--;
    }

  ** finally, add igs **
  HaArrayAddLast(igtg->solns, igs);
  if( DEBUG24 )
    fprintf(stderr, "] KheIgTimeGroupAddSoln returning (added), %d solns\n",
      HaArrayCount(igtg->solns));
  return true;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTimeGroupDebugMTasks(KHE_IG_TIME_GROUP igtg, int verbosity,    */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of the starting mtasks of igtg onto fp with the given        */
/*  verbosity and indent.                                                    */
/*                                                                           */
/*****************************************************************************/

static void KheIgTimeGroupDebugMTasks(KHE_IG_TIME_GROUP igtg,
  int verbosity, int indent, FILE *fp)
{
  KHE_IG_MTASK igmt;  int i;
  fprintf(fp, "%*s[ IgTimeGroup(%s, running_tasks %d)\n",
    indent, "", KheIgTimeGroupId(igtg), /* igtg->starting_avail, */
    igtg->running_tasks);
  HaArrayForEach(igtg->starting_mtasks, igmt, i)
    KheIgMTaskDebug(igmt, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTimeGroupDebugSolnsHeader(KHE_IG_TIME_GROUP igtg, FILE *fp)    */
/*                                                                           */
/*  Print the header line of KheIgTimeGroupDebugSolns.                       */
/*                                                                           */
/*****************************************************************************/

static void KheIgTimeGroupDebugSolnsHeader(KHE_IG_TIME_GROUP igtg, FILE *fp)
{
  fprintf(fp, "IgTimeGroupSolns(%s, %d made, ",
    KheIgTimeGroupId(igtg), igtg->total_solns);
#if DEBUG_COMPATIBLE
  fprintf(fp, "%d compatible, ", igtg->compatible_solns);
#endif
  fprintf(fp, "%d undominated, %d kept)", igtg->undominated_solns,
    HaArrayCount(igtg->final_solns));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTimeGroupDebugSolns(KHE_IG_TIME_GROUP igtg, int verbosity,     */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of igtg's solutions onto fp with the given verbosity and     */
/*  indent.                                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheIgTimeGroupDebugSolns(KHE_IG_TIME_GROUP igtg,
  int verbosity, int indent, FILE *fp)
{
  KHE_IG_SOLN igs;  int i;
  if( indent < 0 )
    KheIgTimeGroupDebugSolnsHeader(igtg, fp);
  else if( verbosity == 1 )
  {
    fprintf(fp, "%*s", indent, "");
    KheIgTimeGroupDebugSolnsHeader(igtg, fp);
    fprintf(fp, "\n");
  }
  else
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheIgTimeGroupDebugSolnsHeader(igtg, fp);
    if( HaArrayCount(igtg->final_solns) <= 10 )
    {
      HaArrayForEach(igtg->final_solns, igs, i)
      {
	fprintf(stderr, "\n");
	KheIgSolnDebugTimetable(igs, igtg->solver, verbosity, indent+2, stderr);
      }
    }
    else
    {
      fprintf(fp, "\n");
      igs = HaArrayFirst(igtg->final_solns);
      KheIgSolnDebugTimetable(igs, igtg->solver, verbosity, indent + 2, fp);
      fprintf(fp, "%*s  ... ... ...\n", indent, "");
      igs = HaArrayLast(igtg->final_solns);
      KheIgSolnDebugTimetable(igs, igtg->solver, verbosity, indent + 2, fp);
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_TASK_GROUP" - construction                             */
/*                                                                           */
/*  Submodule KHE_IG_TASK_GROUP is divided into four parts:  construction,   */
/*  query, complex operations, and debug.                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskGroupCopyGrouperEntry(KHE_IG_TASK_GROUP igtg,              */
/*    KHE_TASK_GROUPER_ENTRY grouper_entry)                                  */
/*                                                                           */
/*  Copy the fields of grouper_entry into igtg.                              */
/*                                                                           */
/*****************************************************************************/

static void KheIgTaskGroupCopyGrouperEntry(KHE_IG_TASK_GROUP igtg,
  KHE_TASK_GROUPER_ENTRY grouper_entry)
{
  igtg->task = grouper_entry->task;
  /* igtg->leader = grouper_entry->leader; */
  igtg->domain = grouper_entry->domain;
  igtg->assigned_resource = grouper_entry->assigned_resource;
  igtg->prev = grouper_entry->prev;
  igtg->interval = grouper_entry->interval;
  igtg->type = grouper_entry->type;
  igtg->has_fixed_unassigned = grouper_entry->has_fixed_unassigned;
  /* igtg->dummy_entry = grouper_entry->dummy_entry; */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK_GROUP KheIgTaskGroupMake(                                    */
/*    KHE_TASK_GROUPER_ENTRY grouper_entry, bool optional,                   */
/*    bool finished_bcd, int index, int primary_durn,                        */
/*    KHE_COST non_must_assign_cost, int overhang, KHE_IG_SOLVER igsv)       */
/*                                                                           */
/*  Make a task group with these attributes.                                 */
/*                                                                           */
/*  The task group is optional if the "optional" argument is true and its    */
/*  predecessors are all optional.                                           */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_TASK_GROUP KheIgTaskGroupMake(
  KHE_TASK_GROUPER_ENTRY grouper_entry, bool optional,
  bool finished_bcd, int index, int primary_durn,
  KHE_COST non_must_assign_cost, int overhang, KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK_GROUP res, prev;
  if( HaArrayCount(igsv->ig_task_group_free_list) > 0 )
    res = HaArrayLastAndDelete(igsv->ig_task_group_free_list);
  else
    HaMake(res, igsv->arena);
  HnAssert(grouper_entry != NULL, "KheIgTaskGroupMake internal error");
  KheIgTaskGroupCopyGrouperEntry(res, grouper_entry);
  prev = KheIgTaskGroupPrev(res);
  res->optional = optional && (prev == NULL || prev->optional);
  res->finished_bcd = finished_bcd;
  res->expand_used = false;
  res->index = index;
  res->primary_durn = primary_durn;
  res->non_must_assign_cost = non_must_assign_cost;
  res->overhang = overhang;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK_GROUP KheIgTaskGroupMakeHistory(KHE_RESOURCE r,              */
/*    int primary_durn, KHE_IG_SOLVER igsv)                                  */
/*                                                                           */
/*  Make a history task group with these attributes.                         */
/*                                                                           */
/*  Implementation note.  In this case the primary duration is used as       */
/*  the index.  This is why it is stored twice.                              */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_TASK_GROUP KheIgTaskGroupMakeHistory(KHE_RESOURCE r,
  int primary_durn, KHE_IG_SOLVER igsv)
{
  struct khe_task_grouper_entry_rec new_entry;
  KheTaskGrouperEntryAddHistory(NULL, r, primary_durn,
    igsv->domain_finder, &new_entry);
  return KheIgTaskGroupMake(&new_entry, false, false, primary_durn,
    primary_durn, 0, 0, igsv);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK_GROUP KheIgTaskGroupMakeInitial(KHE_IG_TASK igt,             */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Make a task group which is initial (i.e. it contains only igt, there     */
/*  is no predecessor).                                                      */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_TASK_GROUP KheIgTaskGroupMakeInitial(KHE_IG_TASK igt,
  KHE_IG_SOLVER igsv)
{
  struct khe_task_grouper_entry_rec new_entry;  KHE_IG_MTASK igmt;
  igmt = igt->ig_mtask;
  KheTaskGrouperEntryAddTask(NULL, igt->task, igsv->days_frame,
    igsv->domain_finder, &new_entry);
  return KheIgTaskGroupMake(&new_entry, igt->optional,
    igmt->finished_cd || igmt->primary_durn >= igsv->max_limit,
    KheIgMTaskIndex(igmt, igmt->full_durn - 1),
    igmt->primary_durn, igt->non_must_assign_cost, igmt->full_durn - 1, igsv);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK_GROUP KheIgTaskGroupMakeSuccessor(KHE_IG_TASK_GROUP igtg,    */
/*    KHE_IG_TASK igt, KHE_TASK_GROUPER_ENTRY new_entry, KHE_IG_SOLVER igsv) */
/*                                                                           */
/*  Make a task group which is igtg plus igt.  The first part of the job,    */
/*  which is to check whether this is feasible and store the outcome of      */
/*  that in new_entry, has already been done.                                */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_TASK_GROUP KheIgTaskGroupMakeSuccessor(KHE_IG_TASK_GROUP igtg,
  KHE_IG_TASK igt, KHE_TASK_GROUPER_ENTRY new_entry, KHE_IG_SOLVER igsv)
{
  KHE_IG_MTASK igmt;  int primary_durn;  KHE_IG_TASK_GROUP res;
  KHE_COST non_must_assign_cost;
  igmt = igt->ig_mtask;
  primary_durn = igtg->primary_durn + igmt->primary_durn;
  non_must_assign_cost = igtg->non_must_assign_cost + igt->non_must_assign_cost;
  res = KheIgTaskGroupMake(new_entry, igt->optional,
    igmt->finished_cd || primary_durn >= igsv->max_limit,
    KheIgMTaskIndex(igmt, igmt->full_durn - 1),
    primary_durn, non_must_assign_cost, igmt->full_durn - 1, igsv);
  if( DEBUG45 )
    fprintf(stderr, "  KheIgTaskGroupMakeSuccessor: new_entry->type %d, "
      "res->type %d\n", (int) new_entry->type, (int) res->type);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK_GROUP KheIgTaskGroupMakeDummy(KHE_IG_TASK_GROUP igtg,        */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Make a dummy successor of igtg.                                          */
/*                                                                           */
/*****************************************************************************/
static KHE_IG_TASK KheIgSolverTaskToIgTask(KHE_IG_SOLVER igsv, KHE_TASK task);

static KHE_IG_TASK_GROUP KheIgTaskGroupMakeDummy(KHE_IG_TASK_GROUP igtg,
  KHE_IG_SOLVER igsv)
{
  struct khe_task_grouper_entry_rec new_entry;
  KHE_IG_TASK igt;
  KheTaskGrouperEntryAddDummy((KHE_TASK_GROUPER_ENTRY) igtg, &new_entry);
  igt = KheIgSolverTaskToIgTask(igsv, igtg->task);
  return KheIgTaskGroupMake(&new_entry, igtg->optional, igtg->finished_bcd,
    KheIgMTaskIndex(igt->ig_mtask, igtg->overhang - 1),
    igtg->primary_durn, igtg->non_must_assign_cost, igtg->overhang - 1, igsv);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK_GROUP KheIgTaskGroupCopy(KHE_IG_TASK_GROUP igtg,             */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Return a copy of igtg.                                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_TASK_GROUP KheIgTaskGroupCopy(KHE_IG_TASK_GROUP igtg,
  KHE_IG_SOLVER igsv)
{
  return KheIgTaskGroupMake((KHE_TASK_GROUPER_ENTRY) igtg, igtg->optional,
    igtg->finished_bcd, igtg->index, igtg->primary_durn,
    igtg->non_must_assign_cost, igtg->overhang, igsv);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskGroupFree(KHE_IG_TASK_GROUP igtg, KHE_IG_SOLVER igsv)      */
/*                                                                           */
/*  Free igtg, by adding it to the free list in igsv.                        */
/*                                                                           */
/*  Implementation note.  We do not free initial task groups, because        */
/*  they are cached in their tasks.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheIgTaskGroupFree(KHE_IG_TASK_GROUP igtg, KHE_IG_SOLVER igsv)
{
  HnAssert(igtg != NULL, "KheIgTaskGroupFree internal error 1");
  HaArrayAddLast(igsv->ig_task_group_free_list, igtg);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_TASK_GROUP" - query                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheIgTaskGroupDomain(KHE_IG_TASK_GROUP igtg)          */
/*                                                                           */
/*  Return the domain of igtg.  This is d(g) in the documentation.           */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_GROUP KheIgTaskGroupDomain(KHE_IG_TASK_GROUP igtg)
{
  /* KHE_TASK_GROUP_DOMAIN_TYPE type; */
  return KheTaskGroupDomainValue(igtg->domain /* , &type */);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK_GROUP KheIgTaskGroupPrev(KHE_IG_TASK_GROUP igtg)             */
/*                                                                           */
/*  Return igtg's predecessor task group.                                    */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_TASK_GROUP KheIgTaskGroupPrev(KHE_IG_TASK_GROUP igtg)
{
  return (KHE_IG_TASK_GROUP) igtg->prev;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupHasVaryingDomains(KHE_IG_TASK_GROUP igtg)             */
/*                                                                           */
/*  Return true if igtg has tasks whose domains vary.                        */
/*                                                                           */
/*****************************************************************************/

/* *** correct but no longer used
static bool KheIgTaskGroupHasVaryingDomains(KHE_IG_TASK_GROUP igtg)
{
  KHE_RESOURCE_GROUP leader_domain;
  if( igtg->leader != NULL )
  {
    leader_domain = KheIgTaskGroupDomain(igtg);
    while( igtg != NULL )
    {
      if( igtg->task != NULL &&
	  !KheResourceGroupEqual(leader_domain, KheTaskDomain(igtg->task)) )
	return true;
      igtg = KheIgTaskGroupPrev(igtg);
    }
  }
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupUndersized(KHE_IG_TASK_GROUP igtg, KHE_IG_SOLVER igsv)*/
/*                                                                           */
/*  Return true if igtg is undersized.  This means not only that its         */
/*  primary duration falls short of the minimum limit, but also that         */
/*  it is not optional, so the short length will have a cost.                */
/*                                                                           */
/*  This code does not check whether igtg comes right at the end of the      */
/*  cycle.  That would be a reason for considering it to be not undersized   */
/*  after all, so care is needed on that point.                              */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskGroupUndersized(KHE_IG_TASK_GROUP igtg, KHE_IG_SOLVER igsv)
{
  return !igtg->optional && igtg->primary_durn < igsv->min_limit;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupIsOrdinary(KHE_IG_TASK_GROUP igtg)                    */
/*                                                                           */
/*  Return true if igtg is an ordinary task group:  not optional, not        */
/*  assigned a resource, and not fixed.                                      */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static bool KheIgTaskGroupIsOrdinary(KHE_IG_TASK_GROUP igtg)
{
  return !igtg->optional && igtg->assigned_resource == NULL &&
    !igtg->has_fixed_unassigned;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupIsSelfFinished(KHE_IG_TASK_GROUP igtg)                */
/*                                                                           */
/*  Return true if igtg is self-finished.  Self-finished groups always come  */
/*  at the end of sorted lists of groups.                                    */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskGroupIsSelfFinished(KHE_IG_TASK_GROUP igtg)
{
  return igtg->finished_bcd && igtg->overhang == 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskGroupSetIndexes(KHE_IG_TASK_GROUP last_igtg,               */
/*    int li, KHE_IG_SOLVER igsv, HA_ARRAY_INT *indexes)                     */
/*                                                                           */
/*  Set *indexes to reflect the times (and history) of last_igtg.            */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used now that indexes are included in task groups
static void KheIgTaskGroupSetIndexes(KHE_IG_TASK_GROUP last_igtg,
  int li, KHE_IG_SOLVER igsv, HA_ARRAY_INT *indexes)
{
  KHE_IG_TASK_GROUP igtg;  int i;  KHE_INTERVAL in;  KHE_IG_TASK igt;
  KHE_TASK task;

  ** index of last_igtg's last day **
  HaArrayClear(*indexes);
  HaArrayAddLast(*indexes, li);

  ** offsets of last_igtg's tasks, plus history **
  for( igtg = last_igtg;  igtg != NULL;  igtg = KheIgTaskGroupPrev(igtg) )
  {
    switch( KheTaskGrouperEntryType((KHE_TASK_GROUPER_ENTRY) igtg) )
    {
      case KHE_TASK_GROUPER_ENTRY_ORDINARY:

	task = KheTaskGrouperEntryTask((KHE_TASK_GROUPER_ENTRY) igtg);
        igt = KheIgSolverTaskToIgTask(igsv, task);
	** ***
	index = KheTaskSolnIndex(task);
	igt = HaArray(igsv->included_tasks, index);
	HnAssert(igt != NULL, "KheIgTaskGroupSetIndexes internal error");
	*** **
	HaArrayAppend(*indexes, igt->ig_mtask->time_offsets, i);
	break;

      case KHE_TASK_GROUPER_ENTRY_HISTORY:

	** include the history value in the index **
	in = KheTaskGrouperEntryInterval((KHE_TASK_GROUPER_ENTRY) igtg) ;
	HaArrayAddLast(*indexes, KheIntervalLength(in));
	break;

      case KHE_TASK_GROUPER_ENTRY_DUMMY:

	** no task here, do nothing **
	** case KHE_TASK_GROUPER_ENTRY_ORDINARY adds all of them at once **
	break;

      default:

	HnAbort("KheIgTaskGroupSetIndexes internal error");
	break;
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheIgTaskGroupNonMustAssignCost(KHE_IG_TASK_GROUP last_igtg,    */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Return the non-must-assign cost of last_igtg.  This is e(g) in the doc.  */
/*                                                                           */
/*****************************************************************************/

/* *** now stored in a field of last_igtg
static KHE_COST KheIgTaskGroupNonMustAssignCost(KHE_IG_TASK_GROUP last_igtg,
  KHE_IG_SOLVER igsv)
{
  KHE_COST res;  KHE_IG_TASK igt;  KHE_IG_TASK_GROUP igtg;
  res = 0;
  for( igtg = last_igtg;  igtg != NULL;  igtg = KheIgTaskGroupPrev(igtg) )
  {
    if( igtg->type == KHE_TASK_GROUPER_ENTRY_ORDINARY )
    {
      HnAssert(igtg->task != NULL,
	"KheIgTaskGroupNonMustAssignCost internal error");
      igt = KheIgSolverTaskToIgTask(igsv, igtg->task);
      res += igt->non_must_assign_cost;
    }
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupIsPredecessor(KHE_IG_TASK_GROUP dt,                   */
/*    KHE_IG_SOLN next_igs)                                                  */
/*                                                                           */
/*  Return true if dt is the predecessor of a task in next_igs, or false     */
/*  if not or if next_igs is NULL.                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskGroupIsPredecessor(KHE_IG_TASK_GROUP dt,
  KHE_IG_SOLN next_igs)
{
  KHE_IG_TASK_GROUP dt2;  int i;
  if( next_igs != NULL )
    HaArrayForEach(next_igs->task_groups, dt2, i)
      if( KheIgTaskGroupPrev(dt2) == dt )
	return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_TASK_GROUP" - complex operations                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupMustAcceptTask(KHE_IG_TASK_GROUP igtg, KHE_TASK task) */
/*                                                                           */
/*  Return true if these have to be linked because they are assigned the     */
/*  same non-NULL resource.                                                  */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskGroupMustAcceptTask(KHE_IG_TASK_GROUP igtg, KHE_TASK task)
{
  KHE_RESOURCE r;
  r = KheTaskAsstResource(task);
  return r != NULL && r == igtg->assigned_resource;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupIsExtendable(KHE_IG_TASK_GROUP igtg,                  */
/*    KHE_IG_TASK igt, KHE_IG_SOLVER igsv, KHE_TASK_GROUPER_ENTRY new_entry) */
/*                                                                           */
/*  If igtg can be extended by adding task, then return true with            */
/*  *new_entry set to the resulting new task grouper entry.  Otherwise       */
/*  return false with *new_entry undefined.                                  */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskGroupIsExtendable(KHE_IG_TASK_GROUP igtg,
  KHE_IG_TASK igt, KHE_IG_SOLVER igsv, KHE_TASK_GROUPER_ENTRY new_entry)
{
  int primary_durn;

  /* (1) not extendable if igtg has overhang */
  if( igtg->overhang > 0 )
    return false;
  HnAssert(igt->task != NULL, "KheIgTaskGroupIsExtendable internal error 2");

  /* (2) not extendable if new class duration is too long (unless assigned) */
  primary_durn = igtg->primary_durn + igt->ig_mtask->primary_durn;
  if( primary_durn > igsv->max_limit &&
      !KheIgTaskGroupMustAcceptTask(igtg, igt->task) )
  {
    if( DEBUG23 && igtg->type == KHE_TASK_GROUPER_ENTRY_HISTORY )
      fprintf(stderr, "  extendable not grouping history %s with task %s (4)\n",
	KheResourceId(igtg->assigned_resource), KheIgTaskId(igt));
    return false;
  }

  /* (3) not extendable if igtg->finished_bcd */
  if( igtg->finished_bcd )
  {
    if( DEBUG23 && igtg->type == KHE_TASK_GROUPER_ENTRY_HISTORY )
      fprintf(stderr, "  extendable not grouping history %s with task %s (1)\n",
	KheResourceId(igtg->assigned_resource), KheIgTaskId(igt));
    return false;
  }

  /* (4) not extendable if igt's placement is first only */
  if( igt->ig_mtask->placement == KHE_PLACEMENT_FIRST_ONLY )
    return false;

  /* not extendable if task continues asst but igtg is not what it continues */
  /* *** obsolete
  if( task_continues_asst && !KheIgTaskGroupMustAcceptTask(igtg, igt->task) )
  {
    if( DEBUG23 && igtg->type == KHE_TASK_GROUPER_ENTRY_HISTORY )
      fprintf(stderr, "  extendable not grouping history %s with task %s (3)\n",
	KheResourceId(igtg->assigned_resource), KheIgTaskId(igt));
    return false;
  }
  *** */

  /* (5) build new_entry; not extendable if can't */
  return KheTaskGrouperEntryAddTask((KHE_TASK_GROUPER_ENTRY) igtg, igt->task,
    igsv->days_frame, igsv->domain_finder, new_entry);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheArrayIntEqual(HA_ARRAY_INT *array_int1, HA_ARRAY_INT *array_int2)*/
/*                                                                           */
/*  Return true if *array_int1 and *array_int1 are equal.                    */
/*                                                                           */
/*****************************************************************************/

static bool KheArrayIntEqual(HA_ARRAY_INT *array_int1, HA_ARRAY_INT *array_int2)
{
  int count1, count2, i;
  if( array_int1 == array_int2 )
    return true;
  count1 = HaArrayCount(*array_int1);
  count2 = HaArrayCount(*array_int2);
  if( count1 != count2 )
    return false;
  for( i = 0;  i < count1;  i++ )
    if( HaArray(*array_int1, i) != HaArray(*array_int2, i) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupIndexesEqual(KHE_IG_TASK_GROUP igtg1,                 */
/*    KHE_IG_TASK_GROUP igtg2)                                               */
/*                                                                           */
/*  Return true if igtg1 and igtg2 have the same indexes.                    */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskGroupIndexesEqual(KHE_IG_TASK_GROUP igtg1,
  KHE_IG_TASK_GROUP igtg2)
{
  while( igtg1 != NULL && igtg2 != NULL )
  {
    if( igtg1->index != igtg2->index )
      return false;
    igtg1 = KheIgTaskGroupPrev(igtg1);
    igtg2 = KheIgTaskGroupPrev(igtg2);
  }
  return igtg1 == NULL && igtg2 == NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupDominates(KHE_IG_TASK_GROUP igtg1,                    */
/*    KHE_IG_TASK_GROUP igtg2, KHE_IG_SOLVER igsv, bool *equal_so_far)       */
/*                                                                           */
/*  Return true if gtg1 dominates igtg2.  In that case, also update          */
/*  *equal_so_far so that it is true if equality holds.                      */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskGroupDominates(KHE_IG_TASK_GROUP igtg1,
  KHE_IG_TASK_GROUP igtg2, KHE_IG_SOLVER igsv, bool *equal_so_far)
{
  KHE_RESOURCE_GROUP rg1, rg2;  /* KHE_COST cost1, cost2; */

  /* (0) primary_durn (subsumed by time indexes, but faster to test first */
  if( igtg1->primary_durn != igtg2->primary_durn )
    return false;

  /* (1) domains */
  rg1 = KheIgTaskGroupDomain(igtg1);
  rg2 = KheIgTaskGroupDomain(igtg2);
  if( !KheResourceGroupSubset(rg2, rg1) )
    return false;
  if( KheResourceGroupResourceCount(rg2) != KheResourceGroupResourceCount(rg1) )
    *equal_so_far = false;

  /* (2) assignments */
  if( igtg1->assigned_resource != igtg2->assigned_resource )
    return false;

  /* (3) fixed non-assignments */
  if( igtg1->has_fixed_unassigned != igtg2->has_fixed_unassigned )
    return false;

  /* (4) times (plus history) */
  if( !KheIgTaskGroupIndexesEqual(igtg1, igtg2) )
    return false;

  /* ***
  KheIgTaskGroupSetIndexes(igtg1, INT_MAX, igsv, &igsv->tmp_indexes1);
  KheIgTaskGroupSetIndexes(igtg2, INT_MAX, igsv, &igsv->tmp_indexes2);
  if( !KheArrayIntEqual(&igsv->tmp_indexes1, &igsv->tmp_indexes2) )
    return false;
  *** */

  /* (5) optionality */
  if( !igtg1->optional && igtg2->optional )
    return false;

  /* (6) non-must-assign costs */
  if( igtg1->non_must_assign_cost > igtg2->non_must_assign_cost )
    return false;

  /* ***
  cost1 = KheIgTaskGroupNonMustAssignCost(igtg1, igsv);
  cost2 = KheIgTaskGroupNonMustAssignCost(igtg2, igsv);
  if( cost1 > cost2 )
    return false;
  *** */

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheIgTaskGroupCost(KHE_IG_TASK_GROUP last_igtg, int li,         */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Return the cost to some resource r of assigning r to last_igtg's group.  */
/*  As a special case, optional groups have cost 0, because we don't need    */
/*  to assign a resource to them at all.                                     */
/*                                                                           */
/*****************************************************************************/
/* static void KheArrayIntDebug(HA_ARRAY_INT *array_int, FILE *fp); */

static KHE_COST KheIgTaskGroupCost(KHE_IG_TASK_GROUP last_igtg,
  KHE_IG_TIME_GROUP igtg, /* int li, */ KHE_IG_SOLVER igsv)
{
  KHE_COST c1, c2, rcost;  bool last;

  /* optional groups have cost 0 */
  if( last_igtg->optional )
    return 0;

  /* find the indexes of last_igtg and retrieve from the cache */
  /* if( li == -1 ) */
  if( igtg == NULL )
  {
    /* this can only be a history task group that was not extended */
    HnAssert(last_igtg->type == KHE_TASK_GROUPER_ENTRY_HISTORY,
      "KheIgTaskGroupCost internal error 1");
    rcost = KheConstraintClassDeterminantToCost(igsv->curr_class,
      last_igtg->primary_durn, false);
    HnAssert(rcost >= 0, "KheIgTaskGroupCost internal error 2");
  }
  else
  {
    /* KheIgTaskGroupSetIndexes(last_igtg, li, igsv, &igsv->tmp_indexes1); */
    if( !KheIgCostTrieRetrieve(igtg->cost_trie, last_igtg, &rcost) )
    {
      /* no cached value, so first find the group cost (iii) */
      c1 = KheTaskGrouperEntryCost((KHE_TASK_GROUPER_ENTRY) last_igtg,
	igsv->days_frame, igsv->soln);
      HnAssert(c1 >= 0, "KheIgTaskGroupCost internal error 3");

      /* then find the constraint class cost (iv) */
      last = (igtg == HaArrayLast(igsv->time_groups));
      c2 = KheConstraintClassDeterminantToCost(igsv->curr_class,
	last_igtg->primary_durn, last);
      HnAssert(c2 >= 0, "KheIgTaskGroupCost internal error 4");

      /* cache the result */
      rcost = c1 + c2;
      /* ***
      if( DEBUG9 )
      {
	fprintf(stderr, "  cost of ");
	KheArrayIntDebug(&igsv->tmp_indexes1, stderr);
	fprintf(stderr, " = %.5f + %.5f = %.5f\n", KheCostShow(c1),
	  KheCostShow(c2), KheCostShow(rcost));
      }
      *** */
      KheIgCostTrieAdd(&igtg->cost_trie, last_igtg, rcost, igsv);
    }
  }

  return rcost + last_igtg->non_must_assign_cost;
  /* return rcost + KheIgTaskGroupNonMustAssignCost(last_igtg, igsv); */
}

/* *** old version that used a separate array of indexes
static void KheArrayIntDebug(HA_ARRAY_INT *array_int, FILE *fp);

static KHE_COST KheIgTaskGroupCost(KHE_IG_TASK_GROUP last_igtg, int li,
  KHE_IG_SOLVER igsv)
{
  int pos;  KHE_COST c1, c2, rcost;  bool last;

  ** optional groups have cost 0 **
  if( last_igtg->optional )
    return 0;

  ** find the indexes of last_igtg and retrieve from the cache **
  if( li == -1 )
  {
    ** special case; no groups ended here, so rcost is 0 **
    rcost = 0;
  }
  else
  {
    KheIgTaskGroupSetIndexes(last_igtg, li, igsv, &igsv->tmp_indexes1);
    if( !KheTrieRetrieve(igsv->cost_trie, &igsv->tmp_indexes1, rcost, pos) )
    {
      ** no cached value, so first find the group cost (iii) **
      c1 = KheTaskGrouperEntryCost((KHE_TASK_GROUPER_ENTRY) last_igtg,
	igsv->days_frame, igsv->soln);
      HnAssert(c1 >= 0, "KheIgTaskGroupCost internal error 1");

      ** then find the constraint class cost (iv) **
      last = (li == HaArrayCount(igsv->time_groups) - 1);
      c2 = KheConstraintClassDeterminantToCost(igsv->curr_class,
	last_igtg->primary_durn, last);
      HnAssert(c2 >= 0, "KheIgTaskGroupCost internal error 2");

      ** cache the result **
      rcost = c1 + c2;
      if( DEBUG9 )
      {
	fprintf(stderr, "  cost of ");
	KheArrayIntDebug(&igsv->tmp_indexes1, stderr);
	fprintf(stderr, " = %.5f + %.5f = %.5f\n", KheCostShow(c1),
	  KheCostShow(c2), KheCostShow(rcost));
      }
      KheTrieAdd(igsv->cost_trie, &igsv->tmp_indexes1, rcost);
    }
  }

  return rcost + KheIgTaskGroupNonMustAssignCost(last_igtg, igsv);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheIgTaskGroupAssignedResourceIndex(KHE_IG_TASK_GROUP igtg)          */
/*                                                                           */
/*  Return the instance index of tgtg's assigned resource, or -1 if none.    */
/*                                                                           */
/*****************************************************************************/

static int KheIgTaskGroupAssignedResourceIndex(KHE_IG_TASK_GROUP igtg)
{
  if( igtg->assigned_resource != NULL )
    return KheResourceInstanceIndex(igtg->assigned_resource);
  else
    return -1;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIgTaskGroupTypedCmp(KHE_IG_TASK_GROUP igtg1,                      */
/*    KHE_IG_TASK_GROUP igtg2)                                               */
/*                                                                           */
/*  Typed comparison function for sorting an array of task groups.  After    */
/*  sorting, self-finished groups appear at the end, thanks to the first     */
/*  two tests.                                                               */
/*                                                                           */
/*****************************************************************************/

static int KheIgTaskGroupTypedCmp(KHE_IG_TASK_GROUP igtg1,
  KHE_IG_TASK_GROUP igtg2)
{
  int cmp, count1, count2;

  /* sort by increasing finished_bcd */
  cmp = (int) igtg1->finished_bcd - (int) igtg2->finished_bcd;
  if( cmp != 0 )  return cmp;

  /* sort by decreasing overhang */
  cmp = igtg2->overhang - igtg1->overhang;
  if( cmp != 0 )  return cmp;

  /* sort by increasing assigned resource index */
  cmp = KheIgTaskGroupAssignedResourceIndex(igtg1) -
    KheIgTaskGroupAssignedResourceIndex(igtg2);
  if( cmp != 0 )  return cmp;

  /* sort by increasing primary duration */
  cmp = igtg1->primary_durn - igtg2->primary_durn;
  if( cmp != 0 )  return cmp;

  /* sort by the size of the task group's domain */
  count1 = KheResourceGroupResourceCount(KheIgTaskGroupDomain(igtg1));
  count2 = KheResourceGroupResourceCount(KheIgTaskGroupDomain(igtg2));
  return count1 - count2;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIgTaskGroupCmp(const void *t1, const void *t2)                    */
/*                                                                           */
/*  Untyped comparison function for sorting an array of ig tasks.            */
/*                                                                           */
/*****************************************************************************/

static int KheIgTaskGroupCmp(const void *t1, const void *t2)
{
  KHE_IG_TASK_GROUP igtg1 = * (KHE_IG_TASK_GROUP *) t1;
  KHE_IG_TASK_GROUP igtg2 = * (KHE_IG_TASK_GROUP *) t2;
  return KheIgTaskGroupTypedCmp(igtg1, igtg2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIgTaskGroupTypedCmp(KHE_IG_TASK_GROUP igtg1,                      */
/*    KHE_IG_TASK_GROUP igtg2)                                               */
/*                                                                           */
/*  Typed comparison function for sorting an array of task groups so that    */
/*  task groups which are already used appear at the end.                    */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static int KheIgTaskGroupExpandUsedTypedCmp(KHE_IG_TASK_GROUP igtg1,
  KHE_IG_TASK_GROUP igtg2)
{
  return (int) igtg1->expand_used - (int) igtg2->expand_used;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheIgTaskGroupCmp(const void *t1, const void *t2)                    */
/*                                                                           */
/*  Untyped comparison function for sorting an array of ig tasks.            */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static int KheIgTaskGroupExpandUsedCmp(const void *t1, const void *t2)
{
  KHE_IG_TASK_GROUP igtg1 = * (KHE_IG_TASK_GROUP *) t1;
  KHE_IG_TASK_GROUP igtg2 = * (KHE_IG_TASK_GROUP *) t2;
  return KheIgTaskGroupExpandUsedTypedCmp(igtg1, igtg2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupSameClass(KHE_IG_TASK_GROUP igtg1,                    */
/*    KHE_IG_TASK_GROUP igtg2)                                               */
/*                                                                           */
/*  Return true if igtg1 and igtg2 belong to the same equivalence class.     */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskGroupSameClass(KHE_IG_TASK_GROUP igtg1,
  KHE_IG_TASK_GROUP igtg2, bool include_domain_in_test)
{
  KHE_RESOURCE_GROUP rg1, rg2;  /* KHE_TASK_GROUP_DOMAIN_TYPE type; */
  /* KHE_COST cost1, cost2; */

  /* domain */
  if( include_domain_in_test )
  {
    HnAssert(igtg1->domain != NULL,
      "KheIgTaskGroupSameClass internal error 1");
    HnAssert(igtg2->domain != NULL,
      "KheIgTaskGroupSameClass internal error 2");
    rg1 = KheTaskGroupDomainValue(igtg1->domain);
    rg2 = KheTaskGroupDomainValue(igtg2->domain);
    if( !KheResourceGroupEqual(rg1, rg2) )
      return false;
  }

  /* times and history */
  if( !KheIgTaskGroupIndexesEqual(igtg1, igtg2) )
    return false;
  /* ***
  KheIgTaskGroupSetIndexes(igtg1, INT_MAX, igsv, &igsv->tmp_indexes1);
  KheIgTaskGroupSetIndexes(igtg2, INT_MAX, igsv, &igsv->tmp_indexes2);
  if( !KheArrayIntEqual(&igsv->tmp_indexes1, &igsv->tmp_indexes2) )
    return false;
  *** */

  /* assignments */
  if( igtg1->assigned_resource != igtg2->assigned_resource )
    return false;

  /* fixed non-assignments */
  if( igtg1->has_fixed_unassigned != igtg2->has_fixed_unassigned )
    return false;

  /* optionality */
  if( igtg1->optional != igtg2->optional )
    return false;

  /* n(g) */
  if( igtg1->non_must_assign_cost != igtg2->non_must_assign_cost )
    return false;

  /* ***
  cost1 = KheIgTaskGroupNonMustAssignCost(igtg1, igsv);
  cost2 = KheIgTaskGroupNonMustAssignCost(igtg2, igsv);
  if( cost1 != cost2 )
    return false;
  *** */

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_TASK_GROUP" - debug                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheArrayIntDebug(HA_ARRAY_INT *array_int, FILE *fp)                 */
/*                                                                           */
/*  Debug print of array_int.                                                */
/*                                                                           */
/*****************************************************************************/

/* correct but currently unused
static void KheArrayIntDebug(HA_ARRAY_INT *array_int, FILE *fp)
{
  int val, i;
  fprintf(fp, "[");
  HaArrayForEach(*array_int, val, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    fprintf(fp, "%d", val);
  }
  fprintf(fp, "[");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheIgTaskGroupId(KHE_IG_TASK_GROUP igtg)                           */
/*                                                                           */
/*  Return an Id for igtg.  This is the Id of igtg's first task.             */
/*                                                                           */
/*****************************************************************************/

static char *KheIgTaskGroupId(KHE_IG_TASK_GROUP igtg)
{
  do
  {
    if( igtg->task != NULL )
      return KheTaskId(igtg->task);
    igtg = KheIgTaskGroupPrev(igtg);
  } while( igtg != NULL );
  return "null";
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskGroupDebugHeader(KHE_IG_TASK_GROUP igtg, FILE *fp)         */
/*                                                                           */
/*  Print the header part of the debug print of igtg.                        */
/*                                                                           */
/*****************************************************************************/

static void KheIgTaskGroupDebugHeader(KHE_IG_TASK_GROUP igtg, FILE *fp)
{
  switch( igtg->type )
  {
    case KHE_TASK_GROUPER_ENTRY_ORDINARY:

      fprintf(fp, "OrdinaryTaskGroup(%s, ", KheTaskId(igtg->task));
      KheTaskGroupDomainDebug(igtg->domain, 2, -1, stderr);
      fprintf(fp, ", primary_durn %d%s%s%s%s)",
	igtg->primary_durn,
	igtg->overhang > 0 ? ", overhang" : "",
	igtg->optional     ? ", optional" : "",
	igtg->finished_bcd ? ", finished_bcd" : "",
	igtg->expand_used ? ", expand_used" : "");
      break;

    case KHE_TASK_GROUPER_ENTRY_HISTORY:

      fprintf(fp, "HistoryTaskGroup(%d, %s)", igtg->primary_durn,
	KheResourceId(igtg->assigned_resource));
      break;

    case KHE_TASK_GROUPER_ENTRY_DUMMY:

      fprintf(fp, "DummyTaskGroup");
      break;

    default:

      HnAbort("KheIgTaskGroupDebugHeader internal error");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskGroupDebug(KHE_IG_TASK_GROUP igtg, int verbosity,          */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of igtg onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

static void KheIgTaskGroupDebug(KHE_IG_TASK_GROUP igtg, int verbosity,
  int indent, FILE *fp)
{
  if( indent < 0 )
    KheIgTaskGroupDebugHeader(igtg, fp);
  else
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheIgTaskGroupDebugHeader(igtg, fp);
    if( verbosity >= 3 && KheIgTaskGroupPrev(igtg) != NULL )
    {
      fprintf(fp, "\n");
      KheIgTaskGroupDebug(KheIgTaskGroupPrev(igtg), verbosity, indent + 2, fp);
      fprintf(fp, "%*s]\n", indent, "");
    }
    else
      fprintf(fp, " ]\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskGroupDebug(KHE_IG_TASK_GROUP igtg, int verbosity,          */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of igtg onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

/* *** good, but currently unused
static void KheIgTaskGroupDebug(KHE_IG_TASK_GROUP igtg, int verbosity,
  int indent, FILE *fp)
{
  if( indent > 0 )
  fprintf(fp, "%*s", indent, "");
  fprintf(fp, "<%s %d:%d %s", KheTaskId(igtg->task), igtg->group_length,
  igtg->overhang, KheResourceGroupId(igtg->group_domain));
  if( verbosity >= 3 && igtg->prev_task_group != NULL )
  {
    fprintf(fp, " ");
    KheIgTaskGroupDebug(igtg->prev_task_group, verbosity, -1, fp);
  }
  fprintf(fp, ">");
  if( indent >= 0 )
  fprintf(fp, "\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_TASK_GROUP_CLASS"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK_GROUP_CLASS KheIgTaskGroupClassMake(KHE_IG_SOLVER igsv)      */
/*                                                                           */
/*  Make a new ig task group class object.                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_TASK_GROUP_CLASS KheIgTaskGroupClassMake(int class_seq,
  KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK_GROUP_CLASS res;
  if( HaArrayCount(igsv->ig_task_group_class_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(igsv->ig_task_group_class_free_list);
    HaArrayClear(res->task_groups);
    HaArrayClear(res->links);
    /* HaArrayClear(res->task_classes); */
  }
  else
  {
    HaMake(res, igsv->arena);
    HaArrayInit(res->task_groups, igsv->arena);
    HaArrayInit(res->links, igsv->arena);
    /* HaArrayInit(res->task_classes, igsv->arena); */
  }
  res->expand_used_count = 0;
  res->class_seq = class_seq;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskGroupClassFree(KHE_IG_TASK_GROUP_CLASS igtgc,              */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Free igtc.                                                               */
/*                                                                           */
/*****************************************************************************/

/* *** correct but currently unused
static void KheIgTaskGroupClassFree(KHE_IG_TASK_GROUP_CLASS igtgc,
  KHE_IG_SOLVER igsv)
{
  HaArrayAddLast(igsv->ig_task_group_class_free_list, igtgc);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupClassAcceptsTaskGroup(KHE_IG_TASK_GROUP_CLASS igtgc,  */
/*    KHE_IG_TASK_GROUP igtg, KHE_IG_SOLVER igsv)                            */
/*                                                                           */
/*  Return true if igtgc accepts igtg.                                       */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskGroupClassAcceptsTaskGroup(KHE_IG_TASK_GROUP_CLASS igtgc,
  KHE_IG_TASK_GROUP igtg)
{
  KHE_IG_TASK_GROUP igtg2;
  HnAssert(HaArrayCount(igtgc->task_groups) > 0,
    "KheIgTaskGroupClassAcceptsTaskGroup internal error");
  igtg2 = HaArrayFirst(igtgc->task_groups);
  return KheIgTaskGroupSameClass(igtg, igtg2, true);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskGroupClassAddTaskGroup(KHE_IG_TASK_GROUP_CLASS igtgc,      */
/*    KHE_IG_TASK_GROUP igtg)                                                */
/*                                                                           */
/*  Add igtg to igtgc.                                                       */
/*                                                                           */
/*****************************************************************************/

static void KheIgTaskGroupClassAddTaskGroup(KHE_IG_TASK_GROUP_CLASS igtgc,
  KHE_IG_TASK_GROUP igtg)
{
  HnAssert(!igtg->expand_used,
    "KheIgTaskGroupClassAddTaskGroup internal error");
  HaArrayAddLast(igtgc->task_groups, igtg);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupClassAcceptsTaskClass(KHE_IG_TASK_GROUP_CLASS igtgc,  */
/*    KHE_IG_TASK_CLASS igtc, KHE_IG_SOLVER igsv)                            */
/*                                                                           */
/*  Return true if igtgc accepts igtc:  if igtc's tasks are acceptable       */
/*  extensions of igtc's task groups.                                        */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskGroupClassAcceptsTaskClass(KHE_IG_TASK_GROUP_CLASS igtgc,
  KHE_IG_TASK_CLASS igtc, KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK_GROUP igtg;  KHE_IG_TASK igt;
  struct khe_task_grouper_entry_rec new_entry;
  igtg = HaArrayFirst(igtgc->task_groups);
  igt = HaArrayFirst(igtc->tasks);
  return KheIgTaskGroupIsExtendable(igtg, igt, igsv, &new_entry);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskGroupClassAddLink(KHE_IG_TASK_GROUP_CLASS igtgc,           */
/*    KHE_IG_LINK link)                                                      */
/*                                                                           */
/*  Add link to igtgc.                                                       */
/*                                                                           */
/*****************************************************************************/

static void KheIgTaskGroupClassAddLink(KHE_IG_TASK_GROUP_CLASS igtgc,
  KHE_IG_LINK link)
{
  HaArrayAddLast(igtgc->links, link);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupClassEquivalentExceptForDomains(                      */
/*    KHE_IG_TASK_GROUP_CLASS igtgc1, KHE_IG_TASK_GROUP_CLASS igtgc2,        */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Return true if igtgc1 and igtgc2 are equivalent except for domains.      */
/*                                                                           */
/*****************************************************************************/

/* *** written in error, we don't need this
static bool KheIgTaskGroupClassEquivalentExceptForDomains(
  KHE_IG_TASK_GROUP_CLASS igtgc1, KHE_IG_TASK_GROUP_CLASS igtgc2,
  KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK_GROUP igtg1, igtg2;
  igtg1 = HaArrayFirst(igtgc1->task_groups);
  igtg2 = HaArrayFirst(igtgc2->task_groups);
  return KheIgTaskGroupSameClass(igtg1, igtg2, false, igsv);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheIgTaskGroupClassTypedCmp(KHE_IG_TASK_GROUP_CLASS igtgc1,          */
/*    KHE_IG_TASK_GROUP_CLASS igtgc2)                                        */
/*                                                                           */
/*  Typed comparison function for sorting an array of task group classes     */
/*  so that classes whose task groups are undersized come first.             */
/*                                                                           */
/*  Implementation note.  Sadly, we can't use KheIgTaskGroupUndersized to    */
/*  make this comparison, because we don't have access to the igsv object    */
/*  that KheIgTaskGroupUndersized needs.  But still, what we have here is    */
/*  correct.  It places optional task groups (which by definition are never  */
/*  undersized) after non-optional task groups, and within each of those     */
/*  two categories it places longer task groups after shorter task groups.   */
/*                                                                           */
/*****************************************************************************/

static int KheIgTaskGroupClassTypedCmp(KHE_IG_TASK_GROUP_CLASS igtgc1,
  KHE_IG_TASK_GROUP_CLASS igtgc2)
{
  KHE_IG_TASK_GROUP igtg1 = HaArrayFirst(igtgc1->task_groups);
  KHE_IG_TASK_GROUP igtg2 = HaArrayFirst(igtgc2->task_groups);
  int cmp;
  cmp = (int) igtg1->optional - (int) igtg2->optional;
  if( cmp != 0 )
    return cmp;
  else
    return igtg1->primary_durn - igtg2->primary_durn;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIgTaskGroupClassCmp(const void *t1, const void *t2)               */
/*                                                                           */
/*  Untyped comparison function for sorting an array of task group classes   */
/*  so that undersized ones come first.                                      */
/*                                                                           */
/*****************************************************************************/

static int KheIgTaskGroupClassCmp(const void *t1, const void *t2)
{
  KHE_IG_TASK_GROUP_CLASS igtgc1 = * (KHE_IG_TASK_GROUP_CLASS *) t1;
  KHE_IG_TASK_GROUP_CLASS igtgc2 = * (KHE_IG_TASK_GROUP_CLASS *) t2;
  return KheIgTaskGroupClassTypedCmp(igtgc1, igtgc2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIgTaskGroupClassExpandUnusedCount(KHE_IG_TASK_GROUP_CLASS igtgc)  */
/*                                                                           */
/*  Return the number of task groups of igtgc whose expand_used is false.    */
/*                                                                           */
/*****************************************************************************/

static int KheIgTaskGroupClassExpandUnusedCount(KHE_IG_TASK_GROUP_CLASS igtgc)
{
  return HaArrayCount(igtgc->task_groups) - igtgc->expand_used_count;
  /* ***
  KHE_IG_TASK_GROUP igtg;  int i, res;
  res = 0;
  HaArrayForEach(igtgc->task_groups, igtg, i)
    if( !igtg->expand_used )
      res++;
  return res;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupClassHasNonZeroOverhang(KHE_IG_TASK_GROUP_CLASS igtgc)*/
/*                                                                           */
/*  The task groups of igtgc have the same overhang.  Return true if this    */
/*  common overhang is non-zero.                                             */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskGroupClassHasNonZeroOverhang(KHE_IG_TASK_GROUP_CLASS igtgc)
{
  KHE_IG_TASK_GROUP igtg;
  igtg = HaArrayFirst(igtgc->task_groups);
  return igtg->overhang > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupClassUndersized(KHE_IG_TASK_GROUP_CLASS igtgc,        */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Return true if the task groups of igtgc are undersized.  They are either */
/*  all undersized or all not undersized.                                    */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskGroupClassUndersized(KHE_IG_TASK_GROUP_CLASS igtgc,
  KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK_GROUP igtg;
  igtg = HaArrayFirst(igtgc->task_groups);
  return KheIgTaskGroupUndersized(igtg, igsv);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIgTaskGroupClassExpandUnusedTasksCount(                           */
/*    KHE_IG_TASK_GROUP_CLASS igtgc)                                         */
/*                                                                           */
/*  Return the number of tasks available for assignment to igtgc.            */
/*                                                                           */
/*****************************************************************************/
static bool KheIgLinkIsOpen(KHE_IG_LINK link);

static int KheIgTaskGroupClassExpandUnusedTasksCount(
  KHE_IG_TASK_GROUP_CLASS igtgc)
{
  int res, i;  KHE_IG_LINK link;
  res = 0;
  HaArrayForEach(igtgc->links, link, i)
    if( KheIgLinkIsOpen(link) && link->task_class != NULL )
      res += KheIgTaskClassExpandUnusedCount(link->task_class);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskGroupClassDebugHeader(KHE_IG_TASK_GROUP_CLASS igtgc,       */
/*    KHE_IG_SOLVER igsv, FILE *fp)                                          */
/*                                                                           */
/*  Print the header part of the debug print of igtgc onto fp.               */
/*                                                                           */
/*****************************************************************************/

static void KheIgTaskGroupClassDebugHeader(KHE_IG_TASK_GROUP_CLASS igtgc,
  KHE_IG_SOLVER igsv, FILE *fp)
{
  fprintf(fp, "TaskGroupClass (undersized %s, expand_used_count %d)",
    bool_show(KheIgTaskGroupClassUndersized(igtgc, igsv)),
    igtgc->expand_used_count);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskGroupClassDebug(KHE_IG_TASK_GROUP_CLASS igtgc,             */
/*    KHE_IG_SOLVER igsv, int verbosity, int indent, FILE *fp)               */
/*                                                                           */
/*  Debug print of igtgc onto fp with the given verbosity and indent.        */
/*                                                                           */
/*****************************************************************************/
static void KheIgLinkDebug(KHE_IG_LINK link, KHE_IG_SOLVER igsv,
  int verbosity, int indent, FILE *fp);

static void KheIgTaskGroupClassDebug(KHE_IG_TASK_GROUP_CLASS igtgc,
  KHE_IG_SOLVER igsv, int verbosity, int indent, FILE *fp)
{
  KHE_IG_TASK_GROUP igtg;  int i;  KHE_IG_LINK link;
  if( indent < 0 )
    KheIgTaskGroupClassDebugHeader(igtgc, igsv, fp);
  else
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheIgTaskGroupClassDebugHeader(igtgc, igsv, fp);
    fprintf(fp, "\n");
    HaArrayForEach(igtgc->task_groups, igtg, i)
      KheIgTaskGroupDebug(igtg, verbosity, indent + 2, fp);
    if( verbosity == 5 )
      HaArrayForEach(igtgc->links, link, i)
	KheIgLinkDebug(link, igsv, verbosity - 1, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_LINK"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_LINK KheIgLinkMake(KHE_IG_TASK_GROUP_CLASS igtgc,                 */
/*    KHE_IG_TASK_CLASS igtc, KHE_IG_SOLVER igsv)                            */
/*                                                                           */
/*  Make a new link object from igtgc to igtc.  Here igtgc could be          */
/*  NULL, but igtc may not be NULL.                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_LINK KheIgLinkMake(KHE_IG_TASK_GROUP_CLASS igtgc,
  KHE_IG_TASK_CLASS igtc, KHE_IG_SOLVER igsv)
{
  KHE_IG_LINK res;  KHE_IG_TASK igt;  KHE_IG_TASK_GROUP igtg;
  KHE_TASK_GROUP_DOMAIN igtgd;
  HnAssert(igtgc != NULL || igtc != NULL, "KheIgLinkMake internal error");
  if( HaArrayCount(igsv->ig_link_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(igsv->ig_link_free_list);
    HaArrayClear(res->block_list);
  }
  else
  {
    HaMake(res, igsv->arena);
    HaArrayInit(res->block_list, igsv->arena);
  }
  res->task_group_class = igtgc;
  res->task_class = igtc;
  if( igtgc == NULL )
  {
    igt = HaArrayFirst(igtc->tasks);
    res->domain = KheTaskDomain(igt->task);
  }
  else if( igtc == NULL )
  {
    igtg = HaArrayFirst(igtgc->task_groups);
    res->domain = KheTaskGroupDomainValue(igtg->domain);
  }
  else
  {
    igtg = HaArrayFirst(igtgc->task_groups);
    igt = HaArrayFirst(igtc->tasks);
    igtgd = KheTaskGroupDomainMake(igsv->domain_finder, igtg->domain,
      KheTaskDomain(igt->task));
    res->domain = KheTaskGroupDomainValue(igtgd);
  }
  res->used_count = 0;
  res->blocked_count = 0;
  if( DEBUG52 )
  {
    if( igtgc != NULL )
    {
      int i, j;  KHE_IG_TASK_GROUP igtg;  KHE_IG_TASK igt;
      KHE_INTERVAL in1, in2;
      HaArrayForEach(igtgc->task_groups, igtg, i)
	HaArrayForEach(igtc->tasks, igt, j)
	{
	  in1 = igtg->interval;
	  in2 = KheMTaskInterval(igt->ig_mtask->mtask);
	  HnAssert(KheIntervalDisjoint(in1, in2),
	    "KheIgLinkMake internal error (igtgc %d intersects igt %d)", i, j);
	}
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgLinkMakeAndAdd(KHE_IG_TASK_GROUP_CLASS igtgc,                  */
/*    KHE_IG_TASK_CLASS igtc, KHE_IG_EXPANDER ige)                           */
/*                                                                           */
/*  Make a link from igtgc to igtc and add it to ige->all_links,             */
/*  igtgc->links (if igtgc != NULL), and igtc->links (if igtc != NULL).      */
/*                                                                           */
/*****************************************************************************/

static void KheIgLinkMakeAndAdd(KHE_IG_TASK_GROUP_CLASS igtgc,
  KHE_IG_TASK_CLASS igtc, KHE_IG_EXPANDER ige)
{
  KHE_IG_LINK res;
  res = KheIgLinkMake(igtgc, igtc, ige->solver);
  HaArrayAddLast(ige->all_links, res);
  if( igtgc != NULL )
    KheIgTaskGroupClassAddLink(igtgc, res);
  /* if( !KheIgTaskGroupClassUndersized(igtgc, ige->solver) ) */
  if( igtc != NULL )
    KheIgTaskClassAddLink(igtc, res);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgLinkFree(KHE_IG_LINK link, KHE_IG_SOLVER igsv)                 */
/*                                                                           */
/*  Free link.                                                               */
/*                                                                           */
/*****************************************************************************/

/* *** correct but currently unused
static void KheIgLinkFree(KHE_IG_LINK link, KHE_IG_SOLVER igsv)
{
  HaArrayAddLast(igsv->ig_link_free_list, link);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheDomainIsSuperSet(KHE_IG_TASK_GROUP g, KHE_IG_TASK s1,            */
/*    KHE_IG_TASK s2, bool *proper, KHE_IG_SOLVER igsv)                      */
/*                                                                           */
/*  Return true if d(g + s1) superset d(g + s2).  And if it is, set          */
/*  *proper to true if the relationship is proper.                           */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheDomainIsSuperSet(KHE_IG_TASK_GROUP g, KHE_IG_TASK s1,
  KHE_IG_TASK s2, bool *proper, KHE_IG_SOLVER igsv)
{
  KHE_RESOURCE_GROUP rgs1, rgs2, rg_s1, rg_s2;
  KHE_TASK_GROUP_DOMAIN dgs1, dgs2;  int count1, count2;
  if( g != NULL )
  {
    rg_s1 = KheTaskDomain(s1->task);
    rg_s2 = KheTaskDomain(s2->task);
    dgs1 = KheTaskGroupDomainMake(igsv->domain_finder, g->domain, rg_s1);
    rgs1 = KheTaskGroupDomainValue(dgs1 );
    dgs2 = KheTaskGroupDomainMake(igsv->domain_finder, g->domain, rg_s2);
    rgs2 = KheTaskGroupDomainValue(dgs2 );
  }
  else
  {
    rgs1 = KheTaskDomain(s1->task);
    rgs2 = KheTaskDomain(s2->task);
  }
  if( !KheResourceGroupSubset(rgs2, rgs1) )
    return *proper = false, false;
  count1 = KheResourceGroupResourceCount(rgs1);
  count2 = KheResourceGroupResourceCount(rgs2);
  return *proper = (count1 > count2), true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheDomainIsSuperSet2(KHE_IG_TASK_GROUP g1, KHE_IG_TASK_GROUP g2,    */
/*    KHE_IG_TASK s1, KHE_IG_TASK s2, bool *proper, KHE_IG_SOLVER igsv)      */
/*                                                                           */
/*  Return true if d(g1 + s2) superset d(g2 + s1).  Any one of the four      */
/*  could be NULL.  Set *proper to true if the relationship is proper.       */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheDomainIsSuperSet2(KHE_IG_TASK_GROUP g1, KHE_IG_TASK_GROUP g2,
  KHE_IG_TASK s1, KHE_IG_TASK s2, bool *proper, KHE_IG_SOLVER igsv)
{
  KHE_RESOURCE_GROUP rgs1, rgs2, rg_s1, rg_s2;
  KHE_TASK_GROUP_DOMAIN dgs1, dgs2;  int count1, count2;
  ** KHE_TASK_GROUP_DOMAIN_TYPE type; **
  if( g != NULL )
  {
    rg_s1 = KheTaskDomain(s1->task);
    rg_s2 = KheTaskDomain(s2->task);
    dgs1 = KheTaskGroupDomainMake(igsv->domain_finder, g->domain, rg_s1);
    rgs1 = KheTaskGroupDomainValue(dgs1);
    dgs2 = KheTaskGroupDomainMake(igsv->domain_finder, g->domain, rg_s2);
    rgs2 = KheTaskGroupDomainValue(dgs2);
  }
  else
  {
    rgs1 = KheTaskDomain(s1->task);
    rgs2 = KheTaskDomain(s2->task);
  }
  if( !KheResourceGroupSubset(rgs2, rgs1) )
    return *proper = false, false;
  count1 = KheResourceGroupResourceCount(rgs1);
  count2 = KheResourceGroupResourceCount(rgs2);
  return *proper = (count1 > count2), true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgLinkSuperSet(KHE_IG_LINK link11, KHE_IG_LINK link22,           */
/*    KHE_IG_LINK link12, KHE_IG_LINK link21, bool *proper)                  */
/*                                                                           */
/*  Evaluate the superset condition between these four links, including      */
/*  at least one of the two parts being a proper superset.                   */
/*                                                                           */
/*****************************************************************************/

static bool KheIgLinkSuperSet(KHE_IG_LINK link11, KHE_IG_LINK link22,
  KHE_IG_LINK link12, KHE_IG_LINK link21)
{
  int count12, count21, count11, count22;

  /* return false if condition between link12 and link11 fails */
  if( !KheResourceGroupSubset(link11->domain, link12->domain) )
    return false;

  /* return false if condition between link21 and link22 fails */
  if( !KheResourceGroupSubset(link22->domain, link21->domain) )
    return false;

  /* success, so return true if at least one of the conditions is proper */
  count11 = KheResourceGroupResourceCount(link11->domain);
  count22 = KheResourceGroupResourceCount(link22->domain);
  count12 = KheResourceGroupResourceCount(link12->domain);
  count21 = KheResourceGroupResourceCount(link21->domain);
  return count12 > count11 || count21 > count22;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgLinkRetrieve(KHE_IG_TASK_GROUP_CLASS igtgc,                    */
/*    KHE_IG_TASK_CLASS igtc, KHE_IG_LINK *res)                              */
/*                                                                           */
/*  If there is a link from igtgc to igtc, return true and set *res to       */
/*  that link, otherwise return false.  At most one of igtgc and igtc        */
/*  may be NULL.                                                             */
/*                                                                           */
/*****************************************************************************/

static bool KheIgLinkRetrieve(KHE_IG_TASK_GROUP_CLASS igtgc,
  KHE_IG_TASK_CLASS igtc, KHE_IG_LINK *res)
{
  int i;
  if( igtgc != NULL )
  {
    HaArrayForEach(igtgc->links, *res, i)
      if( (*res)->task_class == igtc )
	return true;
  }
  else if( igtc != NULL )
  {
    HaArrayForEach(igtc->links, *res, i)
      if( (*res)->task_group_class == igtgc )
	return true;
  }
  else
  {
    HnAbort("KheIgLinkRetrieve internal error");
  }

  /* no luck, no such link */
  return *res = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgLinkFindComplementaryPair(KHE_IG_LINK link11,                  */
/*    KHE_IG_LINK link22, KHE_IG_LINK *link12, KHE_IG_LINK *link21)          */
/*                                                                           */
/*  Given that link11 represents g1 + s1, and link22 represents g2 + s2,     */
/*  find the links representing g1 + s2 and g2 + s1; or return false if      */
/*  one or both of those links does not exist.                               */
/*                                                                           */
/*****************************************************************************/

static bool KheIgLinkFindComplementaryPair(KHE_IG_LINK link11,
  KHE_IG_LINK link22, KHE_IG_LINK *link12, KHE_IG_LINK *link21)
{
  if( !KheIgLinkRetrieve(link11->task_group_class, link22->task_class, link12) )
    return false;
  if( !KheIgLinkRetrieve(link22->task_group_class, link11->task_class, link21) )
    return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskClassSameExceptDomain(KHE_IG_TASK_CLASS igtc1,             */
/*    KHE_IG_TASK_CLASS igtc2)                                               */
/*                                                                           */
/*  Return true if the tasks of igtc1 and igtc2 would lie in the same        */
/*  class, were it not for their domains being different.                    */
/*                                                                           */
/*****************************************************************************/

/* *** probably better to inline this
static bool KheIgTaskClassSameExceptDomain(KHE_IG_TASK_CLASS igtc1,
  KHE_IG_TASK_CLASS igtc2)
{
  KHE_IG_TASK s1, s2;
  s1 = HaArrayFirst(igtc1->tasks);
  s2 = HaArrayFirst(igtc2->tasks);
  return KheIgTaskSameClass(s1, s2, false);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupClassSameExceptDomain(KHE_IG_TASK_GROUP_CLASS igtgc1, */
/*    KHE_IG_TASK_GROUP_CLASS igtgc2)                                        */
/*                                                                           */
/*  Return true if the task groups of igtgc1 and igtgc2 would lie in the     */
/*  same class, were it not for their domains being different.               */
/*                                                                           */
/*****************************************************************************/

/* *** probably better to inline this
static bool KheIgTaskGroupClassSameExceptDomain(KHE_IG_TASK_GROUP_CLASS igtgc1,
  KHE_IG_TASK_GROUP_CLASS igtgc2)
{
  KHE_IG_TASK_GROUP g1, g2;
  g1 = HaArrayFirst(igtgc1->task_groups);
  g2 = HaArrayFirst(igtgc2->task_groups);
  return KheIgTaskGroupSameClass(g1, g2, false);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgLinksAreBlocked(KHE_IG_LINK link11, KHE_IG_LINK link22,        */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Return true if link11 and link22 are blocked.                            */
/*                                                                           */
/*****************************************************************************/

static bool KheIgLinksAreBlocked(KHE_IG_LINK link11, KHE_IG_LINK link22,
  KHE_IG_SOLVER igsv)
{
  KHE_IG_LINK link12, link21;

  /* link22->task_group_class and link22->task_class must be non-NULL */
  if( link22->task_group_class == NULL || link22->task_class == NULL )
    return false;

  /* task group classes must be different */
  if( link11->task_group_class == link22->task_group_class )
    return false;

  /* task classes must be different */
  if( link11->task_class == link22->task_class )
    return false;

  /* the two links must have a complementary pair */
  if( !KheIgLinkFindComplementaryPair(link11, link22, &link12, &link21) )
    return false;

  /* the superset condition must hold */
  return KheIgLinkSuperSet(link11, link22, link12, link21);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgLinkAddBlock(KHE_IG_LINK link, KHE_IG_LINK blocked_link)       */
/*                                                                           */
/*  Record in link the fact that use of link prevents use of blocked_link.   */
/*                                                                           */
/*****************************************************************************/

static void KheIgLinkAddBlock(KHE_IG_LINK link, KHE_IG_LINK blocked_link)
{
  HaArrayAddLast(link->block_list, blocked_link);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgLinkIsOpen(KHE_IG_LINK link)                                   */
/*                                                                           */
/*  Return true if link is open (not blocked).                               */
/*                                                                           */
/*****************************************************************************/

static bool KheIgLinkIsOpen(KHE_IG_LINK link)
{
  return link->blocked_count == 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgLinkUseBegin(KHE_IG_LINK link)                                 */
/*                                                                           */
/*  Begin to use link.                                                       */
/*                                                                           */
/*****************************************************************************/

static void KheIgLinkUseBegin(KHE_IG_LINK link)
{
  KHE_IG_LINK link2;  int i;
  HnAssert(KheIgLinkIsOpen(link), "KheIgLinkUseBegin internal error 1");
  if( link->used_count == 0 )
    HaArrayForEach(link->block_list, link2, i)
    {
      HnAssert(link2->used_count == 0, "KheIgLinkUseBegin internal error 2");
      link2->blocked_count++;
    }
  link->used_count++;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgLinkUseEnd(KHE_IG_LINK link)                                   */
/*                                                                           */
/*  Finish using link.                                                       */
/*                                                                           */
/*****************************************************************************/

static void KheIgLinkUseEnd(KHE_IG_LINK link)
{
  KHE_IG_LINK link2;  int i;
  HnAssert(link->used_count > 0, "KheIgLinkUseEnd internal error 1");
  link->used_count--;
  if( link->used_count == 0 )
    HaArrayForEach(link->block_list, link2, i)
    {
      HnAssert(link2->blocked_count > 0, "KheIgLinkUseEnd internal error 2");
      link2->blocked_count--;
    }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgLinkDebugHeader(KHE_IG_LINK link, FILE *fp)                    */
/*                                                                           */
/*  Debug print of the header of link.                                       */
/*                                                                           */
/*****************************************************************************/

static void KheIgLinkDebugHeader(KHE_IG_LINK link, FILE *fp)
{
  fprintf(fp, "Link(used %d, blocked %d, block_list %d)", link->used_count,
    link->blocked_count, HaArrayCount(link->block_list));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgLinkDebugClasses(KHE_IG_LINK link, KHE_IG_SOLVER igsv,         */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of the task group class and task class of link.              */
/*                                                                           */
/*****************************************************************************/

static void KheIgLinkDebugClasses(KHE_IG_LINK link, KHE_IG_SOLVER igsv,
  int verbosity, int indent, FILE *fp)
{
  if( link->task_group_class != NULL )
    KheIgTaskGroupClassDebug(link->task_group_class, igsv,
      verbosity, indent, fp);
  else
    fprintf(fp, "%*sNULL Task Group Class\n", indent, "");
  if( link->task_class != NULL )
    KheIgTaskClassDebug(link->task_class, verbosity, indent, fp);
  else
    fprintf(fp, "%*sNULL Task Class\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgLinkDebug(KHE_IG_LINK link, KHE_IG_SOLVER igsv,                */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of link onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

static void KheIgLinkDebug(KHE_IG_LINK link, KHE_IG_SOLVER igsv,
  int verbosity, int indent, FILE *fp)
{
  KHE_IG_LINK link2;  int i;
  if( indent < 0 )
    KheIgLinkDebugHeader(link, fp);
  else
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheIgLinkDebugHeader(link, fp);
    fprintf(fp, "\n");
    KheIgLinkDebugClasses(link, igsv, verbosity, indent + 2, fp);
    HaArrayForEach(link->block_list, link2, i)
    {
      fprintf(fp, "%*s  blocked:\n", indent, "");
      KheIgLinkDebugClasses(link2, igsv, verbosity, indent + 4, fp);
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_SOLN" - construction                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_SOLN KheIgSolnGet(KHE_IG_SOLVER igsv)                             */
/*                                                                           */
/*  Get a fresh soln object with its task_groups field initialized but all   */
/*  other fields undefined.                                                  */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_SOLN KheIgSolnGet(KHE_IG_SOLVER igsv)
{
  KHE_IG_SOLN res;
  if( HaArrayCount(igsv->ig_soln_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(igsv->ig_soln_free_list);
    HaArrayClear(res->task_groups);
  }
  else
  {
    HaMake(res, igsv->arena);
    HaArrayInit(res->task_groups, igsv->arena);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnInitOtherFields(KHE_IG_SOLN igs, KHE_IG_SOLN prev_igs,     */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Initialize the fields of igs other than task_groups to a value suited    */
/*  to following on from prev_igs (which may be NULL).  This includes        */
/*  setting igs->randomizer to a fresh value.                                */
/*                                                                           */
/*****************************************************************************/
static int KheIgSolverRandom(KHE_IG_SOLVER igsv);

static void KheIgSolnInitOtherFields(KHE_IG_SOLN igs, KHE_IG_SOLN prev_igs,
  KHE_IG_SOLVER igsv)
{
  if( prev_igs == NULL )
  {
    igs->cost = 0;
    /* igs->varying_domain_groups = 0; */
  }
  else
  {
    igs->cost = prev_igs->cost;
    /* igs->varying_domain_groups = prev_igs->varying_domain_groups; */
  }
  igs->randomizer = KheIgSolverRandom(igsv);
#if DEBUG_COMPATIBLE
  igs->compatible = false;
  igs->cc_index = igsv->cc_index;
#endif
  igs->prev_igs = prev_igs;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnClearTaskGroups(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv)       */
/*                                                                           */
/*  Clear the task groups of igs.  Only non-initial ones are freed.          */
/*                                                                           */
/*****************************************************************************/

static void KheIgSolnClearTaskGroups(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK_GROUP igtg;  int i;
  HaArrayForEach(igs->task_groups, igtg, i)
    if( igtg->prev != NULL )
      KheIgTaskGroupFree(igtg, igsv);
  HaArrayClear(igs->task_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_SOLN KheIgSolnMake(KHE_IG_SOLN prev_igs, KHE_IG_SOLVER igsv)      */
/*                                                                           */
/*  Make a new soln object following on from prev_igs with no task groups.   */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_SOLN KheIgSolnMake(KHE_IG_SOLN prev_igs, KHE_IG_SOLVER igsv)
{
  KHE_IG_SOLN res;
  res = KheIgSolnGet(igsv);
  KheIgSolnInitOtherFields(res, prev_igs, igsv);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnReset(KHE_IG_SOLN igs, KHE_IG_SOLN prev_igs,               */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Reset igs to the state that KheIgSolnMake(prev_igs, igsv) would put it   */
/*  into, including a fresh randomizer.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheIgSolnReset(KHE_IG_SOLN igs, KHE_IG_SOLN prev_igs,
  KHE_IG_SOLVER igsv)
{
  KheIgSolnClearTaskGroups(igs, igsv);
  KheIgSolnInitOtherFields(igs, prev_igs, igsv);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_SOLN KheIgSolnCopy(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv)           */
/*                                                                           */
/*  Return a copy of igs.                                                    */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_SOLN KheIgSolnCopy(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv)
{
  KHE_IG_SOLN res;  KHE_IG_TASK_GROUP igtg;  int i;
  res = KheIgSolnGet(igsv);
  HaArrayForEach(igs->task_groups, igtg, i)
    HaArrayAddLast(res->task_groups, KheIgTaskGroupCopy(igtg, igsv));
  res->cost = igs->cost;
  /* res->varying_domain_groups = igs->varying_domain_groups; */
  res->randomizer = igs->randomizer;
#if DEBUG_COMPATIBLE
  res->compatible = igs->compatible;
  res->cc_index = igs->cc_index;
#endif
  res->prev_igs = igs->prev_igs;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnFree(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv)                  */
/*                                                                           */
/*  Free igs:  add it to the free list in igsv.  Also free its task groups.  */
/*                                                                           */
/*****************************************************************************/

static void KheIgSolnFree(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv)
{
  KheIgSolnClearTaskGroups(igs, igsv);
  HaArrayAddLast(igsv->ig_soln_free_list, igs);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_SOLN" - query                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheIgSolnNonSelfFinishedGroupCount(KHE_IG_SOLN igs)                  */
/*                                                                           */
/*  Return the number of non-self-finished task groups in igs, assuming      */
/*  that the task groups are sorted so that the self-finished ones appear    */
/*  at the end.  Note that the code searches the task groups in reverse.     */
/*                                                                           */
/*****************************************************************************/

static int KheIgSolnNonSelfFinishedGroupCount(KHE_IG_SOLN igs)
{
  int i;  KHE_IG_TASK_GROUP igtg;
  HaArrayForEachReverse(igs->task_groups, igtg, i)
    if( !KheIgTaskGroupIsSelfFinished(igtg) )
      break;
  return i + 1;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgSolnTaskContinuesAsst(KHE_IG_SOLN igs, KHE_TASK task)          */
/*                                                                           */
/*  Return true if there is only one choice for task because it continues    */
/*  with the same resource assignment as one of the task groups of soln.     */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheIgSolnTaskContinuesAsst(KHE_IG_SOLN igs, KHE_TASK task)
{
  KHE_RESOURCE r;  KHE_IG_TASK_GROUP igtg;  int i;
  r = KheTaskAsstResource(task);
  if( r == NULL )
    return false;
  HaArrayForEach(igs->task_groups, igtg, i)
    if( igtg->assigned_resource == r )
      return true;
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_SOLN" - complex operations                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheIgSolnAddLengthenerTasks(KHE_IG_SOLN best_igs,KHE_IG_SOLVER igsv)*/
/*                                                                           */
/*  Try adding lengthener tasks to igsv based on best_igs, and return true   */
/*  if any were added.  Otherwise return false.                              */
/*                                                                           */
/*****************************************************************************/

static bool KheIgSolnAddLengthenerTasks(KHE_IG_SOLN best_igs,
  KHE_IG_SOLVER igsv)
{
  KHE_IG_SOLN igs;  int i, j, k;  KHE_IG_TASK_GROUP dt, dt2, prev;
  KHE_IG_TIME_GROUP igtg, succ_igtg, pred_igtg;  bool res;  KHE_IG_MTASK igmt;
  KHE_RESOURCE_GROUP rg;  KHE_TASK task;  KHE_IG_TASK igt;
  KHE_COST non_asst_cost, asst_cost;

  if( DEBUG39 )
    fprintf(stderr, "[ KheIgSolnAddLengthenerTasks(best_igs, igsv)\n");

  /* can't compare with other_igs after adding lengthener tasks */
#if DEBUG_COMPATIBLE
  HaArrayForEach(igsv->time_groups, igtg, i)
    if( igtg->other_igs != NULL )
    {
      KheIgSolnFree(igtg->other_igs, igsv);
      igtg->other_igs = NULL;
    }
#endif

  /* set all dt->last_of_best_group to true */
  for( igs = best_igs;  igs != NULL;  igs = igs->prev_igs )
    HaArrayForEach(igs->task_groups, dt, j)
      dt->last_of_best_group = true;

  /* set some dt->last_of_best_group to false */
  for( igs = best_igs;  igs != NULL;  igs = igs->prev_igs )
    HaArrayForEach(igs->task_groups, dt, j)
    {
      prev = KheIgTaskGroupPrev(dt);
      if( prev != NULL )
	prev->last_of_best_group = false;
    }

  /* look for time groups that will suit us */
  res = false;
  igs = best_igs;
  succ_igtg = NULL;
  HaArrayForEachReverse(igsv->time_groups, igtg, i)
  {
    if( succ_igtg != NULL ) /* correctly ignores undersized groups at the end */
    {
      HaArrayForEach(igs->task_groups, dt, j)   /* igs lies in igtg */
      {
	if( dt->last_of_best_group && KheIgTaskGroupUndersized(dt, igsv) )
	{
	  /* undersized, try increasing its length at the end */
	  rg = KheIgTaskGroupDomain(dt);
	  if( KheIgTimeGroupFindBestLengthenerMTask(succ_igtg, rg,false,&igmt) )
	  {
	    task = KheMTaskTask(igmt->mtask,
	      HaArrayCount(igmt->included_tasks), &non_asst_cost, &asst_cost);
	    igt = KheIgMTaskAddTask(igmt, task, true, igsv);
	    if( DEBUG39 )
	      KheIgTaskDebug(igt, 2, 2, stderr);
	    res = true;
	  }

	  /* undersized, try increasing its length at the start */
	  k = i, dt2 = KheIgTaskGroupPrev(dt);
	  while( dt2 != NULL )
	    k--, dt2 = KheIgTaskGroupPrev(dt2);
	  if( k > 0 )
	  {
	    pred_igtg = HaArray(igsv->time_groups, k - 1);
	    /* rg = KheIgTaskGroupDomain(dt); */
	    if( KheIgTimeGroupFindBestLengthenerMTask(pred_igtg,rg,true,&igmt) )
	    {
	      task = KheMTaskTask(igmt->mtask,
		HaArrayCount(igmt->included_tasks), &non_asst_cost, &asst_cost);
	      igt = KheIgMTaskAddTask(igmt, task, true, igsv);
	      if( DEBUG39 )
		KheIgTaskDebug(igt, 2, 2, stderr);
	      res = true;
	    }
	  }
	}
      }
    }
    succ_igtg = igtg;
    igs = igs->prev_igs;
  }

  /* all done */
  if( DEBUG39 )
    fprintf(stderr, "] KheIgSolnAddLengthenerTasks returning %s\n",
      bool_show(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgSolnDominates(KHE_IG_SOLN igs1, KHE_IG_SOLN igs2,              */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Return true if igs1 dominates igs2.                                      */
/*                                                                           */
/*  Implementation note.  In actual use, the number of task groups in        */
/*  the two solutions will always be equal, because it is equal to the       */
/*  number of included tasks at the time the solution ends.  However,        */
/*  this function is also used for comparing a solution generated by         */
/*  the algorithm with a solution generated by another solver, and           */
/*  that other solution need not contain exactly the same tasks.  So         */
/*  here we simply return false if the number of task groups differs.        */
/*                                                                           */
/*****************************************************************************/
static void KheIgSolnDebug(KHE_IG_SOLN igs, int verbosity, int indent,
  FILE *fp);

static bool KheIgSolnDominates(KHE_IG_SOLN igs1, KHE_IG_SOLN igs2,
  KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK_GROUP igtg1, igtg2;  int i, count1, count2;
  bool equal_so_far;

  /* *** see implementation note above
  if( DEBUG46 )
  {
    if( HaArrayCount(igs1->task_groups) != HaArrayCount(igs2->task_groups) )
    {
      fprintf(stderr, "KheIgSolnDominates failing (%d != %d)\n",
        HaArrayCount(igs1->task_groups), HaArrayCount(igs2->task_groups));
      KheIgSolnDebug(igs1, 2, 2, stderr);
      KheIgSolnDebug(igs2, 2, 2, stderr);
    }
  }
  HnAssert(HaArrayCount(igs1->task_groups) == HaArrayCount(igs2->task_groups),
    "KheIgSolnDominates internal error 1");
  *** */

  /* for dominance, the cost of igs1 must not exceed the cost of igs2 */
  if( igs1->cost > igs2->cost )
    return false;
  equal_so_far = (igs1->cost == igs2->cost);

  /* for dominance, equal numbers of non-self-finished groups must be present */
  count1 = KheIgSolnNonSelfFinishedGroupCount(igs1);
  count2 = KheIgSolnNonSelfFinishedGroupCount(igs2);
  if( count1 != count2 )
    return false;

  /* for dominance, corresponding non-self-finished groups must dominate */
  for( i = 0;  i < count1;  i++ )
  {
    igtg1 = HaArray(igs1->task_groups, i);
    igtg2 = HaArray(igs2->task_groups, i);
    if( !KheIgTaskGroupDominates(igtg1, igtg2, igsv, &equal_so_far) )
      return false;
  }

  /* if not equal_so_far, igs1 dominates */
  if( !equal_so_far )
    return true;

  /* now equal, so try the number of varying domain groups */
  /* ***
  if( igs1->varying_domain_groups != igs2->varying_domain_groups )
    return igs1->varying_domain_groups < igs2->varying_domain_groups;
  *** */

  /* everything equal, use randomizer as last resort */
  return igs1->randomizer <= igs2->randomizer;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupBuildAndClear(KHE_IG_TASK_GROUP last_igtg,              */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Build one task group starting at last_igtg.  Also clear it out.          */
/*                                                                           */
/*****************************************************************************/

static void KheTaskGroupBuildAndClear(KHE_IG_TASK_GROUP last_igtg,
  KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK_GROUP igtg, prev_igtg;

  /* build the group */
  KheTaskGrouperEntryMakeGroup((KHE_TASK_GROUPER_ENTRY) last_igtg,
    igsv->soln_adjuster);

  /* break up the group, to prevent it from being grouped again */
  for( igtg = last_igtg;  igtg != NULL;  igtg = prev_igtg )
  {
    prev_igtg = KheIgTaskGroupPrev(igtg);
    igtg->prev = NULL;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIgSolnBuildGroups(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv)            */
/*                                                                           */
/*  Build the groups defined by igs and return the number of groups made.    */
/*  Parameter run is only for debugging.                                     */
/*                                                                           */
/*****************************************************************************/

static int KheIgSolnBuildGroups(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK_GROUP igtg;  int i, res;
  if( DEBUG11 )
  {
    fprintf(stderr, "[ KheIgSolnBuildGroups(%s, %s, %s) cc_index %d:\n",
      KheInstanceId(KheSolnInstance(igsv->soln)),
      KheResourceTypeId(igsv->resource_type),
      KheConstraintClassId(igsv->curr_class), igsv->cc_index);
    KheIgSolnDebugTimetable(igs, igsv, 2, 2, stderr);
  }
  res = 0;
  for( ;  igs != NULL;  igs = igs->prev_igs )
  {
    HaArrayForEach(igs->task_groups, igtg, i)
      if( igtg->prev != NULL )
      {
	KheTaskGroupBuildAndClear(igtg, igsv);
	res++;
      }
  }
  if( DEBUG11 )
    fprintf(stderr, "] KheIgSolnBuildGroups returning %d\n", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIgSolnTypedCmp(KHE_IG_SOLN igs1, KHE_IG_SOLN igs2)                */
/*                                                                           */
/*  Typed comparison function for sorting an array of solutions by           */
/*  increasing cost.                                                         */
/*                                                                           */
/*****************************************************************************/

static int KheIgSolnTypedCmp(KHE_IG_SOLN igs1, KHE_IG_SOLN igs2)
{
  return KheCostCmp(igs1->cost, igs2->cost);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIgSolnCmp(const void *t1, const void *t2)                         */
/*                                                                           */
/*  Untyped comparison function for sorting an array of solutions by         */
/*  increasing cost.                                                         */
/*                                                                           */
/*****************************************************************************/

static int KheIgSolnCmp(const void *t1, const void *t2)
{
  KHE_IG_SOLN igs1 = * (KHE_IG_SOLN *) t1;
  KHE_IG_SOLN igs2 = * (KHE_IG_SOLN *) t2;
  return KheIgSolnTypedCmp(igs1, igs2);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_SOLN - history"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheBusyDaysLimit(KHE_IG_SOLVER igsv, KHE_RESOURCE r)                 */
/*                                                                           */
/*  Return r's busy days limit.                                              */
/*                                                                           */
/*****************************************************************************/

static int KheBusyDaysLimit(KHE_IG_SOLVER igsv, KHE_RESOURCE r)
{
  int res, i, lim;  KHE_CONSTRAINT_CLASS cc;
  res = INT_MAX;
  HaArrayForEach(igsv->busy_days_classes, cc, i)
  {
    lim = KheConstraintClassResourceMaximumMinusHistory(cc, r);
    if( lim < res )
      res = lim;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceExtraDuration(KHE_RESOURCE r, KHE_IG_SOLVER igsv)         */
/*                                                                           */
/*  Return the extra duration needed for resource r.                         */
/*                                                                           */
/*****************************************************************************/

static int KheResourceExtraDuration(KHE_RESOURCE r, KHE_IG_SOLVER igsv)
{
  int res, bd_limit;
  res = KheConstraintClassResourceHistory(igsv->curr_class, r);
  if( res > 0 )
  {
    if( DEBUG16 )
      fprintf(stderr, "[ KheResourceExtraDuration(%s)\n", KheResourceId(r));
    bd_limit = KheBusyDaysLimit(igsv, r);
    if( bd_limit < INT_MAX && igsv->max_limit - bd_limit > res )
    {
      res = igsv->max_limit - bd_limit;
      if( DEBUG16 )
       fprintf(stderr,
	  "] KheResourceExtraDuration returning busy %d - %d = %d\n",
	  igsv->max_limit, bd_limit, res);
    }
    else
      if( DEBUG16 )
	fprintf(stderr, "] KheResourceExtraDuration returning history %d\n",
	  res);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK_GROUP KheIgBuildHistoryTaskGroup(KHE_RESOURCE r,             */
/*    int durn, KHE_IG_SOLVER igsv)                                          */
/*                                                                           */
/*  Make a history task group with these attributes.                         */
/*                                                                           */
/*****************************************************************************/

/* *** renamed KheIgTaskGroupMakeHistory and moved
static KHE_IG_TASK_GROUP KheIgBuildHistoryTaskGroup(KHE_RESOURCE r,
  int primary_durn, KHE_IG_SOLVER igsv)
{
  struct khe_task_grouper_entry_rec new_entry;
  KheTaskGrouperEntryAddHistory(NULL, r, primary_durn, &new_entry);
  return KheIgTaskGr oupMake(&new_entry, false, false, primary_durn, 0, igsv);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_SOLN KheIgBuildHistorySoln(KHE_IG_SOLVER igsv,                    */
/*    KHE_CONSTRAINT_CLASS cc)                                               */
/*                                                                           */
/*  Build an initial solution containing just history groups.                */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_SOLN KheIgBuildHistorySoln(KHE_IG_SOLVER igsv)
{
  KHE_IG_SOLN res;  int i, durn;  KHE_RESOURCE r;  KHE_IG_TASK_GROUP igtg;
  res = KheIgSolnMake(NULL, igsv);
  for( i = 0;  i < KheResourceTypeResourceCount(igsv->resource_type);  i++ )
  {
    r = KheResourceTypeResource(igsv->resource_type, i);
    durn = KheResourceExtraDuration(r, igsv);
    if( durn > 0 )
    {
      igtg = KheIgTaskGroupMakeHistory(r, durn, igsv);
      HaArrayAddLast(res->task_groups, igtg);
      if( DEBUG20 )
	fprintf(stderr, "  adding history(%s, %d) task group\n",
	  KheResourceId(r), durn);
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_SOLN" - debug and display                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnDebugHeader(KHE_IG_SOLN igs, FILE *fp)                     */
/*                                                                           */
/*  Print the header of the debug of igs.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheIgSolnDebugHeader(KHE_IG_SOLN igs, FILE *fp)
{
  fprintf(fp, "Soln(%.5f, randomizer %d, %d groups)",
    KheCostShow(igs->cost), igs->randomizer, HaArrayCount(igs->task_groups));
  /* ***
  fprintf(fp, "Soln(%.5f, varying_domain_groups %d, randomizer %d, %d groups)",
    KheCostShow(igs->cost), igs->varying_domain_groups, igs->randomizer,
    HaArrayCount(igs->task_groups));
  *** */
#if DEBUG_COMPATIBLE
  fprintf(fp, " cc_index %d", igs->cc_index);
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnDebug(KHE_IG_SOLN igs, int verbosity, int indent,          */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of igs onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheIgSolnDebug(KHE_IG_SOLN igs, int verbosity, int indent,
  FILE *fp)
{
  KHE_IG_TASK_GROUP igtg;  int i;
  if( indent < 0 )
    KheIgSolnDebugHeader(igs, fp);
  else
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheIgSolnDebugHeader(igs, fp);
    fprintf(fp, "\n");
    HaArrayForEach(igs->task_groups, igtg, i)
      KheIgTaskGroupDebug(igtg, verbosity, indent + 2, fp);
    if( verbosity >= 4 && igs->prev_igs != NULL )
      KheIgSolnDebug(igs->prev_igs, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUPED_TASKS_DISPLAY KheIgSolnDisplay(KHE_IG_SOLN igs,              */
/*    KHE_IG_SOLVER igsv, HA_ARENA a)                                        */
/*                                                                           */
/*  Return a grouped tasks display representing igs.                         */
/*                                                                           */
/*****************************************************************************/

static KHE_GROUPED_TASKS_DISPLAY KheIgSolnDisplay(KHE_IG_SOLN igs,
  KHE_IG_SOLVER igsv, HA_ARENA a)
{
  KHE_GROUPED_TASKS_DISPLAY gtd;  KHE_INTERVAL in;
  KHE_IG_TASK_GROUP igtg;  int i, minimum, maximum;  KHE_IG_SOLN next_igs;
  KHE_CONSTRAINT_CLASS cc;
  cc = igsv->curr_class;
  minimum = KheConstraintClassMinimum(cc);
  maximum = KheConstraintClassMaximum(cc);
  gtd = KheGroupedTasksDisplayMake(igsv->soln, KheConstraintClassId(cc),
    minimum, maximum, igs->cost, igsv->days_frame, a);
  next_igs = NULL;
  while( igs != NULL )
  {
    HaArrayForEach(igs->task_groups, igtg, i)
      if( /*!igtg->optional &&*/ !KheIgTaskGroupIsPredecessor(igtg, next_igs) )
      {
	KheGroupedTasksDisplayGroupBegin(gtd, igtg->optional, i);
	do
	{
	  switch( KheTaskGrouperEntryType((KHE_TASK_GROUPER_ENTRY) igtg) )
	  {
	    case KHE_TASK_GROUPER_ENTRY_ORDINARY:

	      /* add one task to the display */
	      KheGroupedTasksDisplayGroupAddTask(gtd, igtg->task);
	      break;

	    case KHE_TASK_GROUPER_ENTRY_HISTORY:

	      /* add history to the display */
	      in = KheTaskGrouperEntryInterval((KHE_TASK_GROUPER_ENTRY) igtg);
              KheGroupedTasksDisplayGroupAddHistory(gtd,
		igtg->assigned_resource, KheIntervalLength(in));
	      break;

	    case KHE_TASK_GROUPER_ENTRY_DUMMY:

	      /* nothing to do here */
	      break;
	  }
	  igtg = KheIgTaskGroupPrev(igtg);
	} while( igtg != NULL );
	KheGroupedTasksDisplayGroupEnd(gtd);
      }
    next_igs = igs;
    igs = igs->prev_igs;
  }
  return gtd;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnDebugTimetable(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv,        */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of igs (in the form of a timetable) onto fp with the given   */
/*  verbosity and indent.                                                    */
/*                                                                           */
/*****************************************************************************/

static void KheIgSolnDebugTimetable(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv,
  int verbosity, int indent, FILE *fp)
{
  KHE_GROUPED_TASKS_DISPLAY gtd;  HA_ARENA a;
  a = KheSolnArenaBegin(igsv->soln);
  gtd = KheIgSolnDisplay(igs, igsv, a);
  KheGroupedTasksDisplayPrint(gtd, false, indent, fp);
  KheSolnArenaEnd(igsv->soln, a);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_SOLN_SET"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_SOLN_SET KheIgSolnSetMake(KHE_IG_SOLVER igsv)                     */
/*                                                                           */
/*  Make a new, empty solution set.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_IG_SOLN_SET KheIgSolnSetMake(KHE_IG_SOLVER igsv)
{
  KHE_IG_SOLN_SET res;
  if( HaArrayCount(igsv->ig_soln_set_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(igsv->ig_soln_set_free_list);
    HaArrayClear(res->solns);
  }
  else
  {
    HaMake(res, igsv->arena);
    HaArrayInit(res->solns, igsv->arena);
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnSetFree(KHE_IG_SOLN_SET igss, KHE_IG_SOLVER igsv)          */
/*                                                                           */
/*  Free igss, but don't free its solutions.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheIgSolnSetFree(KHE_IG_SOLN_SET igss, KHE_IG_SOLVER igsv)
{
  HaArrayAddLast(igsv->ig_soln_set_free_list, igss);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnSetAddSoln(KHE_IG_SOLN_SET igss, KHE_IG_SOLN igs,          */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Add igs to igss, with dominance testing.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheIgSolnSetAddSoln(KHE_IG_SOLN_SET igss, KHE_IG_SOLN igs,
  KHE_IG_SOLVER igsv)
{
  KHE_IG_SOLN igs2;  int i;

  ** if igs is dominated, free igs and return without changing anything else **
  HaArrayForEach(igss->solns, igs2, i)
    if( KheIgSolnDominates(igs2, igs, igsv) )
    {
#if DEBUG_COMPATIBLE
      if( DEBUG43 && igs->compatible )
      {
	fprintf(stderr, "  new compatible solution dominated by this %s "
	  "solution:\n", igs2->compatible ? "compatible" : "incompatible");
	KheIgSolnDebugTimetable(igs2, igsv, 2, 2, stderr);
      }
#endif
      KheIgSolnFree(igs, igsv);
      if( DEBUG24 )
	fprintf(stderr, "] KheIgSolnSetAddSoln returning "
	  "(dominated by soln %d)\n", i);
      return;
    }

  ** igs is not dominated, so free anything dominated by it **
  HaArrayForEach(igss->solns, igs2, i)
    if( KheIgSolnDominates(igs, igs2, igsv) )
    {
#if DEBUG_COMPATIBLE
      if( DEBUG43 && igs2->compatible )
      {
	fprintf(stderr, "  this previously saved compatible solution:\n");
	KheIgSolnDebugTimetable(igs2, igsv, 2, 2, stderr);
	fprintf(stderr, "  is dominated by this new %s solution:\n",
	  igs->compatible ? "compatible" : "incompatible");
	KheIgSolnDebugTimetable(igs, igsv, 2, 2, stderr);
      }
#endif
      KheIgSolnFree(igs2, igsv);
      HaArrayDeleteAndPlug(igss->solns, i);
      if( DEBUG24 )
	fprintf(stderr, "  KheIgSolnSetAddSoln deleted dominated soln "
	  "%d, %d solns left\n", i, HaArrayCount(igss->solns));
      i--;
    }

  ** finally, add igs **
  HaArrayAddLast(igss->solns, igs);
  if( DEBUG24 )
    fprintf(stderr, "] KheIgSolnSetAddSoln returning (added), %d solns\n",
      HaArrayCount(igss->solns));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_SOLN_CACHE"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_SOLN_CACHE KheIgSolnCacheMake(KHE_IG_SOLVER igsv)                 */
/*                                                                           */
/*  Make a new, empty soln cache.  NB we don't worry about freeing them.     */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_IG_SOLN_CACHE KheIgSolnCacheMake(KHE_IG_SOLVER igsv)
{
  KHE_IG_SOLN_CACHE res;
  HaMake(res, igsv->arena);
  res->total_solns = 0;
  res->undominated_solns = 0;
#if DEBUG_COMPATIBLE
  res->other_igs = NULL;
  res->compatible_solns = 0;
#endif
  HaArrayInit(res->soln_sets, igsv->arena);
  HaArrayInit(res->final_solns, igsv->arena);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnCacheClear(KHE_IG_SOLN_CACHE igsc, KHE_IG_SOLVER igsv)     */
/*                                                                           */
/*  Clear igsc back to empty, including freeing the solns in final_solns.    */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheIgSolnCacheClear(KHE_IG_SOLN_CACHE igsc, KHE_IG_SOLVER igsv)
{
  KHE_IG_SOLN_SET igss;  int i;  KHE_IG_SOLN igs;
  igsc->total_solns = 0;
  igsc->undominated_solns = 0;
  HaArrayForEach(igsc->soln_sets, igss, i)
    if( igss != NULL )
      KheIgSolnSetFree(igss, igsv);
  HaArrayClear(igsc->soln_sets);
  HaArrayForEach(igsc->final_solns, igs, i)
    KheIgSolnFree(igs, igsv);
  HaArrayClear(igsc->final_solns);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnCacheAddSoln(KHE_IG_SOLN_CACHE igsc, KHE_IG_SOLN igs,      */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Add igs to igsc, with dominance testing.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheIgSolnNonSelfFinishedGroupCount(KHE_IG_SOLN igs);

static void KheIgSolnCacheAddSoln(KHE_IG_SOLN_CACHE igsc, KHE_IG_SOLN igs,
  KHE_IG_SOLVER igsv)
{
  int index;  KHE_IG_SOLN_SET igss;

  ** keep some running totals **
  igsc->total_solns++;
#if DEBUG_COMPATIBLE
  if( igs->compatible )
    igtg->compatible_solns++;
#endif

  ** find the appropriate soln set and add igs to that **
  index = KheIgSolnNonSelfFinishedGroupCount(igs);
  HaArrayFill(igsc->soln_sets, index + 1, NULL);
  igss = HaArray(igsc->soln_sets, index);
  if( igss == NULL )
  {
    igss = KheIgSolnSetMake(igsv);
    HaArrayPut(igsc->soln_sets, index, igss);
  }
  KheIgSolnSetAddSoln(igss, igs, igsv);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnCacheFinalize(KHE_IG_SOLN_CACHE igsc, KHE_IG_SOLVER igsv)  */
/*                                                                           */
/*  Finalize igsc.  This involves merging the solution sets into a single    */
/*  list, sorting them by increasing cost, then deleting the most costly     */
/*  ones until there are at most MAX_KEEP left.                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheIgSolnCacheFinalize(KHE_IG_SOLN_CACHE igsc, KHE_IG_SOLVER igsv)
{
  int i, j;  KHE_IG_SOLN_SET igss;  KHE_IG_SOLN igs;  bool started;
  if( DEBUG53 )
  {
    fprintf(stderr, "KheIgSolnCacheFinalize(");
    started = false;
    HaArrayForEach(igsc->soln_sets, igss, i)
      if( igss != NULL )
      {
	if( started )
	  fprintf(stderr, ", ");
	fprintf(stderr, "%d:%d", i, HaArrayCount(igss->solns));
	started = true;
      }
    fprintf(stderr, ")\n");
  }
  HaArrayClear(igsc->final_solns);
  HaArrayForEach(igsc->soln_sets, igss, i)
    if( igss != NULL )
      HaArrayAppend(igsc->final_solns, igss->solns, j);
#if DEBUG_COMPATIBLE
  HaArrayForEach(igsc->final_solns, igs, j)
    HnAssert(igs->run==igsv->run, "KheIgSolnCacheFinalize internal error");
#endif
  HaArraySort(igsc->final_solns, &KheIgSolnCmp);
  igsc->undominated_solns = HaArrayCount(igsc->final_solns);
  while( HaArrayCount(igsc->final_solns) > MAX_KEEP )
  {
    igs = HaArrayLastAndDelete(igsc->final_solns);
    KheIgSolnFree(igs, igsv);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnCacheDebugFinalSolnsHeader(KHE_IG_SOLN_CACHE igsc,         */
/*    char *label, FILE *fp)                                                 */
/*                                                                           */
/*  Print the header of the debug print of igsc's final solutions onto fp.   */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheIgSolnCacheDebugFinalSolnsHeader(KHE_IG_SOLN_CACHE igsc,
  char *label, FILE *fp)
{
  fprintf(fp, "Final solutions(%s, %d made, ", label, igsc->total_solns);
#if DEBUG_COMPATIBLE
  fprintf(fp, "%d compatible, ", igsc->compatible_solns);
#endif
  fprintf(fp, "%d undominated, %d kept)", igsc->undominated_solns,
    HaArrayCount(igsc->final_solns));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnCacheDebugFinalSolns(KHE_IG_SOLN_CACHE igsc,               */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of igsc's final solutions onto fp with the given verbosity   */
/*  and indent.                                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheIgSolnCacheDebugFinalSolns(KHE_IG_SOLN_CACHE igsc,
  char *label, KHE_IG_SOLVER igsv, int verbosity, int indent, FILE *fp)
{
  KHE_IG_SOLN igs;  int i;
  if( indent < 0 )
    KheIgSolnCacheDebugFinalSolnsHeader(igsc, label, fp);
  else if( verbosity == 1 )
  {
    fprintf(fp, "%*s", indent, "");
    KheIgSolnCacheDebugFinalSolnsHeader(igsc, label, fp);
    fprintf(fp, "\n");
  }
  else
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheIgSolnCacheDebugFinalSolnsHeader(igsc, label, fp);
    if( HaArrayCount(igsc->final_solns) <= 30 )
    {
      HaArrayForEach(igsc->final_solns, igs, i)
      {
	fprintf(stderr, "\n");
	KheIgSolnDebugTimetable(igs, igsv, verbosity, indent+2, stderr);
      }
    }
    else
    {
      fprintf(fp, "\n");
      igs = HaArrayFirst(igsc->final_solns);
      KheIgSolnDebugTimetable(igs, igsv, verbosity, indent + 2, fp);
      fprintf(fp, "%*s  ... ... ...\n", indent, "");
      igs = HaArrayLast(igsc->final_solns);
      KheIgSolnDebugTimetable(igs, igsv, verbosity, indent + 2, fp);
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_SOLN_SET_TRIE"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnSetTrieClear(KHE_IG_SOLN_SET_TRIE trie)                    */
/*                                                                           */
/*  Clear trie but not its solutions.                                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheIgSolnSetTrieClear(KHE_IG_SOLN_SET_TRIE trie, KHE_IG_SOLVER igsv)
{
  KHE_IG_SOLN_SET_TRIE child;  int i;
  if( trie != NULL )
  {
    HaArrayForEach(trie->children, child, i)
      KheIgSolnSetTrieClear(child, igsv);
    HaArrayAddLast(igsv->ig_soln_set_trie_free_list, trie);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_SOLN_SET_TRIE KheIgSolnSetTrieMake(KHE_IG_SOLVER igsv)            */
/*                                                                           */
/*  Make a new, empty soln set trie node.                                    */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_SOLN_SET_TRIE KheIgSolnSetTrieMake(KHE_IG_SOLVER igsv)
{
  KHE_IG_SOLN_SET_TRIE res;
  if( HaArrayCount(igsv->ig_soln_set_trie_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(igsv->ig_soln_set_trie_free_list);
    HaArrayClear(res->solns);
    HaArrayClear(res->children);
  }
  else
  {
    HaMake(res, igsv->arena);
    HaArrayInit(res->solns, igsv->arena);
    HaArrayInit(res->children, igsv->arena);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_SOLN_SET_TRIE KheIgSolnSetTrieExtend(KHE_IG_SOLN_SET_TRIE trie,   */
/*    int index)                                                             */
/*                                                                           */
/*  Ensure that trie has a child at index, and return that child.            */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_SOLN_SET_TRIE KheIgSolnSetTrieExtend(KHE_IG_SOLN_SET_TRIE trie,
  int index, KHE_IG_SOLVER igsv)
{
  KHE_IG_SOLN_SET_TRIE res;
  HaArrayFill(trie->children, index + 1, NULL);
  res = HaArray(trie->children, index);
  if( res == NULL )
  {
    res = KheIgSolnSetTrieMake(igsv);
    HaArrayPut(trie->children, index, res);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_SOLN_SET_TRIE KheIgSolnSetTrieRetrieve(                           */
/*    KHE_IG_SOLN_SET_TRIE *trie, KHE_IG_SOLN soln)                          */
/*                                                                           */
/*  Retrieve from *trie the node that should contain soln.  This always      */
/*  succeeds because it makes the node if it is not already present.         */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_SOLN_SET_TRIE KheIgSolnSetTrieRetrieve(
  KHE_IG_SOLN_SET_TRIE *trie, KHE_IG_SOLN igs, KHE_IG_SOLVER igsv)
{
  int non_self_finished_count, i;  KHE_IG_SOLN_SET_TRIE res;
  KHE_IG_TASK_GROUP igtg;

  /* make the root node if required */
  if( *trie == NULL )
    *trie = KheIgSolnSetTrieMake(igsv);

  /* extend by KheIgSolnNonSelfFinishedGroupCount */
  non_self_finished_count = KheIgSolnNonSelfFinishedGroupCount(igs);
  res = KheIgSolnSetTrieExtend(*trie, non_self_finished_count, igsv);

  /* extend by primary durns */
  for( i = 0;  i < non_self_finished_count;  i++ )
  {
    igtg = HaArray(igs->task_groups, i);
    res = KheIgSolnSetTrieExtend(res, igtg->primary_durn, igsv);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnSetTrieAddSoln(KHE_IG_SOLN_SET_TRIE *trie,                 */
/*    KHE_IG_SOLN soln, KHE_IG_SOLVER igsv)                                  */
/*                                                                           */
/*  Add soln to trie, with dominance testing.                                */
/*                                                                           */
/*****************************************************************************/
static KHE_IG_TASK_GROUP KheIgTaskGroupPrev(KHE_IG_TASK_GROUP igtg);

static void KheIgSolnSetTrieAddSoln(KHE_IG_SOLN_SET_TRIE *trie,
  KHE_IG_SOLN igs, KHE_IG_SOLVER igsv)
{
  KHE_IG_SOLN_SET_TRIE node;  KHE_IG_SOLN igs2;  int i;

  /* find the node to insert into, possibly updating *trie */
  node = KheIgSolnSetTrieRetrieve(trie, igs, igsv);

  /* if igs is dominated, free igs and return without changing anything else */
  HaArrayForEach(node->solns, igs2, i)
    if( KheIgSolnDominates(igs2, igs, igsv) )
    {
#if DEBUG_COMPATIBLE
      if( DEBUG43 && igs->compatible )
      {
	fprintf(stderr, "  new compatible solution dominated by this %s "
	  "solution:\n", igs2->compatible ? "compatible" : "incompatible");
	KheIgSolnDebugTimetable(igs2, igsv, 2, 2, stderr);
      }
#endif
      KheIgSolnFree(igs, igsv);
      if( DEBUG24 )
	fprintf(stderr, "] KheIgSolnSetAddSoln returning "
	  "(dominated by soln %d)\n", i);
      return;
    }

  /* igs is not dominated, so free anything dominated by it */
  HaArrayForEach(node->solns, igs2, i)
    if( KheIgSolnDominates(igs, igs2, igsv) )
    {
#if DEBUG_COMPATIBLE
      if( DEBUG43 && igs2->compatible )
      {
	fprintf(stderr, "  this previously saved compatible solution:\n");
	KheIgSolnDebugTimetable(igs2, igsv, 2, 2, stderr);
	fprintf(stderr, "  is dominated by this new %s solution:\n",
	  igs->compatible ? "compatible" : "incompatible");
	KheIgSolnDebugTimetable(igs, igsv, 2, 2, stderr);
      }
#endif
      KheIgSolnFree(igs2, igsv);
      HaArrayDeleteAndPlug(node->solns, i);
      if( DEBUG24 )
	fprintf(stderr, "  KheIgSolnSetAddSoln deleted dominated soln "
	  "%d, %d solns left in node\n", i, HaArrayCount(node->solns));
      i--;
    }

  /* finally, add igs */
  HaArrayAddLast(node->solns, igs);
  if( DEBUG24 )
    fprintf(stderr, "] KheIgSolnSetAddSoln returning (added), %d solns\n",
      HaArrayCount(node->solns));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnSetTrieGatherAndClear(KHE_IG_SOLN_SET_TRIE trie,             */
/*    ARRAY_KHE_IG_SOLN *array_igs, KHE_IG_SOLVER igsv)                      */
/*                                                                           */
/*  Gather the solutions of trie into *array_igs, and clear trie.            */
/*                                                                           */
/*****************************************************************************/

static void KheSolnSetTrieGatherAndClear(KHE_IG_SOLN_SET_TRIE trie,
  ARRAY_KHE_IG_SOLN *array_igs, KHE_IG_SOLVER igsv)
{
  KHE_IG_SOLN_SET_TRIE child_trie;  int i;
  if( trie != NULL )
  {
    /* gather the solutions stored in trie */
    HaArrayAppend(*array_igs, trie->solns, i);

    /* gather and clear the children */
    HaArrayForEach(trie->children, child_trie, i)
      KheSolnSetTrieGatherAndClear(child_trie, array_igs, igsv);

    /* free trie */
    HaArrayAddLast(igsv->ig_soln_set_trie_free_list, trie);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnSetTrieDebug(KHE_IG_SOLN_SET_TRIE trie, int verbosity,     */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of trie with the given verbosity and indent.                 */
/*                                                                           */
/*****************************************************************************/

/* *** not ready to use
static void KheIgSolnSetTrieDebug(KHE_IG_SOLN_SET_TRIE trie, int verbosity,
  int indent, FILE *fp)
{
  KHE_IG_SOLN_SET_TRIE child_trie;  int i;
  if( trie != NULL )
  {
    if( trie->value != -1 )
      fprintf(fp, "%*s%.5f\n", indent, "", KheCostShow(trie->value));
    fprintf(fp, "%*s[\n", indent, "");
    HaArrayForEach(trie->children, child_trie, i)
      if( child_trie != NULL )
      {
	fprintf(fp, "%*s  %d:\n", indent, "", i);
        KheIgCostTrieDebug(child_trie, verbosity, indent + 2, fp);
      }
    fprintf(fp, "%*s]\n", indent, "");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_EXPANDER"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_EXPANDER KheIgExpanderMake(KHE_IG_SOLVER igsv)                    */
/*                                                                           */
/*  Make a new expander object with these attributes.                        */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_EXPANDER KheIgExpanderMake(KHE_IG_SOLVER igsv)
{
  KHE_IG_EXPANDER res;
  HaMake(res, igsv->arena);
  res->solver = igsv;
  res->prev_igs = NULL;
  HaArrayInit(res->prev_task_group_classes, igsv->arena);
  res->next_igs = KheIgSolnMake(NULL, igsv);
  res->next_igtg = NULL;
  HaArrayInit(res->all_links, igsv->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgExpanderContainsAcceptingTaskGroupClass(KHE_IG_EXPANDER ige,   */
/*    KHE_IG_TASK_GROUP igtg, KHE_IG_TASK_GROUP_CLASS *igtc)                 */
/*                                                                           */
/*  If ige contains a task group class that accepts igtg, return true        */
/*  with *igtc set to that class.  Otherwise return false.                   */
/*                                                                           */
/*****************************************************************************/

static bool KheIgExpanderContainsAcceptingTaskGroupClass(KHE_IG_EXPANDER ige,
  KHE_IG_TASK_GROUP igtg, KHE_IG_TASK_GROUP_CLASS *igtc)
{
  int i;
  HaArrayForEach(ige->prev_task_group_classes, *igtc, i)
    if( KheIgTaskGroupClassAcceptsTaskGroup(*igtc, igtg) )
      return true;
  return *igtc = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderReset(KHE_IG_EXPANDER ige, KHE_IG_SOLN prev_igs,       */
/*    KHE_IG_TIME_GROUP next_igtg)                                           */
/*                                                                           */
/*  Reset ige so that it is ready to expand prev_igs, with the resulting     */
/*  new solutions being added to time group next_igtg.                       */
/*                                                                           */
/*****************************************************************************/

static void KheIgExpanderReset(KHE_IG_EXPANDER ige, KHE_IG_SOLN prev_igs,
  KHE_IG_TIME_GROUP next_igtg)
{
  int i, j, class_seq;  KHE_IG_TASK_GROUP igtg;  KHE_IG_SOLVER igsv;
  KHE_IG_TASK_GROUP_CLASS igtgc, igtgc1;  KHE_IG_TASK_CLASS igtc;
  KHE_IG_LINK link1, link2;

  /* reset the fields of ige for the new expansion */
  igsv = ige->solver;
  ige->prev_igs = prev_igs;
  HaArrayAppend(igsv->ig_task_group_class_free_list,
    ige->prev_task_group_classes, i);
  HaArrayClear(ige->prev_task_group_classes);
  KheIgSolnReset(ige->next_igs, prev_igs, igsv);
  ige->next_igtg = next_igtg;
  HaArrayAppend(igsv->ig_link_free_list, ige->all_links, i);
  HaArrayClear(ige->all_links);

  /* reset next_igtg's task classes for the new expansion */
  HaArrayForEach(next_igtg->task_classes, igtc, i)
    KheIgTaskClassExpandReset(igtc, igsv);

  /* set ige->prev_task_group_classes to the task group classes for this exp. */
  if( prev_igs != NULL )
  {
    class_seq = 0;
    HaArrayForEach(prev_igs->task_groups, igtg, i)
    {
      igtg->expand_used = false;
      if( !KheIgExpanderContainsAcceptingTaskGroupClass(ige, igtg, &igtgc) )
      {
	igtgc = KheIgTaskGroupClassMake(class_seq++, igsv);
	HaArrayAddLast(ige->prev_task_group_classes, igtgc);
      }
      KheIgTaskGroupClassAddTaskGroup(igtgc, igtg);
    }
  }

  /* set links between non-NULL task group classes and non-NULL task classes */
  HaArrayForEach(ige->prev_task_group_classes, igtgc, i)
    HaArrayForEach(next_igtg->task_classes, igtc, j)
      if( KheIgTaskGroupClassAcceptsTaskClass(igtgc, igtc, igsv) )
	KheIgLinkMakeAndAdd(igtgc, igtc, ige);

  /* set links between the NULL task group class and non-NULL task classes */
  HaArrayForEach(next_igtg->task_classes, igtc, j)
    KheIgLinkMakeAndAdd(NULL, igtc, ige);

  /* set links between non-NULL task group classes and the NULL task class */
  HaArrayForEach(ige->prev_task_group_classes, igtgc, j)
    KheIgLinkMakeAndAdd(igtgc, NULL, ige);

  /* set blocks as required */
  HaArrayForEach(ige->all_links, link1, i)
    HaArrayForEach(ige->all_links, link2, j)
      if( KheIgLinksAreBlocked(link1, link2, igsv) )
      {
	KheIgLinkAddBlock(link1, link2);
	KheIgLinkAddBlock(link2, link1);
      }

  /* debug the blocks */
  if( DEBUG50(567) )
    HaArrayForEach(ige->prev_task_group_classes, igtgc1, i)
      KheIgTaskGroupClassDebug(igtgc1, igsv, 5, 2, stderr);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderFinishedCostAdd(KHE_IG_SOLN igs, KHE_IG_EXPANDER ige)  */
/*                                                                           */
/*  Add to igs->cost the costs of all groups that have just finished.        */
/*                                                                           */
/*  Design note.  This function could be called KheIgSolnFinishedCostAdd,    */
/*  except that it relies on prev_igtg->expand_used, tying it to expansion.  */
/*  But we don't pass an expander parameter, because this function is also   */
/*  called by KheIgSolverExpandOther, which does not use an expander.        */
/*                                                                           */
/*****************************************************************************/

static void KheIgExpanderFinishedCostAdd(KHE_IG_SOLN igs,
  KHE_IG_TIME_GROUP next_igtg, KHE_IG_SOLVER igsv)
{
  int i;  KHE_IG_TASK_GROUP prev_igtg, igtg;  KHE_COST cost;
  KHE_IG_TIME_GROUP igtg_prev;

  /* add costs of groups that must end here i.e. self-finished groups */
  HaArrayForEachReverse(igs->task_groups, igtg, i)
  {
    if( DEBUG32(igtg) )
      fprintf(stderr, "  CostAdd at %s (primary_durn %d, self_finished %s)\n",
	KheIgTaskGroupId(igtg), igtg->primary_durn,
	bool_show(KheIgTaskGroupIsSelfFinished(igtg)));
    if( !KheIgTaskGroupIsSelfFinished(igtg) )
      break;
    cost = KheIgTaskGroupCost(igtg, next_igtg, igsv);
    if( DEBUG49 && cost > 0 )
    {
      fprintf(stderr, "  this self-finished igtg has cost %.5f:\n",
	KheCostShow(cost));
      KheIgTaskGroupDebug(igtg, 2, 2, stderr);
    }
    igs->cost += cost;
    /* ***
    if( KheIgTaskGroupHasVaryingDomains(igtg) )
      igs->varying_domain_groups++;
    *** */
  }

  /* add costs of groups that ended previously i.e. (a) groups */
  if( igs->prev_igs != NULL )
  {
    if( next_igtg->index == 0 )
      igtg_prev = NULL;
    else
      igtg_prev = HaArray(igsv->time_groups, next_igtg->index - 1);
    HaArrayForEach(igs->prev_igs->task_groups, prev_igtg, i)
      if( !prev_igtg->expand_used && !KheIgTaskGroupIsSelfFinished(prev_igtg) )
      {
	cost = KheIgTaskGroupCost(prev_igtg, igtg_prev, igsv);
	if( DEBUG49 && cost > 0 )
	{
	  fprintf(stderr, "  this non-self-finished igtg has cost %.5f:\n",
	    KheCostShow(cost));
	  KheIgTaskGroupDebug(prev_igtg, 2, 2, stderr);
	}
	igs->cost += cost;
	/* ***
	if( KheIgTaskGroupHasVaryingDomains(prev_igtg) )
	  igs->varying_domain_groups++;
	*** */
      }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderHandleOverhangCases(KHE_IG_EXPANDER ige)               */
/*                                                                           */
/*  Add to ige->next_igs the overhang tasks from ige->prev_igs (those        */
/*  whose overhang is non-zero).                                             */
/*                                                                           */
/*****************************************************************************/

static void KheIgExpanderHandleOverhangCases(KHE_IG_EXPANDER ige)
{
  KHE_IG_TASK_GROUP igtg, new_igtg;  int i, j;  KHE_IG_TASK_GROUP_CLASS igtgc;
  /* if( ige->prev_igs != NULL ) not needed when iterating over classes */
  HaArrayForEach(ige->prev_task_group_classes, igtgc, i)
    if( KheIgTaskGroupClassHasNonZeroOverhang(igtgc) )
      HaArrayForEach(igtgc->task_groups, igtg, j)
      {
	new_igtg = KheIgTaskGroupMakeDummy(igtg, ige->solver);
	HaArrayAddLast(ige->next_igs->task_groups, new_igtg);
	igtg->expand_used = true;
	igtgc->expand_used_count++;
      }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgExpanderHasAssignedTaskGroup(KHE_IG_EXPANDER ige,              */
/*    KHE_RESOURCE r, KHE_IG_TASK_GROUP_CLASS *igtgc,KHE_IG_TASK_GROUP *igtg)*/
/*                                                                           */
/*  If ige's previous solution contains a task group assigned r, set         */
/*  *igtgc to that task group's class and *igtg to the task group and        */
/*  return true.  Otherwise return false.                                    */
/*                                                                           */
/*  Implementation note.  All task groups in a given class have the same     */
/*  assigned resource, which means that we only need to check the first      */
/*  task group in each class.                                                */
/*                                                                           */
/*****************************************************************************/

static bool KheIgExpanderHasAssignedTaskGroup(KHE_IG_EXPANDER ige,
  KHE_RESOURCE r, KHE_IG_TASK_GROUP_CLASS *igtgc, KHE_IG_TASK_GROUP *igtg)
{
  int i;
  HaArrayForEach(ige->prev_task_group_classes, *igtgc, i)
  {
    *igtg = HaArrayFirst((*igtgc)->task_groups);
    if( (*igtg)->assigned_resource == r )
      return true;
  }
  return *igtgc = NULL, *igtg = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIgExpanderHandleSameResourceAsstCases(KHE_IG_EXPANDER ige)        */
/*                                                                           */
/*  Add to ige's solution the assignments required by cases where a task     */
/*  group and a task have the same non-NULL resource assignments.            */
/*                                                                           */
/*  Implementation note.  All tasks in a given class have the same           */
/*  assigned resource, which means that we only need to check the first      */
/*  task in each class.                                                      */
/*                                                                           */
/*****************************************************************************/

static void KheIgExpanderHandleSameResourceAsstCases(KHE_IG_EXPANDER ige)
{
  KHE_IG_TASK igt;  KHE_RESOURCE r;  KHE_IG_TASK_GROUP igtg, next_igtg;
  int i;  KHE_IG_SOLVER igsv;  struct khe_task_grouper_entry_rec new_entry;
  KHE_IG_TASK_CLASS igtc;  KHE_IG_TASK_GROUP_CLASS igtgc;
  igsv = ige->solver;
  /* if( ige->prev_igs != NULL ) don't need this since we search classes */
  HaArrayForEach(ige->next_igtg->task_classes, igtc, i)
  {
    igt = HaArrayFirst(igtc->tasks);
    r = KheTaskAsstResource(igt->task);
    if( r != NULL && KheIgExpanderHasAssignedTaskGroup(ige, r, &igtgc, &igtg) )
    {
      KheTaskGrouperEntryAddTask((KHE_TASK_GROUPER_ENTRY) igtg, igt->task,
	igsv->days_frame, igsv->domain_finder, &new_entry);
      next_igtg = KheIgTaskGroupMakeSuccessor(igtg, igt, &new_entry, igsv);
      HaArrayAddLast(ige->next_igs->task_groups, next_igtg);
      igtg->expand_used = true;
      igtgc->expand_used_count++;
      igt->expand_used = true;
      igtc->expand_used_count++;
#if DEBUG_EXPAND_PREV
      igt->expand_prev = igtg;
#endif
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderMakeOneAssignment(KHE_IG_EXPANDER ige,                 */
/*    KHE_IG_TASK_GROUP_CLASS igtgc, KHE_IG_TASK_CLASS igtc)                 */
/*                                                                           */
/*  As part of the expansion undertaken by ige, add a new task group         */
/*  which assigns one task from igtc to one task group from igtgc.  Or       */
/*  igtgc can be NULL, in which case the task starts a new task group.       */
/*                                                                           */
/*****************************************************************************/

static void KheIgExpanderMakeOneAssignment(KHE_IG_EXPANDER ige,
  KHE_IG_TASK_GROUP_CLASS igtgc, KHE_IG_TASK_CLASS igtc)
{
  struct khe_task_grouper_entry_rec new_entry;  KHE_IG_TASK_GROUP igtg;
  KHE_IG_TASK igt;  KHE_IG_SOLVER igsv;  KHE_IG_TASK_GROUP next_igtg;

  /* update igtgc and its task group */
  if( igtgc != NULL )
  {
    HnAssert(0 <= igtgc->expand_used_count &&
      igtgc->expand_used_count < HaArrayCount(igtgc->task_groups),
      "KheIgExpanderMakeOneAssignment internal error 1");
    igtg = HaArray(igtgc->task_groups, igtgc->expand_used_count);
    igtg->expand_used = true;
    igtgc->expand_used_count++;
  }
  else
    igtg = NULL;

  /* update igtc and its task */
  HnAssert(0 <= igtc->expand_used_count &&
    igtc->expand_used_count < HaArrayCount(igtc->tasks),
    "KheIgExpanderMakeOneAssignment internal error 2");
  igt = HaArray(igtc->tasks, igtc->expand_used_count);
  igt->expand_used = true;
#if DEBUG_EXPAND_PREV
  igt->expand_prev = igtg;
#endif
  igtc->expand_used_count++;

  /* update ige->next_igs */
  igsv = ige->solver;
  if( igtgc != NULL )
  {
    if( !KheTaskGrouperEntryAddTaskUnchecked((KHE_TASK_GROUPER_ENTRY) igtg,
	  igt->task, igsv->days_frame, igsv->domain_finder, &new_entry) )
    {
      if( DEBUG51 )
      {
	fprintf(stderr, "  KheIgExpanderMakeOneAssignment failing on link:\n");
	KheIgTaskGroupDebug(igtg, 2, 2, stderr);
	KheIgTaskDebug(igt, 2, 2, stderr);
      }
      HnAbort("KheIgExpanderMakeOneAssignment internal error 3");
    }
    next_igtg = KheIgTaskGroupMakeSuccessor(igtg, igt, &new_entry, igsv);
  }
  else
    next_igtg = igt->initial_task_group;
    /* next_igtg = KheIgTaskGroupMakeInitial(igt, igsv); */
  HaArrayAddLast(ige->next_igs->task_groups, next_igtg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderDeleteOneAssignment(KHE_IG_EXPANDER ige,               */
/*    KHE_IG_TASK_GROUP_CLASS igtgc, KHE_IG_TASK_CLASS igtc)                 */
/*                                                                           */
/*  Undo the most recent non-undone call to KheIgExpanderMakeOneAssignment.  */
/*                                                                           */
/*****************************************************************************/

static void KheIgExpanderDeleteOneAssignment(KHE_IG_EXPANDER ige,
  KHE_IG_TASK_GROUP_CLASS igtgc, KHE_IG_TASK_CLASS igtc)
{
  KHE_IG_TASK_GROUP igtg;  KHE_IG_TASK igt;  KHE_IG_SOLVER igsv;
  KHE_IG_TASK_GROUP next_igtg;

  /* update ige->next_igs */
  igsv = ige->solver;
  next_igtg = HaArrayLastAndDelete(ige->next_igs->task_groups);
  if( next_igtg->prev != NULL )
    KheIgTaskGroupFree(next_igtg, igsv);

  /* update igtc and its task */
  igtc->expand_used_count--;
  igt = HaArray(igtc->tasks, igtc->expand_used_count);
#if DEBUG_EXPAND_PREV
  igt->expand_prev = NULL;
#endif
  igt->expand_used = false;

  /* update igtgc and its task group */
  if( igtgc != NULL )
  {
    igtgc->expand_used_count--;
    igtg = HaArray(igtgc->task_groups, igtgc->expand_used_count);
    igtg->expand_used = false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderCopyAndSaveSoln(KHE_IG_EXPANDER ige)                   */
/*                                                                           */
/*  Make a copy of ige's current solution and save it in ige->next_igtg,     */
/*  with dominance testing.                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheIgExpanderCopyAndSaveSoln(KHE_IG_EXPANDER ige)
{
  KHE_IG_SOLVER igsv;  KHE_IG_SOLN igs;
  igsv = ige->solver;
  igs = KheIgSolnCopy(ige->next_igs, igsv);
  HaArraySort(igs->task_groups, &KheIgTaskGroupCmp);
  KheIgExpanderFinishedCostAdd(igs, ige->next_igtg, igsv);
#if DEBUG_COMPATIBLE
  if( ige->next_igtg->other_igs != NULL )
  {
    HnAssert(ige->next_igtg->other_igs->cc_index == ige->solver->cc_index,
      "KheIgExpanderCopyAndSaveSoln internal error in other_igs (%d != %d)\n",
      ige->next_igtg->other_igs->cc_index, ige->solver->cc_index);
    HnAssert(ige->next_igtg->other_igs->cc_index == ige->solver->cc_index,
      "KheIgExpanderCopyAndSaveSoln internal error in igs (%d != %d)\n",
      igs->cc_index, ige->solver->cc_index);
    igs->compatible = KheIgSolnDominates(igs, ige->next_igtg->other_igs, igsv);
    if( DEBUG43 && igs->compatible )
    {
      fprintf(stderr, "  found compatible solution for %s:\n",
	KheIgTimeGroupId(ige->next_igtg));
      KheIgSolnDebugTimetable(igs, igsv, 2, 2, stderr);
    }
  }
#endif
  KheIgTimeGroupAddSoln(ige->next_igtg, igs);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderDoAssignRemainingTasks(KHE_IG_EXPANDER ige,            */
/*    KHE_IG_TASK_CLASS igtc, int igtc_index, int igtgc_index,               */
/*    int wanted_tasks)                                                      */
/*                                                                           */
/*  Assign wanted_tasks tasks from igtc (whose index is igtc_index) to       */
/*  task group classes from igtgc_index on.                                  */
/*                                                                           */
/*  Implementation note.  Unlike KheIgExpanderDoAssignUndersizedTaskGroup,   */
/*  where there may be a non-trivial minimum number of assignments, here     */
/*  the minimum number of assignments wanted is always 0, because leftover   */
/*  tasks can always be assigned to new groups.                              */
/*                                                                           */
/*****************************************************************************/
static void KheIgExpanderAssignRemainingTasks(KHE_IG_EXPANDER ige,
  int igtc_index);

static void KheIgExpanderDoAssignRemainingTasks(KHE_IG_EXPANDER ige,
  KHE_IG_TASK_CLASS igtc, int igtc_index, int igtgc_index, int wanted_tasks)
{
  int i, max_wanted, unused_count;  KHE_IG_TASK_GROUP_CLASS igtgc;
  KHE_IG_LINK link;
  if( igtgc_index == HaArrayCount(igtc->links) - 1 )  /* i.e. link to empty */
  {
    /* all task group classes done; remaining wanted tasks make new groups */
    link = HaArray(igtc->links, igtgc_index);
    igtgc = link->task_group_class;
    HnAssert(igtgc == NULL,
      "KheIgExpanderDoAssignRemainingTasks internal error 1");
    if( KheIgLinkIsOpen(link) )
    {
      for( i = 0;  i < wanted_tasks;  i++ )
      {
	KheIgExpanderMakeOneAssignment(ige, NULL, igtc);
	KheIgLinkUseBegin(link);
      }
      KheIgExpanderAssignRemainingTasks(ige, igtc_index + 1);
      for( i = 0;  i < wanted_tasks;  i++ )
      {
	KheIgExpanderDeleteOneAssignment(ige, NULL, igtc);
	KheIgLinkUseEnd(link);
      }
    }
  }
  else
  {
    link = HaArray(igtc->links, igtgc_index);
    if( KheIgLinkIsOpen(link) )
    {
      igtgc = link->task_group_class;
      HnAssert(igtgc != NULL,
	"KheIgExpanderDoAssignRemainingTasks internal error 2");
      unused_count = KheIgTaskGroupClassExpandUnusedCount(igtgc);
      max_wanted = min(wanted_tasks, unused_count);
      KheIgExpanderDoAssignRemainingTasks(ige, igtc, igtc_index,
	igtgc_index + 1, wanted_tasks);
      for( i = 0;  i < max_wanted;  i++ )
      {
	KheIgExpanderMakeOneAssignment(ige, igtgc, igtc);
	KheIgLinkUseBegin(link);
	KheIgExpanderDoAssignRemainingTasks(ige, igtc, igtc_index,
	  igtgc_index + 1, wanted_tasks - (i + 1));
      }
      for( i = 0;  i < max_wanted;  i++ )
      {
	KheIgExpanderDeleteOneAssignment(ige, igtgc, igtc);
	KheIgLinkUseEnd(link);
      }
    }
    else
      KheIgExpanderDoAssignRemainingTasks(ige, igtc, igtc_index,
	igtgc_index + 1, wanted_tasks);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderAssignRemainingTasks(KHE_IG_EXPANDER ige,              */
/*    int igtc_index)                                                        */
/*                                                                           */
/*  Assign the remaining unused tasks of the current expansion, starting     */
/*  with the unused tasks in the task class with index igtc_index.           */
/*                                                                           */
/*****************************************************************************/

static void KheIgExpanderAssignRemainingTasks(KHE_IG_EXPANDER ige,
  int igtc_index)
{
  KHE_IG_TASK_CLASS igtc;  int wanted_tasks, i;  bool have_link;
  KHE_IG_TASK_GROUP_CLASS igtgc;  KHE_IG_LINK link;
  if( DEBUG57(ige) && igtc_index == 0 )
  {
    int j;  KHE_IG_TASK_GROUP igtg;
    fprintf(stderr, "  undersized and unused before %s:\n",
      KheIgTimeGroupId(ige->next_igtg));
    HaArrayForEach(ige->prev_task_group_classes, igtgc, i)
      HaArrayForEach(igtgc->task_groups, igtg, j)
        if( KheIgTaskGroupUndersized(igtg, ige->solver) && !igtg->expand_used )
	  KheIgTaskGroupDebug(igtg, 2, 4, stderr);
  }
  if( igtc_index >= HaArrayCount(ige->next_igtg->task_classes) )
  {
    /* make sure no unused task groups are blocked */
    HaArrayForEach(ige->prev_task_group_classes, igtgc, i)
      if( KheIgTaskGroupClassExpandUnusedCount(igtgc) > 0 )
      {
	have_link = KheIgLinkRetrieve(igtgc, NULL, &link);
	HnAssert(have_link, "KheIgExpanderAssignRemainingTasks internal error");
	if( !KheIgLinkIsOpen(link) )
	  return;
      }

    /* base of recursion; copy soln and save (with dominance testing) */
    KheIgExpanderCopyAndSaveSoln(ige);
  }
  else
  {
    /* handle igtc, the task class at index igtc_index */
    igtc = HaArray(ige->next_igtg->task_classes, igtc_index);
    wanted_tasks = KheIgTaskClassExpandUnusedCount(igtc);
    if( wanted_tasks == 0 )
      KheIgExpanderAssignRemainingTasks(ige, igtc_index + 1);
    else
      KheIgExpanderDoAssignRemainingTasks(ige, igtc, igtc_index, 0,
	wanted_tasks);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderDoAssignUndersizedTaskGroup(KHE_IG_EXPANDER ige,       */
/*    KHE_IG_TASK_GROUP_CLASS igtgc, int igtgc_index,                        */
/*    int igtc_index, int wanted_tasks, int avail_tasks)                     */
/*                                                                           */
/*  Assign wanted_tasks task groups from igtgc (whose index in ige is        */
/*  igtgc_index), starting from the task class in igtgc whose index          */
/*  is igtc_index.  There are avail_tasks available for use by these         */
/*  assignments in this and later task classes.                              */
/*                                                                           */
/*****************************************************************************/
static void KheIgExpanderAssignUndersizedTaskGroups(KHE_IG_EXPANDER ige,
  int igtgc_index);

static void KheIgExpanderDoAssignUndersizedTaskGroup(KHE_IG_EXPANDER ige,
  KHE_IG_TASK_GROUP_CLASS igtgc, int igtgc_index,
  int igtc_index, int wanted_tasks, int avail_tasks)
{
  KHE_IG_TASK_CLASS igtc;  KHE_IG_LINK link;
  int igtc_avail_tasks, next_avail_tasks, min_wanted, max_wanted, i;
  if( wanted_tasks <= 0 )
  {
    /* nothing more to do with igtgc, so go on to next task group class */
    KheIgExpanderAssignUndersizedTaskGroups(ige, igtgc_index + 1);
  }
  else
  {
    /* get igtc and the minimum and maximum tasks wanted for it */
    HnAssert(igtc_index < HaArrayCount(igtgc->links),
      "KheIgExpanderDoAssignUndersizedTaskGroup internal error 1");
    link = HaArray(igtgc->links, igtc_index);
    if( link->task_class != NULL && KheIgLinkIsOpen(link) )
    {
      igtc = link->task_class;
      igtc_avail_tasks = KheIgTaskClassExpandUnusedCount(igtc);
      next_avail_tasks = avail_tasks - igtc_avail_tasks;
      min_wanted = max(0, wanted_tasks - next_avail_tasks);
      max_wanted = min(wanted_tasks, igtc_avail_tasks);
      HnAssert(min_wanted <= max_wanted,
	"KheIgExpanderDoAssignUndersizedTaskGroup internal error 2");
      if( DEBUG57(ige) )
        fprintf(stderr, "%*s[ expanding undersized %d.%d (min_wanted %d,"
	  " max_wanted %d)\n", 2 * (igtgc_index + igtc_index + 1), "",
	  igtgc_index, igtc_index, min_wanted, max_wanted);

      for( i = 0;  i <= max_wanted;  i++ )
      {
	/* at this point we have made i assignments */
	if( i >= min_wanted )
	  KheIgExpanderDoAssignUndersizedTaskGroup(ige, igtgc, igtgc_index,
	    igtc_index + 1, wanted_tasks - i, next_avail_tasks);
	if( i < max_wanted )
	{
	  KheIgExpanderMakeOneAssignment(ige, igtgc, igtc);
	  KheIgLinkUseBegin(link);
	}
      }

      /* ***
      ** try the first option (always present) of assigning min_wanted **
      for( i = 0;  i < min_wanted;  i++ )
      {
	** at this point, the number of assignments made is i **
	KheIgExpanderMakeOneAssignment(ige, igtgc, igtc);
	KheIgLinkUseBegin(link);
	** at this point, the number of assignments made is i + 1 **
      }
      ** at this point, the number of assignments made is i (== min_wanted) **
      KheIgExpanderDoAssignUndersizedTaskGroup(ige, igtgc, igtgc_index,
	igtc_index + 1, wanted_tasks - min_wanted, next_avail_tasks);

      ** try the remaining options of assigning min_wanted + 1 ... max_wanted **
      for( ;  i < max_wanted;  i++ )
      {
	** at this point, the number of assignments made is i **
	KheIgExpanderMakeOneAssignment(ige, igtgc, igtc);
	KheIgLinkUseBegin(link);
	** at this point, the number of assignments made is i + 1 **
	KheIgExpanderDoAssignUndersizedTaskGroup(ige, igtgc, igtgc_index,
	  igtc_index + 1, wanted_tasks - (i + 1), next_avail_tasks);
      }
      ** at this point, the number of assignments made is i (== max_wanted) **
      *** */

      /* undo the max_wanted assignments we've just tried */
      for( i = 0;  i < max_wanted;  i++ )
      {
	KheIgExpanderDeleteOneAssignment(ige, igtgc, igtc);
	KheIgLinkUseEnd(link);
      }
      if( DEBUG57(ige) )
        fprintf(stderr, "%*s]\n", 2 * (igtgc_index + igtc_index + 1), "");
    }
    else
      KheIgExpanderDoAssignUndersizedTaskGroup(ige, igtgc, igtgc_index,
	igtc_index + 1, wanted_tasks, avail_tasks);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderAssignUndersizedTaskGroups(KHE_IG_EXPANDER ige,        */
/*    int task_group_index)                                                  */
/*                                                                           */
/*  Assign undersized task groups, then assign unused tasks.  Each unused    */
/*  task group in each undersized task group class is assigned a task by     */
/*  this function (unless there are too few tasks), then each unused task    */
/*  in each task class is assigned by KheIgExpanderAssignRemainingTasks.     */
/*                                                                           */
/*  Implementation note.  This function assumes that the task group          */
/*  classes of ige are sorted so that undersized classes come first.         */
/*  So when it reaches the first non-undersized class (or goes off           */
/*  the end), it calls KheIgExpanderAssignRemainingTasks to assign the       */
/*  remaining unused tasks.                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheIgExpanderAssignUndersizedTaskGroups(KHE_IG_EXPANDER ige,
  int igtgc_index)
{
  KHE_IG_TASK_GROUP_CLASS igtgc;  int wanted_tasks, avail_tasks;
  if( igtgc_index >= HaArrayCount(ige->prev_task_group_classes) )
  {
    /* no more task classes, switch to assigning unused tasks */
    KheIgExpanderAssignRemainingTasks(ige, 0);
  }
  else
  {
    /* handle igtgc, the task group class at index igtgc_index */
    igtgc = HaArray(ige->prev_task_group_classes, igtgc_index);
    if( !KheIgTaskGroupClassUndersized(igtgc, ige->solver) )
    {
      /* no more undersized task classes, switch to assigning unused tasks */
      KheIgExpanderAssignRemainingTasks(ige, 0);
    }
    else
    {
      /* find the number of tasks wanted and available; make them consistent */
      wanted_tasks = KheIgTaskGroupClassExpandUnusedCount(igtgc);
      avail_tasks = KheIgTaskGroupClassExpandUnusedTasksCount(igtgc);
      if( DEBUG57(ige) )
	fprintf(stderr, "%*s[ expanding undersized %d (wanted %d, avail %d)\n",
	  2 * igtgc_index, "", igtgc_index, wanted_tasks, avail_tasks);
      if( wanted_tasks > avail_tasks )
	wanted_tasks = avail_tasks;

      /* assign the task group class, starting by using the first task class */
      KheIgExpanderDoAssignUndersizedTaskGroup(ige, igtgc, igtgc_index,
	0, wanted_tasks, avail_tasks);
      if( DEBUG57(ige) )
	fprintf(stderr, "%*s]\n", 2 * igtgc_index, "");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderDebugTaskGroupsAndTasks(KHE_IG_EXPANDER ige,           */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug the task groups classes and task classes being solved by ige.      */
/*                                                                           */
/*****************************************************************************/

static void KheIgExpanderDebugTaskGroupsAndTasks(KHE_IG_EXPANDER ige,
  int verbosity, int indent, FILE *fp)
{
  KHE_IG_TASK_GROUP_CLASS igtgc;  KHE_IG_TASK_CLASS igtc;  int i;
  HaArrayForEach(ige->prev_task_group_classes, igtgc, i)
    KheIgTaskGroupClassDebug(igtgc, ige->solver, verbosity, indent, fp);
  HaArrayForEach(ige->next_igtg->task_classes, igtc, i)
    KheIgTaskClassDebug(igtc, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderExpand(KHE_IG_EXPANDER ige, KHE_IG_SOLN prev_igs,      */
/*    KHE_IG_TIME_GROUP next_igtg, int debug_index1, int debug_index2)       */
/*                                                                           */
/*  Using ige, extend prev_igs in all possible ways into next_igtg.          */
/*                                                                           */
/*****************************************************************************/

static void KheIgExpanderExpand(KHE_IG_EXPANDER ige, KHE_IG_SOLN prev_igs,
  KHE_IG_TIME_GROUP next_igtg, int debug_index1, int debug_index2)
{
  /* reset ige for expanding prev_igs into next_igtg */
  KheIgExpanderReset(ige, prev_igs, next_igtg);
  if( DEBUG21 || DEBUG41(next_igtg, debug_index1) || DEBUG56(next_igtg) )
    fprintf(stderr, "[ KheIgExpanderExpand(ige, prev_igs, %s, %d of %d)\n",
      KheIgTimeGroupId(next_igtg), debug_index1, debug_index2);

  /* handle overhang and continuing asst cases */
  KheIgExpanderHandleOverhangCases(ige);
  KheIgExpanderHandleSameResourceAsstCases(ige);

  /* make undersized task group classes come first */
  HaArraySort(ige->prev_task_group_classes, &KheIgTaskGroupClassCmp);
  if( DEBUG21 || DEBUG41(next_igtg, debug_index1) || DEBUG56(next_igtg) )
  {
    fprintf(stderr, "  task group classes and task classes near start:\n");
    KheIgExpanderDebugTaskGroupsAndTasks(ige, 5, 2, stderr);
  }

  /* make already used task groups come last in each task group */
  /* *** overhang and continuing are in singleton classes, so nothing to do here
  HaArrayForEach(ige->prev_task_group_classes, igtgc, i)
    HaArraySort(igtgc->task_groups, &KheIgTaskGroupExpandUsedCmp);
  *** */

  /* assign undersized task groups, then remaining unused tasks */
  KheIgExpanderAssignUndersizedTaskGroups(ige, 0);
  if( DEBUG21 || DEBUG41(next_igtg, debug_index1) || DEBUG56(next_igtg) )
    fprintf(stderr, "] KheIgExpanderExpand returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_EXPANDER - obsolete code"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheExpanderDebugFn(void *value, int verbosity,                      */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of one expander.                                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheExpanderDebugFn(void *value, int verbosity,
  int indent, FILE *fp)
{
  KHE_IG_EXPANDER ige;
  ige = (KHE_IG_EXPANDER) value;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "Expander(%s)", KheIgTimeGroupId(ige->next_igtg));
  if( indent >= 0 )
    fprintf(fp, "\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskGroupDebugFn(void *value, int verbosity,                   */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of one demand node (an ig task group).                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheIgTaskGroupDebugFn(void *value, int verbosity,
  int indent, FILE *fp)
{
  KHE_IG_TASK_GROUP igtg;
  igtg = (KHE_IG_TASK_GROUP) value;
  KheIgTaskGroupDebug(igtg, verbosity, indent, fp);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskDebugFn(void *value, int verbosity, int indent, FILE *fp)  */
/*                                                                           */
/*  Debug print of one supply node (an ig task).                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheIgTaskDebugFn(void *value, int verbosity, int indent, FILE *fp)
{
  KHE_IG_TASK igt;
  igt = (KHE_IG_TASK) value;
  KheIgTaskDebug(igt, verbosity, indent, fp);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIgCostDebugFn(int c1, int c2, int c3, FILE *fp)                  */
/*                                                                           */
/*  Debug print of one cost in the matching.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheIgCostDebugFn(int c1, int c2, int c3, FILE *fp)
{
  fprintf(fp, "%d", c1);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTryIgTaskGroup(KHE_IG_TASK_GROUP next_igtg, int mtask_index,     */
/*    int task_index, KHE_TASK task, int skipped_undersized_count, int i,    */
/*    int depth, KHE_IG_EXPANDER ige)                                        */
/*                                                                           */
/*  Try adding next_igtg to ige->next_igs and recursing.                     */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheIgExpanderHandleDefaultCases
static void KheIgExpanderTryStartingTasksB(KHE_IG_EXPANDER ige,
  int mtask_index, int task_index, int next_i,
  int skipped_undersized_count, int depth);

static void KheTryIgTaskGroup(KHE_IG_TASK_GROUP next_igtg, int mtask_index,
  int task_index, KHE_TASK task, int skipped_undersized_count, int i,
  int depth, KHE_IG_EXPANDER ige)
{
  int next_i;
  HaArrayAddLast(ige->next_igs->task_groups, next_igtg);
  next_i = (KheTaskAsstResource(task) != NULL ? 0 : i + 1);
  KheIgExpanderTryStartingTasksB(ige, mtask_index,
    task_index + 1, next_i, skipped_undersized_count, depth + 1);
  HaArrayDeleteLast(ige->next_igs->task_groups);
  KheIgTaskGroupFree(next_igtg, ige->solver);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderTryStartingTasksB(KHE_IG_EXPANDER ige,                 */
/*    int mtask_index, int task_index, int prev_i)                           */
/*                                                                           */
/*  Try all ways to add one starting task to ige->next_igs and recurse.      */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheIgExpanderHandleDefaultCases
static void KheIgExpanderTryStartingTasksA(KHE_IG_EXPANDER ige,
  int mtask_index, int depth);

static void KheIgExpanderTryStartingTasksB(KHE_IG_EXPANDER ige,
  int mtask_index, int task_index, int next_i,
  int skipped_undersized_count, int depth)
{
  KHE_MTASK mt;  KHE_COST non_asst_cost, asst_cost;
  KHE_IG_TASK_GROUP prev_igtg, next_igtg;  KHE_IG_SOLVER igsv;
  KHE_TASK task;  int i, primary_durn;  bool optional, task_continues_asst;
  struct khe_task_grouper_entry_rec new_entry;  KHE_IG_MTASK igmt;

  if( DEBUG22 )
    fprintf(stderr, "%*s[ KheIgExpanderTryStartingTasksB()\n", depth * 2, "");
  igmt = HaArray(ige->next_igtg->starting_ mtasks, mtask_index);
  mt = igmt->mtask;
  if( task_index >= HaArrayCount(igmt->included_tasks) )
  {
    ** finished with this mtask, go on to the next mtask **
    KheIgExpanderTryStartingTasksA(ige, mtask_index + 1, depth + 1);
  }
  else
  {
    ** get the task_index'th task from mt **
    igsv = ige->solver;
    task = KheMTaskTask(mt, task_index, &non_asst_cost, &asst_cost);
    optional = (igmt->primary_durn == 0 || non_asst_cost <= asst_cost);
    task_continues_asst = KheIgSolnTaskContinuesAsst(ige->prev_igs, task);
    i = next_i;

    if( igmt->placement != KHE_PLACEMENT_FIRST_ONLY && ige->prev_igs != NULL )
    {
      ** try grouping task with extendable task groups from ige->prev_igs **
      for( ;  i < HaArrayCount(ige->task_group_used);  i++ )
      {
	if( !HaArray(ige->task_group_used, i) )
	{
	  prev_igtg = HaArray(ige->prev_igs->task_groups, i);
	  ** ***
	  if( KheIgTaskGroupExtendable(prev_igtg, task, igmt, optional,
		task_continues_asst, igmt->last_task_in_group,igsv,&next_igtg) )
	  *** **
	  if( KheIgTaskGroupIsExtendable(prev_igtg, task, igmt,
		task_continues_asst, igsv, &new_entry) )
	  {
	    if( DEBUG30(ige, task) || DEBUG31(ige, prev_igtg) )
	      fprintf(stderr, "%*s[ (%s, %s)\n", depth * 2, "",
                KheIgTaskGroupId(prev_igtg), KheTaskId(task));
	    primary_durn = prev_igtg->primary_durn + igmt->primary_durn;
            next_igtg = KheIgTaskGr oupMake(&new_entry, optional,
	      igmt->last_task_in_group || primary_durn >= igsv->max_limit,
	      primary_durn, igmt->full_durn - 1, igsv);
	    HaArrayPut(ige->task_group_used, i, true);
            KheTryIgTaskGroup(next_igtg, mtask_index, task_index, task,
	      skipped_undersized_count, i, depth, ige);
	    HaArrayPut(ige->task_group_used, i, false);
	    if( KheIgTask GroupUndersized(prev_igtg, igsv) )
	      skipped_undersized_count++;
	    if( DEBUG30(ige, task) || DEBUG31(ige, prev_igtg) )
	      fprintf(stderr, "%*s]\n", depth * 2, "");
	  }
	}
      }
    }

    ** try a fresh start for task, if permissible **
    if( !task_continues_asst && skipped_undersized_count == 0 )
    {
      KheTaskGrouperEntryAddTask(NULL, task, igsv->days_frame,
	igsv->domain_finder, &new_entry);
      next_igtg = KheIgTaskG roupMake(&new_entry, optional,
	igmt->last_task_in_group || igmt->primary_durn >= igsv->max_limit,
	igmt->primary_durn, igmt->full_durn - 1, igsv);
      KheTryIgTaskGroup(next_igtg, mtask_index, task_index, task,
	skipped_undersized_count, i, depth, ige);
    }
  }
  if( DEBUG22 )
    fprintf(stderr, "%*s] KheIgExpanderTryStartingTasksB\n", depth * 2, "");
}
****/


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderTryStartingTasksA(KHE_IG_EXPANDER ige,                 */
/*    int mtask_index)                                                       */
/*                                                                           */
/*  Try all ways to add to ige->next_igs the tasks of one starting mtask,    */
/*  and recurse.                                                             */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheIgExpanderHandleDefaultCases
static void KheIgExpanderTryStartingTasksA(KHE_IG_EXPANDER ige,
    int mtask_index, int depth)
{
  KHE_IG_SOLN igs;
  if( DEBUG7 )
    fprintf(stderr, "%*s[ TryStartingTasksA(ige, %d)%s\n", depth * 2, "",
      mtask_index,
      mtask_index >= HaArrayCount(ige->next_igtg->st arting_mtasks) ?
      " base" : "");
  if( mtask_index >= HaArrayCount(ige->next_igtg->st arting_mtasks) )
  {
    ** base of recursion; copy ige->next_igs, sort its tasks, find its cost **
    igs = KheIgSolnCopy(ige->next_igs, ige->solver);
    HaArraySort(igs->task_groups, &KheIgTaskGroupCmp);
    KheIgExpanderFinishedCostAdd(igs, ige);
    KheIgTimeGroupAddSoln(ige->next_igtg, igs);
  }
  else
  {
    ** explore assignments to the first task of mt **
    KheIgExpanderTryStartingTasksB(ige, mtask_index, 0, 0, 0, depth + 1);
  }
  if( DEBUG7 )
    fprintf(stderr, "%*s] TryStartingTasksA returning\n", depth * 2, "");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheDoIgTaskGroup(KHE_IG_TASK_GROUP next_igtg,                       */
/*    int task_index, KHE_IG_TASK igt, int skipped_undersized_count, int i,  */
/*    KHE_IG_EXPANDER ige)                                                   */
/*                                                                           */
/*  Helper function for KheIgExpanderHandleDefaultCases.  It adds next_igtg  */
/*  to the current solution, recurses, then removes next_igtg and frees it.  */
/*                                                                           */
/*****************************************************************************/
/* ***
static void KheIgExpanderHandleDefaultCases(KHE_IG_EXPANDER ige, int task_index,
  int next_i ** , int skipped_undersized_count **);

static void KheDoIgTaskGroup(KHE_IG_TASK_GROUP next_igtg,
  int task_index, KHE_IG_TASK igt, ** int skipped_undersized_count, **
  int i, KHE_IG_EXPANDER ige)
{
  int igtg_index;
  igt->expand_used = true;
  HaArrayAddLast(ige->next_igs->task_groups, next_igtg);
  igtg_index = igt->equivalent_to_prev ? i + 1 : 0;
  KheIgExpanderHandleDefaultCases(ige, task_index + 1, igtg_index
    ** , skipped_undersized_count **);
  HaArrayDeleteLast(ige->next_igs->task_groups);
  KheIgTaskGroupFree(next_igtg, ige->solver);
  igt->expand_used = false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderHandleDefaultCases(KHE_IG_EXPANDER ige,                */
/*    int task_index, int igtg_index, int skipped_undersized_count)          */
/*                                                                           */
/*  Carry out one expansion, from the task at task_index onwards.            */
/*                                                                           */
/*  If task_index >= HaArrayCount(ige->next_igtg->starting_tasks), we are    */
/*  off the end and it is time to evaluate the solution we have built.       */
/*  Otherwise, we need to decide what to do with the task at task_index.     */
/*  We can add it to an existing group (any group from the one at index      */
/*  igtg_index onwards), or we can use it to start a new group.  If          */
/*  skipped_undersized_count > 0 this last option is not open.               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheIgExpanderHandleDefaultCases(KHE_IG_EXPANDER ige,
  int task_index, int igtg_index ** , int skipped_undersized_count **)
{
  KHE_IG_SOLN igs;  KHE_IG_TASK igt;
  KHE_IG_TASK_GROUP prev_igtg, next_igtg;  KHE_IG_SOLVER igsv;
  ** int primary_durn; **  bool task_continues_asst;
  struct khe_task_grouper_entry_rec new_entry;  KHE_IG_MTASK igmt;
  if( DEBUG7 )
    fprintf(stderr, "%*s[ KheIgExpanderHandleDefaultCases(ige, %d)%s\n",
      task_index * 2, "", task_index,
      task_index >= HaArrayCount(ige->next_igtg->starting_tasks) ? " base":"");
  igsv = ige->solver;
  if( task_index >= HaArrayCount(ige->next_igtg->starting_tasks) )
  {
    ** base of recursion; copy ige->next_igs, finish it, add it to next_igtg **
    igs = KheIgSolnCopy(ige->next_igs, igsv);
    HaArraySort(igs->task_groups, &KheIgTaskGroupCmp);
    KheIgExpanderFinishedCostAdd(igs, ige);
    KheIgTimeGroupAddSoln(ige->next_igtg, igs);
  }
  else
  {
    ** explore assignments to current igt **
    igt = HaArray(ige->next_igtg->starting_tasks, task_index);
    igmt = igt->ig_mtask;
    task_continues_asst = KheIgSolnTaskContinuesAsst(ige->prev_igs, igt->task);
    HnAssert(!task_continues_asst,
      "KheIgExpanderHandleDefaultCases internal error 1");
    if( igmt->placement != KHE_PLACEMENT_FIRST_ONLY && ige->prev_igs != NULL )
    {
      ** try grouping igt with extendable task groups from ige->prev_igs **
      for(; igtg_index < HaArrayCount(ige->prev_igs->task_groups); igtg_index++)
      {
	prev_igtg = HaArray(ige->prev_igs->task_groups, igtg_index);
	if( !prev_igtg->expand_used )
	{
	  if( KheIgTaskGroupIsExtendable(prev_igtg, igt,
		** task_continues_asst, ** igsv, &new_entry) )
	  {
	    if( DEBUG30(ige, igt->task) || DEBUG31(ige, prev_igtg) )
	      fprintf(stderr, "%*s[ (%s, %s)\n", task_index * 2, "",
                KheIgTaskGroupId(prev_igtg), KheIgTaskId(igt));
	    next_igtg = KheIgTaskGroupMakeSuccessor(prev_igtg, igt,
              &new_entry, igsv);
	    ** ***
	    primary_durn = prev_igtg->primary_durn + igmt->primary_durn;
            next_igtg = KheIgTaskGr oupMake(&new_entry, igt->optional,
	      igmt->last_task_in_group || primary_durn >= igsv->max_limit,
	      primary_durn, igmt->full_durn - 1, igsv);
	    *** **
	    ** HaArrayPut(ige->task_group_used, igtg_index, true); **
	    prev_igtg->expand_used = true;
            KheDoIgTaskGroup(next_igtg, task_index, igt,
	      ** skipped_undersized_count, ** igtg_index, ige);
	    prev_igtg->expand_used = false;
	    ** ***
	    if( KheIgTaskGroupUndersized(prev_igtg, igsv) )
	      skipped_undersized_count++;
	    *** **
	    if( DEBUG30(ige, igt->task) || DEBUG31(ige, prev_igtg) )
	      fprintf(stderr, "%*s]\n", task_index * 2, "");
	  }
	}
      }
    }

    ** try a fresh start for igt, if permissible **
    ** ***
    if( ** !task_continues_asst && ** skipped_undersized_count == 0 )
    {
    *** **
    {
      next_igtg = KheIgTaskGroupMakeInitial(igt, igsv);
      ** ***
      KheTaskGrouperEntryAddTask(NULL, igt->task, igsv->days_frame,
	igsv->domain_finder, &new_entry);
      next_igtg = KheIgTaskGr oupMake(&new_entry, igt->optional,
	igmt->last_task_in_group || igmt->primary_durn >= igsv->max_limit,
	igmt->primary_durn, igmt->full_durn - 1, igsv);
      *** **
      KheDoIgTaskGroup(next_igtg, task_index, igt,
	** skipped_undersized_count, ** igtg_index, ige);
    }
  }
}
*** */

/* *** old version
static void KheIgExpanderHandleOverhangCases(KHE_IG_EXPANDER ige)
{
  KHE_IG_TASK_GROUP igtg, new_igtg;  int i;
  if( ige->prev_igs != NULL )
    HaArrayForEach(ige->prev_igs->task_groups, igtg, i)
      if( igtg->overhang > 0 )
      {
	new_igtg = KheIgTaskGroupMakeDummy(igtg, ige->solver);
	HaArrayAddLast(ige->next_igs->task_groups, new_igtg);
	igtg->expand_used = true;
      }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderDeleteOverhangTasks(KHE_IG_EXPANDER ige, int num)      */
/*                                                                           */
/*  Delete the continuing tasks of ige.  There are num of them.              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheIgExpanderDeleteOverhangTasks(KHE_IG_EXPANDER ige, int num)
{
  int i;  KHE_IG_TASK_GROUP igtg;
  for( i = 0;  i < num;  i++ )
  {
    igtg = HaArrayLastAndDelete(ige->next_igs->task_groups);
    KheIgTaskGroupFree(igtg, ige->solver);
    ** HaArrayPut(ige->task_used, i, false); not quite accurate; not needed **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgSolnHasAssignedGroup(KHE_IG_SOLN igs, KHE_RESOURCE r,          */
/*    KHE_IG_TASK_GROUP *igtg)                                               */
/*                                                                           */
/*  If igs contains a task group assigned r, set *igtg to that task group    */
/*  and return true.  Otherwise return false.                                */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheIgExpanderHasAssignedTaskGroup
static bool KheIgSolnHasAssignedGroup(KHE_IG_SOLN igs, KHE_RESOURCE r,
  KHE_IG_TASK_GROUP *igtg)
{
  int i;
  HaArrayForEach(igs->task_groups, *igtg, i)
    if( (*igtg)->assigned_resource == r )
      return true;
  return *igtg = NULL, false;
}
*** */

/* ***
static void KheIgExpanderHandleSameResourceAsstCases(KHE_IG_EXPANDER ige)
{
  KHE_IG_TASK igt;  KHE_RESOURCE r;  KHE_IG_TASK_GROUP igtg, next_igtg;
  int i;  KHE_IG_SOLVER igsv;  struct khe_task_grouper_entry_rec new_entry;
  igsv = ige->solver;
  if( ige->prev_igs != NULL )
    HaArrayForEach(ige->next_igtg->starting_tasks, igt, i)
    {
      r = KheTaskAsstResource(igt->task);
      if( r != NULL && KheIgSolnHasAssignedGroup(ige->prev_igs, r, &igtg) )
      {
	KheTaskGrouperEntryAddTask((KHE_TASK_GROUPER_ENTRY) igtg, igt->task,
	  igsv->days_frame, igsv->domain_finder, &new_entry);
	next_igtg = KheIgTaskGroupMakeSuccessor(igtg, igt, &new_entry, igsv);
	HaArrayAddLast(ige->next_igs->task_groups, next_igtg);
	igtg->expand_used = true;
	igt->expand_used = true;
	igt->expand_prev = igtg;
      }
    }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderHandleUndersizedCases(KHE_IG_EXPANDER ige)             */
/*                                                                           */
/*  Handle undersized groups by matching them with tasks.                    */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static void KheIgExpanderHandleUndersizedCases(KHE_IG_EXPANDER ige)
{
  KHE_MMATCH_NODE dn, sn;  KHE_IG_TASK_GROUP igtg, next_igtg;  KHE_IG_TASK igt;
  int i, j, rt_count, mult, c1, c2, c3;  KHE_IG_SOLVER igsv;
  struct khe_task_grouper_entry_rec new_entry;
  KHE_TASK_GROUP_DOMAIN domain;  KHE_RESOURCE_GROUP rg;
  KHE_TASK_GROUP_DOMAIN_TYPE domain_type;

  ** clear out any old matching **
  igsv = ige->solver;
  KheMMatchClear(ige->mmatch);
  HaArrayClear(ige->demand_nodes);
  HaArrayClear(ige->supply_nodes);

  ** add the demand nodes **
  HaArrayForEach(ige->prev_igs->task_groups, igtg, i)
    if( !igtg->last_task_in_group && !igtg->expand_used &&
	KheIgTaskGroupUndersized(igtg, igsv) && KheIgTaskGroupIsOrdinary(igtg) )
    {
      dn = KheMMatchDemandNodeMake(ige->mmatch, 1, (void *) igtg);
      HaArrayAddLast(ige->demand_nodes, dn);
    }

  ** add the supply nodes **
  HaArrayForEach(ige->next_igtg->starting_tasks, igt, j)
    if( !igt->expand_used && KheIgTaskIsOrdinary(igt) )
    {
      sn = KheMMatchSupplyNodeMake(ige->mmatch, 1, (void *) igt);
      HaArrayAddLast(ige->supply_nodes, sn);
    }

  ** add the edges **
  rt_count = KheResourceTypeResourceCount(igsv->resource_type);
  HaArrayForEach(ige->demand_nodes, dn, i)
  {
    igtg = (KHE_IG_TASK_GROUP) KheMMatchDemandNodeBack(dn);
    HaArrayForEach(ige->supply_nodes, sn, j)
    {
      igt = (KHE_IG_TASK) KheMMatchSupplyNodeBack(sn);
      if( KheIgTaskGroupIsExtendable(igtg, igt, igsv, &new_entry) )
      {
	domain = KheTaskGrouperEntryDomain(&new_entry);
	rg = KheTaskGroupDomainValue(domain, &domain_type);
	c1 = rt_count - KheResourceGroupResourceCount(rg);
	KheMMatchAddEdge(dn, sn, 1, c1, 0, 0);
      }
    }
  }

  ** find the matching and install the edges **
  KheMMatchSolve(ige->mmatch);
  HaArrayForEach(ige->demand_nodes, dn, i)
    if( KheMMatchResultEdge(dn, &sn, &mult, &c1, &c2, &c3) )
    {
      igtg = (KHE_IG_TASK_GROUP) KheMMatchDemandNodeBack(dn);
      igt = (KHE_IG_TASK) KheMMatchSupplyNodeBack(sn);
      if( !KheIgTaskGroupIsExtendable(igtg, igt, igsv, &new_entry) )
	HnAbort("KheIgExpanderHandleUndersizedCases internal error 1");
      next_igtg = KheIgTaskGroupMakeSuccessor(igtg, igt, &new_entry, igsv);
      HaArrayAddLast(ige->next_igs->task_groups, next_igtg);
      igtg->expand_used = true;
      igt->expand_used = true;
    }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheIgExpanderFirstUnusedTaskIndex(KHE_IG_EXPANDER ige)               */
/*                                                                           */
/*  Return the index in the list of starting tasks of ige of the first       */
/*  task whose expand_used field is false.                                   */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static int KheIgExpanderFirstUnusedTaskIndex(KHE_IG_EXPANDER ige)
{
  KHE_IG_TASK igt;  int i;
  HaArrayForEach(ige->next_igtg->starting_tasks, igt, i)
    if( !igt->expand_used )
      break;
  return i;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderHandleOtherCases(KHE_IG_EXPANDER ige, int igtc_index)  */
/*                                                                           */
/*  Handle all remaining cases, by assigning tasks in all possible ways.     */
/*  Task classes before igtc_index have already been handled.                */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheIgExpanderDebugTaskGroupsAndTasks(KHE_IG_EXPANDER ige,
  int verbosity, int indent, FILE *fp);

static void KheIgExpanderHandleOtherCases(KHE_IG_EXPANDER ige, int igtc_index)
{
  KHE_IG_TASK_GROUP_CLASS igtgc;  KHE_IG_TASK_GROUP igtg, next_igtg;
  KHE_IG_TASK_CLASS igtc;
  int i, j, unused_count, wanted, obtained, count, check_count;
  KHE_IG_TASK igt;  struct khe_task_grouper_entry_rec new_entry;
  KHE_IG_SOLVER igsv;  KHE_IG_EXPAND_INFO igei;
  igsv = ige->solver;
  ** ***
  if( debug && igtc_index == 0  )
  {
    fprintf(stderr, "  at start of KheIgExpanderHandleOtherCases:\n");
    KheIgExpanderDebugTaskGroupsAndTasks(ige, 2, 2, stderr);
  }
  *** **
  if( igtc_index >= HaArrayCount(ige->next_igtg->task_classes) )
  {
    ** base of recursion; copy soln and save (with dominance testing) **
    KheIgExpanderCopyAndSaveSoln(ige);
  }
  else
  {
    ** handle igtc, the task class at index igtc_index **
    igtc = HaArray(ige->next_igtg->task_classes, igtc_index);
    unused_count = KheIgTaskClassExpandUnusedCount(igtc);
    if( unused_count == 0 )
    {
      ** nothing to do in igtc, move on to next task class **
      KheIgExpanderHandleOtherCases(ige, igtc_index + 1);
    }
    else
    {
      ** igtc has unused tasks; grab them **
      igei = KheIgExpandInfoMake(igsv);
      HaArrayClear(igei->tasks);
      HaArrayForEach(igtc->tasks, igt, i)
	if( !igt->expand_used )
	  HaArrayAddLast(igei->tasks, igt);

      ** set up the multiset generator **
      igt = HaArrayFirst(igei->tasks);
      KheMultisetGeneratorSetupBegin(igei->multiset_generator, unused_count);
      HaArrayClear(igei->task_group_classes);
      HaArrayForEach(ige->prev_task_group_classes, igtgc, i)
      {
	count = KheIgTaskGroupClassExpandUnusedCount(igtgc);
	if( count > 0 )
	{
	  igtg = HaArrayFirst(igtgc->task_groups);
	  if( KheIgTaskGroupIsExtendable(igtg, igt, igsv, &new_entry) )
	  {
	    HaArrayAddLast(igei->task_group_classes, igtgc);
	    KheMultisetGeneratorSetupAdd(igei->multiset_generator, 0, count);
	  }
	}
      }
      KheMultisetGeneratorSetupAdd(igei->multiset_generator, 0, unused_count);
      KheMultisetGeneratorSetupEnd(igei->multiset_generator);

      ** handle igtc - try all multisets **
      while( KheMultisetGeneratorNext(igei->multiset_generator) )
      {
	** grab the chosen number of task groups from each task group class **
	HaArrayClear(igei->task_groups);
	HaArrayForEach(igei->task_group_classes, igtgc, i)
	{
	  wanted = KheMultisetGeneratorNextValue(igei->multiset_generator, i);
	  obtained = 0;
	  HaArrayForEach(igtgc->task_groups, igtg, j)
	  {
	    if( obtained >= wanted )
	      break;
	    if( !igtg->expand_used )
	    {
	      HaArrayAddLast(igei->task_groups, igtg);
	      obtained++;
	    }
	  }
	}

	** assign tasks to task groups and also to new groups **
	HaArrayForEach(igei->tasks, igt, i)
	{
	  if( i < HaArrayCount(igei->task_groups) )
	  {
	    ** assign igt to a task group **
	    igtg = HaArray(igei->task_groups, i);
	    if( !KheIgTaskGroupIsExtendable(igtg, igt, igsv, &new_entry) )
	      HnAbort("KheIgExpanderHandleOtherCases internal error 1");
	    next_igtg = KheIgTaskGroupMakeSuccessor(igtg, igt, &new_entry,igsv);
	    igtg->expand_used = true;
	    igtgc->expand_used_count++;
	    igt->expand_used = true;
	    igtc->expand_used_count++;
	    igt->expand_prev = igtg;
	  }
	  else
	  {
	    ** assign igt to a new group **
	    next_igtg = KheIgTaskGroupMakeInitial(igt, igsv);
	    igt->expand_used = true;
	    igtc->expand_used_count++;
	    igt->expand_prev = NULL;
	  }
	  HnAssert(next_igtg != NULL,
	    "KheIgExpanderHandleOtherCases internal errror 2");
	  HaArrayAddLast(ige->next_igs->task_groups, next_igtg);
	}

	** next task class **
	check_count = HaArrayCount(ige->next_igs->task_groups);
        KheIgExpanderHandleOtherCases(ige, igtc_index + 1);
	HnAssert(check_count == HaArrayCount(ige->next_igs->task_groups),
          "KheIgExpanderHandleOtherCases internal errror 3");

	** undo the groups we added just above **
	HaArrayForEachReverse(igei->tasks, igt, i)
	{
	  next_igtg = HaArrayLastAndDelete(ige->next_igs->task_groups);
	  HnAssert(next_igtg != NULL,
	    "KheIgExpanderHandleOtherCases internal errror 3");
	  KheIgTaskGroupFree(next_igtg, igsv);
	  igt->expand_used = false;
	  igtc->expand_used_count--;
	  igt->expand_prev = NULL;
	  if( i < HaArrayCount(igei->task_groups) )
	  {
	    igtg = HaArray(igei->task_groups, i);
	    igtg->expand_used = false;
	    igtgc->expand_used_count--;
	  }
	}
      }

      ** finished with igei now **
      KheIgExpandInfoFree(igei, igsv);
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderHandleUndersizedAndOtherCasesOld(KHE_IG_EXPANDER ige,  */
/*    int task_group_index)                                                  */
/*                                                                           */
/*  Handle undersized and other cases.  Each unused task group in each       */
/*  task group class is assigned a task by this function, then each unused   */
/*  task in each task class is assigned by KheIgExpanderHandleOtherCases.    */
/*                                                                           */
/*  Implementation note.  This function assumes that the task group          */
/*  classes of ige are sorted so that undersized classes come first.         */
/*  So when it reaches the first non-undersized class (or goes off           */
/*  the end), it calls KheIgExpanderHandleOtherCases to handle the           */
/*  remaining unused tasks.                                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheIgExpanderHandleUndersizedAndOtherCasesOld(KHE_IG_EXPANDER ige,
  int igtgc_index)
{
  KHE_IG_TASK_GROUP_CLASS igtgc;  KHE_IG_TASK_GROUP igtg, next_igtg;
  KHE_IG_TASK_CLASS igtc;
  int i, j, count, wanted, obtained, unused_count, num, total_count;
  KHE_IG_TASK igt;  struct khe_task_grouper_entry_rec new_entry;
  KHE_IG_SOLVER igsv;  KHE_IG_EXPAND_INFO igei;
  if( igtgc_index >= HaArrayCount(ige->prev_task_group_classes) )
  {
    ** base of this recursion, switch to assigning unused tasks **
    KheIgExpanderHandleOtherCases(ige, 0);
  }
  else
  {
    ** handle igtgc, the task group class at index igtgc_index **
    igsv = ige->solver;
    igtgc = HaArray(ige->prev_task_group_classes, igtgc_index);
    igtg = HaArrayFirst(igtgc->task_groups);
    if( !KheIgTaskGroupUndersized(igtg, igsv) )
    {
      ** no more undersized task groups, switch to assigning unused tasks **
      KheIgExpanderHandleOtherCases(ige, 0);
    }
    else
    {
      ** handle undersized igtgc, the task group class at index igtgc_index **
      unused_count = KheIgTaskGroupClassExpandUnusedCount(igtgc);
      if( unused_count == 0 )
      {
	** nothing to do in igtgc, move on to next task group class **
	KheIgExpanderHandleUndersizedAndOtherCasesOld(ige, igtgc_index + 1);
      }
      else
      {
	** igtgc has unused task groups; grab them **
	igei = KheIgExpandInfoMake(igsv);
	HaArrayClear(igei->task_groups);
	HaArrayForEach(igtgc->task_groups, igtg, i)
	  if( !igtg->expand_used )
	    HaArrayAddLast(igei->task_groups, igtg);

	** set up the multiset generator **
	KheMultisetGeneratorSetupBegin(igei->multiset_generator, unused_count);
	HaArrayClear(igei->task_classes);
	total_count = 0;
	HaArrayForEach(ige->next_igtg->task_classes, igtc, i)
	{
	  count = KheIgTaskClassExpandUnusedCount(igtc);
	  if( count > 0 )
	  {
	    igt = HaArrayFirst(igtc->tasks);
	    if( KheIgTaskGroupIsExtendable(igtg, igt, igsv, &new_entry) )
	    {
	      HaArrayAddLast(igei->task_classes, igtc);
	      KheMultisetGeneratorSetupAdd(igei->multiset_generator, 0, count);
	      total_count += count;
	    }
	  }
	}
	if( total_count < unused_count )
	    KheMultisetGeneratorSetupAdd(igei->multiset_generator, 0,
	      unused_count - total_count);
	KheMultisetGeneratorSetupEnd(igei->multiset_generator);

	** handle igtgc - try all multisets **
	while( KheMultisetGeneratorNext(igei->multiset_generator) )
	{
	  ** grab the chosen number of tasks from each task class **
	  HaArrayClear(igei->tasks);
	  HaArrayForEach(igei->task_classes, igtc, i)
	  {
	    wanted = KheMultisetGeneratorNextValue(igei->multiset_generator, i);
	    obtained = 0;
	    HaArrayForEach(igtc->tasks, igt, j)
	    {
	      if( obtained >= wanted )
		break;
	      if( !igt->expand_used )
	      {
		HaArrayAddLast(igei->tasks, igt);
		obtained++;
	      }
	    }
	  }

	  ** assign tasks to corresponding task groups **
	  num = min(HaArrayCount(igei->task_groups), HaArrayCount(igei->tasks));
	  for( i = 0;  i < num;  i++ )
	  {
	    igtg = HaArray(igei->task_groups, i);
	    igt = HaArray(igei->tasks, i);
	    if( !KheIgTaskGroupIsExtendable(igtg, igt, igsv, &new_entry) )
	     HnAbort("KheIgExpanderHandleUndersizedOtherCases internal error");
	    next_igtg = KheIgTaskGroupMakeSuccessor(igtg, igt, &new_entry,igsv);
	    HaArrayAddLast(ige->next_igs->task_groups, next_igtg);
	    igtg->expand_used = true;
	    igtgc->expand_used_count++;
	    igt->expand_used = true;
	    igtc->expand_used_count++;
	    igt->expand_prev = igtg;
	  }

	  ** next undersized task group **
	  KheIgExpanderHandleUndersizedAndOtherCasesOld(ige, igtgc_index + 1);

	  ** undo the groups we added just above **
	  for( i = num - 1;  i >= 0;  i-- )
	  {
	    igtg = HaArray(igtgc->task_groups, i);
	    igt = HaArray(igei->tasks, i);
	    igtg->expand_used = false;
	    igtgc->expand_used_count--;
	    next_igtg = HaArrayLastAndDelete(ige->next_igs->task_groups);
	    KheIgTaskGroupFree(next_igtg, igsv);
	    igt->expand_used = false;
	    igtc->expand_used_count--;
	    igt->expand_prev = NULL;
	  }
	}

	** finished with igei now **
	KheIgExpandInfoFree(igei, igsv);
      }
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheIgHandleTaskClass(KHE_IG_EXPANDER ige, int igtc_index)
{
}
*** */


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheIgTaskClassExtendsIgTaskGroup(KHE_IG_TASK_CLASS igtc,
  KHE_IG_TASK_GROUP igtg, KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK igt;  int i;  struct khe_task_grouper_entry_rec new_entry;
  HaArrayForEach(igtc->tasks, igt, i)
    if( !igt->expand_used )
      return KheIgTaskGroupIsExtendable(igtg, igt, igsv, &new_entry);
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheIgHandleUndersizedTaskGroupClass(KHE_IG_EXPANDER ige,
  int igtgc_index)
{
  KHE_IG_TASK_GROUP_CLASS igtgc;  int i;  KHE_IG_SOLVER igsv;
  KHE_IG_EXPAND_INFO igei;  KHE_IG_TASK_CLASS igtc;  KHE_IG_TASK_GROUP igtg;
  if( igtgc_index >= HaArrayCount(ige->prev_task_group_classes) )
  {
    ** there are no more undersized task group classes to handle **
    KheIgHandleTaskClass(ige, 0);
  }
  else
  {
    ** handle igtgc **
    igtgc = HaArray(ige->prev_task_group_classes, igtgc_index);
    igei = KheIgExpandInfoMake(igsv);

    ** grab the unused task groups of igtgc **
    igsv = ige->solver;
    HaArrayClear(igei->task_groups);
    HaArrayForEach(igtgc->task_groups, igtg, i)
      if( !igtg->expand_used )
	HaArrayAddLast(igei->task_groups, igtg);

    ** if there are no unused task groups, move on **
    if( HaArrayCount(igei->task_groups) == 0 )
    {
      KheIgHandleUndersizedTaskGroupClass(ige, igtgc_index + 1);
    }
    else
    {
      ** get the first unused task group and the task classes that extend it **
      igtg = HaArrayFirst(igei->task_groups);
      HaArrayClear(igei->task_classes);
      HaArrayForEach(ige->next_igtg->task_classes, igtc, i)
	if( KheIgTaskClassExtendsIgTaskGroup(igtc, igtg, igsv) )
	  HaArrayAddLast(igei->task_classes, igtc);
    }

    ** free igei **
    KheIgExpandInfoFree(igei, igsv);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_SOLVER"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_SOLVER KheIgSolverMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,       */
/*    KHE_OPTIONS options, int ig_min, int ig_range, KHE_MTASK_FINDER mtf,   */
/*    KHE_SOLN_ADJUSTER sa, HA_ARENA a)                                      */
/*                                                                           */
/*  Make a ig solver object with these attributes.                           */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_SOLVER KheIgSolverMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, int ig_min, int ig_range, KHE_MTASK_FINDER mtf,
  KHE_SOLN_ADJUSTER sa, HA_ARENA a)
{
  KHE_IG_SOLVER res;  /* KHE_BALANCE_SOLVER bs; */
  int demand, supply;  KHE_COST task_cost, resource_cost;
  HaMake(res, a);

  /* free lists */
  HaArrayInit(res->ig_mtask_free_list, a);
  HaArrayInit(res->ig_task_free_list, a);
  HaArrayInit(res->ig_task_class_free_list, a);
  HaArrayInit(res->ig_time_group_free_list, a);
  HaArrayInit(res->ig_cost_trie_free_list, a);
  HaArrayInit(res->ig_task_group_free_list, a);
  HaArrayInit(res->ig_task_group_class_free_list, a);
  HaArrayInit(res->ig_link_free_list, a);
  HaArrayInit(res->ig_soln_free_list, a);
  HaArrayInit(res->ig_soln_set_trie_free_list, a);
  /* HaArrayInit(res->ig_soln_set_free_list, a); */
  /* HaArrayInit(res->ig_expand_info_free_list, a); */
  /* HaArrayInit(res->ig_expand_node_free_list, a); */

  /* fields defined (and mostly constant) throughout the solve */
  res->arena = a;
  res->soln = soln;
  res->resource_type = rt;
  res->options = options;
  res->ig_min = ig_min;
  res->ig_range = ig_range;
  res->mtask_finder = mtf;
  res->days_frame = KheMTaskFinderDaysFrame(mtf);
  res->domain_finder = KheTaskGroupDomainFinderMake(soln, a);
  res->soln_adjuster = sa;
  /* ***
  bs = KheBalanceSolverMake(res->soln, rt, res->days_frame, a);
  res->marginal_cost = KheBalanceSolverMarginalCost(bs);
  *** */
  KheResourceDemandExceedsSupply(soln, rt, &demand, &supply, &task_cost,
    &resource_cost);
  if( DEBUG9 )
    fprintf(stderr, "  KheResourceDemandExceedsSupply(soln, %s): "
      "demand %d, supply %d, task_cost %.5f, resource_cost %.5f\n",
      KheResourceTypeId(rt), demand, supply, KheCostShow(task_cost),
      KheCostShow(resource_cost));
  res->marginal_cost = resource_cost;
  /* ***
  if( res->marginal_cost < 0 )
    res->marginal_cost = 0;
  *** */
  res->candidate_ccf = KheConstraintClassFinderMake(rt, res->days_frame, a);
  res->busy_days_ccf = KheConstraintClassFinderMake(rt, res->days_frame, a);
  res->complete_weekends_ccf = KheConstraintClassFinderMake(rt,
    res->days_frame, a);
  KheConstraintClassFinderAddCompleteWeekendsConstraints(
    res->complete_weekends_ccf, true);
  res->groups_count = 0;
  /* res->cost_trie = NULL; moved to time groups */
  /* KheTrieInit(res->cost_trie, a); */

  /* fields defined when solving one constraint class */
  res->curr_class = NULL;
  res->min_limit = -1;
  res->max_limit = -1;
  res->soln_seq_num = 0;
  /* res->mtask_seq_num = 0; */
  HaArrayInit(res->busy_days_classes, a);
  HaArrayInit(res->time_groups, a);
  HaArrayInit(res->included_tasks, a);
  /* res->cost_table = KheIntervalCostTableMake(a); */
#if DEBUG_COMPATIBLE
  res->cc_index = 0;
#endif

  /* scratch fields */
  res->expander = KheIgExpanderMake(res);
  /* HaArrayInit(res->tmp_indexes1, a); */
  /* HaArrayInit(res->tmp_indexes2, a); */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolverAddTimeGroup(KHE_IG_SOLVER igsv, KHE_TIME_GROUP tg)      */
/*                                                                           */
/*  Add tg to igsv.                                                          */
/*                                                                           */
/*****************************************************************************/

static void KheIgSolverAddTimeGroup(KHE_IG_SOLVER igsv, KHE_TIME_GROUP tg)
{
  KHE_IG_TIME_GROUP igtg;
  igtg = KheIgTimeGroupMake(tg, HaArrayCount(igsv->time_groups), igsv);
  HaArrayAddLast(igsv->time_groups, igtg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolverAddMTask(KHE_IG_SOLVER igsv, KHE_IG_MTASK igmt)          */
/*                                                                           */
/*  Add igmt to igsv.  Although its tasks must be defined at this point (so  */
/*  that the calls to KheIgTimeGroupAddRunningMTask work correctly), we do   */
/*  not add its tasks here; they are added separately.                       */
/*                                                                           */
/*****************************************************************************/

static void KheIgSolverAddMTask(KHE_IG_SOLVER igsv, KHE_IG_MTASK igmt)
{
  KHE_INTERVAL in;  KHE_IG_TIME_GROUP igtg;  int i;

  /* add igmt to igsv's time groups */
  in = KheMTaskInterval(igmt->mtask);
  igtg = HaArray(igsv->time_groups, in.first);
  KheIgTimeGroupAddStartingMTask(igtg, igmt);
  for( i = in.first;  i <= in.last;  i++ )
  {
    igtg = HaArray(igsv->time_groups, i);
    KheIgTimeGroupAddRunningMTask(igtg, igmt);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskIsCompulsory(KHE_MTASK mt, KHE_IG_SOLVER igsv,              */
/*    KHE_PLACEMENT *pl, int *primary_durn)                                  */
/*                                                                           */
/*  Return true if mt is a compulsory mtask.                                 */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskIsCompulsory(KHE_MTASK mt, KHE_IG_SOLVER igsv,
  KHE_PLACEMENT *pl, int *primary_durn)
{
  return KheMTaskIsAdmissible(mt, igsv, pl, primary_durn) && *primary_durn > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolverAddCompulsoryMTasks(KHE_IG_SOLVER igsv)                  */
/*                                                                           */
/*  Add the compulsory mtasks for solving igsv->class to igsv's time groups. */
/*                                                                           */
/*****************************************************************************/

static void KheIgSolverAddCompulsoryMTasks(KHE_IG_SOLVER igsv)
{
  int i, j, random_offset, index, count, primary_durn, included_tasks_count;
  KHE_MTASK mt;  KHE_IG_MTASK igmt;  KHE_PLACEMENT pl;  KHE_TASK task;
  KHE_COST non_asst_cost, asst_cost;
  random_offset = KheSolnDiversifier(igsv->soln) * 97 + 37;
  count = KheMTaskFinderMTaskCount(igsv->mtask_finder);
  for( i = 0;  i < count;  i++ )
  {
    index = (i + random_offset) % count;
    mt = KheMTaskFinderMTask(igsv->mtask_finder, index);
    if( DEBUG12 )
    {
      fprintf(stderr, "  KheIgSolverAddCompulsoryMTasks considering mtask:\n");
      KheMTaskDebug(mt, 2, 2, stderr);
    }
    if( KheMTaskIsCompulsory(mt, igsv, &pl, &primary_durn) )
    {
      igmt = KheIgMTaskMake(mt, primary_durn, pl, igsv);
      included_tasks_count = KheMTaskAssignedTaskCount(mt) +
	KheMTaskNeedsAssignmentTaskCount(mt);
      for( j = 0;  j < included_tasks_count;  j++ )
      {
	task = KheMTaskTask(mt, j, &non_asst_cost, &asst_cost);
	KheIgMTaskAddTask(igmt, task, false, igsv);
      }
      KheIgSolverAddMTask(igsv, igmt);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTimeGroupToFrameIndex(KHE_TIME_GROUP tg, KHE_FRAME days_frame)    */
/*                                                                           */
/*  Assuming that tg is non-empty and lies entirely within one day of        */
/*  days_frame, find the index in days_frame of that day.                    */
/*                                                                           */
/*****************************************************************************/

static int KheTimeGroupToFrameIndex(KHE_TIME_GROUP tg, KHE_FRAME days_frame)
{
  KHE_TIME t;
  HnAssert(KheTimeGroupTimeCount(tg) > 0,
    "KheTimeGroupToFrameIndex internal error");
  t = KheTimeGroupTime(tg, 0);
  return KheFrameTimeIndex(days_frame, t);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDifferByOne(int a, int b)                                        */
/*                                                                           */
/*  Return true if a and b differ by one.                                    */
/*                                                                           */
/*****************************************************************************/

static bool KheDifferByOne(int a, int b)
{
  return a + 1 == b || b + 1 == a;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskIsWeekend(KHE_MTASK mt, int weekend_day_index,              */
/*    bool ending, int igtg_offset, KHE_IG_SOLVER igsv)                      */
/*                                                                           */
/*  Return true if mt is a weekend mtask starting or ending on the day       */
/*  with index weekend_day_index.  Altogether the conditions are:            */
/*                                                                           */
/*    * mt is admissible                                                     */
/*    * mt's primary duration is 0 (so it's not compulsory)                  */
/*    * mt starts or ends on day weekend_day_index, depending on ending      */
/*    * mt's offset in its weekend day differs from the usual by one         */
/*                                                                           */
/*  The last condition is a simple way to limit the number of these extra    */
/*  mtasks included in the solve.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskIsWeekend(KHE_MTASK mt, int weekend_day_index,
  bool ending, int igtg_offset, KHE_IG_SOLVER igsv)
{
  KHE_TIME_SET ts;  int mt_offset, primary_durn;  KHE_INTERVAL in;
  KHE_PLACEMENT pl;

  if( DEBUG38 )
    fprintf(stderr, "[ KheMTaskIsWeekend(mt, %s, %s, %d, igsv)\n",
      KheTimeGroupId(KheFrameTimeGroup(igsv->days_frame, weekend_day_index)),
      bool_show(ending), igtg_offset);
  ts = KheMTaskTimeSet(mt);
  mt_offset = KheFrameTimeOffset(igsv->days_frame,
    KheTimeSetTime(ts, ending ? KheTimeSetTimeCount(ts) - 1 : 0));
  in = KheMTaskInterval(mt);
  if( DEBUG38 )
    fprintf(stderr, "] KheMTaskIsWeekend returning %s && %s && %s\n",
      bool_show(KheMTaskIsAdmissible(mt, igsv, &pl, &primary_durn) &&
      primary_durn == 0),
      bool_show((ending ? in.last : in.first) == weekend_day_index),
      bool_show(KheDifferByOne(mt_offset, igtg_offset)));
  return KheMTaskIsAdmissible(mt, igsv, &pl, &primary_durn) &&
      primary_durn == 0 && (ending ? in.last : in.first) == weekend_day_index &&
      KheDifferByOne(mt_offset, igtg_offset);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolverAddWeekendMTask(KHE_IG_SOLVER igsv, KHE_MTASK mt,        */
/*    KHE_PLACEMENT pl, int max_included_task_count)                         */
/*                                                                           */
/*  Add weekend mtask mt with these attributes to igsv.                      */
/*                                                                           */
/*****************************************************************************/

static void KheIgSolverAddWeekendMTask(KHE_IG_SOLVER igsv, KHE_MTASK mt,
  KHE_PLACEMENT pl, int max_included_task_count)
{
  int included_tasks_count, i;  KHE_IG_MTASK igmt;
  KHE_TASK task;  KHE_COST non_asst_cost, asst_cost;
  included_tasks_count = KheMTaskAssignedTaskCount(mt) +
    KheMTaskNeedsAssignmentTaskCount(mt);
  if( DEBUG36 )
  {
    fprintf(stderr, "  [ KheIgSolverAddWeekendMTask()\n");
    fprintf(stderr, "    included_task_count = min(%d, %d)\n",
      included_tasks_count, max_included_task_count);
  }
  if( included_tasks_count > max_included_task_count )
    included_tasks_count = max_included_task_count;
  igmt = KheIgMTaskMake(mt, 0, pl, igsv);
  for( i = 0;  i < included_tasks_count;  i++ )
  {
    task = KheMTaskTask(mt, i, &non_asst_cost, &asst_cost);
    KheIgMTaskAddTask(igmt, task, false, igsv);
  }
  if( DEBUG36 )
  {
    KheIgMTaskDebug(igmt, 2, 2, stderr);
    fprintf(stderr, "  ] KheIgSolverAddWeekendMTask returning\n");
  }
  KheIgSolverAddMTask(igsv, igmt /* , true */);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolverAddWeekendMTasks(KHE_IG_SOLVER igsv,                     */
/*    KHE_CONSTRAINT_CLASS cc)                                               */
/*                                                                           */
/*  Add weekend mtasks (derived from complete weekends constraints) to igsv. */
/*                                                                           */
/*****************************************************************************/

static void KheIgSolverAddWeekendMTasks(KHE_IG_SOLVER igsv)
{
  int i, j, count, cw_index0, cw_index1, delta;
  KHE_CONSTRAINT_CLASS cw_cc;  KHE_TIME_GROUP cw_tg0, cw_tg1;  KHE_POLARITY po;
  KHE_IG_TIME_GROUP igtg0, igtg1;  KHE_MTASK_SET mts;  KHE_MTASK mt;
  if( DEBUG36 )
    fprintf(stderr, "[ KheIgSolverAddWeekendMTasks(igsv, cc)\n");
  count = KheConstraintClassFinderClassCount(igsv->complete_weekends_ccf);
  for( i = 0;  i < count;  i++ )
  {
    cw_cc = KheConstraintClassFinderClass(igsv->complete_weekends_ccf, i);
    HnAssert(KheConstraintClassTimeGroupCount(cw_cc) == 2,
      "KheIgSolverAddWeekendMTasks internal error");
    cw_tg0 = KheConstraintClassTimeGroup(cw_cc, 0, &po);
    cw_index0 = KheTimeGroupToFrameIndex(cw_tg0, igsv->days_frame);
    igtg0 = HaArray(igsv->time_groups, cw_index0);
    cw_tg1 = KheConstraintClassTimeGroup(cw_cc, 1, &po);
    cw_index1 = KheTimeGroupToFrameIndex(cw_tg1, igsv->days_frame);
    igtg1 = HaArray(igsv->time_groups, cw_index1);
    if( igtg0->running_tasks < igtg1->running_tasks )
    {
      /* we want weekend mtasks ending during igtg0 */
      delta = igtg1->running_tasks - igtg0->running_tasks;
      if( DEBUG36 )
	fprintf(stderr, "  %s candidate mtasks:\n", KheIgTimeGroupId(igtg0));
      mts = KheMTaskFinderMTasksInTimeGroup(igsv->mtask_finder,
	igsv->resource_type, KheFrameTimeGroup(igsv->days_frame, cw_index0));
      for( j = 0;  j < KheMTaskSetMTaskCount(mts);  j++ )
      {
	mt = KheMTaskSetMTask(mts, j);
        if( KheMTaskIsWeekend(mt, cw_index0, true, igtg0->offset, igsv) )
	{
	  /* mt is a candidate for adding to the mtasks */
	  KheIgSolverAddWeekendMTask(igsv, mt, KHE_PLACEMENT_FIRST_ONLY, delta);
	}
      }
    }
    else if( igtg0->running_tasks > igtg1->running_tasks )
    {
      /* we want weekend mtasks starting during igtg1 */
      delta = igtg0->running_tasks - igtg1->running_tasks;
      if( DEBUG36 )
	fprintf(stderr, "  %s candidate mtasks:\n", KheIgTimeGroupId(igtg1));
      mts = KheMTaskFinderMTasksInTimeGroup(igsv->mtask_finder,
	igsv->resource_type, KheFrameTimeGroup(igsv->days_frame, cw_index1));
      for( j = 0;  j < KheMTaskSetMTaskCount(mts);  j++ )
      {
	mt = KheMTaskSetMTask(mts, j);
        if( KheMTaskIsWeekend(mt, cw_index1, false, igtg1->offset, igsv) )
	{
	  /* mt is a candidate for adding to the mtasks */
	  KheIgSolverAddWeekendMTask(igsv, mt, KHE_PLACEMENT_LAST_ONLY, delta);
	}
      }
    }
    else
    {
      /* no weekend mtasks wanted, since equal number of tasks running */
      if( DEBUG36 )
	fprintf(stderr, "  %s-%s: no candidate mtasks (equality)\n",
	  KheIgTimeGroupId(igtg0), KheIgTimeGroupId(igtg1));
    }
  }
  if( DEBUG36 )
    fprintf(stderr, "]\n");
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIgSolverRandom(KHE_IG_SOLVER igsv)                                */
/*                                                                           */
/*  Return a somewhat random number.                                         */
/*                                                                           */
/*****************************************************************************/

static int KheIgSolverRandom(KHE_IG_SOLVER igsv)
{
  int num, res;
  num = igsv->soln_seq_num++;
  res = (num * num * KheSolnDiversifier(igsv->soln)) % 59;
  return (res < 0 ? - res : res);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK KheIgSolverTaskToIgTask(KHE_IG_SOLVER igsv, KHE_TASK task)   */
/*                                                                           */
/*  Find the ig task corresponding to task.  This must exist.                */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_TASK KheIgSolverTaskToIgTask(KHE_IG_SOLVER igsv, KHE_TASK task)
{
  int index;  KHE_IG_TASK res;
  index = KheTaskSolnIndex(task);
  res = HaArray(igsv->included_tasks, index);
  HnAssert(res != NULL, "KheIgSolverTaskToIgTask internal error");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolverDebug(KHE_IG_SOLVER igsv, int verbosity, int indent,     */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of igsv onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

static void KheIgSolverDebug(KHE_IG_SOLVER igsv, int verbosity, int indent,
  FILE *fp)
{
  KHE_IG_TIME_GROUP igtg;  int i;
  fprintf(fp, "%*s[ IgSolver(%s)\n", indent, "",
    KheResourceTypeId(igsv->resource_type));
  HaArrayForEach(igsv->time_groups, igtg, i)
    KheIgTimeGroupDebugMTasks(igtg, verbosity, indent+2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "conversion of an existing soln into an ig soln"               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheFindNonPlaceholderOtherSoln(KHE_IG_SOLVER igsv,                  */
/*    KHE_SOLN *other_soln, char **id)                                       */
/*                                                                           */
/*  If there is a solution for the current instance which is not the         */
/*  current solution and is not a placeholder, set *other_soln to one        */
/*  such solution and return true, otherwise return false.                   */
/*                                                                           */
/*****************************************************************************/

#if DEBUG_COMPATIBLE
static bool KheFindNonPlaceholderOtherSoln(KHE_IG_SOLVER igsv,
  KHE_SOLN *other_soln, char **id)
{
  KHE_INSTANCE ins;  int i, j, k;  KHE_ARCHIVE archive;
  KHE_SOLN_GROUP soln_group;
  ins = KheSolnInstance(igsv->soln);
  for( i = 0;  i < KheInstanceArchiveCount(ins);  i++ )
  {
    archive = KheInstanceArchive(ins, i);
    for( j = 0;  j < KheArchiveSolnGroupCount(archive);  j++ )
    {
      soln_group = KheArchiveSolnGroup(archive, j);
      for( k = 0;  k < KheSolnGroupSolnCount(soln_group);  k++ )
      {
	*other_soln = KheSolnGroupSoln(soln_group, k);
	if( KheSolnInstance(*other_soln) == ins && *other_soln != igsv->soln &&
            KheSolnType(*other_soln) == KHE_SOLN_ORDINARY )
	  return *id = KheSolnGroupId(soln_group), true;
      }
    }
  }
  return *other_soln = NULL, *id = NULL, false;
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskIsAssignedInOtherSoln(KHE_TASK this_task,                    */
/*    KHE_SOLN other_soln, KHE_RESOURCE *r)                                  */
/*                                                                           */
/*  First find other_task, the task of other_soln corresponding to           */
/*  this_task.  Then if other_task is assigned a resource, set *r to that    */
/*  resource and return true.  Otherwise set *r to NULL and return false.    */
/*                                                                           */
/*  Design note.  This function is the only one that actually interrogates   */
/*  other_soln, yet it does not return anything from other_soln.  That's a   */
/*  good thing.  We don't want bits of other_soln contaminating our code.    */
/*                                                                           */
/*****************************************************************************/

#if DEBUG_COMPATIBLE
static bool KheTaskIsAssignedInOtherSoln(KHE_TASK this_task,
  KHE_SOLN other_soln, KHE_RESOURCE *r)
{
  KHE_TASK other_task;  KHE_EVENT_RESOURCE er;

  /* find other_task, the task corresponding to this_task in other_soln */
  er = KheTaskEventResource(this_task);
  HnAssert(KheEventResourceTaskCount(other_soln, er) == 1,
    "KheTaskIsAssignedInOtherSoln internal error");
  other_task = KheEventResourceTask(other_soln, er, 0);

  /* return other_task's assignment */
  *r = KheTaskAsstResource(other_task);
  return (*r != NULL);
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupIsAssignedResourceInOtherSoln(                        */
/*    KHE_IG_TASK_GROUP igtg, KHE_RESOURCE r, KHE_SOLN other_soln)           */
/*                                                                           */
/*  Return true if igtg is assigned r in other_soln.                         */
/*                                                                           */
/*****************************************************************************/

#if DEBUG_COMPATIBLE
static bool KheIgTaskGroupIsAssignedResourceInOtherSoln(
  KHE_IG_TASK_GROUP igtg, KHE_RESOURCE r, KHE_SOLN other_soln)
{
  KHE_RESOURCE r2;
  switch( igtg->type )
  {
    case KHE_TASK_GROUPER_ENTRY_ORDINARY:

      return KheTaskIsAssignedInOtherSoln(igtg->task, other_soln, &r2) &&
	r2 == r;

    case KHE_TASK_GROUPER_ENTRY_HISTORY:

      return igtg->assigned_resource == r;

    case KHE_TASK_GROUPER_ENTRY_DUMMY:

      return KheIgTaskGroupIsAssignedResourceInOtherSoln(
	KheIgTaskGroupPrev(igtg), r, other_soln);

    default:

      HnAbort("KheIgTaskGroupIsAssignedResourceInOtherSoln internal error");
      return false;  /* keep compiler happy */
  }
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgSolnContainsResourceAsstInOtherSoln(KHE_IG_SOLN prev_igs,      */
/*    KHE_RESOURCE r, KHE_SOLN other_soln, KHE_IG_TASK_GROUP *igtg)          */
/*                                                                           */
/*  If prev_igs contains a task group assigned r, set *igtg to that task     */
/*  group and return true.  Otherwise set *igtg to NULL and return false.    */
/*                                                                           */
/*****************************************************************************/

#if DEBUG_COMPATIBLE
static bool KheIgSolnContainsResourceAsstInOtherSoln(KHE_IG_SOLN prev_igs,
  KHE_RESOURCE r, KHE_SOLN other_soln, KHE_IG_TASK_GROUP *igtg)
{
  int i;
  HaArrayForEach(prev_igs->task_groups, *igtg, i)
    if( KheIgTaskGroupIsAssignedResourceInOtherSoln(*igtg, r, other_soln) )
      return true;
  return *igtg = NULL, false;
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_SOLN KheIgSolverExpandOther(KHE_IG_SOLVER igsv,                   */
/*    KHE_SOLN other_soln, KHE_IG_SOLN prev_igs, KHE_IG_TIME_GROUP next_igtg)*/
/*                                                                           */
/*  Expand prev_igs into next_igtg in one way, using other_soln as a guide.  */
/*                                                                           */
/*****************************************************************************/

#if DEBUG_COMPATIBLE
static KHE_IG_SOLN KheIgSolverExpandOther(KHE_IG_SOLVER igsv,
  KHE_SOLN other_soln, KHE_IG_SOLN prev_igs, KHE_IG_TIME_GROUP next_igtg)
{
  KHE_IG_SOLN res;  KHE_RESOURCE r;  KHE_IG_TASK_GROUP igtg;
  struct khe_task_grouper_entry_rec new_entry;  int i, j;
  KHE_IG_TASK_CLASS igtc;  KHE_IG_TASK igt;
  if( DEBUG45 )
  {
    fprintf(stderr, "[ KheIgSolverExpandOther(igsv, ...) prev_igs:\n");
    KheIgSolnDebugTimetable(prev_igs, igsv, 2, 2, stderr);
  }

  res = KheIgSolnMake(prev_igs, igsv);
  HaArrayForEach(next_igtg->task_classes, igtc, i)
    HaArrayForEach(igtc->tasks, igt, j)
    {
      if( KheTaskIsAssignedInOtherSoln(igt->task, other_soln, &r) &&
	  KheIgSolnContainsResourceAsstInOtherSoln(prev_igs, r, other_soln,
	    &igtg) )
      {
	/* igt continues previous group */
	/* need dummy here if task continues (still to do) */
	if( DEBUG45 )
	{
	  fprintf(stderr, "  found predecessor task group: ");
	  KheIgTaskGroupDebug(igtg, 2, -1, stderr);
	  fprintf(stderr, "\n");
	}
	if( !KheTaskGrouperEntryAddTaskUnchecked((KHE_TASK_GROUPER_ENTRY) igtg,
	    igt->task, igsv->days_frame, igsv->domain_finder, &new_entry) )
	  HnAbort("KheIgSolverExpandOther internal error");
	igtg->expand_used = true;
	igtg = KheIgTaskGroupMakeSuccessor(igtg, igt, &new_entry, igsv);
	if( DEBUG45 )
	{
	  fprintf(stderr, "  adding successor task group (igt optional %s): ",
	    bool_show(igt->optional));
	  KheIgTaskGroupDebug(igtg, 2, -1, stderr);
	  fprintf(stderr, "\n");
	}
      }
      else
      {
	/* igt starts a new group */
	/* igtg = KheIgTaskGroupMakeInitial(igt, igsv); */
	igtg = igt->initial_task_group;
	if( DEBUG45 )
	{
	  fprintf(stderr, "  adding initial task group (igt optional %s): ",
	    bool_show(igt->optional));
	  KheIgTaskGroupDebug(igtg, 2, -1, stderr);
	  fprintf(stderr, " \n");  /* the space prevents code merging */
	}
      }
      HaArrayAddLast(res->task_groups, igtg);
    }
  HaArraySort(res->task_groups, &KheIgTaskGroupCmp);
  KheIgExpanderFinishedCostAdd(res, next_igtg, igsv);
  if( DEBUG45 )
  {
    fprintf(stderr, "  KheIgSolverExpandOther returning res:\n");
    KheIgSolnDebugTimetable(res, igsv, 2, 2, stderr);
    fprintf(stderr, "]\n");
  }
  return res;
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolverBuildOtherSoln(KHE_IG_SOLVER igsv, KHE_SOLN other_soln)  */
/*                                                                           */
/*  Convert other_soln into an ig soln and store it in the other_soln        */
/*  fields of igsv's time groups.                                            */
/*                                                                           */
/*****************************************************************************/

#if DEBUG_COMPATIBLE
static KHE_IG_SOLN KheIgSolverBuildOtherSoln(KHE_IG_SOLVER igsv,
  KHE_SOLN other_soln, char *id)
{
  KHE_IG_SOLN prev_igs, next_igs;  KHE_IG_TIME_GROUP next_igtg;  int i;
  KHE_INSTANCE ins;

  /* make sure that the two solutions are for the same instance */
  if( DEBUG48 )
    fprintf(stderr, "[ KheIgSolverBuildOtherSoln\n");
  ins = KheSolnInstance(igsv->soln);
  HnAssert(ins == KheSolnInstance(other_soln),
    "KheIgSolverBuildOtherSoln internal error");

  /* make a task tree if not already done (this may be unnecessary) */
  /* *** omitting this since no longer sure if it has been done
  if( KheSolnTaskingCount(other_soln) == 0 )
    KheTaskTreeMake(other_soln, igsv->options);
  *** */

  /* starting from the history solution, build a solution for each time group */
  prev_igs = KheIgBuildHistorySoln(igsv);
  if( DEBUG48 )
    fprintf(stderr, "  KheIgSolverBuildOtherSoln init soln cost %.5f\n",
      KheCostShow(prev_igs->cost));
  HaArrayForEach(igsv->time_groups, next_igtg, i)
  {
    next_igs = KheIgSolverExpandOther(igsv, other_soln, prev_igs, next_igtg);
    if( DEBUG48 )
      fprintf(stderr, "  KheIgSolverBuildOtherSoln %s soln cost %.5f\n",
	KheIgTimeGroupId(next_igtg), KheCostShow(next_igs->cost));
    next_igtg->other_igs = next_igs;
    prev_igs = next_igs;
  }
  if( DEBUG48 )
    fprintf(stderr, "] KheIgSolverBuildOtherSoln returning\n");
  return prev_igs;
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main function"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheIgSolverDoSolve(KHE_IG_SOLVER igsv, KHE_IG_SOLN *best_igs)       */
/*                                                                           */
/*  Carry out one actual solve, assuming everything has been set up.         */
/*  If successful, return true with *best_igs set to the best solution;      */
/*  otherwise return false with *best_igs set to NULL.                       */
/*                                                                           */
/*****************************************************************************/

static bool KheIgSolverDoSolve(KHE_IG_SOLVER igsv, KHE_IG_SOLN *best_igs)
{
  KHE_IG_TIME_GROUP prev_igtg, next_igtg, last_igtg;  int i, j;
  KHE_IG_SOLN prev_igs;

  if( DEBUG19 || DEBUG34 || DEBUG26 || DEBUG33(i) )
    fprintf(stderr, "[ KheIgSolverDoSolve(cc_index %d, %d time groups)\n",
      igsv->cc_index, HaArrayCount(igsv->time_groups));

  /* clear out any previous solutions and reset the time group tasks */
  HaArrayForEach(igsv->time_groups, next_igtg, i)
  {
    KheIgTimeGroupDeleteSolns(next_igtg, igsv);
    KheIgTimeGroupSetTaskClasses(next_igtg, igsv);
  }

  /* do the solve */
  prev_igtg = NULL;
  HaArrayForEach(igsv->time_groups, next_igtg, i)
  {
    if( DEBUG19 || DEBUG34 || DEBUG26 || DEBUG33(i) )
      fprintf(stderr, "  KheIgSolverDoSolve starting to solve for %s\n",
	KheIgTimeGroupId(next_igtg));
    /* ***
    if( DEBUG47 && HaArrayCount(next_igtg->solns) > 0 )
    {
      fprintf(stderr, "  KheIgSolverDoSolve failing at %s (%d solns), e.g.\n",
        KheIgTimeGroupId(next_igtg), HaArrayCount(next_igtg->solns));
      KheIgSolnDebugTimetable(HaArrayFirst(next_igtg->solns), igsv, 2,2,stderr);
    }
    *** */
    HnAssert(next_igtg->soln_set_trie == NULL,
      "KheIgSolverDoSolve internal error 1");
    if( prev_igtg == NULL )
    {
      prev_igs = KheIgBuildHistorySoln(igsv);
      if( DEBUG19 )
      {
	fprintf(stderr, "  KheIgSolverDoSolve initial solution:\n");
	KheIgSolnDebugTimetable(prev_igs, igsv, 2, 2, stderr);
      }
      KheIgExpanderExpand(igsv->expander, prev_igs, next_igtg, -1, -1);
      if( DEBUG19 )
	fprintf(stderr, "  KheIgSolverDoSolve finished first expand\n");
    }
    else
      HaArrayForEach(prev_igtg->final_solns, prev_igs, j)
	KheIgExpanderExpand(igsv->expander, prev_igs, next_igtg, j,
	  HaArrayCount(prev_igtg->final_solns));
    HnAssert(HaArrayCount(next_igtg->final_solns) == 0,
      "KheIgSolverDoSolve internal error 2");
    KheSolnSetTrieGatherAndClear(next_igtg->soln_set_trie,
      &next_igtg->final_solns, igsv);
    next_igtg->soln_set_trie = NULL;
#if DEBUG_COMPATIBLE
    {
      KHE_IG_SOLN igs;
      HaArrayForEach(next_igtg->final_solns, igs, j)
	HnAssert(igs->cc_index == igsv->cc_index,
	  "KheIgSolverDoSolve internal error 2");
    }
#endif
    HaArraySort(next_igtg->final_solns, &KheIgSolnCmp);
    next_igtg->undominated_solns = HaArrayCount(next_igtg->final_solns);
    while( HaArrayCount(next_igtg->final_solns) > MAX_KEEP )
    {
      prev_igs = HaArrayLastAndDelete(next_igtg->final_solns);
      KheIgSolnFree(prev_igs, igsv);
    }
    if( DEBUG33(i) )
      KheIgTimeGroupDebugSolns(next_igtg, 2, 2, stderr);
    else if( DEBUG26 )
      KheIgTimeGroupDebugSolns(next_igtg, 1, 2, stderr);
    if( DEBUG54 )
      KheIgCostTrieDebug(next_igtg->cost_trie, 2, 2, stderr);

    /* KheIgSolnCacheFinalize(next_igtg->soln_cache, igsv); */
    /* ***
    #if DEBUG_COMPATIBLE
    {
      KHE_IG_SOLN igs;
      HaArrayForEach(next_igtg->solns, igs, j)
	HnAssert(igs->cc_index == igsv->cc_index,
	  "KheIgSolverDoSolve internal error 2");
    }
    #endif
    HaArraySort(next_igtg->solns, &KheIgSolnCmp);
    next_igtg->undominated_solns = HaArrayCount(next_igtg->solns);
    ** ***
    if( DEBUG34 && igsv->other_gtd != NULL )
      KheIgTimeGroupDebugCompare(next_igtg, igsv);
    *** **
    while( HaArrayCount(next_igtg->solns) > MAX_KEEP )
    {
      prev_igs = HaArrayLastAndDelete(next_igtg->solns);
      KheIgSolnFree(prev_igs, igsv);
    }
    if( DEBUG33(i) )
      KheIgTimeGroupDebugSolns(next_igtg, 2, 2, stderr);
    else if( DEBUG26 )
      KheIgTimeGroupDebugSolns(next_igtg, 1, 2, stderr);
    if( DEBUG33(i) )
      KheIgSolnCacheDebugFinalSolns(next_igtg->soln_cache,
	KheIgTimeGroupId(next_igtg), igsv, 2, 2, stderr);
    else if( DEBUG26 )
      KheIgSolnCacheDebugFinalSolns(next_igtg->soln_cache,
	KheIgTimeGroupId(next_igtg), igsv, 1, 2, stderr);
    *** */

    prev_igtg = next_igtg;
  }

  /* if there is a best solution, build its groups */
  last_igtg = HaArrayLast(igsv->time_groups);
  if( DEBUG28 && HaArrayCount(last_igtg->final_solns) > 1 )
  {
    fprintf(stderr, "[ %d final solutions:\n",
      HaArrayCount(last_igtg->final_solns));
    KheIgSolnDebug(HaArrayFirst(last_igtg->final_solns), 3, 2, stderr);
    fprintf(stderr, "  ... ... ...\n");
    KheIgSolnDebug(HaArrayLast(last_igtg->final_solns), 3, 2, stderr);
    fprintf(stderr, "]\n");
  }
  HnAssert(HaArrayCount(last_igtg->final_solns) <= 1,
    "KheIgSolverDoSolve internal error 3");
  if( HaArrayCount(last_igtg->final_solns) == 1 )
  {
    *best_igs = HaArrayFirst(last_igtg->final_solns);
    if( DEBUG10 )
    {
      fprintf(stderr, "  KheIgSolverDoSolve(%s, %s) best_igs:\n",
	KheInstanceId(KheSolnInstance(igsv->soln)),
	KheResourceTypeId(igsv->resource_type));
      KheIgSolnDebugTimetable(*best_igs, igsv, 2, 2, stderr);
    }
    if( DEBUG19 || DEBUG34 || DEBUG26 || DEBUG33(i) )
      fprintf(stderr, "] KheIgSolverDoSolve returning true\n");
    return true;
  }
  else
  {
    if( DEBUG19 || DEBUG34 || DEBUG26 || DEBUG33(i) )
      fprintf(stderr, "] KheIgSolverDoSolve returning false\n");
    return *best_igs = NULL, false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolverSolveConstraintClass(KHE_IG_SOLVER igsv,                 */
/*    KHE_CONSTRAINT_CLASS cc)                                               */
/*                                                                           */
/*  Solve constraint class cc.                                               */
/*                                                                           */
/*****************************************************************************/

static void KheIgSolverSolveConstraintClass(KHE_IG_SOLVER igsv,
  KHE_CONSTRAINT_CLASS cc)
{
  KHE_IG_SOLN best_igs, best_igs2;  KHE_TIMER timer;  char buff[20];
  int i, j;  KHE_TIME_GROUP tg;  KHE_IG_TIME_GROUP igtg;  KHE_POLARITY po;
  KHE_CONSTRAINT_CLASS busy_days_cc;

  if( DEBUG8 || DEBUG27 )
  {
    fprintf(stderr, "[ KheIgSolverSolveConstraintClass(cc, igsv)\n");
    KheConstraintClassDebug(cc, 2, 2, stderr);
    timer = KheTimerMake(KheConstraintClassId(cc), KHE_NO_TIME, igsv->arena);
    /* ***
    if( DEBUG25 )
      KheOtheSolnGroupedTasksDebug(cc, igsv);
    *** */
  }

  /* add the class and its limits to igsv */
  igsv->curr_class = cc;
  igsv->min_limit = KheConstraintClassMinimum(cc);
  igsv->max_limit = KheConstraintClassMaximum(cc);
  igsv->soln_seq_num = 0;
  /* igsv->mtask_seq_num = 0; */

  /* add the busy days classes to igsv */
  HaArrayClear(igsv->busy_days_classes);
  for( j = 0; j < KheConstraintClassFinderClassCount(igsv->busy_days_ccf); j++ )
  {
    busy_days_cc = KheConstraintClassFinderClass(igsv->busy_days_ccf, j);
    if( KheIgSelectedClassAcceptsBusyDaysClass(cc, busy_days_cc) )
      HaArrayAddLast(igsv->busy_days_classes, busy_days_cc);
  }

  /* add the time groups to igsv */
  for( i = 0;  i < KheConstraintClassTimeGroupCount(cc);  i++ )
  {
    tg = KheConstraintClassTimeGroup(cc, i, &po);
    KheIgSolverAddTimeGroup(igsv, tg);
  }

  /* add the mtasks to the time groups (all time groups must be present) */
  /* and also add their tasks to igsv->included_tasks */
  HaArrayClear(igsv->included_tasks);
  KheIgSolverAddCompulsoryMTasks(igsv);
  KheIgSolverAddWeekendMTasks(igsv);
  if( DEBUG1 )
    KheIgSolverDebug(igsv, 2, 2, stderr);

  /* clear the cost table */
  /* KheIntervalCostTableClear(igsv->cost_table); */
  /* KheIgCostTrieClear(igsv->cost_trie, igsv); */
  /* igsv->cost_trie = NULL; */
  /* KheTrieClear(igsv->cost_trie); */

  /* build another solution if wanted and present */
#if DEBUG_COMPATIBLE
  {
    KHE_SOLN other_soln;  char *id;  KHE_IG_TIME_GROUP last_igtg;
    if( KheFindNonPlaceholderOtherSoln(igsv, &other_soln, &id) )
    {
      HaArrayForEach(igsv->time_groups, igtg, i)
	KheIgTimeGroupSetTaskClasses(igtg, igsv);
      KheIgSolverBuildOtherSoln(igsv, other_soln, id);
      last_igtg = HaArrayLast(igsv->time_groups);
      fprintf(stderr, "  Other solution (%d time groups):\n",
	HaArrayCount(igsv->time_groups));
      KheIgSolnDebugTimetable(last_igtg->other_igs, igsv, 2, 2, stderr);
    }
  }
#endif

  if( DEBUG18 )
    fprintf(stderr, "  KheIgSolverSolveConstraintClass starting main solve\n");

  /* main algorithm: build solutions for each time group in turn */
  if( !KheIgSolverDoSolve(igsv, &best_igs) )
  {
    if( DEBUG8 )
      fprintf(stderr, "  no solution found\n");
    igsv->groups_count += 0;
  }
  else
  {
    if( DEBUG8 )
    {
      fprintf(stderr, "  solution without optional tasks:\n");
      KheIgSolnDebugTimetable(best_igs, igsv, 2, 2, stderr);
    }
    if( TRY_OPTIONAL_TASKS_SECOND_RUN &&
	KheIgSolnAddLengthenerTasks(best_igs, igsv) &&
	KheIgSolverDoSolve(igsv, &best_igs2) )
    {
      if( DEBUG8 )
      {
	fprintf(stderr, "  solution with optional tasks:\n");
	KheIgSolnDebugTimetable(best_igs2, igsv, 2, 2, stderr);
      }
      igsv->groups_count += KheIgSolnBuildGroups(best_igs2, igsv);
    }
    else
    {
      if( DEBUG8 )
	fprintf(stderr, "  solution with optional tasks not %s\n",
	  TRY_OPTIONAL_TASKS_SECOND_RUN ? "found" : "tried");
      igsv->groups_count += KheIgSolnBuildGroups(best_igs, igsv);
    }
  }

  if( DEBUG18 )
    fprintf(stderr, "  KheIgSolverSolveConstraintClass ending main solve\n");

  /* delete the time groups and mtasks */
  HaArrayForEach(igsv->time_groups, igtg, i)
    KheIgTimeGroupFree(igtg, igsv);
  HaArrayClear(igsv->time_groups);
  if( DEBUG8 || DEBUG27 )
    fprintf(stderr, "] KheIgSolverSolveConstraintClass returning "
      "(groups_count = %d, %s)\n", igsv->groups_count,
      KheTimeShow(KheTimerElapsedTime(timer), buff));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIntervalGrouping(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,             */
/*    KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa)                             */
/*                                                                           */
/*  Carry out interval grouping for rt.  Return the number of groups made.   */
/*                                                                           */
/*****************************************************************************/

int KheIntervalGrouping(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa)
{
  int count, res, ig_min, ig_range;  KHE_IG_SOLVER igsv;  HA_ARENA a;
  KHE_MTASK_FINDER mtf;  KHE_FRAME days_frame;  KHE_CONSTRAINT_CLASS cc;

  /* return early if rt has no resources */
  if( DEBUG1 )
    fprintf(stderr, "[ KheIntervalGrouping(mtf, %s, sa)\n",
      KheResourceTypeId(rt));
  if( KheResourceTypeResourceCount(rt) == 0 )
  {
    fprintf(stderr, "] KheIntervalGrouping returning early (no resources)\n");
    return 0;
  }

  /* get options and make an interval grouping solver */
  a = KheSolnArenaBegin(soln);
  /* ***
  if( DEBUG37 )
    KheTrieTest(a);
  *** */
  days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
  ig_min = KheOptionsGetInt(options, "rs_interval_grouping_min", 4);
  ig_range = KheOptionsGetInt(options, "rs_interval_grouping_range", 2);
  mtf = KheMTaskFinderMake(soln, rt, days_frame, true, a);
  igsv = KheIgSolverMake(soln, rt, options, ig_min, ig_range, mtf, sa, a);

  /* add constraint classes and solve for each class */
  KheConstraintClassFindCandidates(soln, rt, days_frame, igsv->candidate_ccf,
    igsv->busy_days_ccf);
  count = KheConstraintClassFinderClassCount(igsv->candidate_ccf);
  for( igsv->cc_index = 0;  igsv->cc_index < count;  igsv->cc_index++ )
  {
    cc = KheConstraintClassFinderClass(igsv->candidate_ccf, igsv->cc_index);
    if( KheConstraintClassIsSelected(cc, ig_min, ig_range) )
      KheIgSolverSolveConstraintClass(igsv, cc);
  }
  if( DEBUG2 )
    KheIgSolverDebug(igsv, 2, 2, stderr);

  /* return the number of groups made */
  res = igsv->groups_count;
  KheSolnArenaEnd(soln, a);
  if( DEBUG1 )
    fprintf(stderr, "] KheIntervalGrouping returning %d\n", res);
  return res;
}

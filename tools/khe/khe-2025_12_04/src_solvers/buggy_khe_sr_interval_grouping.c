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

#define USE_DOMAIN_DOMINATES 1
#define TRY_OPTIONAL_TASKS_SECOND_RUN 0
#define MAX_KEEP_DEFAULT 20000
#define MAX_BEAM_DEFAULT 1500

#define DEBUG_COMPATIBLE 0
#define DEBUG_EXPAND_PREV 0
#define DEBUG_REFERENCE_COUNT 0

#define DEBUG1 1		/* basic start and end call prints */
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 1		/* print final results as timetables */
#define DEBUG9 0
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
#define DEBUG26 1		/* basic stats about solve on each day */
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

#define DEBUG33(i)  (false && i == 1)

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
  (true && strcmp(KheIgTimeGroupId(igtg), "1Thu2") == 0)

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

#define DEBUG58 0
#define DEBUG59 0
#define DEBUG60 1
#define DEBUG61(i) (false && i == 2)
#define DEBUG62 0
#define DEBUG63 0
#define DEBUG64 0
#define DEBUG65 1
#define DEBUG66 1
#define DEBUG67 1

#define EQ(task, val) (strcmp(KheTaskId(task), val) == 0)
#define DEBUG68(task)							    \
  (true && (EQ(task, "1Thu:Day.31") || EQ(task, "1Thu:Day.32")))

#define DEBUG69 1


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
  KHE_TASK		task;			/* s, d(s), a(s), and f(s)   */
  KHE_IG_MTASK		ig_mtask;		/* its mtask                 */
  /* KHE_COST		non_must_assign_cost; */ /* n(s)                     */
  /* bool		optional; */		/* o(s)                      */
  KHE_COST		non_asst_cost;		/* n(s)                      */
  KHE_COST		asst_cost_plus_beta;	/* a(s) + beta               */
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
/*    soln_set_trie                                                          */
/*      A trie of sets of undominated solutions.                             */
/*                                                                           */
/*    cost_trie                                                              */
/*      A trie of (iii) + (iv) costs, indexed by T'(g).                      */
/*                                                                           */
/*    total_solns                                                            */
/*      The total number of solutions added to this time group.              */
/*                                                                           */
/*    undominated_solutions                                                  */
/*      The number of undominated solutions added to this time group.        */
/*                                                                           */
/*    kept_solns                                                             */
/*      The solutions being kept.  These are undominated solutions, but      */
/*      their number may be less than undominated_solutions, if MAX_KEEP     */
/*      indicates that we want to keep fewer.                                */
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
  ARRAY_KHE_IG_TASK_CLASS task_classes;
  int			running_tasks;		/* included tasks running    */

  /* solutions */
  KHE_IG_SOLN_SET_TRIE  soln_set_trie;		/* trie of soln sets         */
  KHE_IG_COST_TRIE	cost_trie;
  int			total_solns;		/* solns (dominated + undom) */
  int			undominated_solns;	/* solns (undominated)       */
#if DEBUG_COMPATIBLE
  KHE_IG_SOLN		other_igs;		/* from LOR17 or whatever    */
  int			compatible_solns;
#endif
  ARRAY_KHE_IG_SOLN	kept_solns;		/* after day ends            */
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
/*    optional (withdrawn)                                                   */
/*      True when this task group is optional, that is, when all of its      */
/*      tasks are optional.                                                  */
/*                                                                           */
/*    self_finished                                                          */
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
/*    index                                                                  */
/*      One element of T'(g).                                                */
/*                                                                           */
/*    primary_durn                                                           */
/*      The primary duration of the task group (the number of its times      */
/*      that are monitored by the constraint class we are currently solving  */
/*      for).  This includes any history value.  It also includes any        */
/*      primary times in the future that the task is running, i.e. it        */
/*      includes primary times in the overhang.                              */
/*                                                                           */
/*    task_cost (obsolete)                                                   */
/*      The sum of the task costs of the tasks of g, denoted v (g) in        */
/*      the documentation.                                                   */
/*                                                                           */
/*    overhang                                                               */
/*      The number of days in the future that the last task of this task     */
/*      group is running, not including the current day.  For each task      */
/*      (single-day or multiple-day) this number decreases by one on each    */
/*      subsequent day, until on the task's last day its value is 0.         */
/*                                                                           */
/*  If self_finished is true, the group is said to be "self-finished",       */
/*  meaning it must end here.  All other groups are "non-self-finished".     */
/*                                                                           */
/*  When a solution's task groups are sorted, self-finished groups come      */
/*  last.  We make no assumptions about the order that non-self-finished     */
/*  groups appear, except that they precede self-finished groups.            */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_REF_COUNT_TASK,
  KHE_REF_COUNT_TASK_GROUP,
  KHE_REF_COUNT_TASK_GROUP_CLASS,
  KHE_REF_COUNT_SOLN,
} KHE_REF_COUNT_TYPE;

typedef struct khe_ref_count_info_rec {
  KHE_REF_COUNT_TYPE type;
  void		     *referer;
} KHE_REF_COUNT_INFO;

typedef HA_ARRAY(KHE_REF_COUNT_INFO) ARRAY_KHE_REF_COUNT_INFO;

struct khe_ig_task_group_rec {
  /* INHERIT_KHE_TASK_GROUPER_ENTRY */
  KHE_IG_TASK_GROUP	prev;
  KHE_IG_TASK		ig_task;		/* task (unless history)     */
  KHE_TASK_GROUPER_ENTRY tg_entry;		/* task grouper entry        */
  bool			handle_as_assigned;	/* chi(g)                    */
  bool			self_finished;		/* self-finished             */
  bool			expand_used;		/* true if used by expand    */
  short			index;			/* one element of T'(g)      */
  short			primary_durn;		/* p(g)                      */
  short			overhang;
  int			reference_count;	/* memory handling           */
  /* bool		optional; */		/* o(g)                      */
  /* KHE_COST		non_must_assign_cost; */ /* n(g)                     */
  /* KHE_COST		task_cost; */		/* v (g)                     */
#if DEBUG_REFERENCE_COUNT
  ARRAY_KHE_REF_COUNT_INFO  ref_count_info;
#endif
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
  KHE_TASK_GROUP_DOMAIN		domain;
  /* KHE_RESOURCE_GROUP		domain; */
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
  KHE_IG_SOLN		prev_igs;		/* prev soln if any          */
  KHE_COST		cost;			/* notional cost of soln     */
  int			non_self_finished_count; /* non-self-finished groups */
  /* int		randomizer; */		/* random but deterministic  */
#if DEBUG_COMPATIBLE
  bool			compatible;
  int			cc_index;		/* index of cc               */
#endif
};


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
/*    expand_limit                                                           */
/*      An upper limit on the number of solutions that this expand is        */
/*      allowed to generate.                                                 */
/*                                                                           */
/*    next_igtg                                                              */
/*      The day after prev_igs's day; the day next_igs is for.               */
/*                                                                           */
/*    all_links                                                              */
/*      An array of all links built for use during the current expansion.    */
/*      This is used only to simplify freeing them afterwards.               */
/*                                                                           */
/*****************************************************************************/

/* typedef HA_ARRAY(KHE_MMATCH_NODE) ARRAY_KHE_MMATCH_NODE; */
typedef HA_ARRAY(KHE_COST) ARRAY_KHE_COST;

typedef struct khe_ig_expander_rec {

  /* the basics */
  KHE_IG_SOLVER			solver;
  KHE_IG_SOLN			prev_igs;
  KHE_IG_SOLN			next_igs;
  KHE_IG_TIME_GROUP		next_igtg;
  int				expand_limit;

  /* stuff which is temporary or derived from the basics */
  ARRAY_KHE_IG_TASK_GROUP_CLASS	prev_task_group_classes;
  ARRAY_KHE_IG_LINK		all_links;
  ARRAY_KHE_COST		cost_stack;
} *KHE_IG_EXPANDER;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_IG_SOLVER - interval grouping by dynamic programming solver     */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_CONSTRAINT_CLASS) ARRAY_KHE_CONSTRAINT_CLASS;

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

  /* fields defined (and mostly constant) throughout the solve */
  HA_ARENA				arena;
  KHE_SOLN				soln;
  KHE_RESOURCE_TYPE			resource_type;
  KHE_OPTIONS				options;
  int					ig_min;
  int					ig_range;
  bool					ig_two_three;
  bool					ig_complete;
  int					ig_max_keep;
  int					ig_max_beam;
  KHE_MTASK_FINDER			mtask_finder;
  KHE_FRAME				days_frame;
  KHE_TASK_GROUP_DOMAIN_FINDER		domain_finder;
  KHE_TASK_GROUPER			task_grouper;
  KHE_SOLN_ADJUSTER			soln_adjuster;
  KHE_COST				marginal_cost;
  KHE_CONSTRAINT_CLASS_FINDER		candidate_ccf;
  KHE_CONSTRAINT_CLASS_FINDER		busy_days_ccf;
  KHE_CONSTRAINT_CLASS_FINDER		complete_weekends_ccf;
  int					groups_count;

  /* fields defined when solving one constraint class */
  KHE_CONSTRAINT_CLASS			curr_class;
  int					cc_index;
  int					min_limit;
  int					max_limit;
  int					soln_seq_num;
  ARRAY_KHE_CONSTRAINT_CLASS		busy_days_classes;
  ARRAY_KHE_IG_TIME_GROUP		time_groups;
  /* ARRAY_KHE_IG_TASK			included_tasks; */

  /* scratch fields */
  KHE_IG_EXPANDER			expander;
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
      "[ KheConstraintClassIsSelected(candidate_cc min %d max %d)\n",
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
/*  bool KheConstraintClassIsTwoThree(KHE_CONSTRAINT_CLASS candidate_cc)     */
/*                                                                           */
/*  Return true if candidate_cc should be selected according to the          */
/*  two-three rule.                                                          */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintClassIsTwoThree(KHE_CONSTRAINT_CLASS candidate_cc)
{
  int minimum, maximum;
  minimum = KheConstraintClassMinimum(candidate_cc);
  maximum = KheConstraintClassMaximum(candidate_cc);
  if( DEBUG15 )
  {
    fprintf(stderr,
      "[ KheConstraintClassIsTwoThree(candidate_cc)\n");
    KheConstraintClassDebug(candidate_cc, 2, 2, stderr);
    fprintf(stderr, "] KheConstraintClassIsSelected returning"
      " %s && %s && %s\n", bool_show(minimum == 2),
      bool_show(maximum >= 3),
      bool_show(KheConstraintClassCoversResourceType(candidate_cc)));
  }
  return minimum == 2 && maximum >= 3 &&
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
  /* KHE_TASK task;  KHE_COST non_asst_cost, asst_cost; */
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

  /* add domain */
  /* *** done separately
  task = KheMTaskTask(mt, 0, &non_asst_cost, &asst_cost);
  KheTaskGroupDomainFinderAddTaskDomain(igsv->domain_finder,
    KheTaskDomain(task));
  *** */

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
/*    KHE_COST non_asst_cost, KHE_COST asst_cost, KHE_IG_SOLVER igsv)        */
/*                                                                           */
/*  Add task (which has the given non_asst_cost and asst_cost) to            */
/*  igmt->included_tasks and igsv->included_tasks.  Return the new ig task.  */
/*                                                                           */
/*****************************************************************************/
static KHE_IG_TASK KheIgTaskMake(KHE_TASK task, KHE_COST non_asst_cost,
  KHE_COST asst_cost, KHE_IG_MTASK igmt, KHE_IG_SOLVER igsv);

static KHE_IG_TASK KheIgMTaskAddTask(KHE_IG_MTASK igmt, KHE_TASK task,
  KHE_COST non_asst_cost, KHE_COST asst_cost, KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK res;  /* int index; */

  /* make the new ig task */
  res = KheIgTaskMake(task, non_asst_cost, asst_cost, igmt, igsv);

  /* add the new ig task to igsv */
  /* *** withdrawn
  index = KheTaskSolnIndex(task);
  HaArrayFill(igsv->included_tasks, index + 1, NULL);
  HnAssert(HaArray(igsv->included_tasks, index) == NULL,
    "KheIgMTaskAddTask internal error");
  HaArrayPut(igsv->included_tasks, index, res);
  *** */

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
/*  from.  This function checks conditions (3), (4), (5), and (6).           */
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
  bool in_cc_tg;  KHE_INTERVAL full_in, primary_in;  KHE_TASK task;
  KHE_COST non_asst_cost, asst_cost;

  /* condition (3): mt's tasks may not be fixed non-assigned */
  /* NB fixed mtasks always contain a single task */
  if( KheMTaskAssignIsFixed(mt) )
  {
    task = KheMTaskTask(mt, 0, &non_asst_cost, &asst_cost);
    if( KheTaskAsstResource(task) == NULL )
    {
      if( DEBUG14 )
	fprintf(stderr, "] KheMTaskIsAdmissible ret. false (fixed non-asst)\n");
      return false;
    }
  }

  /* condition (4): mt must have no overlaps and no gaps */
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

  /* condition (5): mt must contain at least one assigned time */
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

  /* condition (6): primary times (times monitored by igsv->curr_class) must */
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
/*  bool KheIgTaskIsMustAssign(KHE_TASK task, KHE_COST non_asst_cost,        */
/*    KHE_COST asst_cost)                                                    */
/*                                                                           */
/*  Assuming that task is admissible and that its non-assignment and         */
/*  assignment costs are as given, return true if it is a must-assign task.  */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskIsMustAssign(KHE_TASK task, KHE_COST non_asst_cost,
  KHE_COST asst_cost)
{
  return non_asst_cost > asst_cost || KheTaskAsstResource(task) != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK KheIgTaskMake(KHE_TASK task, KHE_COST non_asst_cost,         */
/*    KHE_COST asst_cost, KHE_IG_MTASK igmt, KHE_IG_SOLVER igsv)             */
/*                                                                           */
/*  Return an interval grouping task object with these attributes.  Do       */
/*  not add it to anything (KheIgMTaskAddTask does that).                    */
/*                                                                           */
/*****************************************************************************/
static KHE_IG_TASK_GROUP KheIgTaskGroupMakeInitial(KHE_IG_TASK igt,
  KHE_IG_SOLVER igsv);
static void KheIgTaskGroupReference(KHE_IG_TASK_GROUP igtg,
  KHE_REF_COUNT_TYPE type, void *referer);

static KHE_IG_TASK KheIgTaskMake(KHE_TASK task, KHE_COST non_asst_cost,
  KHE_COST asst_cost, KHE_IG_MTASK igmt, KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK res;
  if( DEBUG65 )
    fprintf(stderr, "KheIgTaskMake(%s)\n", KheTaskId(task));
  HnAssert(KheTaskIsProperRoot(task), "KheIgTaskMake internal error (task %s)",
    KheTaskId(task));
  if( HaArrayCount(igsv->ig_task_free_list) > 0 )
    res = HaArrayLastAndDelete(igsv->ig_task_free_list);
  else
    HaMake(res, igsv->arena);
  res->task = task;
  res->ig_mtask = igmt;
  res->non_asst_cost = non_asst_cost;
  if( KheIgTaskIsMustAssign(task, non_asst_cost, asst_cost) )
    res->asst_cost_plus_beta = asst_cost;
  else
    res->asst_cost_plus_beta = asst_cost +
      igsv->marginal_cost * KheTaskTotalDuration(task);
  /* ***
  res->task_cost = 0;
  if( KheTaskAsstResource(task) == NULL )
    res->task_cost += (asst_cost - non_asst_cost);
  if( non_asst_cost <= asst_cost && igsv->marginal_cost > 0 )
    res->task_cost += igsv->marginal_cost * KheTaskTotalDuration(task);
  *** */
  /* ***
  if( non_must_assign )
    res->non_must_assign_cost = (asst_cost - non_asst_cost) +
      igsv->marginal_cost * KheTaskTotalDuration(task);
  else
    res->non_must_assign_cost = 0;
  res->optional = igmt->primary_durn == 0 || (non_asst_cost <= asst_cost);
  *** */
  res->initial_task_group = KheIgTaskGroupMakeInitial(res, igsv);
  KheIgTaskGroupReference(res->initial_task_group,
    KHE_REF_COUNT_TASK, (void *) res);
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
/* ***
static void KheIgTaskGroupFree(KHE_IG_TASK_GROUP igtg, KHE_IG_SOLVER igsv);
*** */
static void KheIgTaskGroupUnReference(KHE_IG_TASK_GROUP igtg,
  KHE_REF_COUNT_TYPE type, void *referer, KHE_IG_SOLVER igsv, bool debug);
static char *KheIgTaskId(KHE_IG_TASK igt);
static void KheIgTaskGroupDebug(KHE_IG_TASK_GROUP igtg, KHE_IG_SOLVER igsv,
  int verbosity, int indent, FILE *fp);

static void KheIgTaskFree(KHE_IG_TASK igt, KHE_IG_SOLVER igsv)
{
#if DEBUG_REFERENCE_COUNT
  if( igt->initial_task_group->reference_count > 1 )
  {
    fprintf(stderr, "KheIgTaskFree failing on (%s rc %d):\n", KheIgTaskId(igt),
      igt->initial_task_group->reference_count);
    KheIgTaskGroupDebug(igt->initial_task_group, igsv, 3, 2, stderr);
  }
#endif
  HnAssert(igt->initial_task_group->reference_count == 1,
    "KheIgTaskFree internal error (%s rc %d)", KheIgTaskId(igt),
    igt->initial_task_group->reference_count);
  KheIgTaskGroupUnReference(igt->initial_task_group, KHE_REF_COUNT_TASK,
    (void *) igt, igsv, false);
  /* KheIgTaskGroupFree(igt->initial_task_group, igsv); */
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
/*  bool KheIgTaskSameClass(KHE_IG_TASK igt1, KHE_IG_TASK igt2,              */
/*    bool include_domain_in_test)                                           */
/*                                                                           */
/*  Return true if igt1 and igt2 belong to the same equivalence class.       */
/*  If include_domain_in_test is true, include their domains in the test,    */
/*  otherwise omit domains.                                                  */
/*                                                                           */
/*  Tasks with different non-assignment and assignment costs can lie in the  */
/*  same class.  This point is discussed in detail in the documentation.     */
/*                                                                           */
/*****************************************************************************/

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
      KheIgTaskHasFixedNonAsst(igt1) == KheIgTaskHasFixedNonAsst(igt2);
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
      igmt1->placement == igmt2->placement;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheIgTaskCost(KHE_IG_TASK igt, bool handle_as_assigned)         */
/*                                                                           */
/*  Return the task cost of igt.  It depends on whether the task will be     */
/*  assigned or not, as expressed by parameter handle_as_assigned.           */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheIgTaskCost(KHE_IG_TASK igt, bool handle_as_assigned)
{
  return handle_as_assigned ? igt->asst_cost_plus_beta : igt->non_asst_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheIgTaskCostDelta(KHE_IG_TASK igt)                             */
/*                                                                           */
/*  Return the task cost delta of igt, that is, n(t) - (a(t) + beta).        */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheIgTaskCostDelta(KHE_IG_TASK igt)
{
  return igt->non_asst_cost - igt->asst_cost_plus_beta;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIgTaskTypedCmp(KHE_IG_TASK igt1, KHE_IG_TASK igt2)                */
/*                                                                           */
/*  Typed comparison function for sorting an array of tasks by decreasing    */
/*  n(s) - (a(s) + beta).  Since beta is a constant, it doesn't matter.      */
/*                                                                           */
/*  Implementation note.  KheCostCmp does the right thing here, even when    */
/*  negative costs are present.                                              */
/*                                                                           */
/*****************************************************************************/

static int KheIgTaskTypedCmp(KHE_IG_TASK igt1, KHE_IG_TASK igt2)
{
  return KheCostCmp(KheIgTaskCostDelta(igt2), KheIgTaskCostDelta(igt1));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIgTaskCmp(const void *t1, const void *t2)                         */
/*                                                                           */
/*  Untyped comparison function for sorting an array of tasks by             */
/*  decreasing n(s) - a(s).                                                  */
/*                                                                           */
/*****************************************************************************/

static int KheIgTaskCmp(const void *t1, const void *t2)
{
  KHE_IG_TASK igt1 = * (KHE_IG_TASK *) t1;
  KHE_IG_TASK igt2 = * (KHE_IG_TASK *) t2;
  return KheIgTaskTypedCmp(igt1, igt2);
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
  fprintf(fp, "IgTask(%s, %s, non_asst_cost %.5f, asst_cost+beta %.5f%s%s)",
    KheIgTaskId(igt), KheResourceGroupId(KheTaskDomain(igt->task)),
    KheCostShow(igt->non_asst_cost), KheCostShow(igt->asst_cost_plus_beta),
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
      key->prev, value, igsv);
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
    *value = trie->value, res = (*value != -1);
  else
  {
    HnAssert(key->index >= 0, "KheTrieImplRetrieve: key item (%d) is neg\n",
      key->index);
    if( key->index >= HaArrayCount(trie->children) )
      *value = -1, res = false;
    else
      res = KheIgCostTrieRetrieve(HaArray(trie->children, key->index),
	key->prev, value);
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
    HaArrayClear(res->kept_solns);
  }
  else
  {
    HaMake(res, igsv->arena);
    HaArrayInit(res->starting_mtasks, igsv->arena);
    HaArrayInit(res->task_classes, igsv->arena);
    res->soln_set_trie = NULL;
    HaArrayInit(res->kept_solns, igsv->arena);
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

  /* set the task classes */
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

  /* sort the tasks in each task class by non-decreasing v(s) */
  HaArrayForEach(igtg->task_classes, igtc, i)
    HaArraySort(igtc->tasks, &KheIgTaskCmp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTimeGroupDeleteSolns(KHE_IG_TIME_GROUP igtg,                   */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Delete igtg's solutions.                                                 */
/*                                                                           */
/*****************************************************************************/
static void KheIgSolnFree(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv, bool debug);

static void KheIgTimeGroupDeleteSolns(KHE_IG_TIME_GROUP igtg,
  KHE_IG_SOLVER igsv, bool debug)
{
  KHE_IG_SOLN igs;  int i;
  /* KheIgSolnCacheClear(igtg->soln_cache, igsv); */
  HnAssert(igtg->soln_set_trie == NULL,
    "KheIgTimeGroupDeleteSolns internal error 1");
  KheIgCostTrieClear(igtg->cost_trie, igsv);
  igtg->cost_trie = NULL;
  igtg->total_solns = 0;
  igtg->undominated_solns = 0;
  HaArrayForEach(igtg->kept_solns, igs, i)
  {
    if( debug )
    {
      KHE_IG_TASK_GROUP igtg2;  int j;
      fprintf(stderr, "  about to delete solution with these task groups:\n");
      HaArrayForEach(igs->task_groups, igtg2, j)
	KheIgTaskGroupDebug(igtg2, igsv, 2, 4, stderr);
    }
    KheIgSolnFree(igs, igsv, debug);
  }
  HaArrayClear(igtg->kept_solns);
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
  if( DEBUG63 )
  {
    fprintf(stderr, "[ KheIgTimeGroupFree(%s, igsv)\n", KheIgTimeGroupId(igtg));
    fprintf(stderr, "  KheIgTimeGroupFree deleting solns\n");
  }
  KheIgTimeGroupDeleteSolns(igtg, igsv, DEBUG63 &&
    igtg->index >= HaArrayCount(igsv->time_groups) - 2);  /* must come first! */
  if( DEBUG63 )
    fprintf(stderr, "  KheIgTimeGroupFree deleting mtasks and tasks\n");
  HaArrayForEach(igtg->starting_mtasks, igmt, i)
    KheIgMTaskFree(igmt, igsv);
  HaArrayClear(igtg->starting_mtasks);
  HaArrayAppend(igsv->ig_task_class_free_list, igtg->task_classes, i);
  HaArrayAddLast(igsv->ig_time_group_free_list, igtg);
  if( DEBUG63 )
    fprintf(stderr, "] KheIgTimeGroupFree\n");
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
static void KheIgSolnSetTrieAddSoln(KHE_IG_SOLN_SET_TRIE *trie,
  KHE_IG_SOLN igs, KHE_IG_SOLVER igsv);

static void KheIgTimeGroupAddSoln(KHE_IG_TIME_GROUP igtg, KHE_IG_SOLN igs)
{
  /* if igs is dominated, free igs and return without changing anything else */
  if( DEBUG24 )
    fprintf(stderr, "[ KheIgTimeGroupAddSoln(%s with %d solns, igs)\n",
      KheIgTimeGroupId(igtg), igtg->undominated_solns);
  if( DEBUG44(igtg) )
  {
    fprintf(stderr, "  adding %s solution:\n", KheIgTimeGroupId(igtg));
    KheIgSolnDebugTimetable(igs, igtg->solver, 2, 2, stderr);
  }
  igtg->total_solns++;
#if DEBUG_COMPATIBLE
  if( igs->compatible )
    igtg->compatible_solns++;
#endif
  KheIgSolnSetTrieAddSoln(&igtg->soln_set_trie, igs, igtg->solver);
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
    indent, "", KheIgTimeGroupId(igtg), igtg->running_tasks);
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
    HaArrayCount(igtg->kept_solns));
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
    if( HaArrayCount(igtg->kept_solns) <= 100 )
    {
      HaArrayForEach(igtg->kept_solns, igs, i)
      {
	fprintf(stderr, "\n");
	KheIgSolnDebugTimetable(igs, igtg->solver, verbosity, indent+2, stderr);
      }
    }
    else
    {
      fprintf(fp, "\n");
      igs = HaArrayFirst(igtg->kept_solns);
      KheIgSolnDebugTimetable(igs, igtg->solver, verbosity, indent + 2, fp);
      fprintf(fp, "%*s  ... ... ...\n", indent, "");
      igs = HaArrayLast(igtg->kept_solns);
      KheIgSolnDebugTimetable(igs, igtg->solver, verbosity, indent + 2, fp);
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeGroupDebugSolnCostHisto(KHE_IG_TIME_GROUP igtg,              */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print a cost histogram.                                            */
/*                                                                           */
/*****************************************************************************/
#define HISTO_COLUMNS 10

static void KheTimeGroupDebugSolnCostHisto(KHE_IG_TIME_GROUP igtg,
  int verbosity, int indent, FILE *fp)
{
  KHE_COST min_cost, max_cost, final_max_cost, delta_cost;
  KHE_IG_SOLN igs;  int count, i, min_index, max_index;
  fprintf(fp, "%*s[ KheTimeGroupDebugSolnCostHisto(%s):\n",
    indent, "", KheIgTimeGroupId(igtg));
  count = HaArrayCount(igtg->kept_solns);
  if( count == 0 )
    fprintf(fp, "%*s  no solns\n", indent, "");
  else
  {
    min_cost = HaArrayFirst(igtg->kept_solns)->cost;
    final_max_cost = max_cost = HaArrayLast(igtg->kept_solns)->cost;
    /* ***
    fprintf(fp, "%*s  cost range: %.5f - %.5f\n", indent, "",
      KheCostShow(min_cost), KheCostShow(max_cost));
    *** */
    if( count < HISTO_COLUMNS || max_cost - min_cost < HISTO_COLUMNS )
      fprintf(fp, "%*s  %.5f - %.5f   %d\n", indent, "",
	KheCostShow(min_cost), KheCostShow(max_cost), count);
    else
    {
      delta_cost = (max_cost - min_cost) / HISTO_COLUMNS;
      min_index = 0;
      do
      {
	/* find the next max_cost and max_index */
	max_cost = min_cost + delta_cost;
	for( i = min_index;  i < HaArrayCount(igtg->kept_solns);  i++ )
	{
	  igs = HaArray(igtg->kept_solns, i);
	  if( igs->cost >= max_cost )
	    break;
	}
	max_index = i - 1;

	/* print the entry */
	fprintf(fp, "%*s  %8.5f - %8.5f  %3d\n", indent, "",
	  KheCostShow(min_cost), KheCostShow(max_cost),
	  max_index - min_index + 1);

	/* move on to the next interval */
	min_index = max_index + 1;
	min_cost = max_cost;
      } while( max_cost <= final_max_cost );
    }
  }
  fprintf(fp, "%*s]\n", indent, "");
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

/* ***
static void KheIgTaskGroupCopyGrouperEntry(KHE_IG_TASK_GROUP igtg,
  KHE_TASK_GROUPER_ENTRY grouper_entry)
{
  igtg->task = grouper_entry->task;
  igtg->domain = grouper_entry->domain;
  igtg->assigned_resource = grouper_entry->assigned_resource;
  igtg->prev = grouper_entry->prev;
  igtg->interval = grouper_entry->interval;
  igtg->type = grouper_entry->type;
  igtg->has_fixed_unassigned = grouper_entry->has_fixed_unassigned;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskGroupCheck(KHE_IG_TASK_GROUP igtg)                         */
/*                                                                           */
/*  Bug-finding function.                                                    */
/*                                                                           */
/*****************************************************************************/

static void KheIgTaskGroupCheck(KHE_IG_TASK_GROUP igtg)
{
  KHE_TASK task1, task2;
  if( DEBUG69 && igtg->ig_task != NULL )
  {
    task1 = igtg->ig_task->task;
    task2 = KheTaskGrouperEntryTask(igtg->tg_entry);
    HnAssert(task1 == task2, "KheIgTaskGroupCheck failing at %p "
      "(ig_task %s != tg_entry %s)", (void *) igtg,
      task1 == NULL ? "NULL" : KheTaskId(task1),
      task2 == NULL ? "NULL" : KheTaskId(task2));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK_GROUP KheIgTaskGroupMake(                                    */
/*    KHE_TASK_GROUPER_ENTRY grouper_entry, bool self_finished, int index,   */
/*    int primary_durn, KHE_COST task_cost, int overhang, KHE_IG_SOLVER igsv)*/
/*                                                                           */
/*  Make a task group with these attributes.                                 */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_TASK_GROUP KheIgTaskGroupMake(KHE_IG_TASK_GROUP prev,
  KHE_IG_TASK ig_task, KHE_TASK_GROUPER_ENTRY tg_entry,
  bool handle_as_assigned, bool self_finished, int index,
  int primary_durn, int overhang, KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK_GROUP res;
  if( HaArrayCount(igsv->ig_task_group_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(igsv->ig_task_group_free_list);
#if DEBUG_REFERENCE_COUNT
    HaArrayClear(res->ref_count_info);
#endif
  }
  else
  {
    HaMake(res, igsv->arena);
#if DEBUG_REFERENCE_COUNT
    HaArrayInit(res->ref_count_info, igsv->arena);
#endif
  }
  HnAssert(tg_entry != NULL, "KheIgTaskGroupMake internal error");
  /* KheIgTaskGroupCopyGrouperEntry(res, grouper_entry); */
  res->prev = prev;
  res->ig_task = ig_task;
  res->tg_entry = tg_entry;
  res->handle_as_assigned = handle_as_assigned;
  res->self_finished = self_finished;
  res->expand_used = false;
  res->index = index;
  res->primary_durn = primary_durn;
  /* res->task_cost = task_cost; */
  res->overhang = overhang;
  res->reference_count = 0;
  KheIgTaskGroupCheck(res);
  return res;
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

/* *** no longer used
static void KheIgTaskGroupFree(KHE_IG_TASK_GROUP igtg, KHE_IG_SOLVER igsv)
{
  HaArrayAddLast(igsv->ig_task_group_free_list, igtg);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskGroupReference(KHE_IG_TASK_GROUP igtg)                     */
/*                                                                           */
/*  Call this when you link to igtg.                                         */
/*                                                                           */
/*****************************************************************************/

static void KheIgTaskGroupReference(KHE_IG_TASK_GROUP igtg,
  KHE_REF_COUNT_TYPE type, void *referer)
{
#if DEBUG_REFERENCE_COUNT
  KHE_REF_COUNT_INFO rci;
  rci.type = type;
  rci.referer = referer;
  HaArrayAddLast(igtg->ref_count_info, rci);
#endif
  igtg->reference_count++;
  KheIgTaskGroupCheck(igtg);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

#if DEBUG_REFERENCE_COUNT
static bool KheIgTaskGroupContainsReference(KHE_IG_TASK_GROUP igtg,
  KHE_REF_COUNT_TYPE type, void *referer, int *pos)
{
  KHE_REF_COUNT_INFO rci;  int i;
  HaArrayForEachReverse(igtg->ref_count_info, rci, i)
    if( rci.type == type && rci.referer == referer )
      return *pos = i, true;
  return *pos = -1, false;
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  void KheDebugFree(KHE_IG_TASK_GROUP igtg)                                */
/*                                                                           */
/*  Bug-hunting function.                                                    */
/*                                                                           */
/*****************************************************************************/

static void KheDebugFree(KHE_IG_TASK_GROUP igtg)
{
  KHE_IG_TASK igt;
  igt = igtg->ig_task;
  if( igt != NULL && DEBUG68(igt->task) )
    fprintf(stderr, "  freeing igtg %p containing %s\n", (void *) igtg,
      KheTaskId(igt->task));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskGroupUnReference(KHE_IG_TASK_GROUP igtg,                   */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Call this when you unlink from igtg.                                     */
/*                                                                           */
/*****************************************************************************/

static void KheIgTaskGroupUnReference(KHE_IG_TASK_GROUP igtg,
  KHE_REF_COUNT_TYPE type, void *referer, KHE_IG_SOLVER igsv, bool debug)
{
  HnAssert(igtg != NULL, "KheIgTaskGroupUnReference internal error 3");
  if( debug )
  {
    fprintf(stderr, "[ KheIgTaskGroupUnReference(igtg, %d, %p)\n",
      type, referer);
    KheIgTaskGroupDebug(igtg, igsv, 2, 2, stderr);
  }
  KheIgTaskGroupCheck(igtg);
#if DEBUG_REFERENCE_COUNT
  {
    int pos;
    HnAssert(HaArrayCount(igtg->ref_count_info) == igtg->reference_count,
      "KheIgTaskGroupUnReference internal error 1");
    if( !KheIgTaskGroupContainsReference(igtg, type, referer, &pos) )
    {
      fprintf(stderr, "KheIgTaskGroupUnReference failing on %d %p:\n",
	type, referer);
      KheIgTaskGroupDebug(igtg, igsv, 3, 2, stderr);
      HnAbort("KheIgTaskGroupUnReference internal error 2");
    }
    HaArrayDeleteAndPlug(igtg->ref_count_info, pos);
  }
#endif
  igtg->reference_count--;
  HnAssert(igtg->reference_count >= 0,
    "KheIgTaskGroupUnReference internal error 2");
  if( igtg->reference_count == 0 )
  {
    /* free igtg->tg_entry unless igtg->prev will do it */
    if( igtg->prev == NULL || igtg->prev->tg_entry != igtg->tg_entry )
      KheTaskGrouperEntryFree(igtg->tg_entry, igsv->task_grouper);

    /* unreference igtg->prev */
    if( igtg->prev != NULL )
      KheIgTaskGroupUnReference(igtg->prev, KHE_REF_COUNT_TASK_GROUP,
	(void *) igtg, igsv, debug);

    /* add igtg to the free list */
    if( DEBUG67 )
      KheDebugFree(igtg);
    HaArrayAddLast(igsv->ig_task_group_free_list, igtg);
  }
  if( debug )
    fprintf(stderr, "]\n");
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
  KHE_TASK_GROUPER_ENTRY tg_entry;
  /* struct khe_task_grouper_entry_rec new_entry; */
  tg_entry = KheTaskGrouperEntryMakeHistory(igsv->task_grouper,
    r, primary_durn);
  /* ***
  KheTaskGrouperEntryAddHistory(NULL, r, primary_durn, igsv->domain_finder,
    &new_entry);
  *** */
  return KheIgTaskGroupMake(NULL, NULL, tg_entry, true, false, primary_durn,
    primary_durn, 0, igsv);
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
  /* struct khe_task_grouper_entry_rec new_entry; */
  KHE_IG_TASK_GROUP res;
  KHE_IG_MTASK igmt;  KHE_TASK_GROUPER_ENTRY tg_entry;
  igmt = igt->ig_mtask;
  KheTaskGrouperEntryMakeTask(igsv->task_grouper, NULL, igt->task,
    false, &tg_entry);
  res = KheIgTaskGroupMake(NULL, igt, tg_entry,
    KheTaskAsstResource(igt->task) != NULL,
    igmt->finished_cd || igmt->primary_durn >= igsv->max_limit,
    KheIgMTaskIndex(igmt, igmt->full_durn - 1),
    igmt->primary_durn, /* igt->task_cost, */ igmt->full_durn - 1, igsv);
  if( DEBUG68(igt->task) )
    fprintf(stderr, "  making initial %p containing %s\n",
      (void *) res, KheTaskId(igt->task));
  return res;
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
/*  Implementation note.  In this case the handle_as_assigned field is       */
/*  always true.  This is because the new group must either have at          */
/*  least two tasks or one task plus history.                                */
/*                                                                           */
/*  Implementation note.  We call KheIgTaskGroupReference because the        */
/*  result object contains a reference to igtg.  This is actually already    */
/*  present in new_entry when this function is called, but still we do it    */
/*  here.                                                                    */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_TASK_GROUP KheIgTaskGroupMakeSuccessor(KHE_IG_TASK_GROUP igtg,
  KHE_IG_TASK igt, KHE_TASK_GROUPER_ENTRY tg_entry, KHE_IG_SOLVER igsv)
{
  KHE_IG_MTASK igmt;  int primary_durn;  KHE_IG_TASK_GROUP res;
  /* KHE_COST task_cost; */
  igmt = igt->ig_mtask;
  primary_durn = igtg->primary_durn + igmt->primary_durn;
  /* task_cost = igtg->task_cost + igt->task_cost; */
  res = KheIgTaskGroupMake(igtg, igt, tg_entry, true,
    igmt->finished_cd || primary_durn >= igsv->max_limit,
    KheIgMTaskIndex(igmt, igmt->full_durn - 1),
    primary_durn, igmt->full_durn - 1, igsv);
  if( DEBUG68(igt->task) )
    fprintf(stderr, "  making successor %p containing %s\n",
      (void *) res, KheTaskId(igt->task));
  KheIgTaskGroupReference(igtg, KHE_REF_COUNT_TASK_GROUP, (void *) res);
  /* ***
  if( DEBUG45 )
    fprintf(stderr, "  KheIgTaskGroupMakeSuccessor: new_entry->type %d, "
      "res->type %d\n", (int) new_entry->type, (int) res->type);
  *** */
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
static KHE_TASK KheIgTaskGroupTask(KHE_IG_TASK_GROUP igtg);
/* ***
static KHE_IG_TASK KheIgSolverTaskToIgTask(KHE_IG_SOLVER igsv, KHE_TASK task);
*** */

static KHE_IG_TASK_GROUP KheIgTaskGroupMakeDummy(KHE_IG_TASK_GROUP igtg,
  KHE_IG_SOLVER igsv)
{
  /* struct khe_task_grouper_entry_rec new_entry; */  KHE_IG_TASK_GROUP res;
  /* KHE_IG_TASK igt; */
  /* KheTaskGrouperEntryAddDummy(( KHE_TASK_GROUPER_ENTRY) igtg, &new_entry); */
  /* igt = KheIgSolverTaskToIgTask(igsv, KheIgTaskGroupTask(igtg)); */
  res = KheIgTaskGroupMake(igtg, igtg->ig_task, igtg->tg_entry,
    igtg->handle_as_assigned, igtg->self_finished,
    KheIgMTaskIndex(igtg->ig_task->ig_mtask, igtg->overhang - 1),
    igtg->primary_durn, /* igtg->task_cost, */ igtg->overhang - 1, igsv);
  KheIgTaskGroupReference(igtg, KHE_REF_COUNT_TASK_GROUP, (void *) res);
  if( DEBUG68(igtg->ig_task->task) )
    fprintf(stderr, "  making dummy %p containing %s\n",
      (void *) res, KheTaskId(igtg->ig_task->task));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK_GROUP KheIgTaskGroupCopy(KHE_IG_TASK_GROUP igtg,             */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Return a copy of igtg.                                                   */
/*                                                                           */
/*****************************************************************************/

/* *** we're sharing now, not copying
static KHE_IG_TASK_GROUP KheIgTaskGroupCopy(KHE_IG_TASK_GROUP igtg,
  KHE_IG_SOLVER igsv)
{
  return KheIgTaskGroupMake(( KHE_TASK_GROUPER_ENTRY) igtg,
    igtg->handle_as_assigned, igtg->self_finished, igtg->index,
    igtg->primary_durn, ** igtg->task_cost, ** igtg->overhang, igsv);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_TASK_GROUP" - query                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheIgTaskGroupTask(KHE_IG_TASK_GROUP igtg)                      */
/*                                                                           */
/*  return igtg's task.                                                      */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK KheIgTaskGroupTask(KHE_IG_TASK_GROUP igtg)
{
  return KheTaskGrouperEntryTask(igtg->tg_entry);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUP_DOMAIN KheIgTaskGroupDomain(KHE_IG_TASK_GROUP igtg)       */
/*                                                                           */
/*  Return the domain of igtg.  This is d(g) in the documentation.           */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_GROUP_DOMAIN KheIgTaskGroupDomain(KHE_IG_TASK_GROUP igtg)
{
  return KheTaskGrouperEntryDomain(igtg->tg_entry);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheIgTaskGroupDomainValue(KHE_IG_TASK_GROUP igtg)     */
/*                                                                           */
/*  Return the domain of igtg.  This is d(g) in the documentation.           */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_GROUP KheIgTaskGroupDomainValue(KHE_IG_TASK_GROUP igtg)
{
  return KheTaskGroupDomainValue(KheIgTaskGroupDomain(igtg));
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheIgTaskGroupAssignedResource(KHE_IG_TASK_GROUP igtg)      */
/*                                                                           */
/*  Return the assigned resource attribute of igtg.                          */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE KheIgTaskGroupAssignedResource(KHE_IG_TASK_GROUP igtg)
{
  return KheTaskGrouperEntryAssignedResource(igtg->tg_entry);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupHasFixedUnassigned(KHE_IG_TASK_GROUP igtg)            */
/*                                                                           */
/*  Return true if igtg has a fixed unassigned task.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheIgTaskGroupHasFixedUnassigned(KHE_IG_TASK_GROUP igtg)
{
  return KheTaskGrouperEntryHasFixedUnassigned(igtg->tg_entry);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUPER_ENTRY_TYPE KheIgTaskGroupType(KHE_IG_TASK_GROUP igtg)   */
/*                                                                           */
/*  Return the entry type of igtg.                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_TASK_GROUPER_ENTRY_TYPE KheIgTaskGroupType(KHE_IG_TASK_GROUP igtg)
{
  return KheTaskGrouperEntryType(igtg->tg_entry);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheIgTaskGroupInterval(KHE_IG_TASK_GROUP igtg)              */
/*                                                                           */
/*  Return the interval of days that igtg is running.                        */
/*                                                                           */
/*****************************************************************************/

static KHE_INTERVAL KheIgTaskGroupInterval(KHE_IG_TASK_GROUP igtg)
{
  return KheTaskGrouperEntryInterval(igtg->tg_entry);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupUndersized(KHE_IG_TASK_GROUP igtg, KHE_IG_SOLVER igsv)*/
/*                                                                           */
/*  Return true if igtg is undersized.  This means not only that its         */
/*  primary duration falls short of the minimum limit, but also that         */
/*  it is not zero, so the short length will have a cost.                    */
/*                                                                           */
/*  This code does not check whether igtg comes right at the end of the      */
/*  cycle.  That would be a reason for considering it to be not undersized   */
/*  after all, so care is needed on that point.                              */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskGroupUndersized(KHE_IG_TASK_GROUP igtg, KHE_IG_SOLVER igsv)
{
  /* return igtg->prev != NULL && igtg->primary_durn < igsv->min_limit; */
  return 0 < igtg->primary_durn && igtg->primary_durn < igsv->min_limit;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupIsSelfFinishedNoOverhang(KHE_IG_TASK_GROUP igtg)      */
/*                                                                           */
/*  Return true if igtg is self-finished with no overhang.  Groups with      */
/*  this property always come at the end of sorted lists of groups.          */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskGroupIsSelfFinishedNoOverhang(KHE_IG_TASK_GROUP igtg)
{
  return igtg->self_finished && igtg->overhang == 0;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK_GROUP KheIgTaskGroupPrev(KHE_IG_TASK_GROUP igtg)             */
/*                                                                           */
/*  Return igtg's predecessor task group, or NULL if none.                   */
/*                                                                           */
/*****************************************************************************/

/* *** have a field for this now
static KHE_IG_TASK_GROUP KheIgTaskGroupPrev(KHE_IG_TASK_GROUP igtg)
{
  return (KHE_IG_TASK_GROUP) igtg->prev;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK_GROUP KheIgTaskGroupDistinctPrev(KHE_IG_TASK_GROUP igtg)     */
/*                                                                           */
/*  Return igtg's first distinct predecessor, or NULL if none.  A distinct   */
/*  predecessor is a predecessor which is an ordinary task group with a      */
/*  different task to igtg's task.                                           */
/*                                                                           */
/*  We don't count a history task group as a distinct predecessor, even      */
/*  though it arguably is, because in the places where this function is      */
/*  called, history is handled separately (by having a non-NULL r(g)).       */
/*                                                                           */
/*****************************************************************************/

/* *** on second thoughts, not needed
static KHE_IG_TASK_GROUP KheIgTaskGroupDistinctPrev(KHE_IG_TASK_GROUP igtg)
{
  KHE_TASK task;  KHE_IG_TASK_GROUP prev;
  HnAssert(igtg->type != KHE_TASK_GROUPER_ENTRY_HISTORY,
    "KheIgTaskGroupDistinctPrev internal error");
  task = igtg->task;
  prev = KheIgTaskGroupPrev(igtg);
  while( prev != NULL )
  {
    if( prev->type == KHE_TASK_GROUPER_ENTRY_ORDINARY && prev->task != task )
      return prev;
    prev = KheIgTaskGroupPrev(igtg);
  }
  return NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupHandleAsAssigned(KHE_IG_TASK_GROUP igtg)              */
/*                                                                           */
/*  Return true if we are handling igtg as assigned, that is, if it has      */
/*  either an assigned resource or a distinct predecessor.                   */
/*                                                                           */
/*****************************************************************************/

/* *** stored as a field now
static bool KheIgTaskGroupHandleAsAssigned(KHE_IG_TASK_GROUP igtg)
{
  return igtg->assigned_resource != NULL ||
    KheIgTaskGroupDistinctPrev(igtg) != NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIgTaskGroupBeginHandleAsAssigned(KHE_IG_TASK_GROUP igtg)         */
/*                                                                           */
/*  Return true if we are beginning to handle igtg as assigned, that is,     */
/*  we handle igtg as assigned but either there is no distinct predecessor   */
/*  or we do not handle that predecessor as assigned.                        */
/*                                                                           */
/*****************************************************************************/

/* *** on second thoughts, not needed
static bool KheIgTaskGroupBeginHandleAsAssigned(KHE_IG_TASK_GROUP igtg)
{
  KHE_IG_TASK_GROUP prev;
  if( !igtg->handle_as_assigned )
    return false;
  prev = KheIgTaskGroupDistinctPrev(igtg);
  return prev == NULL || !prev->handle_as_assigned;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK KheIgTaskGroupSoleTask(KHE_IG_TASK_GROUP igtg,               */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Return the sole task lying in igtg.                                      */
/*                                                                           */
/*****************************************************************************/

/* *** on second thoughts, not needed
static KHE_IG_TASK KheIgTaskGroupSoleTask(KHE_IG_TASK_GROUP igtg,
  KHE_IG_SOLVER igsv)
{
  HnAssert(KheIgTaskGroupDistinctPrev(igtg) == NULL,
    "KheIgTaskGroupSoleTask internal error");
  return KheIgSol verTaskToIgTask(igsv, igtg->task);
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
      if( dt2->prev == dt )
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
  return r != NULL && r == KheIgTaskGroupAssignedResource(igtg);
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
  KHE_IG_TASK igt, KHE_IG_SOLVER igsv, KHE_TASK_GROUPER_ENTRY *new_entry)
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
    if( DEBUG23 && KheIgTaskGroupTask(igtg) == NULL )
      fprintf(stderr, "  extendable not grouping history %s with task %s (4)\n",
	KheResourceId(KheIgTaskGroupAssignedResource(igtg)), KheIgTaskId(igt));
    return false;
  }

  /* (3) not extendable if igtg->self_finished */
  if( igtg->self_finished )
  {
    if( DEBUG23 && KheIgTaskGroupTask(igtg) == NULL )
      fprintf(stderr, "  extendable not grouping history %s with task %s (1)\n",
	KheResourceId(KheIgTaskGroupAssignedResource(igtg)), KheIgTaskId(igt));
    return false;
  }

  /* (4) not extendable if igt's placement is first only */
  if( igt->ig_mtask->placement == KHE_PLACEMENT_FIRST_ONLY )
    return false;

  /* (5) build new_entry; not extendable if can't */
  return KheTaskGrouperEntryMakeTask(igsv->task_grouper, igtg->tg_entry,
    igt->task, false, new_entry);
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
    igtg1 = igtg1->prev;
    igtg2 = igtg2->prev;
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
  KHE_IG_TASK_GROUP igtg2, KHE_IG_SOLVER igsv, /* bool *equal_so_far, */
  bool debug)
{
  /* KHE_RESOURCE_GROUP rg1, rg2;  KHE_COST cost1, cost2; */
  KHE_IG_TASK igt1, igt2;  KHE_RESOURCE r1, r2;  /* bool hfu1, hfu2; */

  /* (0) primary_durn - subsumed by (4), but faster to test first */
  if( igtg1->primary_durn != igtg2->primary_durn )
  {
    if( debug )
      fprintf(stderr, "  primary durations differ: %d != %d\n",
        igtg1->primary_durn, igtg2->primary_durn);
    return false;
  }

  /* (1) last days equal - subsumed by (5) below */

  /* (2) domains */
#if USE_DOMAIN_DOMINATES
  {
    bool proper;
    if( !KheTaskGroupDomainDominates(KheIgTaskGroupDomain(igtg1),
	KheIgTaskGroupDomain(igtg2), &proper) )
    {
      if( debug )
      {
	fprintf(stderr, "  domain compatibility set test fails: ");
	KheTaskGroupDomainDebug(KheIgTaskGroupDomain(igtg1), 2, -1, stderr);
	fprintf(stderr, " vs. ");
	KheTaskGroupDomainDebug(KheIgTaskGroupDomain(igtg2), 2, 0, stderr);
      }
      return false;
    }
    /* ***
    if( proper )
      *equal_so_far = false;
    *** */
  }
#else
  {
    KHE_RESOURCE_GROUP rg1, rg2;
    rg1 = KheIgTaskGroupDomainValue(igtg1);
    rg2 = KheIgTaskGroupDomainValue(igtg2);
    if( !KheResourceGroupSubset(rg2, rg1) )
    {
      if( debug )
      {
	fprintf(stderr, "  domain superset test fails: ");
	KheTaskGroupDomainDebug(igtg1->domain, 2, -1, stderr);
	fprintf(stderr, " vs. ");
	KheTaskGroupDomainDebug(igtg2->domain, 2, 0, stderr);
      }
      return false;
    }
    /* ***
    if(KheResourceGroupResourceCount(rg2) != KheResourceGroupResourceCount(rg1))
      *equal_so_far = false;
    *** */
  }
#endif

  /* (3) assignments */
  r1 = KheIgTaskGroupAssignedResource(igtg1);
  r2 = KheIgTaskGroupAssignedResource(igtg2);
  if( r1 != r2 )
  {
    if( debug )
    {
      fprintf(stderr, "  assigned resources fail: %s != %s\n",
        r1 == NULL ? "NULL" : KheResourceId(r1),
        r2 == NULL ? "NULL" : KheResourceId(r2));
    }
    return false;
  }

  /* (4) fixed non-assignments */
  /* ***
  hfu1 = KheIgTaskGroupHasFixedUnassigned(igtg1);
  hfu2 = KheIgTaskGroupHasFixedUnassigned(igtg2);
  if( hfu1 != hfu2 )
  {
    if( debug )
      fprintf(stderr, "  fixed non-assignment fail: %s != %s\n",
	bool_show(hfu1), bool_show(hfu2));
    return false;
  }
  *** */

  /* (5) times (plus history) */
  if( !KheIgTaskGroupIndexesEqual(igtg1, igtg2) )
  {
    if( debug )
      fprintf(stderr, "  times+history fail\n");
    return false;
  }

  /* (6) chi */
  if( igtg1->handle_as_assigned != igtg2->handle_as_assigned )
  {
    if( debug )
      fprintf(stderr, "  handle as assigned fail\n");
    return false;
  }

  /* (7) task costs when chi = false */
  if( !igtg1->handle_as_assigned )
  {
    igt1 = igtg1->ig_task;
    igt2 = igtg2->ig_task;
    /* ***
    igt1 = KheIgSolverTaskToIgTask(igsv, KheIgTaskGroupTask(igtg1));
    igt2 = KheIgSolverTaskToIgTask(igsv, KheIgTaskGroupTask(igtg2));
    *** */
    if( igt1->non_asst_cost > igt2->non_asst_cost )
    {
      if( debug )
	fprintf(stderr, "  non_asst_cost fail\n");
      return false;
    }
    if( igt1->asst_cost_plus_beta > igt2->asst_cost_plus_beta )
    {
      if( debug )
	fprintf(stderr, "  asst_cost_plus_beta fail\n");
      return false;
    }
  }

  /* (5) task costs */
  /* ***
  if( igtg1->task_cost > igtg2->task_cost )
  {
    if( debug )
      fprintf(stderr, "  task costs fail: igtg1 %.5f > igtg2 %.5f\n",
	KheCostShow(igtg1->task_cost), KheCostShow(igtg2->task_cost));
    return false;
    }
  }
  *** */

  /* (5) task costs */
  /* ***
  if( igtg1->task_cost > igtg2->task_cost )
  {
    if( debug )
      fprintf(stderr, "  task costs fail: igtg1 %.5f > igtg2 %.5f\n",
	KheCostShow(igtg1->task_cost), KheCostShow(igtg2->task_cost));
    return false;
  }
  *** */

  /* ***
  KheIgTaskGroupSetIndexes(igtg1, INT_MAX, igsv, &igsv->tmp_indexes1);
  KheIgTaskGroupSetIndexes(igtg2, INT_MAX, igsv, &igsv->tmp_indexes2);
  if( !KheArrayIntEqual(&igsv->tmp_indexes1, &igsv->tmp_indexes2) )
    return false;
  *** */

  /* (5) optionality */
  /* *** omitting this now
  if( !igtg1->optional && igtg2->optional )
    return false;
  *** */

  /* (5) task costs - but only if primary duration is 1 */
  /* ***
  if( igtg1->primary_durn == 1 )
  {
    if( igtg1->task_cost > igtg2->task_cost )
      return false;
  }
  *** */

  /* (5) task costs - but only if either group could be unassigned */
  /* ***
  if( igtg1->prev == NULL || igtg2->prev == NULL )
  {
    if( igtg1->task_cost > igtg2->task_cost )
      return false;
  }
  *** */

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
/*  KHE_COST KheIgTaskGroupCost(KHE_IG_TASK_GROUP last_igtg,                 */
/*    bool handle_as_assigned, KHE_IG_TIME_GROUP igtg, KHE_IG_SOLVER igsv)   */
/*                                                                           */
/*  Return the cost of last_igtg, denoted c(g) in the documentation.         */
/*  This depends on whether we are handling last_igtg as assigned or not.    */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheIgTaskGroupCost(KHE_IG_TASK_GROUP last_igtg,
  bool handle_as_assigned, KHE_IG_TIME_GROUP igtg, KHE_IG_SOLVER igsv)
{
  KHE_COST c3, c4, res;  bool last;

  if( handle_as_assigned )
  {
    /* either get (iii) + (iv) from the cache, or calculate it */
    if( last_igtg->primary_durn == 0 )
      res = 0;
    else if( igtg == NULL )
    {
      /* this can only be a history task group that was not extended */
      HnAssert(KheIgTaskGroupTask(last_igtg) == NULL,
	"KheIgTaskGroupCost internal error 1");
      res = KheConstraintClassDeterminantToCost(igsv->curr_class,
	last_igtg->primary_durn, false);
      HnAssert(res >= 0, "KheIgTaskGroupCost internal error 2");
    }
    else if( !KheIgCostTrieRetrieve(igtg->cost_trie, last_igtg, &res) )
    {
      /* no cached value, so first find the group cost (iii) */
      c3 = KheTaskGrouperEntryCost(last_igtg->tg_entry, igsv->task_grouper);
	  /* igsv->days_frame, igsv->soln); */
      HnAssert(c3 >= 0, "KheIgTaskGroupCost internal error 3");

      /* then find the constraint class cost (iv) */
      last = (igtg == HaArrayLast(igsv->time_groups));
      c4 = KheConstraintClassDeterminantToCost(igsv->curr_class,
	last_igtg->primary_durn, last);
      HnAssert(c4 >= 0, "KheIgTaskGroupCost internal error 4");

      /* cache the result */
      res = c3 + c4;
      KheIgCostTrieAdd(&igtg->cost_trie, last_igtg, res, igsv);
    }
    return res;
  }
  else
  {
    /* as the doc says, unassigned groups cost zero */
    return 0;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheIgTaskGroupCost(KHE_IG_TASK_GROUP last_igtg, int li,         */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Return the cost of last_igtg, denoted c(g) in the documentation.         */
/*  Most of this code is devoted to working out c_a(g), consisting of        */
/*  the four parts (i), (ii), (iii), and (iv).                               */
/*                                                                           */
/*****************************************************************************/
/* static void KheArrayIntDebug(HA_ARRAY_INT *array_int, FILE *fp); */

/* *** old version that includes task costs
static KHE_COST KheIgTaskGroupCost(KHE_IG_TASK_GROUP last_igtg,
  KHE_IG_TIME_GROUP igtg, KHE_IG_SOLVER igsv)
{
  KHE_COST c_iii, c_iv, c_iii_iv, c_a;  bool last;

  ** either get (iii) + (iv) from the cache, or calculate it **
  if( last_igtg->primary_durn == 0 )
    c_iii_iv = 0;
  else if( igtg == NULL )
  {
    ** this can only be a history task group that was not extended **
    HnAssert(last_igtg->type == KHE_TASK_GROUPER_ENTRY_HISTORY,
      "KheIgTaskGroupCost internal error 1");
    c_iii_iv = KheConstraintClassDeterminantToCost(igsv->curr_class,
      last_igtg->primary_durn, false);
    HnAssert(c_iii_iv >= 0, "KheIgTaskGroupCost internal error 2");
  }
  else if( !KheIgCostTrieRetrieve(igtg->cost_trie, last_igtg, &c_iii_iv) )
  {
    ** no cached value, so first find the group cost (iii) **
    c_iii = KheTaskGrouperEntryCost(( KHE_TASK_GROUPER_ENTRY) last_igtg,
	igsv->days_frame, igsv->soln);
    HnAssert(c_iii >= 0, "KheIgTaskGroupCost internal error 3");

    ** then find the constraint class cost (iv) **
    last = (igtg == HaArrayLast(igsv->time_groups));
    c_iv = KheConstraintClassDeterminantToCost(igsv->curr_class,
      last_igtg->primary_durn, last);
    HnAssert(c_iv >= 0, "KheIgTaskGroupCost internal error 4");

    ** cache the result **
    c_iii_iv = c_iii + c_iv;
    KheIgCostTrieAdd(&igtg->cost_trie, last_igtg, c_iii_iv, igsv);
  }

  c_a = c_iii_iv + last_igtg->task_cost;
  if( last_igtg->assigned_resource == NULL && last_igtg->prev == NULL )
    return min(c_a, 0);
  else
    return c_a;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheTaskGroupFinishedCost(KHE_IG_TASK_GROUP igtg,                */
/*    KHE_IG_TIME_GROUP next_igtg, KHE_IG_SOLVER igsv)                       */
/*                                                                           */
/*  Work out the finished cost of igtg, that is, the group cost and task     */
/*  cost that we need to add to the solution, assuming igtg is finished.     */
/*                                                                           */
/*  If chi(igtg), this is just the group cost assuming igtg is assigned.     */
/*                                                                           */
/*  If not chi(igtg), we have to work out the total cost assuming that igtg  */
/*  is assigned, then again assuming that it isn't, and take the smaller.    */
/*                                                                           */
/*****************************************************************************/
static char *KheIgTaskGroupId(KHE_IG_TASK_GROUP igtg);

static KHE_COST KheTaskGroupFinishedCost(KHE_IG_TASK_GROUP igtg,
  KHE_IG_TIME_GROUP next_igtg, KHE_IG_SOLVER igsv)
{
  KHE_COST assigned_cost, unassigned_cost, res;  KHE_IG_TASK igt;
  if( igtg->handle_as_assigned )
  {
    res = KheIgTaskGroupCost(igtg, true, next_igtg, igsv);
    if( DEBUG64 && res >= KheCost(1, 0) )
    {
      fprintf(stderr, "  %s assigned cost: %.5f\n",
	KheIgTaskGroupId(igtg), KheCostShow(res));
      KheIgTaskGroupDebug(igtg, igsv, 3, 2, stderr);
    }
  }
  else
  {
    igt = igtg->ig_task;
    /* igt = KheIgSolverTaskToIgTask(igsv, KheIgTaskGroupTask(igtg)); */
    assigned_cost = KheIgTaskGroupCost(igtg, true, next_igtg, igsv) +
      KheIgTaskCost(igt, true);
    unassigned_cost = KheIgTaskGroupCost(igtg, false, next_igtg, igsv) +
      KheIgTaskCost(igt, false);
    res = min(assigned_cost, unassigned_cost);
    if( DEBUG64 && res >= KheCost(1, 0) )
      fprintf(stderr, "  %s assigned cost %.5f, unassigned cost %.5f: %.5f\n",
	KheIgTaskGroupId(igtg), KheCostShow(assigned_cost),
	KheCostShow(unassigned_cost), KheCostShow(res));
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIgTaskGroupAssignedResourceIndex(KHE_IG_TASK_GROUP igtg)          */
/*                                                                           */
/*  Return the instance index of tgtg's assigned resource, or -1 if none.    */
/*                                                                           */
/*****************************************************************************/

static int KheIgTaskGroupAssignedResourceIndex(KHE_IG_TASK_GROUP igtg)
{
  KHE_RESOURCE r;
  r = KheIgTaskGroupAssignedResource(igtg);
  return (r != NULL ?  KheResourceInstanceIndex(r) : -1);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIgTaskGroupTypedCmp(KHE_IG_TASK_GROUP igtg1,                      */
/*    KHE_IG_TASK_GROUP igtg2)                                               */
/*                                                                           */
/*  Typed comparison function for sorting an array of task groups.  After    */
/*  sorting, the groups have two key properties:                             */
/*                                                                           */
/*    (1) Self-finished groups appear at the end.  This is needed for        */
/*        dominance testing, which ignores self-finished groups.             */
/*                                                                           */
/*    (2) Non-self-finished groups appear in non-decreasing primary          */
/*        duration order.  This is assumed by the soln set trie.             */
/*                                                                           */
/*****************************************************************************/

static int KheIgTaskGroupTypedCmp(KHE_IG_TASK_GROUP igtg1,
  KHE_IG_TASK_GROUP igtg2)
{
  int cmp, count1, count2;  bool self_finished1, self_finished2;

  /* make sure that self-finished groups come at the end */
  self_finished1 = KheIgTaskGroupIsSelfFinishedNoOverhang(igtg1);
  self_finished2 = KheIgTaskGroupIsSelfFinishedNoOverhang(igtg2);
  cmp = (int) self_finished1 - (int) self_finished2;
  if( cmp != 0 )  return cmp;

  /* sort by increasing primary duration */
  cmp = igtg1->primary_durn - igtg2->primary_durn;
  if( cmp != 0 )  return cmp;

  /* sort by decreasing overhang */
  cmp = igtg2->overhang - igtg1->overhang;
  if( cmp != 0 )  return cmp;

  /* sort by increasing assigned resource index */
  cmp = KheIgTaskGroupAssignedResourceIndex(igtg1) -
    KheIgTaskGroupAssignedResourceIndex(igtg2);
  if( cmp != 0 )  return cmp;

  /* sort by the size of the task group's domain */
  count1 = KheResourceGroupResourceCount(KheIgTaskGroupDomainValue(igtg1));
  count2 = KheResourceGroupResourceCount(KheIgTaskGroupDomainValue(igtg2));
  return count1 - count2;

  /* finally, sort by increasing v(g) */
  /* return KheCostCmp(igtg1->task_cost, igtg2->task_cost); */
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
/*  bool KheIgTaskGroupSameClass(KHE_IG_TASK_GROUP igtg1,                    */
/*    KHE_IG_TASK_GROUP igtg2)                                               */
/*                                                                           */
/*  Return true if igtg1 and igtg2 belong to the same equivalence class.     */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskGroupSameClass(KHE_IG_TASK_GROUP igtg1,
  KHE_IG_TASK_GROUP igtg2, bool include_domain_in_test)
{
  KHE_RESOURCE_GROUP rg1, rg2;

  /* times and history */
  if( !KheIgTaskGroupIndexesEqual(igtg1, igtg2) )
    return false;

  /* domain */
  if( include_domain_in_test )
  {
    HnAssert(KheIgTaskGroupDomain(igtg1) != NULL,
      "KheIgTaskGroupSameClass internal error 1");
    HnAssert(KheIgTaskGroupDomain(igtg2) != NULL,
      "KheIgTaskGroupSameClass internal error 2");
    rg1 = KheIgTaskGroupDomainValue(igtg1);
    rg2 = KheIgTaskGroupDomainValue(igtg2);
    if( !KheResourceGroupEqual(rg1, rg2) )
      return false;
  }

  /* assignments */
  if( KheIgTaskGroupAssignedResource(igtg1) !=
      KheIgTaskGroupAssignedResource(igtg2) )
    return false;

  /* fixed non-assignments */
  /* ***
  if( KheIgTaskGroupHasFixedUnassigned(igtg1) !=
      KheIgTaskGroupHasFixedUnassigned(igtg2) )
    return false;
  *** */

  /* handle as assigned */
  if( igtg1->handle_as_assigned != igtg2->handle_as_assigned )
    return false;

  /* task cost v(g) */
  /* *** omitting this now
  if( igtg1->primary_durn == 1 )
  {
    if( igtg1->task_cost != igtg2->task_cost )
      return false;
  }
  *** */

  /* optionality */
  /* ***
  if( igtg1->optional != igtg2->optional )
    return false;
  *** */

  /* n(g) */
  /* ***
  if( igtg1->non_must_assign_cost != igtg2->non_must_assign_cost )
    return false;
  *** */

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
/*  char *KheIgTaskGroupId(KHE_IG_TASK_GROUP igtg)                           */
/*                                                                           */
/*  Return an Id for igtg.  This is the Id of igtg's first task.             */
/*                                                                           */
/*****************************************************************************/

static char *KheIgTaskGroupId(KHE_IG_TASK_GROUP igtg)
{
  do
  {
    if( KheIgTaskGroupTask(igtg) != NULL )
      return KheTaskId(KheIgTaskGroupTask(igtg));
    igtg = igtg->prev;
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
  KHE_TASK task1, task2;
  task1 = KheIgTaskGroupTask(igtg);
  if( task1 != NULL )
  {
    /* ordinary task group */
    task2 = (igtg->ig_task == NULL ? NULL : igtg->ig_task->task);
    fprintf(fp, "OrdinaryTaskGroup(%s : %s, ", KheTaskId(task1),
      task2 == NULL ? "NULL" : KheTaskId(task2));
    KheTaskGroupDomainDebug(KheIgTaskGroupDomain(igtg), 2, -1, stderr);
    fprintf(fp, ", primary_durn %d%s%s%s)",
      igtg->primary_durn, igtg->overhang > 0 ? ", overhang" : "",
      igtg->self_finished ? ", self_finished" : "",
      igtg->expand_used ? ", expand_used" : "");
  }
  else
  {
    /* history task group */
    fprintf(fp, "HistoryTaskGroup(%d, %s)", igtg->primary_durn,
      KheResourceId(KheIgTaskGroupAssignedResource(igtg)));
  }
#if DEBUG_REFERENCE_COUNT
  fprintf(fp, " refs %d", HaArrayCount(igtg->ref_count_info));
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgTaskGroupDebug(KHE_IG_TASK_GROUP igtg, int verbosity,          */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of igtg onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/
static void KheIgTaskGroupClassDebug(KHE_IG_TASK_GROUP_CLASS igtgc,
  KHE_IG_SOLVER igsv, int verbosity, int indent, FILE *fp);

static void KheIgTaskGroupDebug(KHE_IG_TASK_GROUP igtg, KHE_IG_SOLVER igsv,
  int verbosity, int indent, FILE *fp)
{
  if( indent < 0 )
    KheIgTaskGroupDebugHeader(igtg, fp);
  else
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheIgTaskGroupDebugHeader(igtg, fp);
    if( verbosity >= 3 )
    {
      fprintf(fp, "\n");
#if DEBUG_REFERENCE_COUNT
      if( HaArrayCount(igtg->ref_count_info) > 0 )
      {
	KHE_REF_COUNT_INFO rci;  int i;  KHE_IG_TASK igt;
	KHE_IG_TASK_GROUP igtg2;  KHE_IG_TASK_GROUP_CLASS igtgc;
	fprintf(fp, "%*srefs:\n", indent + 2, "");
	HaArrayForEach(igtg->ref_count_info, rci, i)
	{
	  fprintf(stderr, "%*sinfo[%d] = %d %p:\n", indent + 2, "", i,
	    rci.type, rci.referer);
	  switch( rci.type )
	  {
	    case KHE_REF_COUNT_TASK:

	      igt = (KHE_IG_TASK) rci.referer;
	      KheIgTaskDebug(igt, 2, indent + 2, fp);
	      break;

	    case KHE_REF_COUNT_TASK_GROUP:

	      igtg2 = (KHE_IG_TASK_GROUP) rci.referer;
	      KheIgTaskGroupDebug(igtg2, igsv, 2, indent + 2, fp);
	      break;

	    case KHE_REF_COUNT_TASK_GROUP_CLASS:

	      igtgc = (KHE_IG_TASK_GROUP_CLASS) rci.referer;
	      KheIgTaskGroupClassDebug(igtgc, igsv, 2, indent + 2, fp);
	      break;

	    case KHE_REF_COUNT_SOLN:

	      fprintf(fp, "%*ssoln\n", indent + 2, "");
	      break;

	    default:

	      HnAbort("KheIgTaskGroupDebug internal error");
	      break;
	  }
	}
      }
#endif
      if( igtg->prev != NULL )
      {
	fprintf(fp, "%*sprev:\n", indent + 2, "");
	KheIgTaskGroupDebug(igtg->prev, igsv, verbosity, indent + 2, fp);
      }
      fprintf(fp, "%*s]\n", indent, "");
    }
    else
      fprintf(fp, " ]\n");
  }
}


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
  }
  else
  {
    HaMake(res, igsv->arena);
    HaArrayInit(res->task_groups, igsv->arena);
    HaArrayInit(res->links, igsv->arena);
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

static void KheIgTaskGroupClassFree(KHE_IG_TASK_GROUP_CLASS igtgc,
  KHE_IG_SOLVER igsv)
{
  KHE_IG_TASK_GROUP igtg;  int i;
  HaArrayForEach(igtgc->task_groups, igtg, i)
    KheIgTaskGroupUnReference(igtg, KHE_REF_COUNT_TASK_GROUP_CLASS,
      (void *) igtgc, igsv, false);
  HaArrayClear(igtgc->task_groups);
  HaArrayAddLast(igsv->ig_task_group_class_free_list, igtgc);
}


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
  KheIgTaskGroupReference(igtg, KHE_REF_COUNT_TASK_GROUP_CLASS, (void *) igtgc);
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
  /* KHE_TASK_GROUPER_ENTRY entry; */
  /* struct khe_task_grouper_entry_rec new_entry; */
  igtg = HaArrayFirst(igtgc->task_groups);
  igt = HaArrayFirst(igtc->tasks);
  return KheIgTaskGroupIsExtendable(igtg, igt, igsv, NULL);
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
/*  bool KheIgTaskGroupHasZeroPrimaryDurn(KHE_IG_TASK_GROUP igtg)            */
/*                                                                           */
/*  Return true if the primary duration of igtg is 0.                        */
/*                                                                           */
/*****************************************************************************/

static bool KheIgTaskGroupHasZeroPrimaryDurn(KHE_IG_TASK_GROUP igtg)
{
  return igtg->primary_durn == 0;
}


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
  cmp = (int) KheIgTaskGroupHasZeroPrimaryDurn(igtg1) -
    (int) KheIgTaskGroupHasZeroPrimaryDurn(igtg2);
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
      KheIgTaskGroupDebug(igtg, igsv, verbosity, indent + 2, fp);
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
    res->domain = KheTaskGroupDomainMake(igsv->domain_finder, NULL,
      KheTaskDomain(igt->task));
    /* res->domain = KheTaskDomain(igt->task); */
  }
  else if( igtc == NULL )
  {
    igtg = HaArrayFirst(igtgc->task_groups);
    res->domain = KheIgTaskGroupDomain(igtg);
    /* res->domain = KheTaskGroupDomainValue(igtg->domain); */
  }
  else
  {
    igtg = HaArrayFirst(igtgc->task_groups);
    igt = HaArrayFirst(igtc->tasks);
    res->domain = KheTaskGroupDomainMake(igsv->domain_finder,
      KheIgTaskGroupDomain(igtg), KheTaskDomain(igt->task));
    /* ***
    igtgd = KheTaskGroupDomainMake(igsv->domain_finder, igtg->domain,
      KheTaskDomain(igt->task));
    res->domain = KheTaskGroupDomainValue(igtgd);
    *** */
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
	  in1 = KheIgTaskGroupInterval(igtg);
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
#if USE_DOMAIN_DOMINATES
  bool proper1, proper2;
  if( !KheTaskGroupDomainDominates(link12->domain, link11->domain, &proper1) )
    return false;
  if( !KheTaskGroupDomainDominates(link21->domain, link22->domain, &proper2) )
    return false;
  return proper1 || proper2;
#else
  int count12, count21, count11, count22;
  KHE_RESOURCE_GROUP domain11, domain22, domain12, domain21;

  domain11 = KheTaskGroupDomainValue(link11->domain);
  domain22 = KheTaskGroupDomainValue(link22->domain);
  domain12 = KheTaskGroupDomainValue(link12->domain);
  domain21 = KheTaskGroupDomainValue(link21->domain);

  /* return false if condition between link12 and link11 fails */
  if( !KheResourceGroupSubset(domain11, domain12) )
    return false;

  /* return false if condition between link21 and link22 fails */
  if( !KheResourceGroupSubset(domain22, domain21) )
    return false;

  /* success, so return true if at least one of the conditions is proper */
  count11 = KheResourceGroupResourceCount(domain11);
  count22 = KheResourceGroupResourceCount(domain22);
  count12 = KheResourceGroupResourceCount(domain12);
  count21 = KheResourceGroupResourceCount(domain21);
  return count12 > count11 || count21 > count22;
#endif
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
/* static int KheIgSolverRandom(KHE_IG_SOLVER igsv); */

static void KheIgSolnInitOtherFields(KHE_IG_SOLN igs, KHE_IG_SOLN prev_igs,
  KHE_IG_SOLVER igsv)
{
  igs->prev_igs = prev_igs;
  igs->cost = (prev_igs == NULL ? 0 : prev_igs->cost);
  igs->non_self_finished_count = -1;   /* initially undefined */
  /* igs->randomizer = KheIgSolverRandom(igsv); */
#if DEBUG_COMPATIBLE
  igs->compatible = false;
  igs->cc_index = igsv->cc_index;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnClearTaskGroups(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv)       */
/*                                                                           */
/*  Clear the task groups of igs.  Only non-initial ones are freed.          */
/*                                                                           */
/*****************************************************************************/

static void KheIgSolnClearTaskGroups(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv,
  bool debug)
{
  KHE_IG_TASK_GROUP igtg;  int i;
  HaArrayForEach(igs->task_groups, igtg, i)
    KheIgTaskGroupUnReference(igtg, KHE_REF_COUNT_SOLN, (void *) igs,
      igsv, debug);
    /* ***
    if( igtg->prev != NULL )
      KheIgTaskGroupFree(igtg, igsv);
    *** */
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
  KheIgSolnClearTaskGroups(igs, igsv, false);
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
  {
    HaArrayAddLast(res->task_groups, igtg);
    KheIgTaskGroupReference(igtg, KHE_REF_COUNT_SOLN, (void *) res);
    if( DEBUG66 )
    {
      int j;  KHE_IG_TASK_GROUP igtg2;
      for( j = 0;  j < i;  j++ )
      {
	igtg2 = HaArray(igs->task_groups, j);
	if( igtg2->ig_task != NULL && igtg->ig_task != NULL &&
	    igtg2->ig_task->task == igtg->ig_task->task )
	  HnAbort("KheIgSolnCopy: duplicate ig_task in soln");
      }
    }
  }
    /* HaArrayAddLast(res->task_groups, KheIgTaskGroupCopy(igtg, igsv)); */
  res->prev_igs = igs->prev_igs;
  res->cost = igs->cost;
  HnAssert(res->cost >= 0, "KheIgSolnCopy internal error (negative cost)");
  res->non_self_finished_count = igs->non_self_finished_count;
  /* res->randomizer = igs->randomizer; */
#if DEBUG_COMPATIBLE
  res->compatible = igs->compatible;
  res->cc_index = igs->cc_index;
#endif
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnFree(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv)                  */
/*                                                                           */
/*  Free igs:  add it to the free list in igsv.  Also free its task groups.  */
/*  But don't free any predecessor solution.                                 */
/*                                                                           */
/*****************************************************************************/

static void KheIgSolnFree(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv, bool debug)
{
  KheIgSolnClearTaskGroups(igs, igsv, debug);
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

/* *** correct but no longer used
static int KheIgSolnNonSelfFinishedGroupCount(KHE_IG_SOLN igs)
{
  int i;  KHE_IG_TASK_GROUP igtg;
  HaArrayForEachReverse(igs->task_groups, igtg, i)
    if( !KheIgTaskGroupIsSelfFinishedNoOverhang(igtg) )
      break;
  return i + 1;
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
      prev = dt->prev;
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
	  rg = KheIgTaskGroupDomainValue(dt);
	  if( KheIgTimeGroupFindBestLengthenerMTask(succ_igtg, rg,false,&igmt) )
	  {
	    task = KheMTaskTask(igmt->mtask,
	      HaArrayCount(igmt->included_tasks), &non_asst_cost, &asst_cost);
	    igt = KheIgMTaskAddTask(igmt, task, non_asst_cost, asst_cost, igsv);
	    if( DEBUG39 )
	      KheIgTaskDebug(igt, 2, 2, stderr);
	    res = true;
	  }

	  /* undersized, try increasing its length at the start */
	  k = i, dt2 = dt->prev;
	  while( dt2 != NULL )
	    k--, dt2 = dt2->prev;
	  if( k > 0 )
	  {
	    pred_igtg = HaArray(igsv->time_groups, k - 1);
	    /* rg = KheIgTaskGroupDomain(dt); */
	    if( KheIgTimeGroupFindBestLengthenerMTask(pred_igtg,rg,true,&igmt) )
	    {
	      task = KheMTaskTask(igmt->mtask,
		HaArrayCount(igmt->included_tasks), &non_asst_cost, &asst_cost);
	      igt = KheIgMTaskAddTask(igmt, task, non_asst_cost,asst_cost,igsv);
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
static void KheIgSolnDebug(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv,
  int verbosity, int indent, FILE *fp);

static bool KheIgSolnDominates(KHE_IG_SOLN igs1, KHE_IG_SOLN igs2,
  KHE_IG_SOLVER igsv, bool debug)
{
  KHE_IG_TASK_GROUP igtg1, igtg2;  int i, count1, count2;
  /* bool equal_so_far; */

  if( debug )
  {
    fprintf(stderr, "[ KheIgSolnDominates(igs1, igs2)\n");
    KheIgSolnDebugTimetable(igs1, igsv, 2, 2, stderr);
    KheIgSolnDebugTimetable(igs2, igsv, 2, 2, stderr);
  }

  /* for dominance, the cost of igs1 must not exceed the cost of igs2 */
  if( igs1->cost > igs2->cost )
  {
    if( debug )
      fprintf(stderr, "] returning false at cost:  igs1 %.5f > igs2 %.5f\n",
	KheCostShow(igs1->cost), KheCostShow(igs2->cost));
    return false;
  }
  /* equal_so_far = (igs1->cost == igs2->cost); */

  /* for dominance, equal numbers of non-self-finished groups must be present */
  count1 = igs1->non_self_finished_count;
  count2 = igs2->non_self_finished_count;
  /* ***
  count1 = KheIgSolnNonSelfFinishedGroupCount(igs1);
  count2 = KheIgSolnNonSelfFinishedGroupCount(igs2);
  *** */
  if( count1 != count2 )
  {
    if( debug )
      fprintf(stderr, "] returning false at group count:  %d != %d\n",
	count1, count2);
    return false;
  }

  /* for dominance, corresponding non-self-finished groups must dominate */
  for( i = 0;  i < count1;  i++ )
  {
    igtg1 = HaArray(igs1->task_groups, i);
    igtg2 = HaArray(igs2->task_groups, i);
    if( !KheIgTaskGroupDominates(igtg1, igtg2, igsv, /*&equal_so_far,*/ false) )
    {
      if( debug )
      {
	fprintf(stderr, "] returning false at task group %d, ", i);
        KheIgTaskGroupDominates(igtg1, igtg2, igsv, /* &equal_so_far, */ true);
      }
      return false;
    }
  }

  /* everything equal, return true */
  if( debug )
    fprintf(stderr, "] returning true at !equal_so_far\n");
  return true;

  /* if not equal_so_far, igs1 dominates */
  /* ***
  if( !equal_so_far )
  {
    if( debug )
      fprintf(stderr, "] returning true at !equal_so_far\n");
    return true;
  }
  *** */

  /* everything equal, use randomizer as last resort */
  /* ***
  if( debug )
    fprintf(stderr, "] returning %s at randomizer\n",
      bool_show(igs1->randomizer <= igs2->randomizer));
  return igs1->randomizer <= igs2->randomizer;
  *** */
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

  /* build the group, taking care to keep the mtask finder up to date */
  KheMTaskFinderTaskGrouperEntryMakeGroup(igsv->mtask_finder,
    last_igtg->tg_entry, igsv->soln_adjuster);
  /* ***
  KheTaskGrouperEntryMakeGroup(last_igtg->tg_entry, igsv->soln_adjuster);
  *** */

  /* break up the group, to prevent it from being grouped again */
  for( igtg = last_igtg;  igtg != NULL;  igtg = prev_igtg )
  {
    prev_igtg = igtg->prev;
    if( prev_igtg != NULL )
      KheIgTaskGroupUnReference(prev_igtg, KHE_REF_COUNT_TASK_GROUP,
	(void *) igtg, igsv, false);
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
      KheIgTaskGroupReference(igtg, KHE_REF_COUNT_SOLN, (void *) res);
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
  fprintf(fp, "Soln(%.5f, %d groups)", KheCostShow(igs->cost),
    HaArrayCount(igs->task_groups));
#if DEBUG_COMPATIBLE
  fprintf(fp, " cc_index %d", igs->cc_index);
#endif
  /* ***
  fprintf(fp, "Soln(%.5f, randomizer %d, %d groups)",
    KheCostShow(igs->cost), igs->randomizer, HaArrayCount(igs->task_groups));
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolnDebug(KHE_IG_SOLN igs, int verbosity, int indent,          */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of igs onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheIgSolnDebug(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv,
  int verbosity, int indent, FILE *fp)
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
      KheIgTaskGroupDebug(igtg, igsv, verbosity, indent + 2, fp);
    if( verbosity >= 4 && igs->prev_igs != NULL )
      KheIgSolnDebug(igs->prev_igs, igsv, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDebugTask(KHE_TASK task, KHE_IG_TASK_GROUP igtg,                 */
/*    KHE_IG_SOLVER igsv)                                                    */
/*                                                                           */
/*  Bug-finding function.                                                    */
/*                                                                           */
/*****************************************************************************/

static void KheDebugTask(KHE_TASK task, KHE_IG_TASK_GROUP igtg,
  KHE_IG_SOLVER igsv)
{
  if( DEBUG68(task) )
  {
    fprintf(stderr, "  task %s lies in task group %p:\n",
      KheTaskId(task), (void *) igtg);
    KheIgTaskGroupDebug(igtg, igsv, 2, 2, stderr);
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
  KHE_GROUPED_TASKS_DISPLAY gtd;  KHE_INTERVAL in;  KHE_TASK task;
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
      if( !KheIgTaskGroupIsPredecessor(igtg, next_igs) )
      {
	KheGroupedTasksDisplayGroupBegin(gtd,
	  KheIgTaskGroupHasZeroPrimaryDurn(igtg), igtg->primary_durn, i);
	do
	{
	  task = KheIgTaskGroupTask(igtg);
	  if( task != NULL )
	  {
	    /* ordinary task group; add one task to the display */
	    KheGroupedTasksDisplayGroupAddTask(gtd, task);
	    if( DEBUG67 )
              KheDebugTask(task, igtg, igsv);
	  }
	  else
	  {
	    /* history task group; add history to the display */
	    in = KheIgTaskGroupInterval(igtg);
	    KheGroupedTasksDisplayGroupAddHistory(gtd,
	      KheIgTaskGroupAssignedResource(igtg), KheIntervalLength(in));
	  }
	  igtg = igtg->prev;
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
/*  Submodule "KHE_IG_SOLN_SET_TRIE"                                         */
/*                                                                           */
/*****************************************************************************/

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

/* *** old version
static KHE_IG_SOLN_SET_TRIE KheIgSolnSetTrieRetrieve(
  KHE_IG_SOLN_SET_TRIE *trie, KHE_IG_SOLN igs, KHE_IG_SOLVER igsv)
{
  int non_self_finished_count, i;  KHE_IG_SOLN_SET_TRIE res;
  KHE_IG_TASK_GROUP igtg;

  ** make the root node if required **
  if( *trie == NULL )
    *trie = KheIgSolnSetTrieMake(igsv);

  ** extend by KheIgSolnNonSelfFinishedGroupCount **
  non_self_finished_count = KheIgSolnNonSelfFinishedGroupCount(igs);
  res = KheIgSolnSetTrieExtend(*trie, non_self_finished_count, igsv);

  ** extend by primary durns **
  for( i = 0;  i < non_self_finished_count;  i++ )
  {
    igtg = HaArray(igs->task_groups, i);
    res = KheIgSolnSetTrieExtend(res, igtg->primary_durn, igsv);
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheNextRun(KHE_IG_SOLN igs, int primary_durn, int *index)            */
/*                                                                           */
/*  Search the task groups of igs starting from position *index for          */
/*  an initial run of task groups with the given primary duration.           */
/*  The initial value of *index is assumed to be not off the end.            */
/*                                                                           */
/*  Return the length of the run found, and also reset *index to the         */
/*  first position beyond the end of that run (possibly off the end).        */
/*  If there is no such run, return 0 with *index unchanged.                 */
/*                                                                           */
/*****************************************************************************/

static int KheNextRun(KHE_IG_SOLN igs, KHE_IG_SOLVER igsv,
  int primary_durn, int *index)
{
  KHE_IG_TASK_GROUP igtg;  int res;
  res = 0;
  do
  {
    igtg = HaArray(igs->task_groups, *index);
    if( DEBUG60 && igtg->primary_durn < primary_durn )
    {
      fprintf(stderr, "  KheNextRun failing at index %d:\n", *index);
      KheIgSolnDebug(igs, igsv, 2, 2, stderr);
    }
    HnAssert(igtg->primary_durn >= primary_durn, "KheNextRun internal error"
      " (failed %d >= %d)", igtg->primary_durn, primary_durn);
    if( igtg->primary_durn > primary_durn )
      break;
    res++;
    (*index)++;
  } while( *index < igs->non_self_finished_count );
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
  int i, primary_durn, val;  KHE_IG_SOLN_SET_TRIE res;

  /* make the root node if required */
  if( *trie == NULL )
    *trie = KheIgSolnSetTrieMake(igsv);

  /* extend by runs of equal primary durns */
  res = *trie;
  primary_durn = 0;
  i = 0;
  for( primary_durn = 0;  i < igs->non_self_finished_count;  primary_durn++ )
  {
    val = KheNextRun(igs, igsv, primary_durn, &i);
    res = KheIgSolnSetTrieExtend(res, val, igsv);
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

static void KheIgSolnSetTrieAddSoln(KHE_IG_SOLN_SET_TRIE *trie,
  KHE_IG_SOLN igs, KHE_IG_SOLVER igsv)
{
  KHE_IG_SOLN_SET_TRIE node;  KHE_IG_SOLN igs2;  int i;

  /* find the node to insert into, possibly updating *trie */
  node = KheIgSolnSetTrieRetrieve(trie, igs, igsv);

  /* if igs is dominated, free igs and return without changing anything else */
  HaArrayForEach(node->solns, igs2, i)
    if( KheIgSolnDominates(igs2, igs, igsv, false) )
    {
#if DEBUG_COMPATIBLE
      if( DEBUG43 && igs->compatible )
      {
	fprintf(stderr, "  new compatible solution dominated by this %s "
	  "solution:\n", igs2->compatible ? "compatible" : "incompatible");
	KheIgSolnDebugTimetable(igs2, igsv, 2, 2, stderr);
      }
#endif
      KheIgSolnFree(igs, igsv, false);
      if( DEBUG24 )
	fprintf(stderr, "] KheIgSolnSetAddSoln returning "
	  "(dominated by soln %d)\n", i);
      return;
    }

  /* igs is not dominated, so free anything dominated by it */
  HaArrayForEach(node->solns, igs2, i)
    if( KheIgSolnDominates(igs, igs2, igsv, false) )
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
      KheIgSolnFree(igs2, igsv, false);
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

static void KheIgSolnSetTrieDoDebug(KHE_IG_SOLN_SET_TRIE trie, int depth,
  int index, int verbosity, int indent, FILE *fp)
{
  KHE_IG_SOLN_SET_TRIE child_trie;  int i;
  if( trie != NULL )
  {
    if( index >= 0 )
      fprintf(fp, "%*s%d:%d[", indent, "", depth, index);
    else
      fprintf(fp, "%*s[", indent, "");
    if( HaArrayCount(trie->solns) > 0 )
      fprintf(fp, " %d", HaArrayCount(trie->solns));
    if( HaArrayCount(trie->children) == 0 )
      fprintf(fp, " ]\n");
    else
    {
      fprintf(fp, "\n");
      HaArrayForEach(trie->children, child_trie, i)
        KheIgSolnSetTrieDoDebug(child_trie, depth + 1, i,
	  verbosity, indent + 2, fp);
      fprintf(fp, "%*s]\n", indent, "");
    }
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

static void KheIgSolnSetTrieDebug(KHE_IG_SOLN_SET_TRIE trie, int verbosity,
  int indent, FILE *fp)
{
  KheIgSolnSetTrieDoDebug(trie, -1, -1, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnSetTrieDoDebugOneSolnSet(KHE_IG_SOLN_SET_TRIE trie,          */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Find a node of trie with a moderate number of solutions in it, and       */
/*  print those solutions.                                                   */
/*                                                                           */
/*****************************************************************************/

static void KheSolnSetTrieDoDebugOneSolnSet(KHE_IG_SOLN_SET_TRIE trie,
  KHE_IG_SOLVER igsv, bool *done, int verbosity, int indent, FILE *fp)
{
  KHE_IG_SOLN igs, igs1, igs2;  int i, offset, count;
  KHE_IG_SOLN_SET_TRIE child_trie;
  if( trie != NULL )
  {
    if( HaArrayCount(trie->solns) >= 10 && HaArrayCount(trie->solns) <= 20 )
    {
      /* this node will do; debug it and return */
      fprintf(fp, "%*s[ KheSolnSetTrieDebugOneSolnSet (%d solns):\n",
	indent, "", HaArrayCount(trie->solns));
      HaArrayForEach(trie->solns, igs, i)
	KheIgSolnDebugTimetable(igs, igsv, verbosity, indent + 2, fp);
      *done = true;
      igs1 = HaArray(trie->solns, 3);
      igs2 = HaArray(trie->solns, 0);
      KheIgSolnDominates(igs1, igs2, igsv, true);
      fprintf(fp, "%*s]\n", indent, "");
    }

    offset = 19;
    count = HaArrayCount(trie->children);
    for( i = 0;  !*done && i < count;  i++ )
    {
      child_trie = HaArray(trie->children, (i + offset) % count);
      KheSolnSetTrieDoDebugOneSolnSet(child_trie, igsv, done,
	verbosity, indent, fp);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnSetTrieDebugOneSolnSet(KHE_IG_SOLN_SET_TRIE trie,            */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Find a node of trie with a moderate number of solutions in it, and       */
/*  print those solutions.                                                   */
/*                                                                           */
/*****************************************************************************/

static void KheSolnSetTrieDebugOneSolnSet(KHE_IG_SOLN_SET_TRIE trie,
  KHE_IG_SOLVER igsv, int verbosity, int indent, FILE *fp)
{
  bool done;
  done = false;
  KheSolnSetTrieDoDebugOneSolnSet(trie, igsv, &done, verbosity, indent, fp);
}


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
  HaArrayInit(res->cost_stack, igsv->arena);
  res->next_igtg = NULL;
  HaArrayInit(res->all_links, igsv->arena);
  return res;
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
	bool_show(KheIgTaskGroupIsSelfFinishedNoOverhang(igtg)));
    if( !KheIgTaskGroupIsSelfFinishedNoOverhang(igtg) )
      break;
    cost = KheTaskGroupFinishedCost(igtg, next_igtg, igsv);
    /* cost = KheIgTaskGroupCost(igtg, next_igtg, igsv); */
    HnAssert(cost >= 0, "KheIgExpanderFinishedCostAdd internal error 1");
    if( DEBUG49 && cost > 0 )
    {
      fprintf(stderr, "  this self-finished igtg has cost %.5f:\n",
	KheCostShow(cost));
      KheIgTaskGroupDebug(igtg, igsv, 2, 2, stderr);
    }
    igs->cost += cost;
  }

  /* store the number of non-self-finished groups */
  igs->non_self_finished_count = i + 1;

  /* add costs of groups that ended previously i.e. (a) groups */
  if( igs->prev_igs != NULL )
  {
    if( next_igtg->index == 0 )
      igtg_prev = NULL;
    else
      igtg_prev = HaArray(igsv->time_groups, next_igtg->index - 1);
    HaArrayForEach(igs->prev_igs->task_groups, prev_igtg, i)
      if( !prev_igtg->expand_used &&
	  !KheIgTaskGroupIsSelfFinishedNoOverhang(prev_igtg) )
      {
	cost = KheTaskGroupFinishedCost(prev_igtg, igtg_prev, igsv);
	/* cost = KheIgTaskGroupCost(prev_igtg, igtg_prev, igsv); */
	HnAssert(cost >= 0, "KheIgExpanderFinishedCostAdd internal error 2");
	if( DEBUG49 && cost > 0 )
	{
	  fprintf(stderr, "  this non-self-finished igtg has cost %.5f:\n",
	    KheCostShow(cost));
	  KheIgTaskGroupDebug(prev_igtg, igsv, 2, 2, stderr);
	}
	igs->cost += cost;
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
  HaArrayForEach(ige->prev_task_group_classes, igtgc, i)
    if( KheIgTaskGroupClassHasNonZeroOverhang(igtgc) )
      HaArrayForEach(igtgc->task_groups, igtg, j)
      {
	KheIgTaskGroupCheck(igtg);
	new_igtg = KheIgTaskGroupMakeDummy(igtg, ige->solver);
	HaArrayAddLast(ige->next_igs->task_groups, new_igtg);
	KheIgTaskGroupReference(new_igtg, KHE_REF_COUNT_SOLN,
	  (void *) ige->next_igs);
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
    if( KheIgTaskGroupAssignedResource(*igtg) == r )
      return true;
  }
  return *igtgc = NULL, *igtg = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheInitialTaskCostDelta(KHE_IG_TASK_GROUP igtg, KHE_IG_TASK igt)*/
/*                                                                           */
/*  Return the change in task cost when initial task group igtg containing   */
/*  task igt is added to the current solution.                               */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheInitialTaskCostDelta(KHE_IG_TASK_GROUP igtg, KHE_IG_TASK igt)
{
  return igtg->handle_as_assigned ? KheIgTaskCost(igt, true) : 0;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheSuccessorTaskCostDelta(KHE_IG_TASK_GROUP prev_igtg,          */
/*    KHE_IG_TASK igt, KHE_IG_SOLVER igsv)                                   */
/*                                                                           */
/*  Return the change in task cost when a new task group consisting of       */
/*  pref_igtg followed by igt is added to the current solution.              */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheSuccessorTaskCostDelta(KHE_IG_TASK_GROUP prev_igtg,
  KHE_IG_TASK igt, KHE_IG_SOLVER igsv)
{
  /* KHE_IG_TASK prev_igt; */
  if( prev_igtg->handle_as_assigned )
    return KheIgTaskCost(igt, true);
  else
  {
    /* ***
    prev_igt = prev_igtg->ig_task;
    prev_igt = KheIgSolverTaskToIgTask(igsv, KheIgTaskGroupTask(prev_igtg));
    *** */
    return KheIgTaskCost(prev_igtg->ig_task, true)
      /* - KheIgTaskCost(prev_igtg->ig_task, false) still to do */
      + KheIgTaskCost(igt, true);
  }
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
  int i;  KHE_IG_SOLVER igsv;  KHE_TASK_GROUPER_ENTRY new_entry;
  /* struct khe_task_grouper_entry_rec new_entry; */
  KHE_IG_TASK_CLASS igtc;  KHE_IG_TASK_GROUP_CLASS igtgc;  KHE_COST cost_delta;
  igsv = ige->solver;
  HaArrayForEach(ige->next_igtg->task_classes, igtc, i)
  {
    igt = HaArrayFirst(igtc->tasks);
    r = KheTaskAsstResource(igt->task);
    if( r != NULL && KheIgExpanderHasAssignedTaskGroup(ige, r, &igtgc, &igtg) )
    {
      KheTaskGrouperEntryMakeTask(igsv->task_grouper, igtg->tg_entry,
	igt->task, false, &new_entry);
      /* ***
      KheTaskGrouperEntryAddTask(( KHE_TASK_GROUPER_ENTRY) igtg, igt->task,
	igsv->days_frame, igsv->domain_finder, &new_entry);
      *** */
      next_igtg = KheIgTaskGroupMakeSuccessor(igtg, igt, new_entry, igsv);
      cost_delta = KheSuccessorTaskCostDelta(igtg, igt, igsv);
      ige->next_igs->cost += cost_delta;
      /* HaArrayAddLast(ige->cost_stack, cost_delta); never removed */
      HaArrayAddLast(ige->next_igs->task_groups, next_igtg);
      KheIgTaskGroupReference(next_igtg, KHE_REF_COUNT_SOLN,
	(void *) ige->next_igs);
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
/*  This also adds the task cost of the new group to the solution cost,      */
/*  and pushes that cost onto the expander's cost stack so that it can be    */
/*  subtracted from the solution cost later, when the assignment is undone.  */
/*                                                                           */
/*****************************************************************************/

static void KheIgExpanderMakeOneAssignment(KHE_IG_EXPANDER ige,
  KHE_IG_TASK_GROUP_CLASS igtgc, KHE_IG_TASK_CLASS igtc)
{
  /* struct khe_task_grouper_entry_rec new_entry; */
  KHE_IG_TASK_GROUP igtg;  KHE_TASK_GROUPER_ENTRY new_entry;
  KHE_IG_TASK igt;  KHE_IG_SOLVER igsv;  KHE_IG_TASK_GROUP next_igtg;
  KHE_COST cost_delta;

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
    if( !KheTaskGrouperEntryMakeTask(igsv->task_grouper, igtg->tg_entry,
	  igt->task, true, &new_entry) )
    {
      if( DEBUG51 )
      {
	fprintf(stderr, "  KheIgExpanderMakeOneAssignment failing on link:\n");
	KheIgTaskGroupDebug(igtg, igsv, 2, 2, stderr);
	KheIgTaskDebug(igt, 2, 2, stderr);
      }
      HnAbort("KheIgExpanderMakeOneAssignment internal error 3");
    }
    next_igtg = KheIgTaskGroupMakeSuccessor(igtg, igt, new_entry, igsv);
    cost_delta = KheSuccessorTaskCostDelta(igtg, igt, igsv);
  }
  else
  {
    next_igtg = igt->initial_task_group;
    cost_delta = KheInitialTaskCostDelta(next_igtg, igt);
  }
  ige->next_igs->cost += cost_delta;
  /* ***
  if( DEBUG64 && ige->next_igs->cost >= KheCost(1, 0) )
    fprintf(stderr, "  after adding %s %s, cost %.5f\n", KheIgTaskId(igt),
      igtgc != NULL ? "non-initial" : "initial",
      KheCostShow(ige->next_igs->cost));
  *** */
  HaArrayAddLast(ige->cost_stack, cost_delta);
  HaArrayAddLast(ige->next_igs->task_groups, next_igtg);
  KheIgTaskGroupReference(next_igtg, KHE_REF_COUNT_SOLN,
    (void *) ige->next_igs);
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
  KHE_IG_TASK_GROUP next_igtg;  KHE_COST cost_delta;

  /* subtract the cost of the assignmnt */
  cost_delta = HaArrayLastAndDelete(ige->cost_stack);
  ige->next_igs->cost -= cost_delta;

  /* update ige->next_igs */
  igsv = ige->solver;
  next_igtg = HaArrayLastAndDelete(ige->next_igs->task_groups);
  KheIgTaskGroupUnReference(next_igtg, KHE_REF_COUNT_SOLN,
    (void *) ige->next_igs, igsv, false);
  /* ***
  if( next_igtg->prev != NULL )
    KheIgTaskGroupFree(next_igtg, igsv);
  *** */

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
  ige->expand_limit--;
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
	  KheIgTaskGroupDebug(igtg, ige->solver, 2, 4, stderr);
  }
  if( ige->expand_limit <= 0 )
  {
    /* give up if expand limit reached */
    return;
  }
  else if( igtc_index >= HaArrayCount(ige->next_igtg->task_classes) )
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
  if( ige->expand_limit <= 0 )
  {
    /* give up if expand limit reached */
    return;
  }
  else if( wanted_tasks <= 0 )
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
/*  bool KheIgExpanderNextUndersizedTaskGroup(KHE_IG_EXPANDER ige,           */
/*    int *igtgc_index, KHE_IG_TASK_GROUP_CLASS *igtgc)                      */
/*                                                                           */
/*  Find the first undersized task group class at or after *igtgc_index.     */
/*  If found, reset *igtgc_index to its new index, set *igtgc to the         */
/*  task group class, and return true.  Otherwise return false.              */
/*                                                                           */
/*****************************************************************************/

static bool KheIgExpanderNextUndersizedTaskGroup(KHE_IG_EXPANDER ige,
  int *igtgc_index, KHE_IG_TASK_GROUP_CLASS *igtgc)
{
  while( *igtgc_index < HaArrayCount(ige->prev_task_group_classes) )
  {
    *igtgc = HaArray(ige->prev_task_group_classes, *igtgc_index);
    if( KheIgTaskGroupClassUndersized(*igtgc, ige->solver) )
      return true;
    (*igtgc_index)++;
  }
  return *igtgc = NULL, false;
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
/*****************************************************************************/

static void KheIgExpanderAssignUndersizedTaskGroups(KHE_IG_EXPANDER ige,
  int igtgc_index)
{
  KHE_IG_TASK_GROUP_CLASS igtgc;  int wanted_tasks, avail_tasks;
  if( ige->expand_limit <= 0 )
  {
    /* give up if expand limit reached */
    return;
  }
  else if( ige->solver->ig_complete ||
      !KheIgExpanderNextUndersizedTaskGroup(ige, &igtgc_index, &igtgc) )
  {
    /* switch to assigning unused tasks */
    KheIgExpanderAssignRemainingTasks(ige, 0);
  }
  else
  {
    /* igtgc is the next undersized task group class */
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
/*  void KheIgExpanderExpandBegin(KHE_IG_EXPANDER ige,                       */
/*    KHE_IG_SOLN prev_igs, KHE_IG_TIME_GROUP next_igtg, int expand_limit)   */
/*                                                                           */
/*  Set up ige so that it is ready to expand prev_igs, with the resulting    */
/*  new solutions being added to time group next_igtg.                       */
/*                                                                           */
/*****************************************************************************/

static void KheIgExpanderExpandBegin(KHE_IG_EXPANDER ige,
  KHE_IG_SOLN prev_igs, KHE_IG_TIME_GROUP next_igtg, int expand_limit)
{
  int i, j, class_seq;  KHE_IG_TASK_GROUP igtg;  KHE_IG_SOLVER igsv;
  KHE_IG_TASK_GROUP_CLASS igtgc, igtgc1;  KHE_IG_TASK_CLASS igtc;
  KHE_IG_LINK link1, link2;

  /* the basics (ige->solver remains constant) */
  igsv = ige->solver;
  ige->prev_igs = prev_igs;
  KheIgSolnReset(ige->next_igs, prev_igs, igsv);
  ige->next_igtg = next_igtg;
  ige->expand_limit = expand_limit;

  /* stuff which is temporary or derived from the basics */
  /* reset next_igtg's task classes for the new expansion */
  HaArrayForEach(next_igtg->task_classes, igtc, i)
    KheIgTaskClassExpandReset(igtc, igsv);

  /* set ige->prev_task_group_classes to the task group classes for this exp. */
  HnAssert(HaArrayCount(ige->prev_task_group_classes) == 0,
    "KheIgExpanderExpandBegin internal error 1");
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
      KheIgTaskGroupCheck(igtg);
    }
  }

  /* set links between non-NULL task group classes and non-NULL task classes */
  HnAssert(HaArrayCount(ige->all_links) == 0,
    "KheIgExpanderExpandBegin internal error 2");
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

  /* the cost stack should be empty */
  HnAssert(HaArrayCount(ige->cost_stack) == 0,
    "KheIgExpanderExpandBegin internal error 3");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderExpandEnd(KHE_IG_EXPANDER ige)                         */
/*                                                                           */
/*  Wrap up one expansion, freeing things that are no longer needed.         */
/*                                                                           */
/*****************************************************************************/

static void KheIgExpanderExpandEnd(KHE_IG_EXPANDER ige)
{
  KHE_IG_TASK_GROUP_CLASS igtgc;  int i;  KHE_IG_SOLVER igsv;

  /* clear out the task groups (needed for reference counting) */
  igsv = ige->solver;
  KheIgSolnClearTaskGroups(ige->next_igs, igsv, false);

  /* clear out the task group classes */
  HaArrayForEach(ige->prev_task_group_classes, igtgc, i)
    KheIgTaskGroupClassFree(igtgc, igsv);
  /* ***
  HaArrayAppend(igsv->ig_task_group_class_free_list,
    ige->prev_task_group_classes, i);
  *** */
  HaArrayClear(ige->prev_task_group_classes);

  /* clear out the links */
  HaArrayAppend(igsv->ig_link_free_list, ige->all_links, i);
  HaArrayClear(ige->all_links);

  /* the cost stack should be empty */
  HnAssert(HaArrayCount(ige->cost_stack) == 0,
    "KheIgExpanderExpandEnd internal error");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgExpanderExpand(KHE_IG_EXPANDER ige, KHE_IG_SOLN prev_igs,      */
/*    KHE_IG_TIME_GROUP next_igtg, int expand_limit, int debug_index1,       */
/*    int debug_index2)                                                      */
/*                                                                           */
/*  Using ige, extend prev_igs in all possible ways into next_igtg.          */
/*  But generate at most expand_limit solutions.                             */
/*                                                                           */
/*****************************************************************************/

static void KheIgExpanderExpand(KHE_IG_EXPANDER ige, KHE_IG_SOLN prev_igs,
  KHE_IG_TIME_GROUP next_igtg, int expand_limit, int debug_index1,
  int debug_index2)
{
  /* reset ige for expanding prev_igs into next_igtg */
  KheIgExpanderExpandBegin(ige, prev_igs, next_igtg, expand_limit);
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

  /* assign undersized task groups, then remaining unused tasks */
  KheIgExpanderAssignUndersizedTaskGroups(ige, 0);
  KheIgExpanderExpandEnd(ige);
  if( DEBUG21 || DEBUG41(next_igtg, debug_index1) || DEBUG56(next_igtg) )
    fprintf(stderr, "] KheIgExpanderExpand returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_IG_SOLVER"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_SOLVER KheIgSolverMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,       */
/*    KHE_OPTIONS options, int ig_min, int ig_range, bool ig_complete,       */
/*    int ig_max_keep, int ig_max_beam, KHE_MTASK_FINDER mtf,                */
/*    KHE_TASK_GROUP_DOMAIN_FINDER tgdf, KHE_SOLN_ADJUSTER sa, HA_ARENA a)   */
/*                                                                           */
/*  Make a ig solver object with these attributes.                           */
/*                                                                           */
/*****************************************************************************/

static KHE_IG_SOLVER KheIgSolverMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, int ig_min, int ig_range, bool ig_two_three,
  bool ig_complete, int ig_max_keep, int ig_max_beam, KHE_MTASK_FINDER mtf,
  KHE_TASK_GROUP_DOMAIN_FINDER tgdf, KHE_SOLN_ADJUSTER sa, HA_ARENA a)
{
  KHE_IG_SOLVER res;  int demand, supply;  KHE_COST task_cost, resource_cost;

  /* free lists */
  HaMake(res, a);
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

  /* fields defined (and mostly constant) throughout the solve */
  res->arena = a;
  res->soln = soln;
  res->resource_type = rt;
  res->options = options;
  res->ig_min = ig_min;
  res->ig_range = ig_range;
  res->ig_two_three = ig_two_three;
  res->ig_complete = ig_complete;
  res->ig_max_keep = ig_max_keep;
  res->ig_max_beam = ig_max_beam;
  res->mtask_finder = mtf;
  res->days_frame = KheMTaskFinderDaysFrame(mtf);
  res->domain_finder = tgdf;
  /* res->domain_finder = KheTaskGrou pDomainFinderMake(soln, a); */
  res->task_grouper = KheTaskGrouperMake(soln, res->days_frame,
    res->domain_finder, a);
  if( DEBUG58 )
    fprintf(stderr, "  KheIgSolverMake making domain finder %p\n",
      (void *) res->domain_finder);
  res->soln_adjuster = sa;
  KheResourceDemandExceedsSupply(soln, rt, &demand, &supply, &task_cost,
    &resource_cost);
  if( DEBUG9 )
    fprintf(stderr, "  KheResourceDemandExceedsSupply(soln, %s): "
      "demand %d, supply %d, task_cost %.5f, resource_cost %.5f\n",
      KheResourceTypeId(rt), demand, supply, KheCostShow(task_cost),
      KheCostShow(resource_cost));
  res->marginal_cost = resource_cost;
  res->candidate_ccf = KheConstraintClassFinderMake(rt, res->days_frame, a);
  res->busy_days_ccf = KheConstraintClassFinderMake(rt, res->days_frame, a);
  res->complete_weekends_ccf = KheConstraintClassFinderMake(rt,
    res->days_frame, a);
  KheConstraintClassFinderAddCompleteWeekendsConstraints(
    res->complete_weekends_ccf, true);
  res->groups_count = 0;

  /* fields defined when solving one constraint class */
  res->curr_class = NULL;
  res->min_limit = -1;
  res->max_limit = -1;
  res->soln_seq_num = 0;
  HaArrayInit(res->busy_days_classes, a);
  HaArrayInit(res->time_groups, a);
  /* HaArrayInit(res->included_tasks, a); */
#if DEBUG_COMPATIBLE
  res->cc_index = 0;
#endif

  /* scratch fields */
  res->expander = KheIgExpanderMake(res);
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
/*  bool KheMTaskIsAdmissiblePrimary(KHE_MTASK mt, KHE_IG_SOLVER igsv,       */
/*    KHE_PLACEMENT *pl, int *primary_durn)                                  */
/*                                                                           */
/*  Return true if mt's times mean that its tasks are admissible primary     */
/*  tasks.                                                                   */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskIsAdmissiblePrimary(KHE_MTASK mt, KHE_IG_SOLVER igsv,
  KHE_PLACEMENT *pl, int *primary_durn)
{
  return KheMTaskIsAdmissible(mt, igsv, pl, primary_durn) && *primary_durn > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIgSolverAddAdmissiblePrimaryMTasks(KHE_IG_SOLVER igsv)           */
/*                                                                           */
/*  Add the admissible primary mtasks for solving igsv->class to igsv's      */
/*  time groups.  Also add these mtasks' primary must-assign tasks.          */
/*                                                                           */
/*****************************************************************************/

static void KheIgSolverAddAdmissiblePrimaryMTasks(KHE_IG_SOLVER igsv)
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
      fprintf(stderr, "  KheIgSolverAddAdmissiblePrimaryMTasks considering:\n");
      KheMTaskDebug(mt, 2, 2, stderr);
    }
    if( KheMTaskIsAdmissiblePrimary(mt, igsv, &pl, &primary_durn) )
    {
      igmt = KheIgMTaskMake(mt, primary_durn, pl, igsv);
      included_tasks_count = KheMTaskAssignedTaskCount(mt) +
	KheMTaskNeedsAssignmentTaskCount(mt);
      for( j = 0;  j < included_tasks_count;  j++ )
      {
	task = KheMTaskTask(mt, j, &non_asst_cost, &asst_cost);
	HnAssert(KheIgTaskIsMustAssign(task, non_asst_cost, asst_cost),
	  "KheIgSolverAddAdmissiblePrimaryMTasks internal error");
	KheIgMTaskAddTask(igmt, task, non_asst_cost, asst_cost, igsv);
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
    KheIgMTaskAddTask(igmt, task, non_asst_cost, asst_cost, igsv);
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

/* *** OK but not currently used
static int KheIgSolverRandom(KHE_IG_SOLVER igsv)
{
  int num, res;
  num = igsv->soln_seq_num++;
  res = (num * num * KheSolnDiversifier(igsv->soln)) % 59;
  return (res < 0 ? - res : res);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_IG_TASK KheIgSolverTaskToIgTask(KHE_IG_SOLVER igsv, KHE_TASK task)   */
/*                                                                           */
/*  Find the ig task corresponding to task.  This must exist.                */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
static KHE_IG_TASK KheIgSolverTaskToIgTask(KHE_IG_SOLVER igsv, KHE_TASK task)
{
  int index;  KHE_IG_TASK res;
  index = KheTaskSolnIndex(task);
  res = HaArray(igsv->included_tasks, index);
  HnAssert(res != NULL, "KheIgSolverTaskToIgTask internal error");
  return res;
}
*** */


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

      return KheIgTaskGroupIsAssignedResourceInOtherSoln(igtg->prev, r,
	other_soln);

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
	if( !KheTaskGrouperEntryAddTaskUnchecked(igtg->tg_entry,
	    igt->task, igsv->days_frame, igsv->domain_finder, &new_entry) )
	  HnAbort("KheIgSolverExpandOther internal error");
	igtg->expand_used = true;
	igtg = KheIgTaskGroupMakeSuccessor(igtg, igt, &new_entry, igsv);
	if( DEBUG45 )
	{
	  fprintf(stderr, "  adding successor task group: ");
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
	  fprintf(stderr, "  adding initial task group: ");
	  KheIgTaskGroupDebug(igtg, 2, -1, stderr);
	  fprintf(stderr, " \n");  /* the space prevents code merging */
	}
      }
      HaArrayAddLast(res->task_groups, igtg);
      KheIgTaskGroupReference(igtg, KHE_REF_COUNT_SOLN, (void *) res);
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
  KHE_IG_SOLN prev_igs;  int expand_limit;

  if( DEBUG19 || DEBUG34 || DEBUG26 || DEBUG33(i) )
    fprintf(stderr, "[ KheIgSolverDoSolve(cc_index %d, %d time groups)\n",
      igsv->cc_index, HaArrayCount(igsv->time_groups));

  /* clear out any previous solutions and reset the time group tasks */
  HaArrayForEach(igsv->time_groups, next_igtg, i)
  {
    KheIgTimeGroupDeleteSolns(next_igtg, igsv, false);
    KheIgTimeGroupSetTaskClasses(next_igtg, igsv);
  }

  /* do the solve */
  prev_igtg = NULL;
  expand_limit = INT_MAX;
  HaArrayForEach(igsv->time_groups, next_igtg, i)
  {
    if( DEBUG19 || DEBUG34 || DEBUG26 || DEBUG33(i) )
    {
      fprintf(stderr, "  KheIgSolverDoSolve starting to solve for %s",
	KheIgTimeGroupId(next_igtg));
      if( expand_limit < INT_MAX )
	fprintf(stderr, " (expand_limit %d)", expand_limit);
      fprintf(stderr, "\n");
    }
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
      KheIgExpanderExpand(igsv->expander, prev_igs, next_igtg, expand_limit,
	-1, -1);
      if( DEBUG19 )
	fprintf(stderr, "  KheIgSolverDoSolve finished first expand\n");
    }
    else
      HaArrayForEach(prev_igtg->kept_solns, prev_igs, j)
	KheIgExpanderExpand(igsv->expander, prev_igs, next_igtg,
	  expand_limit, j, HaArrayCount(prev_igtg->kept_solns));
    HnAssert(HaArrayCount(next_igtg->kept_solns) == 0,
      "KheIgSolverDoSolve internal error 2");
    if( DEBUG59 )
      KheIgSolnSetTrieDebug(next_igtg->soln_set_trie, 2, 2, stderr);
    if( DEBUG61(i) )
      KheSolnSetTrieDebugOneSolnSet(next_igtg->soln_set_trie, igsv,
	2, 2, stderr);
    KheSolnSetTrieGatherAndClear(next_igtg->soln_set_trie,
      &next_igtg->kept_solns, igsv);
    next_igtg->soln_set_trie = NULL;
#if DEBUG_COMPATIBLE
    {
      KHE_IG_SOLN igs;
      HaArrayForEach(next_igtg->kept_solns, igs, j)
	HnAssert(igs->cc_index == igsv->cc_index,
	  "KheIgSolverDoSolve internal error 2");
    }
#endif
    HaArraySort(next_igtg->kept_solns, &KheIgSolnCmp);
    next_igtg->undominated_solns = HaArrayCount(next_igtg->kept_solns);
    if( DEBUG62 )
      KheTimeGroupDebugSolnCostHisto(next_igtg, 2, 2, stderr);
    while( HaArrayCount(next_igtg->kept_solns) > igsv->ig_max_keep )
    {
      prev_igs = HaArrayLastAndDelete(next_igtg->kept_solns);
      KheIgSolnFree(prev_igs, igsv, false);
    }
    if( DEBUG33(i) )
    {
      KheIgTimeGroupDebugSolns(next_igtg, 2, 2, stderr);
      /* ***
      if( HaArrayCount(next_igtg->kept_solns) >= 4 )
      {
        KHE_IG_SOLN igs1, igs2;
	igs1 = HaArray(next_igtg->kept_solns, 1);
	igs2 = HaArray(next_igtg->kept_solns, 3);
	KheIgSolnDominates(igs1, igs2, igsv, true);
      }
      *** */
    }
    else if( DEBUG26 )
      KheIgTimeGroupDebugSolns(next_igtg, 1, 2, stderr);
    if( DEBUG54 )
      KheIgCostTrieDebug(next_igtg->cost_trie, 2, 2, stderr);
    if( HaArrayCount(next_igtg->kept_solns) > igsv->ig_max_beam )
      expand_limit = 1;
    else
      expand_limit = INT_MAX;
    prev_igtg = next_igtg;
  }

  /* if there is a best solution, build its groups */
  last_igtg = HaArrayLast(igsv->time_groups);
  if( DEBUG28 && HaArrayCount(last_igtg->kept_solns) > 1 )
  {
    fprintf(stderr, "[ %d final solutions:\n",
      HaArrayCount(last_igtg->kept_solns));
    KheIgSolnDebug(HaArrayFirst(last_igtg->kept_solns), igsv, 3, 2, stderr);
    fprintf(stderr, "  ... ... ...\n");
    KheIgSolnDebug(HaArrayLast(last_igtg->kept_solns), igsv, 3, 2, stderr);
    fprintf(stderr, "]\n");
  }
  HnAssert(HaArrayCount(last_igtg->kept_solns) <= 1,
    "KheIgSolverDoSolve internal error 3");
  if( HaArrayCount(last_igtg->kept_solns) == 1 )
  {
    *best_igs = HaArrayFirst(last_igtg->kept_solns);
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
  KHE_CONSTRAINT_CLASS cc, bool two_three)
{
  KHE_IG_SOLN best_igs, best_igs2;  KHE_TIMER timer;  char buff[20];
  int i, j;  KHE_TIME_GROUP tg;  KHE_IG_TIME_GROUP igtg;  KHE_POLARITY po;
  KHE_CONSTRAINT_CLASS busy_days_cc;

  if( DEBUG8 || DEBUG27 )
  {
    fprintf(stderr, "[ KheIgSolverSolveConstraintClass(cc, igsv)\n");
    KheConstraintClassDebug(cc, 2, 2, stderr);
    timer = KheTimerMake(KheConstraintClassId(cc), KHE_NO_TIME, igsv->arena);
  }

  /* add the class and its limits to igsv */
  igsv->curr_class = cc;
  igsv->min_limit = KheConstraintClassMinimum(cc);
  igsv->max_limit = (two_three ? 3 : KheConstraintClassMaximum(cc));
  igsv->soln_seq_num = 0;

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
  /* HaArrayClear(igsv->included_tasks); */
  KheIgSolverAddAdmissiblePrimaryMTasks(igsv);
  KheIgSolverAddWeekendMTasks(igsv);
  /* ***
  if( DEBUG1 )
    KheIgSolverDebug(igsv, 2, 2, stderr);
  *** */

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
  HaArrayForEachReverse(igsv->time_groups, igtg, i)
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
  KHE_OPTIONS options, KHE_TASK_GROUP_DOMAIN_FINDER tgdf,
  KHE_SOLN_ADJUSTER sa)
{
  int count, res, ig_min, ig_range, ig_max_keep, ig_max_beam, i;
  KHE_IG_SOLVER igsv; HA_ARENA a;  KHE_MTASK_FINDER mtf;  KHE_MTASK mt;
  KHE_FRAME days_frame;  KHE_CONSTRAINT_CLASS cc;
  bool ig_two_three, ig_complete;

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
  days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
  ig_min = KheOptionsGetInt(options, "rs_interval_grouping_min", 4);
  ig_range = KheOptionsGetInt(options, "rs_interval_grouping_range", 2);
  ig_two_three = KheOptionsGetBool(options, "rs_interval_grouping_two_three",
    false);
  ig_max_keep = KheOptionsGetInt(options, "rs_interval_grouping_max_keep",
    MAX_KEEP_DEFAULT);
  ig_max_beam = KheOptionsGetInt(options, "rs_interval_grouping_max_beam",
    MAX_BEAM_DEFAULT);
  ig_complete = KheOptionsGetBool(options, "rs_interval_grouping_complete",
    false);
  mtf = KheMTaskFinderMake(soln, rt, days_frame, true, a);
  igsv = KheIgSolverMake(soln, rt, options, ig_min, ig_range, ig_two_three,
    ig_complete, ig_max_keep, ig_max_beam, mtf, tgdf, sa, a);

  /* add the task domains to the domain finder */
  count = KheMTaskFinderMTaskCount(igsv->mtask_finder);
  for( i = 0;  i < count;  i++ )
  {
    mt = KheMTaskFinderMTask(igsv->mtask_finder, i);
    KheTaskGroupDomainFinderAddTaskDomain(igsv->domain_finder,
      KheMTaskDomain(mt));
  }

  /* add constraint classes and solve for each class */
  KheConstraintClassFindCandidates(soln, rt, days_frame, igsv->candidate_ccf,
    igsv->busy_days_ccf);
  count = KheConstraintClassFinderClassCount(igsv->candidate_ccf);
  for( igsv->cc_index = 0;  igsv->cc_index < count;  igsv->cc_index++ )
  {
    cc = KheConstraintClassFinderClass(igsv->candidate_ccf, igsv->cc_index);
    if( KheConstraintClassIsSelected(cc, ig_min, ig_range) )
      KheIgSolverSolveConstraintClass(igsv, cc, false);
    else if( ig_two_three && KheConstraintClassIsTwoThree(cc) )
      KheIgSolverSolveConstraintClass(igsv, cc, true);
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

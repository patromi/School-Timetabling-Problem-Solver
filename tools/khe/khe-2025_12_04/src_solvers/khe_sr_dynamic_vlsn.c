
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
/*  FILE:         khe_sr_dynamic_vlsn.c                                      */
/*  DESCRIPTION:  Dynamic resource VLSN solving                              */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>
#define is_digit(x) ((x) >= '0' && (x) <= '9')
#define bool_show(x) ((x) ? "true" : "false")
#define max(x, y) ((x) > (y) ? (x) : (y))
#define min(x, y) ((x) < (y) ? (x) : (y))

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0	/* cluster */
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0
#define DEBUG9 0
#define DEBUG10 0
#define DEBUG11 0
#define DEBUG12 0
#define DEBUG13 0
#define DEBUG14 0
#define DEBUG15 0
#define DEBUG16 1   /* don't adjust PreferResourcesRedundancy monitors */
#define DEBUG17 0
#define DEBUG18 0


/*****************************************************************************/
/*                                                                           */
/* Submodule "type declarations - forward declarations"                      */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_vlsn_solver_rec *KHE_VLSN_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "type declarations - cluster constraint sets"                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CLUSTER_CONSTRAINT_SET - a set of very similar cluster constraints   */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT)
  ARRAY_KHE_CLUSTER_BUSY_TIMES_CONSTRAINT;

typedef struct khe_cluster_constraint_set_rec {
  ARRAY_KHE_CLUSTER_BUSY_TIMES_CONSTRAINT constraints;
} *KHE_CLUSTER_CONSTRAINT_SET;

typedef HA_ARRAY(KHE_CLUSTER_CONSTRAINT_SET) ARRAY_KHE_CLUSTER_CONSTRAINT_SET;


/*****************************************************************************/
/*                                                                           */
/*  KHE_CLUSTER_GROUPER - a set of cluster constraint sets                   */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_cluster_grouper_rec {
  ARRAY_KHE_CLUSTER_BUSY_TIMES_CONSTRAINT	all_cluster_constraints;
  ARRAY_KHE_CLUSTER_CONSTRAINT_SET		constraint_sets;
} *KHE_CLUSTER_GROUPER;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "type declarations - neighbourhoods"                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  Type KHE_INTERVAL_SET - a set of days                                    */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_INTERVAL) ARRAY_KHE_INTERVAL;

typedef struct khe_interval_set_rec {
  ARRAY_KHE_INTERVAL	intervals;
} *KHE_INTERVAL_SET;

typedef HA_ARRAY(KHE_INTERVAL_SET) ARRAY_KHE_INTERVAL_SET;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_NHOOD - a neighbourhood; a set of resources and a set of days   */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_nhood_rec {
  KHE_RESOURCE_SET		resources;
  KHE_INTERVAL_SET		days;
} KHE_NHOOD;


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD_SHAPE - a (resource_count, day_count) pair                     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_nhood_shape_rec {
  int			resource_count;
  int			day_count;
} KHE_NHOOD_SHAPE;

typedef HA_ARRAY(KHE_NHOOD_SHAPE) ARRAY_KHE_NHOOD_SHAPE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD_SHAPE_SET - a set of neighbourhood shapes                      */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_nhood_shape_set_rec {
  ARRAY_KHE_NHOOD_SHAPE		shapes;
} *KHE_NHOOD_SHAPE_SET;


/*****************************************************************************/
/*                                                                           */
/* Submodule "type declarations - resources"                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  Type KHE_FOLLOWER_RESOURCE - a follower resource                         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_follower_resource_rec {
  KHE_RESOURCE		resource;
  int			matches;
  float			similarity;
} *KHE_FOLLOWER_RESOURCE;

typedef HA_ARRAY(KHE_FOLLOWER_RESOURCE) ARRAY_KHE_FOLLOWER_RESOURCE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_LEADER_RESOURCE - a leader resource                             */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_leader_resource_rec {
  KHE_RESOURCE			resource;
  ARRAY_KHE_FOLLOWER_RESOURCE	followers;
} *KHE_LEADER_RESOURCE;

typedef HA_ARRAY(KHE_LEADER_RESOURCE) ARRAY_KHE_LEADER_RESOURCE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVAIL_RESOURCE - a resource with its availability attached           */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_avail_resource_rec {
  KHE_RESOURCE			resource;
  int				non_rigidity;
  int				avail_times;
  int				unavail_times;
  int				random;
  int				index;
} *KHE_AVAIL_RESOURCE;

typedef HA_ARRAY(KHE_AVAIL_RESOURCE) ARRAY_KHE_AVAIL_RESOURCE;


/*****************************************************************************/
/*                                                                           */
/* Submodule "type declarations - prefer functions"                          */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_PREFER_ANY_TAG,
  KHE_PREFER_OVERLOADED_TAG,
  KHE_PREFER_BUSY_FOR_TIME_GROUP_TAG,
  KHE_PREFER_BUSY_FOR_TIME_GROUP2_TAG,
  KHE_PREFER_FREE_FOR_TASK_SET_TAG,
  KHE_PREFER_FREE_FOR_TASK_SET_AND_BELOW_LIMIT_TAG,
  KHE_PREFER_FREE_FOR_TASK_TAG,
  KHE_PREFER_FREE_FOR_TASK_AND_PREFERRED_TAG,
  KHE_PREFER_FREE_FOR_TASK_AND_UNPREFERRED_TAG
} KHE_PREFER_TAG;

#define INHERIT_PREFER						\
  KHE_PREFER_TAG	tag;

typedef struct khe_prefer_rec {
  INHERIT_PREFER
} *KHE_PREFER;

typedef struct khe_prefer_any_rec {
  INHERIT_PREFER
} *KHE_PREFER_ANY;

typedef struct khe_prefer_overloaded_rec {
  INHERIT_PREFER
} *KHE_PREFER_OVERLOADED;

typedef struct khe_prefer_busy_for_time_group_rec {
  INHERIT_PREFER
  KHE_TIME_GROUP	tg;
  KHE_INTERVAL		in;
  int			offset;
} *KHE_PREFER_BUSY_FOR_TIME_GROUP;

typedef struct khe_prefer_busy_for_time_group2_rec {
  INHERIT_PREFER
  KHE_TIME_GROUP	tg1;
  KHE_TIME_GROUP	tg2;
  KHE_INTERVAL		in;
  int			offset;
} *KHE_PREFER_BUSY_FOR_TIME_GROUP2;

typedef struct khe_prefer_free_for_task_set_rec {
  INHERIT_PREFER
  KHE_TASK_SET		ts;
  KHE_INTERVAL		in;
  int			offset;
} *KHE_PREFER_FREE_FOR_TASK_SET;

typedef struct khe_prefer_free_for_task_set_and_below_limit_rec {
  INHERIT_PREFER
  KHE_TASK_SET		ts;
  KHE_INTERVAL		in;
  int			offset;
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;
} *KHE_PREFER_FREE_FOR_TASK_SET_AND_BELOW_LIMIT;

typedef struct khe_prefer_free_for_task_rec {
  INHERIT_PREFER
  KHE_TASK		t;
} *KHE_PREFER_FREE_FOR_TASK;

typedef struct khe_prefer_free_for_task_and_preferred_rec {
  INHERIT_PREFER
  KHE_TASK		t;
  KHE_RESOURCE_GROUP	rg;
} *KHE_PREFER_FREE_FOR_TASK_AND_PREFERRED;

typedef struct khe_prefer_free_for_task_and_unpreferred_rec {
  INHERIT_PREFER
  KHE_TASK		t;
  KHE_RESOURCE_GROUP	rg;
} *KHE_PREFER_FREE_FOR_TASK_AND_UNPREFERRED;


/*****************************************************************************/
/*                                                                           */
/* Submodule "type declarations - monitors"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_MONITOR_TAG - type tag for VLSN monitors                        */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_VLSN_ASSIGN_RESOURCE_MONITOR_TAG,
  KHE_VLSN_PREFER_RESOURCES_MONITOR_TAG,
  KHE_VLSN_LIMIT_RESOURCES_MONITOR_TAG,
  KHE_VLSN_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG,
  KHE_VLSN_CLUSTER_BUSY_TIMES_MONITOR_TAG,
  KHE_VLSN_LIMIT_BUSY_TIMES_MONITOR_TAG,
  KHE_VLSN_LIMIT_WORKLOAD_MONITOR_TAG,
  KHE_VLSN_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG
} KHE_VLSN_MONITOR_TAG;


/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_MONITOR - abstract supertype of VLSN monitors                   */
/*                                                                           */
/*****************************************************************************/

#define INHERIT_KHE_VLSN_MONITOR					\
  KHE_VLSN_MONITOR_TAG		tag;					\
  KHE_MONITOR			monitor;

typedef struct khe_vlsn_monitor_rec {
  INHERIT_KHE_VLSN_MONITOR
} *KHE_VLSN_MONITOR;

typedef HA_ARRAY(KHE_VLSN_MONITOR) ARRAY_KHE_VLSN_MONITOR;


/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_ASSIGN_RESOURCE_MONITOR - VLSN assign resource monitor          */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_RESOURCE) ARRAY_KHE_RESOURCE;

typedef struct khe_vlsn_assign_resource_monitor_rec {
  INHERIT_KHE_VLSN_MONITOR
} *KHE_VLSN_ASSIGN_RESOURCE_MONITOR;


/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_PREFER_RESOURCES_MONITOR - VLSN prefer resources monitor        */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_vlsn_prefer_resources_monitor_rec {
  INHERIT_KHE_VLSN_MONITOR
} *KHE_VLSN_PREFER_RESOURCES_MONITOR;


/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_LIMIT_RESOURCES_MONITOR - VLSN limit resources monitor          */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_vlsn_limit_resources_monitor_rec {
  INHERIT_KHE_VLSN_MONITOR
} *KHE_VLSN_LIMIT_RESOURCES_MONITOR;


/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_AVOID_UNAVAILABLE_TIMES_MONITOR - VLSN avoid unavailable times m*/
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_TIME) ARRAY_KHE_TIME;

typedef struct khe_vlsn_avoid_unavailable_times_monitor_rec {
  INHERIT_KHE_VLSN_MONITOR
} *KHE_VLSN_AVOID_UNAVAILABLE_TIMES_MONITOR;


/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_CLUSTER_BUSY_TIMES_MONITOR - VLSN cluster busy times monitor    */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_vlsn_cluster_busy_times_monitor_rec {
  INHERIT_KHE_VLSN_MONITOR
} *KHE_VLSN_CLUSTER_BUSY_TIMES_MONITOR;


/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_LIMIT_BUSY_TIMES_MONITOR - VLSN limit busy times monitor        */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_vlsn_limit_busy_times_monitor_rec {
  INHERIT_KHE_VLSN_MONITOR
} *KHE_VLSN_LIMIT_BUSY_TIMES_MONITOR;


/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_LIMIT_WORKLOAD_MONITOR - VLSN limit workload monitor            */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_vlsn_limit_workload_monitor_rec {
  INHERIT_KHE_VLSN_MONITOR
} *KHE_VLSN_LIMIT_WORKLOAD_MONITOR;


/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_LIMIT_ACTIVE_INTERVALS_MONITOR - VLSN limit active intervals m. */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_LAIM_MAKE_SHORTER_AT_LEFT,
  KHE_LAIM_MAKE_SHORTER_AT_RIGHT,
  KHE_LAIM_MAKE_LONGER_AT_LEFT,
  KHE_LAIM_MAKE_EMPTY,
  KHE_LAIM_MAKE_LONGER_AT_RIGHT
} KHE_LAIM_CASE;

typedef HA_ARRAY(KHE_LAIM_CASE) ARRAY_KHE_LAIM_CASE;

typedef struct khe_vlsn_limit_active_intervals_monitor_rec {
  INHERIT_KHE_VLSN_MONITOR
  int			single_shift_offset;
  ARRAY_KHE_LAIM_CASE	cases;
} *KHE_VLSN_LIMIT_ACTIVE_INTERVALS_MONITOR;


/*****************************************************************************/
/*                                                                           */
/* Submodule "type declarations - resource selects"                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  Type KHE_RESOURCE_SELECT - determines a sequence of sets of resources    */
/*                                                                           */
/*****************************************************************************/

/* tags of concrete subtypes */
typedef enum {
  KHE_RESOURCE_SELECT_SINGLE_TAG,
  KHE_RESOURCE_SELECT_ENUM_TAG,
  KHE_RESOURCE_SELECT_CLUSTER_TAG,
  KHE_RESOURCE_SELECT_CHOOSE_TAG,
} KHE_RESOURCE_SELECT_TAG;

/* common fields */
#define INHERIT_RESOURCE_SELECT						\
  KHE_RESOURCE_SELECT_TAG	tag;					\
  KHE_VLSN_SOLVER		solver;

/* the abstract type itself */
typedef struct khe_resource_select_rec {
  INHERIT_RESOURCE_SELECT
} *KHE_RESOURCE_SELECT;


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SELECT_SINGLE - a given sequence of one set of resources    */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_resource_select_single_rec {
  INHERIT_RESOURCE_SELECT
  KHE_RESOURCE_SET		resource_set;
} *KHE_RESOURCE_SELECT_SINGLE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SELECT_ENUM - a given sequence of sets of resources         */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_RESOURCE_SET) ARRAY_KHE_RESOURCE_SET;

typedef struct khe_resource_select_enum_rec {
  INHERIT_RESOURCE_SELECT
  ARRAY_KHE_RESOURCE_SET	resource_sets;
} *KHE_RESOURCE_SELECT_ENUM;


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SELECT_CLUSTER - resource sets from cluster constraints     */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_UNDERLOADED	= 0,
  KHE_MAXIMAL		= 1,
  KHE_OVERLOADED	= 2
} KHE_RESOURCE_LOAD;

typedef struct khe_resource_select_cluster_rec {
  INHERIT_RESOURCE_SELECT
  char				*constraint_str;
  ARRAY_KHE_RESOURCE		load_class[KHE_OVERLOADED + 1];
  ARRAY_KHE_RESOURCE_SET	resource_sets;
} *KHE_RESOURCE_SELECT_CLUSTER;


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SELECT_CHOOSE - resource sets chosen randomly               */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_LEADER_ANY,
  KHE_LEADER_OVERLOADED
} KHE_LEADER_TYPE;

typedef enum {
  KHE_FOLLOWER_ANY,
  KHE_FOLLOWER_SIMILAR,
  KHE_FOLLOWER_SIMILAR_AND_UNDERLOADED
} KHE_FOLLOWER_TYPE;

typedef struct khe_resource_select_choose_rec {
  INHERIT_RESOURCE_SELECT
  int				resource_count;
  char				*input_label;
  KHE_LEADER_TYPE		leader_type;
  KHE_FOLLOWER_TYPE		follower_type;
  ARRAY_KHE_LEADER_RESOURCE	leaders;
} *KHE_RESOURCE_SELECT_CHOOSE;


/*****************************************************************************/
/*                                                                           */
/* Submodule "type declarations - day selects"                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DAY_SELECT - determines a sequence of sets of days              */
/*                                                                           */
/*****************************************************************************/

/* tags of concrete subtypes */
typedef enum {
  KHE_DAY_SELECT_SINGLE_TAG,
  KHE_DAY_SELECT_ENUM_TAG,
  KHE_DAY_SELECT_CHOOSE_TAG
} KHE_DAY_SELECT_TAG;

/* common fields */
#define INHERIT_DAY_SELECT						\
  KHE_DAY_SELECT_TAG	tag;						\
  KHE_VLSN_SOLVER	solver;

/* the abstract type itself */
typedef struct khe_day_select_rec {
  INHERIT_DAY_SELECT
} *KHE_DAY_SELECT;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DAY_SELECT_SINGLE - a sequence of one set of days               */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_day_select_single_rec {
  INHERIT_DAY_SELECT
  KHE_INTERVAL_SET		interval_set;
} *KHE_DAY_SELECT_SINGLE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DAY_SELECT_ENUM - an explicit sequence of sets of days          */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_day_select_enum_rec {
  INHERIT_DAY_SELECT
  ARRAY_KHE_INTERVAL_SET	interval_sets;
} *KHE_DAY_SELECT_ENUM;


/*****************************************************************************/
/*                                                                           */
/*  KHE_DAY_SELECT_CHOOSE - sets of days chosen at random                    */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_day_select_choose_rec {
  INHERIT_DAY_SELECT
  int				choose_len;
} *KHE_DAY_SELECT_CHOOSE;


/*****************************************************************************/
/*                                                                           */
/* Submodule "type declarations - selects"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  Type KHE_SELECT - selects resources and days                             */
/*                                                                           */
/*****************************************************************************/

/* tags of concrete subtypes (last three make up KHE_SELECT_COMBINED) */
typedef enum {
  KHE_SELECT_SEPARATE_TAG,
  KHE_SELECT_SAME_SHIFT_TAG,
  KHE_SELECT_DEFECTIVE_SHIFT_TAG,
  KHE_SELECT_TARGETED_TAG
} KHE_SELECT_TAG;

/* common fields */
#define INHERIT_SELECT							\
  KHE_SELECT_TAG		tag;					\
  KHE_VLSN_SOLVER		solver;

/* the abstract type itself */
typedef struct khe_select_rec {
  INHERIT_SELECT
} *KHE_SELECT;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_SELECT_SEPARATE - selects resources and days separately         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_select_separate_rec {
  INHERIT_SELECT
  KHE_RESOURCE_SELECT		resource_select;
  KHE_DAY_SELECT		day_select;
} *KHE_SELECT_SEPARATE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_SELECT_COMBINED - selects resources and days together           */
/*                                                                           */
/*****************************************************************************/

/* tags of concrete subtypes are defined under KHE_SELECT */

/* common fields */
#define INHERIT_SELECT_COMBINED					\
  INHERIT_SELECT

/* the abstract type itself */
typedef struct khe_select_combined_rec {
  INHERIT_SELECT_COMBINED
} *KHE_SELECT_COMBINED;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SELECT_SAME_SHIFT - selects whose resources are busy on same shift   */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_select_same_shift_rec {
  INHERIT_SELECT_COMBINED
  int				resource_count;
  int				days_count;
} *KHE_SELECT_SAME_SHIFT;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SELECT_DEFECTIVE_SHIFT - selects whose resource have defective shift */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR)
  ARRAY_KHE_LIMIT_ACTIVE_INTERVALS_MONITOR;

typedef struct khe_select_defective_shift_rec {
  INHERIT_SELECT_COMBINED
  int						resource_count;
  int						days_count;
  ARRAY_KHE_LIMIT_ACTIVE_INTERVALS_MONITOR	all_monitors;
  ARRAY_KHE_LIMIT_ACTIVE_INTERVALS_MONITOR	defective_monitors;
} *KHE_SELECT_DEFECTIVE_SHIFT;


/*****************************************************************************/
/*                                                                           */
/*  KHE_SELECT_TARGETED - a targeted select                                  */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_select_targeted_rec {
  INHERIT_SELECT_COMBINED
  KHE_NHOOD_SHAPE_SET		shape_set;
  ARRAY_KHE_VLSN_MONITOR	all_monitors;
  ARRAY_KHE_VLSN_MONITOR	defective_monitors;
  int				defective_monitor_index;
} *KHE_SELECT_TARGETED;


/*****************************************************************************/
/*                                                                           */
/* Submodule "type declarations - other"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  Type KHE_SOLVE_ARGUMENTS - arguments to a solve                          */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_solve_arguments_rec {
  KHE_SELECT		select;				/* R and D           */
  bool			priqueue;			/* Q                 */
  bool			extra_selection;		/* E                 */
  bool			expand_by_shifts;		/* S                 */
  bool			shift_pairs;			/* P                 */
  bool			correlated_exprs;		/* C                 */
  int			daily_expand_limit;		/* L                 */
  int			daily_prune_trigger;		/* T                 */
  int			resource_expand_limit;		/* M                 */
  int			dom_approx;			/* A                 */
  KHE_DRS_DOM_KIND	main_dom_kind;			/* K                 */
  bool			cache;				/* J                 */
  KHE_DRS_DOM_KIND	cache_dom_kind;			/* J                 */
} *KHE_SOLVE_ARGUMENTS;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DIM_POS_OPT - one option of one position of one dimension       */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_drs_dim_pos_opt_rec {
  char				tag;		/* capital letter */
  union {
    bool			bool_val;
    int				int_val;
    KHE_DRS_DOM_KIND		dom_kind_val;
    KHE_RESOURCE_SELECT		resource_select_val;
    KHE_DAY_SELECT		day_select_val;
  } u;
} *KHE_DIM_POS_OPT;

typedef HA_ARRAY(KHE_DIM_POS_OPT) ARRAY_KHE_DIM_POS_OPT;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DIM_POS - one position of one dimension                         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_dim_pos_rec {
  ARRAY_KHE_DIM_POS_OPT		options;
} *KHE_DIM_POS;

typedef HA_ARRAY(KHE_DIM_POS) ARRAY_KHE_DIM_POS;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DIM - one dimension                                             */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_dim_rec {
  bool				days_dim;		/* true if dim is Z  */
  ARRAY_KHE_DIM_POS		dim_poses;
} *KHE_DIM;

typedef HA_ARRAY(KHE_DIM) ARRAY_KHE_DIM;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_OPTIMAL_RESULT - one optimal result                             */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_optimal_result_rec {
  KHE_RESOURCE_SET	resource_set;
  KHE_INTERVAL_SET	day_set;
  KHE_COST		cost;
} *KHE_OPTIMAL_RESULT;

typedef HA_ARRAY(KHE_OPTIMAL_RESULT) ARRAY_KHE_OPTIMAL_RESULT;


/*****************************************************************************/
/*                                                                           */
/*  KHE_OPTIMAL_RESULT_SET - a set of optimal results                        */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_optimal_result_set_rec {
  ARRAY_KHE_OPTIMAL_RESULT	results;
} *KHE_OPTIMAL_RESULT_SET;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_VLSN_SOLVER - controller for solving and testing                */
/*                                                                           */
/*****************************************************************************/

struct khe_vlsn_solver_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_RESOURCE_TYPE		resource_type;
  KHE_OPTIONS			options;
  KHE_FRAME			days_frame;
  KHE_RANDOM_GENERATOR		rgen;
  ARRAY_KHE_LEADER_RESOURCE	resources;
  KHE_CLUSTER_GROUPER		cluster_grouper;
  KHE_DYNAMIC_RESOURCE_SOLVER	solver;
  ARRAY_KHE_DIM			test_dims;
  KHE_OPTIMAL_RESULT_SET	optimal_results;
  ARRAY_KHE_RESOURCE_SET	resource_set_free_list;
  ARRAY_KHE_FOLLOWER_RESOURCE	follower_resource_free_list;
  ARRAY_KHE_LEADER_RESOURCE	leader_resource_free_list;

  /* scratch variables used when constructing neighbourhoods */
  KHE_TASK_SET			tasks;
  ARRAY_KHE_RESOURCE		preferred_resources;
  ARRAY_KHE_RESOURCE		other_resources;
  HA_ARRAY_INT			tg_indexes;
  KHE_RESOURCE_SET		scratch_rs;
  KHE_INTERVAL_SET		scratch_is;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_CLUSTER_CONSTRAINT_SET"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CLUSTER_CONSTRAINT_SET KheClusterConstraintSetMake(HA_ARENA a)       */
/*                                                                           */
/*  Make a new, empty constraint set.                                        */
/*                                                                           */
/*****************************************************************************/

static KHE_CLUSTER_CONSTRAINT_SET KheClusterConstraintSetMake(HA_ARENA a)
{
  KHE_CLUSTER_CONSTRAINT_SET res;
  HaMake(res, a);
  HaArrayInit(res->constraints, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintSetContains(KHE_CLUSTER_CONSTRAINT_SET cs,             */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Return true if cs contains c.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintSetContains(KHE_CLUSTER_CONSTRAINT_SET cs,
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  int pos;
  return HaArrayContains(cs->constraints, c, &pos);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_CLUSTER_GROUPER"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CLUSTER_GROUPER KheClusterGrouperMake(HA_ARENA a)                    */
/*                                                                           */
/*  Make a cluster grouper object.                                           */
/*                                                                           */
/*****************************************************************************/

static KHE_CLUSTER_GROUPER KheClusterGrouperMake(HA_ARENA a)
{
  KHE_CLUSTER_GROUPER res;
  HaMake(res, a);
  HaArrayInit(res->all_cluster_constraints, a);
  HaArrayInit(res->constraint_sets, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesConstraintTypedCmp(                               */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc1,                               */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc2)                               */
/*                                                                           */
/*  Typed comparison function for bringing similar cluster busy times        */
/*  constraints together.                                                    */
/*                                                                           */
/*****************************************************************************/

static int KheClusterBusyTimesConstraintTypedCmp(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc1,
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc2)
{
  int i, count1, count2, cmp, offset1, offset2;
  KHE_TIME_GROUP tg1, tg2;  KHE_POLARITY po1, po2;

  /* same offsets */
  count1 = KheClusterBusyTimesConstraintAppliesToOffsetCount(cbtc1);
  count2 = KheClusterBusyTimesConstraintAppliesToOffsetCount(cbtc2);
  if( count1 != count2 )
    return count1 - count2;
  for( i = 0;  i < count1;  i++ )
  {
    offset1 = KheClusterBusyTimesConstraintAppliesToOffset(cbtc1, i);
    offset2 = KheClusterBusyTimesConstraintAppliesToOffset(cbtc2, i);
    if( offset1 != offset2 )
      return offset1 - offset2;
  }

  /* same time groups */
  count1 = KheClusterBusyTimesConstraintTimeGroupCount(cbtc1);
  count2 = KheClusterBusyTimesConstraintTimeGroupCount(cbtc2);
  if( count1 != count2 )
    return false;
  for( i = 0;  i < count1;  i++ )
  {
    tg1 = KheClusterBusyTimesConstraintTimeGroup(cbtc1, i, 0, &po1);
    tg2 = KheClusterBusyTimesConstraintTimeGroup(cbtc2, i, 0, &po2);
    cmp = KheTimeGroupTypedCmp(tg1, tg2);
    if( cmp != 0 )
      return cmp;
    if( po1 != po2 )
      return (int) po1 - (int) po2;
  }

  /* all good, return 0 */
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesConstraintCmp(const void *t1, const void *t2)     */
/*                                                                           */
/*  Untyped comparison function for bringing similar cluster busy times      */
/*  constraints together.                                                    */
/*                                                                           */
/*****************************************************************************/

static int KheClusterBusyTimesConstraintCmp(const void *t1, const void *t2)
{
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc1, cbtc2;
  cbtc1 = * (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT *) t1;
  cbtc2 = * (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT *) t2;
  return KheClusterBusyTimesConstraintTypedCmp(cbtc1, cbtc2);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CLUSTER_GROUPER KheClusterGrouperBuild(KHE_INSTANCE ins,             */
/*    KHE_RESOURCE_TYPE rt, HA_ARENA a)                                      */
/*                                                                           */
/*  Build an cluster grouper object and fill it with cluster constraints.    */
/*                                                                           */
/*****************************************************************************/

static KHE_CLUSTER_GROUPER KheClusterGrouperBuild(KHE_INSTANCE ins,
  KHE_RESOURCE_TYPE rt, HA_ARENA a)
{
  KHE_CLUSTER_GROUPER cg;  KHE_CONSTRAINT c;  int i, j;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc, cbtc_i, cbtc_j;
  KHE_CLUSTER_CONSTRAINT_SET cs;

  /* make a cluster grouper object and add all cluster constraints to it */
  cg = KheClusterGrouperMake(a);
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintTag(c) == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG )
    {
      cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c;
      if( KheClusterBusyTimesConstraintResourceOfTypeCount(cbtc, rt) > 0 )
	HaArrayAddLast(cg->all_cluster_constraints, cbtc);
    }
  }

  /* sort the cluster constraints to bring similar ones together */
  HaArraySort(cg->all_cluster_constraints, &KheClusterBusyTimesConstraintCmp);

  /* distribute the cluster constraints into similar sets */
  for( i = 0;  i < HaArrayCount(cg->all_cluster_constraints);  i = j )
  {
    cbtc_i = HaArray(cg->all_cluster_constraints, i);
    cs = KheClusterConstraintSetMake(a);
    HaArrayAddLast(cg->constraint_sets, cs);
    HaArrayAddLast(cs->constraints, cbtc_i);
    for( j = i + 1;  j < HaArrayCount(cg->all_cluster_constraints);  j++ )
    {
      cbtc_j = HaArray(cg->all_cluster_constraints, j);
      if( KheClusterBusyTimesConstraintTypedCmp(cbtc_i, cbtc_j) != 0 )
	break;
      HaArrayAddLast(cs->constraints, cbtc_j);
    }
  }

  /* all done */
  return cg;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CLUSTER_CONSTRAINT_SET KheClusterGrouperFindConstraintSet(           */
/*    KHE_CLUSTER_GROUPER cg, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)           */
/*                                                                           */
/*  Find the constraint set in cg containing c.  There must be one.          */
/*                                                                           */
/*****************************************************************************/

static KHE_CLUSTER_CONSTRAINT_SET KheClusterGrouperFindConstraintSet(
  KHE_CLUSTER_GROUPER cg, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  KHE_CLUSTER_CONSTRAINT_SET cs;  int i;
  HaArrayForEach(cg->constraint_sets, cs, i)
    if( KheConstraintSetContains(cs, c) )
      return cs;
  HnAbort("KheClusterGrouperFindConstraintSet internal error");
  return NULL;  /* keep compiler happy */
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "basic parsing"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSkipWhiteSpace(char **str)                                       */
/*                                                                           */
/*  Skip white space in *str.                                                */
/*                                                                           */
/*****************************************************************************/

static void KheSkipWhiteSpace(char **str)
{
  while( **str == ' ' || **str == '\t' )
    (*str)++;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheParseHasEnded(char **str)                                        */
/*                                                                           */
/*  Return true if the parse of *str has ended.                              */
/*                                                                           */
/*****************************************************************************/

static bool KheParseHasEnded(char **str)
{
  KheSkipWhiteSpace(str);
  return **str == '\0';
}


/*****************************************************************************/
/*                                                                           */
/*  char KheCharParse(char **str)                                            */
/*                                                                           */
/*  Read the next non-white-space character from *str, return it, and        */
/*  skip over it.                                                            */
/*                                                                           */
/*****************************************************************************/

static char KheCharParse(char **str)
{
  char res;
  KheSkipWhiteSpace(str);
  res = **str;
  (*str)++;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheStrSkip(char **str, char *val)                                   */
/*                                                                           */
/*  Skip non-white-space string val and return true if successful, else      */
/*  change nothing and return false.                                         */
/*                                                                           */
/*****************************************************************************/

static bool KheStrSkip(char **str, char *val)
{
  char *p, *q;
  KheSkipWhiteSpace(str);
  for( p = *str, q = val;  *q != '\0';  p++, q++ )
    if( *p != *q )
      return false;
  *str = p;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheBoolParse(char **str)                                            */
/*                                                                           */
/*  Read the next bool from *str, return it, and skip over it.               */
/*                                                                           */
/*****************************************************************************/

static bool KheBoolParse(char **str)
{
  if( KheStrSkip(str, "true") )
    return true;
  else if( KheStrSkip(str, "false") )
    return false;
  else
  {
    HnAbort("error where true or false expected in rs_drs_test option");
    return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIntParse(char **str)                                              */
/*                                                                           */
/*  Read the next int from *str, return it, and skip over it.                */
/*                                                                           */
/*****************************************************************************/

static int KheIntParse(char **str)
{
  int res;
  KheSkipWhiteSpace(str);
  if( sscanf(*str, "%d", &res) != 1 )
    HnAbort("error where integer expected in rs_drs_test option");
  while( is_digit(**str) )
    (*str)++;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheStringParse(char **str, char stop)                              */
/*                                                                           */
/*  Read the next non-white-space string from *str, return it, and skip      */
/*  over it.                                                                 */
/*                                                                           */
/*****************************************************************************/

static char *KheStringParse(char **str, char stop)
{
  static char buff[200];  char *p, *q;
  KheSkipWhiteSpace(str);
  for( p = *str, q = buff; *p != '\0' && *p != stop && q < &buff[199]; p++, q++)
    *q = *p;
  *q = '\0';
  *str = p;
  return buff;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_RESOURCE_SET" - additional to khe_platform.h              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SET KheResourceSetParse(char **str, KHE_RESOURCE_TYPE rt,   */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Parse a resource set (represented by a comma-separated sequence of       */
/*  integer indexes into rt).                                                */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_SET KheResourceSetParse(char **str, KHE_RESOURCE_TYPE rt,
  HA_ARENA a)
{
  KHE_RESOURCE_SET res;  KHE_RESOURCE r;  int val;
  res = KheResourceSetMake(rt, a);
  do
  {
    val = KheIntParse(str);
    if( 0 <= val && val < KheResourceTypeResourceCount(rt) )
    {
      r = KheResourceTypeResource(rt, val);
      KheResourceSetAddResource(res, r);
    }
  } while( KheStrSkip(str, ",") );
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetAddDisplay(KHE_RESOURCE_SET rset, HA_ARRAY_NCHAR *res)*/
/*                                                                           */
/*  Add a display of rset to *res.                                           */
/*                                                                           */
/*****************************************************************************/

static void KheResourceSetAddDisplay(KHE_RESOURCE_SET rset, HA_ARRAY_NCHAR *res)
{
  int i;  KHE_RESOURCE r;
  for( i = 0;  i < KheResourceSetResourceCount(rset);  i++ )
  {
    if( i > 0 )
      HnStringAdd(res, ",");
    r = KheResourceSetResource(rset, i);
    HnStringAdd(res, "%s", KheResourceId(r));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_INTERVAL"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheIntervalParse(char **str)                                */
/*                                                                           */
/*  Parse an interval.                                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_INTERVAL KheIntervalParse(char **str)
{
  int first, last;
  first = KheIntParse(str);
  if( !KheStrSkip(str, "-") )
    HnAbort("error where interval hyphen expected in rs_drs_test option");
  last = KheIntParse(str);
  return KheIntervalMake(first, last);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheIntervalAdjustLength(KHE_INTERVAL in, int len,           */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Adjust the length of "in" randomly until it is len, or as close to len   */
/*  as possible.  This function implements formulas documented in the Guide. */
/*                                                                           */
/*****************************************************************************/

static KHE_INTERVAL KheIntervalAdjustLength(KHE_INTERVAL in, int len,
  KHE_VLSN_SOLVER vs)
{
  int delta, a, b, x;

  HnAssert(len >= 1, "KheIntervalAdjustLength internal error (len %d)", len);
  if( KheIntervalLength(in) < len )
  {
    /* in needs to grow */
    delta = len - KheIntervalLength(in);
    a = in.first;
    b = KheFrameTimeGroupCount(vs->days_frame) - 1 - in.last;
    if( delta > a + b )
    {
      in.first -= a;
      in.last  += b;
    }
    else
    {
      x = KheRandomGeneratorNextRange(&vs->rgen, max(0, delta - b),
	min(delta, a));
      in.first -= x;
      in.last += delta - x;
    }
  }
  else if( KheIntervalLength(in) > len )
  {
    /* in needs to shrink */
    delta = KheIntervalLength(in) - len;
    x = KheRandomGeneratorNextRange(&vs->rgen, 0, delta);
    in.first += x;
    in.last -= delta - x;
  }
  return in;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalAddDisplay(KHE_INTERVAL in, HA_ARRAY_NCHAR *res)         */
/*                                                                           */
/*  Add a display of in to res.                                              */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalAddDisplay(KHE_INTERVAL in, HA_ARRAY_NCHAR *res)
{
  HnStringAdd(res, "%d-%d", KheIntervalFirst(in), KheIntervalLast(in));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalDebug(KHE_INTERVAL in, FILE *fp)                         */
/*                                                                           */
/*  Debug print of in onto fp.                                               */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalDebug(KHE_INTERVAL in, FILE *fp)
{
  fprintf(fp, "%d-%d", KheIntervalFirst(in), KheIntervalLast(in));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_INTERVAL_SET"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL_SET KheIntervalSetMake(HA_ARENA a)                          */
/*                                                                           */
/*  Make a new, empty interval set object.                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_INTERVAL_SET KheIntervalSetMake(HA_ARENA a)
{
  KHE_INTERVAL_SET res;
  HaMake(res, a);
  HaArrayInit(res->intervals, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL_SET KheIntervalSetParse(char **str, HA_ARENA a)             */
/*                                                                           */
/*  Parse an interval set.                                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_INTERVAL_SET KheIntervalSetParse(char **str, HA_ARENA a)
{
  KHE_INTERVAL in;  KHE_INTERVAL_SET res;
  res = KheIntervalSetMake(a);
  do
  {
    in = KheIntervalParse(str);
    HaArrayAddLast(res->intervals, in);
  } while( KheStrSkip(str, ",") );
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalSetAddInterval(KHE_INTERVAL_SET is, KHE_INTERVAL in)     */
/*                                                                           */
/*  Add in to is.                                                            */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalSetAddInterval(KHE_INTERVAL_SET is, KHE_INTERVAL in)
{
  HaArrayAddLast(is->intervals, in);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalSetClear(KHE_INTERVAL_SET is)                            */
/*                                                                           */
/*  Clear is back to the empty interval set.                                 */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalSetClear(KHE_INTERVAL_SET is)
{
  HaArrayClear(is->intervals);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalSetResetSingle(KHE_INTERVAL_SET is, KHE_INTERVAL in)     */
/*                                                                           */
/*  Reset is so that it contains just in.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalSetResetSingle(KHE_INTERVAL_SET is, KHE_INTERVAL in)
{
  KheIntervalSetClear(is);
  KheIntervalSetAddInterval(is, in);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalSetEqual(KHE_INTERVAL_SET is1, KHE_INTERVAL_SET is2)     */
/*                                                                           */
/*  Return true if is1 and is2 are equal.                                    */
/*                                                                           */
/*****************************************************************************/

static bool KheIntervalSetEqual(KHE_INTERVAL_SET is1, KHE_INTERVAL_SET is2)
{
  KHE_INTERVAL i1, i2;  int i;
  if( is1 == is2 )
    return true;
  if( HaArrayCount(is1->intervals) != HaArrayCount(is2->intervals) )
    return false;
  for( i = 0;  i < HaArrayCount(is1->intervals);  i++ )
  {
    i1 = HaArray(is1->intervals, i);
    i2 = HaArray(is2->intervals, i);
    if( !KheIntervalEqual(i1, i2) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalSetResetRandom(KHE_INTERVAL_SET is, int len,             */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Reset is to an interval set containing one set of days of length len,    */
/*  or less if there aren't enough days.                                     */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalSetResetRandom(KHE_INTERVAL_SET is, int len,
  KHE_VLSN_SOLVER vs)
{
  int tg_count, start;

  /* choose a random start position for the interval, start */
  tg_count = KheFrameTimeGroupCount(vs->days_frame);
  if( len > tg_count )
    len = tg_count;
  start = KheRandomGeneratorNextRange(&vs->rgen, 0, tg_count - len);

  /* reset is so that it contains just [start, start + len - 1] */
  KheIntervalSetResetSingle(is, KheIntervalMake(start, start + len - 1));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalSetAddDisplay(KHE_INTERVAL_SET is, HA_ARRAY_NCHAR *res)  */
/*                                                                           */
/*  Add a representation of is to res.                                       */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalSetAddDisplay(KHE_INTERVAL_SET is, HA_ARRAY_NCHAR *res)
{
  KHE_INTERVAL in;  int i;
  HaArrayForEach(is->intervals, in, i)
  {
    if( i > 0 )
      HnStringAdd(res, ",");
    KheIntervalAddDisplay(in, res);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalSetDebug(KHE_INTERVAL_SET is, FILE *fp)                  */
/*                                                                           */
/*  Debug printf of is onto fp.                                              */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalSetDebug(KHE_INTERVAL_SET is, FILE *fp)
{
  KHE_INTERVAL in;  int i;
  HaArrayForEach(is->intervals, in, i)
  {
    if( i > 0 )
      fprintf(fp, ",");
    KheIntervalDebug(in, fp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_NHOOD"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheNHoodMake(KHE_RESOURCE_SET resources, KHE_INTERVAL_SET days)*/
/*                                                                           */
/*  Make a new nhood object with these attributes.  No memory allocation.    */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheNHoodMake(KHE_RESOURCE_SET resources, KHE_INTERVAL_SET days)
{
  KHE_NHOOD res;
  res.resources = resources;
  res.days = days;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheNHoodMakeNull(void)                                         */
/*                                                                           */
/*  Return a new null nhood object.                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheNHoodMakeNull(void)
{
  return KheNHoodMake(NULL, NULL);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNHoodIsNull(KHE_NHOOD nhood)                                     */
/*                                                                           */
/*  Return true if nhood is a NULL neighbourhood.                            */
/*                                                                           */
/*****************************************************************************/

static bool KheNHoodIsNull(KHE_NHOOD nhood)
{
  return nhood.resources == NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNHoodDebug(KHE_NHOOD nhood, int verbosity, int indent, FILE *fp) */
/*                                                                           */
/*  Debug print of nhood onto fp with the given verbosity and indent.        */
/*                                                                           */
/*****************************************************************************/

static void KheNHoodDebug(KHE_NHOOD nhood, int verbosity, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  /* fprintf(fp, "{"); */
  if( KheNHoodIsNull(nhood) )
    fprintf(fp, "null:null");
  else
  {
    KheResourceSetDebug(nhood.resources, verbosity, -1, fp);
    fprintf(fp, ":");
    KheIntervalSetDebug(nhood.days, fp);
  }
  /* fprintf(fp, "}"); */
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_NHOOD_SHAPE"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD_SHAPE KheNHoodShapeMake(int resource_count, int day_count)     */
/*                                                                           */
/*  Make a new neighbourhood shape object with these attributes.             */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD_SHAPE KheNHoodShapeMake(int resource_count, int day_count)
{
  KHE_NHOOD_SHAPE res;
  res.resource_count = resource_count;
  res.day_count = day_count;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD_SHAPE KheNHoodShapeParse(char **str, KHE_VLSN_SOLVER vs)       */
/*                                                                           */
/*  Parse a neighbourhood shape object according to the grammar              */
/*                                                                           */
/*    int ":" int                                                            */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD_SHAPE KheNHoodShapeParse(char **str, KHE_VLSN_SOLVER vs)
{
  int resource_count, day_count;
  resource_count = KheIntParse(str);
  KheStrSkip(str, ":");
  day_count = KheIntParse(str);
  return KheNHoodShapeMake(resource_count, day_count);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheNHoodShapeTypedCmp(KHE_NHOOD_SHAPE shape1, KHE_NHOOD_SHAPE shape2)*/
/*                                                                           */
/*  Typed comparison function which sorts shapes so that the number of       */
/*  resources is increasing, and for equal resources the number of days      */
/*  is increasing.                                                           */
/*                                                                           */
/*****************************************************************************/

static int KheNHoodShapeTypedCmp(KHE_NHOOD_SHAPE shape1, KHE_NHOOD_SHAPE shape2)
{
  int cmp;
  cmp = shape1.resource_count - shape2.resource_count;
  if( cmp != 0 )
    return cmp;
  else
    return shape1.day_count - shape2.day_count;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheNHoodShapeCmp(const void *t1, const void *t2)                     */
/*                                                                           */
/*  Untyped comparison function which sorts shapes.                          */
/*                                                                           */
/*****************************************************************************/

static int KheNHoodShapeCmp(const void *t1, const void *t2)
{
  KHE_NHOOD_SHAPE shape1 = * (KHE_NHOOD_SHAPE *) t1;
  KHE_NHOOD_SHAPE shape2 = * (KHE_NHOOD_SHAPE *) t2;
  return KheNHoodShapeTypedCmp(shape1, shape2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNHoodShapeDebug(KHE_NHOOD_SHAPE shape, int verbosity,            */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of shape onto fp with the given verbosity and indent.        */
/*                                                                           */
/*****************************************************************************/

static void KheNHoodShapeDebug(KHE_NHOOD_SHAPE shape, int verbosity,
  int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "%d:%d", shape.resource_count, shape.day_count);
  if( indent > 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_NHOOD_SHAPE_SET"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD_SHAPE_SET KheNHoodShapeSetMake(HA_ARENA a)                     */
/*                                                                           */
/*  Make a new, empty neighbourhood shape set object.                        */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD_SHAPE_SET KheNHoodShapeSetMake(HA_ARENA a)
{
  KHE_NHOOD_SHAPE_SET res;
  HaMake(res, a);
  HaArrayInit(res->shapes, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNHoodShapeSetAddNHoodShape(KHE_NHOOD_SHAPE_SET shape_set,        */
/*    KHE_NHOOD_SHAPE shape)                                                 */
/*                                                                           */
/*  Add shape to shape_set.                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheNHoodShapeSetAddNHoodShape(KHE_NHOOD_SHAPE_SET shape_set,
  KHE_NHOOD_SHAPE shape)
{
  HaArrayAddLast(shape_set->shapes, shape);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD_SHAPE KheNHoodShapeSetChooseNHoodShape(                        */
/*     KHE_NHOOD_SHAPE_SET shape_set, KHE_VLSN_SOLVER vs)                    */
/*                                                                           */
/*  Choose a random neighbourhood shape from shape_set.                      */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD_SHAPE KheNHoodShapeSetChooseNHoodShape(
   KHE_NHOOD_SHAPE_SET shape_set, KHE_VLSN_SOLVER vs)
{
  int index;
  HnAssert(HaArrayCount(shape_set->shapes) > 0,
    "KheNHoodShapeSetChooseNHoodShape internal error");
  index = KheRandomGeneratorNextRange(&vs->rgen,
    0, HaArrayCount(shape_set->shapes) - 1);
  return HaArray(shape_set->shapes, index);
}

/* *** old version that included a minimum resource count
static KHE_NHOOD_SHAPE KheNHoodShapeSetChooseNHoodShape(
   KHE_NHOOD_SHAPE_SET shape_set, int min_rcount, KHE_VLSN_SOLVER vs)
{
  KHE_NHOOD_SHAPE shape;  int i, pos;

  ** select a random shape whose rcount >= min_rcount **
  HaArrayForEach(shape_set->shapes, shape, i)
    if( shape.resource_count >= min_rcount )
    {
      ** can select anywhere from i to the end **
      pos = KheRandomGeneratorNextRange(&vs->rgen,
	i, HaArrayCount(shape_set->shapes) - 1);
      return HaArray(shape_set->shapes, pos);
    }

  ** no luck, so select the shape with the largest rcount **
  return HaArrayLast(shape_set->shapes);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD_SHAPE_SET KheNHoodShapeSetParse(char **str, KHE_VLSN_SOLVER vs)*/
/*                                                                           */
/*  Parse a neighbourhood shape set.  Sort the elements before returning it. */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD_SHAPE_SET KheNHoodShapeSetParse(char **str, KHE_VLSN_SOLVER vs)
{
  KHE_NHOOD_SHAPE_SET res;  KHE_NHOOD_SHAPE shape;
  res = KheNHoodShapeSetMake(vs->arena);
  shape = KheNHoodShapeParse(str, vs);
  KheNHoodShapeSetAddNHoodShape(res, shape);
  while( KheStrSkip(str, ",") )
  {
    shape = KheNHoodShapeParse(str, vs);
    KheNHoodShapeSetAddNHoodShape(res, shape);
  }
  HaArraySort(res->shapes, &KheNHoodShapeCmp);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNHoodShapeSetDebug(KHE_NHOOD_SHAPE_SET shape_set, int verbosity, */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of shape_set onto fp with the given verbosity and indent.    */
/*                                                                           */
/*****************************************************************************/

static void KheNHoodShapeSetDebug(KHE_NHOOD_SHAPE_SET shape_set, int verbosity,
  int indent, FILE *fp)
{
  KHE_NHOOD_SHAPE shape;  int i;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  HaArrayForEach(shape_set->shapes, shape, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    KheNHoodShapeDebug(shape, verbosity, -1, fp);
  }
  if( indent > 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_FOLLOWER_RESOURCE"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_FOLLOWER_RESOURCE KheFollowerResourceMake(KHE_RESOURCE r,            */
/*    int matches, float similarity, KHE_VLSN_SOLVER vs)                     */
/*                                                                           */
/*  Make a new follower resource object with these attributes.               */
/*                                                                           */
/*****************************************************************************/

static KHE_FOLLOWER_RESOURCE KheFollowerResourceMake(KHE_RESOURCE r,
  int matches, float similarity, KHE_VLSN_SOLVER vs)
{
  KHE_FOLLOWER_RESOURCE res;
  if( HaArrayCount(vs->follower_resource_free_list) > 0 )
    res = HaArrayLastAndDelete(vs->follower_resource_free_list);
  else
    HaMake(res, vs->arena);
  res->resource = r;
  res->matches = matches;
  res->similarity = similarity;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheFollowerResourceTypedCmp(KHE_FOLLOWER_RESOURCE fr1,               */
/*    KHE_FOLLOWER_RESOURCE fr2)                                             */
/*                                                                           */
/*  Typed comparison function for sorting follower resources by              */
/*  non-increasing similarity.                                               */
/*                                                                           */
/*****************************************************************************/

static int KheFollowerResourceTypedCmp(KHE_FOLLOWER_RESOURCE fr1,
  KHE_FOLLOWER_RESOURCE fr2)
{
  float cmp = fr2->similarity - fr1->similarity;
  return cmp < 0.0 ? -1 : cmp == 0.0 ? 0 : 1;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheFollowerResourceCmp(const void *t1, const void *t2)               */
/*                                                                           */
/*  Untyped comparison function for sorting follower resources by            */
/*  non-increasing similarity.                                               */
/*                                                                           */
/*****************************************************************************/

static int KheFollowerResourceCmp(const void *t1, const void *t2)
{
  KHE_FOLLOWER_RESOURCE fr1 = * (KHE_FOLLOWER_RESOURCE *) t1;
  KHE_FOLLOWER_RESOURCE fr2 = * (KHE_FOLLOWER_RESOURCE *) t2;
  return KheFollowerResourceTypedCmp(fr1, fr2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFollowerResourceDebug(KHE_FOLLOWER_RESOURCE fr,                  */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of fr onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheFollowerResourceDebug(KHE_FOLLOWER_RESOURCE fr,
  int verbosity, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "%s:%d", KheResourceId(fr->resource), fr->matches);
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_LEADER_RESOURCE"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_LEADER_RESOURCE KheLeaderResourceMake(KHE_RESOURCE r,                */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Make a new leader resource object.                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_LEADER_RESOURCE KheLeaderResourceMake(KHE_RESOURCE r,
  KHE_VLSN_SOLVER vs)
{
  KHE_LEADER_RESOURCE res;
  if( HaArrayCount(vs->leader_resource_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(vs->leader_resource_free_list);
    HaArrayClear(res->followers);
  }
  else
  {
    HaMake(res, vs->arena);
    HaArrayInit(res->followers, vs->arena);
  }
  res->resource = r;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLeaderResourceFree(KHE_LEADER_RESOURCE lr, KHE_VLSN_SOLVER vs)   */
/*                                                                           */
/*  Free lr by adding it to the free list in vs.                             */
/*                                                                           */
/*****************************************************************************/

static void KheLeaderResourceFree(KHE_LEADER_RESOURCE lr, KHE_VLSN_SOLVER vs)
{
  int i;
  HaArrayAppend(vs->follower_resource_free_list, lr->followers, i);
  HaArrayAddLast(vs->leader_resource_free_list, lr);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceDomainsSimilar(KHE_RESOURCE r1, KHE_RESOURCE r2,         */
/*    KHE_SOLN soln)                                                         */
/*                                                                           */
/*  Return true if at least 60% of the tasks assigned r1 and r2 can be       */
/*  moved from r1 to r2 or r2 to r1, as far as their domains are concerned.  */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceDomainsSimilar(KHE_RESOURCE r1, KHE_RESOURCE r2,
  KHE_SOLN soln, int *matches, float *similarity)
{
  int i, total;  KHE_TASK task;

  /* count r1's tasks that could be assigned r2 */
  total = 0;
  *matches = 0;
  for( i = 0;  i < KheResourceAssignedTaskCount(soln, r1);  i++ )
  {
    task = KheResourceAssignedTask(soln, r1, i);
    if( KheTaskIsProperRoot(task) )
    {
      total++;
      if( KheResourceGroupContains(KheTaskDomain(task), r2) )
	(*matches)++;
    }
  }

  /* count r2's tasks that could be assigned r1 */
  for( i = 0;  i < KheResourceAssignedTaskCount(soln, r2);  i++ )
  {
    task = KheResourceAssignedTask(soln, r2, i);
    if( KheTaskIsProperRoot(task) )
    {
      total++;
      if( KheResourceGroupContains(KheTaskDomain(task), r1) )
	(*matches)++;
    }
  }

  /* return true if at least 60% are movable */
  *similarity = (total == 0 ? 0.0 : ((float) *matches / (float) total));
  return *similarity >= 0.6;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceIsUnderloaded(KHE_RESOURCE r, KHE_SOLN soln)             */
/*                                                                           */
/*  Return true if r is underloaded in soln.                                 */
/*                                                                           */
/*  Implementation note.  The results of KheResourceAvailableBusyTimes and   */
/*  KheResourceAvailableWorkload are cached in soln, so there is no          */
/*  significant inefficiency in accessing them repeatedly.                   */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceIsUnderloaded(KHE_RESOURCE r, KHE_SOLN soln)
{
  int avail_int;  float avail_float;
  KheResourceAvailableBusyTimes(soln, r, &avail_int);
  KheResourceAvailableWorkload(soln, r, &avail_float);
  return avail_int > 0 && avail_float > 0.0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceIsOverloaded(KHE_RESOURCE r, KHE_SOLN soln)              */
/*                                                                           */
/*  Return true if r is overloaded in soln.                                  */
/*                                                                           */
/*  Implementation note.  The results of KheResourceAvailableBusyTimes and   */
/*  KheResourceAvailableWorkload are cached in soln, so there is no          */
/*  significant inefficiency in accessing them repeatedly.                   */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceIsOverloaded(KHE_RESOURCE r, KHE_SOLN soln)
{
  int avail_int;  float avail_float;
  KheResourceAvailableBusyTimes(soln, r, &avail_int);
  KheResourceAvailableWorkload(soln, r, &avail_float);
  return avail_int < 0 || avail_float < 0.0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceIsFollower(KHE_RESOURCE leader_r, KHE_RESOURCE r,        */
/*    KHE_FOLLOWER_TYPE ftype, KHE_SOLN soln, int *matches)                  */
/*                                                                           */
/*  If r is a follower of leader_r, return true and set *matches to some     */
/*  non-negative value.  Otherwise return false with *matches set to -1.     */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceIsFollower(KHE_RESOURCE leader_r, KHE_RESOURCE r,
  KHE_FOLLOWER_TYPE ftype, KHE_SOLN soln, int *matches, float *similarity)
{
  bool similar;

  /* r never follows itself */
  if( r == leader_r )
    return *matches = -1, false;

  /* work out whether leader_r and r are similar, and set *matches */
  similar = KheResourceDomainsSimilar(leader_r, r, soln, matches, similarity);

  /* the result depends on the follower type */
  switch( ftype )
  {
    case KHE_FOLLOWER_ANY:

      return true;

    case KHE_FOLLOWER_SIMILAR:

      return similar;

    case KHE_FOLLOWER_SIMILAR_AND_UNDERLOADED:

      return similar && KheResourceIsUnderloaded(r, soln);

    default:

      HnAbort("KheResourceSelectIsFollower internal error");
      return *matches = -1, false;   /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LEADER_RESOURCE KheLeaderResourceGet(KHE_RESOURCE r,                 */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Retrieve or make the leader resource object containing resources         */
/*  that are similar to r.                                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_LEADER_RESOURCE KheLeaderResourceGet(KHE_RESOURCE r,
  KHE_VLSN_SOLVER vs)
{
  int index, i, matches;  KHE_LEADER_RESOURCE res;  KHE_RESOURCE r2;
  float similarity;

  index = KheResourceResourceTypeIndex(r);
  HaArrayFill(vs->resources, index + 1, NULL);
  res = HaArray(vs->resources, index);
  if( res == NULL )
  {
    /* make the leader resource object and add follower resources to it */
    res = KheLeaderResourceMake(r, vs);
    for( i = 0;  i < KheResourceTypeResourceCount(vs->resource_type);  i++ )
    {
      r2 = KheResourceTypeResource(vs->resource_type, i);
      if( KheResourceIsFollower(r, r2, KHE_FOLLOWER_ANY, vs->soln,
	  &matches, &similarity) )
	HaArrayAddLast(res->followers,
	  KheFollowerResourceMake(r2, matches, similarity, vs));
    }

    /* sort the followers by non-increasing similarity */
    HaArraySort(res->followers, &KheFollowerResourceCmp);

    /* add the new leader resource object to vs->resources */
    HaArrayPut(vs->resources, index, res);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLeaderResourceDebug(KHE_LEADER_RESOURCE lr,                      */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug printf of lr onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheLeaderResourceDebug(KHE_LEADER_RESOURCE lr,
  int verbosity, int indent, FILE *fp)
{
  KHE_FOLLOWER_RESOURCE fr;  int i;
  fprintf(fp, "%*s[ Leader Resource %s\n", indent, "",
    KheResourceId(lr->resource));
  if( HaArrayCount(lr->followers) == 0 )
    fprintf(fp, "%*sno follower resources\n", indent + 2, "");
  else
  {
    HaArrayForEach(lr->followers, fr, i)
    {
      if( i == 0 )
	fprintf(fp, "%*s", indent + 2, "");
      else if( i % 8 == 0 )
	fprintf(fp, ",\n%*s", indent + 2, "");
      else
	fprintf(fp, ", ");
      KheFollowerResourceDebug(fr, 1, -1, fp);
    }
    fprintf(fp, "\n");
  }
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_AVAIL_RESOURCE"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_AVAIL_RESOURCE KheAvailResourceMake(KHE_RESOURCE r, KHE_SOLN soln,   */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Make an avail resource object with these attributes.                     */
/*                                                                           */
/*****************************************************************************/

static KHE_AVAIL_RESOURCE KheAvailResourceMake(KHE_RESOURCE r, KHE_SOLN soln,
  int non_rigidity, int frame_len, int random, HA_ARENA a)
{
  KHE_AVAIL_RESOURCE res;  KHE_TIME_GROUP hard_unavail_tg;
  HaMake(res, a);
  res->resource = r;
  res->random = random;
  KheResourceAvailableBusyTimes(soln, r, &res->avail_times);
  if( res->avail_times > frame_len )
    res->avail_times = frame_len;
  hard_unavail_tg = KheResourceHardUnavailableTimeGroup(r);
  res->non_rigidity = non_rigidity;
  res->unavail_times = KheTimeGroupTimeCount(hard_unavail_tg);
  res->index = KheResourceResourceTypeIndex(r);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvailResourceTypedCmp(KHE_AVAIL_RESOURCE ar1,                     */
/*    KHE_AVAIL_RESOURCE ar2)                                                */
/*                                                                           */
/*  Typed comparison function for sorting an array of KHE_AVAIL_RESOURCE     */
/*  objects by increasing non-rigidity and decreasing available times.       */
/*                                                                           */
/*****************************************************************************/

static int KheAvailResourceTypedCmp2(KHE_AVAIL_RESOURCE ar1,
  KHE_AVAIL_RESOURCE ar2)
{
  int cmp;

  /* first priority is decreasing available times */
  cmp = ar2->avail_times - ar1->avail_times;
  if( cmp != 0 )
    return cmp;

  /* after that, decreasing unavailable times, then randomness and index */
  cmp = ar2->unavail_times - ar1->unavail_times;
  if( cmp != 0 )
    return cmp;
  cmp = ar1->random - ar2->random;
  if( cmp != 0 )
    return cmp;
  return ar1->index - ar2->index;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvailResourceTypedCmp(KHE_AVAIL_RESOURCE ar1,                     */
/*    KHE_AVAIL_RESOURCE ar2)                                                */
/*                                                                           */
/*  Typed comparison function for sorting an array of KHE_AVAIL_RESOURCE     */
/*  objects by increasing non-rigidity and decreasing available times.       */
/*                                                                           */
/*****************************************************************************/

/* *** not currently used
static int KheAvailResourceTypedCmp(KHE_AVAIL_RESOURCE ar1,
  KHE_AVAIL_RESOURCE ar2)
{
  int cmp;

  ** first priority is increasing non-rigidity **
  cmp = ar1->non_rigidity - ar2->non_rigidity;
  if( cmp != 0 )
    return cmp;

  ** second priority is decreasing available times **
  cmp = ar2->avail_times - ar1->avail_times;
  if( cmp != 0 )
    return cmp;

  ** after that, decreasing unavailable times, then randomness and index **
  cmp = ar2->unavail_times - ar1->unavail_times;
  if( cmp != 0 )
    return cmp;
  cmp = ar1->random - ar2->random;
  if( cmp != 0 )
    return cmp;
  return ar1->index - ar2->index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheAvailResourceCmp(const void *t1, const void *t2)                  */
/*                                                                           */
/*  Untyped comparison function for sorting an array of KHE_AVAIL_RESOURCE   */
/*  objects by decreasing available times and increasing index.              */
/*                                                                           */
/*****************************************************************************/

/* *** not currently used
static int KheAvailResourceCmp(const void *t1, const void *t2)
{
  KHE_AVAIL_RESOURCE ar1 = * (KHE_AVAIL_RESOURCE *) t1;
  KHE_AVAIL_RESOURCE ar2 = * (KHE_AVAIL_RESOURCE *) t2;
  return KheAvailResourceTypedCmp(ar1, ar2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheAvailResourceCmp(const void *t1, const void *t2)                  */
/*                                                                           */
/*  Untyped comparison function for sorting an array of KHE_AVAIL_RESOURCE   */
/*  objects by decreasing available times and increasing index.              */
/*                                                                           */
/*****************************************************************************/

static int KheAvailResourceCmp2(const void *t1, const void *t2)
{
  KHE_AVAIL_RESOURCE ar1 = * (KHE_AVAIL_RESOURCE *) t1;
  KHE_AVAIL_RESOURCE ar2 = * (KHE_AVAIL_RESOURCE *) t2;
  return KheAvailResourceTypedCmp2(ar1, ar2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvailResourceDebug(KHE_AVAIL_RESOURCE ar, int verbosity,         */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of ar with the given verbosity and indent.                   */
/*                                                                           */
/*****************************************************************************/

static void KheAvailResourceDebug(KHE_AVAIL_RESOURCE ar, int verbosity,
  int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "[%s non_rigidity %d, avail %d, unavail %d, random %d, index %d]",
    KheResourceId(ar->resource), ar->non_rigidity, ar->avail_times,
    ar->unavail_times, ar->random, ar->index);
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_PREFER and resource selection"                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceIsAssignableFreeAndAvailableForTask(KHE_RESOURCE r,      */
/*    KHE_TASK task, KHE_VLSN_SOLVER vs)                                     */
/*                                                                           */
/*  Return true if r is assignable to task, free during the *days* that      */
/*  task is running, and available during the *times* that task is running.  */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceIsAssignableFreeAndAvailableForTask(KHE_RESOURCE r,
  KHE_TASK task, KHE_VLSN_SOLVER vs)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  if( !KheResourceGroupContains(KheTaskDomain(task), r) )
    return false;
  rtm = KheResourceTimetableMonitor(vs->soln, r);
  return KheResourceTimetableMonitorFreeForTask(rtm, task, vs->days_frame,
    NULL, true) && KheResourceTimetableMonitorAvailableForTask(rtm, task);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceIsAssignableFreeAndAvailableForTaskSet(KHE_RESOURCE r,   */
/*    KHE_TASK_SET ts, KHE_VLSN_SOLVER vs)                                   */
/*                                                                           */
/*  Return true if r is assignable, free and available for at least one      */
/*  (not all) of the tasks of ts.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceIsAssignableFreeAndAvailableForTaskSet(KHE_RESOURCE r,
  KHE_TASK_SET ts, KHE_VLSN_SOLVER vs)
{
  KHE_TASK task;  int i;
  for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
  {
    task = KheTaskSetTask(ts, i);
    if( KheResourceIsAssignableFreeAndAvailableForTask(r, task, vs) )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceIsFreeForTimeGroup(KHE_RESOURCE r, KHE_TIME_GROUP tg,    */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Return true if r is free during the days covering tg.                    */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceIsFreeForTimeGroup(KHE_RESOURCE r, KHE_TIME_GROUP tg,
  KHE_VLSN_SOLVER vs)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  rtm = KheResourceTimetableMonitor(vs->soln, r);
  return KheResourceTimetableMonitorFreeForTimeGroup(rtm, tg,
    vs->days_frame, NULL, true);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSatisfiesSameShiftRule(KHE_RESOURCE r,                   */
/*    KHE_INTERVAL in, int offset, KHE_VLSN_SOLVER vs)                       */
/*                                                                           */
/*  Return true if r satisfies the same shift rule:                          */
/*                                                                           */
/*    * All r's shifts during the days of "in" have the given offset;        */
/*                                                                           */
/*    * r has at least one shift during the chosen days with that offset.    */
/*                                                                           */
/*  As a special case, if offset == -1, return true.                         */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceSatisfiesSameShiftRule(KHE_RESOURCE r,
  KHE_INTERVAL in, int offset, KHE_VLSN_SOLVER vs)
{
  int offset_count, i, j;  KHE_TIME_GROUP tg;  KHE_TIME t;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  if( offset == -1 )
    return true;
  rtm = KheResourceTimetableMonitor(vs->soln, r);
  offset_count = 0;
  for( i = in.first;  i <= in.last;  i++ )
  {
    tg = KheFrameTimeGroup(vs->days_frame, i);
    for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
    {
      t = KheTimeGroupTime(tg, j);
      if( KheResourceTimetableMonitorTimeTaskCount(rtm, t) > 0 )
      {
	/* r is busy at t */
	if( j == offset )
	  offset_count++;
	else
	  return false;  /* r is busy outside offset */
      }
    }
  }
  return offset_count > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceBelowLimit(KHE_RESOURCE r,                               */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm, KHE_VLSN_SOLVER vs)               */
/*                                                                           */
/*  Return true if r is strictly below the upper limit on its version        */
/*  of cbtm (which itself monitors some other resource).                     */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceBelowLimit(KHE_RESOURCE r,
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm, KHE_VLSN_SOLVER vs)
{
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc, cbtc_r;  int offset, i;
  KHE_MONITOR m;  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm_r;
  int active_group_count, open_group_count, minimum, maximum;  bool allow_zero;
  KHE_CLUSTER_CONSTRAINT_SET cs;

  /* get the constraint, offset, and constraint set we are looking for */
  cbtc = KheClusterBusyTimesMonitorConstraint(cbtm);
  offset = KheClusterBusyTimesMonitorOffset(cbtm);
  cs = KheClusterGrouperFindConstraintSet(vs->cluster_grouper, cbtc);

  /* search for cbtm_r, which is r's version of cbtm */
  for( i = 0;  i < KheSolnResourceMonitorCount(vs->soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(vs->soln, r, i);
    if( KheMonitorTag(m) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG )
    {
      cbtm_r = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
      cbtc_r = KheClusterBusyTimesMonitorConstraint(cbtm_r);
      if( KheConstraintSetContains(cs, cbtc_r) &&
          KheClusterBusyTimesMonitorOffset(cbtm_r) == offset )
      {
	/* cbtm_r is one of the monitors we are looking for */
	KheClusterBusyTimesMonitorActiveTimeGroupCount(cbtm_r,
	  &active_group_count, &open_group_count, &minimum, &maximum,
	  &allow_zero);
	if( active_group_count >= maximum )
	{
	  if( DEBUG17 )
	    fprintf(stderr, "  vlsn not preferring %s (%d >= %d) for %s\n",
	      KheResourceId(r), active_group_count, maximum,
	      KheMonitorId((KHE_MONITOR) cbtm));
	  return false;
	}
      }
    }
  }

  /* all monitors OK (possibly because there are none) */
  if( DEBUG17 )
    fprintf(stderr, "  vlsn preferring %s for %s\n",
      KheResourceId(r), KheMonitorId((KHE_MONITOR) cbtm));
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PREFER KhePreferAny(KHE_PREFER_ANY p)                                */
/*  KHE_PREFER KhePreferOverloaded(KHE_PREFER_OVERLOADED p)                  */
/*  KHE_PREFER KhePreferBusyForTimeGroup(KHE_PREFER_BUSY_FOR_TIME_GROUP p,   */
/*    KHE_TIME_GROUP tg, KHE_INTERVAL in, int offset)                        */
/*  KHE_PREFER KhePreferBusyForTimeGroup2(KHE_PREFER_BUSY_FOR_TIME_GROUP2 p, */
/*    KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2, KHE_INTERVAL in, int offset)   */
/*  KHE_PREFER KhePreferFreeForTaskSet(KHE_PREFER_FREE_FOR_TASK_SET p,       */
/*    KHE_TASK_SET ts, KHE_INTERVAL in, int offset)                          */
/*  KHE_PREFER KhePreferFreeForTask(KHE_PREFER_FREE_FOR_TASK p, KHE_TASK t)  */
/*  KHE_PREFER KhePreferFreeForTaskAndPreferred(                             */
/*    KHE_PREFER_FREE_FOR_TASK_AND_PREFERRED p, KHE_TASK t,                  */
/*    KHE_RESOURCE_GROUP rg)                                                 */
/*  KHE_PREFER KhePreferFreeForTaskAndUnPreferred(                           */
/*    KHE_PREFER_FREE_FOR_TASK_AND_UNPREFERRED p, KHE_TASK t,                */
/*    KHE_RESOURCE_GROUP rg)                                                 */
/*                                                                           */
/*  Make a prefer object with these attributes.                              */
/*                                                                           */
/*****************************************************************************/

static KHE_PREFER KhePreferAny(KHE_PREFER_ANY p)
{
  p->tag = KHE_PREFER_ANY_TAG;
  return (KHE_PREFER) p;
}

static KHE_PREFER KhePreferOverloaded(KHE_PREFER_OVERLOADED p)
{
  p->tag = KHE_PREFER_OVERLOADED_TAG;
  return (KHE_PREFER) p;
}

static KHE_PREFER KhePreferBusyForTimeGroup(KHE_PREFER_BUSY_FOR_TIME_GROUP p,
  KHE_TIME_GROUP tg, KHE_INTERVAL in, int offset)
{
  p->tag = KHE_PREFER_BUSY_FOR_TIME_GROUP_TAG;
  p->tg = tg;
  p->in = in;
  p->offset = offset;
  return (KHE_PREFER) p;
}

static KHE_PREFER KhePreferBusyForTimeGroup2(KHE_PREFER_BUSY_FOR_TIME_GROUP2 p,
  KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2, KHE_INTERVAL in, int offset)
{
  p->tag = KHE_PREFER_BUSY_FOR_TIME_GROUP2_TAG;
  p->tg1 = tg1;
  p->tg2 = tg2;
  p->in = in;
  p->offset = offset;
  return (KHE_PREFER) p;
}

static KHE_PREFER KhePreferFreeForTaskSet(KHE_PREFER_FREE_FOR_TASK_SET p,
  KHE_TASK_SET ts, KHE_INTERVAL in, int offset)
{
  p->tag = KHE_PREFER_FREE_FOR_TASK_SET_TAG;
  p->ts = ts;
  p->in = in;
  p->offset = offset;
  return (KHE_PREFER) p;
}

static KHE_PREFER KhePreferFreeForTaskSetAndBelowLimit(
  KHE_PREFER_FREE_FOR_TASK_SET_AND_BELOW_LIMIT p,
  KHE_TASK_SET ts, KHE_INTERVAL in, int offset,
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm)
{
  p->tag = KHE_PREFER_FREE_FOR_TASK_SET_AND_BELOW_LIMIT_TAG;
  p->ts = ts;
  p->in = in;
  p->offset = offset;
  p->cbtm = cbtm;
  return (KHE_PREFER) p;
}

static KHE_PREFER KhePreferFreeForTask(KHE_PREFER_FREE_FOR_TASK p, KHE_TASK t)
{
  p->tag = KHE_PREFER_FREE_FOR_TASK_TAG;
  p->t = t;
  return (KHE_PREFER) p;
}

static KHE_PREFER KhePreferFreeForTaskAndPreferred(
  KHE_PREFER_FREE_FOR_TASK_AND_PREFERRED p, KHE_TASK t, KHE_RESOURCE_GROUP rg)
{
  p->tag = KHE_PREFER_FREE_FOR_TASK_AND_PREFERRED_TAG;
  p->t = t;
  p->rg = rg;
  return (KHE_PREFER) p;
}

static KHE_PREFER KhePreferFreeForTaskAndUnPreferred(
  KHE_PREFER_FREE_FOR_TASK_AND_UNPREFERRED p, KHE_TASK t, KHE_RESOURCE_GROUP rg)
{
  p->tag = KHE_PREFER_FREE_FOR_TASK_AND_UNPREFERRED_TAG;
  p->t = t;
  p->rg = rg;
  return (KHE_PREFER) p;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePreferTest(KHE_PREFER p, KHE_RESOURCE r, KHE_VLSN_SOLVER vs)     */
/*                                                                           */
/*  Carry out the prefer test represented by p.                              */
/*                                                                           */
/*****************************************************************************/

static bool KhePreferTest(KHE_PREFER p, KHE_RESOURCE r, KHE_VLSN_SOLVER vs)
{
  KHE_PREFER_BUSY_FOR_TIME_GROUP pbtg;
  KHE_PREFER_BUSY_FOR_TIME_GROUP2 pbtg2;
  KHE_PREFER_FREE_FOR_TASK_SET pfts;
  KHE_PREFER_FREE_FOR_TASK pft;
  KHE_PREFER_FREE_FOR_TASK_AND_PREFERRED pftp;
  KHE_PREFER_FREE_FOR_TASK_AND_UNPREFERRED pftu;
  KHE_PREFER_FREE_FOR_TASK_SET_AND_BELOW_LIMIT pftsbl;
  int busy;  float work;
  switch( p->tag )
  {
    case KHE_PREFER_ANY_TAG:

      return true;

    case KHE_PREFER_OVERLOADED_TAG:

      if( KheResourceAvailableBusyTimes(vs->soln, r, &busy) && busy < 0 )
	return true;
      else if( KheResourceAvailableWorkload(vs->soln, r, &work) && work < 0 )
	return true;
      else
	return false;

    case KHE_PREFER_BUSY_FOR_TIME_GROUP_TAG:

      pbtg = (KHE_PREFER_BUSY_FOR_TIME_GROUP) p;
      return !KheResourceIsFreeForTimeGroup(r, pbtg->tg, vs) &&
        KheResourceSatisfiesSameShiftRule(r, pbtg->in, pbtg->offset, vs);

    case KHE_PREFER_BUSY_FOR_TIME_GROUP2_TAG:

      pbtg2 = (KHE_PREFER_BUSY_FOR_TIME_GROUP2) p;
      return ( !KheResourceIsFreeForTimeGroup(r, pbtg2->tg1, vs) ||
	       !KheResourceIsFreeForTimeGroup(r, pbtg2->tg2, vs) )
	&& KheResourceSatisfiesSameShiftRule(r, pbtg2->in, pbtg2->offset, vs);

    case KHE_PREFER_FREE_FOR_TASK_SET_TAG:

      pfts = (KHE_PREFER_FREE_FOR_TASK_SET) p;
      return KheResourceIsAssignableFreeAndAvailableForTaskSet(r, pfts->ts, vs)
	&& KheResourceSatisfiesSameShiftRule(r, pfts->in, pfts->offset, vs);

    case KHE_PREFER_FREE_FOR_TASK_SET_AND_BELOW_LIMIT_TAG:

      pftsbl = (KHE_PREFER_FREE_FOR_TASK_SET_AND_BELOW_LIMIT) p;
      return KheResourceIsAssignableFreeAndAvailableForTaskSet(r, pftsbl->ts,vs)
	&& KheResourceSatisfiesSameShiftRule(r, pftsbl->in, pftsbl->offset, vs)
	&& KheResourceBelowLimit(r, pftsbl->cbtm, vs);

    case KHE_PREFER_FREE_FOR_TASK_TAG:

      pft = (KHE_PREFER_FREE_FOR_TASK) p;
      return KheResourceIsAssignableFreeAndAvailableForTask(r, pft->t, vs);

    case KHE_PREFER_FREE_FOR_TASK_AND_PREFERRED_TAG:

      pftp = (KHE_PREFER_FREE_FOR_TASK_AND_PREFERRED) p;
      return KheResourceIsAssignableFreeAndAvailableForTask(r, pftp->t, vs)
	&& KheResourceGroupContains(pftp->rg, r);

    case KHE_PREFER_FREE_FOR_TASK_AND_UNPREFERRED_TAG:

      pftu = (KHE_PREFER_FREE_FOR_TASK_AND_UNPREFERRED) p;
      return KheResourceIsAssignableFreeAndAvailableForTask(r, pftu->t, vs)
	&& !KheResourceGroupContains(pftu->rg, r);

    default:

      HnAbort("KhePreferTest internal error (tag %d)", p->tag);
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SET KheVLSNSolverSelectResources(KHE_VLSN_SOLVER vs,        */
/*    int count, KHE_RESOURCE include_r)                                     */
/*                                                                           */
/*  Assuming that vs->preferred_resources has been initialized to a set of   */
/*  preferred resources not containing include_r, and vs->other_resources    */
/*  has been initialized to a set of other resources not containing          */
/*  include_r, select count resources from these sets, preferring            */
/*  resources from vs->preferred resources.  If include_r != NULL, always    */
/*  include include_r as one of the count resources.                         */
/*                                                                           */
/*  The selected resources are returned in a scratch resource set from vs.   */
/*                                                                           */
/*****************************************************************************/

/* *** merged into the following function
static KHE_RESOURCE_SET KheVLSNSolverDoSelectResources(KHE_VLSN_SOLVER vs,
  int count, KHE_RESOURCE include_r)
{
  int index, preferred_count, other_count;  KHE_RESOURCE r;
  KheResourceSetClear(vs->scratch_rs);
  if( include_r != NULL )
    KheResourceSetAddResource(vs->scratch_rs, include_r);
  while( KheResourceSetResourceCount(vs->scratch_rs) < count )
  {
    preferred_count = HaArrayCount(vs->preferred_resources);
    other_count = HaArrayCount(vs->other_resources);
    if( preferred_count > 0 )
    {
      ** choose a preferred resource **
      index = KheRandomGeneratorNextRange(&vs->rgen, 0, preferred_count - 1);
      r = HaArray(vs->preferred_resources, index);
      KheResourceSetAddResource(vs->scratch_rs, r);
      HaArrayDeleteAndPlug(vs->preferred_resources, index);
    }
    else if( other_count > 0 )
    {
      ** no preferred resources left, so choose an other resource **
      index = KheRandomGeneratorNextRange(&vs->rgen, 0, other_count - 1);
      r = HaArray(vs->other_resources, index);
      KheResourceSetAddResource(vs->scratch_rs, r);
      HaArrayDeleteAndPlug(vs->other_resources, index);
    }
    else
    {
      ** all resources taken, so quit **
      break;
    }
  }
  return vs->scratch_rs;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SET KheVLSNSolverResources(KHE_VLSN_SOLVER vs,              */
/*    int count, KHE_RESOURCE leader_r, KHE_PREFERRED_FN prefer)             */
/*                                                                           */
/*  Return a resource set containing (no more than) count resources,         */
/*  similar to and including leader_r, with preference given to resources    */
/*  that elicit a true result from function prefer.                          */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_SET KheVLSNSolverResources(KHE_VLSN_SOLVER vs,
  int count, KHE_RESOURCE leader_r, KHE_PREFER prefer)
{
  int i, stop_index;  KHE_FOLLOWER_RESOURCE fr;
  int index, preferred_count, other_count;  KHE_RESOURCE r;
  KHE_LEADER_RESOURCE lr;

  /* divide up to 5*(count - 1) followers into preferred and other */
  HaArrayClear(vs->preferred_resources);
  HaArrayClear(vs->other_resources);
  lr = KheLeaderResourceGet(leader_r, vs);
  stop_index = min(5*(count - 1), HaArrayCount(lr->followers));
  for( i = 0;  i < stop_index;  i++ )
  {
    fr = HaArray(lr->followers, i);
    if( KhePreferTest(prefer, fr->resource, vs) )
      HaArrayAddLast(vs->preferred_resources, fr->resource);
    else
      HaArrayAddLast(vs->other_resources, fr->resource);
  }

  /* make the choices and store them in vs->scratch_rs */
  KheResourceSetClear(vs->scratch_rs);
  KheResourceSetAddResource(vs->scratch_rs, leader_r);
  while( KheResourceSetResourceCount(vs->scratch_rs) < count )
  {
    preferred_count = HaArrayCount(vs->preferred_resources);
    other_count = HaArrayCount(vs->other_resources);
    if( preferred_count > 0 )
    {
      /* choose a preferred resource */
      index = KheRandomGeneratorNextRange(&vs->rgen, 0, preferred_count - 1);
      r = HaArray(vs->preferred_resources, index);
      KheResourceSetAddResource(vs->scratch_rs, r);
      HaArrayDeleteAndPlug(vs->preferred_resources, index);
    }
    else if( other_count > 0 )
    {
      /* no preferred resources left, so choose an other resource */
      index = KheRandomGeneratorNextRange(&vs->rgen, 0, other_count - 1);
      r = HaArray(vs->other_resources, index);
      KheResourceSetAddResource(vs->scratch_rs, r);
      HaArrayDeleteAndPlug(vs->other_resources, index);
    }
    else
    {
      /* all resources taken, so quit */
      break;
    }
  }
  return vs->scratch_rs;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "helper functions for targeted VLSN search"                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheMakeResourceBusy(KHE_RESOURCE r, KHE_TIME_GROUP tg,         */
/*    int offset, KHE_NHOOD_SHAPE shape, KHE_VLSN_SOLVER vs)                 */
/*                                                                           */
/*  Find a neighbourhood suited to making r busy during tg.                  */
/*                                                                           */
/*  If offset != -1, prefer resources busy only at times with this offset.   */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheMakeResourceBusy(KHE_RESOURCE r, KHE_TIME_GROUP tg,
  int offset, KHE_NHOOD_SHAPE shape, KHE_VLSN_SOLVER vs)
{
  KHE_INTERVAL in;  KHE_RESOURCE_SET rs;
  struct khe_prefer_busy_for_time_group_rec pbtg;

  /* let D be the usual conversion of the set of days covering tg */
  in = KheTimeGroupInterval(tg, vs->days_frame);
  in = KheIntervalAdjustLength(in, shape.day_count, vs);
  KheIntervalSetResetSingle(vs->scratch_is, in);

  /* choose r plus r_i - 1 similar resources, preferring busy resources */
  rs = KheVLSNSolverResources(vs, shape.resource_count, r,
    KhePreferBusyForTimeGroup(&pbtg, tg, in, offset));
  return KheNHoodMake(rs, vs->scratch_is);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheMakeResourceBusy2(KHE_RESOURCE r, KHE_TIME_GROUP tg1,       */
/*    KHE_TIME_GROUP tg2, int offset, KHE_NHOOD_SHAPE shape,                 */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Like KheMakeResourceBusy, except that the neighbourhood's days need to   */
/*  cover two time groups, and we prefer resources that are busy for at      */
/*  least one of those time groups.                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheMakeResourceBusy2(KHE_RESOURCE r, KHE_TIME_GROUP tg1,
  KHE_TIME_GROUP tg2, int offset, KHE_NHOOD_SHAPE shape,
  KHE_VLSN_SOLVER vs)
{
  KHE_INTERVAL in, in1, in2;  KHE_RESOURCE_SET rs;
  struct khe_prefer_busy_for_time_group2_rec pbtg2;

  /* let D be the usual conversion of the set of days covering tg1 and tg2 */
  in1 = KheTimeGroupInterval(tg1, vs->days_frame);
  in2 = KheTimeGroupInterval(tg2, vs->days_frame);
  in = KheIntervalUnion(in1, in2);
  in = KheIntervalAdjustLength(in, shape.day_count, vs);
  KheIntervalSetResetSingle(vs->scratch_is, in);

  /* choose r plus r_i - 1 similar resources, preferring busy resources */
  rs = KheVLSNSolverResources(vs, shape.resource_count, r,
    KhePreferBusyForTimeGroup2(&pbtg2, tg1, tg2, in, offset));
  return KheNHoodMake(rs, vs->scratch_is);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheMakeResourceFree(KHE_RESOURCE r, KHE_TIME_GROUP tg,         */
/*    int offset, KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm, KHE_NHOOD_SHAPE shape,*/
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Find a neighbourhood suited to making r free during tg.                  */
/*                                                                           */
/*  If offset != -1, prefer resources busy only at times with this offset.   */
/*  If cbtm != NULL, we prefer resources that are underloaded at cbtm.       */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheMakeResourceFree(KHE_RESOURCE r, KHE_TIME_GROUP tg,
  int offset, KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm, KHE_NHOOD_SHAPE shape,
  KHE_VLSN_SOLVER vs)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_INTERVAL in;  KHE_RESOURCE_SET rs;
  struct khe_prefer_free_for_task_set_rec pfts;
  struct khe_prefer_free_for_task_set_and_below_limit_rec pftsbl;
  KHE_PREFER prefer;

  /* find the tasks that are making r busy during tg */
  KheTaskSetClear(vs->tasks);
  rtm = KheResourceTimetableMonitor(vs->soln, r);
  KheResourceTimetableMonitorAddProperRootTasksInTimeGroup(rtm, tg, true,
    vs->tasks);
  if( DEBUG12 && KheTaskSetTaskCount(vs->tasks) == 0 )
  {
    fprintf(stderr, "KheMakeResourceFree(%s, ", KheResourceId(r));
    KheTimeGroupDebug(tg, 2, -1, stderr);
    fprintf(stderr, ", %d, shape, vs) failing\n", offset);
    KheResourceTimetableMonitorPrintTimetable(rtm, vs->days_frame,
      10, 2, stderr);
  }
  HnAssert(KheTaskSetTaskCount(vs->tasks) > 0,
    "KheMakeResourceFree internal error");

  /* let D be the usual conversion of the days containing the tasks' times */
  in = KheTaskSetInterval(vs->tasks, vs->days_frame);
  in = KheIntervalAdjustLength(in, shape.day_count, vs);
  KheIntervalSetResetSingle(vs->scratch_is, in);

  /* choose r plus r_i - 1 similar resources, preferring free resources */
  /* NB KhePreferFreeForTaskSet prefers resources that are free for at  */
  /* least one task pf vs->tasks, not resources that are free for all   */
  if( cbtm == NULL )
    prefer = KhePreferFreeForTaskSet(&pfts, vs->tasks, in, offset);
  else
    prefer = KhePreferFreeForTaskSetAndBelowLimit(&pftsbl, vs->tasks,
      in, offset, cbtm);
  rs = KheVLSNSolverResources(vs, shape.resource_count, r, prefer);
  return KheNHoodMake(rs, vs->scratch_is);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheMakeResourceFree2(KHE_RESOURCE r, KHE_TIME_GROUP tg1,       */
/*    KHE_TIME_GROUP tg2, int offset, KHE_NHOOD_SHAPE shape,                 */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Like KheMakeResourceFree only covering two time groups.                  */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheMakeResourceFree2(KHE_RESOURCE r, KHE_TIME_GROUP tg1,
  KHE_TIME_GROUP tg2, int offset, KHE_NHOOD_SHAPE shape,
  KHE_VLSN_SOLVER vs)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_INTERVAL in, in1, in2;
  KHE_RESOURCE_SET rs;  struct khe_prefer_free_for_task_set_rec pfts;

  /* find the tasks that are making r busy during tg1 and tg2 */
  KheTaskSetClear(vs->tasks);
  rtm = KheResourceTimetableMonitor(vs->soln, r);
  KheResourceTimetableMonitorAddProperRootTasksInTimeGroup(rtm, tg1, true,
    vs->tasks);
  KheResourceTimetableMonitorAddProperRootTasksInTimeGroup(rtm, tg2, true,
    vs->tasks);
  if( KheTaskSetTaskCount(vs->tasks) == 0 )
    KheNHoodMakeNull(); /* this can happen; we don't include all time groups */

  /* let D be the usual conversion of the days containing the tasks' times */
  in1 = KheTimeGroupInterval(tg1, vs->days_frame);
  in2 = KheTimeGroupInterval(tg2, vs->days_frame);
  in = KheIntervalUnion(in1, in2);
  in = KheIntervalAdjustLength(in, shape.day_count, vs);
  KheIntervalSetResetSingle(vs->scratch_is, in);

  /* choose r plus r_i - 1 similar resources, preferring free resources */
  /* NB KhePreferFreeForTaskSet prefers resources that are free for at  */
  /* least one task pf vs->tasks, not resources that are free for all   */
  rs = KheVLSNSolverResources(vs, shape.resource_count, r,
    KhePreferFreeForTaskSet(&pfts, vs->tasks, in, offset));
  return KheNHoodMake(rs, vs->scratch_is);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheMakeResourceBusyOrFree(bool make_busy, KHE_RESOURCE r,      */
/*    KHE_TIME_GROUP tg, int offset, KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm,    */
/*    KHE_NHOOD_SHAPE shape, KHE_VLSN_SOLVER vs)                             */
/*                                                                           */
/*  Call either KheMakeResourceBusy or KheMakeResourceFree, depending on     */
/*  make_busy.                                                               */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheMakeResourceBusyOrFree(bool make_busy, KHE_RESOURCE r,
  KHE_TIME_GROUP tg, int offset, KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm,
  KHE_NHOOD_SHAPE shape, KHE_VLSN_SOLVER vs)
{
  if( make_busy )
    return KheMakeResourceBusy(r, tg, offset, shape, vs);
  else
    return KheMakeResourceFree(r, tg, offset, cbtm, shape, vs);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheMakeResourceLessBusy(KHE_RESOURCE r, KHE_TIME_GROUP tg,     */
/*    KHE_NHOOD_SHAPE shape, KHE_VLSN_SOLVER vs)                             */
/*                                                                           */
/*  Find a neighbourhood suited to making r less busy during tg.             */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheMakeResourceLessBusy(KHE_RESOURCE r, KHE_TIME_GROUP tg,
  KHE_NHOOD_SHAPE shape, KHE_VLSN_SOLVER vs)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_INTERVAL in;  KHE_TASK task;
  int index, count;  KHE_RESOURCE_SET rs;
  struct khe_prefer_free_for_task_rec pft;

  /* find the proper root tasks that are making r busy during tg */
  KheTaskSetClear(vs->tasks);
  rtm = KheResourceTimetableMonitor(vs->soln, r);
  KheResourceTimetableMonitorAddProperRootTasksInTimeGroup(rtm, tg, true,
    vs->tasks);
  HnAssert(KheTaskSetTaskCount(vs->tasks) > 0,
    "KheMakeResourceFree internal error");

  /* choose one of those proper root tasks */
  count = KheTaskSetTaskCount(vs->tasks);
  HnAssert(count > 0, "KheMakeResourceLessBusy internal error");
  index = KheRandomGeneratorNextRange(&vs->rgen, 0, count - 1);
  task = KheTaskSetTask(vs->tasks, index);

  /* let D be the usual conversion of the days containing task's times */
  in = KheTaskInterval(task, vs->days_frame);
  in = KheIntervalAdjustLength(in, shape.day_count, vs);
  KheIntervalSetResetSingle(vs->scratch_is, in);

  /* choose r plus r_i - 1 similar resources, preferring free resources */
  rs = KheVLSNSolverResources(vs, shape.resource_count, r,
    KhePreferFreeForTask(&pft, task));
  return KheNHoodMake(rs, vs->scratch_is);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_VLSN_ASSIGN_RESOURCE_MONITOR"                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_ASSIGN_RESOURCE_MONITOR KheVLSNAssignResourceMonitorMake(       */
/*    KHE_ASSIGN_RESOURCE_MONITOR arm, KHE_VLSN_SOLVER vs)                   */
/*                                                                           */
/*  Make a VLSN assign resource monitor object.                              */
/*                                                                           */
/*****************************************************************************/

static KHE_VLSN_ASSIGN_RESOURCE_MONITOR KheVLSNAssignResourceMonitorMake(
  KHE_ASSIGN_RESOURCE_MONITOR arm, KHE_VLSN_SOLVER vs)
{
  KHE_VLSN_ASSIGN_RESOURCE_MONITOR res;
  HaMake(res, vs->arena);
  res->tag = KHE_VLSN_ASSIGN_RESOURCE_MONITOR_TAG;
  res->monitor = (KHE_MONITOR) arm;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheChooseResource(KHE_TASK t, KHE_RESOURCE_GROUP rg,        */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Choose a resource that is assignable, free, and available for t.         */
/*  If rg is non-NULL the chosen resource must also be an element of rg.     */
/*  Return NULL if there is no such resource.                                */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE KheChooseResource(KHE_TASK t, KHE_RESOURCE_GROUP rg,
  KHE_VLSN_SOLVER vs)
{
  KHE_RESOURCE_GROUP domain;  int i, count, index;  KHE_RESOURCE r;

  /* set vs->scratch_rs to t's assignable, free, and available resources */
  domain = KheTaskDomain(t);
  KheResourceSetClear(vs->scratch_rs);
  for( i = 0;  i < KheResourceGroupResourceCount(domain);  i++ )
  {
    r = KheResourceGroupResource(domain, i);
    if( KheResourceIsAssignableFreeAndAvailableForTask(r, t, vs)
	&& (rg == NULL || KheResourceGroupContains(rg, r)) )
      KheResourceSetAddResource(vs->scratch_rs, r);
  }

  /* choose and return one of vs->scratch_rs's resources */
  count = KheResourceSetResourceCount(vs->scratch_rs);
  if( count == 0 )
    return NULL;
  index = KheRandomGeneratorNextRange(&vs->rgen, 0, count - 1);
  return KheResourceSetResource(vs->scratch_rs, index);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheVLSNAssignResourceMonitorNextNHood(                         */
/*    KHE_VLSN_ASSIGN_RESOURCE_MONITOR varm,KHE_SELECT_TARGETED rst)         */
/*                                                                           */
/*  Return a random neighbourhood that addresses a defect in this monitor.   */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheVLSNAssignResourceMonitorNextNHood(
  KHE_VLSN_ASSIGN_RESOURCE_MONITOR varm, KHE_SELECT_TARGETED rst)
{
  KHE_EVENT_RESOURCE arm_er;  int i, index, count;  KHE_TASK task;
  KHE_ASSIGN_RESOURCE_MONITOR arm;  KHE_VLSN_SOLVER vs;  KHE_RESOURCE r;
  KHE_NHOOD_SHAPE shape;  KHE_INTERVAL in;  KHE_RESOURCE_SET rs;
  struct khe_prefer_free_for_task_rec pft;

  /* boilerplate */
  vs = rst->solver;
  arm = (KHE_ASSIGN_RESOURCE_MONITOR) varm->monitor;
  arm_er = KheAssignResourceMonitorEventResource(arm);

  /* choose an (r_i, d_i) */
  shape = KheNHoodShapeSetChooseNHoodShape(rst->shape_set, vs);

  /* choose an unassigned task monitored by arm */
  KheTaskSetClear(vs->tasks);
  for( i = 0;  i < KheEventResourceTaskCount(vs->soln, arm_er);  i++ )
  {
    task = KheEventResourceTask(vs->soln, arm_er, i);
    if( KheTaskAsstResource(task) == NULL )
      KheTaskSetAddTask(vs->tasks, task);
  }
  count = KheTaskSetTaskCount(vs->tasks);
  HnAssert(count > 0, "KheVLSNAssignResourceMonitorNextNHood internal error");
  index = KheRandomGeneratorNextRange(&vs->rgen, 0, count - 1);
  task = KheTaskSetTask(vs->tasks, index);
  task = KheTaskProperRoot(task);

  /* let D be the usual conversion of the set of days that task is running */
  in = KheTaskInterval(task, vs->days_frame);
  in = KheIntervalAdjustLength(in, shape.day_count, vs);
  KheIntervalSetResetSingle(vs->scratch_is, in);

  /* choose a resource that could be assigned to task */
  r = KheChooseResource(task, NULL, vs);
  if( r == NULL )
    return KheNHoodMakeNull();

  /* choose similar resources, preferring assignable, free and avail for task */
  rs = KheVLSNSolverResources(vs, shape.resource_count, r,
    KhePreferFreeForTask(&pft, task));
  return KheNHoodMake(rs, vs->scratch_is);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_VLSN_PREFER_RESOURCES_MONITOR"                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_PREFER_RESOURCES_MONITOR KheVLSNPreferResourcesMonitorMake(     */
/*    KHE_PREFER_RESOURCES_MONITOR prm, KHE_VLSN_SOLVER vs)                  */
/*                                                                           */
/*  Make a VLSN prefer resources monitor object.                             */
/*                                                                           */
/*****************************************************************************/

static KHE_VLSN_PREFER_RESOURCES_MONITOR KheVLSNPreferResourcesMonitorMake(
  KHE_PREFER_RESOURCES_MONITOR prm, KHE_VLSN_SOLVER vs)
{
  KHE_VLSN_PREFER_RESOURCES_MONITOR res;
  HaMake(res, vs->arena);
  res->tag = KHE_VLSN_PREFER_RESOURCES_MONITOR_TAG;
  res->monitor = (KHE_MONITOR) prm;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheVLSNPreferResourcesMonitorNextNHood(                        */
/*   KHE_VLSN_PREFER_RESOURCES_MONITOR vprm,KHE_SELECT_TARGETED rst)         */
/*                                                                           */
/*  Return a random neighbourhood that addresses a defect in this monitor.   */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheVLSNPreferResourcesMonitorNextNHood(
  KHE_VLSN_PREFER_RESOURCES_MONITOR vprm, KHE_SELECT_TARGETED rst)
{
  KHE_EVENT_RESOURCE prm_er;  int i, index, count;  KHE_TASK task;
  KHE_PREFER_RESOURCES_MONITOR prm;  KHE_VLSN_SOLVER vs;
  KHE_RESOURCE r, task_r;  KHE_RESOURCE_SET rs;  KHE_PREFER prefer;
  KHE_NHOOD_SHAPE shape;  KHE_INTERVAL in;  KHE_RESOURCE_GROUP rg;
  struct khe_prefer_free_for_task_and_preferred_rec pftp;
  struct khe_prefer_overloaded_rec po;

  /* boilerplate */
  vs = rst->solver;
  prm = (KHE_PREFER_RESOURCES_MONITOR) vprm->monitor;
  prm_er = KhePreferResourcesMonitorEventResource(prm);
  rg = KhePreferResourcesMonitorDomain(prm);

  /* choose an (r_i, d_i) */
  shape = KheNHoodShapeSetChooseNHoodShape(rst->shape_set, vs);

  /* choose a task monitored by vprm assigned an unpreferred resource */
  KheTaskSetClear(vs->tasks);
  for( i = 0;  i < KheEventResourceTaskCount(vs->soln, prm_er);  i++ )
  {
    task = KheEventResourceTask(vs->soln, prm_er, i);
    r = KheTaskAsstResource(task);
    if( r != NULL && !KheResourceGroupContains(rg, r) )
      KheTaskSetAddTask(vs->tasks, task);
  }
  count = KheTaskSetTaskCount(vs->tasks);
  HnAssert(count > 0,
    "KheVLSNPreferResourcesMonitorNextNHood internal error 1");
  index = KheRandomGeneratorNextRange(&vs->rgen, 0, count - 1);
  task = KheTaskSetTask(vs->tasks, index);
  task = KheTaskProperRoot(task);

  /* let D be the usual conversion of the set of days that task is running */
  in = KheTaskInterval(task, vs->days_frame);
  in = KheIntervalAdjustLength(in, shape.day_count, vs);
  KheIntervalSetResetSingle(vs->scratch_is, in);

  /* find the chosen task's assigned resource */
  task_r = KheTaskAsstResource(task);
  HnAssert(task_r != NULL,
    "KheVLSNPreferResourcesMonitorNextNHood internal error 2");

  /* choose resources depending on whether task needs assignment or not */
  if( KheTaskNeedsAssignment(task) )
    prefer = KhePreferFreeForTaskAndPreferred(&pftp, task, rg);
  else
    prefer = KhePreferOverloaded(&po);
  rs = KheVLSNSolverResources(vs, shape.resource_count, task_r, prefer);
  return KheNHoodMake(rs, vs->scratch_is);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_VLSN_LIMIT_RESOURCES_MONITOR"                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_LIMIT_RESOURCES_MONITOR KheVLSNLimitResourcesMonitorMake(       */
/*    KHE_LIMIT_RESOURCES_MONITOR lrm, KHE_VLSN_SOLVER vs)                   */
/*                                                                           */
/*  Make a VLSN limit resources monitor object.                              */
/*                                                                           */
/*****************************************************************************/

static KHE_VLSN_LIMIT_RESOURCES_MONITOR KheVLSNLimitResourcesMonitorMake(
  KHE_LIMIT_RESOURCES_MONITOR lrm, KHE_VLSN_SOLVER vs)
{
  KHE_VLSN_LIMIT_RESOURCES_MONITOR res;
  HaMake(res, vs->arena);
  res->tag = KHE_VLSN_LIMIT_RESOURCES_MONITOR_TAG;
  res->monitor = (KHE_MONITOR) lrm;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheVLSNLimitResourcesMonitorNextNHood(                         */
/*    KHE_VLSN_LIMIT_RESOURCES_MONITOR vlrm, KHE_SELECT_TARGETED rst)        */
/*                                                                           */
/*  Return a random neighbourhood that addresses a defect in this monitor.   */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheVLSNLimitResourcesMonitorNextNHood(
  KHE_VLSN_LIMIT_RESOURCES_MONITOR vlrm, KHE_SELECT_TARGETED rst)
{
  int i, j, index, count, egi, minimum, maximum, active_durn;  KHE_TASK task;
  bool overload, underload;  KHE_VLSN_SOLVER vs;  KHE_RESOURCE_SET rs;
  KHE_LIMIT_RESOURCES_MONITOR lrm;  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  KHE_EVENT_RESOURCE er;  KHE_NHOOD_SHAPE shape;  KHE_INTERVAL in;
  KHE_RESOURCE_GROUP m_domain;  KHE_RESOURCE task_r, r;
  struct khe_prefer_free_for_task_and_unpreferred_rec pftu;
  struct khe_prefer_free_for_task_and_preferred_rec pftp;

  /* boilerplate */
  vs = rst->solver;
  lrm = (KHE_LIMIT_RESOURCES_MONITOR) vlrm->monitor;
  lrc = KheLimitResourcesMonitorConstraint(lrm);
  egi = KheLimitResourcesMonitorEventGroupIndex(lrm);
  m_domain = KheLimitResourcesConstraintDomain(lrc);

  /* choose an (r_i, d_i) */
  shape = KheNHoodShapeSetChooseNHoodShape(rst->shape_set, vs);

  /* work out whether this is an underload or an overload */
  KheLimitResourcesMonitorActiveDuration(lrm, &minimum, &maximum, &active_durn);
  underload = (active_durn < minimum);
  overload = (active_durn > maximum);
  HnAssert(overload != underload,
    "KheVLSNLimitResourcesMonitorNextNHood internal error 1");

  /* select the tasks we need to made a choice from */
  KheTaskSetClear(vs->tasks);
  for( i = 0; i < KheLimitResourcesConstraintEventResourceCount(lrc, egi); i++ )
  {
    er = KheLimitResourcesConstraintEventResource(lrc, egi, i);
    for( j = 0;  j < KheEventResourceTaskCount(vs->soln, er);  j++ )
    {
      task = KheEventResourceTask(vs->soln, er, j);
      r = KheTaskAsstResource(task);
      if( overload )
      {
	if( r != NULL && KheResourceGroupContains(m_domain, r) )
	  KheTaskSetAddTask(vs->tasks, task);
      }
      else /* underload */
      {
	if( r == NULL || !KheResourceGroupContains(m_domain, r) )
	  KheTaskSetAddTask(vs->tasks, task);
      }
    }
  }

  /* choose one of the selected tasks */
  count = KheTaskSetTaskCount(vs->tasks);
  if( count == 0 )  /* can happen in principle when there is an underload */
    return KheNHoodMakeNull();
  index = KheRandomGeneratorNextRange(&vs->rgen, 0, count - 1);
  task = KheTaskSetTask(vs->tasks, index);
  task = KheTaskProperRoot(task);
  task_r = KheTaskAsstResource(task);

  /* let D be the usual conversion of the set of days that task is running */
  in = KheTaskInterval(task, vs->days_frame);
  in = KheIntervalAdjustLength(in, shape.day_count, vs);
  KheIntervalSetResetSingle(vs->scratch_is, in);

  /* choose resources, depending on whether we have an overload or underload */
  if( overload )
  {
    HnAssert(task_r != NULL,
      "KheVLSNLimitResourcesMonitorNextNHood internal error 2");
    rs = KheVLSNSolverResources(vs, shape.resource_count, task_r,
      KhePreferFreeForTaskAndUnPreferred(&pftu, task, m_domain));
  }
  else
  {
    if( task_r == NULL )
    {
      task_r = KheChooseResource(task, m_domain, vs);
      if( task_r == NULL )
	return KheNHoodMakeNull();
    }
    rs = KheVLSNSolverResources(vs, shape.resource_count, task_r,
      KhePreferFreeForTaskAndPreferred(&pftp, task, m_domain));
  }
  return KheNHoodMake(rs, vs->scratch_is);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_VLSN_AVOID_UNAVAILABLE_TIMES_MONITOR"                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_AVOID_UNAVAILABLE_TIMES_MONITOR                                 */
/*    KheVLSNAvoidUnavailableTimesMonitorMake(                               */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR autm, KHE_VLSN_SOLVER vs)          */
/*                                                                           */
/*  Make a VLSN avoid unavailable times monitor object.                      */
/*                                                                           */
/*****************************************************************************/

static KHE_VLSN_AVOID_UNAVAILABLE_TIMES_MONITOR
  KheVLSNAvoidUnavailableTimesMonitorMake(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR autm, KHE_VLSN_SOLVER vs)
{
  KHE_VLSN_AVOID_UNAVAILABLE_TIMES_MONITOR res;
  HaMake(res, vs->arena);
  res->tag = KHE_VLSN_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG;
  res->monitor = (KHE_MONITOR) autm;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheVLSNAvoidUnavailableTimesMonitorNextNHood(                  */
/*    KHE_VLSN_AVOID_UNAVAILABLE_TIMES_MONITOR vautm,                        */
/*    KHE_SELECT_TARGETED rst)                                               */
/*                                                                           */
/*  Return a random neighbourhood that addresses a defect in this monitor.   */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheVLSNAvoidUnavailableTimesMonitorNextNHood(
  KHE_VLSN_AVOID_UNAVAILABLE_TIMES_MONITOR vautm, KHE_SELECT_TARGETED rst)
{
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR autm;  KHE_RESOURCE autm_r;
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT autc;  KHE_TIME_GROUP tg;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  int index, count;
  KHE_VLSN_SOLVER vs;  KHE_NHOOD_SHAPE shape;  KHE_INTERVAL in;
  KHE_TASK task;  KHE_RESOURCE_SET rs;
  struct khe_prefer_free_for_task_rec pft;  struct khe_prefer_any_rec pa;

  /* boilerplate */
  vs = rst->solver;
  autm = (KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) vautm->monitor;
  autc = KheAvoidUnavailableTimesMonitorConstraint(autm);
  autm_r = KheAvoidUnavailableTimesMonitorResource(autm);
  rtm = KheResourceTimetableMonitor(vs->soln, autm_r);
  tg = KheAvoidUnavailableTimesConstraintUnavailableTimes(autc);

  /* choose an (r_i, d_i) */
  shape = KheNHoodShapeSetChooseNHoodShape(rst->shape_set, vs);

  /* select the proper root tasks assigned autm_r when autm_r is unavailable */
  KheTaskSetClear(vs->tasks);
  KheResourceTimetableMonitorAddProperRootTasksInTimeGroup(rtm, tg, false,
    vs->tasks);

  /* choose one of the selected proper root tasks */
  count = KheTaskSetTaskCount(vs->tasks);
  HnAssert(count > 0,
    "KheVLSNAvoidUnavailableTimesMonitorNextNHood internal error");
  index = KheRandomGeneratorNextRange(&vs->rgen, 0, count - 1);
  task = KheTaskSetTask(vs->tasks, index);

  /* let D be the usual conversion of the set of days that task is running */
  in = KheTaskInterval(task, vs->days_frame);
  in = KheIntervalAdjustLength(in, shape.day_count, vs);
  KheIntervalSetResetSingle(vs->scratch_is, in);

  /* find the preferred and other resources */
  rs = KheVLSNSolverResources(vs, shape.resource_count, autm_r,
    KheTaskNeedsAssignment(task) ?
      KhePreferFreeForTask(&pft, task) : KhePreferAny(&pa));
  return KheNHoodMake(rs, vs->scratch_is);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_VLSN_CLUSTER_BUSY_TIMES_MONITOR"                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_CLUSTER_BUSY_TIMES_MONITOR KheVLSNClusterBusyTimesMonitorMake(  */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm, KHE_VLSN_SOLVER vs)               */
/*                                                                           */
/*  Return a VLSN cluster busy times monitor object, or NULL if can't.       */
/*                                                                           */
/*****************************************************************************/

static KHE_VLSN_CLUSTER_BUSY_TIMES_MONITOR KheVLSNClusterBusyTimesMonitorMake(
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm, KHE_VLSN_SOLVER vs)
{
  KHE_VLSN_CLUSTER_BUSY_TIMES_MONITOR res;
  HaMake(res, vs->arena);
  res->tag = KHE_VLSN_CLUSTER_BUSY_TIMES_MONITOR_TAG;
  res->monitor = (KHE_MONITOR) cbtm;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheVLSNClusterBusyTimesMonitorNextNHood(                       */
/*    KHE_VLSN_CLUSTER_BUSY_TIMES_MONITOR vcbtm, KHE_SELECT_TARGETED rst)    */
/*                                                                           */
/*  Return a random neighbourhood that addresses a defect in this monitor.   */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheVLSNClusterBusyTimesMonitorNextNHood(
  KHE_VLSN_CLUSTER_BUSY_TIMES_MONITOR vcbtm, KHE_SELECT_TARGETED rst)
{
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;  KHE_RESOURCE cbtm_r;
  int i, index, tg_index;  KHE_VLSN_SOLVER vs;  KHE_NHOOD_SHAPE shape;
  int active_group_count, open_group_count, minimum, maximum, bc, count;
  bool allow_zero, underload, overload;  KHE_TIME_GROUP tg;  KHE_POLARITY po;

  /* boilerplate */
  vs = rst->solver;
  cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) vcbtm->monitor;
  cbtm_r = KheClusterBusyTimesMonitorResource(cbtm);

  /* choose an (r_i, d_i) */
  shape = KheNHoodShapeSetChooseNHoodShape(rst->shape_set, vs);

  /* sort out whether this defect is an underload or an overload */
  KheClusterBusyTimesMonitorActiveTimeGroupCount(cbtm, &active_group_count,
    &open_group_count, &minimum, &maximum, &allow_zero);
  overload = (active_group_count > maximum);
  underload = (active_group_count + open_group_count < minimum);
  HnAssert(underload != overload,
    "KheVLSNClusterBusyTimesMonitorNextNHood internal error 1");

  if( overload )
  {
    /* overloaded; get the indexes of the active time groups */
    HaArrayClear(vs->tg_indexes);
    for( i = 0;  i < KheClusterBusyTimesMonitorTimeGroupCount(cbtm);  i++ )
      if( KheClusterBusyTimesMonitorTimeGroupIsActive(cbtm, i, &tg, &po, &bc) )
	HaArrayAddLast(vs->tg_indexes, i);

    /* choose an active time group */
    count = HaArrayCount(vs->tg_indexes);
    if( count == 0 )
      return KheNHoodMakeNull();
    index = KheRandomGeneratorNextRange(&vs->rgen, 0, count - 1);
    tg_index = HaArray(vs->tg_indexes, index);
    tg = KheClusterBusyTimesMonitorTimeGroup(cbtm, tg_index, &po);

    /* make tg busy or free depending on po */
    /* we prefer resources that are underloaded on their version of cbtm */
    return KheMakeResourceBusyOrFree(po == KHE_NEGATIVE, cbtm_r, tg, -1,
      cbtm, shape, vs);
  }
  else if( allow_zero && active_group_count == 1 && open_group_count == 0 &&
    KheRandomGeneratorNextBool(&vs->rgen) )
  {
    /* allow_zero underload; fixed by freeing the single active time group */
    HaArrayClear(vs->tg_indexes);
    for( i = 0;  i < KheClusterBusyTimesMonitorTimeGroupCount(cbtm);  i++ )
      if( KheClusterBusyTimesMonitorTimeGroupIsActive(cbtm, i, &tg, &po, &bc) )
	HaArrayAddLast(vs->tg_indexes, i);

    /* choose the single active time group */
    count = HaArrayCount(vs->tg_indexes);
    HnAssert(count == 1,
      "KheVLSNClusterBusyTimesMonitorNextNHood internal error 2");
    tg_index = HaArrayFirst(vs->tg_indexes);
    tg = KheClusterBusyTimesMonitorTimeGroup(cbtm, tg_index, &po);

    /* make tg busy or free depending on po */
    return KheMakeResourceBusyOrFree(po == KHE_NEGATIVE, cbtm_r, tg, -1,
      NULL, shape, vs);
  }
  else
  {
    /* regular underload; get the indexes of the non-active time groups */
    HaArrayClear(vs->tg_indexes);
    for( i = 0;  i < KheClusterBusyTimesMonitorTimeGroupCount(cbtm);  i++ )
      if( !KheClusterBusyTimesMonitorTimeGroupIsActive(cbtm, i, &tg, &po, &bc) )
	HaArrayAddLast(vs->tg_indexes, i);

    /* choose a non-active time group tg */
    count = HaArrayCount(vs->tg_indexes);
    if( count == 0 )
      return KheNHoodMakeNull();
    index = KheRandomGeneratorNextRange(&vs->rgen, 0, count - 1);
    tg_index = HaArray(vs->tg_indexes, index);
    tg = KheClusterBusyTimesMonitorTimeGroup(cbtm, tg_index, &po);

    /* make tg busy or free depending on po */
    return KheMakeResourceBusyOrFree(po == KHE_POSITIVE, cbtm_r, tg, -1,
      NULL, shape, vs);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_VLSN_LIMIT_BUSY_TIMES_MONITOR"                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_LIMIT_BUSY_TIMES_MONITOR KheVLSNLimitBusyTimesMonitorMake(      */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR lbtm, KHE_VLSN_SOLVER vs)                 */
/*                                                                           */
/*  Make a VLSN cluster busy times monitor object.                           */
/*                                                                           */
/*****************************************************************************/

static KHE_VLSN_LIMIT_BUSY_TIMES_MONITOR KheVLSNLimitBusyTimesMonitorMake(
  KHE_LIMIT_BUSY_TIMES_MONITOR lbtm, KHE_VLSN_SOLVER vs)
{
  KHE_VLSN_LIMIT_BUSY_TIMES_MONITOR res;  int i, junk;  KHE_TIME_GROUP tg;

  /* first make sure that the time groups are all non-empty */
  for( i = 0;  i < KheLimitBusyTimesMonitorTimeGroupCount(lbtm);  i++ )
  {
    tg = KheLimitBusyTimesMonitorTimeGroup(lbtm, i, &junk);
    if( KheTimeGroupTimeCount(tg) == 0 )
      return NULL;
  }

  /* now make and return the object */
  HaMake(res, vs->arena);
  res->tag = KHE_VLSN_LIMIT_BUSY_TIMES_MONITOR_TAG;
  res->monitor = (KHE_MONITOR) lbtm;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheVLSNLimitBusyTimesMonitorNextNHood(                         */
/*    KHE_VLSN_LIMIT_BUSY_TIMES_MONITOR vlbtm, KHE_SELECT_TARGETED rst)      */
/*                                                                           */
/*  Return a random resource set that addresses a defect in this monitor.    */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheVLSNLimitBusyTimesMonitorNextNHood(
  KHE_VLSN_LIMIT_BUSY_TIMES_MONITOR vlbtm, KHE_SELECT_TARGETED rst)
{
  int busy_count, open_count, minimum, maximum, count, index;
  KHE_VLSN_SOLVER vs;  KHE_LIMIT_BUSY_TIMES_MONITOR lbtm; KHE_RESOURCE lbtm_r;
  KHE_TIME_GROUP tg;  bool allow_zero, underload, overload;
  KHE_NHOOD_SHAPE shape;  KHE_COST_FUNCTION cost_fn;

  /* boilerplate */
  vs = rst->solver;
  lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) vlbtm->monitor;
  lbtm_r = KheLimitBusyTimesMonitorResource(lbtm);

  /* choose an (r_i, d_i) */
  shape = KheNHoodShapeSetChooseNHoodShape(rst->shape_set, vs);

  /* choose a defective time group */
  count = KheLimitBusyTimesMonitorDefectiveTimeGroupCount(lbtm);
  HnAssert(count > 0, "KheVLSNLimitBusyTimesMonitorNextNHood internal error 1");
  index = KheRandomGeneratorNextRange(&vs->rgen, 0, count - 1);
  KheLimitBusyTimesMonitorDefectiveTimeGroup(lbtm, index, &tg,
    &busy_count, &open_count, &minimum, &maximum, &allow_zero);
  overload = (busy_count > maximum);
  underload = (busy_count + open_count < minimum);
  HnAssert(underload != overload,
    "KheVLSNLimitBusyTimesMonitorNextNHood internal error 2");

  if( overload )
  {
    /* overload; fix by making lbtm_r less busy (or free if step cost fn) */
    /* ***
    cost_fn = KheConstraintCostFunction(KheMonit orConstraint(vlbtm->monitor));
    *** */
    cost_fn = KheMonitorCostFunction(vlbtm->monitor);
    if( cost_fn == KHE_STEP_COST_FUNCTION )
      return KheMakeResourceFree(lbtm_r, tg, -1, NULL, shape, vs);
    else
      return KheMakeResourceLessBusy(lbtm_r, tg, shape, vs);
  }
  else if( allow_zero && 0 < busy_count && busy_count <= 2 &&
    KheRandomGeneratorNextBool(&vs->rgen) )
  {
    /* allow_zero underload; fix by making lbtm_r free */
    return KheMakeResourceFree(lbtm_r, tg, -1, NULL, shape, vs);
  }
  else
  {
    /* regular underload; fix by making lbtm_r busier */
    return KheMakeResourceBusy(lbtm_r, tg, -1, shape, vs);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_VLSN_LIMIT_WORKLOAD_MONITOR"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_LIMIT_WORKLOAD_MONITOR KheVLSNLimitWorkloadMonitorMake(         */
/*    KHE_LIMIT_WORKLOAD_MONITOR lwm, KHE_VLSN_SOLVER vs)                    */
/*                                                                           */
/*  Make a VLSN cluster busy times monitor object.                           */
/*                                                                           */
/*****************************************************************************/

static KHE_VLSN_LIMIT_WORKLOAD_MONITOR KheVLSNLimitWorkloadMonitorMake(
  KHE_LIMIT_WORKLOAD_MONITOR lwm, KHE_VLSN_SOLVER vs)
{
  KHE_VLSN_LIMIT_WORKLOAD_MONITOR res;  int i;  float junk;  KHE_TIME_GROUP tg;

  /* first make sure that the time groups are all non-empty */
  for( i = 0;  i < KheLimitWorkloadMonitorTimeGroupCount(lwm);  i++ )
  {
    tg = KheLimitWorkloadMonitorTimeGroup(lwm, i, &junk);
    if( KheTimeGroupTimeCount(tg) == 0 )
      return NULL;
  }

  /* now make and return the object */
  HaMake(res, vs->arena);
  res->tag = KHE_VLSN_LIMIT_BUSY_TIMES_MONITOR_TAG;
  res->monitor = (KHE_MONITOR) lwm;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheVLSNLimitWorkloadMonitorNextNHood(                          */
/*    KHE_VLSN_LIMIT_WORKLOAD_MONITOR vlwm, KHE_SELECT_TARGETED rst)         */
/*                                                                           */
/*  Return a random resource set that addresses a defect in this monitor.    */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheVLSNLimitWorkloadMonitorNextNHood(
  KHE_VLSN_LIMIT_WORKLOAD_MONITOR vlwm, KHE_SELECT_TARGETED rst)
{
  int open_count, minimum, maximum, count, index;  float workload;
  KHE_VLSN_SOLVER vs;  KHE_LIMIT_WORKLOAD_MONITOR lwm; KHE_RESOURCE lwm_r;
  KHE_TIME_GROUP tg;  bool allow_zero, underload, overload;
  KHE_NHOOD_SHAPE shape;  KHE_COST_FUNCTION cost_fn;

  /* boilerplate */
  vs = rst->solver;
  lwm = (KHE_LIMIT_WORKLOAD_MONITOR) vlwm->monitor;
  lwm_r = KheLimitWorkloadMonitorResource(lwm);

  /* choose an (r_i, d_i) */
  shape = KheNHoodShapeSetChooseNHoodShape(rst->shape_set, vs);

  /* choose a defective time group */
  count = KheLimitWorkloadMonitorDefectiveTimeGroupCount(lwm);
  HnAssert(count > 0, "KheVLSNLimitWorkloadMonitorNextNHood internal error 1");
  index = KheRandomGeneratorNextRange(&vs->rgen, 0, count - 1);
  KheLimitWorkloadMonitorDefectiveTimeGroup(lwm, index, &tg,
    &workload, &open_count, &minimum, &maximum, &allow_zero);
  overload = (workload > maximum);
  underload = (workload < minimum);
  HnAssert(underload != overload,
    "KheVLSNLimitWorkloadMonitorNextNHood internal error 2");

  if( overload )
  {
    /* overload; fix by making lwm_r less busy (or free if step cost fn) */
    /* ***
    cost_fn = KheConstraintCostFunction(KheM onitorConstraint(vlwm->monitor));
    *** */
    cost_fn = KheMonitorCostFunction(vlwm->monitor);
    if( cost_fn == KHE_STEP_COST_FUNCTION )
      return KheMakeResourceFree(lwm_r, tg, -1, NULL, shape, vs);
    else
      return KheMakeResourceLessBusy(lwm_r, tg, shape, vs);
  }
  else if( allow_zero && KheRandomGeneratorNextBool(&vs->rgen) )
  {
    /* allow_zero underload; fix by making lwm_r free */
    return KheMakeResourceFree(lwm_r, tg, -1, NULL, shape, vs);
  }
  else
  {
    /* regular underload; fix by making lwm_r busier */
    return KheMakeResourceBusy(lwm_r, tg, -1, shape, vs);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_VLSN_LIMIT_ACTIVE_INTERVALS_MONITOR"                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitActiveIntervalsMonitorIsSingleShift(                        */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim, KHE_FRAME days_frame,         */
/*    int *offset)                                                           */
/*                                                                           */
/*  If laim monitors a single shift, that is, if it monitors at least one    */
/*  time group, its time groups are all positive and all singletons, and     */
/*  their times lie in consecutive days of days_frame at the same offsets,   */
/*  then return true with *offset set to their common offset.  Otherwise     */
/*  return false is *offset set to -1.                                       */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitActiveIntervalsMonitorIsSingleShift(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim, KHE_FRAME days_frame,
  int *offset)
{
  int i, index_in_frame, prev_index_in_frame, index_in_tg, prev_index_in_tg;
  int tg_count;  KHE_TIME_GROUP tg, frame_tg;  KHE_POLARITY po;  KHE_TIME t;

  /* the monitor must monitor at least one time group */
  tg_count = KheLimitActiveIntervalsMonitorTimeGroupCount(laim);
  if( tg_count == 0 )
    return *offset = -1, false;

  /* for each time group */
  prev_index_in_frame = prev_index_in_tg = -1;  /* keep compiler happy */
  for( i = 0;  i < tg_count;  i++ )
  {
    /* make sure the time group is a singleton and positive */
    tg = KheLimitActiveIntervalsMonitorTimeGroup(laim, i, &po);
    if( KheTimeGroupTimeCount(tg) != 1 || po != KHE_POSITIVE )
      return *offset = -1, false;

    /* get the time group's time's index_in_frame and index_in_tg */
    t = KheTimeGroupTime(tg, 0);
    index_in_frame = KheFrameTimeIndex(days_frame, t);
    frame_tg = KheFrameTimeGroup(days_frame, index_in_frame);
    if( !KheTimeGroupContains(frame_tg, t, &index_in_tg) )
      HnAbort("KheLimitActiveIntervalsMonitorIsSingleShift internal error");

    /* make sure index_in_frame and index_in_tg are consistent with prev */
    if( i > 0 )
    {
      /* the monitored shifts must lie on consecutive days */
      if( index_in_frame != prev_index_in_frame + 1 )
	return *offset = -1, false;

      /* the monitored shifts must be the same each day */
      if( index_in_tg != prev_index_in_tg )
	return *offset = -1, false;
    }

    /* remember today's indexes for comparison with tomorrow's */
    prev_index_in_frame = index_in_frame;
    prev_index_in_tg = index_in_tg;
  }

  /* all good */
  return *offset = prev_index_in_tg, true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_LIMIT_ACTIVE_INTERVALS_MONITOR                                  */
/*    KheVLSNLimitActiveIntervalsMonitorMake(                                */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim, KHE_VLSN_SOLVER vs)           */
/*                                                                           */
/*  Make a VLSN limit active intervals monitor object.                       */
/*                                                                           */
/*****************************************************************************/

static KHE_VLSN_LIMIT_ACTIVE_INTERVALS_MONITOR
  KheVLSNLimitActiveIntervalsMonitorMake(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim, KHE_VLSN_SOLVER vs)
{
  KHE_VLSN_LIMIT_ACTIVE_INTERVALS_MONITOR res;
  HaMake(res, vs->arena);
  res->tag = KHE_VLSN_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG;
  res->monitor = (KHE_MONITOR) laim;
  KheLimitActiveIntervalsMonitorIsSingleShift(laim, vs->days_frame,
   &res->single_shift_offset);
  HaArrayInit(res->cases, vs->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsMonitorTimeGroupFirstDayIndex(                */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim, int index,                    */
/*    KHE_FRAME days_frame)                                                  */
/*                                                                           */
/*  Convert an index into laim's time groups into an index into days_frame.  */
/*                                                                           */
/*****************************************************************************/

static int KheLimitActiveIntervalsMonitorTimeGroupFirstDayIndex(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim, int index, KHE_FRAME days_frame)
{
  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  tg = KheLimitActiveIntervalsMonitorTimeGroup(laim, index, &po);
  return KheFrameTimeIndex(days_frame, KheTimeGroupTime(tg, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsMonitorTimeGroupLastDayIndex(                 */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim, int index,                    */
/*    KHE_FRAME days_frame)                                                  */
/*                                                                           */
/*  Convert an index into laim's time groups into an index into days_frame.  */
/*                                                                           */
/*****************************************************************************/

static int KheLimitActiveIntervalsMonitorTimeGroupLastDayIndex(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim, int index, KHE_FRAME days_frame)
{
  KHE_TIME_GROUP tg;  KHE_POLARITY po;  int count;
  tg = KheLimitActiveIntervalsMonitorTimeGroup(laim, index, &po);
  count = KheTimeGroupTimeCount(tg);
  return KheFrameTimeIndex(days_frame, KheTimeGroupTime(tg, count - 1));
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheVLSNLimitActiveIntervalsMonitorNextNHood(                   */
/*    KHE_VLSN_LIMIT_ACTIVE_INTERVALS_MONITOR vlaim, KHE_SELECT_TARGETED rst)*/
/*                                                                           */
/*  Return a random neighbourhood that addresses a defect in this monitor.   */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheVLSNLimitActiveIntervalsMonitorNextNHood(
  KHE_VLSN_LIMIT_ACTIVE_INTERVALS_MONITOR vlaim, KHE_SELECT_TARGETED rst)
{
  int count, index, history_before, first_index, last_index, history_after;
  int offset;  KHE_VLSN_SOLVER vs;  KHE_RESOURCE laim_r;
  bool too_long;  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;
  KHE_NHOOD_SHAPE shape;  KHE_TIME_GROUP tg, tg2;  KHE_POLARITY po;

  /* get laim and its offset (possibly -1 meaning not applicable) */
  vs = rst->solver;
  laim = (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) vlaim->monitor;
  laim_r = KheLimitActiveIntervalsMonitorResource(laim);
  offset = vlaim->single_shift_offset;

  /* choose an (r_i, d_i) */
  shape = KheNHoodShapeSetChooseNHoodShape(rst->shape_set, vs);

  /* choose a random defective interval [first_index, last_index] from laim */
  count = KheLimitActiveIntervalsMonitorDefectiveIntervalCount(laim);
  HnAssert(count > 0,
    "KheVLSNLimitActiveIntervalsMonitorNextNHood internal error 1");
  index = KheRandomGeneratorNextRange(&vs->rgen, 0, count - 1);
  KheLimitActiveIntervalsMonitorDefectiveInterval(laim, index,
    &history_before, &first_index, &last_index, &history_after, &too_long);
  /* NO! if( last_index < 0 )  last_index = 0; */

  /* sort out which cases apply to this interval */
  HaArrayClear(vlaim->cases);
  if( too_long )
  {
    /* try to make the first time group inactive */
    if( history_before == 0 && first_index <= last_index )
      HaArrayAddLast(vlaim->cases, KHE_LAIM_MAKE_SHORTER_AT_LEFT);

    /* try to make the last time group inactive, but don't double up */
    if( history_after == 0 && first_index <= last_index )
    {
      if( HaArrayCount(vlaim->cases) == 0 || first_index != last_index )
	HaArrayAddLast(vlaim->cases, KHE_LAIM_MAKE_SHORTER_AT_RIGHT);
    }
  }
  else /* too short */
  {
    /* try to make the time group left of first active */
    if( first_index > 0 )
      HaArrayAddLast(vlaim->cases, KHE_LAIM_MAKE_LONGER_AT_LEFT);

    /* try to make the entire interval inactive */
    if( history_before == 0 && history_after == 0 && first_index <= last_index )
      HaArrayAddLast(vlaim->cases, KHE_LAIM_MAKE_EMPTY);

    /* try to make the time group right of last active */
    if( last_index < count - 1 )
      HaArrayAddLast(vlaim->cases, KHE_LAIM_MAKE_LONGER_AT_RIGHT);
  }

  /* make a random selection from the available cases and handle it */
  count = HaArrayCount(vlaim->cases);
  if( count == 0 )
    return KheNHoodMakeNull();  /* next to impossible in practice */
  index = KheRandomGeneratorNextRange(&vs->rgen, 0, count - 1);
  switch( HaArray(vlaim->cases, index) )
  {
    case KHE_LAIM_MAKE_SHORTER_AT_LEFT:

      /* try to make the first time group inactive */
      tg = KheLimitActiveIntervalsMonitorTimeGroup(laim, first_index, &po);
      return KheMakeResourceBusyOrFree(po == KHE_NEGATIVE,
	laim_r, tg, offset, NULL, shape, vs);

    case KHE_LAIM_MAKE_SHORTER_AT_RIGHT:

      /* try to make the last time group inactive */
      tg = KheLimitActiveIntervalsMonitorTimeGroup(laim, last_index, &po);
      return KheMakeResourceBusyOrFree(po == KHE_NEGATIVE,
	laim_r, tg, offset, NULL, shape, vs);

    case KHE_LAIM_MAKE_LONGER_AT_LEFT:

      /* try to make the time group left of first active */
      tg = KheLimitActiveIntervalsMonitorTimeGroup(laim, first_index - 1, &po);
      return KheMakeResourceBusyOrFree(po == KHE_POSITIVE,
	laim_r, tg, offset, NULL, shape, vs);

    case KHE_LAIM_MAKE_EMPTY:

      /* try to make the entire interval inactive */
      if( first_index == last_index )
      {
	tg = KheLimitActiveIntervalsMonitorTimeGroup(laim, first_index, &po);
	return KheMakeResourceBusyOrFree(po == KHE_NEGATIVE,
	  laim_r, tg, offset, NULL, shape, vs);
      }
      else
      {
	tg = KheLimitActiveIntervalsMonitorTimeGroup(laim, first_index, &po);
	tg2 = KheLimitActiveIntervalsMonitorTimeGroup(laim, last_index, &po);
	if( po == KHE_POSITIVE )
	  return KheMakeResourceFree2(laim_r, tg, tg2, offset, shape, vs);
	else
	  return KheMakeResourceBusy2(laim_r, tg, tg2, offset, shape, vs);
      }

    case KHE_LAIM_MAKE_LONGER_AT_RIGHT:

      /* try to make the time group right of last active */
      tg = KheLimitActiveIntervalsMonitorTimeGroup(laim, last_index + 1, &po);
      return KheMakeResourceBusyOrFree(po == KHE_POSITIVE,
	laim_r, tg, offset, NULL, shape, vs);

    default:

      HnAbort("KheVLSNLimitActiveIntervalsMonitorNextNHood internal error 2");
      return KheNHoodMakeNull();  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_VLSN_MONITOR"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheVLSNMonitorTypedCmp(KHE_VLSN_MONITOR vm1, KHE_VLSN_MONITOR vm2)   */
/*                                                                           */
/*  Typed comparison function for sorting an array of VLSN monitors by       */
/*  decreasing cost.                                                         */
/*                                                                           */
/*****************************************************************************/

static int KheVLSNMonitorTypedCmp(KHE_VLSN_MONITOR vm1, KHE_VLSN_MONITOR vm2)
{
  return KheCostCmp(KheMonitorCost(vm2->monitor), KheMonitorCost(vm1->monitor));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheVLSNMonitorCmp(const void *t1, const void *t2)                    */
/*                                                                           */
/*  Untyped comparison function for sorting an array of VLSN monitors by     */
/*  decreasing cost.                                                         */
/*                                                                           */
/*****************************************************************************/

static int KheVLSNMonitorCmp(const void *t1, const void *t2)
{
  KHE_VLSN_MONITOR vm1 = * (KHE_VLSN_MONITOR *) t1;
  KHE_VLSN_MONITOR vm2 = * (KHE_VLSN_MONITOR *) t2;
  return KheVLSNMonitorTypedCmp(vm1, vm2);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheVLSNMonitorNextNHood(KHE_VLSN_MONITOR vm,                   */
/*    KHE_SELECT_TARGETED rst)                                               */
/*                                                                           */
/*  Return a random neighbourhood.                                           */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheVLSNMonitorNextNHood(KHE_VLSN_MONITOR vm,
  KHE_SELECT_TARGETED rst)
{
  switch( vm->tag )
  {
    case KHE_VLSN_ASSIGN_RESOURCE_MONITOR_TAG:

      return KheVLSNAssignResourceMonitorNextNHood(
	(KHE_VLSN_ASSIGN_RESOURCE_MONITOR) vm, rst);

    case KHE_VLSN_PREFER_RESOURCES_MONITOR_TAG:

      return KheVLSNPreferResourcesMonitorNextNHood(
	(KHE_VLSN_PREFER_RESOURCES_MONITOR) vm, rst);

    case KHE_VLSN_LIMIT_RESOURCES_MONITOR_TAG:

      return KheVLSNLimitResourcesMonitorNextNHood(
	(KHE_VLSN_LIMIT_RESOURCES_MONITOR) vm, rst);

    case KHE_VLSN_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      return KheVLSNAvoidUnavailableTimesMonitorNextNHood(
	(KHE_VLSN_AVOID_UNAVAILABLE_TIMES_MONITOR) vm, rst);

    case KHE_VLSN_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      return KheVLSNClusterBusyTimesMonitorNextNHood(
	(KHE_VLSN_CLUSTER_BUSY_TIMES_MONITOR) vm, rst);

    case KHE_VLSN_LIMIT_BUSY_TIMES_MONITOR_TAG:

      return KheVLSNLimitBusyTimesMonitorNextNHood(
	(KHE_VLSN_LIMIT_BUSY_TIMES_MONITOR) vm, rst);

    case KHE_VLSN_LIMIT_WORKLOAD_MONITOR_TAG:

      return KheVLSNLimitWorkloadMonitorNextNHood(
	(KHE_VLSN_LIMIT_WORKLOAD_MONITOR) vm, rst);

    case KHE_VLSN_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      return KheVLSNLimitActiveIntervalsMonitorNextNHood(
	(KHE_VLSN_LIMIT_ACTIVE_INTERVALS_MONITOR) vm, rst);

    default:

      HnAbort("KheVLSNMonitorNextNHood internal error (%d)", vm->tag);
      return KheNHoodMakeNull();  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_RESOURCE_SELECT_SINGLE"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SELECT_SINGLE KheResourceSelectSingleMake(                  */
/*    KHE_VLSN_SOLVER vs, KHE_RESOURCE_SET rs, HA_ARENA a)                   */
/*                                                                           */
/*  Make a single resource select object with these attributes.              */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_SELECT_SINGLE KheResourceSelectSingleMake(
  KHE_VLSN_SOLVER vs, KHE_RESOURCE_SET rs)
{
  KHE_RESOURCE_SELECT_SINGLE res;
  HaMake(res, vs->arena);
  res->tag = KHE_RESOURCE_SELECT_SINGLE_TAG;
  res->solver = vs;
  res->resource_set = rs;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SELECT_SINGLE KheResourceSelectSingleParse(char **str,      */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Parse an enum resource select according to the grammar                   */
/*                                                                           */
/*    int_set                                                                */
/*                                                                           */
/*  where int_set is int { "," int }.                                        */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_SELECT_SINGLE KheResourceSelectSingleParse(char **str,
  KHE_VLSN_SOLVER vs)
{
  KHE_RESOURCE_SET rs;
  rs = KheResourceSetParse(str, vs->resource_type, vs->arena);
  return KheResourceSelectSingleMake(vs, rs);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSelectSingleReset(KHE_RESOURCE_SELECT_SINGLE rss)        */
/*                                                                           */
/*  Reset rss after a success, returning true if anything changed.           */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceSelectSingleReset(KHE_RESOURCE_SELECT_SINGLE rss)
{
  /* nothing to do, and nothing has changed */
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSelectSingleIsOpen(KHE_RESOURCE_SELECT_SINGLE rss,       */
/*    int i, KHE_RESOURCE_SET *rset)                                         */
/*                                                                           */
/*  If rss is open to iteration i, return true and set *rset to the resource */
/*  set to use on that iteration.  Else set *rset to NULL and return false.  */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceSelectSingleIsOpen(KHE_RESOURCE_SELECT_SINGLE rss,
  int i, KHE_RESOURCE_SET *rset)
{
  if( i == 0 )
    return *rset = rss->resource_set, true;
  else
    return *rset = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSelectSingleDebug(KHE_RESOURCE_SELECT_SINGLE rss,        */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of rss onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheResourceSelectSingleDebug(KHE_RESOURCE_SELECT_SINGLE rss,
  int verbosity, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  KheResourceSetDebug(rss->resource_set, verbosity, -1, fp);
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_RESOURCE_SELECT_ENUM"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SELECT_ENUM KheResourceSelectEnumMake(KHE_VLSN_SOLVER vs)   */
/*                                                                           */
/*  Make an enumerated resource select object with these attributes.         */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_SELECT_ENUM KheResourceSelectEnumMake(KHE_VLSN_SOLVER vs)
{
  KHE_RESOURCE_SELECT_ENUM res;
  HaMake(res, vs->arena);
  res->tag = KHE_RESOURCE_SELECT_ENUM_TAG;
  res->solver = vs;
  HaArrayInit(res->resource_sets, vs->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SELECT_ENUM KheResourceSelectEnumParse(char **str,          */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Parse an enum resource select according to the grammar                   */
/*                                                                           */
/*    int_set { ";" int_set }                                                */
/*                                                                           */
/*  where int_set is int { "," int }.                                        */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_SELECT_ENUM KheResourceSelectEnumParse(char **str,
  KHE_VLSN_SOLVER vs)
{
  KHE_RESOURCE_SELECT_ENUM res;  KHE_RESOURCE_SET rs;
  res = KheResourceSelectEnumMake(vs);
  do
  {
    rs = KheResourceSetParse(str, vs->resource_type, vs->arena);
    HaArrayAddLast(res->resource_sets, rs);
  } while( KheStrSkip(str, ";") );
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSelectEnumReset(KHE_RESOURCE_SELECT_ENUM rse)            */
/*                                                                           */
/*  Reset rse after a success, returning true if anything changed.           */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceSelectEnumReset(KHE_RESOURCE_SELECT_ENUM rse)
{
  /* nothing to do, and nothing has changed */
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSelectEnumIsOpen(KHE_RESOURCE_SELECT_ENUM rse,           */
/*    int i, KHE_RESOURCE_SET *rset)                                         */
/*                                                                           */
/*  If rse is open to iteration i, return true and set *rset to the resource */
/*  set to use on that iteration.  Else set *rset to NULL and return false.  */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceSelectEnumIsOpen(KHE_RESOURCE_SELECT_ENUM rse,
  int i, KHE_RESOURCE_SET *rset)
{
  if( i < HaArrayCount(rse->resource_sets) )
    return *rset = HaArray(rse->resource_sets, i), true;
  else
    return *rset = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSelectEnumDebug(KHE_RESOURCE_SELECT_ENUM rse,            */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of rse onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheResourceSelectEnumDebug(KHE_RESOURCE_SELECT_ENUM rse,
  int verbosity, int indent, FILE *fp)
{
  KHE_RESOURCE_SET rset;  int i;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  HaArrayForEach(rse->resource_sets, rset, i)
  {
    if( i > 0 )
    {
      if( indent > 0 )
	fprintf(fp, "\n%*s", indent, "");
      else
	fprintf(fp, ";");
    }
    KheResourceSetDebug(rset, verbosity, -1, fp);
  }
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_RESOURCE_SELECT_ALL"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SET KheResourceSetGet(KHE_VLSN_SOLVER vs)                   */
/*                                                                           */
/*  Get a resource set from vs's free list or from its arena.                */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_SET KheResourceSetGet(KHE_VLSN_SOLVER vs)
{
  KHE_RESOURCE_SET res;
  if( HaArrayCount(vs->resource_set_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(vs->resource_set_free_list);
    KheResourceSetClear(res);
  }
  else
    res = KheResourceSetMake(vs->resource_type, vs->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SELECT_ENUM KheResourceSelectAllMake(KHE_VLSN_SOLVER vs,    */
/*    int resource_count, int count, KHE_RESOURCE_GROUP *leftover_rg)        */
/*                                                                           */
/*  Make an all(resource_count) resource select.  This is in fact just an    */
/*  enum resource select with groups of resource_count resources.            */
/*                                                                           */
/*  Only the first count resources go into the resource select; any          */
/*  leftovers go into *leftover_rg.                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_SELECT_ENUM KheResourceSelectAllMake(KHE_VLSN_SOLVER vs,
  int resource_count, int count, KHE_RESOURCE_GROUP *leftover_rg)
{
  KHE_RESOURCE_SELECT_ENUM res;  KHE_RESOURCE_SET rs;  KHE_RESOURCE r;
  ARRAY_KHE_AVAIL_RESOURCE avail_resources;  KHE_AVAIL_RESOURCE ar;
  int i, non_rigidity, frame_len, random;  KHE_CONSEC_SOLVER cs;

  /* build an array of avail resources and sort */
  cs = KheConsecSolverMake(vs->soln, vs->days_frame);
  HaArrayInit(avail_resources, vs->arena);
  frame_len = KheFrameTimeGroupCount(vs->days_frame);
  for( i = 0;  i < KheResourceTypeResourceCount(vs->resource_type);  i++ )
  {
    r = KheResourceTypeResource(vs->resource_type, i);
    non_rigidity = KheConsecSolverNonRigidity(cs, r);
    random = KheRandomGeneratorNextRange(&vs->rgen, 0, 10000);
    ar = KheAvailResourceMake(r, vs->soln, non_rigidity, frame_len,
      random, vs->arena);
    HaArrayAddLast(avail_resources, ar);
  }
  HaArraySort(avail_resources, &KheAvailResourceCmp2);
  KheConsecSolverDelete(cs);

  if( DEBUG8 )
  {
    fprintf(stderr, "  [ avail resources after sorting (there are %d):\n",
      HaArrayCount(avail_resources));
    HaArrayForEach(avail_resources, ar, i)
    {
      if( i == count )
	fprintf(stderr,"    ++++ drs above here, time sweep below here ++++\n");
      KheAvailResourceDebug(ar, 2, 4, stderr);
    }
    if( i == count )
      fprintf(stderr, "    ++++ drs above here, nothing for time sweep ++++\n");
    fprintf(stderr, "  ]\n");
  }

  /* build the resource select (assumed always wanted, i.e. count > 0) */
  res = KheResourceSelectEnumMake(vs);
  rs = NULL;  /* keep compiler happy */
  for( i = 0;  i < count;  i++ )
  {
    /* move to next resource set if time for that */
    if( i % resource_count == 0 )
    {
      rs = KheResourceSetGet(vs);
      HaArrayAddLast(res->resource_sets, rs);
    }

    /* add current resource to rs */
    ar = HaArray(avail_resources, i);
    KheResourceSetAddResource(rs, ar->resource);
  }

  /* build the leftover resource group, but only if there are leftovers */
  if( leftover_rg != NULL )
  {
    if( count < HaArrayCount(avail_resources) )
    {
      KheSolnResourceGroupBegin(vs->soln, vs->resource_type);
      for( i = count;  i < HaArrayCount(avail_resources);  i++ )
      {
	ar = HaArray(avail_resources, i);
	KheSolnResourceGroupAddResource(vs->soln, ar->resource);
      }
      *leftover_rg = KheSolnResourceGroupEnd(vs->soln);
    }
    else
      *leftover_rg = NULL;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SELECT_ENUM KheResourceSelectAllParse(char **str,           */
/*    KHE_RESOURCE_TYPE rt, KHE_SOLN soln, HA_ARENA a)                       */
/*                                                                           */
/*  Parse a resource select according to the grammar                         */
/*                                                                           */
/*    "all" "(" int ")"                                                      */
/*                                                                           */
/*  The initial "all" has already been read.                                 */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_SELECT_ENUM KheResourceSelectAllParse(char **str,
  KHE_VLSN_SOLVER vs)
{
  KHE_RESOURCE_SELECT_ENUM res;  int resource_count;
  KheStrSkip(str, "(");
  resource_count = KheIntParse(str);
  KheStrSkip(str, ")");
  res = KheResourceSelectAllMake(vs, resource_count,
    KheResourceTypeResourceCount(vs->resource_type), NULL);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_RESOURCE_SELECT_CLUSTER"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SELECT_CLUSTER KheResourceSelectClusterMake(                */
/*    KHE_RESOURCE_TYPE rt, char *constraint_str, HA_ARENA a)                */
/*                                                                           */
/*  Make a cluster resource select object with these attributes.             */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_SELECT_CLUSTER KheResourceSelectClusterMake(
  char *constraint_str, KHE_VLSN_SOLVER vs)
{
  KHE_RESOURCE_SELECT_CLUSTER res;  int i;
  HaMake(res, vs->arena);
  res->tag = KHE_RESOURCE_SELECT_CLUSTER_TAG;
  res->solver = vs;
  res->constraint_str = constraint_str;
  for( i = KHE_UNDERLOADED;  i <= KHE_OVERLOADED;  i++ )
    HaArrayInit(res->load_class[i], vs->arena);
  HaArrayInit(res->resource_sets, vs->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SELECT_CLUSTER KheResourceSelectClusterParse(char **str,    */
/*    KHE_RESOURCE_TYPE rt, KHE_SOLN soln, HA_ARENA a)                       */
/*                                                                           */
/*  Parse a cluster resource select according to the grammar                 */
/*                                                                           */
/*     "cluster" "(" str ")"                                                 */
/*                                                                           */
/*  The initial "cluster" has already been read.  Also set the result.       */
/*                                                                           */
/*****************************************************************************/
static bool KheResourceSelectClusterReset(KHE_RESOURCE_SELECT_CLUSTER rsc);

static KHE_RESOURCE_SELECT_CLUSTER KheResourceSelectClusterParse(char **str,
  KHE_VLSN_SOLVER vs)
{
  KHE_RESOURCE_SELECT_CLUSTER res;  char *constraint_str;
  KheStrSkip(str, "(");
  constraint_str = KheStringParse(str, ')');
  KheStrSkip(str, ")");
  res = KheResourceSelectClusterMake(constraint_str, vs);
  KheResourceSelectClusterReset(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int FindIndex(KHE_RESOURCE_SELECT rs, KHE_RESOURCE over_r,               */
/*    KHE_RESOURCE_LOAD r_load)                                              */
/*                                                                           */
/*  Find an index into rs->load_class[r_load] which points to a resource     */
/*  whose index is close to over_r's index.                                  */
/*                                                                           */
/*  Here rs->load_class[r_load] must contain at least one element.           */
/*                                                                           */
/*****************************************************************************/

static int FindIndex(KHE_RESOURCE_SELECT_CLUSTER rsc, KHE_RESOURCE over_r,
  KHE_RESOURCE_LOAD r_load)
{
  int over_r_index, i;  KHE_RESOURCE r;
  HnAssert(HaArrayCount(rsc->load_class[r_load]) > 0,
    "FindIndex internal error");
  over_r_index = KheResourceResourceTypeIndex(over_r);
  HaArrayForEachReverse(rsc->load_class[r_load], r, i)
    if( KheResourceResourceTypeIndex(r) < over_r_index )
      return i;
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSelectClusterReset(KHE_RESOURCE_SELECT_CLUSTER rsc)      */
/*                                                                           */
/*  Set rsc, because it's new, or we've just had a success.                  */
/*                                                                           */
/*****************************************************************************/
static void KheResourceSelectClusterDebug(KHE_RESOURCE_SELECT_CLUSTER rsc,
  int verbosity, int indent, FILE *fp);

static bool KheResourceSelectClusterReset(KHE_RESOURCE_SELECT_CLUSTER rsc)
{
  int i, j, k, k_offset, k_tries, x, active_group_count, open_group_count;
  int over_count, max_count, under_count, minimum, maximum;
  bool allow_zero;  KHE_CONSTRAINT c;  KHE_RESOURCE_LOAD r_load;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;  KHE_MONITOR m;
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;  KHE_RESOURCE_SET rset;
  KHE_RESOURCE r, over_r, under_r, max_r;  KHE_VLSN_SOLVER vs;

  vs = rsc->solver;
  if( DEBUG5 )
    fprintf(stderr, "[ KheResourceSelectClusterReset(%s, soln, \"%s\")\n",
      KheResourceTypeId(vs->resource_type), rsc->constraint_str);

  /* free any existing resources and resource sets */
  for( i = KHE_UNDERLOADED;  i <= KHE_OVERLOADED;  i++ )
    HaArrayClear(rsc->load_class[i]);
  HaArrayAppend(vs->resource_set_free_list, rsc->resource_sets, i);
  HaArrayClear(rsc->resource_sets);

  /* classify each resource of type rt */
  for( i = 0;  i < KheResourceTypeResourceCount(vs->resource_type);  i++ )
  {
    r = KheResourceTypeResource(vs->resource_type, i);
    r_load = KHE_UNDERLOADED;
    for( j = 0;  j < KheSolnResourceMonitorCount(vs->soln, r);  j++ )
    {
      m = KheSolnResourceMonitor(vs->soln, r, j);
      c = KheMonitorConstraint(m);
      if( KheMonitorTag(m) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG &&
          (HnStringContains(KheConstraintId(c), rsc->constraint_str, &x) ||
	   HnStringContains(KheConstraintName(c), rsc->constraint_str,&x)) )
      {
        cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c;
	if( KheClusterBusyTimesConstraintAllPositive(cbtc) )
	{
	  cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
	  KheClusterBusyTimesMonitorActiveTimeGroupCount(cbtm,
	    &active_group_count, &open_group_count, &minimum, &maximum,
	    &allow_zero);
	  if( active_group_count > maximum )
	  {
	    /* r is overloaded */
	    r_load = max(r_load, KHE_OVERLOADED);
	  }
	  else if( active_group_count == maximum )
	  {
	    /* r is maximal */
	    r_load = max(r_load, KHE_MAXIMAL);
	  }
	}
      }
    }
    HaArrayAddLast(rsc->load_class[r_load], r);
    if( DEBUG5 )
      fprintf(stderr, "  adding %s to class %d\n", KheResourceId(r), r_load);
  }

  /* build the resource sets and add them to res */
  under_count = HaArrayCount(rsc->load_class[KHE_UNDERLOADED]);
  max_count = HaArrayCount(rsc->load_class[KHE_MAXIMAL]);
  over_count = HaArrayCount(rsc->load_class[KHE_OVERLOADED]);
  /* *** did not work well so I've withdrawn it for now
  if( under_count >= 2 )
  {
    ** build triples (over, under, under) **
    for( i = 0;  i < over_count;  i++ )
    {
      over_r = HaArray(rs->u.cl.load_class[KHE_OVERLOADED], i);
      j = FindIndex(rs, over_r, KHE_UNDERLOADED);
      under_r1 = HaArray(rs->u.cl.load_class[KHE_UNDERLOADED], j);
      if( j < under_count - 1 )
	under_r2 = HaArray(rs->u.cl.load_class[KHE_UNDERLOADED], j + 1);
      else
	under_r2 = HaArray(rs->u.cl.load_class[KHE_UNDERLOADED], j - 1);
      if( DEBUG5 )
	fprintf(stderr, "  triple (over %s, under %s, under %s)\n",
	  KheResourceId(over_r), KheResourceId(under_r1),
	  KheResourceId(under_r2));
      rset = KheResourceSetGet(rs, a);
      KheResourceSetAddResource(rset, over_r);
      KheResourceSetAddResource(rset, under_r1);
      KheResourceSetAddResource(rset, under_r2);
      HaArrayAddLast(rs->u.cl.resource_sets, rset);
    }
  }
  else
  *** */
  if( max_count > 0 )
  {
    /* build triples (over, max, under) */
    k_tries = min(max_count, 3);
    for( i = 0;  i < over_count;  i++ )
    {
      over_r = HaArray(rsc->load_class[KHE_OVERLOADED], i);
      for( j = 0;  j < under_count;  j++ )
      {
	under_r = HaArray(rsc->load_class[KHE_UNDERLOADED], j);
	k_offset = FindIndex(rsc, over_r, KHE_MAXIMAL);
	for( k = 0;  k < k_tries;  k++ )
	{
	  max_r = HaArray(rsc->load_class[KHE_MAXIMAL],
	    (k_offset + k) % max_count);
	  if( DEBUG5 )
	    fprintf(stderr, "  triple (over %s, under %s, max %s)\n",
	      KheResourceId(over_r), KheResourceId(under_r),
	      KheResourceId(max_r));
	  rset = KheResourceSetGet(vs);
	  KheResourceSetAddResource(rset, over_r);
	  KheResourceSetAddResource(rset, under_r);
	  KheResourceSetAddResource(rset, max_r);
	  HaArrayAddLast(rsc->resource_sets, rset);
	}
      }
    }
  }
  else
  {
    /* no maximal resources, so just build pairs (over, under) */
    for( i = 0;  i < over_count; i++ )
    {
      over_r = HaArray(rsc->load_class[KHE_OVERLOADED], i);
      for( j = 0; j < under_count; j++ )
      {
	under_r = HaArray(rsc->load_class[KHE_UNDERLOADED], j);
	if( DEBUG5 )
	  fprintf(stderr, "  pair (over %s, under %s)\n",
	    KheResourceId(over_r), KheResourceId(under_r));
	rset = KheResourceSetGet(vs);
	KheResourceSetAddResource(rset, over_r);
	KheResourceSetAddResource(rset, under_r);
	HaArrayAddLast(rsc->resource_sets, rset);
      }
    }
  }

  if( DEBUG5 )
  {
    fprintf(stderr, "  KheResourceSelectClusterReset made resource sets:\n");
    KheResourceSelectClusterDebug(rsc, 2, 2, stderr);
    fprintf(stderr, "] KheResourceSelectClusterReset returning\n");
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSelectClusterIsOpen(KHE_RESOURCE_SELECT_CLUSTER rsc,     */
/*    int i, KHE_RESOURCE_SET *rset)                                         */
/*                                                                           */
/*  If rsc is open to iteration i, return true and set *rset to the resource */
/*  set to use on that iteration.  Else set *rset to NULL and return false.  */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceSelectClusterIsOpen(KHE_RESOURCE_SELECT_CLUSTER rsc,
  int i, KHE_RESOURCE_SET *rset)
{
  if( i < HaArrayCount(rsc->resource_sets) )
  {
    /* return the ith resource set */
    return *rset = HaArray(rsc->resource_sets, i), true;
  }
  else
    return *rset = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSelectClusterDebug(KHE_RESOURCE_SELECT_CLUSTER rsc,      */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of rs onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheResourceSelectClusterDebug(KHE_RESOURCE_SELECT_CLUSTER rsc,
  int verbosity, int indent, FILE *fp)
{
  KHE_RESOURCE_SET rset;  int i;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  HaArrayForEach(rsc->resource_sets, rset, i)
  {
    if( i > 0 )
    {
      if( indent > 0 )
	fprintf(fp, "\n%*s", indent, "");
      else
	fprintf(fp, ";");
    }
    KheResourceSetDebug(rset, verbosity, -1, fp);
  }
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_RESOURCE_SELECT_CHOOSE"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SELECT_CHOOSE KheResourceSelectChooseMake(                  */
/*    int resource_count, char *input_label, KHE_LEADER_TYPE lt,             */
/*    KHE_FOLLOWER_TYPE ft, KHE_VLSN_SOLVER vs)                              */
/*                                                                           */
/*  Make an avail resource select object with these attributes.              */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_SELECT_CHOOSE KheResourceSelectChooseMake(
  int resource_count, char *input_label, KHE_LEADER_TYPE lt,
  KHE_FOLLOWER_TYPE ft, KHE_VLSN_SOLVER vs)
{
  KHE_RESOURCE_SELECT_CHOOSE res;
  HaMake(res, vs->arena);
  res->tag = KHE_RESOURCE_SELECT_CHOOSE_TAG;
  res->solver = vs;
  res->resource_count = resource_count;
  res->input_label = input_label;
  res->leader_type = lt;
  res->follower_type = ft;
  HaArrayInit(res->leaders, vs->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SELECT_CHOOSE KheResourceSelectChooseParse(char **str,      */
/*    KHE_RESOURCE_TYPE rt, char *input_label, KHE_LEADER_TYPE lt,           */
/*    KHE_FOLLOWER_TYPE ft, KHE_SOLN soln, HA_ARENA a)                       */
/*                                                                           */
/*  Parse an avail resource select according to the grammar                  */
/*                                                                           */
/*    [ "choose" | "avail" | "similar" ] "(" int ")"                         */
/*                                                                           */
/*  The initial "choose" (etc.) has already been read.  Also set the result. */
/*                                                                           */
/*****************************************************************************/
static bool KheResourceSelectChooseReset(KHE_RESOURCE_SELECT_CHOOSE rsc);

static KHE_RESOURCE_SELECT_CHOOSE KheResourceSelectChooseParse(char **str,
  char *input_label, KHE_LEADER_TYPE lt, KHE_FOLLOWER_TYPE ft,
  KHE_VLSN_SOLVER vs)
{
  KHE_RESOURCE_SELECT_CHOOSE res;  int resource_count;
  KheStrSkip(str, "(");
  resource_count = KheIntParse(str);
  KheStrSkip(str, ")");
  res = KheResourceSelectChooseMake(resource_count, input_label, lt, ft, vs);
  KheResourceSelectChooseReset(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSelectIsLeader(KHE_RESOURCE_SELECT_CHOOSE rsc,           */
/*    KHE_RESOURCE r, KHE_SOLN soln)                                         */
/*                                                                           */
/*  Return true if r is a leader.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceSelectResourceIsLeader(KHE_RESOURCE_SELECT_CHOOSE rsc,
  KHE_RESOURCE r, KHE_SOLN soln)
{
  switch( rsc->leader_type )
  {
    case KHE_LEADER_ANY:

      return true;

    case KHE_LEADER_OVERLOADED:

      return KheResourceIsOverloaded(r, soln);

    default:

      HnAbort("KheResourceSelectIsLeader internal error");
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSelectChooseReset(KHE_RESOURCE_SELECT_CHOOSE rsc)        */
/*                                                                           */
/*  Reset rsc, because it's new, or we've just had a success.                */
/*                                                                           */
/*****************************************************************************/
static void KheResourceSelectChooseDebug(KHE_RESOURCE_SELECT_CHOOSE rsc,
  int verbosity, int indent, FILE *fp);

static bool KheResourceSelectChooseReset(KHE_RESOURCE_SELECT_CHOOSE rsc)
{
  int i, j, matches;  KHE_RESOURCE r, r2;  KHE_LEADER_RESOURCE lr;
  KHE_VLSN_SOLVER vs;  float similarity;

  vs = rsc->solver;
  if( DEBUG7 )
    fprintf(stderr, "[ KheResourceSelectChooseReset(%s, \"%s(%d)\", -)\n",
      KheResourceTypeId(vs->resource_type), rsc->input_label,
      rsc->resource_count);

  /* clear out any old leaders */
  HaArrayForEach(rsc->leaders, lr, i)
    KheLeaderResourceFree(lr, vs);
  HaArrayClear(rsc->leaders);

  /* get the new leaders, along with their followers */
  for( i = 0;  i < KheResourceTypeResourceCount(vs->resource_type);  i++ )
  {
    r = KheResourceTypeResource(vs->resource_type, i);
    if( KheResourceSelectResourceIsLeader(rsc, r, vs->soln) )
    {
      /* make a leader object lr for r and add r's followers to it */
      lr = KheLeaderResourceMake(r, vs);
      for( j = 0;  j < KheResourceTypeResourceCount(vs->resource_type);  j++ )
      {
	r2 = KheResourceTypeResource(vs->resource_type, j);
        if( KheResourceIsFollower(r, r2, rsc->follower_type, vs->soln,
	    &matches, &similarity) )
	  HaArrayAddLast(lr->followers,
	    KheFollowerResourceMake(r2, matches, similarity, vs));
      }

      /* sort the followers by non-increasing similarity */
      HaArraySort(lr->followers, &KheFollowerResourceCmp);

      /* add lr to leaders, but only if it has at least one follower */
      if( HaArrayCount(lr->followers) > 0 )
	HaArrayAddLast(rsc->leaders, lr);
      else
	KheLeaderResourceFree(lr, vs);
    }
  }

  if( DEBUG7 )
  {
    KheResourceSelectChooseDebug(rsc, 2, 2, stderr);
    fprintf(stderr, "] KheResourceSelectChooseReset returning\n");
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SET KheResourceSelectChooseNextResourceSet(                 */
/*    KHE_RESOURCE_SELECT_CHOOSE rsc)                                        */
/*                                                                           */
/*  Choose a resource set at random from rsc.                                */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_SET KheResourceSelectChooseNextResourceSet(
  KHE_RESOURCE_SELECT_CHOOSE rsc)
{
  KHE_RESOURCE_SET rset;  int i, pos, count;  KHE_VLSN_SOLVER vs;
  KHE_LEADER_RESOURCE lr;  KHE_FOLLOWER_RESOURCE fr;
  HnAssert(rsc->tag == KHE_RESOURCE_SELECT_CHOOSE_TAG,
    "KheResourceSelectChooseNextResourceSet internal error");
  vs = rsc->solver;
  rset = vs->scratch_rs;
  KheResourceSetClear(rset);

  /* choose one leader resource */
  pos = KheRandomGeneratorNextRange(&vs->rgen, 0, HaArrayCount(rsc->leaders)-1);
  lr = HaArray(rsc->leaders, pos);
  KheResourceSetAddResource(rset, lr->resource);

  /* choose resource_count - 1 follower resources */
  count = min(rsc->resource_count - 1, HaArrayCount(lr->followers));
  for( i = 0;  i < count;  i++ )
  {
    pos = KheRandomGeneratorNextRange(&vs->rgen, i,
      HaArrayCount(lr->followers) - 1);
    HaArraySwap(lr->followers, i, pos, fr);
    fr = HaArray(lr->followers, i);
    KheResourceSetAddResource(rset, fr->resource);
  }
  return rset;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSelectChooseIsOpen(KHE_RESOURCE_SELECT_CHOOSE rsc,       */
/*    int i, KHE_RESOURCE_SET *rset)                                         */
/*                                                                           */
/*  If rsc is open to iteration i, return true and set *rset to the resource */
/*  set to use on that iteration.  Else set *rset to NULL and return false.  */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceSelectChooseIsOpen(KHE_RESOURCE_SELECT_CHOOSE rsc,
  int i, KHE_RESOURCE_SET *rset)
{
  if( HaArrayCount(rsc->leaders) > 0 )
    return *rset = KheResourceSelectChooseNextResourceSet(rsc), true;
  else
    return *rset = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSelectChooseDebugHeader(KHE_RESOURCE_SELECT_CHOOSE rsc,  */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of the header of rsc onto fp.                                */
/*                                                                           */
/*****************************************************************************/

static void KheResourceSelectChooseDebugHeader(KHE_RESOURCE_SELECT_CHOOSE rsc,
  FILE *fp)
{
  fprintf(fp, "%s(%d)", rsc->input_label, rsc->resource_count);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSelectChooseDebug(KHE_RESOURCE_SELECT_CHOOSE rsc,        */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of rs onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheResourceSelectChooseDebug(KHE_RESOURCE_SELECT_CHOOSE rsc,
  int verbosity, int indent, FILE *fp)
{
  int i;  KHE_LEADER_RESOURCE lr;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheResourceSelectChooseDebugHeader(rsc, fp);
    fprintf(fp, ":\n");
    HaArrayForEach(rsc->leaders, lr, i)
      KheLeaderResourceDebug(lr, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheResourceSelectChooseDebugHeader(rsc, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_RESOURCE_SELECT"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SELECT KheResourceSelectParse(char **str,                   */
/*    bool allow_multiple, KHE_VLSN_SOLVER vs)                               */
/*                                                                           */
/*  Parse a resource select.  If allow_multiple is true, the syntax is       */
/*                                                                           */
/*    resource_select  ::=  all  |  choose  |  cluster  |  literal           */
/*    all              ::=  "all" "(" int ")"                                */
/*    choose           ::=  [ "choose" | "avail" | "similar" ] "(" int ")"   */
/*    cluster          ::=  "cluster" "(" str "," int ")"                    */
/*    literal          ::=  int_set { ";" int_set }                          */
/*    int_set          ::=  int { "," int }                                  */
/*                                                                           */
/*  If allow_multiple is false, the { ";" int_set } part is omitted.         */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_SELECT KheResourceSelectParse(char **str,
  bool allow_multiple, KHE_VLSN_SOLVER vs)
{
  if( KheStrSkip(str, "all") )
    return (KHE_RESOURCE_SELECT) KheResourceSelectAllParse(str, vs);
  else if( KheStrSkip(str, "choose") )
    return (KHE_RESOURCE_SELECT) KheResourceSelectChooseParse(str,
      "choose", KHE_LEADER_ANY, KHE_FOLLOWER_ANY, vs);
  else if( KheStrSkip(str, "avail") )
    return (KHE_RESOURCE_SELECT) KheResourceSelectChooseParse(str, "avail",
      KHE_LEADER_OVERLOADED, KHE_FOLLOWER_SIMILAR_AND_UNDERLOADED, vs);
  else if( KheStrSkip(str, "similar") )
    return (KHE_RESOURCE_SELECT) KheResourceSelectChooseParse(str,
      "similar", KHE_LEADER_ANY, KHE_FOLLOWER_SIMILAR, vs);
  else if( KheStrSkip(str, "cluster") )
    return (KHE_RESOURCE_SELECT) KheResourceSelectClusterParse(str, vs);
  else if( allow_multiple )
    return (KHE_RESOURCE_SELECT) KheResourceSelectEnumParse(str, vs);
  else
    return (KHE_RESOURCE_SELECT) KheResourceSelectSingleParse(str, vs);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSelectIsOpen(KHE_RESOURCE_SELECT rs,                     */
/*    int i, KHE_RESOURCE_SET *rset)                                         */
/*                                                                           */
/*  If rs is open to iteration i, return true and set *rset to the resource  */
/*  set to use on that iteration.  Else set *rset to NULL and return false.  */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceSelectIsOpen(KHE_RESOURCE_SELECT rs,
  int i, KHE_RESOURCE_SET *rset)
{
  switch( rs->tag )
  {
    case KHE_RESOURCE_SELECT_SINGLE_TAG:

      return KheResourceSelectSingleIsOpen((KHE_RESOURCE_SELECT_SINGLE) rs,
	i, rset);

    case KHE_RESOURCE_SELECT_ENUM_TAG:

      return KheResourceSelectEnumIsOpen((KHE_RESOURCE_SELECT_ENUM) rs,
	i, rset);

    case KHE_RESOURCE_SELECT_CLUSTER_TAG:

      return KheResourceSelectClusterIsOpen((KHE_RESOURCE_SELECT_CLUSTER) rs,
	i, rset);
      break;

    case KHE_RESOURCE_SELECT_CHOOSE_TAG:

      return KheResourceSelectChooseIsOpen((KHE_RESOURCE_SELECT_CHOOSE) rs,
	i, rset);

    default:

      HnAbort("KheResourceSelectIsOpen internal error");
      return *rset = NULL, false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KheResourceSelectForEachResourceSet(KHE_RESOURCE_SELECT rs,              */
/*    KHE_RESOURCE_SET rset, int i)                                          */
/*                                                                           */
/*  Iterator for visiting each resource set of rs.                           */
/*                                                                           */
/*****************************************************************************/

#define KheResourceSelectForEachResourceSet(rs, rset, i)		\
for( (i) = 0; KheResourceSelectIsOpen((rs), (i), &(rset)); (i)++ )


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSelectReset(KHE_RESOURCE_SELECT rs)                      */
/*                                                                           */
/*  Reset rs, ready for a fresh start.  Return true if anything changed.     */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceSelectReset(KHE_RESOURCE_SELECT rs)
{
  switch( rs->tag )
  {
    case KHE_RESOURCE_SELECT_SINGLE_TAG:

      return KheResourceSelectSingleReset((KHE_RESOURCE_SELECT_SINGLE) rs);

    case KHE_RESOURCE_SELECT_ENUM_TAG:

      return KheResourceSelectEnumReset((KHE_RESOURCE_SELECT_ENUM) rs);

    case KHE_RESOURCE_SELECT_CLUSTER_TAG:

      return KheResourceSelectClusterReset((KHE_RESOURCE_SELECT_CLUSTER) rs);

    case KHE_RESOURCE_SELECT_CHOOSE_TAG:

      return KheResourceSelectChooseReset((KHE_RESOURCE_SELECT_CHOOSE) rs);

    default:

      HnAbort("KheResourceSelectReset internal error");
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSelectDebug(KHE_RESOURCE_SELECT rs,                      */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of rs onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheResourceSelectDebug(KHE_RESOURCE_SELECT rs,
  int verbosity, int indent, FILE *fp)
{
  switch( rs->tag )
  {
    case KHE_RESOURCE_SELECT_SINGLE_TAG:

      KheResourceSelectSingleDebug((KHE_RESOURCE_SELECT_SINGLE) rs,
	verbosity, indent, fp);
      break;

    case KHE_RESOURCE_SELECT_ENUM_TAG:

      KheResourceSelectEnumDebug((KHE_RESOURCE_SELECT_ENUM) rs,
	verbosity, indent, fp);
      break;

    case KHE_RESOURCE_SELECT_CLUSTER_TAG:

      KheResourceSelectClusterDebug((KHE_RESOURCE_SELECT_CLUSTER) rs,
	verbosity, indent, fp);
      break;

    case KHE_RESOURCE_SELECT_CHOOSE_TAG:

      KheResourceSelectChooseDebug((KHE_RESOURCE_SELECT_CHOOSE) rs,
	verbosity, indent, fp);
      break;

    default:

      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DAY_SELECT_SINGLE"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DAY_SELECT_SINGLE KheDaySelectSingleMake(KHE_INTERVAL_SET is,        */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Make a day select representing a single set of days.                     */
/*                                                                           */
/*****************************************************************************/

static KHE_DAY_SELECT_SINGLE KheDaySelectSingleMake(KHE_INTERVAL_SET is,
  KHE_VLSN_SOLVER vs)
{
  KHE_DAY_SELECT_SINGLE res;
  HaMake(res, vs->arena);
  res->tag = KHE_DAY_SELECT_SINGLE_TAG;
  res->solver = vs;
  res->interval_set = is;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DAY_SELECT_SINGLE KheDaySelectSingleParse(char **str,                */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Parse a day select object which is a sequence of interval sets.          */
/*                                                                           */
/*****************************************************************************/

static KHE_DAY_SELECT_SINGLE KheDaySelectSingleParse(char **str,
  KHE_VLSN_SOLVER vs)
{
  KHE_INTERVAL_SET is;
  is = KheIntervalSetParse(str, vs->arena);
  return KheDaySelectSingleMake(is, vs);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDaySelectSingleSetInterval(KHE_DAY_SELECT_SINGLE dss,            */
/*    KHE_INTERVAL in)                                                       */
/*                                                                           */
/*  Set dss's interval set to contain just this one interval.                */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static void KheDaySelectSingleSetInterval(KHE_DAY_SELECT_SINGLE dss,
  KHE_INTERVAL in)
{
  KheIntervalSetClear(dss->interval_set);
  KheIntervalSetAddInterval(dss->interval_set, in);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheDaySelectSingleIsOpen(KHE_DAY_SELECT_SINGLE dss,                 */
/*    int i, KHE_INTERVAL_SET *is)                                           */
/*                                                                           */
/*  If dss is open to iteration i, return true and set *is to the interval   */
/*  set to use on that iteration.  Else set *is to NULL and return false.    */
/*                                                                           */
/*****************************************************************************/

static bool KheDaySelectSingleIsOpen(KHE_DAY_SELECT_SINGLE dss,
  int i, KHE_INTERVAL_SET *is)
{
  if( i == 0 )
    return *is = dss->interval_set, true;
  else
    return *is = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDaySelectSingleDebug(KHE_DAY_SELECT_SINGLE dsc,                  */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of dsc onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheDaySelectSingleDebug(KHE_DAY_SELECT_SINGLE dsc,
  int verbosity, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  KheIntervalSetDebug(dsc->interval_set, fp);
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DAY_SELECT_ENUM"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DAY_SELECT_ENUM KheDaySelectEnumMake(KHE_VLSN_SOLVER vs)             */
/*                                                                           */
/*  Make a day select representing a sequence of sets of days.               */
/*                                                                           */
/*****************************************************************************/

static KHE_DAY_SELECT_ENUM KheDaySelectEnumMake(KHE_VLSN_SOLVER vs)
{
  KHE_DAY_SELECT_ENUM res;
  HaMake(res, vs->arena);
  res->tag = KHE_DAY_SELECT_ENUM_TAG;
  res->solver = vs;
  HaArrayInit(res->interval_sets, vs->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DAY_SELECT_ENUM KheDaySelectEnumParse(char **str, KHE_VLSN_SOLVER vs)*/
/*                                                                           */
/*  Parse a day select object which is a sequence of interval sets.          */
/*                                                                           */
/*****************************************************************************/

static KHE_DAY_SELECT_ENUM KheDaySelectEnumParse(char **str, KHE_VLSN_SOLVER vs)
{
  KHE_DAY_SELECT_ENUM res;  KHE_INTERVAL_SET is;
  res = KheDaySelectEnumMake(vs);
  do
  {
    is = KheIntervalSetParse(str, vs->arena);
    HaArrayAddLast(res->interval_sets, is);
  } while( KheStrSkip(str, ";") );
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DAY_SELECT_ENUM KheDaySelectAllMake(int first, int len, int inc,     */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Make an "all(len, inc)" day select object, by expanding into an          */
/*  explicit interval set.  Only start from index first rather than 0.       */
/*                                                                           */
/*****************************************************************************/

static KHE_DAY_SELECT_ENUM KheDaySelectAllMake(int first, int len, int inc,
  KHE_VLSN_SOLVER vs)
{
  KHE_INTERVAL in;  KHE_INTERVAL_SET is;  KHE_DAY_SELECT_ENUM res;  int i, last;
  res = KheDaySelectEnumMake(vs);
  for( i = first;  i < KheFrameTimeGroupCount(vs->days_frame);  i += inc )
  {
    last = i + len - 1;
    if( last >= KheFrameTimeGroupCount(vs->days_frame) )
      last = KheFrameTimeGroupCount(vs->days_frame) - 1;
    is = KheIntervalSetMake(vs->arena);
    in = KheIntervalMake(i, last);
    KheIntervalSetAddInterval(is, in);
    HaArrayAddLast(res->interval_sets, is);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DAY_SELECT_ENUM KheDaySelectAllParse(char **str, KHE_VLSN_SOLVER vs) */
/*                                                                           */
/*  Parse "all", "all(len)", or "all(len, inc)"; "all" is already parsed.    */
/*                                                                           */
/*****************************************************************************/

static KHE_DAY_SELECT_ENUM KheDaySelectAllParse(char **str, KHE_VLSN_SOLVER vs)
{
  int len, inc;

  /* get len and inc from all, all(len), or all(len, inc) */
  if( KheStrSkip(str, "(") )
  {
    len = KheIntParse(str);
    if( KheStrSkip(str, ",") )
      inc = KheIntParse(str);
    else
      inc = len;
    KheStrSkip(str, ")");
  }
  else
    len = inc = KheFrameTimeGroupCount(vs->days_frame);

  /* sanity check on len */
  if( len < 0 )
    len = 0;
  else if( len > KheFrameTimeGroupCount(vs->days_frame) )
    len = KheFrameTimeGroupCount(vs->days_frame);

  /* sanity check on inc */
  if( inc <= 0 )
    inc = 1;

  /* build and return the object */
  return KheDaySelectAllMake(0, len, inc, vs);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDaySelectEnumIsOpen(KHE_DAY_SELECT_ENUM dse,                     */
/*    int i, KHE_INTERVAL_SET *is)                                           */
/*                                                                           */
/*  If dse is open to iteration i, return true and set *is to the interval   */
/*  set to use on that iteration.  Else set *is to NULL and return false.    */
/*                                                                           */
/*****************************************************************************/

static bool KheDaySelectEnumIsOpen(KHE_DAY_SELECT_ENUM dse,
  int i, KHE_INTERVAL_SET *is)
{
  if( i < HaArrayCount(dse->interval_sets) )
    return *is = HaArray(dse->interval_sets, i), true;
  else
    return *is = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDaySelectEnumDebug(KHE_DAY_SELECT_ENUM dse,                      */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of dse onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheDaySelectEnumDebug(KHE_DAY_SELECT_ENUM dse,
  int verbosity, int indent, FILE *fp)
{
  KHE_INTERVAL_SET is;  int i;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  HaArrayForEach(dse->interval_sets, is, i)
  {
    if( i > 0 )
      fprintf(fp, ";");
    KheIntervalSetDebug(is, fp);
  }
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DAY_SELECT_CHOOSE"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DAY_SELECT_CHOOSE KheDaySelectChooseMake(int choose_len,             */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Make a day select representing a potentially infinite sequence of        */
/*  sets of choose_len consecutive days, chosen randomly.                    */
/*                                                                           */
/*  Implementation note.  The interval added to res->interval_set is a       */
/*  placeholder.                                                             */
/*                                                                           */
/*****************************************************************************/

static KHE_DAY_SELECT_CHOOSE KheDaySelectChooseMake(int choose_len,
  KHE_VLSN_SOLVER vs)
{
  KHE_DAY_SELECT_CHOOSE res;
  HaMake(res, vs->arena);
  res->tag = KHE_DAY_SELECT_CHOOSE_TAG;
  res->solver = vs;
  res->choose_len = choose_len;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DAY_SELECT KheDaySelectChooseParse(char **str,                       */
/*    KHE_FRAME days_frame, HA_ARENA a)                                      */
/*                                                                           */
/*  Parse a "choose(x)" day select; the "choose" has already been read.      */
/*                                                                           */
/*****************************************************************************/

static KHE_DAY_SELECT_CHOOSE KheDaySelectChooseParse(char **str,
  KHE_VLSN_SOLVER vs)
{
  int choose_len;
  KheStrSkip(str, "(");
  choose_len = KheIntParse(str);
  if( choose_len < 1 )
    choose_len = 1;
  else if( choose_len > KheFrameTimeGroupCount(vs->days_frame) )
    choose_len = KheFrameTimeGroupCount(vs->days_frame);
  KheStrSkip(str, ")");
  return KheDaySelectChooseMake(choose_len, vs);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDaySelectChooseIsOpen(KHE_DAY_SELECT_CHOOSE dsc,                 */
/*    int i, KHE_INTERVAL_SET *is)                                           */
/*                                                                           */
/*  If dsc is open to iteration i, return true and set *is to the interval   */
/*  set to use on that iteration.  Else set *is to NULL and return false.    */
/*                                                                           */
/*****************************************************************************/

static bool KheDaySelectChooseIsOpen(KHE_DAY_SELECT_CHOOSE dsc,
  int i, KHE_INTERVAL_SET *is)
{
  KHE_VLSN_SOLVER vs;
  if( i == 0 )
  {
    /* return an interval set containing one interval chosen at random */
    vs = dsc->solver;
    KheIntervalSetResetRandom(vs->scratch_is, dsc->choose_len, dsc->solver);
    return *is = vs->scratch_is, true;
  }
  else
    return *is = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDaySelectChooseDebug(KHE_DAY_SELECT_CHOOSE dsc,                  */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of dsc onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheDaySelectChooseDebug(KHE_DAY_SELECT_CHOOSE dsc,
  int verbosity, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "choose(%d)", dsc->choose_len);
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DAY_SELECT"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DAY_SELECT KheDaySelectParse(char **str, bool allow_multiple,        */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Parse a day select object.  If allow_multiple is true, the syntax is     */
/*                                                                           */
/*    day_select     ::=  interval_sets  |  all  |  choose                   */
/*                                                                           */
/*    interval_sets  ::=  interval_set { ";" interval_set }                  */
/*    interval_set   ::=  interval { "," interval }                          */
/*    interval       ::=  int "-" int                                        */
/*                                                                           */
/*    all            ::=  "all" [ "(" int [ "," int ] ")" ]                  */
/*                                                                           */
/*    choose         ::=  "choose" "(" int ")"                               */
/*                                                                           */
/*  If allow_multiple is false, the { ";" interval_set } part is omitted.    */
/*                                                                           */
/*****************************************************************************/

static KHE_DAY_SELECT KheDaySelectParse(char **str, bool allow_multiple,
  KHE_VLSN_SOLVER vs)
{
  if( KheStrSkip(str, "all") )
    return (KHE_DAY_SELECT) KheDaySelectAllParse(str, vs);
  else if( KheStrSkip(str, "choose") )
    return (KHE_DAY_SELECT) KheDaySelectChooseParse(str, vs);
  else if( is_digit(**str) )
  {
    if( allow_multiple )
      return (KHE_DAY_SELECT) KheDaySelectEnumParse(str, vs);
    else
      return (KHE_DAY_SELECT) KheDaySelectSingleParse(str, vs);
  }
  else
  {
    HnAbort("KheDynamicResourceVLSNSolve: error in rs_drs option after |");
    return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDaySelectIsOpen(KHE_DAY_SELECT ds, int i, KHE_INTERVAL_SET *is)  */
/*                                                                           */
/*  If ds is open to iteration i, return true and set *is to the interval    */
/*  set to use on that iteration.  Else set *is to NULL and return false.    */
/*                                                                           */
/*  When choosing at random, there is just one (random) interval set.        */
/*                                                                           */
/*****************************************************************************/

static bool KheDaySelectIsOpen(KHE_DAY_SELECT ds, int i, KHE_INTERVAL_SET *is)
{
  switch( ds->tag )
  {
    case KHE_DAY_SELECT_SINGLE_TAG:

      return KheDaySelectSingleIsOpen((KHE_DAY_SELECT_SINGLE) ds, i, is);

    case KHE_DAY_SELECT_ENUM_TAG:

      return KheDaySelectEnumIsOpen((KHE_DAY_SELECT_ENUM) ds, i, is);

    case KHE_DAY_SELECT_CHOOSE_TAG:

      return KheDaySelectChooseIsOpen((KHE_DAY_SELECT_CHOOSE) ds, i, is);

    default:

      HnAbort("KheDaySelectIsOpen internal error");
      return *is = NULL, false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KheDaySelectForEachIntervalSet(KHE_DAY_SELECT ds, KHE_INTERVAL_SET is,   */
/*    int i)                                                                 */
/*                                                                           */
/*  Iterator for visiting each interval set of ds.                           */
/*                                                                           */
/*****************************************************************************/

#define KheDaySelectForEachIntervalSet(ds, iset, i)			\
for( (i) = 0;  KheDaySelectIsOpen((ds), (i), &(iset));  (i)++ )


/*****************************************************************************/
/*                                                                           */
/*  void KheDaySelectDebug(KHE_DAY_SELECT ds, int verbosity,                 */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of ds onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheDaySelectDebug(KHE_DAY_SELECT ds, int verbosity,
  int indent, FILE *fp)
{
  switch( ds->tag )
  {
    case KHE_DAY_SELECT_SINGLE_TAG:

      KheDaySelectSingleDebug((KHE_DAY_SELECT_SINGLE) ds, verbosity, indent,fp);
      break;

    case KHE_DAY_SELECT_ENUM_TAG:

      KheDaySelectEnumDebug((KHE_DAY_SELECT_ENUM) ds, verbosity, indent, fp);
      break;

    case KHE_DAY_SELECT_CHOOSE_TAG:

      KheDaySelectChooseDebug((KHE_DAY_SELECT_CHOOSE) ds, verbosity, indent,fp);
      break;

    default:

      HnAbort("KheDaySelectDebug internal error");
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SELECT_SEPARATE" - separate resource and day selects      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SELECT_SEPARATE KheSelectSeparateMake(KHE_RESOURCE_SELECT rsel,      */
/*    KHE_DAY_SELECT dsel, KHE_VLSN_SOLVER vs)                               */
/*                                                                           */
/*  Make a new separate select object with these attributes.                 */
/*                                                                           */
/*****************************************************************************/

static KHE_SELECT_SEPARATE KheSelectSeparateMake(KHE_RESOURCE_SELECT rsel,
  KHE_DAY_SELECT dsel, KHE_VLSN_SOLVER vs)
{
  KHE_SELECT_SEPARATE res;
  HaMake(res, vs->arena);
  res->tag = KHE_SELECT_SEPARATE_TAG;
  res->solver = vs;
  res->resource_select = rsel;
  res->day_select = dsel;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SELECT_SEPARATE KheSelectSeparateParse(char **str,                   */
/*    bool allow_multiple, KHE_VLSN_SOLVER vs)                               */
/*                                                                           */
/*  Parse a separate select according to the grammar                         */
/*                                                                           */
/*    resoure_select "|" day_select                                          */
/*                                                                           */
/*  The caller has already verified that a "|" is present in *str.           */
/*                                                                           */
/*****************************************************************************/

static KHE_SELECT_SEPARATE KheSelectSeparateParse(char **str,
  bool allow_multiple, KHE_VLSN_SOLVER vs)
{
  KHE_RESOURCE_SELECT resource_select;  KHE_DAY_SELECT day_select;
  resource_select = KheResourceSelectParse(str, allow_multiple, vs);
  if( !KheStrSkip(str, "|") )
    HnAbort("error where | separating resources from days expected");
  day_select = KheDaySelectParse(str, allow_multiple, vs);
  return KheSelectSeparateMake(resource_select, day_select, vs);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSelectSeparateReset(KHE_SELECT_SEPARATE ss)                      */
/*                                                                           */
/*  Reset ss, returning true if anything changed.                            */
/*                                                                           */
/*****************************************************************************/

static bool KheSelectSeparateReset(KHE_SELECT_SEPARATE ss)
{
  return KheResourceSelectReset(ss->resource_select);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSelectSeparateDebug(KHE_SELECT_SEPARATE ss, int verbosity,       */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of ss with the given verbosity and indent.                   */
/*                                                                           */
/*****************************************************************************/

static void KheSelectSeparateDebug(KHE_SELECT_SEPARATE ss, int verbosity,
  int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  KheResourceSelectDebug(ss->resource_select, verbosity, -1, fp);
  fprintf(fp, "|");
  KheDaySelectDebug(ss->day_select, verbosity, -1, fp);
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SELECT_SAME_SHIFT"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SELECT_SAME_SHIFT KheSelectSameShiftMake(int resource_count,         */
/*    int days_count, KHE_VLSN_SOLVER vs)                                    */
/*                                                                           */
/*  Make a same_shift resource select with these attributes.                 */
/*                                                                           */
/*****************************************************************************/

static KHE_SELECT_SAME_SHIFT KheSelectSameShiftMake(int resource_count,
  int days_count, KHE_VLSN_SOLVER vs)
{
  KHE_SELECT_SAME_SHIFT res;
  HaMake(res, vs->arena);
  res->tag = KHE_SELECT_SAME_SHIFT_TAG;
  res->solver = vs;
  res->resource_count = resource_count;
  res->days_count = days_count;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SELECT_SAME_SHIFT KheSelectSameShiftParse(char **str,                */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Parse a same shift resource select according to the grammar              */
/*                                                                           */
/*    "same_shift" "(" int "," int ")"                                       */
/*                                                                           */
/*  The initial "same_shift" has already been read.                          */
/*                                                                           */
/*****************************************************************************/

static KHE_SELECT_SAME_SHIFT KheSelectSameShiftParse(char **str,
  KHE_VLSN_SOLVER vs)
{
  KHE_SELECT_SAME_SHIFT res;  int resource_count, days_count;
  KheStrSkip(str, "(");
  resource_count = KheIntParse(str);
  KheStrSkip(str, ",");
  days_count = KheIntParse(str);
  KheStrSkip(str, ")");
  HnAssert(resource_count >= 1,"KheSelectSameShiftParse: in "
    "rs_drs=\"same_shift(%d, %d)\", resource_count (%d) is too small",
    resource_count, days_count, resource_count);
  HnAssert(days_count >= 1,"KheSelectSameShiftParse: in "
    "rs_drs=\"same_shift(%d, %d)\", days_count (%d) is too small",
    resource_count, days_count, days_count);
  res = KheSelectSameShiftMake(resource_count, days_count, vs);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSelectSameShiftReset(KHE_SELECT_SAME_SHIFT rsss)                 */
/*                                                                           */
/*  Set rsss, because it's new, or we've just had a success.                 */
/*                                                                           */
/*****************************************************************************/

static bool KheSelectSameShiftReset(KHE_SELECT_SAME_SHIFT rsss)
{
  /* nothing to do here, and nothing has changed */
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheSelectSameShiftNextNHood(KHE_SELECT_SAME_SHIFT rsss)        */
/*                                                                           */
/*  Choose a neighbourhood at random from rsss.                              */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheSelectSameShiftNextNHood(KHE_SELECT_SAME_SHIFT rsss)
{
  KHE_FRAME days_frame;  KHE_INTERVAL in;  KHE_VLSN_SOLVER vs;
  int i, min_times_per_day, times_per_day, offset; KHE_TIME_GROUP tg;
  KHE_RESOURCE r;  KHE_RESOURCE_TYPE rt;  KHE_RESOURCE_SET rs;

  /* choose rsss->days_count days at random and set rsss->day_select */
  vs = rsss->solver;
  KheIntervalSetResetRandom(vs->scratch_is, rsss->days_count, vs);

  /* find the minimum number of times on any of the selected days */
  days_frame = vs->days_frame;
  min_times_per_day = INT_MAX;
  in = HaArrayFirst(vs->scratch_is->intervals);
  for( i = in.first;  i <= in.last;  i++ )
  {
    tg = KheFrameTimeGroup(days_frame, i);
    times_per_day = KheTimeGroupTimeCount(tg);
    HnAssert(times_per_day > 0, "KheSelectSameShiftNextNHood: day %s "
      "has no times", KheTimeGroupId(tg));
    if( times_per_day < min_times_per_day )
      min_times_per_day = times_per_day;
  }
  HnAssert(0 < min_times_per_day && min_times_per_day < INT_MAX,
    "KheSelectSameShiftNextNHood internal error 2");

  /* choose an offset at random */
  offset = KheRandomGeneratorNextRange(&vs->rgen, 0,
    min_times_per_day - 1);

  /* gather all resources which satisfy the same_shift rule */
  rt = vs->resource_type;
  rs = vs->scratch_rs;
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    if( KheResourceSatisfiesSameShiftRule(r, in, offset, vs) )
      KheResourceSetAddResource(rs, r);
  }

  /* make random deletions until there are rsss->resource_count resources */
  while( KheResourceSetResourceCount(rs) > rsss->resource_count )
  {
    i = KheRandomGeneratorNextRange(&vs->rgen, 0,
      KheResourceSetResourceCount(rs) - 1);
    r = KheResourceSetResource(rs, i);
    KheResourceSetDeleteResource(rs, r);
  }

  return KheNHoodMake(rs, vs->scratch_is);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSelectSameShiftIsOpen(KHE_SELECT_SAME_SHIFT rsss,                */
/*    int i, KHE_NHOOD *nhood)                                               */
/*                                                                           */
/*  If rsss is open to iteration i, return true and set *nhood to the nhood  */
/*  set to use on that iteration.  Else set *nhood to NULL and return false. */
/*                                                                           */
/*****************************************************************************/

static bool KheSelectSameShiftIsOpen(KHE_SELECT_SAME_SHIFT rsss,
  int i, KHE_NHOOD *nhood)
{
  return *nhood = KheSelectSameShiftNextNHood(rsss), true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSelectSameShiftDebugHeader(KHE_SELECT_SAME_SHIFT rsss, FILE *fp) */
/*                                                                           */
/*  Debug print of the header of rsss onto fp.                               */
/*                                                                           */
/*****************************************************************************/

static void KheSelectSameShiftDebugHeader(KHE_SELECT_SAME_SHIFT rsss, FILE *fp)
{
  fprintf(fp, "same_shift(%d, %d)", rsss->resource_count, rsss->days_count);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSelectSameShiftDebug(KHE_SELECT_SAME_SHIFT rsss, int verbosity,  */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of rsss onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

static void KheSelectSameShiftDebug(KHE_SELECT_SAME_SHIFT rsss, int verbosity,
  int indent, FILE *fp)
{
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheSelectSameShiftDebugHeader(rsss, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheSelectSameShiftDebugHeader(rsss, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SELECT_DEFECTIVE_SHIFT"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SELECT_DEFECTIVE_SHIFT KheSelectDefectiveShiftMake(                  */
/*    int resource_count, int days_count, KHE_VLSN_SOLVER vs)                */
/*                                                                           */
/*  Make a defective shift resource select.                                  */
/*                                                                           */
/*****************************************************************************/
static KHE_INTERVAL_SET KheIntervalSetMake(HA_ARENA a);
static void KheIntervalSetAddInterval(KHE_INTERVAL_SET is, KHE_INTERVAL in);
static KHE_DAY_SELECT_SINGLE KheDaySelectSingleMake(KHE_INTERVAL_SET is,
  KHE_VLSN_SOLVER vs);

static KHE_SELECT_DEFECTIVE_SHIFT KheSelectDefectiveShiftMake(
  int resource_count, int days_count, KHE_VLSN_SOLVER vs)
{
  KHE_SELECT_DEFECTIVE_SHIFT res;  KHE_RESOURCE r;  int i, j, junk;
  KHE_MONITOR m;  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;
  /* KHE_INTERVAL_SET is; */

  /* make the basic object */
  if( DEBUG10 )
    fprintf(stderr, "[ KheSelectDefectiveShiftMake(%d, %d)\n",
      resource_count, days_count);
  HaMake(res, vs->arena);
  res->tag = KHE_SELECT_DEFECTIVE_SHIFT_TAG;
  res->solver = vs;
  /* is = KheIntervalSetMake(vs->arena); */
  /* KheIntervalSetAddInterval(is, KheIntervalMake(0, 0)); */
  res->resource_count = resource_count;
  res->days_count = days_count;
  HaArrayInit(res->all_monitors, vs->arena);
  HaArrayInit(res->defective_monitors, vs->arena);

  /* fill in the all_monitors array */
  for( i = 0;  i < KheResourceTypeResourceCount(vs->resource_type);  i++ )
  {
    r = KheResourceTypeResource(vs->resource_type, i);
    for( j = 0;  j < KheSolnResourceMonitorCount(vs->soln, r);  j++ )
    {
      m = KheSolnResourceMonitor(vs->soln, r, j);
      if( KheMonitorTag(m) == KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG )
      {
	laim = (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m;
	if( KheLimitActiveIntervalsMonitorIsSingleShift(laim, vs->days_frame,
	      &junk) )
	{
	  if( DEBUG10 )
	    fprintf(stderr, "  adding shift monitor %s\n", KheMonitorId(m));
	  HaArrayAddLast(res->all_monitors, laim);
	}
      }
    }
  }
  if( DEBUG10 )
    fprintf(stderr, "] KheSelectDefectiveShiftMake returning\n");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SELECT_DEFECTIVE_SHIFT KheSelectDefectiveShiftParse(char **str,      */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Parse a defective shift resource select according to the grammar         */
/*                                                                           */
/*    "defective_shift" "(" int "," int ")"                                  */
/*                                                                           */
/*  The initial "defective_shift" has already been read.                     */
/*                                                                           */
/*****************************************************************************/
static bool KheSelectDefectiveShiftReset(KHE_SELECT_DEFECTIVE_SHIFT rsds);

static KHE_SELECT_DEFECTIVE_SHIFT KheSelectDefectiveShiftParse(char **str,
  KHE_VLSN_SOLVER vs)
{
  KHE_SELECT_DEFECTIVE_SHIFT res;  int resource_count, days_count;
  KheStrSkip(str, "(");
  resource_count = KheIntParse(str);
  KheStrSkip(str, ",");
  days_count = KheIntParse(str);
  KheStrSkip(str, ")");
  HnAssert(resource_count >= 1,"KheSelectDefectiveShiftParse: in "
    "rs_drs=\"same_shift(%d, %d)\", resource_count (%d) is too small",
    resource_count, days_count, resource_count);
  HnAssert(days_count >= 1,"KheSelectDefectiveShiftParse: in "
    "rs_drs=\"same_shift(%d, %d)\", days_count (%d) is too small",
    resource_count, days_count, days_count);
  res = KheSelectDefectiveShiftMake(resource_count, days_count, vs);
  KheSelectDefectiveShiftReset(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSelectDefectiveShiftReset(KHE_SELECT_DEFECTIVE_SHIFT rsds)       */
/*                                                                           */
/*  Reset rsds, because it's new, or we've just had a success.               */
/*                                                                           */
/*  This is a simple matter of resetting the defective_monitors array to     */
/*  contain all the monitors from all_monitors whose cost is non-zero.       */
/*                                                                           */
/*****************************************************************************/

static bool KheSelectDefectiveShiftReset(KHE_SELECT_DEFECTIVE_SHIFT rsds)
{
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;  int i;
  if( DEBUG9 )
    fprintf(stderr, "[ KheSelectDefectiveShiftReset(rsds):\n");
  HaArrayClear(rsds->defective_monitors);
  HaArrayForEach(rsds->all_monitors, laim, i)
    if( KheMonitorCost((KHE_MONITOR) laim) > 0 )
    {
      HaArrayAddLast(rsds->defective_monitors, laim);
      if( DEBUG9 )
	fprintf(stderr, "  defective monitor (cost %.5f) %s\n",
	  KheCostShow(KheMonitorCost((KHE_MONITOR) laim)),
	  KheMonitorId((KHE_MONITOR) laim));
    }
  if( DEBUG9 )
    fprintf(stderr, "] KheSelectDefectiveShiftReset returning\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheSelectDefectiveShiftNextNHood(                              */
/*    KHE_SELECT_DEFECTIVE_SHIFT rsds)                                       */
/*                                                                           */
/*  Choose a resource set at random from rsds.  Return NULL if no luck.      */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheSelectDefectiveShiftNextNHood(
  KHE_SELECT_DEFECTIVE_SHIFT rsds)
{
  int i, count, index, history_before, first_index, last_index, history_after;
  int offset;  KHE_INTERVAL in;  KHE_VLSN_SOLVER vs;  KHE_RESOURCE r, laim_r;
  KHE_TIME_GROUP tg, frame_tg;  KHE_POLARITY po;  KHE_RESOURCE_TYPE rt;
  bool too_long;  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;  KHE_TIME t;
  KHE_RESOURCE_SET rs;

  /* choose laim, a random monitor from rsds->defective_monitors */
  count = HaArrayCount(rsds->defective_monitors);
  if( count == 0 )
    return KheNHoodMakeNull();
  vs = rsds->solver;
  index = KheRandomGeneratorNextRange(&vs->rgen, 0, count - 1);
  laim = HaArray(rsds->defective_monitors, index);

  /* get laim's offset */
  tg = KheLimitActiveIntervalsMonitorTimeGroup(laim, 0, &po);
  t = KheTimeGroupTime(tg, 0);
  frame_tg = KheFrameTimeTimeGroup(vs->days_frame, t);
  if( !KheTimeGroupContains(frame_tg, t, &offset) )
    HnAbort("KheSelectDefectiveShiftNextNHood internal error");

  /* choose [first_index, last_index], a random defective interval from laim */
  count = KheLimitActiveIntervalsMonitorDefectiveIntervalCount(laim);
  HnAssert(count > 0, "KheSelectDefectiveShiftNextNHood internal error");
  index = KheRandomGeneratorNextRange(&vs->rgen, 0, count - 1);
  KheLimitActiveIntervalsMonitorDefectiveInterval(laim, index, &history_before,
    &first_index, &last_index, &history_after, &too_long);
  if( last_index < 0 )  last_index = 0;

  /* choose in, a random days interval including [first_index, last_index] */
  in.first = KheLimitActiveIntervalsMonitorTimeGroupFirstDayIndex(laim,
    first_index, vs->days_frame);
  in.last = KheLimitActiveIntervalsMonitorTimeGroupLastDayIndex(laim,
    last_index, vs->days_frame);
  in = KheIntervalAdjustLength(in, rsds->days_count, vs);

  /* set rsds->day_select so that it selects in */
  KheIntervalSetResetSingle(vs->scratch_is, in);

  /* gather all resources except laim_r which satisfy the same_shift rule */
  rt = vs->resource_type;
  rs = vs->scratch_rs;
  laim_r = KheLimitActiveIntervalsMonitorResource(laim);
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    if( r != laim_r && KheResourceSatisfiesSameShiftRule(r, in, offset, vs) )
      KheResourceSetAddResource(rs, r);
  }

  /* make random deletions until there are rsds->resource_count -1 resources */
  while( KheResourceSetResourceCount(rs) > rsds->resource_count - 1 )
  {
    i = KheRandomGeneratorNextRange(&vs->rgen, 0,
      KheResourceSetResourceCount(rs) - 1);
    r = KheResourceSetResource(rs, i);
    KheResourceSetDeleteResource(rs, r);
  }

  /* add laim_r to rs and return rs */
  KheResourceSetAddResource(rs, laim_r);
  if( DEBUG9 )
  {
    fprintf(stderr, "[ KheSelectDefectiveShiftNextNHood(rsds):\n");
    fprintf(stderr, "  defective monitor (cost %.5f) %s, offset %d\n",
      KheCostShow(KheMonitorCost((KHE_MONITOR) laim)),
      KheMonitorId((KHE_MONITOR) laim), offset);
    fprintf(stderr, "  days %s, resources ",
      KheIntervalShow(in, vs->days_frame));
    KheResourceSetDebug(rs, 2, 0,  stderr);
    fprintf(stderr, "] KheSelectDefectiveShiftNextNHood returning\n");
  }
  return KheNHoodMake(rs, vs->scratch_is);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSelectDefectiveShiftIsOpen(KHE_SELECT_DEFECTIVE_SHIFT rsds,      */
/*    int i, KHE_NHOOD *nhood)                                               */
/*                                                                           */
/*  If rsds is open to iteration i, return true and set *nhood to the nhood  */
/*  to use on that iteration.  Else set *nhood to NULL and return false.     */
/*                                                                           */
/*****************************************************************************/

static bool KheSelectDefectiveShiftIsOpen(KHE_SELECT_DEFECTIVE_SHIFT rsds,
  int i, KHE_NHOOD *nhood)
{
  *nhood = KheSelectDefectiveShiftNextNHood(rsds);
  return !KheNHoodIsNull(*nhood);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSelectDefectiveShiftDebugHeader(KHE_SELECT_DEFECTIVE_SHIFT rsds, */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of the header of rsc onto fp.                                */
/*                                                                           */
/*****************************************************************************/

static void KheSelectDefectiveShiftDebugHeader(KHE_SELECT_DEFECTIVE_SHIFT rsds,
  FILE *fp)
{
  fprintf(fp, "defective_shift(%d, %d)", rsds->resource_count,rsds->days_count);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSelectDefectiveShiftDebug(KHE_SELECT_DEFECTIVE_SHIFT rsds,       */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of rsds onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

static void KheSelectDefectiveShiftDebug(KHE_SELECT_DEFECTIVE_SHIFT rsds,
  int verbosity, int indent, FILE *fp)
{
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheSelectDefectiveShiftDebugHeader(rsds, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheSelectDefectiveShiftDebugHeader(rsds, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SELECT_TARGETED"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SELECT_TARGETED KheSelectTargetedMake(KHE_NHOOD_SHAPE_SET shape_set, */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Make a targeted select.                                                  */
/*                                                                           */
/*****************************************************************************/

static KHE_SELECT_TARGETED KheSelectTargetedMake(KHE_NHOOD_SHAPE_SET shape_set,
  KHE_VLSN_SOLVER vs)
{
  KHE_SELECT_TARGETED res;  KHE_RESOURCE r;  int i, egi;
  KHE_MONITOR m;  KHE_INTERVAL_SET is;
  KHE_VLSN_MONITOR vm;
  KHE_ASSIGN_RESOURCE_MONITOR arm;
  KHE_PREFER_RESOURCES_MONITOR prm;
  KHE_LIMIT_RESOURCES_MONITOR lrm;  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR autm;
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;
  KHE_LIMIT_BUSY_TIMES_MONITOR lbtm;
  KHE_LIMIT_WORKLOAD_MONITOR lwm;
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;
  KHE_EVENT_RESOURCE er;

  /* make the basic object */
  if( DEBUG10 )
    fprintf(stderr, "[ KheSelectTargetedMake(vs)\n");
  HaMake(res, vs->arena);
  res->tag = KHE_SELECT_TARGETED_TAG;
  res->solver = vs;
  is = KheIntervalSetMake(vs->arena);
  KheIntervalSetAddInterval(is, KheIntervalMake(0, 0));
  res->shape_set = shape_set;
  HaArrayInit(res->all_monitors, vs->arena);
  HaArrayInit(res->defective_monitors, vs->arena);
  res->defective_monitor_index = -1;  /* it is incremented before use */

  /* add all relevant monitors to the all_monitors array */
  for( i = 0;  i < KheSolnMonitorCount(vs->soln);  i++ )
  {
    m = KheSolnMonitor(vs->soln, i);
    vm = NULL;
    switch( KheMonitorTag(m) )
    {
      case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

	arm = (KHE_ASSIGN_RESOURCE_MONITOR) m;
	er = KheAssignResourceMonitorEventResource(arm);
	if( KheEventResourceResourceType(er) == vs->resource_type )
	  vm = (KHE_VLSN_MONITOR) KheVLSNAssignResourceMonitorMake(arm, vs);
	break;

      case KHE_PREFER_RESOURCES_MONITOR_TAG:

	prm = (KHE_PREFER_RESOURCES_MONITOR) m;
	er = KhePreferResourcesMonitorEventResource(prm);
	if( KheEventResourceResourceType(er) == vs->resource_type )
	  vm = (KHE_VLSN_MONITOR) KheVLSNPreferResourcesMonitorMake(prm, vs);
	break;

      case KHE_LIMIT_RESOURCES_MONITOR_TAG:

	lrm = (KHE_LIMIT_RESOURCES_MONITOR) m;
	lrc = KheLimitResourcesMonitorConstraint(lrm);
	egi = KheLimitResourcesMonitorEventGroupIndex(lrm);
	if( KheLimitResourcesConstraintEventResourceCount(lrc, egi) > 0 )
	{
	  er = KheLimitResourcesConstraintEventResource(lrc, egi, 0);
	  if( KheEventResourceResourceType(er) == vs->resource_type )
	    vm = (KHE_VLSN_MONITOR) KheVLSNLimitResourcesMonitorMake(lrm, vs);
	}
	break;

      case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

	autm = (KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m;
	r = KheAvoidUnavailableTimesMonitorResource(autm);
	if( KheResourceResourceType(r) == vs->resource_type )
	  vm = (KHE_VLSN_MONITOR)
	    KheVLSNAvoidUnavailableTimesMonitorMake(autm, vs);
	break;

      case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

	cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
	r = KheClusterBusyTimesMonitorResource(cbtm);
	if( KheResourceResourceType(r) == vs->resource_type )
	    /* KheClusterBusyTimesMonitorTimeGroupCount(cbtm) < 5 ) */
	  vm = (KHE_VLSN_MONITOR) KheVLSNClusterBusyTimesMonitorMake(cbtm, vs);
	break;

      case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

	lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) m;
	r = KheLimitBusyTimesMonitorResource(lbtm);
	if( KheResourceResourceType(r) == vs->resource_type )
	  vm = (KHE_VLSN_MONITOR) KheVLSNLimitBusyTimesMonitorMake(lbtm, vs);
	break;

      case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

	lwm = (KHE_LIMIT_WORKLOAD_MONITOR) m;
	r = KheLimitWorkloadMonitorResource(lwm);
	if( KheResourceResourceType(r) == vs->resource_type )
	  vm = (KHE_VLSN_MONITOR) KheVLSNLimitWorkloadMonitorMake(lwm, vs);
	break;

      case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

	laim = (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m;
	r = KheLimitActiveIntervalsMonitorResource(laim);
	if( KheResourceResourceType(r) == vs->resource_type )
	  vm = (KHE_VLSN_MONITOR)
	    KheVLSNLimitActiveIntervalsMonitorMake(laim, vs);
	break;

      default:

	/* ignore other types */
	break;
    }
    if( vm != NULL )
      HaArrayAddLast(res->all_monitors, vm);
  }
  if( DEBUG10 )
    fprintf(stderr, "] KheSelectTargetedMake returning\n");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SELECT_TARGETED KheSelectTargetedParse(char **str,                   */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Parse a defective shift resource select according to the grammar         */
/*                                                                           */
/*    "targeted" "(" shape { "," shape } ")"                                 */
/*                                                                           */
/*  The initial "targeted" has already been read.  As part of parsing,       */
/*  sort the shapes.                                                         */
/*                                                                           */
/*****************************************************************************/
static bool KheSelectTargetedReset(KHE_SELECT_TARGETED rst);
static void KheSelectTargetedDebug(KHE_SELECT_TARGETED rst, int verbosity,
  int indent, FILE *fp);

static KHE_SELECT_TARGETED KheSelectTargetedParse(char **str,
  KHE_VLSN_SOLVER vs)
{
  KHE_SELECT_TARGETED res;  KHE_NHOOD_SHAPE_SET shape_set;
  KheStrSkip(str, "(");
  shape_set = KheNHoodShapeSetParse(str, vs);
  KheStrSkip(str, ")");
  res = KheSelectTargetedMake(shape_set, vs);
  KheSelectTargetedReset(res);
  if( DEBUG14 )
  {
    fprintf(stderr, "  KheSelectTargetedParse returning ");
    KheSelectTargetedDebug(res, 1, 0, stderr);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSelectTargetedReset(KHE_SELECT_TARGETED rst)                     */
/*                                                                           */
/*  Reset rst, because it's new, or we've just had a success.                */
/*                                                                           */
/*  This is a simple matter of resetting the defective_monitors array to     */
/*  contain all the monitors from all_monitors whose cost is non-zero,       */
/*  then sorting the array by decreasing cost.                               */
/*                                                                           */
/*****************************************************************************/

static bool KheSelectTargetedReset(KHE_SELECT_TARGETED rst)
{
  KHE_VLSN_MONITOR vm;  int i;
  if( DEBUG9 )
    fprintf(stderr, "[ KheSelectTargetedReset(rst):\n");
  HaArrayClear(rst->defective_monitors);
  HaArrayForEach(rst->all_monitors, vm, i)
    if( KheMonitorCost(vm->monitor) > 0 )
    {
      HaArrayAddLast(rst->defective_monitors, vm);
      if( DEBUG9 )
	fprintf(stderr, "  defective monitor (cost %.5f) %s\n",
	  KheCostShow(KheMonitorCost(vm->monitor)),
	  KheMonitorId(vm->monitor));
    }
  HaArraySort(rst->defective_monitors, &KheVLSNMonitorCmp);
  if( DEBUG9 )
    fprintf(stderr, "] KheSelectTargetedReset returning\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NHOOD KheSelectTargetedNextNHood(KHE_SELECT_TARGETED rst)            */
/*                                                                           */
/*  Choose a resource set at random from rst.  Return NULL if no luck.       */
/*                                                                           */
/*****************************************************************************/

static KHE_NHOOD KheSelectTargetedNextNHood(KHE_SELECT_TARGETED rst)
{
  KHE_VLSN_MONITOR vm;  KHE_NHOOD res;

  /* find the next defective VLSN monitor and make the choice from that */
  while( HaArrayCount(rst->defective_monitors) > 0 )
  {
    rst->defective_monitor_index = (rst->defective_monitor_index + 1) %
      HaArrayCount(rst->defective_monitors);
    vm = HaArray(rst->defective_monitors, rst->defective_monitor_index);
    res = KheVLSNMonitorNextNHood(vm, rst);
    if( !KheNHoodIsNull(res) )
    {
      if( DEBUG11 )
      {
	fprintf(stderr, "  targeted nhood ");
	KheNHoodDebug(res, 3, -1, stderr);
	fprintf(stderr, " for %s %s\n",
	  KheMonitorTagShow(KheMonitorTag(vm->monitor)),
	  KheMonitorId(vm->monitor));
	/* KheMonitorDebug(vm->monitor, 2, 2, stderr); */
      }
      return res;
    }
    HaArrayDeleteAndPlug(rst->defective_monitors, rst->defective_monitor_index);
    rst->defective_monitor_index--;
  }

  /* no defective VLSN monitors available to choose from */
  return KheNHoodMakeNull();
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSelectTargetedIsOpen(KHE_SELECT_TARGETED rst, int i,             */
/*    KHE_NHOOD *nhood)                                                      */
/*                                                                           */
/*  If rst is open to iteration i, return true and set *rset to the resource */
/*  set to use on that iteration.  Else set *rset to NULL and return false.  */
/*                                                                           */
/*****************************************************************************/

static bool KheSelectTargetedIsOpen(KHE_SELECT_TARGETED rst, int i,
  KHE_NHOOD *nhood)
{
  *nhood = KheSelectTargetedNextNHood(rst);
  return !KheNHoodIsNull(*nhood);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSelectTargetedDebugHeader(KHE_SELECT_TARGETED rst, FILE *fp)     */
/*                                                                           */
/*  Debug print of the header of rst onto fp.                                */
/*                                                                           */
/*****************************************************************************/

static void KheSelectTargetedDebugHeader(KHE_SELECT_TARGETED rst, FILE *fp)
{
  fprintf(fp, "targeted(");
  KheNHoodShapeSetDebug(rst->shape_set, 1, -1, fp);
  fprintf(fp, ")");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSelectTargetedDebug(KHE_SELECT_TARGETED rst, int verbosity,      */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of rst onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheSelectTargetedDebug(KHE_SELECT_TARGETED rst, int verbosity,
  int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  KheSelectTargetedDebugHeader(rst, fp);
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SELECT_COMBINED"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SELECT_COMBINED KheSelectCombinedParse(char **str,                   */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Parse a combined select.  The syntax is                                  */
/*                                                                           */
/*    combined  ::=  same_shift  |  defective_shift  |  targeted             */
/*                                                                           */
/*    same_shift       ::=  "same_shift" "(" int "," int ")"                 */
/*    defective_shift  ::=  "defective_shift" "(" int "," int ")"            */
/*    targeted         ::=  "targeted" "(" shape { "," shape } ")"           */
/*                                                                           */
/*****************************************************************************/

static KHE_SELECT_COMBINED KheSelectCombinedParse(char **str,
  KHE_VLSN_SOLVER vs)
{
  if( KheStrSkip(str, "same_shift") )
    return (KHE_SELECT_COMBINED) KheSelectSameShiftParse(str, vs);
  else if( KheStrSkip(str, "defective_shift") )
    return (KHE_SELECT_COMBINED) KheSelectDefectiveShiftParse(str,vs);
  else if( KheStrSkip(str, "targeted") )
    return (KHE_SELECT_COMBINED) KheSelectTargetedParse(str, vs);
  else
  {
    HnAbort("error at start of combined rs_drs value");
    return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSelectCombinedReset(KHE_SELECT_COMBINED sc)                      */
/*                                                                           */
/*  Reset sc.                                                                */
/*                                                                           */
/*****************************************************************************/

static bool KheSelectCombinedReset(KHE_SELECT_COMBINED sc)
{
  switch( sc->tag )
  {
    case KHE_SELECT_SAME_SHIFT_TAG:

      return KheSelectSameShiftReset((KHE_SELECT_SAME_SHIFT) sc);

    case KHE_SELECT_DEFECTIVE_SHIFT_TAG:

      return KheSelectDefectiveShiftReset((KHE_SELECT_DEFECTIVE_SHIFT) sc);

    case KHE_SELECT_TARGETED_TAG:

      return KheSelectTargetedReset((KHE_SELECT_TARGETED) sc);

    default:

      HnAbort("KheSelectCombinedReset internal error (%d)\n", sc->tag);
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSelectCombinedIsOpen(KHE_SELECT_COMBINED sc, int i,              */
/*    KHE_NHOOD *nhood)                                                      */
/*                                                                           */
/*  If sc is open to iteration i, return true and set *nhood to the nhood    */
/*  to use on that iteration.  Else set *nhood to NULL and return false.     */
/*                                                                           */
/*****************************************************************************/

static bool KheSelectCombinedIsOpen(KHE_SELECT_COMBINED sc, int i,
  KHE_NHOOD *nhood)
{
  switch( sc->tag )
  {
    case KHE_SELECT_SAME_SHIFT_TAG:

      return KheSelectSameShiftIsOpen((KHE_SELECT_SAME_SHIFT) sc, i, nhood);

    case KHE_SELECT_DEFECTIVE_SHIFT_TAG:

      return KheSelectDefectiveShiftIsOpen((KHE_SELECT_DEFECTIVE_SHIFT) sc,
	i, nhood);

    case KHE_SELECT_TARGETED_TAG:

      return KheSelectTargetedIsOpen((KHE_SELECT_TARGETED) sc, i, nhood);

    default:

      HnAbort("KheSelectCombinedIsOpen internal error (%d)\n", sc->tag);
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KheSelectCombinedForEachNHood(KHE_SELECT_COMBINED sc, KHE_NHOOD nhood,   */
/*    int i)                                                                 */
/*                                                                           */
/*  Iterator for visiting each neighbourhood of sc.                          */
/*                                                                           */
/*****************************************************************************/

#define KheSelectCombinedForEachNHood(sc, nhood, i)		\
for( (i) = 0;  KheSelectCombinedIsOpen((sc), (i), &(nhood));  (i)++ )


/*****************************************************************************/
/*                                                                           */
/*  void KheSelectCombinedDebug(KHE_SELECT_COMBINED sc, int verbosity,       */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of sc onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheSelectCombinedDebug(KHE_SELECT_COMBINED sc, int verbosity,
  int indent, FILE *fp)
{
  switch( sc->tag )
  {
    case KHE_SELECT_SAME_SHIFT_TAG:

      KheSelectSameShiftDebug((KHE_SELECT_SAME_SHIFT) sc,
	verbosity, indent, fp);
      break;

    case KHE_SELECT_DEFECTIVE_SHIFT_TAG:

      KheSelectDefectiveShiftDebug((KHE_SELECT_DEFECTIVE_SHIFT) sc,
	verbosity, indent, fp);
      break;

    case KHE_SELECT_TARGETED_TAG:

      KheSelectTargetedDebug((KHE_SELECT_TARGETED) sc,
	verbosity, indent, fp);
      break;

    default:

      HnAbort("KheSelectCombinedDebug internal error (%d)\n", sc->tag);
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SELECT"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SELECT KheSelectParse(char **str, bool allow_multiple,               */
/*    KHE_VLSN_SOLVER vs)                                                    */
/*                                                                           */
/*  Parse a select.  The syntax is                                           */
/*                                                                           */
/*    select  ::=  separate  |  combined                                     */
/*                                                                           */
/*  The allow_multiple affects separate only.                                */
/*                                                                           */
/*****************************************************************************/

static KHE_SELECT KheSelectParse(char **str, bool allow_multiple,
  KHE_VLSN_SOLVER vs)
{
  if( strstr(*str, "|") != NULL )
    return (KHE_SELECT) KheSelectSeparateParse(str, allow_multiple, vs);
  else
    return (KHE_SELECT) KheSelectCombinedParse(str, vs);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSelectReset(KHE_SELECT s)                                        */
/*                                                                           */
/*  Reset s, ready for a fresh start.  Return true if anything changed.      */
/*                                                                           */
/*****************************************************************************/

static bool KheSelectReset(KHE_SELECT s)
{
  switch( s->tag )
  {
    case KHE_SELECT_SEPARATE_TAG:

      return KheSelectSeparateReset((KHE_SELECT_SEPARATE) s);

    case KHE_SELECT_SAME_SHIFT_TAG:
    case KHE_SELECT_DEFECTIVE_SHIFT_TAG:
    case KHE_SELECT_TARGETED_TAG:

      return KheSelectCombinedReset((KHE_SELECT_COMBINED) s);

    default:

      HnAbort("KheSelectReset internal error (%d)", s->tag);
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSelectDebug(KHE_SELECT s, int verbosity, int indent, FILE *fp)   */
/*                                                                           */
/*  Debug print of rs onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheSelectDebug(KHE_SELECT s, int verbosity, int indent, FILE *fp)
{
  switch( s->tag )
  {
    case KHE_SELECT_SEPARATE_TAG:

      KheSelectSeparateDebug((KHE_SELECT_SEPARATE) s, verbosity, indent, fp);
      break;

    case KHE_SELECT_SAME_SHIFT_TAG:
    case KHE_SELECT_DEFECTIVE_SHIFT_TAG:
    case KHE_SELECT_TARGETED_TAG:

      KheSelectCombinedDebug((KHE_SELECT_COMBINED) s, verbosity, indent, fp);
      break;

    default:

      HnAbort("KheSelectDebug internal error (%d)", s->tag);
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DOM_KIND"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DRS_DOM_KIND KheDomKindParse(char **str)                             */
/*                                                                           */
/*  Read the next dom kind and return it.                                    */
/*                                                                           */
/*****************************************************************************/

static KHE_DRS_DOM_KIND KheDomKindParse(char **str)
{
  if( KheStrSkip(str, "list_none") )
    return KHE_DRS_DOM_LIST_NONE;
  else if( KheStrSkip(str, "list_separate") )
    return KHE_DRS_DOM_LIST_SEPARATE;
  else if( KheStrSkip(str, "list_tradeoff") )
    return KHE_DRS_DOM_LIST_TRADEOFF;
  else if( KheStrSkip(str, "list_tabulated") )
    return KHE_DRS_DOM_LIST_TABULATED;
  else if( KheStrSkip(str, "hash_equality") )
    return KHE_DRS_DOM_HASH_EQUALITY;
  else if( KheStrSkip(str, "hash_medium") )
    return KHE_DRS_DOM_HASH_MEDIUM;
  /* ***
  else if( KheStrSkip(str, "trie_separate") )
    return KHE_DRS_DOM_TRIE_SEPARATE;
  else if( KheStrSkip(str, "trie_tradeoff") )
    return KHE_DRS_DOM_TRIE_TRADEOFF;
  *** */
  else if( KheStrSkip(str, "indexed_tradeoff") )
    return KHE_DRS_DOM_INDEXED_TRADEOFF;
  else if( KheStrSkip(str, "indexed_tabulated") )
    return KHE_DRS_DOM_INDEXED_TABULATED;
  else
  {
    HnAbort("error where dom kind name expected in rs_drs_test option");
    return KHE_DRS_DOM_LIST_NONE;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheDomKindName(KHE_DRS_DOM_KIND dom_kind)                          */
/*                                                                           */
/*  Return the string name of dom_kind.                                      */
/*                                                                           */
/*****************************************************************************/

static char *KheDomKindName(KHE_DRS_DOM_KIND dom_kind)
{
  switch( dom_kind )
  {
    case KHE_DRS_DOM_LIST_NONE:		return "list_none";
    case KHE_DRS_DOM_LIST_SEPARATE:	return "list_separate";
    case KHE_DRS_DOM_LIST_TRADEOFF:	return "list_tradeoff";
    case KHE_DRS_DOM_LIST_TABULATED:	return "list_tabulated";
    case KHE_DRS_DOM_HASH_EQUALITY:	return "hash_equality";
    case KHE_DRS_DOM_HASH_MEDIUM:	return "hash_medium";
    /* ***
    case KHE_DRS_DOM_TRIE_SEPARATE:	return "trie_separate";
    case KHE_DRS_DOM_TRIE_TRADEOFF:	return "trie_tradeoff";
    *** */
    case KHE_DRS_DOM_INDEXED_TRADEOFF:	return "indexed_tradeoff";
    case KHE_DRS_DOM_INDEXED_TABULATED:	return "indexed_tabulated";

    default:
      HnAbort("KheDomKindName internal error");
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SOLVE_ARGUMENTS"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLVE_ARGUMENTS KheSolveArgumentsGet(KHE_OPTIONS options, HA_ARENA a)*/
/*                                                                           */
/*  Get a set of solve arguments from options.                               */
/*                                                                           */
/*****************************************************************************/
static void KheSolveArgumentsDebug(KHE_SOLVE_ARGUMENTS sa, int verbosity,
  int indent, FILE *fp);

static KHE_SOLVE_ARGUMENTS KheSolveArgumentsGet(KHE_VLSN_SOLVER vs)
{
  KHE_SOLVE_ARGUMENTS res;  char *str;

  /* make a new uninitialized solve arguments object */
  HaMake(res, vs->arena);

  /* get the rs_drs option */
  str = KheOptionsGet(vs->options, "rs_drs", "targeted(4:8, 5:6, 6:3)");
  res->select = KheSelectParse(&str, true, vs);

  /* get the Boolean options */
  res->priqueue =
    KheOptionsGetBool(vs->options, "rs_drs_priqueue", false);
  res->extra_selection =
    KheOptionsGetBool(vs->options, "rs_drs_extra_selection", true);
  res->expand_by_shifts =
    KheOptionsGetBool(vs->options, "rs_drs_expand_by_shifts", true);
  res->shift_pairs =
    KheOptionsGetBool(vs->options, "rs_drs_shift_pairs", false);
  res->correlated_exprs =
    KheOptionsGetBool(vs->options, "rs_drs_correlated_exprs", false);

  /* get the int options */
  res->daily_expand_limit =
    KheOptionsGetInt(vs->options, "rs_drs_daily_expand_limit", 10000);
  res->daily_prune_trigger =
    KheOptionsGetInt(vs->options, "rs_drs_daily_prune_trigger", 0);
  res->resource_expand_limit =
    KheOptionsGetInt(vs->options, "rs_drs_resource_expand_limit", 0);
  res->dom_approx =
    KheOptionsGetInt(vs->options, "rs_drs_dom_approx", 0);

  /* get the cache options */
  str = KheOptionsGet(vs->options, "rs_drs_dom_kind", "indexed_tabulated");
  res->main_dom_kind = KheDomKindParse(&str);
  str = KheOptionsGet(vs->options, "rs_drs_cache_dom_kind", "nocache");
  if( strcmp(str, "nocache") == 0 )
    res->cache = false, res->cache_dom_kind = KHE_DRS_DOM_LIST_NONE;
  else if( strcmp(str, "same") == 0 )
    res->cache = true, res->cache_dom_kind = res->main_dom_kind;
  else
    res->cache = true, res->cache_dom_kind = KheDomKindParse(&str);

  /* all done */
  if( DEBUG3 )
    KheSolveArgumentsDebug(res, 1, 0, stderr);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLVE_ARGUMENTS KheSolveArgumentsCopy(KHE_SOLVE_ARGUMENTS sa,        */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Make a copy of sa.                                                       */
/*                                                                           */
/*  Implementation note.  There is no need for a deep copy of sa->resources  */
/*  and sa->days, because these values are either used as is or changed to   */
/*  a completely different value.  In fact, a deep copy would be wrong,      */
/*  because it would change the meaning of choose(x).                        */
/*                                                                           */
/*****************************************************************************/

static KHE_SOLVE_ARGUMENTS KheSolveArgumentsCopy(KHE_SOLVE_ARGUMENTS sa,
  HA_ARENA a)
{
  KHE_SOLVE_ARGUMENTS res;
  HaMake(res, a);
  res->select = sa->select;
  res->priqueue = sa->priqueue;
  res->extra_selection = sa->extra_selection;
  res->expand_by_shifts = sa->expand_by_shifts;
  res->shift_pairs = sa->shift_pairs;
  res->correlated_exprs = sa->correlated_exprs;
  res->daily_expand_limit = sa->daily_expand_limit;
  res->daily_prune_trigger = sa->daily_prune_trigger;
  res->resource_expand_limit = sa->resource_expand_limit;
  res->dom_approx = sa->dom_approx;
  res->main_dom_kind = sa->main_dom_kind;
  res->cache = sa->cache;
  res->cache_dom_kind = sa->cache_dom_kind;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolveArgumentsUpdateWithDimPosOpt(KHE_SOLVE_ARGUMENTS sa,        */
/*    KHE_DIM_POS_OPT opt)                                                   */
/*                                                                           */
/*  Update sa to reflect the option in opt.                                  */
/*                                                                           */
/*****************************************************************************/

static void KheSolveArgumentsUpdateWithDimPosOpt(KHE_SOLVE_ARGUMENTS sa,
  KHE_DIM_POS_OPT opt)
{
  KHE_SELECT_SEPARATE ss;
  switch( opt->tag )
  {
    case 'R':

      HnAssert(sa->select->tag == KHE_SELECT_SEPARATE_TAG,
	"VLSN internal error:  unsuitable rs_drs option");
      ss = (KHE_SELECT_SEPARATE) sa->select;
      ss->resource_select = opt->u.resource_select_val;
      break;

    case 'D':

      HnAssert(sa->select->tag == KHE_SELECT_SEPARATE_TAG,
	"VLSN internal error:  unsuitable rs_drs option");
      ss = (KHE_SELECT_SEPARATE) sa->select;
      ss->day_select = opt->u.day_select_val;
      break;

    case 'Q':

      sa->priqueue = opt->u.bool_val;
      break;

    case 'E':

      sa->extra_selection = opt->u.bool_val;
      break;

    case 'S':

      sa->expand_by_shifts = opt->u.bool_val;
      break;

    case 'P':

      sa->shift_pairs = opt->u.bool_val;
      break;

    case 'C':

      sa->correlated_exprs = opt->u.bool_val;
      break;

    case 'L':

      sa->daily_expand_limit = opt->u.int_val;
      break;

    case 'T':

      sa->daily_prune_trigger = opt->u.int_val;
      break;

    case 'M':

      sa->resource_expand_limit = opt->u.int_val;
      break;

    case 'A':

      sa->dom_approx = opt->u.int_val;
      break;

    case 'K':

      sa->main_dom_kind = opt->u.dom_kind_val;
      break;

    case 'J':

      sa->cache = true;
      sa->cache_dom_kind = opt->u.dom_kind_val;
      break;

    default:

      HnAbort("KheSolveArgumentsUpdateWithDimPosOpt internal error");
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLVE_ARGUMENTS KheSolveArgumentsCopyAndUpdate(                      */
/*    KHE_SOLVE_ARGUMENTS sa, KHE_DIM_POS dim_pos, HA_ARENA a)               */
/*                                                                           */
/*  Copy sa and update it to reflect dim_pos.                                */
/*                                                                           */
/*****************************************************************************/

static KHE_SOLVE_ARGUMENTS KheSolveArgumentsCopyAndUpdate(
  KHE_SOLVE_ARGUMENTS sa, KHE_DIM_POS dim_pos, HA_ARENA a)
{
  KHE_DIM_POS_OPT opt;  int i;
  KHE_SOLVE_ARGUMENTS res;
  res = KheSolveArgumentsCopy(sa, a);
  HaArrayForEach(dim_pos->options, opt, i)
    KheSolveArgumentsUpdateWithDimPosOpt(res, opt);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolveArgumentsDebug(KHE_SOLVE_ARGUMENTS sa, int verbosity,       */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of sa onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheBoolDebug(char *name, bool val, int verbosity,
  int indent, FILE *fp)
{
  fprintf(fp, "%*s%s %s\n", indent, "", name, bool_show(val));
}

static void KheIntDebug(char *name, int val, int verbosity,
  int indent, FILE *fp)
{
  fprintf(fp, "%*s%s %d\n", indent, "", name, val);
}

static void KheSolveArgumentsDebug(KHE_SOLVE_ARGUMENTS sa, int verbosity,
  int indent, FILE *fp)
{
  if( indent >= 0 )
  {
    fprintf(stderr, "%*s[ KHE_SOLVE_ARGUMENTS\n", indent, "");
    KheSelectDebug(sa->select, verbosity, indent + 2, fp);
    KheBoolDebug("priqueue", sa->priqueue, verbosity, indent, fp);
    KheBoolDebug("extra_selection", sa->extra_selection, verbosity, indent, fp);
    KheBoolDebug("expand_by_shifts", sa->expand_by_shifts, verbosity,
      indent, fp);
    KheBoolDebug("shift_pairs", sa->shift_pairs, verbosity,
      indent, fp);
    KheBoolDebug("correlated_exprs", sa->correlated_exprs, verbosity,
      indent, fp);
    KheIntDebug("daily_expand_limit", sa->daily_expand_limit, verbosity,
      indent, fp);
    KheIntDebug("daily_prune_trigger", sa->daily_prune_trigger, verbosity,
      indent, fp);
    KheIntDebug("resource_expand_limit", sa->resource_expand_limit, verbosity,
      indent, fp);
    KheIntDebug("dom_approx", sa->dom_approx, verbosity,
      indent, fp);
    fprintf(fp, "%*smain_dom_kind %s\n", indent + 2, "",
      KheDomKindName(sa->main_dom_kind));
    if( sa->cache )
      fprintf(fp, "%*scache_dom_kind %s\n", indent + 2, "",
	KheDomKindName(sa->cache_dom_kind));
    fprintf(stderr, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_OPTIMAL_RESULT"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_OPTIMAL_RESULT KheOptimalResultMake(KHE_RESOURCE_SET rs,             */
/*    KHE_INTERVAL_SET is, KHE_COST cost, HA_ARENA a)                        */
/*                                                                           */
/*  Make an optimal result object with these attributes.                     */
/*                                                                           */
/*****************************************************************************/

static KHE_OPTIMAL_RESULT KheOptimalResultMake(KHE_RESOURCE_SET rs,
  KHE_INTERVAL_SET is, KHE_COST cost, HA_ARENA a)
{
  KHE_OPTIMAL_RESULT res;
  HaMake(res, a);
  res->resource_set = rs;
  res->day_set = is;
  res->cost = cost;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_OPTIMAL_RESULT_SET"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_OPTIMAL_RESULT_SET KheOptimalResultSetMake(HA_ARENA a)               */
/*                                                                           */
/*  Make a new, empty optimal result set.                                    */
/*                                                                           */
/*****************************************************************************/

static KHE_OPTIMAL_RESULT_SET KheOptimalResultSetMake(HA_ARENA a)
{
  KHE_OPTIMAL_RESULT_SET res;
  HaMake(res, a);
  HaArrayInit(res->results, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOptimalResultSetMeldResult(KHE_OPTIMAL_RESULT_SET ors,           */
/*    KHE_RESOURCE_SET rs, KHE_INTERVAL_SET is, KHE_COST cost, HA_ARENA a)   */
/*                                                                           */
/*  Meld a new result into ors.                                              */
/*                                                                           */
/*****************************************************************************/

static void KheOptimalResultSetMeldResult(KHE_OPTIMAL_RESULT_SET ors,
  KHE_RESOURCE_SET rs, KHE_INTERVAL_SET is, KHE_COST cost, HA_ARENA a)
{
  KHE_OPTIMAL_RESULT or;  int i;

  /* search for an existing result for this rs and is, and check costs */
  HaArrayForEach(ors->results, or, i)
    if( KheResourceSetEqual(rs, or->resource_set) &&
	KheIntervalSetEqual(is, or->day_set) )
    {
      HnAssert(or->cost == cost,
	"KheOptimalResultSetMeldResult: inconsistent costs");
      return;
    }

  /* no record yet, so add a new one */
  or = KheOptimalResultMake(rs, is, cost, a);
  HaArrayAddLast(ors->results, or);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DIM_POS_OPT"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DIM_POS_OPT KheDimPosOptParse(char **str, KHE_VLSN_SOLVER vs)        */
/*                                                                           */
/*  Parse and return a dim pos option.                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_DIM_POS_OPT KheDimPosOptParse(char **str, KHE_VLSN_SOLVER vs)
{
  KHE_DIM_POS_OPT res;
  HaMake(res, vs->arena);
  res->tag = KheCharParse(str);
  switch( res->tag )
  {
    /* resource select options */
    case 'R':

      res->u.resource_select_val = KheResourceSelectParse(str, false, vs);
      break;

    /* day select options */
    case 'D':

      res->u.day_select_val = KheDaySelectParse(str, false, vs);
      break;

    /* boolean options */
    case 'Q':
    case 'E':
    case 'S':
    case 'P':
    case 'C':

      res->u.bool_val = KheBoolParse(str);
      break;

    /* int options */
    case 'L':
    case 'T':
    case 'M':
    case 'A':

      res->u.int_val = KheIntParse(str);
      break;

    /* dom_kind options */
    case 'K':
    case 'J':

      res->u.dom_kind_val = KheDomKindParse(str);
      break;

    /* erroneous Z option - it's parsed separately */
    case 'Z':

      HnAbort("'Z' not sole element of last rs_drs_test dimension");
      break;

    default:

      HnAbort("unknown item name '%c' in rs_drs_test option", res->tag);
      break;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDimPosOptAddDisplay(KHE_DIM_POS_OPT opt, HA_ARRAY_NCHAR *res)    */
/*                                                                           */
/*  Add a string representation of opt to res.                               */
/*                                                                           */
/*****************************************************************************/

static void KheDimPosOptAddDisplay(KHE_DIM_POS_OPT opt, HA_ARRAY_NCHAR *res)
{
  KHE_RESOURCE_SET rs;  KHE_INTERVAL_SET is;
  switch( opt->tag )
  {
    /* resource select options */
    case 'R':

      if( !KheResourceSelectIsOpen(opt->u.resource_select_val, 0, &rs) )
	HnAbort("KheDimPosOptAddDisplay internal error 1");
      KheResourceSetAddDisplay(rs, res);
      break;

    /* day select options */
    case 'D':

      if( !KheDaySelectIsOpen(opt->u.day_select_val, 0, &is) )
	HnAbort("KheDimPosOptAddDisplay internal error 2");
      KheIntervalSetAddDisplay(is, res);
      break;

    /* boolean options */
    case 'Q':
    case 'E':
    case 'S':
    case 'P':
    case 'C':

      HnStringAdd(res, "%c%s", opt->tag, bool_show(opt->u.bool_val));
      break;

    /* int options */
    case 'L':
    case 'T':
    case 'M':
    case 'A':

      HnStringAdd(res, "%c%d", opt->tag, opt->u.int_val);
      break;

    /* dom_kind options */
    case 'K':
    case 'J':

      HnStringAdd(res, "%c%s", opt->tag, KheDomKindName(opt->u.dom_kind_val));
      break;

    default:

      HnAbort("unknown item name '%c' in rs_drs_test option", opt->tag);
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DIM_POS"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DIM_POS KheDimPosParse(char **str, KHE_VLSN_SOLVER vs)               */
/*                                                                           */
/*  Parse *str to find one dim pos.                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_DIM_POS KheDimPosParse(char **str, KHE_VLSN_SOLVER vs)
{
  KHE_DIM_POS res;  KHE_DIM_POS_OPT opt;
  HaMake(res, vs->arena);
  HaArrayInit(res->options, vs->arena);
  if( **str >= 'A' && **str <= 'Z' )
  {
    do
    {
      opt = KheDimPosOptParse(str, vs);
      HaArrayAddLast(res->options, opt);
    }
    while( KheStrSkip(str, ":") );
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDimPosAddDisplay(KHE_DIM_POS dim_pos, HA_ARRAY_NCHAR *res)       */
/*                                                                           */
/*  Add a display of dim_pos to res.                                         */
/*                                                                           */
/*****************************************************************************/

static void KheDimPosAddDisplay(KHE_DIM_POS dim_pos, HA_ARRAY_NCHAR *res)
{
  KHE_DIM_POS_OPT opt;  int i;
  HaArrayForEach(dim_pos->options, opt, i)
  {
    if( i > 0 )
      HnStringAdd(res, ":");
    KheDimPosOptAddDisplay(opt, res);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheDimPosDisplay(KHE_DIM_POS dim_pos, HA_ARENA a)                  */
/*                                                                           */
/*  Return a name for dim_pos based on its elements.                         */
/*                                                                           */
/*****************************************************************************/

static char *KheDimPosDisplay(KHE_DIM_POS dim_pos, HA_ARENA a)
{
  HA_ARRAY_NCHAR res;
  HnStringBegin(res, a);
  KheDimPosAddDisplay(dim_pos, &res);
  return HnStringEnd(res);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DIM"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DIM KheDimParse(char **str, KHE_RESOURCE_TYPE rt,                    */
/*    KHE_SOLN soln, KHE_FRAME days_frame, HA_ARENA a)                       */
/*                                                                           */
/*  Parse str and return the dim.                                            */
/*                                                                           */
/*****************************************************************************/

static KHE_DIM KheDimParse(char **str, KHE_VLSN_SOLVER vs)
{
  KHE_DIM res;  KHE_DIM_POS dim_pos;
  HaMake(res, vs->arena);
  HaArrayInit(res->dim_poses, vs->arena);
  if( KheStrSkip(str, "Z") )
  {
    HnAssert(KheParseHasEnded(str), "KheDimParse: Z not at end");
    res->days_dim = true;
  }
  else
  {
    res->days_dim = false;
    do
    {
      dim_pos = KheDimPosParse(str, vs);
      HaArrayAddLast(res->dim_poses, dim_pos);
    } while( KheStrSkip(str, ";") );
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDimAddDisplay(KHE_DIM dim, HA_ARRAY_NCHAR *res)                  */
/*                                                                           */
/*  Add a display of dim to res.                                             */
/*                                                                           */
/*****************************************************************************/

static void KheDimAddDisplay(KHE_DIM dim, HA_ARRAY_NCHAR *res)
{
  KHE_DIM_POS dim_pos;  int i;
  HaArrayForEach(dim->dim_poses, dim_pos, i)
  {
    if( i > 0 )
      HnStringAdd(res, "; ");
    KheDimPosAddDisplay(dim_pos, res);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheDimDisplay(KHE_DIM dim, HA_ARENA a)                             */
/*                                                                           */
/*  Return a name for dim based on its elements.                             */
/*                                                                           */
/*****************************************************************************/

static char *KheDimDisplay(KHE_DIM dim, HA_ARENA a)
{
  HA_ARRAY_NCHAR res;
  if( dim->days_dim )
    return "Days";
  else
  {
    HnStringBegin(res, a);
    KheDimAddDisplay(dim, &res);
    return HnStringEnd(res);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_VLSN_SOLVER"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_VLSN_SOLVER KheVLSNSolverBegin(KHE_DYNAMIC_RESOURCE_SOLVER drs,      */
/*    KHE_SOLN soln, KHE_RESOURCE_TYPE rt, KHE_OPTIONS options)              */
/*                                                                           */
/*  Make and return a new KHE_VLSN_SOLVER object based on these attributes.  */
/*  Parameter drs must be non-NULL.                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_VLSN_SOLVER KheVLSNSolverBegin(KHE_DYNAMIC_RESOURCE_SOLVER drs,
  KHE_SOLN soln, KHE_RESOURCE_TYPE rt, KHE_OPTIONS options)
{
  KHE_VLSN_SOLVER res;  HA_ARENA a;
  a = KheSolnArenaBegin(soln);
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->resource_type = rt;
  res->options = options;
  res->days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
  KheRandomGeneratorInit(&res->rgen, 300 * KheSolnDiversifier(soln) + 17);
  HaArrayInit(res->resources, a);
  res->cluster_grouper = KheClusterGrouperBuild(KheSolnInstance(soln), rt, a);
  res->solver = drs;
  HaArrayInit(res->test_dims, a);
  res->optimal_results = KheOptimalResultSetMake(a);
  HaArrayInit(res->resource_set_free_list, a);
  HaArrayInit(res->follower_resource_free_list, a);
  HaArrayInit(res->leader_resource_free_list, a);

  res->tasks = KheTaskSetMake(soln);
  HaArrayInit(res->preferred_resources, a);
  HaArrayInit(res->other_resources, a);
  HaArrayInit(res->tg_indexes, a);
  res->scratch_rs = KheResourceSetMake(rt, a);
  res->scratch_is = KheIntervalSetMake(a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheVLSNSolverEnd(KHE_VLSN_SOLVER vs)                                */
/*                                                                           */
/*  Finish up with vs.                                                       */
/*                                                                           */
/*****************************************************************************/

static void KheVLSNSolverEnd(KHE_VLSN_SOLVER vs)
{
  KheDynamicResourceSolverDelete(vs->solver);
  KheSolnArenaEnd(vs->soln, vs->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheVLSNSolverParseTest(char **str, KHE_VLSN_SOLVER vs)              */
/*                                                                           */
/*  Parse one test (a sequence of two or more dimensions) and add it to vs.  */
/*                                                                           */
/*****************************************************************************/

static void KheVLSNSolverParseTest(char **str, KHE_VLSN_SOLVER vs)
{
  KHE_DIM dim;
  do
  {
    dim = KheDimParse(str, vs);
    HaArrayAddLast(vs->test_dims, dim);
  } while( KheStrSkip(str, "|") );
  HnAssert(KheParseHasEnded(str),
    "stray characters \"%s\" at end of rs_drs_test", *str);
  HnAssert(HaArrayCount(vs->test_dims) >= 2,
    "too few | characters in rs_drs_test");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDrsAddResources(KHE_DYNAMIC_RESOURCE_SOLVER drs,                 */
/*    KHE_RESOURCE_SET rset)                                                 */
/*                                                                           */
/*  Add rset to drs.                                                         */
/*                                                                           */
/*****************************************************************************/

static void KheDrsAddResources(KHE_DYNAMIC_RESOURCE_SOLVER drs,
  KHE_RESOURCE_SET rset)
{
  int i;
  for( i = 0;  i < KheResourceSetResourceCount(rset);  i++ )
    KheDynamicResourceSolverAddResource(drs, KheResourceSetResource(rset, i));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDrsAddDays(KHE_DYNAMIC_RESOURCE_SOLVER drs, KHE_INTERVAL_SET is) */
/*                                                                           */
/*  Add is to drs.                                                           */
/*                                                                           */
/*****************************************************************************/

static void KheDrsAddDays(KHE_DYNAMIC_RESOURCE_SOLVER drs, KHE_INTERVAL_SET is)
{
  int i;  KHE_INTERVAL in;
  HaArrayForEach(is->intervals, in, i)
    KheDynamicResourceSolverAddDayRange(drs, in.first, in.last);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheVLSNSolverLimitReached(KHE_VLSN_SOLVER vs, KHE_COST lb,          */
/*    int solve_count, int solve_limit, int fail_count, int fail_limit)      */
/*                                                                           */
/*  Return true if a limit has been reached.                                 */
/*                                                                           */
/*****************************************************************************/

static bool KheVLSNSolverLimitReached(KHE_VLSN_SOLVER vs, KHE_COST lb,
  int solve_count, int solve_limit, int fail_count, int fail_limit)
{

  if( KheSolnCost(vs->soln) <= lb || solve_count >= solve_limit ||
    fail_count >= fail_limit || KheOptionsTimeLimitReached(vs->options))
  {
    /* quit now if soln is perfect or we've reached some limit */
    if( DEBUG6 )
    {
      fprintf(stderr, "  KheVLSNSolverSolve returning: ");
      if( KheSolnCost(vs->soln) == 0 )
	fprintf(stderr, "soln cost (%.1f) at lower bound", KheCostShow(lb));
      else if( solve_count >= solve_limit )
	fprintf(stderr, "solve_count (%d) >= solve_limit (%d)",
	  solve_count, solve_limit);
      else if( fail_count >= fail_limit )
	fprintf(stderr, "fail_count (%d) >= fail_limit (%d)",
	  fail_count, fail_limit);
      else if( KheOptionsTimeLimitReached(vs->options) )
	fprintf(stderr, "time limit reached");
      else
	fprintf(stderr, "??");
      fprintf(stderr, "\n");
    }
    return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheVLSNSolverSolve(KHE_VLSN_SOLVER vs, KHE_SOLVE_ARGUMENTS sa,      */
/*    int solve_limit, int fail_limit)                                       */
/*                                                                           */
/*  Perform a VLSN solve using vs, with up to solve_limit solves, and        */
/*  up to fail_limit consecutive solves that produce no improvement.         */
/*                                                                           */
/*****************************************************************************/

static void KheVLSNSolverSolve(KHE_VLSN_SOLVER vs, KHE_SOLVE_ARGUMENTS sa,
  int solve_limit, int fail_limit)
{
  int solve_count, fail_count, i, j;  KHE_RESOURCE_SET rset;
  KHE_INTERVAL_SET iset;  KHE_COST lb;  bool success;  KHE_NHOOD nhood;
  KHE_SELECT_SEPARATE ss;  KHE_SELECT s;  KHE_SELECT_COMBINED sc;
  KHE_COST prev_cost;

  lb = KheMonitorLowerBound((KHE_MONITOR) vs->soln);
  s = sa->select;
RESTART:
  solve_count = 0;
  fail_count = 0;
  switch( s->tag )
  {
    case KHE_SELECT_SEPARATE_TAG:

      ss = (KHE_SELECT_SEPARATE) s;
      KheResourceSelectForEachResourceSet(ss->resource_select, rset, i)
      {
	KheDaySelectForEachIntervalSet(ss->day_select, iset, j)
	{
	  if( KheVLSNSolverLimitReached(vs, lb, solve_count, solve_limit,
		fail_count, fail_limit) )
	    return;
	  KheDrsAddResources(vs->solver, rset);
	  KheDrsAddDays(vs->solver, iset);
	  prev_cost = KheSolnCost(vs->soln);
	  success = KheDynamicResourceSolverSolve(vs->solver, sa->priqueue,
	    sa->extra_selection, sa->expand_by_shifts, sa->shift_pairs,
	    sa->correlated_exprs, sa->daily_expand_limit,
	    sa->daily_prune_trigger, sa->resource_expand_limit, sa->dom_approx,
	    sa->main_dom_kind, sa->cache, sa->cache_dom_kind);
	  if( DEBUG13 )
	  {
	    if( success )
	      fprintf(stderr, "  success: %.5f -> %.5f\n",
		KheCostShow(prev_cost), KheCostShow(KheSolnCost(vs->soln)));
	    else
	      fprintf(stderr, "  failure: %.5f\n",
		KheCostShow(KheSolnCost(vs->soln)));
	  }
	  if( success && KheSelectReset(s) )
	  {
	    /* restart */
	    goto RESTART;
	  }
	  solve_count++;
	  fail_count = (success ? 0 : fail_count + 1);
	}
      }
      break;

    case KHE_SELECT_SAME_SHIFT_TAG:
    case KHE_SELECT_DEFECTIVE_SHIFT_TAG:
    case KHE_SELECT_TARGETED_TAG:

      sc = (KHE_SELECT_COMBINED) s;
      KheSelectCombinedForEachNHood(sc, nhood, i)
      {
	if( KheVLSNSolverLimitReached(vs, lb, solve_count, solve_limit,
	      fail_count, fail_limit) )
	  return;
	KheDrsAddResources(vs->solver, nhood.resources);
	KheDrsAddDays(vs->solver, nhood.days);
	prev_cost = KheSolnCost(vs->soln);
	success = KheDynamicResourceSolverSolve(vs->solver, sa->priqueue,
	  sa->extra_selection, sa->expand_by_shifts, sa->shift_pairs,
	  sa->correlated_exprs, sa->daily_expand_limit,
	  sa->daily_prune_trigger, sa->resource_expand_limit, sa->dom_approx,
	  sa->main_dom_kind, sa->cache, sa->cache_dom_kind);
	if( DEBUG13 )
	{
	  if( success )
	    fprintf(stderr, "  success: %.5f -> %.5f\n",
	      KheCostShow(prev_cost), KheCostShow(KheSolnCost(vs->soln)));
	  else
	    fprintf(stderr, "  failure: %.5f\n",
	      KheCostShow(KheSolnCost(vs->soln)));
	}
	if( success && KheSelectReset(s) )
	{
	  /* restart */
	  goto RESTART;
	}
	solve_count++;
	fail_count = (success ? 0 : fail_count + 1);
      }
      break;

    default:

      HnAbort("KheVLSNSolverSolve internal error (%d)", s->tag);
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheVLSNSolverDoOneTest(KHE_VLSN_SOLVER vs, KHE_SOLVE_ARGUMENTS sa)  */
/*                                                                           */
/*  Carry out one test using solve arguments sa.                             */
/*                                                                           */
/*****************************************************************************/

static void KheVLSNSolverDoOneTest(KHE_VLSN_SOLVER vs, KHE_SOLVE_ARGUMENTS sa)
{
  KHE_RESOURCE_SET rs;  KHE_INTERVAL_SET is;  KHE_COST cost;
  KHE_SELECT s;  KHE_SELECT_SEPARATE ss;

  s = sa->select;
  HnAssert(s->tag == KHE_SELECT_SEPARATE_TAG,
    "KheVLSNSolverDoOneTest internal error (s->tag %d)", s->tag);
  ss = (KHE_SELECT_SEPARATE) s;
  if( !KheResourceSelectIsOpen(ss->resource_select, 0, &rs) )
    HnAbort("KheVLSNSolverDoOneTest internal error 1");
  /* rset = KheResourceSelectFirstResourceSet(sa->resources, &vs->rgen); */
  KheDrsAddResources(vs->solver, rs);
  if( !KheDaySelectIsOpen(ss->day_select, 0, &is) )
    HnAbort("KheVLSNSolverDoOneTest internal error 2");
  /* iset = KheDaySelectFirstIntervalSet(sa->days, &vs->rgen); */
  KheDrsAddDays(vs->solver, is);
  KheDynamicResourceSolverTest(vs->solver, sa->priqueue,
    sa->extra_selection, sa->expand_by_shifts, sa->shift_pairs,
    sa->correlated_exprs, sa->daily_expand_limit, sa->daily_prune_trigger,
    sa->resource_expand_limit, sa->dom_approx, sa->main_dom_kind,
    sa->cache, sa->cache_dom_kind, &cost);
  if( sa->daily_expand_limit == 0 && sa->daily_prune_trigger == 0 &&
      sa->resource_expand_limit == 0 && sa->dom_approx == 0 )
    KheOptimalResultSetMeldResult(vs->optimal_results, rs, is, cost, vs->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFileAndGraphBegin(KHE_VLSN_SOLVER vs, char *file_stem,           */
/*    char *key_label, char *below_caption, char *left_caption,              */
/*    KHE_FILE *file, KHE_GRAPH *graph, HA_ARENA a)                          */
/*                                                                           */
/*  Set *file to a new output file containing a new *graph with these        */
/*  attributes.                                                              */
/*                                                                           */
/*****************************************************************************/

static void KheFileAndGraphBegin(KHE_VLSN_SOLVER vs, char *file_stem,
  char *key_label, char *below_caption, char *left_caption,
  KHE_FILE *file, KHE_GRAPH *graph, HA_ARENA a)
{
  char *file_name;
  if( strlen(file_stem) == 0 )
    file_name = HnStringMake(a, "res_%s", left_caption);
  else
    file_name = HnStringMake(a, "%s_%s", file_stem, left_caption);
  *file = KheFileBegin(file_name, KHE_FILE_LOUT_STANDALONE);
  *graph = KheGraphBegin(*file);
  KheGraphSetBelowCaption(*graph, below_caption);
  KheGraphSetLeftCaptionAndGap(*graph, left_caption, "1.0c");
  KheGraphSetKeyLabel(*graph, key_label);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFileAndGraphEnd(KHE_FILE file, KHE_GRAPH graph)                  */
/*                                                                           */
/*  End the use of file and graph.                                           */
/*                                                                           */
/*****************************************************************************/

static void KheFileAndGraphEnd(KHE_FILE file, KHE_GRAPH graph)
{
  KheGraphEnd(graph);
  KheFileEnd(file);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheVLSNSolverDoTest(KHE_VLSN_SOLVER vs, int dim_index,              */
/*    KHE_SOLVE_ARGUMENTS sa, char *graph_name, char *key_label)             */
/*                                                                           */
/*  Carry out the actual work of KheVLSNSolverTest.                          */
/*                                                                           */
/*****************************************************************************/

static void KheVLSNSolverDoTest(KHE_VLSN_SOLVER vs, int dim_index,
  KHE_SOLVE_ARGUMENTS sa, char *graph_name, char *key_label)
{
  KHE_DIM dim, last_dim;  KHE_SOLVE_ARGUMENTS sub_sa, sub_sub_sa;
  KHE_DIM_POS dim_pos, last_dim_pos;  HA_ARRAY_NCHAR anc;
  char *sub_graph_name, *sub_graph_key_label, *data_set_name, *below_caption;
  KHE_DATASET_POINTS_TYPE points_type;
  KHE_DATASET made_kd, size_kd, time_kd;
  KHE_FILE made_file, size_file, time_file;
  KHE_GRAPH made_graph, size_graph, time_graph;
  int i, j, k, count, solns_made, table_size;
  int total_solns_made, total_table_size;  float running_time;
  if( dim_index == HaArrayCount(vs->test_dims) - 2 )
  {
    /* execute one set of graphs using sa */
    dim = HaArray(vs->test_dims, dim_index);
    last_dim = HaArray(vs->test_dims, dim_index + 1);

    /* start off the graphs */
    below_caption = KheDimDisplay(last_dim, vs->arena);
    KheFileAndGraphBegin(vs, graph_name, key_label, below_caption,
      "Made", &made_file, &made_graph, vs->arena);
    KheFileAndGraphBegin(vs, graph_name, key_label, below_caption,
      "Size", &size_file, &size_graph, vs->arena);
    KheFileAndGraphBegin(vs, graph_name, key_label, below_caption,
      "Secs", &time_file, &time_graph, vs->arena);

    /* one data set for each position along the second-last dimension */
    HaArrayForEach(dim->dim_poses, dim_pos, i)
    {
      points_type = (KHE_DATASET_POINTS_TYPE) (i % 10 + 1);
      sub_sa = KheSolveArgumentsCopyAndUpdate(sa, dim_pos, vs->arena);
      data_set_name = KheDimPosDisplay(dim_pos, vs->arena);
      made_kd = KheDataSetAdd(made_graph, points_type,
	KHE_DATASET_PAIRS_DASHED, data_set_name);
      size_kd = KheDataSetAdd(size_graph, points_type,
	KHE_DATASET_PAIRS_DASHED, data_set_name);
      time_kd = KheDataSetAdd(time_graph, points_type,
	KHE_DATASET_PAIRS_DASHED, data_set_name);

      if( last_dim->days_dim )
      {
	/* last dimension is Z - one point for each day of a single test */
	KheVLSNSolverDoOneTest(vs, sub_sa);
	count = KheDynamicResourceSolverSolveStatsCount(vs->solver);
	for( j = 0;  j < count;  j++ )
	{
          KheDynamicResourceSolverSolveStats(vs->solver, j, &solns_made,
	    &table_size, &running_time);
	  if( count <= 28 || j % 2 == 0 || j == count - 1 )
	  {
	    KhePointAdd(made_kd, (float) j, solns_made);
	    KhePointAdd(size_kd, (float) j, table_size);
	    KhePointAdd(time_kd, (float) j, running_time);
	  }
	}
      }
      else
      {
	/* last dimension is not Z - one test for each position */
	HaArrayForEach(last_dim->dim_poses, last_dim_pos, j)
	{
	  sub_sub_sa = KheSolveArgumentsCopyAndUpdate(sub_sa, last_dim_pos,
	    vs->arena);
	  KheVLSNSolverDoOneTest(vs, sub_sub_sa);
	  count = KheDynamicResourceSolverSolveStatsCount(vs->solver);
	  total_solns_made = 0;
	  total_table_size = 0;
	  running_time = 0;
	  for( k = 0;  k < count;  k++ )
	  {
	    KheDynamicResourceSolverSolveStats(vs->solver, k,
	      &solns_made, &table_size, &running_time);
            total_solns_made += solns_made;
            total_table_size += table_size;
	  }
	  KhePointAdd(made_kd, (float) j, total_solns_made);
	  KhePointAdd(size_kd, (float) j, total_table_size);
	  KhePointAdd(time_kd, (float) j, running_time);
	}
      }
    }

    /* finish off the graphs */
    KheFileAndGraphEnd(made_file, made_graph);
    KheFileAndGraphEnd(size_file, size_graph);
    KheFileAndGraphEnd(time_file, time_graph);
  }
  else
  {
    /* execute multiple graphs */
    dim = HaArray(vs->test_dims, dim_index);
    HaArrayForEach(dim->dim_poses, dim_pos, i)
    {
      /* make a graph_name for this position */
      if( HaArrayCount(dim->dim_poses) == 1 )
	sub_graph_name = graph_name;
      else if( strlen(graph_name) > 0 )
	sub_graph_name = HnStringMake(vs->arena, "%s.%d", graph_name, i+1);
      else
	sub_graph_name = HnStringMake(vs->arena, "%d", i + 1);

      /* make a key_label for this position */
      HnStringBegin(anc, vs->arena);
      if( strlen(key_label) > 0 )
	HnStringAdd(&anc, "%s|", key_label);
      KheDimPosAddDisplay(dim_pos, &anc);
      sub_graph_key_label = HnStringEnd(anc);

      /* make sub_sa for this position along dim */
      sub_sa = KheSolveArgumentsCopyAndUpdate(sa, dim_pos, vs->arena);

      /* and execute this position */
      KheVLSNSolverDoTest(vs, dim_index + 1, sub_sa, sub_graph_name,
	sub_graph_key_label);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheStrAdjust(char *str, HA_ARENA a)                                */
/*                                                                           */
/*  Return str, adjusted to remove any colons, which are not good in file    */
/*  names.                                                                   */
/*                                                                           */
/*****************************************************************************/

static char *KheStrAdjust(char *str, HA_ARENA a)
{
  HA_ARRAY_NCHAR anc;  char *p, ch;
  HnStringBegin(anc, a);
  for( p = str;  *p != '\0';  p++ )
  {
    ch = *p;
    if( ch == ':' )
      ch = '_';
    HnStringAdd(&anc, "%c", ch);
  }
  return HnStringEnd(anc);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheVLSNSolverTest(KHE_VLSN_SOLVER vs)                               */
/*                                                                           */
/*  Perform a test using vs.                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheVLSNSolverTest(KHE_VLSN_SOLVER vs, KHE_SOLVE_ARGUMENTS sa)
{
  char *rt_id, *file_stem;
  rt_id = KheResourceTypeId(vs->resource_type);
  file_stem = KheStrAdjust(rt_id, vs->arena);
  KheVLSNSolverDoTest(vs, 0, sa, file_stem, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheDynamicResourceVLSNSolve and KheDynamicResourceVLSNTest"   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheDynamicResourceVLSNSolve(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,    */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Carry out a VLSN search using calls to KheDynamicResourceSolverSolve.    */
/*                                                                           */
/*****************************************************************************/

bool KheDynamicResourceVLSNSolve(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options)
{
  float rs_drs_time_limit;  int rs_drs_solve_limit, rs_drs_fail_limit;
  char *str; KHE_COST init_cost;  KHE_TIMER timer;  KHE_VLSN_SOLVER vs;
  KHE_DYNAMIC_RESOURCE_SOLVER drs;  KHE_SOLVE_ARGUMENTS sa;

  /* quit immediately if the rs_drs_off option is true */
  if( KheOptionsGetBool(options, "rs_drs_off", false) )
    return false;

  if( DEBUG1 )
    fprintf(stderr, "[ KheDynamicResourceVLSNSolve(%s, %s, options)\n",
      KheInstanceId(KheSolnInstance(soln)), KheResourceTypeId(rt));

  /* get rs_drs_time_limit and set timer */
  rs_drs_time_limit = KheTimeFromString(
    KheOptionsGet(options, "rs_drs_time_limit", "-"));
  if( rs_drs_time_limit >= 0.0 )
    timer = KheOptionsAddTimer(options, "vlsn", rs_drs_time_limit);
  else
    timer = NULL;

  /* get rs_drs_solve_limit */
  str = KheOptionsGet(options, "rs_drs_solve_limit", "1000");
  if( strcmp(str, "-") == 0 )
    rs_drs_solve_limit = INT_MAX;
  else
    rs_drs_solve_limit = KheOptionsGetInt(options, "rs_drs_solve_limit", 1000);

  /* get rs_drs_fail_limit */
  str = KheOptionsGet(options, "rs_drs_fail_limit", "1000");
  if( strcmp(str, "-") == 0 )
    rs_drs_fail_limit = INT_MAX;
  else
    rs_drs_fail_limit = KheOptionsGetInt(options, "rs_drs_fail_limit", 1000);

  /* create a vs, carry out the solves, and delete the vs */
  init_cost = KheSolnCost(soln);
  drs = KheDynamicResourceSolverMake(soln, rt, options);
  if( drs != NULL )
  {
    vs = KheVLSNSolverBegin(drs, soln, rt, options);
    sa = KheSolveArgumentsGet(vs);
    KheVLSNSolverSolve(vs, sa, rs_drs_solve_limit, rs_drs_fail_limit);
    KheVLSNSolverEnd(vs);
  }

  /* delete the timer */
  if( timer != NULL )
    KheOptionsDeleteTimer(options, timer);

  /* all done, return */
  if( KheSolnCost(soln) < init_cost )
  {
    if( DEBUG1 )
      fprintf(stderr,
	"] KheDynamicResourceVLSNSolve returning true (%.5f < %.5f)\n",
	KheCostShow(KheSolnCost(soln)), KheCostShow(init_cost));
    return true;
  }
  else
  {
    if( DEBUG1 )
     fprintf(stderr, "] KheDynamicResourceVLSNSolve returning false%s\n",
	drs == NULL ? " (no drs)" : "");
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDynamicResourceVLSNTest(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,     */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Test dynamic resource solving and produce graphs of the results, as      */
/*  documented in the User's Guide.                                          */
/*                                                                           */
/*****************************************************************************/

void KheDynamicResourceVLSNTest(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options)
{
  KHE_VLSN_SOLVER vs;  char *rs_drs_test;
  KHE_DYNAMIC_RESOURCE_SOLVER drs;  KHE_SOLVE_ARGUMENTS sa;

  /* get rs_drs_test option and quit immediately if it is "none" */
  rs_drs_test = KheOptionsGet(options, "rs_drs_test", "none");
  if( strcmp(rs_drs_test, "none") == 0 )
    return;

  /* debug */
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheDynamicResourceVLSNTest(%s, %s, options)\n",
      KheInstanceId(KheSolnInstance(soln)), KheResourceTypeId(rt));
    fprintf(stderr, "  %s\n", rs_drs_test);
  }

  /* make a vs, parse a test, run that test, and end */
  drs = KheDynamicResourceSolverMake(soln, rt, options);
  if( drs != NULL )
  {
    vs = KheVLSNSolverBegin(drs, soln, rt, options);
    KheVLSNSolverParseTest(&rs_drs_test, vs);
    sa = KheSolveArgumentsGet(vs);
    KheVLSNSolverTest(vs, sa);
    KheVLSNSolverEnd(vs);
  }

  if( DEBUG2 )
    fprintf(stderr, "] KheDynamicResourceVLSNTest\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MONITOR_ADJUSTER"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_ADJUSTER KheAdjustEventResourceMonitorsBegin(KHE_SOLN soln,     */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Begin a stretch of code within which the event resource monitors of      */
/*  soln are adjusted.                                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_SOLN_ADJUSTER KheAdjustEventResourceMonitorsBegin(KHE_SOLN soln,
  HA_ARENA a)
{
  KHE_MONITOR m;  int i;  KHE_SOLN_ADJUSTER sa;  KHE_COST adjusted_weight;
  sa = KheSolnAdjusterMake(soln, a);
  for( i = 0;  i < KheSolnMonitorCount(soln);  i++ )
  {
    m = KheSolnMonitor(soln, i);
    switch( KheMonitorTag(m) )
    {
      case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
      case KHE_PREFER_RESOURCES_MONITOR_TAG:
      case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
      case KHE_LIMIT_RESOURCES_MONITOR_TAG:

	if( KheMonitorCombinedWeight(m) > 0 )
	{
	  if( KheMonitorCombinedWeight(m) >= KheCost(1, 0) )
	    adjusted_weight = KheCost(0, 2);
	  else
	    adjusted_weight = KheCost(0, 1);
	  if( DEBUG16 )
	  {
	    /* don't adjust PreferResourcesRedundancy monitors */
	    if( KheMonitorConstraint(m) != NULL )
	      KheSolnAdjusterMonitorSetCombinedWeight(sa, m, adjusted_weight);
	  }
	  else
	    KheSolnAdjusterMonitorSetCombinedWeight(sa, m, adjusted_weight);
	}
	break;

      default:

	/* ignore other kinds */
	break;
    }
  }
  return sa;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAdjustEventResourceMonitorsEnd(KHE_SOLN_ADJUSTER sa)             */
/*                                                                           */
/*  End a stretch of code within which the event resource monitors of        */
/*  soln are adjusted.                                                       */
/*                                                                           */
/*****************************************************************************/

static void KheAdjustEventResourceMonitorsEnd(KHE_SOLN_ADJUSTER sa)
{
  KheSolnAdjusterUndo(sa);
  /* KheSolnAdjusterDelete(sa); */
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheDynamicResourceSequentialSolve"                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableFix(KHE_RESOURCE_TIMETABLE_MONITOR rtm)         */
/*                                                                           */
/*  Fix the unfixed assignments of rtm.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheResourceTimetableFix(KHE_RESOURCE_TIMETABLE_MONITOR rtm)
{
  KHE_TASK_SET ts;  KHE_SOLN soln;  KHE_TIME_GROUP full_tg;
  int i;  KHE_TASK task;

  /* get the tasks of rtm into ts */
  soln = KheResourceTimetableMonitorSoln(rtm);
  full_tg = KheInstanceFullTimeGroup(KheSolnInstance(soln));
  ts = KheTaskSetMake(soln);
  KheResourceTimetableMonitorAddProperRootTasksInTimeGroup(rtm, full_tg,
    true, ts);

  /* fix those tasks then free ts */
  for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
  {
    task = KheTaskSetTask(ts, i);
    if( !KheTaskAssignIsFixed(task) )
      KheTaskAssignFix(task);
  }
  KheTaskSetDelete(ts);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetTimetableFix(KHE_RESOURCE_SET rs, KHE_SOLN soln)      */
/*                                                                           */
/*  Fix the timetables of the resources of rs.                               */
/*                                                                           */
/*****************************************************************************/

static void KheResourceSetTimetableFix(KHE_RESOURCE_SET rs, KHE_SOLN soln)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_RESOURCE r;  int i;
  for( i = 0;  i < KheResourceSetResourceCount(rs);  i++ )
  {
    r = KheResourceSetResource(rs, i);
    rtm = KheResourceTimetableMonitor(soln, r);
    KheResourceTimetableFix(rtm);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceInitBusyDays(KHE_RESOURCE r, KHE_VLSN_SOLVER vs)          */
/*                                                                           */
/*  Return the number of initial busy days of r.                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheResourceInitBusyDays(KHE_RESOURCE r, KHE_VLSN_SOLVER vs)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  int i;  KHE_TIME_GROUP tg;
  rtm = KheResourceTimetableMonitor(vs->soln, r);
  for( i = 0;  i < KheFrameTimeGroupCount(vs->days_frame);  i++ )
  {
    tg = KheFrameTimeGroup(vs->days_frame, i);
    if( KheResourceTimetableMonitorFreeForTimeGroup(rtm, tg, NULL, NULL, NULL) )
      break;
  }
  return i;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceSetInitBusyDays(KHE_RESOURCE_SET rset, KHE_VLSN_SOLVER vs)*/
/*                                                                           */
/*  Return the minimum number of initial days that the resources of rset     */
/*  are busy.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheResourceSetInitBusyDays(KHE_RESOURCE_SET rset, KHE_VLSN_SOLVER vs)
{
  int res, i, init_busy_days;  KHE_RESOURCE r;
  res = INT_MAX;
  for( i = 0;  i < KheResourceSetResourceCount(rset);  i++ )
  {
    r = KheResourceSetResource(rset, i);
    init_busy_days = KheResourceInitBusyDays(r, vs);
    if( init_busy_days < res )
      res = init_busy_days;
  }
  HnAssert(res < INT_MAX, "KheResourceSetInitBusyDays internal error");
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheLongTaskAsstDebug(KHE_SOLN soln, int min_durn,                   */
/*    bool assigned, int verbosity, int indent, FILE *fp)                    */
/*                                                                           */
/*  Print a list of tasks of total duration min_durn or greater that are     */
/*  either assigned a resource or not assigned a resource.                   */
/*                                                                           */
/*****************************************************************************/

static void KheLongTaskAsstDebug(KHE_SOLN soln, int min_durn,
  bool assigned, int verbosity, int indent, FILE *fp)
{
  KHE_RESOURCE r;  int i;  KHE_TASK task;
  fprintf(fp, "%*s[ all %s tasks of duration >= %d:\n", indent, "",
    assigned ? "assigned" : "unassigned", min_durn);
  for( i = 0;  i < KheSolnTaskCount(soln);  i++ )
  {
    task = KheSolnTask(soln, i);
    r = KheTaskAsstResource(task);
    if( KheTaskIsProperRoot(task) && (r != NULL) == assigned &&
	KheTaskTotalDuration(task) >= min_durn )
    {
      fprintf(fp, "%*s", indent + 2, "");
      if( r != NULL )
	fprintf(fp, "%s: ", KheResourceId(r));
      KheTaskDebug(task, verbosity, 0, fp);
    }
  }
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDynamicResourceSequentialSolve(KHE_SOLN soln,                    */
/*    KHE_RESOURCE_TYPE rt, KHE_OPTIONS options)                             */
/*                                                                           */
/*  A particular call to KheDynamicResourceVLSNSolve with its own options.   */
/*                                                                           */
/*****************************************************************************/

bool KheDynamicResourceSequentialSolve(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options)
{
  int seq_resources, seq_days, i, j, k, rcount; /* , init_busy_days; */
  bool res, rs_drs_fix;  HA_ARENA a;
  KHE_DYNAMIC_RESOURCE_SOLVER drs;  float frac;  KHE_VLSN_SOLVER vs;
  KHE_SOLN_ADJUSTER sna;  KHE_RESOURCE_SELECT rs;  KHE_DAY_SELECT ds;
  KHE_RESOURCE_SET rset;  KHE_INTERVAL_SET iset;  KHE_SOLVE_ARGUMENTS sa;
  KHE_COST init_cost, final_cost;  KHE_RESOURCE_GROUP leftover_rg;
  KHE_RESOURCE r;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;

  /* debug */
  if( DEBUG8 )
    fprintf(stderr, "[ KheDynamicResourceSequentialSolve(%s, %s)\n",
      KheInstanceId(KheSolnInstance(soln)), KheResourceTypeId(rt));

  /* adjust event resource monitor costs */
  init_cost = KheSolnCost(soln);
  a = KheSolnArenaBegin(soln);

  /* get khe_drs_seq_frac and convert it into a number of resources */
  frac = KheOptionsGetFloat(options, "rs_drs_seq_frac", 0.5);
  if( frac >= 1.0 )
    rcount = KheResourceTypeResourceCount(rt);
  else if( frac <= 0.0 )
    rcount = 0;
  else
    rcount = (int) ((float) KheResourceTypeResourceCount(rt) * frac);

  /* get rs_drs_fix */
  rs_drs_fix = KheOptionsGetBool(options, "rs_drs_fix", false);

  /* debug */
  if( DEBUG8 )
    fprintf(stderr, "  frac %.1f, rt_count %d, rcount %d\n",
      frac, KheResourceTypeResourceCount(rt), rcount);

  /* do by sequential solve whatever we want to do and can do */
  if( rcount == 0 )
  {
    /* no resources for sequential solve, leave all resources for time sweep */
    leftover_rg = KheResourceTypeFullResourceGroup(rt);
  }
  else
  {
    /* at least one resource (possibly all) for sequential solve */
    sna = KheAdjustEventResourceMonitorsBegin(soln, a);
    drs = KheDynamicResourceSolverMake(soln, rt, options);
    if( drs == NULL )
    {
      /* can't build a drs solver, leave all resources for time sweep */
      leftover_rg = KheResourceTypeFullResourceGroup(rt);
      rcount = 0;  /* used by debugging below */
    }
    else
    {
      /* get a resource select based on the rs_drs_seq_resources option */
      /* also set leftover_rg, to NULL if nothing left for time sweep */
      seq_resources = KheOptionsGetInt(options, "rs_drs_seq_resources", 1);
      vs = KheVLSNSolverBegin(drs, soln, rt, options);
      rs = (KHE_RESOURCE_SELECT) KheResourceSelectAllMake(vs,
	seq_resources, rcount, &leftover_rg);

      /* get a day select based on the rs_drs_seq_days option */
      seq_days = KheOptionsGetInt(options, "rs_drs_seq_days", 28);
      /* make the day select, omitting initial busy days */
      /* init_busy_days = KheResourceSetInitBusyDays(rset, vs); */
      ds = (KHE_DAY_SELECT) KheDaySelectAllMake(0, seq_days, seq_days, vs);

      if( DEBUG8 )
	fprintf(stderr,  "  seq_resources %d, seq_days %d\n",
	  seq_resources, seq_days);

      /* get solve arguments */
      sa = KheSolveArgumentsGet(vs);

      /* solve for each resource set and each day set, then delete drs */
      KheResourceSelectForEachResourceSet(rs, rset, i)
      {
	/* solve rset for the day select */
	KheDaySelectForEachIntervalSet(ds, iset, j)
	{
	  KheDrsAddResources(drs, rset);
	  KheDrsAddDays(drs, iset);
	  if( DEBUG15 )
	  {
	    fprintf(stderr, "[ KheDynamicResourceSequentialSolve solving:\n");
	    KheGroupMonitorDebug((KHE_GROUP_MONITOR) soln, 2, 2, stderr);
	  }
	  if( DEBUG8 || DEBUG15 )
	  {
	    fprintf(stderr, "  solving resource%s ",
	      KheResourceSetResourceCount(rset) == 1 ? "" : "s");
	    KheResourceSetDebug(rset, 1, -1, stderr);
	    fprintf(stderr, " on days ");
	    KheIntervalSetDebug(iset, stderr);
	    fprintf(stderr, "\n");
	  }
	  KheDynamicResourceSolverSolve(drs, sa->priqueue,
	    sa->extra_selection, sa->expand_by_shifts, sa->shift_pairs,
	    sa->correlated_exprs, /* sa->daily_expand_limit, */ 500,
	    sa->daily_prune_trigger, sa->resource_expand_limit, sa->dom_approx,
	    sa->main_dom_kind, sa->cache, sa->cache_dom_kind);
	  if( DEBUG15 )
	  {
	    KheGroupMonitorDebug((KHE_GROUP_MONITOR) soln, 2, 2, stderr);
	    fprintf(stderr, "]\n");
	  }
	  if( DEBUG8 )
	  {
	    for( k = 0;  k < KheResourceSetResourceCount(rset);  k++ )
	    {
	      r = KheResourceSetResource(rset, k);
	      rtm = KheResourceTimetableMonitor(soln, r);
	      KheResourceTimetableMonitorPrintTimetable(rtm, vs->days_frame,
		10, 2, stderr);
	      fprintf(stderr, "\n");
	    }
	  }
	}

	/* optionally fix rset */
	if( rs_drs_fix )
	  KheResourceSetTimetableFix(rset, vs->soln);
      }
      KheVLSNSolverEnd(vs);
    }
    KheAdjustEventResourceMonitorsEnd(sna);
  }

  if( DEBUG18 )
  {
    fprintf(stderr, "  [ after sequential and before time sweep:\n");
    KheLongTaskAsstDebug(soln, 3, true, 2, 4, stderr);
    KheLongTaskAsstDebug(soln, 3, false, 2, 4, stderr);
    fprintf(stderr, "  ]\n");
  }

  /* do the leftovers (if any) by time sweep */
  if( leftover_rg != NULL )
    KheTimeSweepAssignResources(soln, leftover_rg, options);

  if( DEBUG18 )
  {
    fprintf(stderr, "  [ after time sweep:\n");
    KheLongTaskAsstDebug(soln, 3, true, 2, 4, stderr);
    KheLongTaskAsstDebug(soln, 3, false, 2, 4, stderr);
    fprintf(stderr, "  ]\n");
  }

  /* wrapup */
  KheSolnArenaEnd(soln, a);
  final_cost = KheSolnCost(soln);
  res = (final_cost < init_cost);
  if( DEBUG8 )
  {
    fprintf(stderr, "  assigned %d resources (%d sequential, %d time sweep)\n",
      KheResourceTypeResourceCount(rt), rcount,
      KheResourceTypeResourceCount(rt) - rcount);
    fprintf(stderr, "] KheDynamicResourceSequentialSolve returning %s "
      "(init %.5f, final %.5f)\n", bool_show(res), KheCostShow(init_cost),
      KheCostShow(final_cost));
  }
  return res;
}
